#!/bin/bash

VIDEO_PATH="$1"
OUTPUT_DIR="$2"

if [ ! -d "$OUTPUT_DIR" ]; then
    echo "creating output data directory "$OUTPUT_DIR"..."
    mkdir -p "$OUTPUT_DIR"
fi

numfiles=$(ls "$OUTPUT_DIR"/*.jpg | wc -l)
echo "NUMFILES: $numfiles"
#NOTE: don't re-extract frames if they already exist
if [ "$numfiles" -eq 0 ]; then
    ffmpeg -i "$VIDEO_PATH" -qscale:v 2 -start_number 0 ""$OUTPUT_DIR"/%06d.jpg"
fi

#write the video metadata out to a text file for the application
VID_INFO_LOG="$OUTPUT_DIR/info.txt"
touch "$VID_INFO_LOG"
ffprobe -i "$VIDEO_PATH" 2>&1 | tee "$VID_INFO_LOG"

#run the labeler application
cd build
#cmake ..
#make
#NOTE: this will run the labeling on the data just extracted. Can run manually to select different directories
./FishLabeler "$OUTPUT_DIR" 
