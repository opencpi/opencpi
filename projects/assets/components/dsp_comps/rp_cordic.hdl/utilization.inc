%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Utilization Report for Worker "rp_cordic"
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\textbf{Worker Build Configuration ``0'':}\\
Table entries are a result of building the worker with the following parameter sets:\
\begin{itemize}
	\item \verb+ocpi_endian=little+
	\item \verb+ocpi_debug=false+
	\item \verb+STAGES=16+
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
		OpenCPI Target & Tool    & Version & Device             & Registers & LUTs  & Fmax (MHz) & Memory/Special Functions                         \\
		\hline
		stratix4       & Quartus & 15.1.0  & EP4SGX230KF40C2    & 1529      & 2,514 & N/A        & \begin{tabular}{@{}c@{}}DSP18x18s=12\end{tabular}\\
		\hline
		virtex6        & ISE     & 14.7    & 6vlx240tff1156-1   & 1470      & 2979  & 145.455    & \begin{tabular}{@{}c@{}}DSP48E1s=6\end{tabular}  \\
		\hline
		zynq           & Vivado  & 2017.1  & xc7z020clg484-1    & 1523      & 3481  & 116.986    & \begin{tabular}{@{}c@{}}DSP48E1s=3\end{tabular}  \\
		\hline
		zynq\_ise      & ISE     & 14.7    & 7z020clg484-1      & 1470      & 2979  & 145.033    & \begin{tabular}{@{}c@{}}DSP48E1s=6\end{tabular}  \\
		\hline
	\end{tabular}
	\end{table}
\end{scriptsize}
