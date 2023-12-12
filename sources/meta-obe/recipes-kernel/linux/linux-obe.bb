KBRANCH ?= "v6.4/standard/base"
KMETA = "yocto-kernel-cache-yocto-6.4"

LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

inherit kernel-yocto kernel fsl-kernel-localversion fsl-vivante-kernel-driver-handler

LOCAL_VERSION ?= ""

# TODO: CVE Exclusions
# CVE exclusions
#include recipes-kernel/linux/cve-exclusion_6.4.inc

LINUX_VERSION ?= "6.4.16"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI = "http://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${LINUX_VERSION}.tar.xz;name=machine "
SRC_URI:append = "https://git.yoctoproject.org/yocto-kernel-cache/snapshot/yocto-kernel-cache-yocto-6.4.tar.gz;type=kmeta;name=meta;branch=yocto-6.4;destsuffix=${KMETA} "
SRC_URI:append = "file://0001-updated-imx8mn-dts-from-nxp-repo.patch "
SRC_URI:append = "file://0002-imx8mn-ddr4-evk-Add-DTS-for-AP1302-ISP.patch "
SRC_URI:append = "file://0003-copied-top-dtsi-for-imx8mn-from-nxp-repo.patch "
SRC_URI:append = "file://0004-Add-camera-DTS-to-Makefile.patch "
SRC_URI:append = "file://0005-Added-voltage-regulators-to-adv7535.patch "
SRC_URI:append = "file://obe-defconfig "

SRC_URI[machine.sha256sum] = "9626ec84a39ecb009bf11a271dd520941159c165d4e62f82e3a77b79d20ff27d"
SRC_URI[meta.sha256sum] = "0cdb914bfd4ac61d0c33fdf64e7312a504579bbbac8d8948476785bac286f11e"

# source directory is not in the usual git folder, but rather linux-<version>
S = "${WORKDIR}/linux-${LINUX_VERSION}"

PV = "${LINUX_VERSION}"

# TODO: when all config is working, use KCONFIG_MODE = "--allnoconfig"
KCONFIG_MODE = "--alldefconfig"
KERNEL_DEFCONFIG = "obe-defconfig"

KERNEL_DEVICETREE = " freescale/${KERNEL_DEVICETREE_BASENAME}.dtb "

KCONF_BSP_AUDIT_LEVEL = "1"

# Functionality flags
KERNEL_EXTRA_FEATURES ?= "features/netfilter/netfilter.scc"
KERNEL_FEATURES:append = " ${KERNEL_EXTRA_FEATURES}"
KERNEL_EXTRA_ARGS += "LOADADDR=${UBOOT_ENTRYPOINT}"

do_deploy:append:mx8m-generic-bsp() {
    # not sure why the dtb does not get copied to the u-boot folder, for now force a copy
    bbwarn "Hack to copy dtb to the uboot directory"
    local dtb_file="imx8mn-ddr4-evk-ap1302.dtb"
    local src_path="${B}/arch/arm64/boot/dts/freescale"
    local dest_path="${B}/../../../u-boot-imx/2023.04/build/imx8mn_ddr4_evk_defconfig/arch/arm/dts"
    install -m 0644 "${src_path}/${dtb_file}"  "${dest_path}/${dtb_file}"
    unset dest_path
    unset src_path
    unset dtb_path
}


