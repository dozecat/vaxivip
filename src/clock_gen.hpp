/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        clock_gen.hpp
 * @brief       Software clock generator for Verilator co-simulation
 * @see         https://github.com/dozecat/axi_lib
 *
 * @details     Template parameter TSTEP_PS is the simulation time step in
 *              picoseconds. init() binds a CData* clock and sets half-period from
 *              frequency (MHz), quantized to whole multiples of TSTEP_PS. At each
 *              time step, pedge()/nedge()/edge() update the clock and report
 *              whether a rising, falling, or any edge occurred at the current
 *              quantized simulation time.
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2025/12/25  Initial release
 ******************************************************************************/


#ifndef CLOCK_GEN_HPP
#define CLOCK_GEN_HPP

#include <verilated.h>
#include <cstdint>
#include <cmath>

template <vluint64_t TSTEP_PS = 10>
class ClockGen {
public:
    ClockGen() = default;

    void init(double freq_mhz, CData* sig) {
        signal = sig;
        vluint64_t raw = (vluint64_t)(500000.0 / freq_mhz + 0.5);
        half_period_ps = ((raw + TSTEP_PS / 2) / TSTEP_PS) * TSTEP_PS;
        next_edge_ps   = half_period_ps;
        *signal        = 0;
        cached_edge    = 0;
        cached_time    = 0;
    }

    bool pedge(vluint64_t time_ps) {
        eval(time_ps);
        return cached_edge == 1;
    }

    bool nedge(vluint64_t time_ps) {
        eval(time_ps);
        return cached_edge == -1;
    }

    bool edge(vluint64_t time_ps) {
        eval(time_ps);
        return cached_edge != 0;
    }

private:
    vluint64_t half_period_ps = 0;
    vluint64_t next_edge_ps   = 0;
    CData*     signal         = nullptr;
    int        cached_edge    = 0;
    vluint64_t cached_time    = 0;

    void eval(vluint64_t time_ps) {
        vluint64_t t = (time_ps / TSTEP_PS) * TSTEP_PS;

        if (cached_time == t) {
            return;
        }

        cached_time = t;
        if (t < next_edge_ps) {
            cached_edge = 0;
            return;
        }

        *signal = !(*signal);
        next_edge_ps += half_period_ps;
        cached_edge = *signal ? 1 : -1;
    }
};

#endif
