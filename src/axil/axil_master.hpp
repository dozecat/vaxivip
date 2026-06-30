/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        axil_master.hpp
 * @brief       AXI4-Lite Master VIP
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     This module implements the Master VIP for AXI4-Lite protocol verification.
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2025/12/30  Initial release
 ******************************************************************************/

#ifndef AXIL_MASTER_HPP
#define AXIL_MASTER_HPP

#include "axil_ptr.hpp"
#include "log.hpp"
#include <queue>

/// @brief AXI4-Lite Master BFM
template <
    size_t DATA_WIDTH = 32,
    size_t ADDR_WIDTH = 16
>
class axil_master {
public:
    Log log;
    axil_master_ptr<DATA_WIDTH, ADDR_WIDTH> port;              ///< Interface signal pointers

    /// @brief Constructor
    axil_master(axil_master_ptr<DATA_WIDTH, ADDR_WIDTH> port) : port(port) {
        clear();
        wr_active = false;
        rd_active = false;
        aw_hs = false;
        w_hs = false;
        b_hs = false;
        ar_hs = false;
        r_hs = false;

        awready_i = false;
        wready_i = false;
        bvalid_i = false;
        bresp_i = 0;
        arready_i = false;
        rvalid_i = false;
        rdata_i = 0;
        rresp_i = 0;
    }

    /// @brief Clear all signals
    void clear() {
        waddr_clr();
        wdata_clr();
        raddr_clr();
        rdata_clr();
        rresp_clr();
    }

    /// @brief Initiate a write transaction
    void write(uint64_t addr, uint64_t data) {
        wr_addr_q.push(addr);
        wr_data_q.push(data);
    }

    /// @brief Initiate a read transaction
    void read(uint64_t addr) {
        rd_addr_q.push(addr);
    }

    /// @brief Retrieve read data
    bool get_read_data(uint64_t &data) {
        if (rd_data_q.empty()) return false;
        data = rd_data_q.front();
        rd_data_q.pop();
        return true;
    }

    /// @brief Cycle tick
    void update_input() {
        awready_i = *(port.awready);
        wready_i  = *(port.wready);
        bvalid_i  = *(port.bvalid);
        bresp_i   = *(port.bresp);
        arready_i = *(port.arready);
        rvalid_i  = *(port.rvalid);
        rdata_i   = *(port.rdata);
        rresp_i   = *(port.rresp);
    }

    void update_output() {
        // 1. Process delayed clears from previous cycle handshakes
        if (aw_hs) { waddr_clr(); aw_hs = false; }
        if (w_hs)  { wdata_clr(); rresp_set(); w_hs = false; }
        if (b_hs)  { rresp_clr(); b_hs = false; }
        if (ar_hs) { raddr_clr(); rdata_set(); ar_hs = false; }
        if (r_hs)  { rdata_clr(); r_hs = false; }

        // 2. Detect Handshakes (Current Cycle)
        // Write Address
        if (awready_i && *(port.awvalid) && !aw_hs) {
            aw_hs = true;
        }

        // Write Data
        if (wready_i && *(port.wvalid) && !w_hs) {
            w_hs = true;
        }

        // Write Response
        if (*(port.bready) && bvalid_i && !b_hs) {
            if (bresp_i != OKAY) {
                log.warning("[AXIL-MST] Write response not OKAY!");
            }
            b_hs = true;
            log.info("[AXIL-MST] WR success !");
            log.info("ADDR:0x", std::hex, current_wr_addr, "  DATA:0x", current_wr_data);
        }

        // Read Address
        if (arready_i && *(port.arvalid) && !ar_hs) {
            ar_hs = true;
        }

        // Read Data
        if (*(port.rready) && rvalid_i && !r_hs) {
            uint64_t data = rdata_i;
            rd_data_q.push(data);
            if (rresp_i != OKAY) {
                log.warning("[AXIL-MST] Read response not OKAY!");
            }
            r_hs = true;
            log.info("[AXIL-MST] RD success !");
            log.info("ADDR:0x", std::hex, current_rd_addr, "  DATA:0x", data);
        }

        // 3. Drive New Requests (if not busy and not in handshake process)
        if (!wr_active && !wr_addr_q.empty() && !wr_data_q.empty()) {
            wr_active = true;
            uint64_t addr = wr_addr_q.front(); wr_addr_q.pop();
            uint64_t data = wr_data_q.front(); wr_data_q.pop();

            current_wr_addr = addr;
            current_wr_data = data;

            waddr_set(addr);
            wdata_set(data);
        }

        if (!rd_active && !rd_addr_q.empty()) {
            rd_active = true;
            uint64_t addr = rd_addr_q.front(); rd_addr_q.pop();

            current_rd_addr = addr;

            raddr_set(addr);
        }
    }

private:
    std::queue<uint64_t> wr_data_q;
    std::queue<uint64_t> wr_addr_q;
    std::queue<uint64_t> rd_addr_q;
    std::queue<uint64_t> rd_data_q;

    bool wr_active;
    bool rd_active;

    bool aw_hs, w_hs, b_hs, ar_hs, r_hs;

    uint64_t current_wr_addr;
    uint64_t current_wr_data;
    uint64_t current_rd_addr;

    bool awready_i;
    bool wready_i;
    bool bvalid_i;
    uint8_t bresp_i;
    bool arready_i;
    bool rvalid_i;
    uint64_t rdata_i;
    uint8_t rresp_i;

    void waddr_set(uint64_t addr) {
        *(port.awaddr)  = addr;
        *(port.awvalid) = true;
    }

    void waddr_clr() {
        *(port.awaddr)  = 0;
        *(port.awvalid) = false;
    }

    void wdata_set(uint64_t data) {
        *(port.wdata)   = data;
        *(port.wstrb)   = (1ULL << (DATA_WIDTH / 8)) - 1; // Enable all bytes
        *(port.wvalid)  = true;
    }

    void wdata_clr() {
        *(port.wdata)   = 0;
        *(port.wstrb)   = 0;
        *(port.wvalid)  = false;
    }

    void rresp_set() {
        *(port.bready)  = true;
    }

    void rresp_clr() {
        *(port.bready)  = false;
        wr_active = false;
    }

    void raddr_set(uint64_t addr) {
        *(port.araddr)  = addr;
        *(port.arvalid) = true;
    }

    void raddr_clr() {
        *(port.araddr)  = 0;
        *(port.arvalid) = false;
    }

    void rdata_set() {
        *(port.rready)  = true;
    }

    void rdata_clr() {
        *(port.rready)  = false;
        rd_active = false;
    }

};

#endif
