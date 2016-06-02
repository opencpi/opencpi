/*
 * We don't use the header "platform.h" to declare these functions because that header
 * is very unclean and introduces all manner of naming collisions.
 */
extern "C" {
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

int
spi_write_then_read(struct spi_device *spi, const unsigned char *txbuf, unsigned n_tx,
		    unsigned char *rxbuf, unsigned n_rx) {
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

  //_ad9361_dig_tune
  //_ad9361_hdl_loopback

}
