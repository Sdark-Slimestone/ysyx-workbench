#!/usr/bin/env python3
import os
import struct

filename = "vram.bin"
width, height = 256, 256
expected_size = width * height * 4

if not os.path.exists(filename):
    print(f"文件 {filename} 不存在")
    exit(1)

if os.path.getsize(filename) != expected_size:
    print(f"文件大小不匹配：应为 {expected_size} 字节，实际为 {os.path.getsize(filename)} 字节")
    exit(1)

with open(filename, "rb") as f:
    data = f.read()

# 解析为 32 位无符号整数列表（小端序）
pixels = [val[0] for val in struct.iter_unpack("<I", data)]

# 生成两种颜色顺序的 PPM 文件
for order in ['RGB', 'BGR']:
    ppm_filename = f"vram_{width}x{height}_{order}.ppm"
    with open(ppm_filename, "wb") as ppm:
        ppm.write(f"P6\n{width} {height}\n255\n".encode())
        for i in range(height):
            for j in range(width):
                val = pixels[i * width + j]
                if order == 'RGB':
                    r = (val >> 16) & 0xFF
                    g = (val >> 8) & 0xFF
                    b = val & 0xFF
                else:  # BGR
                    b = (val >> 16) & 0xFF
                    g = (val >> 8) & 0xFF
                    r = val & 0xFF
                ppm.write(bytes([r, g, b]))

print("已生成 vram_256x256_RGB.ppm 和 vram_256x256_BGR.ppm")
print("请用图片查看器打开，选择颜色正确的文件。")