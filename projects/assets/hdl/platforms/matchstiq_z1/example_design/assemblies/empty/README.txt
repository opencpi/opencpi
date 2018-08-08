This file is protected by Copyright. Please refer to the COPYRIGHT file
distributed with this source distribution.

This file is part of OpenCPI <http://www.opencpi.org>

OpenCPI is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.

This directory is a container for the "empty" assembly from the OpenCPI open source repository.
The container targets the matchstiq_z1 platform
The containers instances and connects the following modules:
 - lime_tx.hdl
 - lime_rx.hdl
  - the subdevice lime_spi.hdl is automatically instanced to support lime_tx and lime_rx
 - lime_adc.hdl
 - lime_dac.hdl
 - si5338.hdl
 - matchstiq_z1_avr.hdl
 - tmp100.hdl
 - pca9534.hdl
 - pca9535.hdl
  - matchstiq_z1_i2c.hdl is automatically instances to support the i2c workers
The primary purpose of this container is to provide a example design that allows for streaming samples to/from file.

