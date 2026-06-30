/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        axil_ptr.hpp
 * @brief       AXI4-Lite Interface Definitions
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     This module implements the interface definitions for AXI4-Lite
 *              protocol verification.
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2025/12/25  Initial release
 ******************************************************************************/

#ifndef AXIL_PTR_HPP
#define AXIL_PTR_HPP

#include "sig.hpp"
#include <cstdint>
#include <cstring>

#ifndef RESP_TYPE_T
#define RESP_TYPE_T
enum resp_type_t {
    OKAY   = 0,
    EXOKAY = 1,
    SLVERR = 2,
    DECERR = 3
};
#endif

/// @brief Check if all signal pointers are assigned and unique
template <typename T>
bool axil_check(const T& p) {
    void* ptrs[] = {
        (void*)p.awaddr, (void*)p.awprot, (void*)p.awready, (void*)p.awvalid,
        (void*)p.bready, (void*)p.bresp,  (void*)p.bvalid,
        (void*)p.wdata,  (void*)p.wready, (void*)p.wstrb,   (void*)p.wvalid,
        (void*)p.araddr, (void*)p.arprot, (void*)p.arready, (void*)p.arvalid,
        (void*)p.rdata,  (void*)p.rready, (void*)p.rresp,   (void*)p.rvalid
    };

    // Total signals: 19
    for (int i = 0; i < 19; ++i) {
        if (ptrs[i] == NULL) return false;
        for (int j = i + 1; j < 19; ++j) {
            if (ptrs[i] == ptrs[j]) return false;
        }
    }
    return true;
}

/**
 * @brief AXI4-Lite interface signals pointer structure
 */
template <
    size_t DATA_WIDTH = 32,
    size_t ADDR_WIDTH = 16
>
struct axil_ptr {
    sig_io(*awaddr , ADDR_WIDTH-1  , 0) = NULL;
    sig_io(*awprot , 2             , 0) = NULL;
    sig_io(*awready, 0             , 0) = NULL;
    sig_io(*awvalid, 0             , 0) = NULL;
    sig_io(*bready , 0             , 0) = NULL;
    sig_io(*bresp  , 1             , 0) = NULL;
    sig_io(*bvalid , 0             , 0) = NULL;
    sig_io(*wdata  , DATA_WIDTH-1  , 0) = NULL;
    sig_io(*wready , 0             , 0) = NULL;
    sig_io(*wstrb  , DATA_WIDTH/8-1, 0) = NULL;
    sig_io(*wvalid , 0             , 0) = NULL;
    sig_io(*araddr , ADDR_WIDTH-1  , 0) = NULL;
    sig_io(*arprot , 2             , 0) = NULL;
    sig_io(*arready, 0             , 0) = NULL;
    sig_io(*arvalid, 0             , 0) = NULL;
    sig_io(*rdata  , DATA_WIDTH-1  , 0) = NULL;
    sig_io(*rready , 0             , 0) = NULL;
    sig_io(*rresp  , 1             , 0) = NULL;
    sig_io(*rvalid , 0             , 0) = NULL;

    /// Check if all signal pointers are assigned
    bool check() {
        return axil_check(*this);
    }
};

template <
    size_t DATA_WIDTH = 32,
    size_t ADDR_WIDTH = 16
