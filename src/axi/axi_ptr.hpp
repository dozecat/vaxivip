/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        axi_ptr.hpp
 * @brief       AXI4 Interface Definitions
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     This module implements the interface definitions for AXI4
 *              protocol verification.
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2025/12/26  Initial release
 ******************************************************************************/

#ifndef AXI_PTR_HPP
#define AXI_PTR_HPP

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
bool axi_check(const T& p) {
    void* ptrs[] = {
        (void*)p.awaddr, (void*)p.awburst, (void*)p.awcache,
        (void*)p.awid,   (void*)p.awlen,   (void*)p.awlock,
        (void*)p.awprot, (void*)p.awqos,   (void*)p.awready,
        (void*)p.awregion, (void*)p.awsize, (void*)p.awvalid,

        (void*)p.wdata,  (void*)p.wid,     (void*)p.wlast,
        (void*)p.wready, (void*)p.wstrb,   (void*)p.wvalid,

        (void*)p.bid,    (void*)p.bready,  (void*)p.bresp,
        (void*)p.bvalid,

        (void*)p.araddr, (void*)p.arburst, (void*)p.arcache,
        (void*)p.arid,   (void*)p.arlen,   (void*)p.arlock,
        (void*)p.arprot, (void*)p.arqos,   (void*)p.arready,
        (void*)p.arregion, (void*)p.arsize, (void*)p.arvalid,

        (void*)p.rdata,  (void*)p.rid,     (void*)p.rlast,
        (void*)p.rready, (void*)p.rresp,   (void*)p.rvalid
    };

    // Total signals: 12 (AW) + 6 (W) + 4 (B) + 12 (AR) + 6 (R) = 40
    for (int i = 0; i < 40; ++i) {
        if (ptrs[i] == NULL) return false;
        for (int j = i + 1; j < 40; ++j) {
            if (ptrs[i] == ptrs[j]) return false;
        }
    }
    return true;
}

/// @brief AXI Interface signals pointer structure
/// Holds pointers to the actual Verilator signals
template <
    size_t DATA_WIDTH = 32,
    size_t ADDR_WIDTH = 32,
    size_t ID_WIDTH = 16
>
struct axi_ptr {
    // Constants for signal widths
    static constexpr size_t LEN_WIDTH  = 8;
    static constexpr size_t LOCK_WIDTH = 1;
    static constexpr size_t QOS_WIDTH  = 4;
    static constexpr size_t REGION_WIDTH = 4;

    // Write Address Channel
    sig_io(*awaddr   , ADDR_WIDTH-1  , 0) = NULL; ///< Write address
    sig_io(*awburst  , 1             , 0) = NULL; ///< Burst type
    sig_io(*awcache  , 3             , 0) = NULL; ///< Cache type
    sig_io(*awid     , ID_WIDTH-1    , 0) = NULL; ///< Write address ID
    sig_io(*awlen    , LEN_WIDTH-1   , 0) = NULL; ///< Burst length
    sig_io(*awlock   , LOCK_WIDTH-1  , 0) = NULL; ///< Lock type
    sig_io(*awprot   , 2             , 0) = NULL; ///< Protection type
    sig_io(*awqos    , QOS_WIDTH-1   , 0) = NULL; ///< Quality of Service
    sig_io(*awready  , 0             , 0) = NULL; ///< Write address ready
    sig_io(*awregion , REGION_WIDTH-1, 0) = NULL; ///< Region identifier
    sig_io(*awsize   , 2             , 0) = NULL; ///< Burst size
    sig_io(*awvalid  , 0             , 0) = NULL; ///< Write address valid

    // Write Data Channel
    sig_io(*wdata    , DATA_WIDTH-1  , 0) = NULL; ///< Write data
    sig_io(*wid      , ID_WIDTH-1    , 0) = NULL; ///< Write ID
    sig_io(*wlast    , 0             , 0) = NULL; ///< Write last
    sig_io(*wready   , 0             , 0) = NULL; ///< Write ready
    sig_io(*wstrb    , DATA_WIDTH/8-1, 0) = NULL; ///< Write strobes
    sig_io(*wvalid   , 0             , 0) = NULL; ///< Write valid

