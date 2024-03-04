# link the pads
media-ctl --links "'ar0144':0->'csis-32e30000.mipi-csi':0[1]"
media-ctl --links "'csis-32e30000.mipi-csi':1->'crossbar':0[1]"
media-ctl --links "'crossbar':2->'mxc_isi.0':0[1]"
media-ctl --links "'mxc_isi.0':1->'mxc_isi.0.capture':0[1]"

