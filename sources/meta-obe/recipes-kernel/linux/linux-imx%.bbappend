# for now point to fslc
FILESEXTRAPATHS:prepend := "${THISDIR}/linux-fslc-imx/files:"
SRC_URI:append = " file://0001-new-ap1302-driver.patch"
SRC_URI:append = " file://0002-imx8mn-ddr4-evk-Add-DTS-for-AP1302-ISP.patch"
SRC_URI:append = " file://0003-add-ap1302-dtb-into-makefile.patch"
SRC_URI:append = " file://0004-added-imx708-driver.patch"
SRC_URI:append = " file://0005-patch-media-type-header-for-imx708.patch"
SRC_URI:append = " file://0006-added-debug-to-ap1302.patch"
SRC_URI:append = " file://0007-added-ar0144-driver.patch"
SRC_URI:append = " file://0008-imx8mn-ddr4-evk-Add-DTS-for-AR0144.patch"
SRC_URI:append = " file://0009-added-ar0144-into-config.patch"
SRC_URI:append = " file://obe6157-defconfig"

KCONFIG_MODE = "--allnoconfig"
unset KBUILD_DEFCONFIG
# linux-imx recipe forces a configuration, so use a delta config to adjust
DELTA_KERNEL_DEFCONFIG = "obe6157-defconfig"

KERNEL_DEVICETREE += " freescale/${KERNEL_DEVICETREE_BASENAME}-ap1302.dtb "

do_deploy:append:mx8m-generic-bsp() {
    # not sure why the dtb does not get copied to the u-boot folder, for now force a copy
    bbwarn "Hack to copy dtb to the uboot directory"
    local dtb_file="imx8mn-ddr4-evk-ap1302.dtb"
    local dtb_file2="imx8mn-ddr4-evk-ar0144.dtb"
    local src_path="${B}/arch/arm64/boot/dts/freescale"
    local dest_path="${B}/../../../u-boot-imx/2023.04-r0/build/imx8mn_ddr4_evk_defconfig/arch/arm/dts"
    install -m 0644 "${src_path}/${dtb_file}"  "${dest_path}/${dtb_file}"
    install -m 0644 "${src_path}/${dtb_file}"  "${dest_path}/${dtb_file2}"
    unset dest_path
    unset src_path
    unset dtb_path
}
