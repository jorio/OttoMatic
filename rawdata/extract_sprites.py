# Convert Otto Matic packed sprites to TGA files

import sys, os, struct

GL_RGB = 0x1907
GL_RGBA = 0x1908
GL_RGB5_A1_EXT = 0x8057
GL_UNSIGNED_SHORT_1_5_5_5_REV = 0x8366

formatNames = {
    GL_RGB: "RGB",
    GL_RGBA: "RGBA",
    GL_RGB5_A1_EXT: "RGB5A1",
    GL_UNSIGNED_SHORT_1_5_5_5_REV: "A1RGB5",
}

TGA_IMAGETYPE_RAW_CMAP		= 1
TGA_IMAGETYPE_RAW_BGR		= 2
TGA_IMAGETYPE_RAW_GRAYSCALE	= 3
TGA_IMAGETYPE_RLE_CMAP		= 9
TGA_IMAGETYPE_RLE_BGR		= 10
TGA_IMAGETYPE_RLE_GRAYSCALE	= 11

def funpack(file, format):
    length = struct.calcsize(format)
    blob = file.read(length)
    return struct.unpack(format, blob)

inPath = sys.argv[1]
outPath = sys.argv[2]
filesize = os.path.getsize(inPath)

basename = os.path.splitext( os.path.basename(inPath))[0]
print("******************", basename)

with open(inPath, 'rb') as f:
    numSprites = funpack(f, ">i")[0]
    assert numSprites < 1000, "is this a little-endian file?"

    for spriteNo in range(numSprites):
        noAlpha = False
        width, height, aspectRatio, srcFormat, dstFormat, bufferSize = funpack(f, ">iifiii")
        srcFormatName = formatNames.get(srcFormat, 'UNKNOWN')
        dstFormatName = formatNames.get(dstFormat, 'UNKNOWN')

        postfix = ""

        if dstFormat != srcFormat:
            if srcFormat == GL_RGBA and dstFormat == GL_RGB:
                noAlpha = True
                postfix = "*** warning: kill alpha"
            else:
                postfix = "*** warning: different formats!"

        print(F"sprite {spriteNo}:\t{width}x{height}\tformat: {srcFormatName} -> {dstFormatName} {postfix}")
        assert aspectRatio == height / width, "weird aspect ratio"

        bpp = 0
        tgaBPP = 0
        tgaFlags = 0   # "Image Descriptor"
        # tgaFlags |= 1<<5   # flip Y. But Otto stores sprites upside-down, so don't set this flag and images will come out in the correct orientation.

        if srcFormat == GL_RGB:
            bpp = tgaBPP = 24
        elif srcFormat == GL_RGBA:
            bpp = tgaBPP = 32
            if noAlpha:
                tgaBPP = 24
            else:
                tgaFlags |= 8
        elif srcFormat == GL_RGB5_A1_EXT:
            assert False, "RGB5_A1 unsupported"
            bpp = tgaBPP = 16
            tgaFlags |= 1
        elif srcFormat == GL_UNSIGNED_SHORT_1_5_5_5_REV:
            bpp = tgaBPP = 16
            if not noAlpha:
                tgaFlags |= 1  # Set alpha bit in Targa 16
        else:
            assert False, "Unsupported source format"

        os.makedirs(outPath, exist_ok=True)
        with open(os.path.join(outPath, F"{basename}{spriteNo:03}.tga"), "wb") as tga:
            tga.write(struct.pack("<xxb xx xx x xx xx hhbb", TGA_IMAGETYPE_RAW_BGR, width, height, tgaBPP, tgaFlags))

            for _ in range(width*height):
                if bpp == 32:
                    r,g,b,a = funpack(f, ">4B")
                    if not noAlpha:
                        tga.write(struct.pack("<4B", b,g,r,a))
                    else:
                        tga.write(struct.pack("<3B", b,g,r))
                elif bpp == 24:
                    r,g,b = funpack(f, ">3B")
                    tga.write(struct.pack("<3B", b,g,r))
                elif srcFormat == GL_UNSIGNED_SHORT_1_5_5_5_REV:
                    sixteen = funpack(f, ">H")[0]
                    tga.write(struct.pack("<H", sixteen))
                else:
                    assert False

            tga.write(b"\0\0\0\0\0\0\0\0TRUEVISION-XFILE.\0")
