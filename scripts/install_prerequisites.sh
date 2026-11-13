#!/usr/bin/env bash

set -euo pipefail

OS_NAME="$(uname -s)"

if [[ "${OS_NAME}" == "Linux" ]]; then
    sudo apt-get update &&
    sudo apt-get install -y \
        build-essential \
        libsdl2-dev \
        cmake \
        ninja-build \
        libssl-dev \
        libusb-1.0-0-dev \
        pkg-config \
        libavcodec-dev \
        libavformat-dev \
        libswscale-dev \
        libavutil-dev \
        libpng-dev \
        gdb-multiarch \
        libgtest-dev \
        imagemagick \
        gcovr \
        libfreetype6 \
        libfreetype-dev \
        python3 \
        python3-dev \
        python3-pip \
        python3-venv

    ./scripts/install_gcc15.sh
elif [[ "${OS_NAME}" == "Darwin" ]]; then
    if ! command -v brew >/dev/null 2>&1; then
        echo "Homebrew is required on macOS. Install it from https://brew.sh and re-run this script." >&2
        exit 1
    fi

    brew update
    brew install \
        gcc \
        cmake \
        ninja \
        openssl \
        sdl2 \
        libusb \
        pkg-config \
        ffmpeg \
        libpng \
        freetype \
        imagemagick \
        googletest \
        gcovr \
        tcl-tk \
        python
else
    echo "Unsupported OS: ${OS_NAME}. This script currently supports Linux and macOS." >&2
    exit 1
fi

python3 -m venv env
source env/bin/activate
pip install -r requirements.txt
