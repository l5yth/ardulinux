# Manual hardware integration tests — requires real GPIO (/dev/gpiochip2)
# and SPI (/dev/spidev*) devices. Not run in CI.
set -e

g++ -o spidev spidev.cpp
g++ -o gpio gpio.cpp
./spidev
./gpio
