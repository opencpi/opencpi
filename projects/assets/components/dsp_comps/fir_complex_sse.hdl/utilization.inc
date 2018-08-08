%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Utilization Report for Worker "fir_complex_sse"
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\textbf{Worker Build Configuration ``0'':}\\
Table entries are a result of building the worker with the following parameter sets:\
\begin{itemize}
	\item \verb+ocpi_endian=little+
	\item \verb+ocpi_debug=false+
	\item \verb+COEFF_WIDTH_p=16+
	\item \verb+DATA_WIDTH_p=16+
	\item \verb+NUM_TAPS_p=64+
\end{itemize}
\begin{scriptsize}
	\begin{table}[H]
	\captionsetup{justification=raggedright,singlelinecheck=false}
	\caption{Worker Build Configuration ``0''}
	\begin{tabular}{|c|c|c|c|c|c|c|c|}
		\hline
		\rowcolor{blue}
		OpenCPI Target & Tool    & Version & Device             & Registers & LUTs  & Fmax (MHz) & Memory/Special Functions                          \\
		\hline
		stratix4       & Quartus & 15.1.0  & EP4SGX230KF40C2    & 11671     & 9,490 & N/A        & \begin{tabular}{@{}c@{}}DSP18x18s=256\end{tabular}\\
		\hline
		virtex6        & ISE     & 14.7    & 6vlx240tff1156-1   & 8436      & 9491  & 156.495    & \begin{tabular}{@{}c@{}}DSP48E1s=128\end{tabular} \\
		\hline
		zynq           & Vivado  & 2017.1  & xc7z020clg484-1    & 8902      & 6766  & 153.304    & \begin{tabular}{@{}c@{}}DSP48E1s=128\end{tabular} \\
		\hline
		zynq\_ise      & ISE     & 14.7    & 7z020clg484-1      & 8436      & 9491  & 159.693    & \begin{tabular}{@{}c@{}}DSP48E1s=128\end{tabular} \\
		\hline
	\end{tabular}
	\end{table}
\end{scriptsize}
