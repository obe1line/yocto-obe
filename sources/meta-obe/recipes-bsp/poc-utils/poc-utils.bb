LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI = ""
SRC_URI:append = " file://poc-scripts/bypass-isp.sh"
SRC_URI:append = " file://poc-scripts/isp.sh"
SRC_URI:append = " file://poc-scripts/power-isp.sh"
SRC_URI:append = " file://poc-scripts/reset.sh"

do_install() {
    install -d ${D}/root/poc-scripts
    install -m 0755 ${WORKDIR}/poc-scripts/bypass-isp.sh ${D}/root/poc-scripts/bypass-isp.sh
    install -m 0755 ${WORKDIR}/poc-scripts/isp.sh ${D}/root/poc-scripts/isp.sh
    install -m 0755 ${WORKDIR}/poc-scripts/power-isp.sh ${D}/root/poc-scripts/power-isp.sh
    install -m 0755 ${WORKDIR}/poc-scripts/reset.sh ${D}/root/poc-scripts/reset.sh
}

FILES:${PN}:append = " /root/poc-scripts/bypass-isp.sh"
FILES:${PN}:append = " /root/poc-scripts/bypass-isp.sh"
FILES:${PN}:append = " /root/poc-scripts/isp.sh"
FILES:${PN}:append = " /root/poc-scripts/power-isp.sh"
FILES:${PN}:append = " /root/poc-scripts/reset.sh"
