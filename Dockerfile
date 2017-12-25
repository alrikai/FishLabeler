#FROM ubuntu:17.10
FROM ubuntu:latest

LABEL maintainer="Alrik Firl firla@oregonstate.edu" \ 
      version="0.1" \ 
      description="fish labeler dockerfile for NRT project"
      
# For SSH access and port redirection
#ENV ROOTPASSWORD sample
#ENV DEBIAN_FRONTEND noninteractive
#RUN echo "deb http://archive.ubuntu.com/ubuntu precise main universe" > /etc/apt/sources.list

# Update packages
RUN apt-get update

# Install system tools / libraries
RUN apt-get -y install build-essential \
    ssh \
    sudo \
    vim \
    git \
    wget \
    make \
    cmake \
    virt-viewer \
    ffmpeg \ 
    libboost-all-dev \ 
    libopencv-dev   

RUN apt-get -y install qt5-default  
RUN apt-get -y install cimg-dev
#RUN apt-get -y install firefox  

#(userid): id -u alrik --> 1000, (groupid): id -g  alrik--> 1000 (this presumably has to be changed if not the 1st user on the system?)
RUN export uid=1000 gid=1000 devname=NRTfish && \
    mkdir -p /home/${devname} && \
    echo "${devname}:x:${uid}:${gid}:Developer,,,:/home/${devname}:/bin/bash" >> /etc/passwd && \
    echo "${devname}:x:${uid}:" >> /etc/group && \
    touch /etc/sudoers.d/${devname} && \
    echo "${devname} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${devname} && \
    chmod 0440 /etc/sudoers.d/${devname} && \
    chown ${uid}:${gid} -R /home/${devname} && \
    mkdir -p /home/NRTfish/fishlabeler/build && \
    chmod 7777 -R /home/NRTfish/fishlabeler

USER NRTfish 
ENV HOME /home/NRTfish
WORKDIR /home/NRTfish

COPY *.cpp fishlabeler/
COPY *.hpp fishlabeler/
COPY CMakeLists.txt fishlabeler/

# Enable additional output from Launcher
ENV QT_VERBOSE true
ENV QT_TESTING true
# Xvfb
ENV DISPLAY :99
