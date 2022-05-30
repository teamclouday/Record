# This file load icon file and generate cpp file
# that contains the raw data of icon
import os
import io
import numpy as np
from PIL import Image

ICON_FILE = os.path.join("assets", "icon.png")
CPP_FILE = os.path.join("src", "icon.cpp")

CPP_CONTENT = """\
// Auto-generated file from genicon.py
extern const int APPICON_W = {};
extern const int APPICON_H = {};
extern const unsigned char APPICON_DATA[] = {{
{}
}};
"""

# load file
img = Image.open(ICON_FILE).convert("RGBA")
APPICON_W, APPICON_H = img.size
img.show()

# convert to GLFWimage layout format
# RGBA to ARBG
img = np.asarray(img)
img = img[:, :, [3,0,2,1]]
img = Image.fromarray(img)

# load raw data
arr = io.BytesIO()
img.save(arr, format="bmp")
APPICON_DATA = ",".join("0x{:02x}".format(b) for b in arr.getvalue())

# save file
output = CPP_CONTENT.format(APPICON_W, APPICON_H, APPICON_DATA)
with open(CPP_FILE, "w") as out:
    out.write(output)

print("done")