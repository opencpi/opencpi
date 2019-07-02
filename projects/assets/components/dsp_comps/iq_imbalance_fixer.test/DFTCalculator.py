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

import numpy as np
#import matplotlib.pyplot as plt
from SampledData import SampledData

class DFTResult:
    """
    Represents the result of a one-dimensional Discrete Fourier Transform. For
    more info, see
    https://en.wikipedia.org/wiki/Discrete_Fourier_transform.
    >>> amp = 1; fs = 100.; phi = 0.; num_samples=1024
    >>> t = np.arange(num_samples, dtype=float) / fs
    >>> f = -23.4293
    >>> x = SampledData(data = amp*np.exp(1j*(2*np.pi*f*t + phi)), fs = fs)
    >>> calculator = DFTCalculator(x)
    >>> _ = calculator.calc(n = len(x.data));
    >>> desired_freq = f
    >>> nearest_freq = calculator.get_nearest_freq_in_result(desired_freq)
    >>> mag = calculator.get_magnitude_of_nearest_freq_in_result(desired_freq, \
                unit="dB_relative_to_unity")
    >>> msg = 'Amplitude of ' + str(nearest_freq) + ' is '
    >>> msg += str(mag) + ' dB relative to unity'
    >>> print(msg)
    Amplitude of -23.4375 is -0.10097193371793894 dB relative to unity
    >>> t = np.arange(num_samples, dtype=float) / fs
    >>> f = 23.4293
    >>> x = SampledData(data = amp*np.exp(1j*(2*np.pi*f*t + phi)), fs = fs)
    >>> calculator = DFTCalculator(x)
    >>> _ = calculator.calc(n = len(x.data));
    >>> desired_freq = f
    >>> nearest_freq = calculator.get_nearest_freq_in_result(desired_freq)
    >>> mag = calculator.get_magnitude_of_nearest_freq_in_result(desired_freq, \
                unit="dB_relative_to_unity")
    >>> msg = 'Amplitude of ' + str(nearest_freq) + ' is '
    >>> msg += str(mag) + ' dB relative to unity'
    >>> print(msg)
    Amplitude of 23.4375 is -0.10097193371793992 dB relative to unity
    >>> calculator.plot(dft_y_axis_units = "dB_relative_to_unity")
    """
    def __init__(self, sampled_data, n = None):
        """
        Parameters
        ----------------
        sampled_data
            Sampled data on which DFT will be calculated.
        n
            Length of the transformed axis of the output.
        """
        self.amplitudes = np.fft.fft(sampled_data.data, n)
        end = sampled_data.fs*(1-(1/n))
        self.freq_bins = np.arange(0., end, sampled_data.fs/n)

    def get_num_dft_points(self):
        return len(self.amplitudes)

    def get_unity_magnitude_normalization_factor(self):
        """
        Returns
        ----------------
            Returns amplitude relative to sampled_data unity, e.g. if
            sampled_data is a complex sinusoid of magnitude 1, the maximum value
            within the DFT result should be close to 1 after the result is
            multiplied by the value returned.
        """
        return 1. / self.get_num_dft_points()

