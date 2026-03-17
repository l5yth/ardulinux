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

#ifdef ARDULINUX_HARDWARE

#include "linux/PosixFile.h"
#include <linux/spi/spidev.h>
#include <mutex>
#include <map>

/**
 * SPIChip implementation backed by a Linux spidev character device.
 *
 * Inherits PosixFile (privately) to manage the file descriptor.  A mutex
 * serialises concurrent beginTransaction()/endTransaction() pairs so that
 * multi-threaded callers sharing one bus do not interleave transactions.
 *
 * Only used when ARDULINUX_HARDWARE is defined (i.e. libgpiod and spidev are
 * present at configure time).
 */
class LinuxSPIChip : public SPIChip, private PosixFile {
  private:
    std::mutex SPIMutex; ///< Guards beginTransaction/endTransaction pairs
    uint32_t defaultSpeed = 2000000; ///< Speed to restore after each transaction

  public:
    /**
     * Open a spidev device and configure it for SPI mode 0, MSB-first.
     *
     * @param name              Path to the spidev device (e.g. /dev/spidev0.0).
     * @param default_frequency Default clock frequency in Hz.
     */
    LinuxSPIChip(const char *name, uint32_t default_frequency) : PosixFile(name) {
      defaultSpeed = default_frequency;
      // Configure the spidev driver: mode 0 (CPOL=0, CPHA=0), MSB-first.
      uint8_t mode = SPI_MODE_0;
      uint8_t lsb = false;
      int status = ioctl(SPI_IOC_WR_MODE, &mode);
      assert(status >= 0);
      status = ioctl(SPI_IOC_WR_LSB_FIRST, &lsb);
      assert(status >= 0);
      status = ioctl(SPI_IOC_WR_MAX_SPEED_HZ, &defaultSpeed);
      assert(status >= 0);
    }

    /**
     * Submit a single SPI_IOC_MESSAGE ioctl for a full-duplex transfer.
     *
     * @param outBuf     Transmit buffer, or NULL to send zeros.
     * @param inBuf      Receive buffer, or NULL to discard MISO bytes.
     * @param bufLen     Number of bytes to transfer.
     * @param deassertCS If true, deassert CS after the transfer.
     * @return 0 on success, negative errno on failure.
     */
    int transfer(const uint8_t *outBuf, uint8_t *inBuf, size_t bufLen,
                bool deassertCS = true) {
      struct spi_ioc_transfer xfer;

      memset(&xfer, 0, sizeof xfer);

      // spi_ioc_transfer stores pointers as unsigned long (kernel ABI).
      xfer.tx_buf = (unsigned long)outBuf;
      xfer.len = bufLen;
      xfer.rx_buf = (unsigned long)inBuf; // NULL is valid: MISO bytes discarded
      xfer.cs_change = deassertCS;

      int status = ioctl(SPI_IOC_MESSAGE(1), &xfer);
      if (status < 0) {
        perror("SPI_IOC_MESSAGE");
        return status;
      }
      return 0;
    }

    /**
     * Lock the bus and apply @p clockSpeed for the upcoming transaction.
     *
     * Callers must call endTransaction() to release the mutex.
     */
    void beginTransaction(uint32_t clockSpeed) {
      SPIMutex.lock();
      assert (ioctl(SPI_IOC_WR_MAX_SPEED_HZ, &clockSpeed) >= 0);
    }

    /**
     * Restore the default clock speed and release the bus mutex.
     */
    void endTransaction() {
      SPIMutex.unlock();
      assert (ioctl(SPI_IOC_WR_MAX_SPEED_HZ, &defaultSpeed) >= 0);
    }
};

/**
 * Cache of open spidev chips keyed by device path.
 *
 * Allows multiple LinuxHardwareSPI instances pointing to the same spidev
 * node to share one LinuxSPIChip (and its mutex) rather than opening the
 * device multiple times.
 */
static std::map<std::string, std::shared_ptr<void>> SPI_map;

#endif // ARDULINUX_HARDWARE

