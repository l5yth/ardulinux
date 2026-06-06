#pragma once
#ifdef ARDULINUX_HARDWARE
#include "linux/LinuxHardwareI2C.h"
#else
#include "simulated/SimHardwareI2C.h"
#endif
