# yocto-obe
Run setup.sh to clone the repositories.

Enable the bitbake environment:
. ./activate.sh

Build a core image:
bitbake core-image-minimal

Write to an SDcard on /dev/sde:
sudo bmaptool copy core-image-minimal-imx8mn-ddr4-evk-ar1335.rootfs.wic.gz /dev/sde

