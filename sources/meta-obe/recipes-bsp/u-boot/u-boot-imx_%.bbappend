# add a dependency on the kernel build and copy the built DTB files at u-boot deploy stage

#do_compile[depends] += " \
#    virtual/kernel:do_deploy \
#    "

#do_deploy:prepend() {
#    install -d ${DEPLOYDIR}/${BOOT_TOOLS}
#    # copy dtbs from kernel build
#    for config in ${UBOOT_MACHINE}; do
#	    cp ${DEPLOY_DIR_IMAGE}/${UBOOT_DTB_NAME} ${B}/${config}/arch/arm/dts/${UBOOT_DTB_NAME}
#    done
#}

