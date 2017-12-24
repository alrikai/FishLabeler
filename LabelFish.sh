#!/bin/bash

VIDEO_PATH="$1"
OUTPUT_DIR="$2"

if [ ! -d "$OUTPUT_DIR" ]; then
    echo "creating output data directory "$OUTPUT_DIR"..."
    mkdir -p "$OUTPUT_DIR"
fi

#TODO: check if there are existing frames already -- if so, then don't re-extract the video?
ffmpeg -i "$VIDEO_PATH" -qscale:v 2 ""$OUTPUT_DIR"/%06d.jpg"

#write the video metadata out to a text file for the application
VID_INFO_LOG="$OUTPUT_DIR/info.txt"
touch "$VID_INFO_LOG"
ffprobe -i "$VIDEO_PATH" 2>&1 | tee "$VID_INFO_LOG"

#run the labeler application
cd build
./FishLabeler
