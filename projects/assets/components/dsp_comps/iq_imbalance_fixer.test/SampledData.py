#!/usr/bin/env python
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

import numpy as np
# Cannot use pyplot in verify.py!
# import matplotlib.pyplot as plt


class SampledData:
    def __init__(self, data, fs):
        """
        Parameters
        ----------------
        data
          Sampled data.
        fs
          Sampling frequency of sampled data in Hz.
        """
        self.data = data
        self.fs = fs

    def get_time_series(self, start_time):
        return start_time + (np.arange(len(self.data), dtype=float) / self.fs)

    # Cannot use pyplot in verify.py!
    #def plot(self, start_time=0):
    #    t = self.get_time_series(start_time)
    #    plt.plot(t, np.real(self.data), '-o')
    #    plt.plot(t, np.imag(self.data), '-o', color='green')
    #    plt.xlabel('Time (sec)')
    #    plt.ylabel('Amplitude')
    #    plt.grid()
    #    plt.show()
