OBE_FILES := "${THISDIR}/linux-fslc/files"
#FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
FILESEXTRAPATHS:prepend := "${OBE_FILES}:"

# add device trees
SRC_URI:append = " file://patches/0001-device-tree-supporting-ov5640.patch"
SRC_URI:append = " file://patches/0002-added-ar0144-dts.patch"
SRC_URI:append = " file://patches/0003-added-poc-dtb-into-makefile.patch"
SRC_URI:append = " file://patches/0006-added-regulators-to-hdmi-adv7535.patch"

# ar0144 driver
SRC_URI:append = " file://patches/0004-added-ar0144-driver.patch"
SRC_URI:append = " file://patches/0005-added-ar0144-into-makefile.patch"
SRC_URI:append = " file://patches/0007-added-ar0144-into-config.patch"

# kernel configuration
SRC_URI:append = " file://config/kernel-6-6.cfg"

unset KERNEL_DEVICETREE
KERNEL_DEVICETREE = " freescale/${KERNEL_DEVICETREE_BASENAME}.dtb "
KERNEL_DEVICETREE:append = " freescale/${KERNEL_DEVICETREE_BASENAME}-ar0144.dtb "


# TEMPORARY HACK to copy additional dtbs - will not be required after refactoring machine/distros
do_deploy:append() {
    bbwarn "Hack to copy dtb to the uboot directory"
    local dtb_file="poc-ar0144.dtb"
    local src_path="${B}/arch/arm64/boot/dts/freescale"
    local dest_path="${B}/../../../u-boot-fslc/2023.10+git/build/imx8mn_ddr4_evk_defconfig/arch/arm/dts"
    install -m 0644 "${src_path}/${dtb_file}"  "${dest_path}/${dtb_file}"
    unset dest_path
    unset src_path
    unset dtb_path
}
