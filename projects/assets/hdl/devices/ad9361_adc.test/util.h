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

#ifndef _UTIL_H
#define _UTIL_H

#include <iostream>

#define PRINT_VAR(x) std::cout << #x << "=" << x << std::endl;
#define PRINT_VARE(x) std::cerr << #x << "=" << x << std::endl;

void print_error(const char* c, bool do_endl = true)
{
  std::cout << "ERROR caused premature exit: " << c;
  if(do_endl)
  {
    std::cout << std::endl;
  }
}

template<typename T> T GETBIT(const size_t idx, T x)
{
  switch(idx)
  {
    case 0:
      return (x & 0x00000001) >> idx;
    case 1:
      return (x & 0x00000002) >> idx;
    case 2:
      return (x & 0x00000004) >> idx;
    case 3:
      return (x & 0x00000008) >> idx;
    case 4:
      return (x & 0x00000010) >> idx;
    case 5:
      return (x & 0x00000020) >> idx;
    case 6:
      return (x & 0x00000040) >> idx;
    case 7:
      return (x & 0x00000080) >> idx;
    case 8:
      return (x & 0x00000100) >> idx;
    case 9:
      return (x & 0x00000200) >> idx;
    case 10:
      return (x & 0x00000400) >> idx;
    case 11:
      return (x & 0x00000800) >> idx;
    case 12:
      return (x & 0x00001000) >> idx;
    case 13:
      return (x & 0x00002000) >> idx;
    case 14:
      return (x & 0x00004000) >> idx;
    default:
      return (x & 0x00008000) >> idx;
  }
}

#endif //_UTIL_H
