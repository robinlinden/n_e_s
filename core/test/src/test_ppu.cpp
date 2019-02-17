#include "core/invalid_address.h"
#include "core/ppu_factory.h"

#include <gtest/gtest.h>

using namespace n_e_s::core;

namespace n_e_s::core {

bool operator==(const IPpu::Registers &a, const IPpu::Registers &b) {
    return a.ctrl == b.ctrl && a.mask == b.mask && a.status == b.status &&
           a.oamaddr == b.oamaddr && a.fine_x_scroll == b.fine_x_scroll &&
           a.vram_addr == b.vram_addr && a.temp_vram_addr == b.temp_vram_addr &&
           a.write_toggle == b.write_toggle;
}

} // namespace n_e_s::core

namespace {

class PpuTest : public ::testing::Test {
public:
    PpuTest() : registers(), ppu(PpuFactory::create(&registers)), expected() {}

    void step_execution(uint32_t cycles) {
        for (uint32_t i = 0; i < cycles; ++i) {
            ppu->execute();
        }
    }

    IPpu::Registers registers;
    std::unique_ptr<IPpu> ppu;

    IPpu::Registers expected;
};

TEST_F(PpuTest, read_invalid_address) {
    EXPECT_THROW(ppu->read_byte(0x5000), InvalidAddress);
}

TEST_F(PpuTest, write_invalid_address) {
    EXPECT_THROW(ppu->write_byte(0x3000, 0xAB), InvalidAddress);
}

TEST_F(PpuTest, read_status_register) {
    registers.status = 0x25;

    const uint8_t status = ppu->read_byte(0x2002);

    EXPECT_EQ(0x25, status);
}

TEST_F(PpuTest, clear_status_when_reading_status) {
    registers.status = 0xFF;
    expected.status = 0x7F;

    ppu->read_byte(0x2002);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, set_vblank_flag_during_vertical_blanking) {
    registers.status = 0x00;
    expected.status = 0x80;

    // The VBlank flag is set at the second cycle of scanline 241
    step_execution(341 * 241 + 2);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, clear_vblank_flag_during_pre_render_line) {
    registers.status = 0x00;
    expected.status = 0x00;

    // The VBlank flag is cleared at the second cycle of scanline 261
    step_execution(341 * 261 + 2);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, write_to_ctrl_register) {
    expected.ctrl = 0xBA;
    expected.temp_vram_addr = 0x800;

    ppu->write_byte(0x2000, 0xBA);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, write_to_mask_register) {
    expected.mask = 0x33;

    ppu->write_byte(0x2001, 0x33);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, write_to_oamaddr_register) {
    expected.oamaddr = 0x05;

    ppu->write_byte(0x2003, 0x05);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, ignore_oamdata_write_background_enabled) {
    registers.mask = 0b00001000;
    registers.oamaddr = 0x02;
    expected.mask = registers.mask;
    expected.oamaddr = registers.oamaddr;

    ppu->write_byte(0x2004, 0x5A);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, ignore_oamdata_write_sprite_enabled) {
    registers.mask = 0b00010000;
    registers.oamaddr = 0x02;
    expected.mask = registers.mask;
    expected.oamaddr = registers.oamaddr;

    ppu->write_byte(0x2004, 0x73);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, ignore_oamdata_during_pre_render_scanline) {
    registers.mask = 0b00011000;
    registers.oamaddr = 0x02;
    expected.status = 0x80;
    expected.mask = registers.mask;
    expected.oamaddr = registers.oamaddr;

    step_execution(341 * 261);

    ppu->write_byte(0x2004, 0x73);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, write_to_oamdata_register_rendering_disabled) {
    expected.oamaddr = 0x01;

    ppu->write_byte(0x2004, 0x77);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, write_to_oamdata_register_during_vertical_blanking) {
    expected.status = 0x80;
    expected.oamaddr = 0x01;

    step_execution(341 * 250);

    ppu->write_byte(0x2004, 0x21);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, write_and_read_oamdata_register) {
    registers.oamaddr = expected.oamaddr = 0x11;

    ppu->write_byte(0x2004, 0x35);
    ppu->write_byte(0x2003, 0x11);
    const uint8_t byte = ppu->read_byte(0x2004);

    EXPECT_EQ(expected, registers);
    EXPECT_EQ(0x35, byte);
}

TEST_F(PpuTest, write_ppu_scroll_one_time) {
    expected.fine_x_scroll = 0b110;
    expected.temp_vram_addr = 0b0000'0000'0001'1101;
    expected.write_toggle = true;

    ppu->write_byte(0x2005, 0b1110'1110);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, write_ppu_scroll_two_times) {
    expected.fine_x_scroll = 0b101;
    expected.temp_vram_addr = 0b0111'0010'1000'0111;
    expected.write_toggle = false;

    ppu->write_byte(0x2005, 0b0011'1101);
    ppu->write_byte(0x2005, 0b1010'0111);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, write_ppu_scroll_nametable_bits_not_overwritten) {
    expected.ctrl = 0b0000'0011;
    expected.temp_vram_addr = 0b00'1100'0001'1111;
    expected.write_toggle = true;

    ppu->write_byte(0x2000, 0b0000'0011);
    ppu->write_byte(0x2005, 0b1111'1000);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, write_ppu_addr_one_time) {
    expected.temp_vram_addr = 0b0010'1101'0000'0000;
    expected.write_toggle = true;

    ppu->write_byte(0x2006, 0b0010'1101);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, write_ppu_addr_two_times) {
    expected.temp_vram_addr = 0b0010'1101'0110'0001;
    expected.vram_addr = expected.temp_vram_addr;
    expected.write_toggle = false;

    ppu->write_byte(0x2006, 0b0010'1101);
    ppu->write_byte(0x2006, 0b0110'0001);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, increment_vram_addr_by_1_after_writing) {
    expected.vram_addr = 0x01;

    ppu->write_byte(0x2007, 0x05);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, increment_vram_addr_by_32_after_writing) {
    registers.ctrl = expected.ctrl = 0x04;
    expected.vram_addr = 0x20;

    ppu->write_byte(0x2007, 0x05);

    EXPECT_EQ(expected, registers);
}

TEST_F(PpuTest, throw_if_ppu_address_is_out_of_range) {
    registers.vram_addr = 0x4001;

    EXPECT_THROW(ppu->write_byte(0x2007, 0x05), InvalidAddress);
}

} // namespace
