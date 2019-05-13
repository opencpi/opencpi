#!/usr/bin/env python2
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

"""
CIC Decimator: Verify output data

Verify args:
1. amplitude used to generate input data file
2. name of output data file
3. name of input data file

Tests:
#1: Not all zeros
#2: Is the expected amount
#3: Matches the expected output
"""
import datetime
import os.path
import sys
import opencpi.colors as color
import numpy as np
import matplotlib.pyplot as plt

# TODO / FIXME - delete this and use class from iq_imbalance_fixer.test instead
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
      return start_time + (np.arange(len(self.data),dtype=float) / self.fs)

  def plot(start_time = 0):
      t = get_time_series(start_time)
      plt.plot(t, np.real(sampled_data.data), '-o')
      plt.plot(t, np.imag(sampled_data.data), '-o', color='green')
      btm_subplot.set_xlabel('Frequency (Hz)')
      btm_subplot.set_ylabel('Amplitude')
      plt.grid()
      plt.show()

# TODO / FIXME - delete this and use class from iq_imbalance_fixer.test instead
class DFTResult:
    """
    Represents the result of a one-dimensional Discrete Fourier Transform. For
    more info, see
    https://en.wikipedia.org/wiki/Discrete_Fourier_transform.
    """
    def __init__(self, sampled_data, n = None, norm = None):
        """
        Parameters
        ----------------
        sampled_data
            Sampled data on which DFT will be calculated.
        n
            Length of the transformed axis of the output.
        norm
            Normalization mode.
        """
        self.norm = norm
        self.amplitudes = np.fft.fft(sampled_data.data, n, norm = norm)
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

    def calc(self, n = None, norm = None):
        """
        Parameters
        ----------------
        n
            Length of the transformed axis of the output.
        norm
            Normalization mode.
        """
        self.result = DFTResult(self.sampled_data, n, norm)
        self.norm = norm
        return self.result

    def get_idx_of_nearest_freq_in_result(self, f):
        """ This has been verified both for positive and negative frequencies.
        Parameters
        ----------------
        f
            Frequency value in Hz.
        """
        fs = self.sampled_data.fs
        idx = int(round(float(f)/float(fs)*float(len(self.result.amplitudes))))
        return idx % self.result.get_num_dft_points()

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
        if self.norm == None:
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
        else:
            # TODO / FIXME - replace exception with functionality
            msg = "abandoning amplitude calc because implementation is not yet "
            msg += "complete for calculation with DFT normalization"
            raise Exception(msg)
        return ret

    def get_max_magnitude_of_positive_freqs(self,
        unit = "dB_relative_to_unity"):
        if self.norm == None:
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
        else:
            # TODO / FIXME - replace exception with functionality
            msg = "abandoning amplitude calc because implementation is not yet "
            msg += "complete for calculation with DFT normalization"
            raise Exception(msg)
        return ret

    def get_max_magnitude_of_negative_freqs(self,
        unit = "dB_relative_to_unity"):
        if self.norm == None:
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
        else:
            # TODO / FIXME - replace exception with functionality
            msg = "abandoning amplitude calc because implementation is not yet "
            msg += "complete for calculation with DFT normalization"
            raise Exception(msg)
        return ret

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

def calc_nearest_freq_and_mag(desired_freq, calc, pre_msg):
    nearest_freq = calc.get_nearest_freq_in_result(desired_freq)
    mag = calc.get_magnitude_of_nearest_freq_in_result(desired_freq,
        unit="dB_relative_to_unity")
    msg = pre_msg + 'Tone at   ' + str(nearest_freq) + ' Hz has magnitude of '
    msg += str(mag) + '\tdB relative to unity'
    print(msg)
    return [nearest_freq, mag]

