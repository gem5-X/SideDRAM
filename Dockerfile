FROM ubuntu:24.04

# Define default shell
SHELL ["/bin/bash", "-c"] 

# Environment variables
ENV SYSTEMC_HOME=/usr/local/systemc

# Optimize the mirrors for a fast APT image
RUN sed -i 's/htt[p|ps]:\/\/archive.ubuntu.com\/ubuntu\//mirror:\/\/mirrors.ubuntu.com\/mirrors.txt/g' /etc/apt/sources.list

# Enable cache for APT and PIP for faster docker image creation time
ENV PIP_CACHE_DIR=/var/cache/buildkit/pip
RUN mkdir -p $PIP_CACHE_DIR
RUN rm -f /etc/apt/apt.conf.d/docker-clean

# Install the APT packages
RUN --mount=type=cache,target=/var/cache/apt \
	apt-get update && \
	apt-get install -y build-essential \
        device-tree-compiler cmake wget git vim

# Copy the project files to the container
COPY . /SideDRAM
WORKDIR /SideDRAM