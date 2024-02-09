v4l2-ctl -d /dev/video0 --verbose --set-fmt-video=width=1280,height=800,pixelformat=RG12 --stream-mmap --stream-count=1 --stream-to=/home/weston/test_v4l2.raw
# cam --camera 1 --capture=1 --file=test_cam.raw
