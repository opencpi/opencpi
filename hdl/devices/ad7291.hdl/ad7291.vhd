--SI5351 Clock Generator I2C Device Worker
-- This worker has the register descriptions in its XML, but
-- delegates the control interface to an I2C subdevice.
architecture rtl of ad7291_worker is
begin
  -- Control plane outputs.  Raw props routed to underlying I2C
  rawprops_out.present    <= '1';
  rawprops_out.reset      <= ctl_in.reset;
  rawprops_out.raw        <= props_in.raw;
  props_out.raw           <= rawprops_in.raw;
end rtl;
