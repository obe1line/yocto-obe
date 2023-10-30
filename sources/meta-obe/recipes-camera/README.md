# Build Notes

## Fetch the Linux source
git clone --depth=1 https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git --branch v6.5.8
cd linux

## Install cross-compile tools
sudo apt install gcc-aarch64-linu-gnu

## Copy the initial configuration file
cp ~/imx/sources/meta-obe/recipes-kernel/linux/files/obe-defconfig .config

## Set the environment variables for cross compilation
export arch=arm64
export CROSS_COMPILE=aarch64-linux-gnu- 

## Configure the kernel
make menuconfig

## Build the kernel and modules
make all