    // Write Response Channel
    sig_io(*bid      , ID_WIDTH-1    , 0) = NULL; ///< Response ID
    sig_io(*bready   , 0             , 0) = NULL; ///< Response ready
    sig_io(*bresp    , 1             , 0) = NULL; ///< Write response
    sig_io(*bvalid   , 0             , 0) = NULL; ///< Response valid

    // Read Address Channel
    sig_io(*araddr   , ADDR_WIDTH-1  , 0) = NULL; ///< Read address
    sig_io(*arburst  , 1             , 0) = NULL; ///< Burst type
    sig_io(*arcache  , 3             , 0) = NULL; ///< Cache type
    sig_io(*arid     , ID_WIDTH-1    , 0) = NULL; ///< Read address ID
    sig_io(*arlen    , LEN_WIDTH-1   , 0) = NULL; ///< Burst length
    sig_io(*arlock   , LOCK_WIDTH-1  , 0) = NULL; ///< Lock type
    sig_io(*arprot   , 2             , 0) = NULL; ///< Protection type
    sig_io(*arqos    , QOS_WIDTH-1   , 0) = NULL; ///< Quality of Service
    sig_io(*arready  , 0             , 0) = NULL; ///< Read address ready
    sig_io(*arregion , REGION_WIDTH-1, 0) = NULL; ///< Region identifier
    sig_io(*arsize   , 2             , 0) = NULL; ///< Burst size
    sig_io(*arvalid  , 0             , 0) = NULL; ///< Read address valid

    // Read Data Channel
    sig_io(*rdata    , DATA_WIDTH-1  , 0) = NULL; ///< Read data
    sig_io(*rid      , ID_WIDTH-1    , 0) = NULL; ///< Read ID
    sig_io(*rlast    , 0             , 0) = NULL; ///< Read last
    sig_io(*rready   , 0             , 0) = NULL; ///< Read ready
    sig_io(*rresp    , 1             , 0) = NULL; ///< Read response
    sig_io(*rvalid   , 0             , 0) = NULL; ///< Read valid

    /// @brief Check if all signal pointers are assigned
    /// @return true if all signals are non-NULL
    bool check() {
        return axi_check(*this);
    }
};

template <
    size_t DATA_WIDTH = 32,
    size_t ADDR_WIDTH = 32,
    size_t ID_WIDTH = 16
>
struct axi_master_ptr {
    // Constants for signal widths
    static constexpr size_t LEN_WIDTH  = 8;
    static constexpr size_t LOCK_WIDTH = 1;
    static constexpr size_t QOS_WIDTH  = 4;
    static constexpr size_t REGION_WIDTH = 4;

    // Write Address Channel
    sig_out(*awaddr   , ADDR_WIDTH-1  , 0) = NULL;
    sig_out(*awburst  , 1             , 0) = NULL;
    sig_out(*awcache  , 3             , 0) = NULL;
    sig_out(*awid     , ID_WIDTH-1    , 0) = NULL;
    sig_out(*awlen    , LEN_WIDTH-1   , 0) = NULL;
    sig_out(*awlock   , LOCK_WIDTH-1  , 0) = NULL;
    sig_out(*awprot   , 2             , 0) = NULL;
    sig_out(*awqos    , QOS_WIDTH-1   , 0) = NULL;
    sig_in (*awready  , 0             , 0) = NULL;
    sig_out(*awregion , REGION_WIDTH-1, 0) = NULL;
    sig_out(*awsize   , 2             , 0) = NULL;
    sig_out(*awvalid  , 0             , 0) = NULL;

    // Write Data Channel
    sig_out(*wdata    , DATA_WIDTH-1  , 0) = NULL;
    sig_out(*wid      , ID_WIDTH-1    , 0) = NULL;
    sig_out(*wlast    , 0             , 0) = NULL;
    sig_in (*wready   , 0             , 0) = NULL;
    sig_out(*wstrb    , DATA_WIDTH/8-1, 0) = NULL;
    sig_out(*wvalid   , 0             , 0) = NULL;

