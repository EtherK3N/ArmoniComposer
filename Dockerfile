# ==============================================================================
# Dockerfile: Isolated C++ Build & Test Environment for K3N Armoni Composer
# ==============================================================================

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install modern C++ build toolchain and JUCE Linux dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    libasound2-dev \
    libjack-jackd2-dev \
    libfreetype6-dev \
    libx11-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxcursor-dev \
    mesa-common-dev \
    libgl1-mesa-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Clone JUCE Framework
RUN git clone --depth 1 https://github.com/juce-framework/JUCE.git /workspace/JUCE

# Copy project source files
COPY . /workspace

# Configure and Build
RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --config Release --target ArmoniComposer_Tests

# Run Automated Test Suite on Container Startup
CMD ["/workspace/build/ArmoniComposer_Tests_artefacts/Release/ArmoniComposer_Tests"]
