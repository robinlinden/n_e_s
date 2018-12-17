#pragma once

#include <cstdint>

namespace n_e_s::core {

class IPpu {
public:
    virtual ~IPpu() = default;

    virtual void execute() = 0;
};

} // namespace n_e_s::core
