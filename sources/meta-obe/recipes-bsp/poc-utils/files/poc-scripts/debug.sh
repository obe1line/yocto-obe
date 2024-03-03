# enable debugging of pipeline drivers
echo 'module ar0144 +p' > /sys/kernel/debug/dynamic_debug/control
echo 'module imx8_isi +p' > /sys/kernel/debug/dynamic_debug/control
echo 'module imx_mipi_csis +p' > /sys/kernel/debug/dynamic_debug/control
echo 'module v4l2* +p' > /sys/kernel/debug/dynamic_debug/control
echo 'module video* +p' > /sys/kernel/debug/dynamic_debug/control

# raise the log level of the kernel
dmesg -n 8