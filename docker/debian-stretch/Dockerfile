# docker build -t rbrich/xcikit-debian:stretch .

FROM debian:stretch-backports

RUN apt-get update

RUN printf 'APT::Get::Install-Recommends "false";\nAPT::Get::Install-Suggests "false";\n' >> '/etc/apt/apt.conf.d/recommends'

# Install development tools
RUN apt-get install -y build-essential git curl ca-certificates
RUN apt-get install -y python-pip python-setuptools python-wheel
RUN apt-get install -y -t stretch-backports cmake

# Install xcikit dependencies
RUN apt-get install -y libfreetype6-dev libglfw3-dev

# Install Conan
RUN pip --no-cache-dir install conan

# Cleanup
RUN apt-get clean
