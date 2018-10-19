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

seed       = [1,1,1,1,1,1,1,1,1,1,1,1]
polynomial = [1,1,1,0,0,0,0,0,1,0,0,0] # x^12 + x^11 + x^10 + x^4 + 1
                                       # (maximal-length 12th degree polynomial)

desired_table_nrows = 4095 # period length of x^12 + x^11 + x^10 + x^4 + 1

assert len(polynomial) == len(seed)

def gen_lfsr_table():

    table = [[0] * len(seed)] * desired_table_nrows
    table[0] = seed

    for row_idx in range(1, desired_table_nrows):
        feedback = 0
        for col_idx in range(0, len(seed)):
            if polynomial[col_idx] == 1:
                feedback = feedback ^ table[row_idx-1][len(seed)-1-col_idx]
        table[row_idx] = [feedback] + table[row_idx-1][0:-1]

        #if(table[row] == table[0]):
        #    print("period =",row)

    return table
     
def main():

    lfsr_table = gen_lfsr_table()

    f = open("lfsr_tables.py", "wb")
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
    f.write(bytes("lfsr_table=[\n    ", "UTF-8"))

    gen_column = 0
    for row in lfsr_table:
        val = 0
        val_rev = 0
        shift_amt = len(seed) - 1
        for col in range(0, len(seed)):
            val += row[col] << shift_amt
            shift_amt -= 1
            val_rev += row[col] << (len(seed)-2-shift_amt)
        f.write(bytes(format(val, '#05x'), "UTF-8"))
        f.write(bytes(",", "UTF-8"))
        gen_column += 1
        if gen_column == 8:
            gen_column = 0
            f.write(bytes("\n    ", "UTF-8"))

    f.write(bytes("]\n", "UTF-8"))

    f.write(bytes("lfsr_table_rev=[\n    ", "UTF-8"))

    gen_column = 0
    for row in lfsr_table:
        val = 0
        shift_amt = len(seed) - 1
        for col in range(0, len(seed)):
            val += row[col] << (len(seed)-1-shift_amt)
            shift_amt -= 1
        f.write(bytes(format(val, '#05x'), "UTF-8"))
        f.write(bytes(",", "UTF-8"))
        gen_column += 1
        if gen_column == 8:
            gen_column = 0
            f.write(bytes("\n    ", "UTF-8"))

    f.write(bytes("]\n", "UTF-8"))
    f.close()

    #[print(row) for row in lfsr_table]
     
if __name__ == "__main__":
    main()
