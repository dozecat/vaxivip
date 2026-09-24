/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        axis_video_slave.hpp
 * @brief       AXI Stream Video Slave (AXI Stream to YUV frame buffer)
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     Receives AXI4-Stream video frames by preset resolution and writes YUV
 *              planar data with configurable bits per channel and pixels per cycle.
 *
 * @ingroup axis_video
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2026/04/05  Initial release
 ******************************************************************************/

#ifndef AXIS_VIDEO_SLAVE_HPP
#define AXIS_VIDEO_SLAVE_HPP

#include "axis_slave.hpp"
#include "frame_mem.hpp"
#include "log.hpp"
#include "axis_video_format.hpp"
#include <cstdio>
#include <string>
#include <vector>

/**
 * @brief AXI4-Stream video slave for stream to YUV frame conversion
 * @tparam BPC Bits per channel (default: 8)
 * @tparam PPC Pixels per cycle (default: 2)
 */
template <uint32_t BPC = 8, uint32_t PPC = 2>
class axis_video_slave {
public:
    static constexpr uint32_t DATA_WIDTH = 3 * PPC * BPC;
    static constexpr uint32_t USER_WIDTH = 1;

    static_assert_bpc(BPC);
    static_assert_ppc(PPC);

    Log log;
    axis_slave<DATA_WIDTH, 1, 1, USER_WIDTH> axis_slv;
    FrameInfo frame_info;
    bool busy = false;
    bool done = false;
    bool end_of_frame = false;
    bool frames_mode = false;
    uint32_t frames_received = 0;
    std::string output_file;
    bool append_to_existing = false;
    uint32_t pixel_idx = 0;

    /// @brief Constructor
    /// @param port AXI4-Stream slave interface pointer
    axis_video_slave(const axis_slave_ptr<DATA_WIDTH, 1, 1, USER_WIDTH>& port) : axis_slv(port) {
        axis_slv.log.quiet = true;
    }

    /// @brief Destructor
    ~axis_video_slave() {
        y_plane.clear();
        u_plane.clear();
        v_plane.clear();
        output_file.clear();
    }

    /// @brief Prepare to receive multiple video frames
    /// @param filename Output filename for saving YUV frames
    /// @param info Frame information (width, height, color depth, etc.)
    /// @param append Whether to append to existing file (default: false)
    void recv_frames(const std::string& filename, const FrameInfo& info, bool append = false) {
        const uint64_t pix = static_cast<uint64_t>(info.width) * static_cast<uint64_t>(info.height);
        if (info.frame_total == 0) {
            if (pix == 0 || !filename.empty()) {
                log.error("axis_video_slave recv_frames: invalid FrameInfo or filename");
                frames_mode = false;
                frames_received = 0;
                output_file.clear();
                append_to_existing = false;
                busy = false;
                done = true;
                end_of_frame = false;
                return;
            }
        }
        FrameInfo fi = info;
        if (fi.frame_total == 0)
            fi.frame_total = 1;
        const uint32_t cd = fi.color_depth;
        if (cd == 0u || cd > 16u || cd > BPC) {
            log.error("axis_video_slave recv_frames: color_depth must be 1..16 and <= BPC");
            frames_mode = false;
            frames_received = 0;
            output_file.clear();
            append_to_existing = false;
            busy = false;
            done = true;
            end_of_frame = false;
            return;
        }
        if (fi.axis_pix_fmt() == AXIS_PIX_FMT_YUYV && ((fi.width & 1u) || (PPC & 1u))) {
            log.error("axis_video_slave recv_frames: AXIS YUYV requires even width and even PPC");
            frames_mode = false;
            frames_received = 0;
            output_file.clear();
            append_to_existing = false;
            busy = false;
            done = true;
            end_of_frame = false;
            return;
        }
        if (fi.pix_fmt == PIX_FMT_YUV420P &&
            ((fi.width & 1u) || (fi.height & 1u) || (PPC & 1u))) {
            log.error("axis_video_slave recv_frames: YUV420P requires even width/height and even PPC");
            frames_mode = false;
            frames_received = 0;
            output_file.clear();
            append_to_existing = false;
            busy = false;
            done = true;
            end_of_frame = false;
            return;
        }
        frame_info = fi;
        frames_received = 0;
        output_file = filename;
        append_to_existing = append;
        frames_mode = !filename.empty();
        done = false;
        end_of_frame = false;
        start_recv_frame();
        if (frames_mode)
            log.info("[axis_video_slave] recv_frames start: file=", filename,
                     ", size=", frame_info.width, "x", frame_info.height,
                     ", frame_total=", frame_info.frame_total,
                     ", append=", append);
    }

    /// @brief Check if end-of-frame reached (reception completed)
    /// @return true if end-of-frame is reached, false otherwise
    bool eof() const { return end_of_frame; }

    /// @brief Check if receive queue is empty
    /// @return true if no completed transactions are queued, false otherwise
    bool empty() { return axis_slv.empty(); }

