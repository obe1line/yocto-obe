# for now point to fslc
FILESEXTRAPATHS:prepend := "${THISDIR}/linux-fslc-imx/files:"
SRC_URI:append = " file://0001-new-ap1302-driver.patch"
SRC_URI:append = " file://0002-imx8mn-ddr4-evk-Add-DTS-for-AP1302-ISP.patch"
SRC_URI:append = " file://0003-add-ap1302-dtb-into-makefile.patch"
SRC_URI:append = " file://0004-added-imx708-driver.patch"
SRC_URI:append = " file://obe6157-defconfig"

# TODO: use KCONFIG_MODE = "--alldefconfig" or --"allnoconfig"
KCONFIG_MODE = "--allnoconfig"
unset KBUILD_DEFCONFIG
KERNEL_DEFCONFIG = "obe6157-defconfig"
#KBUILD_DEFCONFIG:mx8-generic-bsp = "defconfig"


