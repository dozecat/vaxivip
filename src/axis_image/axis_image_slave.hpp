/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        axis_image_slave.hpp
 * @brief       AXI Stream Image Slave (AXI Stream to BMP)
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     Receives AXI4-Stream image by preset resolution and writes BMP with
 *              configurable bits per channel and pixels per cycle.
 *
 * @ingroup axis_image
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2025/12/30  Initial release
 ******************************************************************************/

#ifndef AXIS_IMAGE_SLAVE_HPP
#define AXIS_IMAGE_SLAVE_HPP

#include "axis_slave.hpp"
#include "bmp.hpp"
#include "log.hpp"
#include "axis_video_format.hpp"
#include <string>
#include <queue>

/**
 * @brief AXI4-Stream image slave for stream to BMP conversion
 * @tparam BPC Bits per channel (default: 8)
 * @tparam PPC Pixels per cycle (default: 4)
 */
template <
    uint32_t BPC = 8,
    uint32_t PPC = 4
>
class axis_image_slave {
public:
    static constexpr uint32_t DATA_WIDTH = 3 * PPC * BPC;
    static constexpr uint32_t TKEEP_WIDTH = DATA_WIDTH / 8;
    static constexpr uint32_t USER_WIDTH = 1;

    static_assert_bpc(BPC);
    static_assert_ppc(PPC);

    axis_slave<DATA_WIDTH, 1, 1, USER_WIDTH> axis_slv;

    Log log;

    /// @brief Constructor
    /// @param port AXI4-Stream slave interface pointer
    axis_image_slave(const axis_slave_ptr<DATA_WIDTH, 1, 1, USER_WIDTH>& port)
        : axis_slv(port) {
        axis_slv.log.quiet = true;
        image_info = &bmp.image_info;
        pixel_idx = 0;
        busy = false;
    }

    /// @brief Prepare to receive an image
    /// @param info Image information pointer
    /// @param fname Output filename for saving BMP
    void recv_frame(ImageInfo* info, const std::string& fname) {
        image_info = info;
        filename = fname;
        pixel_idx = 0;
        busy = true;
        bmp.create(info->width, info->height);
        log.info("[IMAGE-SLV] Ready to receive image: ",
                 info->width, "x", info->height);
    }

    /// @brief Check if image reception is in progress
    /// @return true if image is currently being received, false otherwise
    bool is_busy() const {
        return busy;
    }

    /// @brief Check if end-of-frame reached (reception completed)
    /// @return true if reception is complete, false if still busy
    bool eof() const {
        return !busy;
    }

    /// @brief Check if receive queue is empty
    /// @return true if no completed transactions are queued, false otherwise
    bool empty() {
        return axis_slv.empty();
    }

    /// @brief Set TREADY drive value
    /// @param ready TREADY value to drive (true = ready, false = not ready)
    void set_tready(bool ready) {
        axis_slv.set_tready(ready);
    }

    /// @brief Update registered inputs from DUT
    void update_input() {
        axis_slv.update_input();
    }

    /// @brief Drive outputs to DUT and process received data
    void update_output() {
        axis_slv.update_output();
        if (!busy) return;

        const uint32_t total_pixels = image_info->width * image_info->height;
        std::vector<uint8_t> data;
        ssize_t size = axis_slv.recv(data);
        if (size <= 0) return;

        uint32_t bpp = 3 * ((BPC + 7) / 8);
        if (bpp == 0) bpp = 3;

        const uint32_t sz = static_cast<uint32_t>(size);
        bool pkt_ok = true;

        if (sz % bpp != 0) {
            log.error("[IMAGE-SLV] packet size not multiple of pixel stride: size=", sz, " bpp=", bpp);
            pkt_ok = false;
        }

        const uint32_t line_b = image_info->width * bpp;
        if (line_b == 0) {
            busy = false;
            return;
        }

        if ((pixel_idx % image_info->width) != 0) {
            log.error("[IMAGE-SLV] packet not at row boundary (preset W=", image_info->width,
                      " pixel_idx=", pixel_idx, ")");
            pkt_ok = false;
        } else {
            const uint32_t rows_left = (total_pixels - pixel_idx) / image_info->width;
            if (sz % line_b != 0) {
                log.error("[IMAGE-SLV] line width mismatch preset: bytes=", sz,
                          " expect multiple of ", line_b, " (W=", image_info->width, " bpp=", bpp, ")");
                pkt_ok = false;
            } else {
                const uint32_t nlines = sz / line_b;
                if (nlines > rows_left) {
                    log.error("[IMAGE-SLV] row count exceeds preset: packet_lines=", nlines,
                              " rows_left=", rows_left, " (preset H=", image_info->height, ")");
                    pkt_ok = false;
                }
            }
        }

        if (!pkt_ok) {
            log.error("[IMAGE-SLV] frame recv aborted (preset ", image_info->width, "x", image_info->height, ")");
            busy = false;
            return;
        }

        const uint32_t p0 = pixel_idx;
        for (uint32_t off = 0; off + bpp - 1 < static_cast<uint32_t>(size) && pixel_idx < total_pixels;
             off += bpp) {
            uint32_t x = pixel_idx % image_info->width;
            uint32_t y = pixel_idx / image_info->width;
            uint8_t  r = data[off + 0];
            uint8_t  g = data[off + 1];
            uint8_t  b = data[off + 2];
            uint32_t color = (0xFFu << 24) | (static_cast<uint32_t>(r) << 16) |
                             (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
            bmp.set_pixel(x, y, color);
            pixel_idx++;
        }

        const uint32_t consumed_b = (pixel_idx - p0) * bpp;
        if (consumed_b < static_cast<uint32_t>(size)) {
            log.error("[IMAGE-SLV] extra data vs preset ", image_info->width, "x", image_info->height, ": ",
                      static_cast<uint32_t>(size) - consumed_b, " bytes");
        }

        if (pixel_idx >= total_pixels) {
            busy = false;
            bmp.write(filename);
            log.info("[IMAGE-SLV] Image receive complete. Resolution=",
                     image_info->width, "x", image_info->height);
        }
    }

private:
    Bitmap bmp;
    ImageInfo* image_info;
    std::string filename;
    uint32_t pixel_idx = 0;
    bool busy = false;

public:
    /// @brief Write received image to BMP file
    /// @param filename Path to output BMP file
    /// @return true if BMP saved successfully, false otherwise
    bool write_file(const std::string& filename) {
        bool success = bmp.write(filename);
        if (success) {
            log.info("[IMAGE-SLV] Saved BMP: ", filename);
        } else {
            log.error("[IMAGE-SLV] Failed to save BMP: ", filename);
        }
        return success;
    }
};

#endif
