v4l2-ctl -d /dev/video0 --verbose --set-fmt-video=width=1920,height=1080,pixelformat=YUYV --stream-mmap --stream-count=1 --stream-to=test_v4l2.raw
# cam --camera 1 --capture=1 --file=test_cam.raw
