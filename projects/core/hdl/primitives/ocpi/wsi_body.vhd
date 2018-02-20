-- This file is protected by Copyright. Please refer to the COPYRIGHT file
-- distributed with this source distribution.
--
-- This file is part of OpenCPI <http://www.opencpi.org>
--
-- OpenCPI is free software: you can redistribute it and/or modify it under the
-- terms of the GNU Lesser General Public License as published by the Free
-- Software Foundation, either version 3 of the License, or (at your option) any
-- later version.
--
-- OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
-- WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
-- A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
-- details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program. If not, see <http://www.gnu.org/licenses/>.

-- NOTE: the width functions here do the same things as in the software in
---      tools/ocpigen/src/wsi.cxx and thus they must be in sync.
package body wsi is

  function MData_width(dataWidth : natural; byteWidth : natural) return natural is
  begin
    if dataWidth = 0 then
      return 0;
    elsif dataWidth /= byteWidth and byteWidth /= 8 then
      return (8 * dataWidth) / byteWidth;
    else
      return dataWidth;
    end if;
  end MData_width;

  function MDataInfo_width(dataWidth : natural; byteWidth : natural) return natural is
  begin
    if byteWidth /= dataWidth and byteWidth /= 8 then
      return dataWidth - ((8 * dataWidth) / byteWidth);
    else
      return dataWidth;
    end if;
  end MDataInfo_width;

  function MByteEn_width(dataWidth : natural; byteWidth : natural) return natural is
  begin
    return dataWidth / byteWidth;
  end MByteEn_width;

end package body wsi;
