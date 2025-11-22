#pragma once
#include <cstdint>
#include <functional>
#include <random>

namespace opn
{
    struct UUID
    {
        uint64_t high = 0;
        uint64_t low = 0;

        constexpr UUID() = default;

        [[nodiscard]] bool operator==(const UUID& _other) const noexcept
        {
            return high == _other.high && low == _other.low;
        }

        [[nodiscard]] bool operator!=(const UUID& _other) const noexcept
        {
            return !(*this == _other);
        }

        [[nodiscard]] bool operator<(const UUID& _other) const noexcept
        {
            if (high != _other.high) return high < _other.high;
            return (low < _other.low);
        }
    };

    struct UUIDHasher
    {
        std::size_t operator()(const UUID& _uuid) const noexcept
        {
            return std::hash<uint64_t>{}(_uuid.high) ^ (std::hash<uint64_t>{}(_uuid.low) << 1);
        }
    };

    inline UUID generateUUID() noexcept
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint64_t> dist;

        UUID uuid;
        uuid.high = dist(gen);
        uuid.low = dist(gen);

        uuid.high &= 0xFFFFFFFFFFFF0FFFULL;
        uuid.high |= 0x0000000000004000ULL;

        uuid.low &= 0x3FFFFFFFFFFFFFFFULL;
        uuid.low |= 0x8000000000000000ULL;

        return uuid;
    };
}
