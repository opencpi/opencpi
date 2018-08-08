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

% Plot results from VHDL simulation input vs. output

%fileName = input("file to plot? ","s");

% getting data from input file
ifileName  = "agc_tb_in.txt";
A = 16;                   % Input data width
imaxint = 2^(A-1)-1;
irange = 2^A;
ifid = fopen(ifileName,'r');
[din,Nin] = fscanf(ifid,'%x');
fclose(ifid);
d = din + (din > imaxint)*(-irange); % convert to 2's

% getting data from input file
ofileName = "agc_tb_out.txt";
B = 16;                   % Output data width
omaxint = 2^(B-1)-1;
orange = 2^B;
ofid = fopen(ofileName,'r');
%[qout,Nout] = fscanf(ofid,'%lx'); % 64-b
[qout,Nout] = fscanf(ofid,'%x');
fclose(ofid);
q = qout + (qout > omaxint)*(-orange); % convert to 2's

% plot input vs. output
nin  = 1:1:Nin;
nout = 1:1:Nout;
figure(2);
plotyy(nin,d/imaxint, nout,q/omaxint);
%plot(nin,d, nout,q);
grid minor;

