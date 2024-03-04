LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# TODO: change to a loop of the poc-scripts folder

SRC_URI = ""
SRC_URI:append = " file://poc-scripts/bypass-isp.sh"
SRC_URI:append = " file://poc-scripts/isp.sh"
SRC_URI:append = " file://poc-scripts/power-isp.sh"
SRC_URI:append = " file://poc-scripts/reset.sh"
SRC_URI:append = " file://poc-scripts/capture.sh"
SRC_URI:append = " file://poc-scripts/cam.sh"
SRC_URI:append = " file://poc-scripts/debug.sh"

do_install() {
    install -d ${D}/home/root/poc-scripts
    install -m 0755 ${WORKDIR}/poc-scripts/bypass-isp.sh ${D}/home/root/poc-scripts/bypass-isp.sh
    install -m 0755 ${WORKDIR}/poc-scripts/isp.sh ${D}/home/root/poc-scripts/isp.sh
    install -m 0755 ${WORKDIR}/poc-scripts/power-isp.sh ${D}/home/root/poc-scripts/power-isp.sh
    install -m 0755 ${WORKDIR}/poc-scripts/reset.sh ${D}/home/root/poc-scripts/reset.sh
    install -m 0755 ${WORKDIR}/poc-scripts/capture.sh ${D}/home/root/poc-scripts/capture.sh
    install -m 0755 ${WORKDIR}/poc-scripts/cam.sh ${D}/home/root/poc-scripts/cam.sh
    install -m 0755 ${WORKDIR}/poc-scripts/debug.sh ${D}/home/root/poc-scripts/debug.sh
}

FILES:${PN}:append = " /home/root/poc-scripts/bypass-isp.sh"
FILES:${PN}:append = " /home/root/poc-scripts/bypass-isp.sh"
FILES:${PN}:append = " /home/root/poc-scripts/isp.sh"
FILES:${PN}:append = " /home/root/poc-scripts/power-isp.sh"
FILES:${PN}:append = " /home/root/poc-scripts/reset.sh"
FILES:${PN}:append = " /home/root/poc-scripts/capture.sh"
FILES:${PN}:append = " /home/root/poc-scripts/cam.sh"
FILES:${PN}:append = " /home/root/poc-scripts/debug.sh"
