# patch graphics driver for 6.4.16 kernel
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI := " file://0001-Fix-missing-include-on-kernel-6.4.16.patch"

