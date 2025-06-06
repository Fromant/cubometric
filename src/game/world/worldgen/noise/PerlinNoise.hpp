#pragma once
#include <algorithm>
#include <array>
#include <functional>
#include <random>

#include "Interpolator.hpp"

typedef std::function<float(float, float, float)> Interpolator;

const static Interpolator LinearInterpolator = [](float a, float b, float t) {
    return a + t * (b - a);
};

const static Interpolator CosineInterpolator = [](float a, float b, float t) {
    float t2 = (1 - std::cos(t * M_PI)) / 2;
    return a * (1 - t2) + b * t2;
};


const static Interpolator CubicInterpolator = [](float a, float b, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return a * (2 * t3 - 3 * t2 + 1) + b * (3 * t2 - 2 * t3);
};

const static std::array<Interpolator, 3> interpolators = {
        LinearInterpolator,
        CosineInterpolator,
        CubicInterpolator
    };

template <int interpolator_id = 0>
class PerlinNoise {
    Interpolator interpolator = interpolators[interpolator_id];

public:
    explicit PerlinNoise(unsigned seed = 0) {
        reseed(seed);
    }

    void reseed(unsigned seed) {
        // Initialize permutation table
        std::array<int, 256> perm;
        for (int i = 0; i < 256; i++) perm[i] = i;

        // Shuffle using platform-independent RNG
        std::mt19937 engine(seed);
        std::ranges::shuffle(perm, engine);

        // Duplicate permutation table
        for (int i = 0; i < 512; i++) {
            p[i] = perm[i & 255];
        }
    }

    float noise(float x, float y, float z,
                int octaves = 4,
                float persistence = 0.5f,
                float lacunarity = 2.0f) const {
        float total = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float maxValue = 0.0f; // Used for normalization

        for (int i = 0; i < octaves; i++) {
            total += noiseSingle(x * frequency, y * frequency, z * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return total / maxValue;
    }

private:
    std::array<int, 512> p;

    // Fade function as defined by Ken Perlin
    static float fade(float t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    // Linear interpolation
    float lerp(float a, float b, float t) const {
        return interpolator(a, b, t);
    }

    // Gradient function calculates dot product between a pseudorandom gradient vector and the vector from input coordinate to the grid point
    static float grad(int hash, float x, float y, float z) {
        int h = hash & 15; // Take the hashed value and take the first 4 bits of it
        float u = h < 8 ? x : y; // If h<8, u=x. Else u=y.
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : z); // v depends on h

        float g1 = (h & 1) ? u : -u;
        float g2 = (h & 2) ? v : -v;

        return g1 + g2;
    }

    // Compute the noise at a single point
    float noiseSingle(float x, float y, float z) const {
        // Find unit cube that contains point
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;

        // Find relative x,y,z of point in cube
        float xf = x - std::floor(x);
        float yf = y - std::floor(y);
        float zf = z - std::floor(z);

        // Compute fade curves for each of xf,yf,zf
        float u = fade(xf);
        float v = fade(yf);
        float w = fade(zf);

        const int X0 = X & 255;
        const int Y0 = Y & 255;
        const int Z0 = Z & 255;

        const int X1 = (X0 + 1) & 255;
        const int Y1 = (Y0 + 1) & 255;
        const int Z1 = (Z0 + 1) & 255;

        // Hash corners
        int aaa = p[(p[(p[X0]) ^ Y0]) ^ Z0];
        int aba = p[(p[(p[X0]) ^ Y1]) ^ Z0];
        int aab = p[(p[(p[X0]) ^ Y0]) ^ Z1];
        int abb = p[(p[(p[X0]) ^ Y1]) ^ Z1];

        int baa = p[(p[(p[X1]) ^ Y0]) ^ Z0];
        int bba = p[(p[(p[X1]) ^ Y1]) ^ Z0];
        int bab = p[(p[(p[X1]) ^ Y0]) ^ Z1];
        int bbb = p[(p[(p[X1]) ^ Y1]) ^ Z1];

        // Calculate gradient contributions from each corner
        float g000 = grad(aaa, xf, yf, zf);
        float g001 = grad(aab, xf, yf, zf - 1);
        float g010 = grad(aba, xf, yf - 1, zf);
        float g011 = grad(abb, xf, yf - 1, zf - 1);
        float g100 = grad(baa, xf - 1, yf, zf);
        float g101 = grad(bab, xf - 1, yf, zf - 1);
        float g110 = grad(bba, xf - 1, yf - 1, zf);
        float g111 = grad(bbb, xf - 1, yf - 1, zf - 1);

        // Interpolate along x
        float lerp_x00 = lerp(g000, g100, u);
        float lerp_x01 = lerp(g001, g101, u);
        float lerp_x10 = lerp(g010, g110, u);
        float lerp_x11 = lerp(g011, g111, u);

        // Interpolate along y
        float lerp_y0 = lerp(lerp_x00, lerp_x10, v);
        float lerp_y1 = lerp(lerp_x01, lerp_x11, v);

        // Interpolate along z
        return lerp(lerp_y0, lerp_y1, w);
    }
};
