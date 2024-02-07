FILESEXTRAPATHS:prepend := "${THISDIR}/linux-fslc/files:"

# add device trees
SRC_URI:append = " file://0001-device-tree-supporting-ov5640.patch"
SRC_URI:append = " file://0002-added-ar0144-dts.patch"
SRC_URI:append = " file://0003-added-poc-dtb-into-makefile.patch"

# ar0144 driver
SRC_URI:append = " file://0004-added-ar0144-driver.patch"
SRC_URI:append = " file://0005-added-ar0144-into-makefile.patch"

SRC_URI:append = " file://kernel-6-6-defconfig"

unset KERNEL_DEVICETREE
KERNEL_DEVICETREE = " freescale/${KERNEL_DEVICETREE_BASENAME}.dtb "
KERNEL_DEVICETREE:append = " freescale/${KERNEL_DEVICETREE_BASENAME}-ar0144.dtb "

# TODO: use KCONFIG_MODE = "--alldefconfig" or --"allnoconfig"
# KCONFIG_MODE = "--allnoconfig"
unset KBUILD_DEFCONFIG
KERNEL_DEFCONFIG = "kernel-6-6-defconfig"
