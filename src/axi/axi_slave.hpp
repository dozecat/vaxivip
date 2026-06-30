/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        axi_slave.hpp
 * @brief       AXI4 Slave VIP
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     This module implements the Slave VIP for AXI4 protocol verification.
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2025/12/30  Initial release
 ******************************************************************************/

#ifndef AXI_SLAVE_HPP
#define AXI_SLAVE_HPP

#include "axi_ptr.hpp"
#include "axi_common.hpp"
#include "log.hpp"
#include <map>

/// @brief AXI Slave BFM
template <
    size_t DATA_WIDTH = 32,
    size_t ADDR_WIDTH = 32,
    size_t ID_WIDTH = 16
>
class axi_slave {
public:
    Log log;
    axi_slave_ptr<DATA_WIDTH, ADDR_WIDTH, ID_WIDTH> port;

    std::map<uint64_t, uint8_t> mem; // Byte-addressable memory

    /// @brief Constructor
    /// @param port Interface signals pointer
    axi_slave(axi_slave_ptr<DATA_WIDTH, ADDR_WIDTH, ID_WIDTH> port) : port(port) {
        clear();
        aw_latch = false;
        w_active = false;
        w_done_pending = false;
        w_wait_next = false;
        ar_latch = false;
        r_active = false;

        signal_clr(&awaddr_i);
        awburst_i = 0;
        awcache_i = 0;
        signal_clr(&awid_i);
        awlen_i = 0;
        awlock_i = 0;
        awprot_i = 0;
        awqos_i = 0;
        awregion_i = 0;
        awsize_i = 0;
        awvalid_i = false;

        signal_clr(&wdata_i);
        signal_clr(&wid_i);
        wlast_i = false;
        signal_clr(&wstrb_i);
        wvalid_i = false;

        bready_i = false;

        signal_clr(&araddr_i);
        arburst_i = 0;
        arcache_i = 0;
        signal_clr(&arid_i);
        arlen_i = 0;
        arlock_i = 0;
        arprot_i = 0;
        arqos_i = 0;
        arregion_i = 0;
        arsize_i = 0;
        arvalid_i = false;

        rready_i = false;
    }

    /// @brief Reset all signals
    void clear() {
        *(port.awready) = false;
        *(port.wready)  = false;
        *(port.bvalid)  = false;
        *(port.bresp)   = 0;
        *(port.bid)     = 0;

        *(port.arready) = false;
        *(port.rvalid)  = false;

        signal_clr(port.rdata);

        *(port.rresp)   = 0;
        *(port.rlast)   = false;
        *(port.rid)     = 0;
    }

    /// @brief Cycle tick
    void update_input() {
        awaddr_i = *(port.awaddr);
        awburst_i = *(port.awburst);
        awcache_i = *(port.awcache);
        awid_i = *(port.awid);
        awlen_i = *(port.awlen);
        awlock_i = *(port.awlock);
        awprot_i = *(port.awprot);
        awqos_i = *(port.awqos);
        awregion_i = *(port.awregion);
        awsize_i = *(port.awsize);
        awvalid_i = *(port.awvalid);

        wdata_i = *(port.wdata);
        wid_i = *(port.wid);
        wlast_i = *(port.wlast);
        wstrb_i = *(port.wstrb);
        wvalid_i = *(port.wvalid);

        bready_i = *(port.bready);

        araddr_i = *(port.araddr);
        arburst_i = *(port.arburst);
        arcache_i = *(port.arcache);
        arid_i = *(port.arid);
        arlen_i = *(port.arlen);
        arlock_i = *(port.arlock);
        arprot_i = *(port.arprot);
        arqos_i = *(port.arqos);
        arregion_i = *(port.arregion);
        arsize_i = *(port.arsize);
        arvalid_i = *(port.arvalid);

        rready_i = *(port.rready);
    }

