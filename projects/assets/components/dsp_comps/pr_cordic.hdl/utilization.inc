%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Utilization Report for Worker "pr_cordic"
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\textbf{Worker Build Configuration ``0'':}\\
Table entries are a result of building the worker with the following parameter sets:\
\begin{itemize}
	\item \verb+ocpi_endian=little+
	\item \verb+ocpi_debug=false+
	\item \verb+STAGES=18+
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
		OpenCPI Target & Tool    & Version & Device             & Registers & LUTs  & Fmax (MHz) & Memory/Special Functions                        \\
		\hline
		stratix4       & Quartus & 15.1.0  & EP4SGX230KF40C2    & 1434      & 1,886 & N/A        & \begin{tabular}{@{}c@{}}DSP18x18s=8\end{tabular}\\
		\hline
		virtex6        & ISE     & 14.7    & 6vlx240tff1156-1   & 1372      & 3190  & 165.044    & \begin{tabular}{@{}c@{}}DSP48E1s=4\end{tabular} \\
		\hline
		zynq           & Vivado  & 2017.1  & xc7z020clg484-1    & 1417      & 3193  & 120.322    & \begin{tabular}{@{}c@{}}DSP48E1s=2\end{tabular} \\
		\hline
		zynq\_ise      & ISE     & 14.7    & 7z020clg484-1      & 1372      & 3190  & 162.760    & \begin{tabular}{@{}c@{}}DSP48E1s=4\end{tabular} \\
		\hline
	\end{tabular}
	\end{table}
\end{scriptsize}
