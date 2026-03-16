set -e

g++ -o spidev spidev.cpp
g++ -o gpio gpio.cpp
./spidev
./gpio
