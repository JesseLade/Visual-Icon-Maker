#!/bin/bash





g++ -std=c++11 -ObjC++ \
    main.mm main.cpp tinyfiledialogs.c \
    -o "IconMaker" \
    -lraylib \
    -framework Cocoa




