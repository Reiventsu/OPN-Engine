module;
#include <cstdint>
export module opn.System.Jobs.Handle;
import opn.System.Jobs.Types;

export namespace opn {
    struct sJobHandle {
        uint32_t fenceID{};
    };
}