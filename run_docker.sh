#!/bin/bash

# Create a Dockerfile to run the SideDRAM project
docker build -t sidedram_image .

# Launch the Docker container, running ./run.sh
docker run --rm -it \
    -v "$(pwd)/inputs:/SideDRAM/inputs" \
    -v "$(pwd)/stats:/SideDRAM/stats" \
    sidedram_image ./run.sh