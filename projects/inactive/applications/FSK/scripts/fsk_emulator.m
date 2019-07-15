% This file is protected by Copyright. Please refer to the COPYRIGHT file
% distributed with this source distribution.
%
% This file is part of OpenCPI <http://www.opencpi.org>
%
% OpenCPI is free software: you can redistribute it and/or modify it under the
% terms of the GNU Lesser General Public License as published by the Free
% Software Foundation, either version 3 of the License, or (at your option) any
% later version.
%
% OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
% WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
% A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
% details.
%
% You should have received a copy of the GNU Lesser General Public License along
% with this program. If not, see <http://www.gnu.org/licenses/>.

pkg load signal % resample()

fsk_emulator_load_file

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% mfsk_mapper
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
y_mfsk_mapper = (os_jpeg_binary*65535 - 32768)/(2^15);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% zero_pad
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
num_zeros = 38;
samps_per_baud = num_zeros+1;
y_zero_pad = reshape([y_mfsk_mapper; zeros(num_zeros, ...
                     length(y_mfsk_mapper))], 1, ...
                     length(y_mfsk_mapper)*(num_zeros+1));

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% fir_real_sse
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
taps_tx_rx_fir_real_sse = [85 96 105 111 115 116 114 109 100 88 72 53 31 6 ...
  -21 -50 -81 -113 -145 ...
  -177 -207 -235 -259 -280 -295 -305 -307 -302 -287 -263 -229 -184 -128 ...
  -60 20 112 217 334 462 602 752 912 1081 1257 1441 1629 1821 2015 2209 ...
  2402 2593 2779 2958 3130 3292 3443 3581 3705 3814 3906 3981 4038 4076 ...
  4096]/(2^15);
taps_tx_rx_fir_real_sse  = [taps_tx_rx_fir_real_sse ...
                            taps_tx_rx_fir_real_sse(end:-1:1)];
y_tx_fir_real_sse = conv(y_zero_pad, taps_tx_rx_fir_real_sse);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% phase_to_amp_cordic
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
y_phase_to_amp_cordic = exp(j*pi*cumsum(y_tx_fir_real_sse));

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% cic_int
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
R_cic_int_dec = 16;
y_cic_int = resample(y_phase_to_amp_cordic, R_cic_int_dec, 1);
%pwelch(y_cic_int)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% cic_dec
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
y_cic_dec = resample(y_cic_int, 1, R_cic_int_dec);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% rp_cordic
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
y_rp_cordic = diff(angle(y_cic_dec));
y_rp_cordic(find(y_rp_cordic < -pi)) = y_rp_cordic(find(y_rp_cordic < -pi)) ...
                                       + 2*pi;
y_rp_cordic(find(y_rp_cordic > pi)) = y_rp_cordic(find(y_rp_cordic > pi)) ...
                                      - 2*pi;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% fir_real_sse
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
y_rx_fir_real_sse = conv(y_rp_cordic, taps_tx_rx_fir_real_sse);
do_plot = 0;
if(do_plot)
  plot(y_rx_fir_real_sse)
  hold on
  plot(y_tx_fir_real_sse, 'r')
  legend('y_rx_fir_real_sse', 'y_tx_fir_real_sse')
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% BaudTracking
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% manual baud track for now
%baud_align_idx = 12;
baud_align_idx = 12 + 39*4;
x_BaudTracking = y_rx_fir_real_sse(baud_align_idx:samps_per_baud:end);
clear baud_align_idx

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% real_digitizer
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
y_real_digitizer = x_BaudTracking > 0;

%                (backwards cause we're calling conv)
%          <--   e        c         a         f      x0  <---
sync_pattern_real_digitizer = [-1 1 1 1 -1 -1 1 1 -1 1 -1 1 1 1 1 1];
end_of_face_idx = find(conv(sync_pattern_real_digitizer, ...
                       y_real_digitizer*2-1) == ...
                       length(sync_pattern_real_digitizer));
y_real_digitizer = y_real_digitizer(end_of_face_idx+1:end);

disp('*****************************************************************')
disp('this script doesn''t plot or do anything fancy yet, just does')
disp('important calculations and leaves results in variables for')
disp('manual inspection, recommend running ''whos'' and also looking')
disp('at source code for more info')
disp('*****************************************************************')
