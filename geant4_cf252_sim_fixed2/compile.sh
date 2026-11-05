#!/usr/bin/env bash
set -e

g++ -std=c++17 \
    main.cpp \
    detector.cpp \
    he3_electric_field.cpp \
    primary_generator.cpp \
    run_action.cpp \
    event_action.cpp \
    stepping_action.cpp \
    $(geant4-config --cflags) \
    $(root-config --cflags) \
    $(geant4-config --libs) \
    $(root-config --libs) \
    -o main
