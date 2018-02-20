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

% Read in and display results from the zed_fragspex_tx_tb VHDL Testbench


testpt=1;
B = 16;
dly = 0;

%=======================================
% Retrieve the data
%=======================================



din = fopen('../idata/cosinewave.bin','r');
titlestr = 'Input - ';
dout =fopen('../odata/cosinewave.txt','w');

%din = fopen('../idata/out_cic_dwn_file.bin','r');
%titlestr = 'Input - ';
%dout =fopen('../odata/out_cic_dwn_file.txt','w');

%x = fscanf(din,'%04x');
x=fread(din);
real=x(2:2:end);
imag=x(1:2:end);
newreal=bitshift(real,8);
newx=newreal+imag;
%x = x(dly+1:end);
real=newx(1:2:end);
imag=newx(1:2:end);
%printf("%04x\n",newx);
fclose(din);
fprintf(dout,"%04x%04x\n",real,imag);
fclose(dout);
%=======================================
% Fix data format
%=======================================

% Convert to desired integer format

real = (real-2^B).*(real >= 2^(B-1)) + real.*(real < 2^(B-1));

xmax = max(abs(real))

%=======================================
% Plot the time series
%=======================================

n = length(real);
figure(1); clf;
time = (0:n-1);
stem(time,real); hold on; grid on;
xlabel('Samples')
ylabel('Magnitude')
title(strcat(titlestr,'Time Series'));

%=======================================
% Plot the spectrum
%=======================================

figure(2); clf;
win = blackman(n);
win = win/sum(win);
freq = (-0.5:1/n:0.5-1/n);
X = fftshift(20*log10(abs(fft(real .* win))));
plot(freq,X); grid on;
xlim([-1/2 1/2]);
ylim([-60 80]);
xlabel('Frequency (Hz)')
ylabel('Log-Magnitude (dB)')
title(strcat(titlestr,'Spectrum'));

