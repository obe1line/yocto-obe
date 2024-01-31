KBRANCH ?= "v6.6/standard/base"
KMETA = "yocto-kernel-cache-yocto-6.6"

LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

inherit kernel-yocto kernel fsl-kernel-localversion fsl-vivante-kernel-driver-handler

LOCAL_VERSION ?= ""
# do not put the SCM version in the filename as it uses a snapshot zip which fails the build when fetching the hashes
SCMVERSION = "n"

# TODO: CVE Exclusions
# CVE exclusions
#include recipes-kernel/linux/cve-exclusion_6.6.inc

LINUX_VERSION ?= "6.6.14"

FILESEXTRAPATHS:prepend := "${THISDIR}/linux-obe/files:"
SRC_URI = ""
SRC_URI:append = "http://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${LINUX_VERSION}.tar.xz;name=machine "
SRC_URI:append = "https://git.yoctoproject.org/yocto-kernel-cache/snapshot/yocto-kernel-cache-yocto-6.6.tar.gz;type=kmeta;name=meta;branch=yocto-6.6;destsuffix=${KMETA} "

# old?
#SRC_URI:append = "file://0002-imx8mn-ddr4-evk-Add-DTS-for-AP1302-ISP.patch "
#SRC_URI:append = "file://0004-Add-camera-DTS-to-Makefile.patch "
#SRC_URI:append = "file://0006-Added-ap1302-driver-makefile-and-Kconfig.patch "

# device trees from linux-imx repository
SRC_URI:append = "file://0001-updated-device-tree-to-linux-imx-ones.patch "
# fix the regulators
SRC_URI:append = "file://0005-Added-voltage-regulators-to-adv7535.patch "

# ar0144 driver and dts
SRC_URI:append = "file://0001-added-ar0144-dts.patch "
SRC_URI:append = "file://0002-added-ar0144-dtb-into-makefile.patch "
SRC_URI:append = "file://0003-added-ar0144-driver.patch "

# configuration
SRC_URI:append = "file://obe-defconfig "

SRC_URI[machine.sha256sum] = "fbe96b2db3f962cd2a96a849d554300e7a4555995160082d4f323c2a1dfa1584"
SRC_URI[meta.sha256sum] = "213f4d13def33bb493249b9ce2d7e15b8035966286802c325a38b274e5067c8b"

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

# TODO: use the machine dist config to specify dtbs
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


