/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        frame_mem.hpp
 * @brief       Frame memory manager for YUV planar video frames
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     Provides storage and I/O for multi-frame YUV planar video data
 *              with configurable color depth and pixel format.
 *
 * @ingroup axis_video
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2026/04/05  Initial release
 ******************************************************************************/

#ifndef FRAME_MEM_HPP
#define FRAME_MEM_HPP

#include "pix_fmt.hpp"
#include "video_info.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief Frame information structure extending VideoInfo with pixel format
 * @details Contains video frame metadata including width, height, color depth,
 *          and pixel format for YUV planar storage.
 */
class FrameInfo : public VideoInfo {
public:
    PixFmt pix_fmt;
    AxisPixFmt axis_pix_fmt() const { return pix_fmt_axis_pack(pix_fmt); }
    /// Get plane width for given plane index
    uint32_t plane_width(uint32_t plane_index) const;
    /// Get plane height for given plane index
    uint32_t plane_height(uint32_t plane_index) const;
    /// Get total number of planes
    uint32_t plane_total() const;
    /// Check if plane index is valid
    bool plane_ok(uint32_t plane_index) const;
    /// Get total number of samples in plane
    std::size_t plane_samples(uint32_t plane_index) const;
};

/**
 * @brief Frame memory manager for YUV planar video storage
 * @details Provides storage and I/O operations for multi-frame YUV planar video
 *          data with configurable color depth and pixel format.
 */
class FrameMem {
public:
    /// Destructor
    ~FrameMem();
    /// Clear all frame data
    void clear();

    /// Initialize frame memory with given frame information
    bool init(const FrameInfo& info);
    /// Get current frame information
    const FrameInfo& info() const { return frame_info; }

    /// Read YUV planar frames from file
    bool read_file(const std::string& file_path, uint32_t start_frame = 0, uint32_t frame_num = 0);
    /// Write YUV planar frames to file
    bool write_file(const std::string& file_path, bool append = false) const;

    /// Read a line of samples from frame memory
    bool read_line(uint32_t frame_index, uint32_t plane_index, uint32_t y, std::vector<uint16_t>& line) const;
    /// Write a line of samples to frame memory
    bool write_line(uint32_t frame_index, uint32_t plane_index, uint32_t y, const std::vector<uint16_t>& data);

    /// Read a single pixel sample from frame memory
    uint16_t read_pixel(uint32_t frame_index, uint32_t plane_index, uint32_t x, uint32_t y) const;
    /// Write a single pixel sample to frame memory
    bool write_pixel(uint32_t frame_index, uint32_t plane_index, uint32_t x, uint32_t y, uint32_t value);

private:
    FrameInfo frame_info;
    std::array<std::vector<uint16_t>, 3> planes;

    /// Calculate memory offset for pixel access
    std::size_t plane_offset(uint32_t frame_index, uint32_t plane_index, uint32_t y, uint32_t x) const;
};

#endif
