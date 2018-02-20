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

% A model of a simple AGC

% === stimulated input signal ===
fileName = 'agc_tb_in';
B  = 16;        % data bit width
Fs = 16;        % sampling freq, num of samples
D  = 0.3;        % desired output, % peak
Fn = 1/Fs;      % Normalized Frequency, Fn = F/Fs
Ns = 1024;      % Number of samples
maxint = 2^(B-1)-1; % signed integer full scale
printf ("Data width:\t\t%d bits\n", B);
printf ("Sample Rate:\t\t%d samples\n", Fs);

% == DC steps
%{
Navg = Fs;                  % MA buffer length
Ref  = (D*maxint/sqrt(2));  % desired output
x1 = 0.2*maxint*ones(1,Ns);
x2 = 0.9*maxint*ones(1,Ns);
x3 = 0.2*maxint*ones(1,Ns);.

x  = horzcat(x1,x2,x3);
%}

% == slow varying DC signal
%{
Fs = 1024;
Fn = 1/Fs;
Navg = 16;                  % detector buffer length
Ref  = (D*maxint/sqrt(2));  % desired output
t = (0:Ns-1);
x = Ref+0.1*maxint*sin(2*pi*Fn*t);
%}

% == AM sinusoidal signal
Navg = Fs;                  % detector buffer length
Ref  = (D*maxint/sqrt(2));  % desired output in rms
t1 = (0:Ns-1);
x1 = 0.2*maxint*cos(2*pi*Fn*t1);
t2 = (Ns:2*Ns-1);
x2 = 0.9*maxint*cos(2*pi*Fn*t2);
t3 = (2*Ns:3*Ns-1);
x3 = 0.2*maxint*cos(2*pi*Fn*t3);
t4 = (3*Ns:4*Ns-1);
x4 = 0.3*maxint*cos(2*pi*Fn*t4);
t  = horzcat(t1,t2,t3,t4);
x  = horzcat(x1,x2,x3,x4);
%}
printf ("MA Length, Navg:\t%d samples\n", Navg);
printf ("Desired Output Ref: \t0x%X \t%d%% max\n", Ref, D*100);

% == setting feedback coefficient
S    = 2*Navg*(maxint-Ref);
Mu   = 1/S;                 % ideally should be <= this number
Mu_s = Mu*2^(2*B);
printf ("Scale Factor Mu: \t0x%X \t%e\n", Mu_s, Mu);
if (Mu_s > maxint)          % check for overflow
  printf ("Scale Factor Mu 0x%x is too large !!!\n", Mu_s);
endif

% == save the generated signal for VHDL simulation
range = 2^B;
xint = round(x);
nibs = ceil(B/4);
z = (xint >= 0).*xint + (xint < 0).*(xint + range); % convert to unsigned
fmt = strcat('%0',num2str(nibs),'X\n');  % save as signed hex numbers
fid = fopen(strcat(fileName,'.txt'),'w'); % write out to a text file
fprintf(fid,fmt,z);
fclose(fid);
fid = fopen(strcat(fileName,'.bin'),'wb'); % write out to a binary file
fwrite(fid,xint,'int16');
fclose(fid);


% ========= AGC ==========
N       = length(x);     % number of samples in simulation
det_buf = zeros(1,Navg); % buffer to measure output signal

% initial values
gain = ones (1,N);  % loop gain
err  = zeros(1,N);  % loop error
ydet = zeros(1,N);  % output detected
y    = zeros(1,N);  % output

for i = Navg:N % lagging by Navg samples
  % detecting output level
  det_buf(1:Navg) = y((i-Navg+1):i); % buffering
  ydet(i) = sum(abs(det_buf))/Navg;

  % compare to reference
  err(i) = (Ref - ydet(i));

  % correct the gain to VGA
  gain(i) = gain(i-1) + err(i)*Mu;
  if abs(gain(i)) > maxint % limit to max
    gain(i) = (gain(i)>=maxint).*maxint + (gain(i)< maxint).*(-maxint);
  endif

  % VGA, variable gain amplifier
  y(i+1) = gain(i) * x(i);
endfor

% ========================
% plots
figure(1);
n = 1:1:N;
subplot(2,1,1);
plot (n,x/maxint, n,y(2:end)/maxint);
%axis([1 N]);
axis([1 N -1 1]);
grid minor;
title('Simple AGC w/ fixed feedback coefficient');
xlabel('Time(samples)'); ylabel('Amplitude');
legend('Input','Output');

subplot(2,1,2);
plotyy(n,gain(1:N),n,err/maxint);
axis([1 N]);
%axis([1 N -100 100]);
grid minor;
xlabel('Time(samples)'); ylabel('Amplitude');
legend('Loop Error','Loop Gain',"location","southeast");

