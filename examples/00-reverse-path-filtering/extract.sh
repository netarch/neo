#!/bin/bash

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
source "${SCRIPT_DIR}/../common.sh"

extract_stats "$RESULTS_DIR" > "$RESULTS_DIR/stats.csv"
extract_latency "$RESULTS_DIR" > "$RESULTS_DIR/latency.csv"
