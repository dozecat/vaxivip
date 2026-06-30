/******************************************************************************
 * Copyright (C) 2026 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        sim_coop.hpp
 * @brief       Protothread-based simulation engine for Verilator
 * @see         https://github.com/dozecat/axi_lib
 *
 * @details     Combines ClockGen with protothread cooperative scheduling.
 *              Tasks are associated with clock domains; the engine executes
 *              them only on the rising edge of their clock.
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ----  ----------  -----------------------------------------------------
 * 1.0        2026/06/05  Initial release
 ******************************************************************************/

#ifndef SIM_COOP_HPP
#define SIM_COOP_HPP

#include <verilated.h>
#include <verilated_vcd_c.h>
#include "clock_gen.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>

/// Protothread context persisted across invocations
struct PtCtx {
    uint32_t  pt_state   = 0;   ///< Resume line number (switch/case)
    uint32_t  pt_count   = 0;   ///< PT_WAIT_CYCLES counter
    uint64_t  time_ps    = 0;   ///< Simulation time at invocation
    bool      on_edge    = false;
};

/// Begin a protothread; declares local yield flag, must pair with PT_END
#define PT_BEGIN(ctx)         { char _pt_yf = 1; switch ((ctx).pt_state) { case 0:

/// End a protothread and reset state
#define PT_END(ctx)           } (ctx).pt_state = 0; }

/// Wait for next clock edge of the task's clock domain
#define PT_WAIT_CLK(ctx) \
    do { \
        _pt_yf = 0; \
        (ctx).pt_state = __LINE__; \
        case __LINE__: \
        if (_pt_yf == 0) { return; } \
        if (!(ctx).on_edge) return; \
    } while(0)

/// Wait for next clock edge with an extra condition
#define PT_WAIT_CLK_COND(ctx, cond) \
    do { \
        _pt_yf = 0; \
        (ctx).pt_state = __LINE__; \
        case __LINE__: \
        if (_pt_yf == 0) { return; } \
        if (!(ctx).on_edge || !(cond)) return; \
    } while(0)

/// Wait for N clock edges
#define PT_WAIT_CYCLES(ctx, n) \
    do { \
        (ctx).pt_state = __LINE__; \
        case __LINE__: \
        if ((ctx).pt_count < (n)) { \
            if ((ctx).on_edge) (ctx).pt_count++; \
            return; \
        } \
        (ctx).pt_count = 0; \
    } while(0)

/// @brief Cooperative simulation scheduler
/// @tparam TOP      Verilator top module type
/// @tparam TSTEP_PS Simulation time step in picoseconds
template<typename TOP, vluint64_t TSTEP_PS = 10>
class SimCoop {
public:
    TOP*           top = nullptr;
    VerilatedVcdC* tfp = nullptr; ///< VCD trace handle

    SimCoop() = default;
    explicit SimCoop(TOP* t) : top(t) {}

    /// Register a clock domain; returns clock index
    int add_clock(const std::string& name, double freq_mhz, CData* sig) {
        for (size_t i = 0; i < clocks.size(); i++)
            if (clocks[i].name == name)
                throw std::runtime_error("SimCoop: duplicate clock '" + name + "'");
        clocks.push_back({name, ClockGen<TSTEP_PS>{}});
        clocks.back().gen.init(freq_mhz, sig);
        return (int)clocks.size() - 1;
    }

    /// Register a protothread task on a clock domain
    void add_task(const std::string& name, int clock_idx,
                  std::function<void(PtCtx&)> fn) {
        check_clock_idx(clock_idx);
        tasks.push_back({name, clock_idx, std::move(fn), PtCtx{}});
    }

    /// Register pre-eval callback (BFM update_input)
    void add_sample_cb(int clock_idx, std::function<void(vluint64_t)> fn) {
        check_clock_idx(clock_idx);
        sample_cbs.push_back({clock_idx, std::move(fn)});
    }

    /// Register post-eval callback (BFM update_output)
    void add_drive_cb(int clock_idx, std::function<void(vluint64_t)> fn) {
        check_clock_idx(clock_idx);
        drive_cbs.push_back({clock_idx, std::move(fn)});
    }

    void stop() { stopped = true; }

    /// Run simulation for max_time_ps picoseconds
    void run(vluint64_t max_time_ps) {
        if (!top) return;

        vluint64_t time = 0;
        std::vector<bool> clk_edges(clocks.size(), false);

        while (!stopped && !Verilated::gotFinish() && time < max_time_ps) {
            for (size_t i = 0; i < clocks.size(); i++)
                clk_edges[i] = clocks[i].gen.pedge(time);

            for (size_t i = 0; i < sample_cbs.size(); i++)
                if (clk_edges[sample_cbs[i].clock_idx]) sample_cbs[i].fn(time);

            for (size_t i = 0; i < tasks.size(); i++)
                if (clk_edges[tasks[i].clock_idx]) {
                    tasks[i].ctx.on_edge = true;
                    tasks[i].ctx.time_ps = time;
                    tasks[i].fn(tasks[i].ctx);
                    tasks[i].ctx.on_edge = false;
                }

            top->eval();

            for (size_t i = 0; i < drive_cbs.size(); i++)
                if (clk_edges[drive_cbs[i].clock_idx]) drive_cbs[i].fn(time);

            if (tfp) tfp->dump(time);
            time += TSTEP_PS;
        }

        if (tfp) { tfp->close(); tfp = nullptr; }
    }

private:
    struct ClockDesc {
        std::string       name;
        ClockGen<TSTEP_PS> gen;
    };

    struct TaskDesc {
        std::string                     name;
        int                             clock_idx;
        std::function<void(PtCtx&)>     fn;
        PtCtx                           ctx;
    };

    struct CallbackDesc {
        int                             clock_idx;
        std::function<void(vluint64_t)> fn;
    };

    void check_clock_idx(int idx) const {
        if (idx < 0 || idx >= (int)clocks.size())
            throw std::runtime_error("SimCoop: invalid clock index " +
                                     std::to_string(idx));
    }

    std::vector<ClockDesc>    clocks;
    std::vector<TaskDesc>     tasks;
    std::vector<CallbackDesc> sample_cbs;
    std::vector<CallbackDesc> drive_cbs;
    bool                      stopped = false;
};

#endif