namespace arduino {

/**
 * Derive the spidev device path from the compact spi_host encoding.
 *
 * Encoding: low 4 bits = bus number, high 4 bits = device (CS) number.
 *   spi_host = 0x00 → /dev/spidev0.0
 *   spi_host = 0x10 → /dev/spidev0.1
 *   spi_host = 0x01 → /dev/spidev1.0
 */
LinuxHardwareSPI::LinuxHardwareSPI(int8_t spi_host) {
  // Extract bus index from low nibble, device index from high nibble.
  char x = (spi_host & (1 << 4) - 1) + '0';  // bus number digit
  char y = (spi_host >> 4) + '0';              // device number digit
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
  return 0;
}

void LinuxHardwareSPI::transfer(void *buf, size_t count) {
  spiChip->transfer((uint8_t *) buf, (uint8_t *) buf, count, false);
}

void LinuxHardwareSPI::transfer(void *out, void *in, size_t count) {
  spiChip->transfer((uint8_t *) out, (uint8_t *) in, count, false);
}

/** Linux has no SPI interrupt concept; no-op to satisfy the Arduino API. */
void LinuxHardwareSPI::usingInterrupt(int interruptNumber) {}

/** Linux has no SPI interrupt concept; no-op to satisfy the Arduino API. */
void LinuxHardwareSPI::notUsingInterrupt(int interruptNumber) {}

/**
 * Lock the SPI bus and apply the transaction settings.
 *
 * Only SPI_MODE0 and MSBFIRST are supported; other combinations are not
 * implemented by the spidev driver wrapper and will assert-fail.
 */
void LinuxHardwareSPI::beginTransaction(SPISettings settings) {
  assert(settings.getBitOrder() == MSBFIRST); // only MSB-first supported
  assert(settings.getDataMode() == SPI_MODE0);
  if (spiChip)
    spiChip->beginTransaction(settings.getClockFreq());
}

/** Release the SPI bus and restore the default clock speed. */
void LinuxHardwareSPI::endTransaction(void) {
  if (spiChip)
    spiChip->endTransaction();
}

/** No-op to satisfy the Arduino API. */
void LinuxHardwareSPI::attachInterrupt() {}

/** No-op to satisfy the Arduino API. */
void LinuxHardwareSPI::detachInterrupt() {}

/** Open the spidev device using the default frequency (2 MHz). */
void LinuxHardwareSPI::begin() {
  begin(defaultFreq);
}

/**
 * Open or reuse the spidev device with the given frequency.
 *
 * Looks up the device path in SPI_map to reuse an already-open LinuxSPIChip.
 * If no open chip exists, tries to open one; on failure falls back to
 * SimSPIChip so application code can still run without hardware.
 */
void LinuxHardwareSPI::begin(uint32_t freq) {
  if (!spiChip) {
#ifdef ARDULINUX_HARDWARE
    if (SPI_map[spiString] != nullptr) {
      // Reuse an already-open chip for this device path.
      spiChip = std::static_pointer_cast<SPIChip>(SPI_map[spiString]);
    } else {
      try {
        spiChip = std::make_shared<LinuxSPIChip>(spiString.c_str(), freq);
        SPI_map[spiString] = spiChip;  // Cache for future reuse
      } catch (...) {
        printf("No hardware spi chip found...\n");
      }
    }
#endif
    if (!spiChip)  // No hardware available; fall back to no-op simulator
      spiChip = std::make_shared<SimSPIChip>();
  }
}

/**
 * Override the device path and open with the given frequency.
 *
 * @param name  spidev path (e.g. "/dev/spidev1.0").  Ignored if NULL.
 * @param freq  Clock frequency in Hz.
 */
void LinuxHardwareSPI::begin(const char *name, uint32_t freq) {
  if (name != nullptr)
    spiString = std::string(name);
  begin(freq);
}

/** Release the spidev device. */
void LinuxHardwareSPI::end() {
  if (spiChip) {
    spiChip.reset();
    spiChip = NULL;
  }
}

LinuxHardwareSPI SPI;

} // namespace arduino
