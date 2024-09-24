# Use the official Debian image as a base
FROM debian:bullseye-slim

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV VCPKG_ROOT=/vcpkg
ENV CMAKE_ARGS="-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

# Install necessary packages
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    unzip \
	zip \
	pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Clone vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git $VCPKG_ROOT \
    && cd $VCPKG_ROOT \
    && git checkout master \
    && ./bootstrap-vcpkg.sh

# Set the working directory
WORKDIR /app

# Copy your application source code
COPY . .

# Install dependencies using vcpkg
RUN $VCPKG_ROOT/vcpkg install --triplet x64-linux

# Build the application
RUN cmake . $CMAKE_ARGS -DCMAKE_EXE_LINKER_FLAGS="-pthread" && make

# Specify the command to run your application (replace `your_executable` with the actual name)
CMD ["./HackArena2024H2_Cxx"]
