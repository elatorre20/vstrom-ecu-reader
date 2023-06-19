#a tool to convert RGB888 bitmaps to the GRB555 char[]s expected by the GC9A01

def packPixel(r,g,b):
    #takes in three bytes and packs them into an RGB555 pixel
    #then converts that to two bytes for a C array
    r = r>>3
    g = g>>3
    b = b>>3
    pixel = ((r<<10) | (g<<5) | b)
    highByte = format((pixel>>8), "#04x")
    lowByte = format((pixel & 0b000000011111111), "#04x")
    return(highByte + "," + lowByte)


def convertBitmap(fname, size):
    #import the bitmap as a binary, discard header, iterate over pixels 
    bmp = open((fname + ".bmp"), "rb")
    out = open((fname + ".txt"), "w")
    imageData = "const unsigned char "+ fname +"["+str(size)+"]{\n  "
    rawPix = bmp.read()
    rawPix = rawPix[54:]
    for i in range(0,(len(rawPix)-1),3):
        imageData = imageData + (packPixel(rawPix[i], rawPix[i+2], rawPix[i+1])+",")
    imageData = imageData + "\n};"
    out.write(imageData)
    return(imageData)
    