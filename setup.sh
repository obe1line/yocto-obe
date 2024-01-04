pushd sources
[ ! -d "poky" ] && git clone https://git.yoctoproject.org/poky --depth=1 --branch=nanbield
[ ! -d "meta-openembedded" ] && git clone https://github.com/openembedded/meta-openembedded --depth=1 --branch=nanbield
[ ! -d "meta-freescale" ] && git clone https://github.com/Freescale/meta-freescale.git --depth=1 --branch=master
[ ! -d "meta-freescale-distro" ] && git clone https://github.com/Freescale/meta-freescale-distro.git --depth=1 --branch=master
popd

echo Now run ". ./activate.sh" to enable the build environment
