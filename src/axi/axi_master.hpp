/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        axi_master.hpp
 * @brief       AXI4 Master VIP
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     This module implements the Master VIP for AXI4 protocol verification.
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2025/12/30  Initial release
 ******************************************************************************/

#ifndef AXI_MASTER_HPP
#define AXI_MASTER_HPP

#include "axi_ptr.hpp"
#include "axi_common.hpp"
#include "log.hpp"
#include <queue>
#include <type_traits>

/// @brief AXI Master BFM
template <
    size_t DATA_WIDTH = 32,
    size_t ADDR_WIDTH = 32,
    size_t ID_WIDTH = 16
>
class axi_master {
public:
    Log log;
    axi_master_ptr<DATA_WIDTH, ADDR_WIDTH, ID_WIDTH> port;

    /// @brief Constructor
    /// @param port Interface signals pointer
    axi_master(axi_master_ptr<DATA_WIDTH, ADDR_WIDTH, ID_WIDTH> port) : port(port) {
        clear();
        wr_active = false;
        rd_active = false;
        aw_hs = false;
        w_hs = false;
        b_hs = false;
        ar_hs = false;
        r_hs = false;
        w_beat_count = 0;
        r_beat_count = 0;

        awready_i = false;
        wready_i = false;
        signal_clr(&bid_i);
        bresp_i = 0;
        bvalid_i = false;
        arready_i = false;
        signal_clr(&rid_i);
        signal_clr(&rdata_i);
        rresp_i = 0;
        rlast_i = false;
        rvalid_i = false;
    }

    /// @brief Reset all signals
    void clear() {
        waddr_clr();
        wdata_clr();
        raddr_clr();
        rdata_clr();
        rresp_clr();
    }

    /// @brief Write data using FIXED burst type
    /// @param addr Start address
    /// @param data Data to write
    void write_fixed(uint64_t addr, const std::vector<uint8_t>& data, uint32_t id = 0) {
        if (data.empty()) return;
        write_trans t;
        t.addr = addr;
        t.data = data;
        size_t bytes_per_beat = DATA_WIDTH/8;
        t.len = (data.size() + bytes_per_beat - 1) / bytes_per_beat - 1;
        t.burst = 0; // FIXED
        t.id = id;
        wr_q.push(t);
    }

    /// @brief Write data using INCR burst type
    /// @param addr Start address
    /// @param data Data to write
    void write_incr(uint64_t addr, const std::vector<uint8_t>& data, uint32_t id = 0) {
        if (data.empty()) return;
        write_trans t;
        t.addr = addr;
        t.data = data;
        size_t bytes_per_beat = DATA_WIDTH/8;
        t.len = (data.size() + bytes_per_beat - 1) / bytes_per_beat - 1;
        t.burst = 1; // INCR
        t.id = id;
        wr_q.push(t);
    }

    /// @brief Write data using WRAP burst type
    /// @param addr Start address
    /// @param data Data to write
    void write_wrap(uint64_t addr, const std::vector<uint8_t>& data, uint32_t id = 0) {
        if (data.empty()) return;
        write_trans t;
        t.addr = addr;
        t.data = data;
        size_t bytes_per_beat = DATA_WIDTH/8;
        t.len = (data.size() + bytes_per_beat - 1) / bytes_per_beat - 1;
        t.burst = 2; // WRAP
        t.id = id;
        wr_q.push(t);
    }

    /// @brief Request a read transaction (FIXED)
    /// @param addr Address
    /// @param size Total number of bytes to read
    void read_fixed(uint64_t addr, uint32_t size = DATA_WIDTH/8, uint32_t id = 0) {
        read_trans t;
        t.addr = addr;
        size_t bytes_per_beat = DATA_WIDTH/8;
        uint32_t num_beats = (size + bytes_per_beat - 1) / bytes_per_beat;
        if (num_beats == 0) num_beats = 1;
        t.len = num_beats - 1;
        t.burst = 0; // FIXED
        t.size = size;
        t.id = id;
        rd_q.push(t);
    }

    /// @brief Request a read transaction (INCR)
    /// @param addr Address
    /// @param size Total number of bytes to read
    void read_incr(uint64_t addr, uint32_t size = DATA_WIDTH/8, uint32_t id = 0) {
        read_trans t;
        t.addr = addr;
        size_t bytes_per_beat = DATA_WIDTH/8;
        uint32_t num_beats = (size + bytes_per_beat - 1) / bytes_per_beat;
        if (num_beats == 0) num_beats = 1;
        t.len = num_beats - 1;
        t.burst = 1; // INCR
        t.size = size;
        t.id = id;
        rd_q.push(t);
    }

