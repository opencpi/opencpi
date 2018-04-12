%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Utilization Report for Worker "phase_to_amp_cordic"
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\textbf{Worker Build Configuration ``0'':}\\
Table entries are a result of building the worker with the following parameter sets:\
\begin{itemize}
	\item \verb+ocpi_endian=little+
	\item \verb+ocpi_debug=false+
	\item \verb+STAGES=12+
	\item \verb+DATA_EXT=6+
	\item \verb+DATA_WIDTH=16+
\end{itemize}
\begin{scriptsize}
	\begin{table}[H]
	\captionsetup{justification=raggedright,singlelinecheck=false}
	\caption{Worker Build Configuration ``0''}
	\begin{tabular}{|c|c|c|c|c|c|c|c|}
		\hline
		\rowcolor{blue}
		OpenCPI Target & Tool    & Version & Device              & Registers & LUTs  & Fmax (MHz) & Memory/Special Functions                        \\
		\hline
		stratix4       & Quartus & 15.1.0  & EP4SGX230KF40C2     & 1035      & 1,350 & N/A        & \begin{tabular}{@{}c@{}}DSP18x18s=8\end{tabular}\\
		\hline
		virtex6        & ISE     & 14.7    & 6vlx240tff1156-1    & 978       & 2310  & 164.880    & \begin{tabular}{@{}c@{}}DSP48E1s=4\end{tabular} \\
		\hline
		zynq           & Vivado  & 2017.1  & xc7z020clg484-1     & 1021      & 2283  & 120.322    & \begin{tabular}{@{}c@{}}DSP48E1s=2\end{tabular} \\
		\hline
		zynq\_ise      & ISE     & 14.7    & 7z020clg484-1       & 978       & 2310  & 162.602    & \begin{tabular}{@{}c@{}}DSP48E1s=4\end{tabular} \\
		\hline
	\end{tabular}
	\end{table}
\end{scriptsize}
%\textbf{Worker Build Configuration ``1'':}\\
%Table entries are a result of building the worker with the following parameter sets:\
%\begin{itemize}
%	\item \verb+ocpi_endian=little+
%	\item \verb+ocpi_debug=false+
%	\item \verb+STAGES=16+
%	\item \verb+DATA_EXT=6+
%	\item \verb+DATA_WIDTH=16+
%\end{itemize}
%\begin{scriptsize}
%	\begin{table}[H]
%	\captionsetup{justification=raggedright,singlelinecheck=false}
%	\caption{Worker Build Configuration ``1''}
%	\begin{tabular}{|c|c|c|c|c|c|c|c|}
%		\hline
%		\rowcolor{blue}
%		OpenCPI Target & Tool    & Version & Device                 & Registers & LUTs  & Fmax (MHz) & Memory/Special Functions                        \\
%		\hline
%		stratix4       & Quartus & 15.1.0  & EP4SGX230KF40C2        & 1303      & 1,734 & N/A        & \begin{tabular}{@{}c@{}}DSP18x18s=8\end{tabular}\\
%		\hline
%		virtex6        & ISE     & 14.7    & 6vlx240tff1156-1       & 1242      & 2934  & 164.880    & \begin{tabular}{@{}c@{}}DSP48E1s=4\end{tabular} \\
%		\hline
%		zynq           & Vivado  & 2017.1  & xc7z020clg484-1        & 1287      & 2901  & 120.322    & \begin{tabular}{@{}c@{}}DSP48E1s=2\end{tabular} \\
%		\hline
%		zynq\_ise      & ISE     & 14.7    & 7z020clg484-1          & 1242      & 2934  & 162.602    & \begin{tabular}{@{}c@{}}DSP48E1s=4\end{tabular} \\
%		\hline
%	\end{tabular}
%	\end{table}
%\end{scriptsize}
