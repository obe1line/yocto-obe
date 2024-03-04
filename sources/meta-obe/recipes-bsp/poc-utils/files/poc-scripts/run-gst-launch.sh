gst-launch-1.0 --verbose --gst-debug-level=5 v4l2src device=/dev/video0 ! 'video/x-bayer,width=1280,height=800,format=rggb12le,framerate=60/1' ! fakesink
