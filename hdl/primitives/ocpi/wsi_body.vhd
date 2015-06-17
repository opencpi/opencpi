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
