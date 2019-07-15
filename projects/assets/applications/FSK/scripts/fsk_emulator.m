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

do_plot = 1;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% file_read
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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
% tx fir_real_sse
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
taps_tx_rx_fir_real_sse = [85 96 105 111 115 116 114 109 100 88 72 53 31 6 ...
  -21 -50 -81 -113 -145 ...
  -177 -207 -235 -259 -280 -295 -305 -307 -302 -287 -263 -229 -184 -128 ...
  -60 20 112 217 334 462 602 752 912 1081 1257 1441 1629 1821 2015 2209 ...
  2402 2593 2779 2958 3130 3292 3443 3581 3705 3814 3906 3981 4038 4076 ...
  4096]/(2^15);
group_delay_tx_fir_real_sse_samps = length(taps_tx_rx_fir_real_sse)+0.5;
taps_tx_rx_fir_real_sse  = [taps_tx_rx_fir_real_sse ...
                            taps_tx_rx_fir_real_sse(end:-1:1)];
y_tx_fir_real_sse = conv(y_zero_pad, taps_tx_rx_fir_real_sse);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% phase_to_amp_cordic
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
magnitude_phase_to_amp_cordic = 20000/(2^15);
y_phase_to_amp_cordic = exp(j*pi*cumsum(y_tx_fir_real_sse));
y_phase_to_amp_cordic = magnitude_phase_to_amp_cordic*y_phase_to_amp_cordic;

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
% rx fir_real_sse
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
y_rx_fir_real_sse = conv(y_rp_cordic, taps_tx_rx_fir_real_sse);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% BaudTracking
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% manual baud track for now
%baud_align_idx = 12;
baud_align_idx = 12 + samps_per_baud*4;
y_BaudTracking = y_rx_fir_real_sse(baud_align_idx:samps_per_baud:end);
clear baud_align_idx

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% real_digitizer
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
y_real_digitizer = y_BaudTracking > 0;

%                (backwards cause we're calling conv)
%          <--   e        c         a         f      x0  <---
sync_pattern_real_digitizer = [-1 1 1 1 -1 -1 1 1 -1 1 -1 1 1 1 1 1];
end_of_face_idx = find(conv(sync_pattern_real_digitizer, ...
                       y_real_digitizer*2-1) == ...
                       length(sync_pattern_real_digitizer));
y_real_digitizer = y_real_digitizer(end_of_face_idx+1:end);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% plotting
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
if(do_plot)
  figure
  for m=2:-1:1
    subplot(7,1,[(m-1)*4+1,(m-1)*4+3])

    % nn+adjustment_for_indexing will be the center of the symbol pulses (which
    % is not exactly at a sample location due to even number of taps/non-integer
    % group delay
    adjustment_for_indexing=0.5;

    tmp = (group_delay_tx_fir_real_sse_samps:length(y_zero_pad)) ...
          - adjustment_for_indexing;

    if(m == 1)
      h = plot(tmp, y_zero_pad(1:length(tmp)), '.r');
      set(h, 'MarkerSize',10)
      clear h
      hold on
    end
    plot(y_tx_fir_real_sse)
    hold on

    tt = group_delay_tx_fir_real_sse_samps-adjustment_for_indexing;
    nn = tt:samps_per_baud:length(y_tx_fir_real_sse);
    clear tt
    h = plot(nn, y_tx_fir_real_sse(nn),'.');
    set(h, 'MarkerSize',10)
    clear h
    clear adjustment_for_indexing

    if(m == 1)
      title('Data processed by TX fir\_real\_sse worker in FSK application')
    elseif(m == 2)
      title({'';'Same data with alternate zoom'})
    end
    ylabel('Amplitude')

    set(gca, 'XTick', nn)

    % these values cause plot to contain portion in time where sync pattern
    % 0xFACE  occurs
    %pxstart = 74000;
    %pxend = 76000;

    pxstart = 82000;
    pxend = 84000;

    % this is super hackish
    labels = {};
    ii = 1;
    empty = 1;
    loop_start = ceil((pxstart-group_delay_tx_fir_real_sse_samps+0.5)/...
                 samps_per_baud)*samps_per_baud - 0.5 + ...
                 group_delay_tx_fir_real_sse_samps;
    for jj=loop_start:samps_per_baud:pxend
      if empty == 3
        labels{ii} = num2str(jj);
        empty = 0;
      else
        labels{ii} = '';
        empty = empty + 1;
      end
      ii = ii + 1;
    end
    clear ii
    clear jj
    clear empty
    clear loop_start
    set(gca, 'XTickLabel', labels)

    effective_gain_tx_fir_real_sse = max(abs(taps_tx_rx_fir_real_sse));
    if(m == 1)
      set(gca, 'YTick', -1:effective_gain_tx_fir_real_sse:1)
      ylim([-1.25, 2.25])
    elseif(m == 2)
      set(gca, 'YTick', 0.1:0.025:0.175)
      ylim([0.1, 0.175])
    end
    if(m == 1)
      legend('Input data', 'Output data', ...
             'Approximate center of output symbol pulse')
    elseif(m == 2)
      legend('Output data', 'Approximate center of output symbol pulse')
    end
    xlabel('Sample Number')
    xlim([pxstart, pxend])
    grid minor

    if(m == 1)
      h = stem(tmp, y_zero_pad(1:length(tmp)), 'r');
      set(h, 'MarkerSize',0)
      h = plot(nn, y_tx_fir_real_sse(nn),'.');
      set(h, 'MarkerSize',10)
      clear h
    end
  end
  clear m
  clear tmp

  figure
  ll = effective_gain_tx_fir_real_sse*ones(1,length(y_phase_to_amp_cordic));
  plot(ll, 'r')
  hold on
  plot(-ll, 'g')
  clear ll
  plot(y_tx_fir_real_sse)
  h = plot(nn, y_tx_fir_real_sse(nn),'.');
  set(h, 'MarkerSize',15)
  clear h
  xlim([pxstart, pxend])
  set(gca, 'XTick', nn)
  clear nn
  set(gca, 'XTickLabel', labels)
  clear labels
  set(gca, 'YTick', -0.25:effective_gain_tx_fir_real_sse:0.25)
  ylim([-0.25, 0.25])
  set(gca, 'YTickLabel', {'-0.25 pi', '-0.125 pi', '0', '0.125 pi', '0.25 pi'})
  ylabel('Frequency (radians/sample)')
  xlabel('Sample Number')
  t = 'FSK application instantaneous frequency of phase\_to\_amp\_cordic output'
  title(t)
  legend('f_{mark,nominal}', 'f_{space,nominal}', 'Output data', ...
         'Approximate center of output symbol pulse')
  clear t
  grid on
  clear pxstart
  clear pxend
end

disp('**********************************************************************')
disp('data below is available to plot, y_ indicates an output of a worker')
disp('**********************************************************************')
whos
