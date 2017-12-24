#FROM ubuntu:17.10
FROM ubuntu:latest

LABEL maintainer="Alrik Firl firla@oregonstate.edu" \ 
      version="0.1" \ 
      description="fish labeler dockerfile for NRT project"
      
# For SSH access and port redirection
ENV ROOTPASSWORD sample

ENV DEBIAN_FRONTEND noninteractive
RUN echo "deb http://archive.ubuntu.com/ubuntu precise main universe" > /etc/apt/sources.list

# Update packages
RUN apt-get -y update

# Install system tools / libraries
RUN apt-get -y install build-essential \
    ssh \
    vim \
    git \
    wget \
    make \
    cmake \
    virt-viewer \
    qtbase5-dev \
    ffmpeg \ 
    libav-tools \
    libavfilter-dev \ 
    libavformat-dev \ 
    libavcodec-dev \ 
    libavcodec-extra \ 
    libavdevice-dev \ 
    libopencv-dev  \ 
    libopencv-imgcodecs-dev

WORKDIR /home/NRTfish

COPY *.cpp fishlabeler/
COPY *.hpp fishlabeler/
COPY CMakeLists.txt fishlabeler/

RUN mkdir /home/NRTfish/build

# Install vnc, xvfb in order to create a 'fake' display and firefox
RUN apt-get update && apt-get install -y x11vnc xvfb firefox
RUN mkdir ~/.vnc
# Setup a password
RUN x11vnc -storepasswd 1234 ~/.vnc/passwd
# Autostart firefox (might not be the best way, but it does the trick)
RUN bash -c 'echo "firefox" >> /.bashrc'

EXPOSE 5900
CMD    ["x11vnc", "-forever", "-usepw", "-create"]

# Enable additional output from Launcher
ENV QT_VERBOSE true
ENV QT_TESTING true
# Xvfb
ENV DISPLAY :99
