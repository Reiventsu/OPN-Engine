module;
#include <cstdint>
#include <functional>
export module opn.ECS:tEntity;

export namespace opn {
    struct tEntity {
        uint32_t id = 0xFFFFFFFF;

        static constexpr uint32_t INDEX_MASK = 0x000FFFFF;
        static constexpr uint32_t GEN_MASK   = 0xFFF00000;
        static constexpr uint32_t GEN_SHIFT  = 20;

        [[nodiscard]] uint32_t index() const noexcept {
            return id & INDEX_MASK;
        }
        [[nodiscard]] uint32_t generation() const noexcept {
            return (id & GEN_MASK) >> GEN_SHIFT;
        }

        static tEntity make(const uint32_t _index, const uint32_t _generation) noexcept {
            return { _index | (_generation << GEN_SHIFT) };
        }

        bool operator==(const tEntity &other) const noexcept = default;
        [[nodiscard]] bool is_null() const noexcept { return id == 0xFFFFFFFF; }
    };

    constexpr tEntity NULL_ENTITY = { 0xFFFFFFFF };
}
