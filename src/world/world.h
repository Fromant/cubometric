#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "core/config.h"
#include "core/queue.h"
#include "world/chunk.h"
#include "world/mesher.h"
#include "world/worldgen.h"

namespace ih::world {

// Per-chunk lifecycle. Exactly one owner per state:
//   Empty      -> scheduler (render thread) pushes a generate task
//   Generating -> one worker writes block data
//   Generated  -> data is immutable; mesh task queued
//   Meshing    -> one worker reads data + neighbors, writes mesh
//   Ready      -> render thread ingests the mesh; remesh goes via Meshing
//   Dead       -> scheduler retired it; erased when no worker holds a ref
enum class ChunkState : uint8_t { Empty, Generating, Generated, Meshing, Ready, Dead };

struct Chunk {
    explicit Chunk (const core::ChunkDims& dims) : data (dims) {}

    std::atomic<ChunkState> state{ChunkState::Empty};
    ChunkData data;
    std::vector<MeshData> mesh{};
    int refs = 0;  // workers reading this chunk; guarded by World's map mutex
};

// Threaded chunk pipeline: generate -> mesh -> ready-for-upload.
// The render thread drives desired-set updates and drains finished meshes;
// workers never touch Vulkan.
class World {
public:
    World (const core::Config& config, const FaceLayers& faceLayers);
    ~World ();

    void startWorkers ();
    void stopWorkers ();

    // Render thread: maintain the desired chunk set around the camera.
    void updateDesired (ChunkCoord center, int radius);
    // Render thread: take one finished mesh for GPU upload.
    bool takeReadyMesh (ChunkCoord& coord, std::vector<MeshData>& out);
    // Render thread: retire and free chunks far outside the desired set.
    void collectDead (ChunkCoord center, int radius);

    size_t readyCount () const { return m_readyCount.load (std::memory_order_relaxed); }

private:
    void workerLoop ();
    void generateTask (ChunkCoord coord);
    void meshTask (ChunkCoord coord);
    void requestMesh (Chunk& chunk, ChunkCoord coord);

    const core::Config& m_config;
    const FaceLayers& m_faceLayers;
    WorldGenerator m_generator;

    std::mutex m_mapMutex;
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> m_chunks{};

    core::BoundedQueue<ChunkCoord> m_genQueue{4096};
    core::BoundedQueue<ChunkCoord> m_meshQueue{4096};
    core::BoundedQueue<ChunkCoord> m_readyQueue{4096};
    std::atomic<size_t> m_readyCount{0};

    std::vector<std::thread> m_workers{};
    std::atomic<bool> m_running{false};
};

}  // namespace ih::world
