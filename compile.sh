#!/usr/bin/env bash
set -euo pipefail

# FREYA installation root. Override when FREYA is installed elsewhere:
#   FREYA_ROOT=/path/to/fission_v2.0.5 ./compile.sh
FREYA_ROOT="${FREYA_ROOT:-${HOME}/llnl/fission_v2.0.5}"
FREYA_INCLUDE="${FREYA_INCLUDE:-${FREYA_ROOT}/include}"
FREYA_LIB="${FREYA_LIB:-${FREYA_ROOT}/lib}"

if [[ ! -f "${FREYA_INCLUDE}/fissionEvent.h" ]]; then
    echo "ERROR: fissionEvent.h not found in ${FREYA_INCLUDE}" >&2
    echo "Set FREYA_ROOT or FREYA_INCLUDE to your FREYA 2.0.5 installation." >&2
    exit 1
fi

if [[ ! -f "${FREYA_LIB}/libFission.so" && ! -f "${FREYA_LIB}/libFission.a" ]]; then
    echo "ERROR: libFission not found in ${FREYA_LIB}" >&2
    echo "Set FREYA_ROOT or FREYA_LIB to your FREYA installation." >&2
    exit 1
fi

g++ -std=c++17 -pthread \
    main.cpp \
    detector.cpp \
    he3_electric_field.cpp \
    primary_generator.cpp \
    geant4_decay_generator.cpp \
    freya_fission_generator.cpp \
    run_action.cpp \
    event_action.cpp \
    stepping_action.cpp \
    -I"${FREYA_INCLUDE}" \
    $(geant4-config --cflags) \
    $(root-config --cflags) \
    -L"${FREYA_LIB}" \
    -Wl,-rpath,"${FREYA_LIB}" \
    $(geant4-config --libs) \
    $(root-config --libs) \
    -lFission \
    -o main
