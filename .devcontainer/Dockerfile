# .devcontainer/Dockerfile
FROM mcr.microsoft.com/devcontainers/cpp:1-debian-12

# Install additional packages
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
        libcurl4-openssl-dev \
        nlohmann-json3-dev \
        cmake \
        build-essential \
        gdb \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install ngrok for callback testing
RUN curl -Lo ngrok.zip https://bin.equinox.io/c/bNyj1mQVY4c/ngrok-v3-stable-linux-amd64.zip \
    && unzip ngrok.zip \
    && mv ngrok /usr/local/bin \
    && rm ngrok.zip