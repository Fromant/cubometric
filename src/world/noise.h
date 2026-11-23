#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <algorithm>

namespace ih::world {

// Ken Perlin's improved noise, ported 1:1 from the GL build (same permutation
// shuffle and gradient hash, so a given seed reproduces the same terrain).
// Interpolator: 0 = linear, 1 = cosine, 2 = cubic (the GL build used cubic).
template <int Interpolator = 2>
class PerlinNoise {
public:
    explicit PerlinNoise (unsigned seed = 0) { reseed (seed); }

    void reseed (unsigned seed) {
        std::array<int, 256> perm{};
        for (int i = 0; i < 256; ++i) perm[i] = i;
        std::mt19937 engine (seed);
        std::ranges::shuffle (perm, engine);
        for (int i = 0; i < 512; ++i) m_perm[i] = perm[i & 255];
    }

    float noise (float x, float y, float z, int octaves = 4, float persistence = 0.5f,
                 float lacunarity = 2.0f) const {
        float total = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float maxValue = 0.0f;
        for (int i = 0; i < octaves; ++i) {
            total += single (x * frequency, y * frequency, z * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }
        return total / maxValue;
    }

private:
    std::array<int, 512> m_perm{};

    static float fade (float t) { return t * t * t * (t * (t * 6 - 15) + 10); }

    static float lerp (float a, float b, float t) {
        if constexpr (Interpolator == 1) {
            const float t2 = (1 - std::cos (t * 3.14111111111111323846f)) / 2;
            return a * (1 - t2) + b * t2;
        } else if constexpr (Interpolator == 2) {
            const float t2 = t * t;
            const float t3 = t2 * t;
            return a * (2 * t3 - 3 * t2 + 1) + b * (3 * t2 - 2 * t3);
        } else {
            return a + t * (b - a);
        }
    }

    static float grad (int hash, float x, float y, float z) {
        const int h = hash & 15;
        const float u = h < 8 ? x : y;
        const float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) ? u : -u) + ((h & 2) ? v : -v);
    }

    float single (float x, float y, float z) const {
        const int X = static_cast<int> (std::floor (x)) & 255;
        const int Y = static_cast<int> (std::floor (y)) & 255;
        const int Z = static_cast<int> (std::floor (z)) & 255;
        const float xf = x - std::floor (x);
        const float yf = y - std::floor (y);
        const float zf = z - std::floor (z);
        const float u = fade (xf);
        const float v = fade (yf);
        const float w = fade (zf);

        const int X1 = (X + 1) & 255;
        const int Y1 = (Y + 1) & 255;
        const int Z1 = (Z + 1) & 255;

        const int aaa = m_perm[m_perm[m_perm[X] ^ Y] ^ Z];
        const int aba = m_perm[m_perm[m_perm[X] ^ Y1] ^ Z];
        const int aab = m_perm[m_perm[m_perm[X] ^ Y] ^ Z1];
        const int abb = m_perm[m_perm[m_perm[X] ^ Y1] ^ Z1];
        const int baa = m_perm[m_perm[m_perm[X1] ^ Y] ^ Z];
        const int bba = m_perm[m_perm[m_perm[X1] ^ Y1] ^ Z];
        const int bab = m_perm[m_perm[m_perm[X1] ^ Y] ^ Z1];
        const int bbb = m_perm[m_perm[m_perm[X1] ^ Y1] ^ Z1];

        const float x00 = lerp (grad (aaa, xf, yf, zf), grad (baa, xf - 1, yf, zf), u);
        const float x01 = lerp (grad (aab, xf, yf, zf - 1), grad (bab, xf - 1, yf, zf - 1), u);
        const float x10 = lerp (grad (aba, xf, yf - 1, zf), grad (bba, xf - 1, yf - 1, zf), u);
        const float x11 = lerp (grad (abb, xf, yf - 1, zf - 1), grad (bbb, xf - 1, yf - 1, zf - 1), u);
        return lerp (lerp (x00, x10, v), lerp (x01, x11, v), w);
    }
};

}  // namespace ih::world
