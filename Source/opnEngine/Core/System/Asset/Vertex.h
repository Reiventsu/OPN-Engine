#pragma once
#include <array>
#include <cmath>
#include <tuple>

namespace opn::AssetData
{
    constexpr float EPSILON = 1e-6f;

    struct sVertex
    {
        std::array<float, 3> position;
        std::array<float, 3> normal;
        std::array<float, 2> uv;
        std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};

        template <size_t size>
        static int compareFloatArrays(const std::array<float, size>& _a, const std::array<float, size>& _b) noexcept
        {
            for (size_t i = 0; i < size; ++i)
            {
                if (std::abs(_a[i] - _b[i]) < EPSILON) continue;
                if (_a[i] < _b[i]) return -1;
                else return 1;
            }
            return 0;
        }

        bool operator==(const sVertex& _other) const noexcept
        {
            return compareFloatArrays(position, _other.position) == 0 &&
                compareFloatArrays(normal, _other.normal) == 0 &&
                compareFloatArrays(uv, _other.uv) == 0 &&
                compareFloatArrays(color, _other.color) == 0;
        }

        bool operator<(const sVertex& _other) const noexcept
        {
            if (const int pos_cmp = compareFloatArrays(position, _other.position); pos_cmp != 0) return pos_cmp < 0;
            if (const int norm_cmp = compareFloatArrays(normal, _other.normal); norm_cmp != 0) return norm_cmp < 0;
            if (const int uv_cmp = compareFloatArrays(uv, _other.uv); uv_cmp != 0) return uv_cmp < 0;
            return compareFloatArrays(color, _other.color) < 0;
        }
    };

    inline size_t getVertexSize() noexcept
    {
        return sizeof(sVertex);
    }
}
