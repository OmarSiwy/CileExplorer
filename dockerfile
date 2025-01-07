FROM nvidia/cuda:12.6.0-devel-ubuntu20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    wget \
    git \
    build-essential \
    software-properties-common \
    # Add repository for newer GCC versions
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    # Install gcc-11
    && apt-get install -y gcc-11 g++-11 \
    # Install GTK3 and its dependencies
    libgtk-3-dev \
    libglib2.0-dev \
    libcairo2-dev \
    libpango1.0-dev \
    libgdk-pixbuf2.0-dev \
    pkg-config \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 110 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 110

# Install ZIG
RUN wget https://ziglang.org/download/0.13.0/zig-linux-x86_64-0.13.0.tar.xz \
    && tar -xf zig-linux-x86_64-0.13.0.tar.xz \
    && mv zig-linux-x86_64-0.13.0 /usr/local/zig \
    && ln -s /usr/local/zig/zig /usr/local/bin/zig

# Folder Images
RUN mkdir -p /app/include \
    && wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h -O /app/include/stb_image.h

WORKDIR /app
COPY . .

# Set default compiler to GCC 11
ENV CC=/usr/bin/gcc-11
ENV CXX=/usr/bin/g++-11

# Set PKG_CONFIG_PATH for GTK
ENV PKG_CONFIG_PATH=/usr/lib/pkgconfig

# CUDA path:
ENV PATH=/usr/local/cuda/bin:${PATH}
ENV LD_LIBRARY_PATH=/usr/local/cuda/lib64:${LD_LIBRARY_PATH}

CMD ["zig", "build", "test"]
