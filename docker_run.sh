#!/bin/bash

docker build -f ./Dockerfile -t fishlabeler .   

docker run -ti --rm -u NRTfish -v /home/alrik/Data/NRTFish:/data -v /home/alrik/Data/FishFrames:/outdata -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix fishlabeler bash
