#pragma once

#include <random>
#include <vector>

#include <Common/Math/Vector.h>
#include <Common/Math/Matrix.h>
#include <Common/Math/Quaternion.h>

namespace Common::MathBenchmark {
    constexpr int batchSize = 1024;

    static std::vector<float> MakeRandomFloats(const size_t count)
    {
        std::mt19937 rng(0x1234u);
        std::uniform_real_distribution<float> dist(0.5f, 1.5f);
        std::vector<float> values(count);
        for (auto& value : values) {
            value = dist(rng);
        }
        return values;
    }

    template <MathBackend B>
    static std::vector<Vec<float, 4, B>> MakeRandomVecs(const size_t count)
    {
        const auto raw = MakeRandomFloats(count * 4);
        std::vector<Vec<float, 4, B>> result(count);
        for (size_t i = 0; i < count; i++) {
            result[i] = Vec<float, 4, B>(raw[i * 4 + 0], raw[i * 4 + 1], raw[i * 4 + 2], raw[i * 4 + 3]);
        }
        return result;
    }

    template <MathBackend B>
    static std::vector<Mat<float, 4, 4, B>> MakeRandomMats(const size_t count)
    {
        const auto raw = MakeRandomFloats(count * 16);
        std::vector<Mat<float, 4, 4, B>> result(count);
        for (size_t i = 0; i < count; i++) {
            const float* p = &raw[i * 16];
            result[i] = Mat<float, 4, 4, B>(
                p[0], p[1], p[2], p[3],
                p[4], p[5], p[6], p[7],
                p[8], p[9], p[10], p[11],
                p[12], p[13], p[14], p[15]);
        }
        return result;
    }

    template <MathBackend B>
    static std::vector<Quaternion<float, B>> MakeRandomQuats(const size_t count)
    {
        const auto raw = MakeRandomFloats(count * 4);
        std::vector<Quaternion<float, B>> result(count);
        for (size_t i = 0; i < count; i++) {
            result[i] = Quaternion<float, B>(raw[i * 4 + 0], raw[i * 4 + 1], raw[i * 4 + 2], raw[i * 4 + 3]);
        }
        return result;
    }
}
