FROM ubuntu:22.04

LABEL org.opencontainers.image.authors="kychou2@illinois.edu"
LABEL RUN docker run -it --privileged \
    -v /sys/bus/pci/devices:/sys/bus/pci/devices \
    -v /sys/bus/pci/drivers:/sys/bus/pci/drivers \
    -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages \
    -v /sys/devices/system/node:/sys/devices/system/node \
    -v /dev:/dev \
    --name NAME \
    IMAGE

# Build and run-time dependencies for DPDK
# https://doc.dpdk.org/guides/linux_gsg/sys_reqs.html#compilation-of-the-dpdk
RUN apt-get update && \
    apt-get install -y build-essential python3 meson ninja-build \
        python3-pyelftools libnuma-dev libarchive-dev libelf-dev \
        libhugetlbfs-dev libpcap-dev curl gcc xz-utils iproute2 make bash \
        pciutils kmod && \
    apt-get clean

# Build DPDK
COPY build-dpdk.sh /
RUN /build-dpdk.sh

ENV PATH "$PATH:/dpdk/build/app"

ENTRYPOINT ["/bin/bash"]
