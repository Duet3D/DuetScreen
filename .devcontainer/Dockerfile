FROM mcr.microsoft.com/devcontainers/cpp:2-ubuntu24.04

ENV DUETSCREEN_WORKSPACE=/workspaces/DuetScreen \
    BUILDROOT_WORKSPACE=/workspaces/buildroot-duetscreen

# Build-time only: keeps apt quiet while building the image without suppressing
# debconf prompts for anyone running apt interactively inside the container.
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf \
    automake \
    bash \
    bc \
    bison \
    build-essential \
    bzip2 \
    ca-certificates \
    libsdl2-dev \
    cmake \
    cmake-curses-gui \
    cpio \
    diffutils \
    dosfstools \
    file \
    flex \
    gawk \
    gettext \
    git \
    gzip \
    libncurses-dev \
    ninja-build \
    libssl-dev \
    libtool \
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
    python3-venv \
    mkbootimg \
    patch \
    perl \
    rsync \
    software-properties-common \
    tar \
    texinfo \
    u-boot-tools \
    unzip \
    util-linux \
    wget \
    which \
    xxd \
    xz-utils \
    curl \
    # LVGL unit tests (libraries/lvgl/scripts/prerequisites-apt.txt, amd64 only; the
    # i386 packages there are only for LVGL CI's NON_AMD64_BUILD runs)
    ruby-full \
    libjpeg-turbo8-dev \
    libwebp-dev \
    libglfw3-dev \
    libinput-dev \
    libxkbcommon-dev \
    libdrm-dev \
    wayland-protocols \
    libwayland-dev \
    libwayland-bin \
    libudev-dev \
    libuv1-dev \
    && if ! apt-cache show gcc-15 >/dev/null 2>&1; then add-apt-repository --yes --update ppa:ubuntu-toolchain-r/test; fi \
    && apt-get install -y --no-install-recommends gcc-15 g++-15 \
    && rm -rf /var/lib/apt/lists/*

# Python tooling (pillow and the pinned astyle and CMake from requirements.txt) lives in a
# venv at /opt/venv that is first on PATH, so `python3`, `cmake` and `astyle` resolve to
# it without activating anything. A venv rather than the system interpreter because
# Ubuntu 24.04 refuses system-wide pip installs (PEP 668). The build context is the
# repository root, filtered down to requirements.txt by Dockerfile.dockerignore.
ENV VIRTUAL_ENV=/opt/venv
ENV PATH=/opt/venv/bin:$PATH
COPY requirements.txt /tmp/requirements.txt
RUN python3 -m venv /opt/venv \
    && pip install --no-cache-dir --root-user-action=ignore -r /tmp/requirements.txt \
    && rm /tmp/requirements.txt