class DFTCalculator():
    """
    Calculates a one-dimensional Discrete Fourier Transform. For more info, see
    https://en.wikipedia.org/wiki/Discrete_Fourier_transform.
    """
    def __init__(self, sampled_data):
        self.sampled_data = sampled_data

    def calc(self, n = None):
        """
        Parameters
        ----------------
        n
            Length of the transformed axis of the output.
        """
        self.result = DFTResult(self.sampled_data, n)
        return self.result

    def get_idx_of_nearest_freq_in_result(self, f):
        """ This has been verified both for positive and negative frequencies.
        Parameters
        ----------------
        f
            Frequency value in Hz.
        """
        fs = self.sampled_data.fs
        #return int(round(float(f)/float(fs)*float(len(self.result.amplitudes))))
        return int(round(float(f)/float(fs)*float(len(self.result.amplitudes))))

    def get_freq_for_idx_in_result(self, idx):
        """
        Parameters
        ----------------
        idx
            Zero-based index of DFT result, where 0 corresponds to 0 Hz, and the
            last index of the DFT result plus one corresponds to fs Hz.
        """
        fs = self.sampled_data.fs
        return float(idx)/float(self.result.get_num_dft_points())*float(fs)

    def get_nearest_freq_in_result(self, f):
        """
        Parameters
        ----------------
        f
            Frequency value in Hz.
        """
        idx = self.get_idx_of_nearest_freq_in_result(f)
        return self.get_freq_for_idx_in_result(idx)

    def get_magnitude_of_nearest_freq_in_result(self, f,
            unit = "dB_relative_to_unity"):
        """
        Parameters
        ----------------
        f
            Frequency value in Hz.
        Returns
        ----------------
            When unit is "dB_relative_to_unity", returns amplitude in dB
            relative to sampled_data unity, e.g. if sampled_data is a complex
            sinusoid of amplitude 1, the returned value should be
            close to 0 dB.
        """
        eps = pow(10, -10) # error factor to avoid divide by zero in log10
        idx = self.get_idx_of_nearest_freq_in_result(f)
        if unit == None:
            factor = 1
        elif unit == "dB_relative_to_unity":
            factor = self.result.get_unity_magnitude_normalization_factor()
        else:
            msg = "unit was was unsupported value of " + unit
            msg += ", supported values are None and dB_relative_to_unity"
            raise Exception(msg)
        abs_amp = abs(self.result.amplitudes[idx] * factor)
        ret = 20*np.log10(abs_amp + eps)
        return ret

    def get_max_magnitude_of_positive_freqs(self,
        unit = "dB_relative_to_unity"):
        eps = pow(10, -10) # error factor to avoid divide by zero in log10
        if unit == None:
            factor = 1
        elif unit == "dB_relative_to_unity":
            factor = self.result.get_unity_magnitude_normalization_factor()
        else:
            msg = "unit was was unsupported value of " + unit
            msg += ", supported values are None and dB_relative_to_unity"
            raise Exception(msg)
        nn = self.result.get_num_dft_points()
        result_pos_freqs = self.result.amplitudes[0:(nn/2)-1]
        tmp = 20*np.log10(abs(result_pos_freqs) + eps)
        ret = tmp[np.argmax(tmp)]
        return ret

    def get_max_magnitude_of_negative_freqs(self,
        unit = "dB_relative_to_unity"):
        eps = pow(10, -10) # error factor to avoid divide by zero in log10
        if unit == None:
            factor = 1
        elif unit == "dB_relative_to_unity":
            factor = self.result.get_unity_magnitude_normalization_factor()
        else:
            msg = "unit was was unsupported value of " + unit
            msg += ", supported values are None and dB_relative_to_unity"
            raise Exception(msg)
        nn = self.result.get_num_dft_points()
        result_neg_freqs = self.result.amplitudes[nn/2:nn-1]
        tmp = 20*np.log10(abs(result_neg_freqs) + eps)
        ret = tmp[np.argmax(tmp)]
        return ret
"""
    def plot(self, dft_y_axis_units = "dB_relative_to_unity", start_time = 0):
        t = self.sampled_data.get_time_series(start_time)
        fig = plt.figure(1)
        top_subplot = fig.add_subplot(2, 1, 1)
        top_subplot.plot(t, np.real(self.sampled_data.data), '-o')
        top_subplot.plot(t, np.imag(self.sampled_data.data), '-o', color='red')
        top_subplot.set_xlabel('Time (Sec) After Sampling Start')
        top_subplot.set_ylabel('Amplitude')
        top_subplot.grid()
        top_subplot.set_xlim(min(t), max(t))
        top_subplot.legend(['Real', 'Imag'])

        btm_subplot = fig.add_subplot(2, 1, 2)
        bins = self.result.freq_bins

        # classic DFT calculation result's amplitude must be normalized for proper
        # plotting
        factor = None
        if dft_y_axis_units == None:
            btm_subplot.plot(bins, abs(self.result.amplitudes), 'o')
            btm_subplot.set_ylabel('DFT Output Magnitude')
        elif dft_y_axis_units == "dB_relative_to_unity":
            factor = self.result.get_unity_magnitude_normalization_factor()
            amplitudes_to_plot = self.result.amplitudes * factor
            eps = pow(10, -10) # error factor to avoid divide by zero in log10
            btm_subplot.plot(bins, 20*np.log10(abs(amplitudes_to_plot)+eps), 'o')
            btm_subplot.set_ylabel('Magnitude in dB relative to unity')
        else:
            msg = "dft_y_axis_units was " + dft_y_axis_units
            msg += ", supported values are None and dB_relative_to_unity"
            raise Exception(msg)

        #btm_subplot.plot(bins, abs(amplitudes_to_plot), 'o')
        #btm_subplot.set_ylabel('Amplitude')
        btm_subplot.set_xlabel('Frequency (Hz)')
        btm_subplot.grid()
        plt.suptitle('Sampled Data, fs = '+ str(self.sampled_data.fs) + ' Hz')
        plt.show()
"""

if __name__ == "__main__":
    import doctest
    doctest.testmod()

