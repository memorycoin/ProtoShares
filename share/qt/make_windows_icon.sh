#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/memorycoin.png
ICON_DST=../../src/qt/res/icons/memorycoin.ico
convert ${ICON_SRC} -resize 16x16 memorycoin-16.png
convert ${ICON_SRC} -resize 32x32 memorycoin-32.png
convert ${ICON_SRC} -resize 48x48 memorycoin-48.png
convert memorycoin-16.png memorycoin-32.png memorycoin-48.png ${ICON_DST}

