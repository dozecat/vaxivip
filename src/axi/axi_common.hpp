/******************************************************************************
 * Copyright (C) 2025 dozecat. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * @file        axi_common.hpp
 * @brief       AXI4 VIP Common Helpers
 * @see         https://github.com/dozecat/vaxivip
 *
 * @details     Common helper functions for AXI4 VIP.
 *
 * Modification History:
 * Ver   Who  Date        Changes
 * ----  ---- ----------  -----------------------------------------------------
 * 1.0        2025/12/30  Initial release
 ******************************************************************************/

#ifndef AXI_COMMON_HPP
#define AXI_COMMON_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <verilated.h>

// Generic implementation for primitive types (CData, SData, IData, QData)
template <typename T>
void signal_set(T* sig, const std::vector<uint8_t>& data, size_t start_byte, size_t num_bytes) {
    uint64_t val = 0;
    for (size_t i = 0; i < num_bytes && i < 8; ++i) {
        if (start_byte + i < data.size()) {
            val |= ((uint64_t)data[start_byte + i] << (i * 8));
        }
    }
    *sig = (T)val;
}

template <typename T>
void signal_get(const T* sig, std::vector<uint8_t>& data, size_t num_bytes) {
    uint64_t val = *sig;
    data.reserve(data.size() + num_bytes);
    for (size_t i = 0; i < num_bytes; ++i) {
        data.push_back((val >> (i * 8)) & 0xFF);
    }
}

template <typename T>
void signal_clr(T* sig) {
    *sig = 0;
}

// Overloads for VlWide
template <std::size_t W>
void signal_set(VlWide<W>* sig, const std::vector<uint8_t>& data, size_t start_byte, size_t num_bytes) {
    int N = sizeof((*sig).m_storage) / sizeof(uint32_t);
    for (int w = 0; w < N; ++w) {
        uint32_t word_val = 0;
        for (int b = 0; b < 4; ++b) {
            size_t byte_idx = w * 4 + b;
            if (byte_idx < num_bytes) {
                if (start_byte + byte_idx < data.size()) {
                    word_val |= ((uint32_t)data[start_byte + byte_idx] << (b * 8));
                }
            }
        }
        (*sig).m_storage[w] = word_val;
    }
}

template <std::size_t W>
void signal_get(const VlWide<W>* sig, std::vector<uint8_t>& data, size_t num_bytes) {
    int N = sizeof((*sig).m_storage) / sizeof(uint32_t);
    data.reserve(data.size() + num_bytes);
    for (size_t i = 0; i < num_bytes; ++i) {
        int w = i / 4;
        int b = i % 4;
        if (w < N) {
            uint32_t word_val = (*sig).m_storage[w];
            data.push_back((word_val >> (b * 8)) & 0xFF);
        } else {
            data.push_back(0);
        }
    }
}

template <std::size_t W>
void signal_clr(VlWide<W>* sig) {
    int N = sizeof((*sig).m_storage) / sizeof(uint32_t);
    for(int i=0; i<N; ++i) (*sig).m_storage[i] = 0;
}

// Helper to convert burst type to string
static std::string burst_to_string(uint8_t b) {
    switch(b) {
        case 0: return "FIXED";
        case 1: return "INCR";
        case 2: return "WRAP";
        default: return "RSVD";
    }
}

#endif
