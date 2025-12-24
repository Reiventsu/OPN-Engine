module;

#include <cstdint>
#include <functional>
#include <random>

export module opn.system.UUID;

export namespace opn {
    struct UUID {
        uint64_t high = 0;
        uint64_t low = 0;

        constexpr UUID() = default;
        
        [[nodiscard]] constexpr bool isValid() const noexcept {
            return high != 0 || low != 0;
        }

        [[nodiscard]] constexpr auto operator<=>(const UUID &) const noexcept = default;
    };

    struct UUIDHasher {
        std::size_t operator()(const UUID &_uuid) const noexcept {
            const size_t h = std::hash<uint64_t>{}(_uuid.high);
            return h ^ (std::hash<uint64_t>{}(_uuid.low) + 0x9e3779b9 + (h << 6) + (h >> 2));
        }
    };

    inline UUID generateUUID() noexcept {
        static thread_local std::random_device rd;
        static thread_local std::mt19937_64 gen(rd()); 
        static std::uniform_int_distribution<uint64_t> dist;

        UUID uuid;
        uuid.high = dist(gen);
        uuid.low = dist(gen);

        uuid.high &= 0xFFFFFFFFFFFF0FFFULL;
        uuid.high |= 0x0000000000004000ULL;

        uuid.low &= 0x3FFFFFFFFFFFFFFFULL;
        uuid.low |= 0x8000000000000000ULL;

        return uuid;
    }
}
