# format the pads
export fmt=SRGGB12_1X12/1280x800
media-ctl --set-v4l2 "'ar0144':0[fmt:$fmt field:none]"
media-ctl --set-v4l2 "'csis-32e30000.mipi-csi':1[fmt:$fmt field:none]"
media-ctl --set-v4l2 "'crossbar':2[fmt:$fmt field:none]"
media-ctl --set-v4l2 "'mxc_isi.0':1[fmt:$fmt field:none]"

