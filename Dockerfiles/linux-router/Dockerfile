FROM alpine:3.18

LABEL org.opencontainers.image.authors="kychou2@illinois.edu"

COPY start.sh /start.sh
RUN chmod +x /start.sh

RUN apk add --upgrade --no-cache iproute2 && \
    apk add --upgrade --no-cache \
    --repository=http://dl-cdn.alpinelinux.org/alpine/edge/testing/ hping3

# # Build lft
# # http://pwhois.org/lft/
# # https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=lft
# RUN apk add --upgrade --no-cache libpcap-dev alpine-sdk gcc && \
#     curl -o lft-3.91.tar.gz -L http://pwhois.org/dl/index.who?file=lft-3.91.tar.gz && \
#     tar xf lft-3.91.tar.gz && \
#     cd lft-3.91 && \
#     ./configure --prefix=/ && \
#     make && \
#     make install && \
#     rm -rf /lft-3.91.tar.gz /lft-3.91

ENTRYPOINT ["/start.sh"]
