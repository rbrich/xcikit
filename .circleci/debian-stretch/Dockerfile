FROM debian:stretch

RUN apt-get update

# Install development tools
RUN apt-get install --no-install-recommends -y build-essential cmake git ssh curl ca-certificates

# Install FreeType
RUN apt-get install --no-install-recommends -y libfreetype6-dev

# Install GLFW
RUN apt-get install --no-install-recommends -y libglfw3-dev

# Install Catch
RUN cd /usr/local/include && \
    curl -LO https://github.com/catchorg/Catch2/releases/download/v2.2.1/catch.hpp

# Build and install PEGTL
RUN cd /tmp && \
    curl -LO https://github.com/taocpp/PEGTL/archive/2.4.0.tar.gz && \
    tar xf 2.4.0.tar.gz && cd PEGTL-2.4.0 && \
    mkdir build && cd build && \
    cmake .. && \
    make && \
    make install && \
    cd /tmp && \
    rm -rf 2.4.0.tar.gz PEGTL-2.4.0