    // Write Response Channel
    sig_in (*bid      , ID_WIDTH-1    , 0) = NULL;
    sig_out(*bready   , 0             , 0) = NULL;
    sig_in (*bresp    , 1             , 0) = NULL;
    sig_in (*bvalid   , 0             , 0) = NULL;

    // Read Address Channel
    sig_out(*araddr   , ADDR_WIDTH-1  , 0) = NULL;
    sig_out(*arburst  , 1             , 0) = NULL;
    sig_out(*arcache  , 3             , 0) = NULL;
    sig_out(*arid     , ID_WIDTH-1    , 0) = NULL;
    sig_out(*arlen    , LEN_WIDTH-1   , 0) = NULL;
    sig_out(*arlock   , LOCK_WIDTH-1  , 0) = NULL;
    sig_out(*arprot   , 2             , 0) = NULL;
    sig_out(*arqos    , QOS_WIDTH-1   , 0) = NULL;
    sig_in (*arready  , 0             , 0) = NULL;
    sig_out(*arregion , REGION_WIDTH-1, 0) = NULL;
    sig_out(*arsize   , 2             , 0) = NULL;
    sig_out(*arvalid  , 0             , 0) = NULL;

    // Read Data Channel
    sig_in (*rdata    , DATA_WIDTH-1  , 0) = NULL;
    sig_in (*rid      , ID_WIDTH-1    , 0) = NULL;
    sig_in (*rlast    , 0             , 0) = NULL;
    sig_out(*rready   , 0             , 0) = NULL;
    sig_in (*rresp    , 1             , 0) = NULL;
    sig_in (*rvalid   , 0             , 0) = NULL;

    axi_master_ptr() = default;
    axi_master_ptr(const axi_ptr<DATA_WIDTH, ADDR_WIDTH, ID_WIDTH>& port) {
        awaddr   = port.awaddr;
        awburst  = port.awburst;
        awcache  = port.awcache;
        awid     = port.awid;
        awlen    = port.awlen;
        awlock   = port.awlock;
        awprot   = port.awprot;
        awqos    = port.awqos;
        awready  = port.awready;
        awregion = port.awregion;
        awsize   = port.awsize;
        awvalid  = port.awvalid;

        wdata    = port.wdata;
        wid      = port.wid;
        wlast    = port.wlast;
        wready   = port.wready;
        wstrb    = port.wstrb;
        wvalid   = port.wvalid;

        bid      = port.bid;
        bready   = port.bready;
        bresp    = port.bresp;
        bvalid   = port.bvalid;

        araddr   = port.araddr;
        arburst  = port.arburst;
        arcache  = port.arcache;
        arid     = port.arid;
        arlen    = port.arlen;
        arlock   = port.arlock;
        arprot   = port.arprot;
        arqos    = port.arqos;
        arready  = port.arready;
        arregion = port.arregion;
        arsize   = port.arsize;
        arvalid  = port.arvalid;

        rdata    = port.rdata;
        rid      = port.rid;
        rlast    = port.rlast;
        rready   = port.rready;
        rresp    = port.rresp;
        rvalid   = port.rvalid;
    }

    bool check() {
        return axi_check(*this);
    }
};

template <
    size_t DATA_WIDTH = 32,
    size_t ADDR_WIDTH = 32,
    size_t ID_WIDTH = 16
>
struct axi_slave_ptr {
    // Constants for signal widths
    static constexpr size_t LEN_WIDTH  = 8;
    static constexpr size_t LOCK_WIDTH = 1;
    static constexpr size_t QOS_WIDTH  = 4;
    static constexpr size_t REGION_WIDTH = 4;

    // Write Address Channel
    sig_in (*awaddr   , ADDR_WIDTH-1  , 0) = NULL;
    sig_in (*awburst  , 1             , 0) = NULL;
    sig_in (*awcache  , 3             , 0) = NULL;
    sig_in (*awid     , ID_WIDTH-1    , 0) = NULL;
    sig_in (*awlen    , LEN_WIDTH-1   , 0) = NULL;
    sig_in (*awlock   , LOCK_WIDTH-1  , 0) = NULL;
    sig_in (*awprot   , 2             , 0) = NULL;
    sig_in (*awqos    , QOS_WIDTH-1   , 0) = NULL;
    sig_out(*awready  , 0             , 0) = NULL;
    sig_in (*awregion , REGION_WIDTH-1, 0) = NULL;
    sig_in (*awsize   , 2             , 0) = NULL;
    sig_in (*awvalid  , 0             , 0) = NULL;

