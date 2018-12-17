#pragma once

#include "core/ippu.h"

namespace n_e_s::core {

class Ppu : public IPpu {
public:
    Ppu() = default;

    void execute() override;
};

} // namespace n_e_s::core
