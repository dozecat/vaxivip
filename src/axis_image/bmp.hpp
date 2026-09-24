/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        bmp.hpp
 * @brief       BMP Image Processing Class
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     BMP image read/write support for 24-bit and 32-bit uncompressed
 *              formats with bottom‑up pixel ordering. Pixel data is stored in a
 *              contiguous row-major buffer (top row first) for cache-friendly
 *              access and bulk file I/O.
 *
 * @ingroup axis_image
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2025/12/30  Initial release
 ******************************************************************************/

#ifndef BMP_HPP
#define BMP_HPP

#include "image_info.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

/**
 * @brief BMP image container with read/write capabilities
 * @details Stores pixel data in ARGB format (8 bits per channel, alpha channel optional).
 *          Supports 24‑bit and 32‑bit uncompressed BMP files with bottom‑up row order.
 *          Pixels live in a flat row-major buffer (row 0 = top row).
 */
class Bitmap {
public:
    /// @brief Image information
    ImageInfo image_info;

    /// @brief Default constructor
    Bitmap() {
        image_info.width = 0;
        image_info.height = 0;
        image_info.color_depth = COLOR_DEPTH_NONE;
        bit_count = 0;
    }

    ~Bitmap() = default;

    ImageInfo get_image_info() const {
        return image_info;
    }

    void set_image_info(const ImageInfo& info) {
        image_info = info;
    }

    /// @brief Read BMP file from disk
    /// @param filename Path to BMP file
    /// @return true if file read successfully, false otherwise
    bool read(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        uint8_t header[54];
        file.read(reinterpret_cast<char*>(header), sizeof(header));
        if (file.gcount() != static_cast<std::streamsize>(sizeof(header))) {
            return false;
        }
        if (header[0] != 'B' || header[1] != 'M') {
            return false;
        }

        uint32_t data_offset, bi_width, bi_compression;
        int32_t bi_height;
        uint16_t bi_bit_count;
        std::memcpy(&data_offset, header + 10, sizeof(data_offset));
        std::memcpy(&bi_width, header + 18, sizeof(bi_width));
        std::memcpy(&bi_height, header + 22, sizeof(bi_height));
        std::memcpy(&bi_bit_count, header + 28, sizeof(bi_bit_count));
        std::memcpy(&bi_compression, header + 30, sizeof(bi_compression));

        const bool top_down = bi_height < 0;
        image_info.width = bi_width;
        image_info.height = bi_height > 0 ? static_cast<uint32_t>(bi_height)
                                          : static_cast<uint32_t>(-bi_height);
        image_info.color_depth = COLOR_DEPTH_8; // Assuming 8 bits per channel for both 24-bit and 32-bit BMP
        bit_count = bi_bit_count;

        if (bit_count != 24 && bit_count != 32) {
            return false;
        }
        if (bi_compression != 0) {
            return false;
        }

        const uint32_t w = image_info.width;
        const uint32_t h = image_info.height;
        img_data.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0u);
        if (w == 0 || h == 0) {
            return true;
        }

        file.seekg(data_offset, std::ios::beg);
        if (!file) {
            return false;
        }

