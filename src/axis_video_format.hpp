/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        axis_video_format.hpp
 * @brief       AXI Stream Video Format Definitions
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     Defines pixel formats, bits per channel, pixels per cycle,
 *              and validation macros for AXI4-Stream video interfaces.
 *
 * @ingroup axis_video
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2025/12/30  Initial release
 ******************************************************************************/

#ifndef AXIS_VIDEO_FORMAT_HPP
#define AXIS_VIDEO_FORMAT_HPP

#include <cstdint>

/**
 * @brief Axis pixel format types
 *       PPC=4
 *      AXIS Data     144                                0
 * AXIS_PIX_FMT_YUV:   V3 U3 Y3 V2 U2 Y2 V1 U1 Y1 V0 U0 Y0
 * AXIS_PIX_FMT_GBR:   R3 B3 G3 R2 B2 G2 R1 B1 G1 R0 B0 G0
 * AXIS_PIX_FMT_YUYV:  -  -  -  -  V1 Y3 U1 Y2 V1 Y1 U0 Y0
 *
 *       PPC=2
 *      AXIS Data      72              0
 * AXIS_PIX_FMT_YUV:   V1 U1 Y1 V0 U0 Y0
 * AXIS_PIX_FMT_GBR:   R1 B1 G1 R0 B0 G0
 * AXIS_PIX_FMT_YUYV:  -  -  V1 Y1 U0 Y0
 *
 * AXIS_PIX_FMT_YUYV format for YUV420:
 *     PPC=4
 *    AXIS Data                    96                    0
 *  YUV420 Even Lines(first line)  V1 Y2 U1 Y1 V0 Y1 U0 Y0
 *  YUV420 Odd Lines               z  Y2 z  Y1 z  Y1 z  Y0
 *
 *    PPC=2
 *    AXIS Data                    48        0
 *  YUV420 Even Lines(first line)  V0 Y1 U0 Y0
 *  YUV420 Odd Lines               z  Y1 z  Y0
 */

enum AxisPixFmt : uint32_t {
    AXIS_PIX_FMT_NONE = 0,
    AXIS_PIX_FMT_YUV  = 1,
    AXIS_PIX_FMT_YUYV = 2,
    AXIS_PIX_FMT_GBR  = 3
};

/**
 * @brief Bits per color
 */
enum BpcFmt : uint32_t {
    BPC_FMT_NONE = 0,
    BPC_FMT_8 = 8,
    BPC_FMT_10 = 10,
    BPC_FMT_12 = 12
};

/**
 * @brief Pixels per clock
 */
enum PpcFmt : uint32_t {
    PPC_FMT_NONE = 0,
    PPC_FMT_1 = 1,
    PPC_FMT_2 = 2,
    PPC_FMT_4 = 4
};

/**
 * @brief Static assertion macros for BPC and PPC validation
 */
#define static_assert_bpc(bpc) static_assert((bpc) == BPC_FMT_8 || (bpc) == BPC_FMT_10 || (bpc) == BPC_FMT_12, "BPC must be 8, 10, or 12")
#define static_assert_ppc(ppc) static_assert((ppc) == PPC_FMT_1 || (ppc) == PPC_FMT_2 || (ppc) == PPC_FMT_4, "PPC must be 1, 2, or 4")

/** @} */ // end of group axis_video

#endif
