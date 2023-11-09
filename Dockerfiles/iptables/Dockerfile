FROM alpine:3.18

LABEL org.opencontainers.image.authors="kychou2@illinois.edu"

COPY start.sh /start.sh

RUN apk add --upgrade --no-cache iptables && \
    chmod +x /start.sh

ENTRYPOINT ["/start.sh"]