def test_expected_min_gain_diff(freq, in_calc, out_calc, min_allowed_gain_diff_dB):
    """
    Parameters
    ----------------
    freq
        Frequency in Hz for tone to be tested
    in_calc
        DFTCalculator object for data input to iq_imbalance_fixer
    out_calc
        DFTCalculator object for data output from iq_imbalance_fixer
    min_allowed_gain_diff_dB
        Minimum pre-to-post tone gain difference for which a test will succeed.
    """
    [in_freq, in_mag]   = calc_nearest_freq_and_mag(freq, in_calc,  "Input        ")
    [out_freq, out_mag] = calc_nearest_freq_and_mag(freq, out_calc, "Output       ")

    gain = out_mag - in_mag
    if abs(gain) < min_allowed_gain_diff_dB:
        msg = 'FAILED, Tone in->out gain = ' + str(gain)
        msg += " dB, which was less than the minimum "
        msg += "allowed difference of " + str(min_allowed_gain_diff_dB) + " dB"
        print(color.RED + color.BOLD + msg + color.END)
        sys.exit(1)

def test_expected_max_gain_diff(freq, in_calc, out_calc, max_allowed_gain_diff_dB):
    """
    Parameters
    ----------------
    freq
        Frequency in Hz for tone to be tested
    in_calc
        DFTCalculator object for data input to iq_imbalance_fixer
    out_calc
        DFTCalculator object for data output from iq_imbalance_fixer
    max_allowed_gain_diff_dB
        Maximum pre-to-post tone gain difference for which a test will succeed.
    """
    [in_freq, in_mag]   = calc_nearest_freq_and_mag(freq, in_calc,  "Input        ")
    [out_freq, out_mag] = calc_nearest_freq_and_mag(freq, out_calc, "Output       ")

    gain = out_mag - in_mag
    if abs(gain) > max_allowed_gain_diff_dB:
        msg = 'FAILED, Tone in->out gain = ' + str(gain)
        msg += " dB, which was greater than the maximum "
        msg += "allowed difference of " + str(max_allowed_gain_diff_dB) + " dB"
        print(color.RED + color.BOLD + msg + color.END)
        sys.exit(1)

def myround(x, base=8):
    return base * round(float(x) / base)

if len(sys.argv) != 4:
    print("Invalid arguments:  usage is: verify.py <amplitude> <output-file> <input-file>")
    sys.exit(1)
print "    VALIDATE (I/Q 16b binary data file):"

# from OCS or OWD
N=int(os.environ.get("OCPI_TEST_N"))
M=int(os.environ.get("OCPI_TEST_M"))
R=int(os.environ.get("OCPI_TEST_R"))

# from -test.xml (properties that are declared to be test='true')
Ft = int(os.environ.get("OCPI_TEST_TARGET_FREQ"))      # target frequency

print '    UUT:(N=%d, M=%d, R=%d) Test Data:(%d)' % (N,M,R,Ft)

# from arguments to generate.py (-test.xml)
AMPLITUDE = int(sys.argv[1])

# Sample frequency used in generate.py
Fs = float(1024000)
# Sample frequency after decimation.
Fs_dec = Fs / float (R)

# Declare complex data type
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))

# Read input and output data as complex int16 samples
ofilename = open(sys.argv[2], 'rb')
odata = np.fromfile(ofilename, dtype=dt_iq_pair, count=-1)
ofilename.close()
ifilename = open(sys.argv[3], 'rb')
idata = np.fromfile(ifilename, dtype=dt_iq_pair, count=-1)
ifilename.close()

# Deinterleave numpy.array into I & Q arrays
real = odata['real_idx']
imag = odata['imag_idx']

# Test #1 - Check that output data is not all zeros
if all(odata == 0):
    print '    ' + color.RED + color.BOLD + 'FAIL, values are all zero' + color.END
    sys.exit(1)
else:
    print '    PASS - File is not all zeros'

# Test #2 - Check that output data is the expected amount
ifile_nbytes = int(os.stat(sys.argv[3]).st_size)
#print 'DBG:  Input filesize/R:', sys.argv[3],"/",ifile_nbytes,"bytes","/",ifile_nbytes / 4,"samples"
ofile_nbytes = int(os.stat(sys.argv[2]).st_size)
#print 'DBG: Output filesize:\t', sys.argv[2],"/",ofile_nbytes,"bytes","/",ofile_nbytes / 4,"samples"
# Compare input file size (adjusted by decimation factor) and output file size
# Need special rounding to adjust for 'odd' valued decimation factors
if (myround((ifile_nbytes / R), 8) != ofile_nbytes):
    print color.RED, color.BOLD, '    FAIL - Output file is not the correct size', color.END
    sys.exit(1)