    /// @brief Set TREADY drive value
    /// @param ready TREADY value to drive (true = ready, false = not ready)
    void set_tready(bool ready) { axis_slv.set_tready(ready); }

    /// @brief Update registered inputs from DUT
    void update_input() { axis_slv.update_input(); }

    /// @brief Drive outputs to DUT and process received video data
    void update_output() {
        axis_slv.update_output();
        end_of_frame = false;
        if (!busy)
            return;

        const uint32_t total_pixels = frame_info.width * frame_info.height;
        constexpr uint32_t bytes_per_beat = DATA_WIDTH / 8u;
        const uint32_t line_b =
            ((frame_info.width + PPC - 1u) / PPC) * bytes_per_beat;
        std::vector<uint8_t> data;
        const ssize_t size = axis_slv.recv(data);
        if (size <= 0) {
            return;
        }
        const uint32_t sz = static_cast<uint32_t>(size);

        if (line_b == 0 || (pixel_idx % frame_info.width) != 0 || (sz % line_b != 0)) {
            log.error("axis_video_slave update_output: invalid AXIS packet, sz=", sz);
            busy = false;
            if (frames_mode) {
                frames_mode = false;
                frames_received = frame_info.frame_total;
                done = true;
            }
            end_of_frame = true;
            return;
        }

        const uint32_t rows_left = (total_pixels - pixel_idx) / frame_info.width;
        const uint32_t nlines = sz / line_b;
        if (nlines > rows_left) {
            log.error("axis_video_slave update_output: too many lines in packet, nlines=", nlines);
            busy = false;
            if (frames_mode) {
                frames_mode = false;
                frames_received = frame_info.frame_total;
                done = true;
            }
            end_of_frame = true;
            return;
        }

        for (uint32_t ln = 0; ln < nlines; ++ln) {
            const uint8_t* row = data.data() + ln * line_b;
            const uint32_t nbeats = line_b / bytes_per_beat;
            const uint32_t y_row = pixel_idx / frame_info.width;
            const AxisPixFmt afmt = frame_info.axis_pix_fmt();
            const bool y420_odd =
                (afmt == AXIS_PIX_FMT_YUYV) && (frame_info.pix_fmt == PIX_FMT_YUV420P) &&
                ((y_row & 1u) != 0u);
            for (uint32_t bt = 0; bt < nbeats; ++bt) {
                const uint8_t* bptr = row + bt * bytes_per_beat;
                for (uint32_t p = 0; p < PPC; ++p) {
                    const uint32_t x = bt * PPC + p;
                    if (x >= frame_info.width)
                        continue;
                    if (pixel_idx >= total_pixels)
                        break;
                    if (afmt == AXIS_PIX_FMT_YUYV && !y420_odd) {
                        const uint32_t pair = p / 2u;
                        const uint32_t base = pair * 4u;
                        const uint32_t yi = (p & 1u) ? (base + 2u) : (base + 0u);
                        y_plane[pixel_idx] = axis_to_sample(unpack_comp(bptr, yi));
                        u_plane[pixel_idx] = axis_to_sample(unpack_comp(bptr, base + 1u));
                        v_plane[pixel_idx] = axis_to_sample(unpack_comp(bptr, base + 3u));
                    } else if (y420_odd) {
                        const uint32_t pair = p / 2u;
                        const uint32_t base = pair * 4u;
                        const uint32_t yi = (p & 1u) ? (base + 2u) : (base + 0u);
                        y_plane[pixel_idx] = axis_to_sample(unpack_comp(bptr, yi));
                        const size_t up =
                            static_cast<size_t>(pixel_idx) - static_cast<size_t>(frame_info.width);
                        u_plane[pixel_idx] = u_plane[up];
                        v_plane[pixel_idx] = v_plane[up];
                    } else {
                        y_plane[pixel_idx] = axis_to_sample(unpack_comp(bptr, p * 3u + 0u));
                        u_plane[pixel_idx] = axis_to_sample(unpack_comp(bptr, p * 3u + 1u));
                        v_plane[pixel_idx] = axis_to_sample(unpack_comp(bptr, p * 3u + 2u));
                    }
                    ++pixel_idx;
                }
            }
        }

        if (pixel_idx >= total_pixels) {
            busy = false;
            end_of_frame = true;
            if (frames_mode) {
                const bool append = (frames_received > 0) || append_to_existing;
                if (!write_received_frame(output_file, append)) {
                    log.error("axis_video_slave update_output: write_received_frame failed, file=",
                              output_file);
                    frames_mode = false;
                    frames_received = frame_info.frame_total;
                    done = true;
                    return;
                }
                ++frames_received;
                if (frames_received < frame_info.frame_total) {
                    start_recv_frame();
                } else {
                    log.info("[axis_video_slave] recv_frames done: file=", output_file,
                             ", size=", frame_info.width, "x", frame_info.height,
                             ", frames_written=", frames_received);
                    frames_mode = false;
                    done = true;
                }
            }
        }
    }

