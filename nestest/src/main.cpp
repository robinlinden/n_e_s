#include <iomanip>
#include <iostream>
#include <stdexcept>

#include <fmt/format.h>

#include "nes/core/immu.h"
#include "nes/core/imos6502.h"
#include "nes/core/ippu.h"
#include "nes/core/opcode.h"
#include "nes/nes.h"

namespace {

constexpr bool is_negative(uint8_t byte) {
    return (byte & (1u << 7u)) != 0;
}

constexpr uint8_t low_bits(uint8_t byte) {
    return static_cast<uint8_t>(
            byte & static_cast<uint8_t>(~static_cast<uint8_t>(1u << 7u)));
}

constexpr int8_t to_signed(uint8_t byte) {
    if (is_negative(byte)) {
        return low_bits(byte) - static_cast<uint8_t>(128);
    }
    return low_bits(byte);
}

constexpr bool is_undocumented(const n_e_s::core::Opcode &opcode) {
    if (opcode.family == n_e_s::core::Family::NOP &&
            opcode.instruction != n_e_s::core::Instruction::NopImplied) {
        return true;
    }
    if (opcode.family == n_e_s::core::Family::LAX) {
        return true;
    }
    if (opcode.family == n_e_s::core::Family::SAX) {
        return true;
    }
    if (opcode.instruction == n_e_s::core::Instruction::SbcImmediateEB) {
        return true;
    }
    if (opcode.family == n_e_s::core::Family::DCP) {
        return true;
    }
    return false;
}

constexpr int get_arg_count(n_e_s::core::AddressMode mode) {
    switch (mode) {
    case n_e_s::core::AddressMode::Implied:
    case n_e_s::core::AddressMode::Accumulator:
        return 0;
    case n_e_s::core::AddressMode::Immediate:
    case n_e_s::core::AddressMode::Zeropage:
    case n_e_s::core::AddressMode::ZeropageX:
    case n_e_s::core::AddressMode::ZeropageY:
    case n_e_s::core::AddressMode::Relative:
    case n_e_s::core::AddressMode::IndexedIndirect:
    case n_e_s::core::AddressMode::IndirectIndexed:
        return 1;
    case n_e_s::core::AddressMode::Absolute:
    case n_e_s::core::AddressMode::AbsoluteX:
    case n_e_s::core::AddressMode::AbsoluteY:
    case n_e_s::core::AddressMode::Indirect:
        return 2;
    }
    return 0;
}

std::string get_memory_string(const n_e_s::core::Opcode &opcode,
        const uint8_t op1,
        const uint8_t op2,
        const n_e_s::nes::Nes &nes) {
    switch (opcode.address_mode) {
    case n_e_s::core::AddressMode::Implied:
        break;
    case n_e_s::core::AddressMode::Immediate:
        return fmt::format("#${:02X}", op1);
    case n_e_s::core::AddressMode::Zeropage:
        return fmt::format("${:02X} = {:02X}", op1, nes.mmu().read_byte(op1));
    case n_e_s::core::AddressMode::ZeropageX: {
        const uint8_t effective_addr = op1 + nes.cpu_registers().x;
        return fmt::format("${:02X},X @ {:02X} = {:02X}",
                op1,
                effective_addr,
                nes.mmu().read_byte(effective_addr));
    }
    case n_e_s::core::AddressMode::ZeropageY: {
        const uint8_t effective_addr = op1 + nes.cpu_registers().y;
        return fmt::format("${:02X},Y @ {:02X} = {:02X}",
                op1,
                effective_addr,
                nes.mmu().read_byte(effective_addr));
    }
    case n_e_s::core::AddressMode::Relative:
        return fmt::format(
                "${:04X}", to_signed(op1) + nes.cpu().state().start_pc + 2);
    case n_e_s::core::AddressMode::Accumulator:
        return "A";
    case n_e_s::core::AddressMode::IndexedIndirect: {
        const uint8_t midaddr = op1 + nes.cpu_registers().x;
        const uint16_t effective_addr =
                nes.mmu().read_byte(midaddr) +
                (nes.mmu().read_byte(static_cast<uint8_t>(midaddr + 1u)) << 8u);
        return fmt::format("(${:02X},X) @ {:02X} = {:04X} = {:02X}",
                op1,
                midaddr,
                effective_addr,
                nes.mmu().read_byte(effective_addr));
    } break;
    case n_e_s::core::AddressMode::IndirectIndexed: {
        const uint8_t op1_plus_one =
                nes.mmu().read_byte(static_cast<uint8_t>(op1 + 1u));
        const uint16_t midaddr = nes.mmu().read_byte(op1) |
                                 static_cast<uint16_t>(op1_plus_one << 8u);
        const uint16_t effective_addr = midaddr + nes.cpu_registers().y;
        return fmt::format("(${:02X}),Y = {:04X} @ {:04X} = {:02X}",
                op1,
                midaddr,
                effective_addr,
                nes.mmu().read_byte(effective_addr));
    } break;
    case n_e_s::core::AddressMode::Absolute:
        if (opcode.family != n_e_s::core::Family::JSR &&
                opcode.family != n_e_s::core::Family::JMP) {
            const uint8_t resulting_addr =
                    nes.mmu().read_byte(static_cast<uint16_t>(op2 << 8u) | op1);
            return fmt::format(
                    "${:02X}{:02X} = {:02X}", op2, op1, resulting_addr);
        } else {
            return fmt::format("${:02X}{:02X}", op2, op1);
        }
    case n_e_s::core::AddressMode::AbsoluteX: {
        const uint16_t midaddr = op1 | static_cast<uint16_t>(op2 << 8u);
        const uint16_t effective_addr = midaddr + nes.cpu_registers().x;
        return fmt::format("${:04X},X @ {:04X} = {:02X}",
                midaddr,
                effective_addr,
                nes.mmu().read_byte(effective_addr));
    }
    case n_e_s::core::AddressMode::AbsoluteY: {
        const uint16_t midaddr = op1 | static_cast<uint16_t>(op2 << 8u);
        const uint16_t effective_addr = midaddr + nes.cpu_registers().y;
        return fmt::format("${:04X},Y @ {:04X} = {:02X}",
                midaddr,
                effective_addr,
                nes.mmu().read_byte(effective_addr));
    }
    case n_e_s::core::AddressMode::Indirect:
        const uint16_t effective_addr1 = static_cast<uint16_t>(op2 << 8u) | op1;
        const uint16_t effective_addr2 = static_cast<uint16_t>(op2 << 8u) |
                                         static_cast<uint8_t>(op1 + 1u);
        return fmt::format("(${:02X}{:02X}) = {:02X}{:02X}",
                op2,
                op1,
                nes.mmu().read_byte(effective_addr2),
                nes.mmu().read_byte(effective_addr1));
    }
    return "";
}

std::string get_reg_string(const n_e_s::core::CpuRegisters &reg) {
    return fmt::format("A:{:02X} X:{:02X} Y:{:02X} P:{:02X} SP:{:02X}",
            reg.a,
            reg.x,
            reg.y,
            reg.p,
            reg.sp);
}

std::string get_opcode_string(const n_e_s::nes::Nes &nes) {
    const n_e_s::core::IMos6502 &cpu = nes.cpu();
    const auto state = cpu.state();
    const std::uint8_t raw_opcode = nes.mmu().read_byte(state.start_pc);
    const std::uint8_t op1 = nes.mmu().read_byte(state.start_pc + 1u);
    const std::uint8_t op2 = nes.mmu().read_byte(state.start_pc + 2u);

    std::string result =
            fmt::format("{:04X}  {:02X}", state.start_pc, raw_opcode);

    const int arg_count = get_arg_count(state.current_opcode->address_mode);
    if (arg_count >= 1) {
        result += fmt::format(" {:02X}", op1);
        if (arg_count >= 2) {
            result += fmt::format(" {:02X}", op2);
        } else {
            result += fmt::format("   ");
        }
    } else {
        result += fmt::format("      ");
    }
    result += fmt::format(" {}{}",
            is_undocumented(*state.current_opcode) ? "*" : " ",
            n_e_s::core::to_string(state.current_opcode->family));

    return result;
}

std::string get_execution_string(const n_e_s::nes::Nes &nes) {
    const n_e_s::core::IMos6502 &cpu = nes.cpu();
    const auto state = cpu.state();
    const std::string opcode_string = get_opcode_string(nes);
    const std::uint8_t op1 = nes.mmu().read_byte(state.start_pc + 1u);
    const std::uint8_t op2 = nes.mmu().read_byte(state.start_pc + 2u);
    const std::string memory_string =
            get_memory_string(*state.current_opcode, op1, op2, nes);

    const std::string reg_string = get_reg_string(nes.cpu_registers());

    const std::string result = fmt::format("{} {:<28}{} PPU:{:3},{:3} CYC:{}",
            opcode_string,
            memory_string,
            reg_string,
            nes.ppu().cycle() - 1,
            nes.ppu().scanline(),
            nes.current_cycle() / 3 + 7);

    return result;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Expected one argument; nestest.nes" << std::endl;
        return 1;
    }

    try {
        n_e_s::nes::Nes nes;
        nes.load_rom(argv[1]);
        nes.cpu_registers().pc = 0xC000;

        n_e_s::core::CpuState prev_state{};

        for (int i = 0; i < 1000000; ++i) {
            nes.execute();

            const auto state = nes.cpu().state();

            if (i == 0 || prev_state.start_cycle != state.start_cycle) {
                const std::string execution_string = get_execution_string(nes);
                std::cout << execution_string << std::endl;

                prev_state = state;
            }
        }
    } catch (const std::exception &e) {
        std::cout << "Expcetion: " << e.what() << std::endl;
    }
}