else:
    print '    PASS - Output file is the correct size'

# Test #3 - Check that output data values: Unity Gain Response or Tone
if (Ft == 0): # Unity Gain Response (DC input results in DC output)
    # Calculate the expected output DC value, by scaling the input DC value
    # based on the configuration of the worker under test
    AMPLITUDE = int(AMPLITUDE * ((R*M)**N) / 2**np.ceil((N*(np.log2(R*M))))) #overwrite
    #print 'DBG: Test for Unity Gain Response: expected amplitude =', AMPLITUDE
    # Skip startup data, then compare the steady-state output for expected DC value
    skip_index = (N+M)*2
    #print 'DBG: skip_index and I/Q values:', skip_index, real[skip_index], imag[skip_index]
    if all(real[skip_index::] != AMPLITUDE) and all(imag[skip_index::] != AMPLITUDE):
        print color.RED, color.BOLD, '    FAIL - I or Q values do not match the expected constant value of', AMPLITUDE, color.END
        sys.exit(1)
    else:
        #nsamples_matched = len(real[skip_index::] / 2)
        print '    PASS - I & Q values match the expected constant value of', AMPLITUDE
else: # Measure and compare power of tone(s)
    T1 = 50.0
    T2 = 100.0
    T3 = Fs / R # In the first null of the CIC filter
    start_time = datetime.datetime.now()
    print '    Start time =', start_time

    ofile_nsamples = ofile_nbytes / 4
    ifile_nsamples = ifile_nbytes / 4

    #print 'DBG: ifile_nbytes=%d ifile_nsamples=%d' % (ifile_nbytes, ifile_nsamples)
    #print 'DBG: ofile_nbytes=%d ofile_nsamples=%d' % (ofile_nbytes, ofile_nsamples)

    # Construct complex array of vectors for performing FFT
    complex_idata = np.array(np.zeros(ifile_nsamples/2), dtype=np.complex)
    for i in xrange(0,ifile_nsamples/2):
        complex_idata[i] = complex(idata['real_idx'][i], idata['imag_idx'][i])
    complex_odata = np.array(np.zeros(ofile_nsamples), dtype=np.complex)
    for i in xrange(0,ofile_nsamples):
        complex_odata[i] = complex(odata['real_idx'][i], odata['imag_idx'][i])

    in_calc = DFTCalculator(SampledData(complex_idata, Fs))
    out_calc = DFTCalculator(SampledData(complex_odata, Fs_dec))
    in_calc.calc(n = ifile_nsamples/2)
    out_calc.calc(n = ofile_nsamples)
    #in_calc.plot()
    #out_calc.plot()

    # Report time needed to perform FFTs
    end_time = datetime.datetime.today()
    print '    End time =', end_time
    print '    Elapsed time:', end_time - start_time

    # Perform calculations comparing output power to input power
    # Note: Comparison values were determined empirically across entire test suite
    if (M == 2 and R == 8192):
        test_expected_min_gain_diff(T1, in_calc, out_calc, 37.0)
    elif (M == 2 and R == 8191):
        test_expected_min_gain_diff(T1, in_calc, out_calc, 39.0)
    elif (R == 8191):
        test_expected_min_gain_diff(T1, in_calc, out_calc, 8.6)
    else:
        test_expected_max_gain_diff(T1, in_calc, out_calc, 8.0)

    if (R >= 8191):
        test_expected_min_gain_diff(T2, in_calc, out_calc, 37.0)
    else:
        test_expected_max_gain_diff(T2, in_calc, out_calc, 8.0)

    test_expected_min_gain_diff(T3, in_calc, out_calc, 57.0)

print '    Data matched expected results.'
print '    ' + color.GREEN + color.BOLD + 'PASSED' + color.END
