// ArduLinux - Arduino API for Linux
// Copyright (c) 2011-19 Arduino LLC.
// Copyright (c) 2020-23 Geeksville Industries, LLC
// Copyright (c) 2024-26 jp-bennett
// Copyright (c) 2026-27 l5yth
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#include "linux/LinuxHardwareSPI.h"
#include "Utility.h"
#include "logging.h"

#include <assert.h>

#ifdef ARDULINUX_LINUX_HARDWARE

#include "linux/PosixFile.h"
#include <linux/spi/spidev.h>
#include <mutex>
#include <map>

class LinuxSPIChip : public SPIChip, private PosixFile {
  private:
    std::mutex SPIMutex;
    uint32_t defaultSpeed = 2000000;

  public:
    LinuxSPIChip(const char *name, uint32_t default_frequency) : PosixFile(name) {
      defaultSpeed = default_frequency;
      uint8_t mode = SPI_MODE_0;
      uint8_t lsb = false;
      int status = ioctl(SPI_IOC_WR_MODE, &mode);
      assert(status >= 0);
      status = ioctl(SPI_IOC_WR_LSB_FIRST, &lsb);
      assert(status >= 0);
      status = ioctl(SPI_IOC_WR_MAX_SPEED_HZ, &defaultSpeed);
      assert(status >= 0);
    }

    int transfer(const uint8_t *outBuf, uint8_t *inBuf, size_t bufLen,
                bool deassertCS = true) {
      struct spi_ioc_transfer xfer;

      memset(&xfer, 0, sizeof xfer);

      xfer.tx_buf = (unsigned long)outBuf;
      xfer.len = bufLen;

      xfer.rx_buf = (unsigned long)inBuf; // Could be NULL, to ignore RX bytes
      xfer.cs_change = deassertCS;

      int status = ioctl(SPI_IOC_MESSAGE(1), &xfer);
      if (status < 0) {
        perror("SPI_IOC_MESSAGE");
        return status;
      }
      return 0;
    }
    void beginTransaction(uint32_t clockSpeed) {
      SPIMutex.lock();
      assert (ioctl(SPI_IOC_WR_MAX_SPEED_HZ, &clockSpeed) >= 0);
    }
    void endTransaction() {
      SPIMutex.unlock();
      assert (ioctl(SPI_IOC_WR_MAX_SPEED_HZ, &defaultSpeed) >= 0);
    }
};

static std::map<std::string, std::shared_ptr<void>> SPI_map;

#endif // ARDULINUX_LINUX_HARDWARE

namespace arduino {

LinuxHardwareSPI::LinuxHardwareSPI(int8_t spi_host) {
  char x = (spi_host & (1 << 4) - 1) + '0';
  char y = (spi_host >> 4) + '0';
  spiString = "/dev/spidev";
  spiString += x;
  spiString += ".";
  spiString += y;
}

uint8_t LinuxHardwareSPI::transfer(uint8_t data) {
  uint8_t response;
  assert(spiChip);
  spiChip->transfer(&data, &response, 1, false); // leave CS asserted
  return response;
}

uint16_t LinuxHardwareSPI::transfer16(uint16_t data) {
  notImplemented("transfer16");
  assert(0); // make fatal for now to prevent accidental use
  return 0x4242;
}

void LinuxHardwareSPI::transfer(void *buf, size_t count) {
  spiChip->transfer((uint8_t *) buf, (uint8_t *) buf, count, false);
}

void LinuxHardwareSPI::transfer(void *out, void *in, size_t count) {
  spiChip->transfer((uint8_t *) out, (uint8_t *) in, count, false);
}

void LinuxHardwareSPI::usingInterrupt(int interruptNumber) {
  // Do nothing
}

void LinuxHardwareSPI::notUsingInterrupt(int interruptNumber) {
  // Do nothing
}

void LinuxHardwareSPI::beginTransaction(SPISettings settings) {
  assert(settings.getBitOrder() == MSBFIRST); // we don't support changing yet
  assert(settings.getDataMode() == SPI_MODE0);
  if (spiChip)
    spiChip->beginTransaction(settings.getClockFreq());
}

void LinuxHardwareSPI::endTransaction(void) {
  if (spiChip)
    spiChip->endTransaction();
}

void LinuxHardwareSPI::attachInterrupt() {
  // Do nothing
}

void LinuxHardwareSPI::detachInterrupt() {
  // Do nothing
}

void LinuxHardwareSPI::begin() {
  begin(defaultFreq);
}

void LinuxHardwareSPI::begin(uint32_t freq) {
  if (!spiChip) {
#ifdef ARDULINUX_LINUX_HARDWARE
    if (SPI_map[spiString] != nullptr) {
      spiChip = std::static_pointer_cast<SPIChip>(SPI_map[spiString]);
    } else {
      try {
        spiChip = std::make_shared<LinuxSPIChip>(spiString.c_str(), freq);
        SPI_map[spiString] = spiChip;
      } catch (...) {
        printf("No hardware spi chip found...\n");
      }
    }
#endif
    if (!spiChip) // no hw spi found, use the simulator
      spiChip = std::make_shared<SimSPIChip>();
  }
}

void LinuxHardwareSPI::begin(const char *name, uint32_t freq) {
  if (name != nullptr)
    spiString = std::string(name);
  begin(freq);
}

void LinuxHardwareSPI::end() {
  if (spiChip) {
    spiChip.reset();
    spiChip = NULL;
  }
}

LinuxHardwareSPI SPI;

} // namespace arduino
