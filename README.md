# GlandaGPU Userspace Tests

A statically linked C application to test the GlandaGPU DRM driver via its custom IOCTL interface. It can be compiled for the physical DE10-Standard FPGA (ARM) or the QEMU digital twin (x86).

<p align="center">
  <a href="https://www.youtube.com/watch?v=fykBoT6yBR8">
    <img src="https://img.youtube.com/vi/fykBoT6yBR8/maxresdefault.jpg" width="600">
    <br>
    ▶ Watch Video
  </a>
</p>

## Prerequisites
For ARM cross-compilation on Debian/Ubuntu, install the toolchain:
sudo apt install gcc-arm-linux-gnueabihf

## Building

Compile for x86 (QEMU):
make

Cross-compile for ARM (DE10-Standard):
make ARCH=arm

## Usage

Copy the generated `gpu_test` binary into your target's root filesystem (QEMU image or physical SD card) and execute it:
./gpu_test
