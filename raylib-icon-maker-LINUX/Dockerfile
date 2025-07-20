FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y build-essential libgl1-mesa-dev libx11-dev libxcursor-dev libxrandr-dev libxi-dev libxinerama-dev libxxf86vm-dev

# Add raylib (if building from source)
# You can add your own build steps or copy prebuilt libraries.

WORKDIR /src
