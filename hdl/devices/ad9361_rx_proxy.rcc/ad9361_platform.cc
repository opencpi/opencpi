/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * We don't use the header "platform.h" to declare these functions because that header
 * is very unclean and introduces all manner of naming collisions.
 */
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
extern "C" {
#include "ad9361.h"
}
#include "RCC_Worker.h"

extern "C" {
// This function accesses the property space of the worker.
// We figure out the worker object from thread private data to avoid
// any re-entrancy issues (e.g. when there are more than one ad9361s).
int
spi_write_then_read(struct spi_device *spi, const unsigned char *txbuf, unsigned n_tx,
		    unsigned char *rxbuf, unsigned n_rx) {
  uint16_t
    cmd = txbuf[0] << 8 | txbuf[1],
    addr = AD_ADDR(cmd),
    count = (cmd >> 12) + 1;
  bool isRead = cmd & AD_READ;
  // This is actually a call that either writes data or reads data
  // The n_tx is 2 bytes of "command" followed by write data if it is a write.
  assert(n_rx && n_tx == 2 || !n_rx && n_tx > 2);
  assert(n_rx == 0 || n_rx == count);
  assert(n_tx == 2 || n_tx-2 == count);
  OCPI::RCC::RCCUserSlave *s = (OCPI::RCC::RCCUserSlave*)(spi->dev.slave);
  if (isRead)
    s->getRawPropertyBytes(addr, rxbuf, count);
  else
    s->setRawPropertyBytes(addr, txbuf+2, count);
  return -ENODEV;
}

// GPIOs here are not the chip GPIOs, but the system GPIOs that might connect
// to some of the external pins of the ad9361.  There are a number that could potentially
// be connected to SW-based GPIOs, but only two are used by the API and these could be
// SW or FPGA on different systems.

// GPIOs that apply to RX and TX

// SYNC_IN (H5) input to chip, syncs sampling and digital interfaces RX/TX
// RESETB (K5) input to chip, global reset RX/TX
// CTRL_OUT (D4,E4/6,F4/6,G4) output from chip

// RX ONLY

// EN_AGC (G5) input to chip, enable control of receive path gain, RX
// ENABLE (G6) input to chip, enable RX (tighter timing than using SPI)
// CTRL_IN (C5/6,D6/5) input to chip, real time AGC inputs for various modes RX

// TX ONLY

// TXNRX (H4) input to chip, enable TX (tigher timing than using SPI)






// ================================================================================
// GPIO functions are used when the GPIO pins are controlled by SW, not FPGA
// Even though there are three different data types (uint32_t, uint16_t, int),
// then are a sort of configurable virtual pin identifier
// ADI SW only uses them for global reset and global sync
// The other potential uses are probably only sensible in the FPGA

// If SW then you need to "open" the virtual GPIO pin
void
gpio_init(uint32_t device_id) {
}

// Configure the direction, knowing what it is connected to, so another SW_only thing
void
gpio_direction(uint16_t pin, uint8_t direction) {
}

// A double check whether a given gpio is actually usable
bool
gpio_is_valid(int number) {
  return false;
}

// This is what could decide whether it is a SW pin or an FPGA pin for sync
void
gpio_set_value(unsigned gpio, int value) {
}

// From ADI linux platform code
void
udelay(unsigned long usecs) {
  usleep(usecs);
}

// From ADI linux platform code
void
mdelay(unsigned long msecs) {
  usleep(msecs * 1000);
}

#if 0
clk_get_rate
clk_prepare_enable
clk_set_rate
do_div
ERR_PTR
find_first_bit
ilog2
int_sqrt
#endif
  //_ad9361_dig_tune
  //_ad9361_hdl_loopback

}
