/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        pix_fmt.hpp
 * @brief       Pixel format definitions for video processing
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     Defines pixel formats used in YUV planar video processing.
 *
 * @ingroup axis_video
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2026/04/05  Initial release
 ******************************************************************************/

#ifndef PIX_FMT_HPP
#define PIX_FMT_HPP

#include "axis_video_format.hpp"
#include <cstdint>

/**
 * @brief Pixel format enumeration
 * @details Defines supported pixel formats for video processing.
 */
enum PixFmt {
    PIX_FMT_NONE = -1,
    PIX_FMT_YUV444P = 0,   ///< planar YUV 4:4:4
    PIX_FMT_YUV422P = 1,   ///< planar YUV 4:2:2
    PIX_FMT_YUV420P = 2,   ///< planar YUV 4:2:0
    PIX_FMT_RGB24 = 3,     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
    PIX_FMT_BGR24 = 4,     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
};

inline constexpr AxisPixFmt pix_fmt_axis_pack(PixFmt fmt) {
    switch (fmt) {
    case PIX_FMT_NONE:
        return AXIS_PIX_FMT_NONE;
    case PIX_FMT_RGB24:
    case PIX_FMT_BGR24:
        return AXIS_PIX_FMT_GBR;
    case PIX_FMT_YUV444P:
        return AXIS_PIX_FMT_YUV;
    case PIX_FMT_YUV422P:
    case PIX_FMT_YUV420P:
        return AXIS_PIX_FMT_YUYV;
    default:
        return AXIS_PIX_FMT_NONE;
    }
}

/** @} */ // end of group axis_video

#endif
