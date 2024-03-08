DEVFOLDER := "${THISDIR}/linux-fslc/files/dev"
FILESEXTRAPATHS:prepend := "${THISDIR}/linux-fslc/files:"

# add device trees
SRC_URI:append = " file://patches/0001-added-ar0144-dts.patch"

# ar0144 driver
SRC_URI:append = " file://patches/0004-added-ar0144-driver.patch"
SRC_URI:append = " file://patches/0005-added-ar0144-into-makefile.patch"
SRC_URI:append = " file://patches/0007-added-ar0144-into-config.patch"

# kernel configuration
SRC_URI:append = " file://config/kernel-6-6.cfg"

# remove the commit id from the kernel version
SCMVERSION = "n"
# remove suffix from kernel version
LOCALVERSION = "-obe"
LINUX_VERSION_EXTENSION = "-obe"

# prevent the camera drivers auto-loading as they conflict during development
KERNEL_MODULE_AUTOLOAD:remove = "ov5640"
KERNEL_MODULE_AUTOLOAD:remove = "ar0144"

# TODO: set extlinux.conf to correct dtb

do_patch:append() {
    bbwarn "Copy dev files over patched ones while developing (easier that updating patches)"
    cp ${DEVFOLDER}/ar0144.c ${S}/drivers/media/i2c/ar0144.c
    cp ${DEVFOLDER}/imx708.c ${S}/drivers/media/i2c/imx708.c
    cp ${DEVFOLDER}/imx8mn-ddr4-evk-poc-imx708.dts ${S}/arch/arm64/boot/dts/freescale/imx8mn-ddr4-evk-poc-imx708.dts
}