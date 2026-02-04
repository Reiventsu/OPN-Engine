export module opn.Assets.Types;
import opn.System.UUID;

export namespace opn {
    struct sAssetHandle {
        UUID id;
        constexpr sAssetHandle() : id{} {}
        explicit constexpr sAssetHandle(UUID _id) : id{_id} {}

        [[nodiscard]] bool isValid() const { return id.isValid(); };
        bool operator==(const sAssetHandle&) const = default;
    };

    struct sAssetHandleHasher {
        size_t operator()(const sAssetHandle& _handle) const {
            return UUIDHasher{}(_handle.id);
        }
    };
}