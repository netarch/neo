FROM alpine:3.18

LABEL org.opencontainers.image.authors="kychou2@illinois.edu"

ARG NF=policer

# Run-time dependencies for Klint/DPDK
RUN apk add --upgrade --no-cache gcompat

COPY bin /nf
COPY home /home
COPY *.so.* /lib/

ENV LD_LIBRARY_PATH=/lib
ENTRYPOINT ["/nf"]