    /// @brief Save currently received frame to YUV file
    /// @param filename Path to output YUV file
    /// @param append Whether to append to existing file
    /// @return true if frame saved successfully, false otherwise
    bool save_frame(const std::string& filename, bool append) {
        const bool ok = write_received_frame(filename, append);
        if (ok)
            log.info("[axis_video_slave] save_frame ok: file=", filename,
                     ", size=", frame_info.width, "x", frame_info.height,
                     ", frames_written=1, append=", append);
        return ok;
    }

private:
    static uint16_t unpack_comp(const uint8_t* beat, uint32_t comp_idx) {
        constexpr uint32_t mask = (BPC >= 16u) ? 0xFFFFu : ((1u << BPC) - 1u);
        const uint32_t bit = comp_idx * BPC;
        const uint32_t byi = bit >> 3;
        const uint32_t sh = bit & 7u;
        const uint32_t nbytes = (sh + BPC + 7u) >> 3;
        uint32_t word = 0;
        for (uint32_t b = 0; b < nbytes; ++b)
            word |= static_cast<uint32_t>(beat[byi + b]) << (8u * b);
        return static_cast<uint16_t>((word >> sh) & mask);
    }

    uint16_t axis_to_sample(uint16_t v) const {
        const uint32_t cd = frame_info.color_depth;
        const uint32_t mask = (cd >= 32u) ? 0xFFFFu : ((1u << cd) - 1u);
        return static_cast<uint16_t>((static_cast<uint32_t>(v) >> (BPC - cd)) & mask);
    }

    /// Write one plane to file, optionally subsampling horizontally (sub_x) and
    /// taking only every row_step-th source row (for 4:2:0 chroma).
    bool write_plane(FILE* fp, const std::vector<uint16_t>& pl, uint32_t pw, uint32_t ph,
                     uint32_t sub_x, uint32_t row_step, bool planar8) {
        const uint32_t w = frame_info.width;
        if (planar8) {
            tmp8.resize(pw);
            for (uint32_t r = 0; r < ph; ++r) {
                const size_t srow = static_cast<size_t>(r) * row_step * w;
                for (uint32_t x = 0; x < pw; ++x)
                    tmp8[x] = static_cast<uint8_t>(pl[srow + static_cast<size_t>(x) * sub_x] & 0xFFu);
                if (std::fwrite(tmp8.data(), 1, pw, fp) != pw)
                    return false;
            }
        } else {
            tmp16.resize(pw);
            for (uint32_t r = 0; r < ph; ++r) {
                const size_t srow = static_cast<size_t>(r) * row_step * w;
                for (uint32_t x = 0; x < pw; ++x)
                    tmp16[x] = pl[srow + static_cast<size_t>(x) * sub_x];
                if (std::fwrite(tmp16.data(), sizeof(uint16_t), pw, fp) != pw)
                    return false;
            }
        }
        return true;
    }

    bool write_received_frame(const std::string& filename, bool append) {
        const uint32_t w = frame_info.width;
        const uint32_t h = frame_info.height;
        if (w == 0 || h == 0)
            return false;
        FILE* fp = std::fopen(filename.c_str(), append ? "ab" : "wb");
        if (!fp) {
            log.error("axis_video_slave write_received_frame: cannot open file, file=", filename);
            return false;
        }
        const bool planar8 = frame_info.color_depth == static_cast<uint32_t>(COLOR_DEPTH_8);
        bool ok = write_plane(fp, y_plane, w, h, 1, 1, planar8);
        if (ok) {
            if (frame_info.pix_fmt == PIX_FMT_YUV422P) {
                ok = write_plane(fp, u_plane, w / 2u, h, 2, 1, planar8) &&
                     write_plane(fp, v_plane, w / 2u, h, 2, 1, planar8);
            } else if (frame_info.pix_fmt == PIX_FMT_YUV420P) {
                ok = write_plane(fp, u_plane, w / 2u, h / 2u, 2, 2, planar8) &&
                     write_plane(fp, v_plane, w / 2u, h / 2u, 2, 2, planar8);
            } else {
                ok = write_plane(fp, u_plane, w, h, 1, 1, planar8) &&
                     write_plane(fp, v_plane, w, h, 1, 1, planar8);
            }
        }
        std::fclose(fp);
        return ok;
    }

    void start_recv_frame() {
        const uint64_t pix =
            static_cast<uint64_t>(frame_info.width) * static_cast<uint64_t>(frame_info.height);
        pixel_idx = 0;
        busy = pix > 0;
        y_plane.assign(static_cast<size_t>(pix), 0);
        u_plane.assign(static_cast<size_t>(pix), 0);
        v_plane.assign(static_cast<size_t>(pix), 0);
    }
    std::vector<uint16_t> y_plane;
    std::vector<uint16_t> u_plane;
    std::vector<uint16_t> v_plane;
    std::vector<uint8_t> tmp8;
    std::vector<uint16_t> tmp16;
};

#endif
