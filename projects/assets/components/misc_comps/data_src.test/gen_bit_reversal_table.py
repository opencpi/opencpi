#!/usr/bin/env python3
# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

DATA_BIT_WIDTH_p=12

def main():
    f = open("bit_reverse_table.py", "wb")

    f.write(bytes("#!/usr/bin/env python3\n", "UTF-8"))
    f.write(bytes("# This file is protected by Copyright. Please refer to the COPYRIGHT file\n", "UTF-8"))
    f.write(bytes("# distributed with this source distribution.\n", "UTF-8"))
    f.write(bytes("#\n", "UTF-8"))
    f.write(bytes("# This file is part of OpenCPI <http://www.opencpi.org>\n", "UTF-8"))
    f.write(bytes("#\n", "UTF-8"))
    f.write(bytes("# OpenCPI is free software: you can redistribute it and/or modify it under the\n", "UTF-8"))
    f.write(bytes("# terms of the GNU Lesser General Public License as published by the Free\n", "UTF-8"))
    f.write(bytes("# Software Foundation, either version 3 of the License, or (at your option) any\n", "UTF-8"))
    f.write(bytes("# later version.\n", "UTF-8"))
    f.write(bytes("#\n", "UTF-8"))
    f.write(bytes("# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY\n", "UTF-8"))
    f.write(bytes("# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR\n", "UTF-8"))
    f.write(bytes("# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more\n", "UTF-8"))
    f.write(bytes("# details.\n", "UTF-8"))
    f.write(bytes("#\n", "UTF-8"))
    f.write(bytes("# You should have received a copy of the GNU Lesser General Public License along\n", "UTF-8"))
    f.write(bytes("# with this program. If not, see <http://www.gnu.org/licenses/>.\n", "UTF-8"))
    f.write(bytes("\n", "UTF-8"))

    f.write(bytes("bit_reverse_table=[\n    ", "UTF-8"))
    column = 0
    for val in range(0,2**DATA_BIT_WIDTH_p):
        val_rev = 0
        val_rev |= (val & 0x001) << 11
        val_rev |= (val & 0x002) <<  9
        val_rev |= (val & 0x004) <<  7
        val_rev |= (val & 0x008) <<  5
        val_rev |= (val & 0x010) <<  3
        val_rev |= (val & 0x020) <<  1
        val_rev |= (val & 0x040) >>  1
        val_rev |= (val & 0x080) >>  3
        val_rev |= (val & 0x100) >>  5
        val_rev |= (val & 0x200) >>  7
        val_rev |= (val & 0x400) >>  9
        val_rev |= (val & 0x800) >> 11
        f.write(bytes(format(val_rev, '#05x'), "UTF-8"))
        f.write(bytes(",", "UTF-8"))
        column += 1
        if column == 8:
            column = 0
            f.write(bytes("\n    ", "UTF-8"))
    f.write(bytes("]\n", "UTF-8"))
    f.close()

if __name__ == "__main__":
    main()
