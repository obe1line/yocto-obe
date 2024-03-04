#LIBCAMERA_LOG_LEVELS=*:DEBUG cam --camera=1 --list-properties --stream role=raw
LIBCAMERA_LOG_LEVELS=*:DEBUG cam --camera=1 --capture --file=output.raw --stream role=raw
#LIBCAMERA_LOG_LEVELS=*:DEBUG cam --camera=1 --capture=1 --file=output.raw --stream role=raw
#LIBCAMERA_LOG_LEVELS=*:DEBUG cam --list
