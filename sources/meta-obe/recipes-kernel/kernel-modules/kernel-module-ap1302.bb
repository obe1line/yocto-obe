SUMMARY = "AR1335 OnSemi Camera Driver"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://COPYING;md5=b958dc5809a692e2a1bf8bbbfd71a46f"

SRC_URI = " file://ar1335-camera-driver.c "
SRC_URI:append = " file://ar1335-camera-driver.h "
SRC_URI:append = " file://Makefile "
SRC_URI:append = " file://COPYING "

S = "${WORKDIR}"

inherit module
COMPATIBLE_MACHINE = "(imx-nxp-bsp)"