    // Write Data Channel
    sig_in (*wdata    , DATA_WIDTH-1  , 0) = NULL;
    sig_in (*wid      , ID_WIDTH-1    , 0) = NULL;
    sig_in (*wlast    , 0             , 0) = NULL;
    sig_out(*wready   , 0             , 0) = NULL;
    sig_in (*wstrb    , DATA_WIDTH/8-1, 0) = NULL;
    sig_in (*wvalid   , 0             , 0) = NULL;

    // Write Response Channel
    sig_out(*bid      , ID_WIDTH-1    , 0) = NULL;
    sig_in (*bready   , 0             , 0) = NULL;
    sig_out(*bresp    , 1             , 0) = NULL;
    sig_out(*bvalid   , 0             , 0) = NULL;

    // Read Address Channel
    sig_in (*araddr   , ADDR_WIDTH-1  , 0) = NULL;
    sig_in (*arburst  , 1             , 0) = NULL;
    sig_in (*arcache  , 3             , 0) = NULL;
    sig_in (*arid     , ID_WIDTH-1    , 0) = NULL;
    sig_in (*arlen    , LEN_WIDTH-1   , 0) = NULL;
    sig_in (*arlock   , LOCK_WIDTH-1  , 0) = NULL;
    sig_in (*arprot   , 2             , 0) = NULL;
    sig_in (*arqos    , QOS_WIDTH-1   , 0) = NULL;
    sig_out(*arready  , 0             , 0) = NULL;
    sig_in (*arregion , REGION_WIDTH-1, 0) = NULL;
    sig_in (*arsize   , 2             , 0) = NULL;
    sig_in (*arvalid  , 0             , 0) = NULL;

    // Read Data Channel
    sig_out(*rdata    , DATA_WIDTH-1  , 0) = NULL;
    sig_out(*rid      , ID_WIDTH-1    , 0) = NULL;
    sig_out(*rlast    , 0             , 0) = NULL;
    sig_in (*rready   , 0             , 0) = NULL;
    sig_out(*rresp    , 1             , 0) = NULL;
    sig_out(*rvalid   , 0             , 0) = NULL;

    axi_slave_ptr() = default;
    axi_slave_ptr(const axi_ptr<DATA_WIDTH, ADDR_WIDTH, ID_WIDTH>& port) {
        awaddr   = port.awaddr;
        awburst  = port.awburst;
        awcache  = port.awcache;
        awid     = port.awid;
        awlen    = port.awlen;
        awlock   = port.awlock;
        awprot   = port.awprot;
        awqos    = port.awqos;
        awready  = port.awready;
        awregion = port.awregion;
        awsize   = port.awsize;
        awvalid  = port.awvalid;

        wdata    = port.wdata;
        wid      = port.wid;
        wlast    = port.wlast;
        wready   = port.wready;
        wstrb    = port.wstrb;
        wvalid   = port.wvalid;

        bid      = port.bid;
        bready   = port.bready;
        bresp    = port.bresp;
        bvalid   = port.bvalid;

        araddr   = port.araddr;
        arburst  = port.arburst;
        arcache  = port.arcache;
        arid     = port.arid;
        arlen    = port.arlen;
        arlock   = port.arlock;
        arprot   = port.arprot;
        arqos    = port.arqos;
        arready  = port.arready;
        arregion = port.arregion;
        arsize   = port.arsize;
        arvalid  = port.arvalid;

        rdata    = port.rdata;
        rid      = port.rid;
        rlast    = port.rlast;
        rready   = port.rready;
        rresp    = port.rresp;
        rvalid   = port.rvalid;
    }

    bool check() {
        return axi_check(*this);
    }
};

#endif
