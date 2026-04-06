FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

ENV DEBIAN_FRONTEND=noninteractive \
    DUETSCREEN_WORKSPACE=/workspaces/DuetScreen \
    BUILDROOT_WORKSPACE=/workspaces/buildroot-duetscreen

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
    python3-tk \
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
    && if ! apt-cache show gcc-15 >/dev/null 2>&1; then add-apt-repository --yes --update ppa:ubuntu-toolchain-r/test; fi \
    && apt-get install -y --no-install-recommends gcc-15 g++-15 \
    && rm -rf /var/lib/apt/lists/*