        if (bit_count == 32) {
            const uint32_t row_size = w * 4u;
            std::vector<uint8_t> row(row_size);
            for (uint32_t r = 0; r < h; ++r) {
                const uint32_t y = top_down ? r : (h - 1u - r);
                file.read(reinterpret_cast<char*>(row.data()), row_size);
                if (file.gcount() != static_cast<std::streamsize>(row_size)) {
                    return false;
                }
                uint32_t* dst = img_data.data() + static_cast<size_t>(y) * w;
                for (uint32_t x = 0; x < w; ++x) {
                    const uint8_t b = row[x * 4u + 0u];
                    const uint8_t g = row[x * 4u + 1u];
                    const uint8_t r = row[x * 4u + 2u];
                    const uint8_t a = row[x * 4u + 3u];
                    dst[x] = (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) |
                             (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
                }
            }
        } else {
            const uint32_t row_size = ((w * 3u + 3u) / 4u) * 4u;
            std::vector<uint8_t> row(row_size);
            for (uint32_t r = 0; r < h; ++r) {
                const uint32_t y = top_down ? r : (h - 1u - r);
                file.read(reinterpret_cast<char*>(row.data()), row_size);
                if (file.gcount() != static_cast<std::streamsize>(row_size)) {
                    return false;
                }
                uint32_t* dst = img_data.data() + static_cast<size_t>(y) * w;
                for (uint32_t x = 0; x < w; ++x) {
                    const uint8_t b = row[x * 3u + 0u];
                    const uint8_t g = row[x * 3u + 1u];
                    const uint8_t r = row[x * 3u + 2u];
                    dst[x] = 0xFF000000u | (static_cast<uint32_t>(r) << 16) |
                             (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
                }
            }
        }

        return true;
    }

    /// @brief Write BMP file to disk
    /// @param filename Path to output BMP file
    /// @return true if file written successfully, false otherwise
    bool write(const std::string& filename) const {
        if (image_info.width == 0 || image_info.height == 0) {
            return false;
        }

        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        const uint32_t w = image_info.width;
        const uint32_t h = image_info.height;
        const uint32_t row_size = ((w * 3u + 3u) / 4u) * 4u;
        const uint32_t image_size = row_size * h;
        const uint32_t file_size = 54u + image_size;

        uint8_t header[54] = {0};
        header[0] = 'B';
        header[1] = 'M';
        auto put32 = [&header](size_t off, uint32_t v) { std::memcpy(header + off, &v, sizeof(v)); };
        auto put16 = [&header](size_t off, uint16_t v) { std::memcpy(header + off, &v, sizeof(v)); };
        put32(2, file_size);
        put32(10, 54u);
        put32(14, 40u);
        put32(18, w);
        put32(22, h);
        put16(26, 1);
        put16(28, 24);
        put32(30, 0);
        put32(34, image_size);
        file.write(reinterpret_cast<const char*>(header), sizeof(header));

        std::vector<uint8_t> row(row_size, 0);
        for (int y = static_cast<int>(h) - 1; y >= 0; --y) {
            const uint32_t* src = img_data.data() + static_cast<size_t>(y) * w;
            std::fill(row.begin(), row.end(), 0);
            for (uint32_t x = 0; x < w; ++x) {
                const uint32_t pixel = src[x];
                row[x * 3u + 0u] = static_cast<uint8_t>(pixel & 0xFFu);
                row[x * 3u + 1u] = static_cast<uint8_t>((pixel >> 8) & 0xFFu);
                row[x * 3u + 2u] = static_cast<uint8_t>((pixel >> 16) & 0xFFu);
            }
            file.write(reinterpret_cast<const char*>(row.data()), row_size);
        }

        return true;
    }

    /// @brief Get pixel color at specified coordinates
    /// @param x X coordinate (0‑based, left to right)
    /// @param y Y coordinate (0‑based, top to bottom)
    /// @return ARGB color value (0xAARRGGBB), or 0 if out of bounds
    uint32_t get_pixel(uint32_t x, uint32_t y) const {
        if (x >= image_info.width || y >= image_info.height) {
            return 0;
        }
        return img_data[static_cast<size_t>(y) * image_info.width + x];
    }

    /// @brief Set pixel color at specified coordinates
    /// @param x X coordinate (0‑based, left to right)
    /// @param y Y coordinate (0‑based, top to bottom)
    /// @param color ARGB color value (0xAARRGGBB)
    void set_pixel(uint32_t x, uint32_t y, uint32_t color) {
        if (x >= image_info.width || y >= image_info.height) {
            return;
        }
        img_data[static_cast<size_t>(y) * image_info.width + x] = color;
    }

    /// @brief Create a new blank image with specified dimensions
    /// @param w Image width in pixels
    /// @param h Image height in pixels
    void create(uint32_t w, uint32_t h) {
        image_info.width = w;
        image_info.height = h;
        bit_count = 24;
        img_data.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0xFF000000u);
    }

    /// @brief Direct access to the flat ARGB pixel buffer (row-major, top row first)
    const std::vector<uint32_t>& pixels() const { return img_data; }
    std::vector<uint32_t>& pixels() { return img_data; }

private:
    uint16_t bit_count = 0;
    std::vector<uint32_t> img_data;
};

#endif
