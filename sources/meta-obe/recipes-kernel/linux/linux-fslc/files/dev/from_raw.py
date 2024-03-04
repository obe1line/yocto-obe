import cv2
import numpy as np

# width = 1280
# height = 800

width = 1484
height = 856

with open("/tmp/test.raw", "rb") as rawimg:
    # Read the packed 12bits as bytes - each 3 bytes applies 2 pixels
    data = np.fromfile(rawimg, np.uint8, width * height * 3//2)

    data = data.astype(np.uint16)  # Cast the data to uint16 type.
    result = np.zeros(data.size*2//3, np.uint16)  # Initialize matrix for storing the pixels.

    # 12 bits packing: ######## ######## ########
    #                  | 8bits| | 4 | 4  |  8   |
    #                  |  lsb | |msb|lsb |  msb |
    #                  <-----------><----------->
    #                     12 bits       12 bits
    result[0::2] = ((data[1::3] & 15) << 8) | data[0::3]
    result[1::2] = (data[1::3] >> 4) | (data[2::3] << 4)
    bayer_im = np.reshape(result, (height, width))

    # Apply Demosacing (COLOR_BAYER_BG2BGR gives the best result out of the 4 combinations).
    bgr = cv2.cvtColor(bayer_im, cv2.COLOR_BAYER_BG2BGR)  # The result is BGR format with 16 bits per pixel and 12 bits range [0, 2^12-1].

    # Show image for testing (multiply by 16 because imshow requires full uint16 range [0, 2^16-1]).
    # cv2.imshow('bgr', cv2.resize(bgr*16, [width//2, height//2]))
    # cv2.waitKey()
    # cv2.destroyAllWindows()

    # Convert to uint8 before saving as JPEG (not part of the conversion).
    colimg = np.round(bgr.astype(float) * (255/4095))
    cv2.imwrite("/tmp/test.jpeg", colimg)
