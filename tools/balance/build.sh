#!/bin/sh
# Builds the balance report tool with the host compiler (not devkitARM).
cd "$(dirname "$0")" && c++ -std=c++2b -O1 -Wall -I../../include -I../../src -I../../../Butano/butano/include -I../../../Butano/butano/hw/include -o report report.cpp
