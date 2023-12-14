SUMMARY = "AP1320 OnSemi ISP Driver"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://COPYING;md5=b958dc5809a692e2a1bf8bbbfd71a46f"

SRC_URI = " file://ap1302.c "
SRC_URI:append = " file://ap1302.h "
SRC_URI:append = " file://Makefile "
SRC_URI:append = " file://COPYING "

S = "${WORKDIR}"

inherit module
COMPATIBLE_MACHINE = "(imx-nxp-bsp)"
