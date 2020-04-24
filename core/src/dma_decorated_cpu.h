#pragma once

#include "core/icpu.h"
#include "core/immu.h"
#include "core/imos6502.h"
#include "core/ippu.h"

#include <cstdint>

class DmaDecoratedCpu final : public IMos6502 {
public:
    DmaDecoratedCpu(IMos6502 *cpu, IPpu *ppu, IMmu *mmu)
            : cpu_(cpu), ppu_(ppu), mmu_(mmu) {}

    // ICpu
    void execute() override {
        if (dma_cycle_) {
            if (dma_cycle_ > 512) {
                // Do nothing.
            } else if (dma_cycle_ % 2 == 0) {
                tmp_ = mmu_->read_byte(addr_);
            } else {
                constexpr uint16_t kOamData = 0x2004;
                ppu_->write_byte(kOamData, tmp_);
                ++addr_;
            }

            --dma_cycle_;
            return;
        }

        cpu_->execute();
    }

    void reset() override { cpu_->reset(); }

    // IMos6502
    CpuState state() override const { return cpu_->state(); }
    void set_nmi(bool nmi) { cpu_->set_nmi(nmi); }

    //
    void trigger_dma(uint8_t start_addr) {
        dma_cycle_ = 513;
        if (cpu_->state().cycle % 2 != 0) {
            dma_cycle_ += 1;
        }

        addr_ = start_addr * 0x0100;
    }

private:
    IMos6502 *const cpu_;
    IPpu *const ppu_;
    IMmu *const mmu_;
    uint16_t dma_cycle_{0};
    uint16_t addr_{};
    uint8_t tmp_{};
};
