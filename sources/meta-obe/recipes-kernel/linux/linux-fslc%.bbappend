FILESEXTRAPATHS:prepend := "${THISDIR}/linux-fslc/files:"

# add POC device trees
SRC_URI:append = " file://0002-device-tree-supporting-ov5640.patch"
SRC_URI:append = " file://0003-added-poc-dtb-into-makefile.patch"

# add orientation to camera for mandatory libcamera property
# SRC_URI:append = " file://0001-added-orientation-to-ov5640-in-dts.patch"

# ar0144 driver and dts
#SRC_URI:append = " file://0001-added-ar0144-dts.patch"
#SRC_URI:append = " file://0002-added-ar0144-dtb-into-makefile.patch"
#SRC_URI:append = " file://0003-added-ar0144-driver.patch"

SRC_URI:append = " file://kernel-6-6-defconfig"

unset KERNEL_DEVICETREE
KERNEL_DEVICETREE = " freescale/${KERNEL_DEVICETREE_BASENAME}.dtb "

# TODO: use KCONFIG_MODE = "--alldefconfig" or --"allnoconfig"
# KCONFIG_MODE = "--allnoconfig"
unset KBUILD_DEFCONFIG
KERNEL_DEFCONFIG = "kernel-6-6-defconfig"
