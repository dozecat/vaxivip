![Language](https://img.shields.io/badge/Language-C++-f34b7d.svg) ![Simulation](https://img.shields.io/badge/Simulation-Verilator-007ec6.svg) ![License](https://img.shields.io/badge/License-MIT-yellow.svg)

[English](#en) | [中文](#cn)

---

<span id="en">vaxivip</span>
===========================

**vaxivip** is a lightweight AXI protocol Verification IP (VIP) library for [Verilator](https://www.veripool.org/verilator/) simulation. Written in C++, it simulates AXI Master and Slave behaviors in C++ testbenches to verify Verilog/SystemVerilog designs.

## ✨ Features

| VIP Module | Protocol | Core Features | Typical Use Case |
|------------|----------|---------------|------------------|
| **`axi`** (`src/axi/`) | AXI4 | Master/Slave interface, supports **FIXED**, **INCR**, **WRAP** burst transfers | Processor/memory bus verification |
| **`axil`** (`src/axil/`) | AXI4-Lite | Master/Slave interface | AXI4-Lite register access verification |
| **`axis`** (`src/axis/`) | AXI4-Stream | Master/Slave interface | AXI-Stream timing verification |
| **`axis_image`** (`src/axis_image/`) | AXI4-Stream | Bidirectional conversion between BMP images and AXI4-Stream, supports RGB format (Xilinx AXI4-Stream Video compliant) | Image processing IP verification |
| **`axis_video`** (`src/axis_video/`) | AXI4-Stream | Bidirectional conversion between planar video frames and AXI4-Stream, supports YUV444/422/420 formats (Xilinx AXI4-Stream Video compliant) | Video processing IP verification |
| **`clock_gen` / `sim_coop`** (`src/`) | Simulation Utility | Software clock generation + protothread-based cooperative scheduler for multi-clock simulation | Verilator testbench with async clock domains |

## 🚀 Quick Start

### 1. Install Dependencies
```bash
# Ubuntu/Debian
sudo apt-get install verilator g++ make

# macOS (with Homebrew)
brew install verilator
```

### 2. Clone and Explore
```bash
git clone https://github.com/dozecat/vaxivip.git
cd vaxivip
```



### 3. Compile and Run Tests
```bash
# Navigate to test directory
cd tb/axi

# Compile and run simulation
make

# View waveform (optional)
# gtkwave waveform.vcd

# Clean build artifacts
make clean
```

💡 **Tip**: Check other test directories (`tb/axil/`, `tb/axis/`, `tb/axis_image/`) for more examples!

## 📖 Detailed Usage Guide

### Project Structure
```
.
├── src/                # VIP source code
│   ├── axi/            # AXI4: axi_ptr.hpp, axi.hpp, axi_master.hpp, axi_slave.hpp, axi_common.hpp
│   ├── axil/           # AXI4-Lite: axil_ptr.hpp, axil.hpp, axil_master.hpp, axil_slave.hpp
│   ├── axis/           # AXI4-Stream: axis_ptr.hpp, axis.hpp, axis_master.hpp, axis_slave.hpp
│   ├── axis_image/     # AXI4-Stream image: axis_image.hpp, axis_image_*.hpp, bmp.hpp
│   ├── axis_video/     # AXI4-Stream video: axis_video.hpp, axis_video_*.hpp, frame_mem.hpp, pix_fmt.hpp
│   ├── log.hpp         # Shared logging
│   ├── sig.hpp         # Shared signal helpers
│   ├── clock_gen.hpp   # Software clock generator for async clock simulation
│   └── sim_coop.hpp    # Protothread-based cooperative simulation scheduler
├── tb/                 # Testbenches and examples
│   ├── axi/            # AXI4 tests
│   ├── axil/           # AXI4-Lite tests
│   ├── axis/           # AXI4-Stream tests
│   ├── axis_image/     # AXI4-Stream image tests
│   └── axis_video/     # AXI4-Stream video tests
└── README.md
```

### 1. Include Headers
Add each relevant directory under `src/` to your compiler include path (e.g. `-Isrc/axi -Isrc/axil ...`, as in `tb/*/Makefile`). Then:

```cpp
#include "axi_ptr.hpp"    // AXI4 signal structs (optional if only using BFM umbrella)
#include "axi.hpp"        // AXI4 VIP (master + slave)
#include "axil_ptr.hpp"
#include "axil.hpp"       // AXI4-Lite VIP
#include "axis_ptr.hpp"   // AXI4-Stream signal structs (optional)
#include "axis.hpp"       // AXI4-Stream VIP
#include "axis_image.hpp" // AXI4-Stream image VIP
#include "axis_video.hpp" // AXI4-Stream video VIP
#include "clock_gen.hpp"  // Software clock generator (optional)
#include "sim_coop.hpp"   // Protothread simulation engine (optional)
```

### 2. Bind Signals
Bind Verilated model signal pointers to the VIP signal structure:
```cpp
// Define AXI bus structure (match your DUT parameters)
axi_ptr<256, 40, 16> axi_sig;  // DataWidth=256, AddrWidth=40, IdWidth=16

// Bind signals (point VIP pointers to Verilator model signals)
axi_sig.awaddr  = &(top->m_axi_awaddr);
axi_sig.awvalid = &(top->m_axi_awvalid);
axi_sig.awready = &(top->m_axi_awready);
// ... bind all required AXI signals
```

### 3. Create VIP Instances
```cpp
// Create AXI Master instance
axi_master<256, 40, 16> mst(axi_sig);

// Or create AXI Slave instance
axi_slave<256, 40, 16> slv(axi_sig);
```

### 4. Drive in Simulation Loop

**Important**: Call `update_input()` before `eval()` to correctly simulate sampling of input signals at clock edges.

```cpp
while (!Verilated::gotFinish()) {
    // ... clock generation ...

    if (top->clk) {
        // 1. Read input signals (sample at clock edge)
        mst.update_input();
        slv.update_input();
    }

    top->eval();  // Evaluate model logic

    if (top->clk) {
        // 2. Initiate transactions here
        if (cycle_count == 10) {
            std::vector<uint8_t> data = {0xAA, 0xBB, 0xCC, 0xDD};
            mst.write_incr(0x1000, data);
        }

        // 3. Update output signals
        mst.update_output();
        slv.update_output();
    }
}
```

### 5. AXI4-Stream Image VIP Features

**Standard Compliance**: The `axis_image` module follows the **Xilinx AXI4-Stream Video IP and System Design** standard, ensuring compatibility with Xilinx/AMD video processing IP cores. For detailed specifications, refer to the [Xilinx AXI Video IP Product Guide](https://docs.xilinx.com/r/en-US/pg034_axi_video_ip).

```
          __    __    __    __          __    __    __
clk    __/  \__/  \__/  \__/  \__XXXX__/  \__/  \__/  \___XXXX
               ______ _____ _____       _____       ______
tdata  XXXXXXXX__D0__X__D1_X__D2_XXXXXXX__Dn_XXXXXXX__D0__XXXX
               ______ _____ _____       _____       ______
tkeep  XXXXXXXX__K0__X__K1_X__K2_XXXXXXX__Kn_XXXXXXX__K0__XXXX
                _____
tuser  ________/ sof \________________________________________
                                        _____
tlast  ________________________________/ eol \________________

                ______________________________      __________
tvalid ________/     line0                    \____/   line1
       _______________________________________________________
tready
```

**Image Master** (BMP to AXI4-Stream):
```cpp
// Parameters: <BPC (bits per channel), PPC (pixels per beat)>
axis_image_master<8, 4> img_mst(s_mst_port);

// ImageInfo to store width/height
ImageInfo info;

// Read BMP and start sending (non-blocking)
img_mst.send_frame("test.bmp", &info);

// In simulation loop
img_mst.update_input();
top->eval();
img_mst.update_output();

// Check if sending is complete
if (img_mst.eof()) {
    // Image transfer done
}
```

**Image Slave** (AXI4-Stream to BMP):
```cpp
axis_image_slave<8, 4> img_slv(m_slv_port);

// Prepare to receive image (info contains width/height from master)
img_slv.recv_frame(&info, "output.bmp");

// In simulation loop
img_slv.update_input();
top->eval();
img_slv.update_output();

// Image is automatically saved when reception completes
// Can also manually save with: img_slv.write_file("output.bmp");
```

**Video VIP** (`axis_video`) provides bidirectional conversion between planar video frames and AXI4-Stream, supporting multiple video formats including YUV444/422/420, also compliant with Xilinx AXI4-Stream Video standard, suitable for video processing IP verification.

## 🔧 Running Tests

### Environment Setup
Ensure you have:
- Verilator (v5.038+)
- GTKWave (optional, for waveform viewing)
- Make
- G++ (C++11 or higher)

### Execute Tests
For basic testing, follow the same commands shown in the **Quick Start** section. Each test generates a `waveform.vcd` file for waveform viewing with GTKWave or other VCD viewers.

## 📝 Acknowledgements

The design inspiration for this project comes from [soc-simulator](https://github.com/cyyself/soc-simulator). Thanks for its excellent concepts.

## 📄 License

MIT License

Copyright (c) 2025 dozecat

---

<span id="cn">vaxivip</span>
===========================

**vaxivip** 是一个用于 [Verilator](https://www.veripool.org/verilator/) 仿真的轻量级 AXI 协议验证 IP (VIP) 库。它用 C++ 编写，可以在 C++ 测试平台中模拟 AXI Master 和 Slave 行为，从而验证 Verilog/SystemVerilog 设计。

## ✨ 特性概览

| VIP模块                               | 协议        | 功能描述                                                     | 典型应用                |
| ------------------------------------- | ----------- | ------------------------------------------------------------ | ----------------------- |
| **`axi`**（`src/axi/`）               | AXI4        | Master/Slave接口，支持FIXED (固定)，INCR (递增)，WRAP (回环) 突发 | 处理器/内存总线验证     |
| **`axil`**（`src/axil/`）             | AXI4-Lite   | Master/Slave接口                                             | axi4-lite寄存器访问验证 |
| **`axis`**（`src/axis/`）             | AXI4-Stream | Master/Slave接口                                             | axi-stream时序验证      |
| **`axis_image`**（`src/axis_image/`） | AXI4-Stream | BMP图像与AXI4-Stream双向转换，支持RGB格式（遵循Xilinx AXI4-Stream Video标准） | 图像处理IP验证          |
| **`axis_video`**（`src/axis_video/`） | AXI4-Stream | 平面视频帧与AXI4-Stream双向转换，支持YUV444/YUV422/YUV420格式（遵循Xilinx AXI4-Stream Video标准） | 视频处理IP验证 |
| **`clock_gen` / `sim_coop`**（`src/`） | 仿真工具 | 软件时钟生成 + 基于protothread的协作调度器，支持多时钟域异步仿真 | Verilator多时钟域测试平台 |

## 🚀 快速开始

### 1. 安装依赖
```bash
# Ubuntu/Debian
sudo apt-get install verilator g++ make

# macOS (使用 Homebrew)
brew install verilator
```

### 2. 克隆项目
```bash
git clone https://github.com/dozecat/vaxivip.git
cd vaxivip
```

### 3. 编译和运行测试
```bash
# 进入测试目录
cd tb/axi

# 编译并运行仿真
make

# 查看波形（可选）
# gtkwave waveform.vcd

# 清理构建文件
make clean
```

💡 **提示**：查看其他测试目录（`tb/axil/`、`tb/axis/`、`tb/axis_image/`）获取更多示例！

## 📖 详细使用指南

### 项目结构
```
.
├── src/                # VIP源代码
│   ├── axi/            # AXI4：axi_ptr.hpp, axi.hpp, axi_master.hpp, axi_slave.hpp, axi_common.hpp
│   ├── axil/           # AXI4-Lite：axil_ptr.hpp, axil.hpp, axil_master.hpp, axil_slave.hpp
│   ├── axis/           # AXI4-Stream：axis_ptr.hpp, axis.hpp, axis_master.hpp, axis_slave.hpp
│   ├── axis_image/     # AXI4-Stream 图像：axis_image.hpp, axis_image_*.hpp, bmp.hpp
│   ├── axis_video/     # AXI4-Stream 视频：axis_video.hpp, axis_video_*.hpp, frame_mem.hpp, pix_fmt.hpp
│   ├── log.hpp         # 公共日志
│   ├── sig.hpp         # 公共信号辅助
│   ├── clock_gen.hpp   # 异步时钟信号软件生成器
│   └── sim_coop.hpp    # 基于protothread的协作式仿真调度器
├── tb/                 # 测试用例和示例
│   ├── axi/            # AXI4 测试
│   ├── axil/           # AXI4-Lite 测试
│   ├── axis/           # AXI4-Stream 测试
│   ├── axis_image/     # AXI4-Stream 图像测试
│   └── axis_video/     # AXI4-Stream 视频测试
└── README.md
```

### 1. 引入头文件
将 `src` 下对应子目录加入编译 `-I` 路径（示例见各 `tb/*/Makefile`）。然后：

```cpp
#include "axi_ptr.hpp"   // AXI4 信号结构（可省略，若只通过总头使用 BFM）
#include "axi.hpp"       // AXI4 VIP
#include "axil_ptr.hpp"
#include "axil.hpp"      // AXI4-Lite VIP
#include "axis_ptr.hpp"  // AXI4-Stream 信号结构（可省略）
#include "axis.hpp"      // AXI4-Stream VIP
#include "axis_image.hpp" // AXI4-Stream 图像 VIP
#include "axis_video.hpp" // AXI4-Stream 视频 VIP
#include "clock_gen.hpp"  // 软件时钟生成器（可选）
#include "sim_coop.hpp"   // Protothread仿真引擎（可选）
```

### 2. 绑定信号
将 Verilated 模型的信号指针绑定到 VIP 的信号结构体：
```cpp
// 定义AXI总线结构体（与DUT参数匹配）
axi_ptr<256, 40, 16> axi_sig;  // DataWidth=256, AddrWidth=40, IdWidth=16

// 绑定信号（将VIP指针指向Verilator模型信号）
axi_sig.awaddr  = &(top->m_axi_awaddr);
axi_sig.awvalid = &(top->m_axi_awvalid);
axi_sig.awready = &(top->m_axi_awready);
// ... 绑定所有必需的AXI信号
```

### 3. 创建VIP实例
```cpp
// 创建AXI Master实例
axi_master<256, 40, 16> mst(axi_sig);

// 或创建AXI Slave实例
axi_slave<256, 40, 16> slv(axi_sig);
```

### 4. 在仿真循环中驱动

**重要提示**：在 `eval()` 之前调用 `update_input()`，以正确模拟时钟沿的输入信号采样。

```cpp
while (!Verilated::gotFinish()) {
    // ... 时钟生成 ...

    if (top->clk) {
        // 1. 读取输入信号（在时钟沿采样）
        mst.update_input();
        slv.update_input();
    }

    top->eval();  // 计算模型逻辑

    if (top->clk) {
        // 2. 在此处发起事务
        if (cycle_count == 10) {
            std::vector<uint8_t> data = {0xAA, 0xBB, 0xCC, 0xDD};
            mst.write_incr(0x1000, data);
        }

        // 3. 更新输出信号
        mst.update_output();
        slv.update_output();
    }
}
```

### 5. 图像VIP功能

**标准遵循**：`axis_image` 模块遵循 **Xilinx AXI4-Stream Video IP and System Design** 标准，确保与 Xilinx/AMD 视频处理 IP 核的兼容性。详细规范请参考 [Xilinx AXI Video IP 产品指南](https://docs.xilinx.com/r/en-US/pg034_axi_video_ip)。

```
          __    __    __    __          __    __    __
clk    __/  \__/  \__/  \__/  \__XXXX__/  \__/  \__/  \___XXXX
               ______ _____ _____       _____       ______
tdata  XXXXXXXX__D0__X__D1_X__D2_XXXXXXX__Dn_XXXXXXX__D0__XXXX
               ______ _____ _____       _____       ______
tkeep  XXXXXXXX__K0__X__K1_X__K2_XXXXXXX__Kn_XXXXXXX__K0__XXXX
                _____
tuser  ________/ sof \________________________________________
                                        _____
tlast  ________________________________/ eol \________________

                ______________________________      __________
tvalid ________/     line0                    \____/   line1
       _______________________________________________________
tready
```

**图像Master** (BMP转AXI4-Stream)

```cpp
// 参数: <BPC (每通道位数), PPC (每拍像素数)>
axis_image_master<8, 4> img_mst(s_mst_port);

// ImageInfo用于存储宽度/高度信息
ImageInfo info;

// 读取BMP并开始发送（非阻塞）
img_mst.send_frame("test.bmp", &info);

// 在仿真循环中
img_mst.update_input();
top->eval();
img_mst.update_output();

// 检查发送是否完成
if (img_mst.eof()) {
    // 图像传输完成
}
```

**图像Slave** (AXI4-Stream转BMP):
```cpp
axis_image_slave<8, 4> img_slv(m_slv_port);

// 准备接收图像（info包含来自master的宽度/高度信息）
img_slv.recv_frame(&info, "output.bmp");

// 在仿真循环中
img_slv.update_input();
top->eval();
img_slv.update_output();

// 接收完成后图像自动保存
// 也可以手动保存：img_slv.write_file("output.bmp");
```

**视频VIP** (`axis_video`) 提供平面视频帧与AXI4-Stream的双向转换，支持YUV444/422/420等多种视频格式，同样遵循Xilinx AXI4-Stream Video标准，适用于视频处理IP验证。

## 🔧 运行测试

### 环境搭建

确保已安装：
- Verilator (v5.038+)
- GTKWave (可选，用于查看波形)
- Make
- G++ (支持C++11或更高版本)

### 执行测试
基础测试请参考**快速开始**部分中的命令。每个测试都会生成 `waveform.vcd` 波形文件，可使用 GTKWave 或其他 VCD 查看器查看。

## 📝 致谢

本项目的设计灵感来源于[soc-simulator](https://github.com/cyyself/soc-simulator)，感谢其优秀的构思。

## 📄 版权说明

MIT License

Copyright (c) 2025 dozecat
