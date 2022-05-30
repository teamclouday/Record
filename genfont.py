# This file load icon file and generate cpp file
# that contains the raw data of selected font
import os

FONT_FILE = os.path.join("assets",
# "Arimo", "Arimo Regular Nerd Font Complete.ttf"
# "HeavyData", "Heavy Data Nerd Font Complete.ttf"
"Hermit", "Hurmit Medium Nerd Font Complete.otf"
)
CPP_FILE = os.path.join("src", "font.cpp")

CPP_CONTENT = """\
// Auto-generated file from genfont.py
extern const int APPFONT_SIZE = {};
extern const unsigned char APPFONT_DATA[] = {{
{}
}};
"""

with open(FONT_FILE, "rb") as inFile:
    arr = bytearray(inFile.read())
APPFONT_SIZE = len(arr)
APPFONT_DATA = ",".join("0x{:02x}".format(b) for b in arr)

output = CPP_CONTENT.format(APPFONT_SIZE, APPFONT_DATA)
with open(CPP_FILE, "w") as out:
    out.write(output)

print("done")