#include "debug_mmu.h"

#include <cstdio>

namespace n_e_s::core {

uint8_t DebugMmu::read_byte(uint16_t addr) const {
    const uint8_t byte = Mmu::read_byte(addr);
    printf("Read byte %2X from %4X\n", byte, addr);
    return byte;
}
uint16_t DebugMmu::read_word(uint16_t addr) const {
    const uint16_t word = Mmu::read_word(addr);
    printf("Read word %4X from %4X\n", word, addr);
    return word;
}

void DebugMmu::write_byte(uint16_t addr, uint8_t byte) {
    Mmu::write_byte(addr, byte);
    printf("Wrote byte %2X to %4X\n", byte, addr);
}
void DebugMmu::write_word(uint16_t addr, uint16_t word) {
    Mmu::write_word(addr, word);
    printf("Wrote word %4X to %4X\n", word, addr);
}

} // namespace n_e_s::core
