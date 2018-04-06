FROM debian:stretch

RUN apt-get update

# Install development tools
RUN apt-get install --no-install-recommends -y build-essential cmake git ssh curl ca-certificates

# Install FreeType
RUN apt-get install --no-install-recommends -y libfreetype6-dev

# Install GLFW
RUN apt-get install --no-install-recommends -y libglfw3-dev

# Install Conan
RUN pip install conan
