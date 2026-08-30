FROM gcc:bookworm

RUN apt-get update && apt-get install -y \
    wget \
    make \
    valgrind \
    gdb \
    iproute2 \
    dnsutils \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

# Download and install GNU inetutils-2.0 (ping)
RUN wget -q https://ftp.gnu.org/gnu/inetutils/inetutils-2.0.tar.gz && \
    tar -xzf inetutils-2.0.tar.gz && \
    cd inetutils-2.0 && \
    ./configure --disable-servers --disable-clients --enable-ping && \
    make -j$(nproc) && \
    make install && \
    cd .. && rm -rf inetutils-2.0 inetutils-2.0.tar.gz

WORKDIR /app
