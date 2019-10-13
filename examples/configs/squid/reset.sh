#!/bin/bash

if [ $1 ]; then 
  # remove the cache in mem and disk for corresponding target URL
  awk '{print $7}' /home/leon/neo/examples/configs/squid/logs/access.log \
    | grep $1
    | xargs -n 1 squidclient -m PURGE
else 
  echo "no target URL provided"
  exit 1
fi

