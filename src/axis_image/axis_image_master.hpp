/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        axis_image_master.hpp
 * @brief       AXI Stream Image Master (BMP to AXI Stream)
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     Loads BMP and drives AXI4-Stream image data with configurable
 *              bits per channel and pixels per cycle.
 *
 * @ingroup axis_image
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2025/12/30  Initial release
 ******************************************************************************/

#ifndef AXIS_IMAGE_MASTER_HPP
#define AXIS_IMAGE_MASTER_HPP

#include "axis_master.hpp"
#include "bmp.hpp"
#include "image_info.hpp"
#include "log.hpp"
#include "axis_video_format.hpp"
#include <string>
#include <queue>
#include <cstring>

/**
 * @brief AXI4-Stream image master for BMP to stream conversion
 * @tparam BPC Bits per channel (default: 8)
 * @tparam PPC Pixels per cycle (default: 4)
 */
template <
    uint32_t BPC = 8,
    uint32_t PPC = 4
>
class axis_image_master {
public:
    static constexpr uint32_t DATA_WIDTH = 3 * PPC * BPC;
    static constexpr uint32_t TKEEP_WIDTH = DATA_WIDTH / 8;
    static constexpr uint32_t USER_WIDTH = 1;

    static_assert_bpc(BPC);
    static_assert_ppc(PPC);

    axis_master<DATA_WIDTH, 1, 1, USER_WIDTH> axis_mst;
    Log log;
    Bitmap bmp;
    ImageInfo* image_info;
    uint32_t pixel_idx = 0;
    bool sending = false;

    /**
     * @brief Constructor
     * @param port AXI4-Stream master interface pointer
     */
    axis_image_master(const axis_master_ptr<DATA_WIDTH, 1, 1, USER_WIDTH>& port)
        : axis_mst(port) {
        axis_mst.log.quiet = true;
        log.quiet = true;
        image_info = &bmp.image_info;
    }

    /// @brief Load BMP file into internal buffer
    /// @param filename Path to BMP file to load
    /// @return true if BMP loaded successfully, false otherwise
    bool read_file(const std::string& filename) {
        bool success = bmp.read(filename);
        if (success) {
            log.info("[IMAGE-MST] Loaded BMP: ", filename,
                     " (", image_info->width, "x", image_info->height, ")");
        } else {
            log.error("[IMAGE-MST] Failed to load BMP: ", filename);
        }
        return success;
    }

    /// @brief Load and start sending the loaded frame over AXI4-Stream
    /// @param filename Path to BMP file to load
    /// @param info Image information pointer to fill
    void send_frame(const std::string& filename, ImageInfo* info) {
        read_file(filename);
        if (info != nullptr) {
            info->width = image_info->width;
            info->height = image_info->height;
        }
        next_line = 0;
        sending = true;
    }

    /// @brief Check if end-of-frame reached (sending completed)
    /// @return true if sending is complete, false if still sending
    bool eof() const {
        return !sending;
    }

    /// @brief Check if image sending is in progress
    /// @return true if image is currently being sent, false otherwise
    bool busy() const {
        return sending;
    }

    /// @brief Update registered inputs from DUT
    void update_input() {
        axis_mst.update_input();
    }

    /// @brief Drive outputs to DUT and check if sending complete
    void update_output() {
        axis_mst.update_output();
        if (!sending) return;

        // Feed image lines on demand instead of pre-queueing the whole frame.
        // Keep one line queued ahead so consecutive line packets have no gap.
        while (next_line < image_info->height && axis_mst.tx_queue.empty()) {
            std::vector<uint8_t> line_data;
            pack_line(next_line, line_data);
            axis_mst.send(std::move(line_data), 0, 0, 0, next_line == 0);
            ++next_line;
        }

        if (next_line >= image_info->height &&
            axis_mst.tx_buf.empty() && axis_mst.tx_queue.empty()) {
            sending = false;
            log.info("[IMAGE-MST] Image send complete. Resolution=",
                     image_info->width, "x", image_info->height);
        }
    }

private:
    static constexpr uint32_t BYTES_PER_BEAT = DATA_WIDTH / 8u;
    static constexpr uint32_t COMP_PER_BEAT = 3u * PPC;

    uint16_t sample_to_axis(uint8_t sample) const {
        constexpr uint32_t cd = 8;
        constexpr uint32_t mask = (1u << cd) - 1u;
        return static_cast<uint16_t>((static_cast<uint32_t>(sample) & mask) << (BPC - cd));
    }

    static void pack_beat(uint8_t* beat, const uint16_t* comp, uint32_t ncomp) {
        std::memset(beat, 0, BYTES_PER_BEAT);
        constexpr uint32_t mask = (BPC >= 32u) ? 0xFFFFFFFFu : ((1u << BPC) - 1u);
        for (uint32_t k = 0; k < ncomp; ++k) {
            const uint32_t val = static_cast<uint32_t>(comp[k]) & mask;
            const uint32_t bit = k * BPC;
            const uint32_t byi = bit >> 3;
            const uint32_t sh = bit & 7u;
            const uint32_t word = val << sh;
            const uint32_t nbytes = (sh + BPC + 7u) >> 3;
            for (uint32_t b = 0; b < nbytes; ++b)
                beat[byi + b] |= static_cast<uint8_t>(word >> (8u * b));
        }
    }

    uint32_t next_line = 0;

    /// @brief Pack a single image line into an AXI4-Stream packet
    void pack_line(uint32_t y, std::vector<uint8_t>& line_data) {
        const uint32_t nbeats = (image_info->width + PPC - 1u) / PPC;
        line_data.resize(static_cast<size_t>(nbeats) * BYTES_PER_BEAT);

        uint16_t comp[3u * 4u];
        for (uint32_t b = 0; b < nbeats; b++) {
            for (uint32_t p = 0; p < PPC; p++) {
                const uint32_t x = b * PPC + p;
                if (x < image_info->width) {
                    const uint32_t pixel = bmp.get_pixel(x, y);
                    comp[p * 3u + 0u] = sample_to_axis(static_cast<uint8_t>((pixel >> 16) & 0xFF));
                    comp[p * 3u + 1u] = sample_to_axis(static_cast<uint8_t>((pixel >> 8) & 0xFF));
                    comp[p * 3u + 2u] = sample_to_axis(static_cast<uint8_t>(pixel & 0xFF));
                } else {
                    comp[p * 3u + 0u] = 0;
                    comp[p * 3u + 1u] = 0;
                    comp[p * 3u + 2u] = 0;
                }
            }
            uint8_t beat[sizeof(uint64_t) * 4];
            pack_beat(beat, comp, COMP_PER_BEAT);
            std::memcpy(line_data.data() + static_cast<size_t>(b) * BYTES_PER_BEAT,
                        beat, BYTES_PER_BEAT);
        }
    }
};

#endif
