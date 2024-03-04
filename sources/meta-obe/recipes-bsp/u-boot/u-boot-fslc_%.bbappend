
UBOOT_EXTLINUX_LABELS = "ar0144 ov5640"
#
UBOOT_EXTLINUX_DEFAULT_LABEL = "Linux AR0144"
UBOOT_EXTLINUX_TIMEOUT = "30"
#
UBOOT_EXTLINUX_KERNEL_IMAGE:ar0144 = "../Image"
UBOOT_EXTLINUX_FDT:ar0144 = "../imx8mn-ddr4-evk-poc-ar0144.dtb"
UBOOT_EXTLINUX_MENU_DESCRIPTION:ar0144 = "Linux AR0144"
#
UBOOT_EXTLINUX_KERNEL_IMAGE:ov5640 = "../Image"
UBOOT_EXTLINUX_FDT:ar0144 = "../imx8mn-ddr4-evk-poc.dtb"
UBOOT_EXTLINUX_MENU_DESCRIPTION:ov5640 = "Linux OV5640"


# add a dependency on the kernel build and copy the built DTB files at u-boot deploy stage

do_compile[depends] += " \
    virtual/kernel:do_deploy \
    "

do_deploy:prepend() {
    install -d ${DEPLOYDIR}/${BOOT_TOOLS}
    # copy dtbs from kernel build
    for config in ${UBOOT_MACHINE}; do
	    cp ${DEPLOY_DIR_IMAGE}/${UBOOT_DTB_NAME} ${B}/${config}/arch/arm/dts/${UBOOT_DTB_NAME}
    done
}

