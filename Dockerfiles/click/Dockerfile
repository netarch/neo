FROM ubuntu:22.04

LABEL org.opencontainers.image.authors="kychou2@illinois.edu"

ARG CLICK_COMMIT=9197a594b1c314264935106297aff08d97cbe7ee

RUN apt-get update && \
    apt-get install -y build-essential python3 meson ninja-build \
        pkg-config python3-pyelftools libnuma-dev libarchive-dev libelf-dev \
        libhugetlbfs-dev libpcap-dev libclang-dev python3-pip git vim dpdk \
        install-info autoconf && \
    apt-get clean

RUN mkdir /click && \
    cd /click && \
    git init && \
    git remote add origin http://github.com/kohler/click.git && \
    git fetch --depth 1 origin $CLICK_COMMIT && \
    git checkout FETCH_HEAD && \
    ./configure --prefix=/usr --sbindir=/usr/bin --enable-all-elements && \
    make -j && \
    ln -s zcat /usr/bin/gzcat && \
    make install

ENTRYPOINT ["/click/userlevel/click"]
