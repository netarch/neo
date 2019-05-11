#!/bin/bash

EXITCODE=0
SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
SRC_DIR="$(realpath ${SCRIPT_DIR}/../src)"
FILES=$(basename -a $(find ${SRC_DIR} -type f) | sort | uniq -d)

for FILE in $FILES; do
	echo >&2
	echo "[-] multiple source files have the same name '$FILE':" >&2
    FILEPATHS=$(find ${SRC_DIR} -type f -name $FILE)
    for FILEPATH in $FILEPATHS; do
        echo "    $FILEPATH" >&2
    done
	EXITCODE=1
done

exit $EXITCODE