    void update_output() {
        // Write Channel
        // AW Phase
        if (!aw_latch) {
            *(port.awready) = true;
            if (awvalid_i) {
                aw_addr = awaddr_i;
                aw_id = awid_i;
                aw_len = awlen_i;
                aw_burst = awburst_i;
                aw_latch = true;
                w_active = true;
                w_beat_count = 0;
                w_data_accum.clear();
            }
        } else {
            *(port.awready) = false;
        }

        // W Phase
        if (w_done_pending) {
            w_active = false;
            *(port.wready) = false;
            w_done_pending = false;

            // Trigger B Phase
            *(port.bvalid) = true;
            *(port.bresp) = 0; // OKAY
            *(port.bid) = aw_id;
            return; // Ensure BVALID is held for at least one cycle
        }

        if (w_active) {
            if (w_wait_next) {
                 *(port.wready) = false;
                 w_wait_next = false;
            } else {
                *(port.wready) = true;
                if (wvalid_i) {
                    size_t bytes_per_beat = DATA_WIDTH/8;
                    uint64_t base_addr = get_addr(aw_addr, w_beat_count, aw_len, aw_burst, bytes_per_beat);

                    std::vector<uint8_t> beat_data;
                    signal_get(&wdata_i, beat_data, bytes_per_beat);

                    // Get strobes
                    std::vector<uint8_t> strb_vec;
                    size_t strb_width_bytes = (bytes_per_beat + 7) / 8;
                    signal_get(&wstrb_i, strb_vec, strb_width_bytes);

                    for (size_t i=0; i<bytes_per_beat; i++) {
                        bool strb_bit = (strb_vec[i/8] >> (i%8)) & 1;
                        if (strb_bit) {
                            w_data_accum.push_back(beat_data[i]);
                            mem[base_addr + i] = beat_data[i];
                        }
                    }

                    w_beat_count++;
                    w_wait_next = true;

                    if (wlast_i || w_beat_count > aw_len) { // awlen is 0-based
                        w_done_pending = true;
                        // Keep wready=1 for this cycle so Master sees it
                        return;
                    }
                }
            }
        } else {
             *(port.wready) = false;
        }

        // B Phase
        if (*(port.bvalid) && bready_i) {
            *(port.bvalid) = false;
            aw_latch = false; // Ready for next transaction
            log.info("[AXI-SLV] ", burst_to_string(aw_burst), " WR success !");

            log.info("ADDR:0x", std::hex, aw_addr,
               "  LEN:", std::dec, aw_len,
               "  SIZE:", w_data_accum.size(),
               "  ID:0x", std::hex, aw_id);
            log.hexdump(w_data_accum, aw_addr);
        }

        // Read Channel
        // AR Phase
        if (!ar_latch) {
            *(port.arready) = true;
            if (arvalid_i) {
                ar_addr = araddr_i;
                ar_id = arid_i;
                ar_len = arlen_i;
                ar_burst = arburst_i;
                ar_latch = true;
                r_active = true;
                r_beat_count = 0;
                r_data_accum.clear();
            }
        } else {
            *(port.arready) = false;
        }

        // R Phase
        if (r_active) {
            size_t bytes_per_beat = DATA_WIDTH/8;

            if (*(port.rvalid) && rready_i) {
                std::vector<uint8_t> beat_data;
                signal_get(port.rdata, beat_data, bytes_per_beat);
                r_data_accum.insert(r_data_accum.end(), beat_data.begin(), beat_data.end());

                r_beat_count++;
            }

            if (r_beat_count > ar_len) {
                // Done
                r_active = false;
                *(port.rvalid) = false;
                *(port.rlast) = false;
                ar_latch = false;

                log.info("[AXI-SLV] ", burst_to_string(ar_burst), " RD success !");
                log.info("ADDR:0x", std::hex, ar_addr,
                   "  LEN:", std::dec, ar_len,
                   "  SIZE:", r_data_accum.size(),
                   "  ID:0x", std::hex, ar_id);
                log.hexdump(r_data_accum, ar_addr);
                return;
            }

            uint64_t current_addr = get_addr(ar_addr, r_beat_count, ar_len, ar_burst, bytes_per_beat);

            std::vector<uint8_t> beat_data;
            beat_data.reserve(bytes_per_beat);

            for (size_t i=0; i<bytes_per_beat; i++) {
                if (mem.find(current_addr + i) != mem.end()) {
                    beat_data.push_back(mem[current_addr + i]);
                } else {
                    beat_data.push_back(0);
                }
            }

            signal_set(port.rdata, beat_data, 0, bytes_per_beat);

            *(port.rvalid) = true;
            *(port.rresp) = 0; // OKAY
            *(port.rid) = ar_id;

            bool last = (r_beat_count == ar_len);
            *(port.rlast) = last;
        }
    }

private:
    bool aw_latch;
    bool w_active;
    uint64_t aw_addr;
    uint32_t aw_len;
    uint32_t aw_id;
    uint8_t aw_burst;
    uint32_t w_beat_count;
    bool w_done_pending;
    std::vector<uint8_t> w_data_accum;
    bool w_wait_next;

    bool ar_latch;
    bool r_active;
    uint64_t ar_addr;
    uint32_t ar_len;
    uint32_t ar_id;
    uint8_t ar_burst;
    uint32_t r_beat_count;
    std::vector<uint8_t> r_data_accum;

    sig_t(ADDR_WIDTH-1, 0) awaddr_i;
    uint8_t awburst_i;
    uint8_t awcache_i;
    sig_t(ID_WIDTH-1, 0) awid_i;
    uint8_t awlen_i;
    uint8_t awlock_i;
    uint8_t awprot_i;
    uint8_t awqos_i;
    uint8_t awregion_i;
    uint8_t awsize_i;
    bool awvalid_i;

    sig_t(DATA_WIDTH-1, 0) wdata_i;
    sig_t(ID_WIDTH-1, 0) wid_i;
    bool wlast_i;
    sig_t(DATA_WIDTH/8-1, 0) wstrb_i;
    bool wvalid_i;

    bool bready_i;

    sig_t(ADDR_WIDTH-1, 0) araddr_i;
    uint8_t arburst_i;
    uint8_t arcache_i;
    sig_t(ID_WIDTH-1, 0) arid_i;
    uint8_t arlen_i;
    uint8_t arlock_i;
    uint8_t arprot_i;
    uint8_t arqos_i;
    uint8_t arregion_i;
    uint8_t arsize_i;
    bool arvalid_i;

    bool rready_i;

    uint64_t get_addr(uint64_t start_addr, uint32_t beat, uint32_t len, uint8_t burst, size_t bytes_per_beat) {
        if (burst == 0) {
            return start_addr;
        } else if (burst == 1) {
            return start_addr + beat * bytes_per_beat;
        } else if (burst == 2) {
            uint64_t total_bytes = bytes_per_beat * (len + 1);
            uint64_t lower_wrap_boundary = (start_addr / total_bytes) * total_bytes;
            uint64_t current_offset = (start_addr % total_bytes) + (beat * bytes_per_beat);
            return lower_wrap_boundary + (current_offset % total_bytes);
        }
        return start_addr;
    }
};

#endif
