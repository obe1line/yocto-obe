pushd sources
[ ! -d "poky" ] && git clone https://git.yoctoproject.org/poky --depth=1 --branch=mickledore
[ ! -d "meta-arm" ] && git clone https://git.yoctoproject.org/meta-arm --depth=1 --branch=mickledore
[ ! -d "meta-virtualization" ] && git clone https://git.yoctoproject.org/meta-virtualization --depth=1 --branch=mickledore
[ ! -d "meta-openembedded" ] && git clone https://github.com/openembedded/meta-openembedded --depth=1 --branch=mickledore
[ ! -d "meta-freescale" ] && git clone https://github.com/Freescale/meta-freescale.git --depth=1 --branch=master
[ ! -d "meta-freescale-distro" ] && git clone https://github.com/Freescale/meta-freescale-distro.git --depth=1 --branch=master
[ ! -d "meta-imx" ] && git clone https://github.com/nxp-imx/meta-imx.git --depth=1 --branch="rel_imx_6.1.55_2.2.0"
popd

echo Now run ". ./activate.sh" to enable the build environment
echo To build a multimedia image run "bitbake imx-image-multimedia"
