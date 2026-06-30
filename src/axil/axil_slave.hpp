/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        axil_slave.hpp
 * @brief       AXI4-Lite Slave VIP
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     This module implements the Slave VIP for AXI4-Lite protocol verification.
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2025/12/30  Initial release
 ******************************************************************************/

#ifndef AXIL_SLAVE_HPP
#define AXIL_SLAVE_HPP

#include "axil_ptr.hpp"
#include "log.hpp"
#include <map>

/// @brief AXI4-Lite Slave BFM
template <
    size_t DATA_WIDTH = 32,
    size_t ADDR_WIDTH = 16
>
class axil_slave {
public:
    Log log;
    axil_slave_ptr<DATA_WIDTH, ADDR_WIDTH> port;              ///< Interface signal pointers
    std::map<uint64_t, uint64_t> mem;       ///< Memory storage

    /// @brief Constructor
    axil_slave(axil_slave_ptr<DATA_WIDTH, ADDR_WIDTH> port) : port(port) {
        wr_addr_received = false;
        wr_data_received = false;
        wr_resp_sent = false;
        rd_addr_received = false;
        rd_data_sent = false;
        rd_data_reg = 0;

        // Initialize inputs
        awvalid_i = false;
        awaddr_i = 0;
        wvalid_i = false;
        wdata_i = 0;
        wstrb_i = 0;
        bready_i = false;
        arvalid_i = false;
        araddr_i = 0;
        rready_i = false;

        // Default outputs
        *(port.awready) = false;
        *(port.wready) = false;
        *(port.bvalid) = false;
        *(port.arready) = false;
        *(port.rvalid) = false;
    }

    /// @brief Cycle tick
    void update_input() {
        awvalid_i = *(port.awvalid);
        awaddr_i  = *(port.awaddr);

        wvalid_i  = *(port.wvalid);
        wdata_i   = *(port.wdata);
        wstrb_i   = *(port.wstrb);
        bready_i  = *(port.bready);
        arvalid_i = *(port.arvalid);
        araddr_i  = *(port.araddr);

        rready_i  = *(port.rready);
    }

    void update_output() {
        // 2. Update State
        // Write Address
        if (!wr_addr_received && awvalid_i) {
            wr_addr = awaddr_i;
            wr_addr_received = true;
        }

        // Write Data
        if (!wr_data_received && wvalid_i) {
            wr_data = wdata_i;
            wr_data_received = true;
        }

        // Write Response
        if (wr_addr_received && wr_data_received && !wr_resp_sent) {
            mem[wr_addr] = wr_data;
            log.info("[AXIL-SLV] WR success !");
            log.info("ADDR:0x", std::hex, wr_addr, "  DATA:0x", wr_data);
            wr_resp_sent = true;
        } else if (wr_resp_sent && bready_i) {
            wr_resp_sent = false;
            wr_addr_received = false;
            wr_data_received = false;
        }

        // Read Address
        if (!rd_addr_received && arvalid_i) {
            rd_addr = araddr_i;
            rd_addr_received = true;
        }

        // Read Data
        if (rd_addr_received && !rd_data_sent) {
            uint64_t rdata = 0;
            if (mem.count(rd_addr)) {
                rdata = mem[rd_addr];
            }
            log.info("[AXIL-SLV] RD success !");
            log.info("ADDR:0x", std::hex, rd_addr, "  DATA:0x", rdata);
            rd_data_reg = rdata;
            rd_data_sent = true;
        } else if (rd_data_sent && rready_i) {
            rd_data_sent = false;
            rd_addr_received = false;
        }

        // 3. Drive Outputs
        *(port.awready) = !wr_addr_received;
        *(port.wready)  = !wr_data_received;

        *(port.bvalid)  = wr_resp_sent;
        *(port.bresp)   = OKAY;

        *(port.arready) = !rd_addr_received;

        *(port.rvalid)  = rd_data_sent;
        *(port.rdata)   = rd_data_reg;
        *(port.rresp)   = OKAY;
    }

private:
    bool wr_addr_received;
    uint64_t wr_addr;
    bool wr_data_received;
    uint64_t wr_data;
    bool wr_resp_sent;

    bool rd_addr_received;
    uint64_t rd_addr;
    bool rd_data_sent;
    uint64_t rd_data_reg;

    bool awvalid_i;
    uint64_t awaddr_i;
    bool wvalid_i;
    uint64_t wdata_i;
    uint64_t wstrb_i;
    bool bready_i;
    bool arvalid_i;
    uint64_t araddr_i;
    bool rready_i;
};

#endif