>
struct axil_master_ptr {
    sig_out(*awaddr , ADDR_WIDTH-1  , 0) = NULL;
    sig_out(*awprot , 2             , 0) = NULL;
    sig_in (*awready, 0             , 0) = NULL;
    sig_out(*awvalid, 0             , 0) = NULL;
    sig_out(*bready , 0             , 0) = NULL;
    sig_in (*bresp  , 1             , 0) = NULL;
    sig_in (*bvalid , 0             , 0) = NULL;
    sig_out(*wdata  , DATA_WIDTH-1  , 0) = NULL;
    sig_in (*wready , 0             , 0) = NULL;
    sig_out(*wstrb  , DATA_WIDTH/8-1, 0) = NULL;
    sig_out(*wvalid , 0             , 0) = NULL;
    sig_out(*araddr , ADDR_WIDTH-1  , 0) = NULL;
    sig_out(*arprot , 2             , 0) = NULL;
    sig_in (*arready, 0             , 0) = NULL;
    sig_out(*arvalid, 0             , 0) = NULL;
    sig_in (*rdata  , DATA_WIDTH-1  , 0) = NULL;
    sig_out(*rready , 0             , 0) = NULL;
    sig_in (*rresp  , 1             , 0) = NULL;
    sig_in (*rvalid , 0             , 0) = NULL;

    axil_master_ptr() = default;
    axil_master_ptr(const axil_ptr<DATA_WIDTH, ADDR_WIDTH>& port) {
        awaddr  = port.awaddr;
        awprot  = port.awprot;
        awready = port.awready;
        awvalid = port.awvalid;
        bready  = port.bready;
        bresp   = port.bresp;
        bvalid  = port.bvalid;
        wdata   = port.wdata;
        wready  = port.wready;
        wstrb   = port.wstrb;
        wvalid  = port.wvalid;
        araddr  = port.araddr;
        arprot  = port.arprot;
        arready = port.arready;
        arvalid = port.arvalid;
        rdata   = port.rdata;
        rready  = port.rready;
        rresp   = port.rresp;
        rvalid  = port.rvalid;
    }

    /// Check if all signal pointers are assigned
    bool check() {
        return axil_check(*this);
    }
};

template <
    size_t DATA_WIDTH = 32,
    size_t ADDR_WIDTH = 16
>
struct axil_slave_ptr {
    sig_in (*awaddr , ADDR_WIDTH-1  , 0) = NULL;
    sig_in (*awprot , 2             , 0) = NULL;
    sig_out(*awready, 0             , 0) = NULL;
    sig_in (*awvalid, 0             , 0) = NULL;
    sig_in (*bready , 0             , 0) = NULL;
    sig_out(*bresp  , 1             , 0) = NULL;
    sig_out(*bvalid , 0             , 0) = NULL;
    sig_in (*wdata  , DATA_WIDTH-1  , 0) = NULL;
    sig_out(*wready , 0             , 0) = NULL;
    sig_in (*wstrb  , DATA_WIDTH/8-1, 0) = NULL;
    sig_in (*wvalid , 0             , 0) = NULL;
    sig_in (*araddr , ADDR_WIDTH-1  , 0) = NULL;
    sig_in (*arprot , 2             , 0) = NULL;
    sig_out(*arready, 0             , 0) = NULL;
    sig_in (*arvalid, 0             , 0) = NULL;
    sig_out(*rdata  , DATA_WIDTH-1  , 0) = NULL;
    sig_in (*rready , 0             , 0) = NULL;
    sig_out(*rresp  , 1             , 0) = NULL;
    sig_out(*rvalid , 0             , 0) = NULL;

    axil_slave_ptr() = default;
    axil_slave_ptr(const axil_ptr<DATA_WIDTH, ADDR_WIDTH>& port) {
        awaddr  = port.awaddr;
        awprot  = port.awprot;
        awready = port.awready;
        awvalid = port.awvalid;
        bready  = port.bready;
        bresp   = port.bresp;
        bvalid  = port.bvalid;
        wdata   = port.wdata;
        wready  = port.wready;
        wstrb   = port.wstrb;
        wvalid  = port.wvalid;
        araddr  = port.araddr;
        arprot  = port.arprot;
        arready = port.arready;
        arvalid = port.arvalid;
        rdata   = port.rdata;
        rready  = port.rready;
        rresp   = port.rresp;
        rvalid  = port.rvalid;
    }

    /// Check if all signal pointers are assigned
    bool check() {
        return axil_check(*this);
    }
};

#endif