    /// @brief Request a read transaction (WRAP)
    /// @param addr Address
    /// @param size Total number of bytes to read
    void read_wrap(uint64_t addr, uint32_t size = DATA_WIDTH/8, uint32_t id = 0) {
        read_trans t;
        t.addr = addr;
        size_t bytes_per_beat = DATA_WIDTH/8;
        uint32_t num_beats = (size + bytes_per_beat - 1) / bytes_per_beat;
        if (num_beats == 0) num_beats = 1;
        t.len = num_beats - 1;
        t.burst = 2; // WRAP
        t.size = size;
        t.id = id;
        rd_q.push(t);
    }

    /// @brief Get received read data
    /// @param data Output vector
    /// @return true if data available
    bool get_read_data(std::vector<uint8_t> &data) {
        if (rd_data_q.empty()) return false;
        data = rd_data_q.front();
        rd_data_q.pop();
        return true;
    }

    /// @brief Cycle tick
    void update_input() {
        awready_i = *(port.awready);
        wready_i  = *(port.wready);
        bid_i = *(port.bid);
        bresp_i = *(port.bresp);
        bvalid_i = *(port.bvalid);

        arready_i = *(port.arready);
        rid_i = *(port.rid);
        rresp_i = *(port.rresp);
        rlast_i = *(port.rlast);
        rvalid_i = *(port.rvalid);

        rdata_i = *(port.rdata);
    }

    void update_output() {
        // Write Channel
        if (!wr_active && !wr_q.empty()) {
            wr_active = true;
            w_beat_count = 0;
            aw_hs = false;
            w_hs = false;
            b_hs = false;

            write_trans& t = wr_q.front();
            waddr_set(t.addr, t.len, t.burst, t.id);
            wdata_set(t.data, 0, (t.len == 0), t.id);
        } else if (wr_active) {
            write_trans& t = wr_q.front();

            // AW Handshake
            if (!aw_hs) {
                if (awready_i && *(port.awvalid)) {
                    aw_hs = true;
                    waddr_clr();
                }
            }

            // W Handshake
            if (!w_hs) {
                if (wready_i && *(port.wvalid)) {
                    w_beat_count++;
                    if (w_beat_count > t.len) {
                        w_hs = true;
                        wdata_clr();
                    } else {
                        // Next beat
                        bool last = (w_beat_count == t.len);
                        wdata_set(t.data, w_beat_count, last, t.id);
                    }
                }
            }

            // B Handshake
            if (aw_hs && w_hs) {
                if (!b_hs) {
                    rresp_set();
                    if (bvalid_i && *(port.bready)) {
                        b_hs = true;
                        log.info("[AXI-MST] ", burst_to_string(t.burst), " WR success !");

                        log.info("ADDR:0x", std::hex, t.addr,
                           "  LEN:", std::dec, t.len,
                           "  SIZE:", t.data.size(),
                           "  ID:0x", std::hex, t.id);
                        log.hexdump(t.data, t.addr);

                        rresp_clr();
                        wr_q.pop(); // Transaction done
                        wr_active = false; // Reset active flag
                    }
                }
            }
        }

        // Read Channel
        if (!rd_active && !rd_q.empty()) {
            rd_active = true;
            ar_hs = false;
            r_hs = false;
            r_beat_count = 0;
            current_rd_burst.clear();

            read_trans& t = rd_q.front();
            raddr_set(t.addr, t.len, t.burst, t.id);
        } else if (rd_active) {
            read_trans& t = rd_q.front();

            // AR Handshake
            bool ar_hs_edge = false;
            if (!ar_hs) {
                if (arready_i && *(port.arvalid)) {
                    ar_hs = true;
                    ar_hs_edge = true;
                    rdata_set();
                    raddr_clr();
                }
            }

            // R Handshake
            if (ar_hs && !ar_hs_edge) {
                if (!r_hs) {
                    if (rvalid_i && *(port.rready)) {
                        size_t bytes_per_beat = DATA_WIDTH/8;
                        std::vector<uint8_t> beat_data;
                        signal_get(&rdata_i, beat_data, bytes_per_beat);
                        current_rd_burst.insert(current_rd_burst.end(), beat_data.begin(), beat_data.end());

                        if (rlast_i) {
                            r_hs = true;

                            if (current_rd_burst.size() > t.size) {
                                current_rd_burst.resize(t.size);
                            }

                            rd_data_q.push(current_rd_burst);
                            log.info("[AXI-MST] ", burst_to_string(t.burst), " RD success !");

                            log.info("ADDR:0x", std::hex, t.addr,
                                      "  LEN:", std::dec, t.len,
                                      "  SIZE:", current_rd_burst.size(),
                                      "  ID:0x", std::hex, t.id);
                            log.hexdump(current_rd_burst, t.addr);

                            rdata_clr();
                            rd_active = false;
                            rd_q.pop();
                        }
                    }
                }
            }
        }
    }

private:
    struct write_trans {
        uint64_t addr;
        std::vector<uint8_t> data;
        uint32_t len; // Burst length (0-based, so 0 means 1 beat)
        uint8_t burst; // 0=FIXED, 1=INCR, 2=WRAP
        uint32_t id;
    };

