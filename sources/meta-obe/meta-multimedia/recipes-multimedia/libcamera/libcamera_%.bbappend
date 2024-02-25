# use revision as of 16-Feb-2024
SRC_URI = " git://git.libcamera.org/libcamera/libcamera.git;protocol=https;branch=master "
SRCREV = "059bbcdc34e7d887ee02fdd47613bd6e0eb70d16"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# add AR0144 camera properties
SRC_URI:append = " file://patches/0001-Added-AR0144-camera-sensor-properties.patch"
