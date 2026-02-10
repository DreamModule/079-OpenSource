FROM ubuntu:22.04

# Install build tools
RUN apt-get update && apt-get install -y \
    nasm \
    gcc \
    g++ \
    make \
    gcc-multilib \
    g++-multilib \
    binutils \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source files
COPY . .

# Build OS
RUN make all

# Command to run
CMD ["cp", "build/os.img", "/output/os.img"]
