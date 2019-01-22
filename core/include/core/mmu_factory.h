#pragma once

#include "core/imembank.h"
#include "core/immu.h"

#include <memory>

namespace n_e_s::core {

class MmuFactory {
public:
    // A normal, reasonable mmu.
    static std::unique_ptr<IMmu> create(MemBankList mem_banks);

    // A mmu that prints everything read and written via it.
    static std::unique_ptr<IMmu> create_debug(MemBankList mem_banks);
};

} // namespace n_e_s::core
