#!/usr/bin/env bash

EXTERNAL_PATH=$1
CMAKE_OUTPUT_PATH=$2

if [[ "$EXTERNAL_PATH" == "" ]]; then
  echo "No argument passed please path to external tools(build_bbl.sh, run_remu.sh)"
  exit 1
fi

if [[ "$CMAKE_OUTPUT_PATH" == "" ]]; then
  echo "No argument passed please path to cmake output path(build_bbl.sh)"
  exit 1
fi

cd "$EXTERNAL_PATH" || exit 1
"$EXTERNAL_PATH"/build_bbl.sh "$CMAKE_OUTPUT_PATH"
tilix --maximize -e "$EXTERNAL_PATH/run_qemu_gdb.sh"

exit 0