    struct read_trans {
        uint64_t addr;
        uint32_t len; // Burst length (0-based)
        uint8_t burst; // 0=FIXED, 1=INCR, 2=WRAP
        uint32_t size; // Expected size in bytes
        uint32_t id;
    };

    std::queue<write_trans> wr_q;
    std::queue<read_trans> rd_q;
    std::queue<std::vector<uint8_t>> rd_data_q;
    std::vector<uint8_t> current_rd_burst;

    bool wr_active;
    bool rd_active;

    // Handshake flags
    bool aw_hs, w_hs, b_hs, ar_hs, r_hs;

    // Burst tracking
    uint32_t w_beat_count;
    uint32_t r_beat_count;

    // Registered inputs
    bool awready_i;
    bool wready_i;
    sig_t(ID_WIDTH-1, 0) bid_i;
    uint8_t bresp_i;
    bool bvalid_i;
    bool arready_i;
    sig_t(ID_WIDTH-1, 0) rid_i;
    sig_t(DATA_WIDTH-1, 0) rdata_i;
    uint8_t rresp_i;
    bool rlast_i;
    bool rvalid_i;

    // Internal helpers
    uint8_t get_axsize() {
        size_t bytes = DATA_WIDTH / 8;
        uint8_t size = 0;
        while (bytes >>= 1) size++;
        return size;
    }

    void waddr_set(uint64_t addr, uint32_t len, uint8_t burst, uint32_t id) {
        *(port.awaddr)  = addr;
        *(port.awvalid) = true;
        *(port.awburst) = burst;
        *(port.awcache) = 0;
        *(port.awid)    = id;
        *(port.awlen)   = len;
        *(port.awlock)  = 0;
        *(port.awprot)  = 0;
        *(port.awqos)   = 0;
        *(port.awregion)= 0;
        *(port.awsize)  = get_axsize();
    }

    void waddr_clr() {
        *(port.awaddr)  = 0;
        *(port.awvalid) = false;
        *(port.awlen)   = 0;
    }

    void wdata_set(const std::vector<uint8_t>& data, uint32_t beat, bool last, uint32_t id) {
        size_t bytes_per_beat = DATA_WIDTH/8;
        size_t start_idx = beat * bytes_per_beat;

        signal_set(port.wdata, data, start_idx, bytes_per_beat);

        // Calculate strobe based on valid data bytes in this beat
        size_t bytes_in_this_beat = 0;
        if (start_idx < data.size()) {
            bytes_in_this_beat = data.size() - start_idx;
            if (bytes_in_this_beat > bytes_per_beat) bytes_in_this_beat = bytes_per_beat;
        }

        // Strobe width in bits is bytes_per_beat
        // Strobe width in bytes (for storage)
        size_t strb_vec_size = (bytes_per_beat + 7) / 8;
        std::vector<uint8_t> strb_val(strb_vec_size, 0);

        for (size_t i = 0; i < bytes_in_this_beat; ++i) {
            strb_val[i / 8] |= (1 << (i % 8));
        }

        signal_set(port.wstrb, strb_val, 0, strb_vec_size);

        *(port.wvalid)  = true;
        *(port.wlast)   = last;
        *(port.wid)     = id;
    }

    void wdata_clr() {
        signal_clr(port.wdata);
        *(port.wstrb)   = 0;
        *(port.wvalid)  = false;
        *(port.wlast)   = false;
    }

    void rresp_set() {
        *(port.bready)  = true;
    }

    void rresp_clr() {
        *(port.bready)  = false;
        wr_active = false;
    }

    void raddr_set(uint64_t addr, uint32_t len, uint8_t burst, uint32_t id) {
        *(port.araddr)  = addr;
        *(port.arvalid) = true;
        *(port.arburst) = burst;
        *(port.arcache) = 0;
        *(port.arid)    = id;
        *(port.arlen)   = len;
        *(port.arlock)  = 0;
        *(port.arprot)  = 0;
        *(port.arqos)   = 0;
        *(port.arregion)= 0;
        *(port.arsize)  = get_axsize();
    }

    void raddr_clr() {
        *(port.araddr)  = 0;
        *(port.arvalid) = false;
        *(port.arlen)   = 0;
    }

    void rdata_set() {
        *(port.rready)  = true;
    }

    void rdata_clr() {
        *(port.rready)  = false;
    }

};

#endif
