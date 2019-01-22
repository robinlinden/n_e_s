#pragma once

#include "mmu.h"

#include <cstdint>

namespace n_e_s::core {

class DebugMmu : public Mmu {
public:
    uint8_t read_byte(uint16_t addr) const override;
    uint16_t read_word(uint16_t addr) const override;

    void write_byte(uint16_t addr, uint8_t byte) override;
    void write_word(uint16_t addr, uint16_t word) override;
};

} // namespace n_e_s::core
