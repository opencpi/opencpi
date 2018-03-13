%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Utilization Report for Worker "complex_mixer"
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\textbf{Worker Build Configuration ``0'':}\\
Table entries are a result of building the worker with the following parameter sets:\
\begin{itemize}
	\item \verb+PEAK_MONITOR_p=true+
	\item \verb+NCO_DATA_WIDTH_p=12+
	\item \verb+CHIPSCOPE_p=false+
	\item \verb+INPUT_DATA_WIDTH_p=12+
	\item \verb+CORDIC_STAGES_p=16+
	\item \verb+ocpi_endian=little+
	\item \verb+ocpi_debug=false+
	\item \verb+VIVADO_ILA_p=false+
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
		stratix4       & Quartus & 15.1.0  & EP4SGX230KF40C2    & 1268      & 1,742 & N/A        & \begin{tabular}{@{}c@{}}DSP18x18s=8\end{tabular}\\
		\hline
		virtex6        & ISE     & 14.7    & 6vlx240tff1156-1   & 1215      & 2816  & 222.207    & \begin{tabular}{@{}c@{}}DSP48E1s=6\end{tabular} \\
		\hline
		zynq           & Vivado  & 2017.1  & xc7z020clg484-1    & 1214      & 2555  & 129.232    & \begin{tabular}{@{}c@{}}DSP48E1s=6\end{tabular} \\
		\hline
		zynq\_ise      & ISE     & 14.7    & 7z020clg484-1      & 1215      & 2814  & 214.658    & \begin{tabular}{@{}c@{}}DSP48E1s=6\end{tabular} \\
		\hline
	\end{tabular}
	\end{table}
\end{scriptsize}
%\textbf{Worker Build Configuration ``1'':}\\
%Table entries are a result of building the worker with the following parameter sets:\
%\begin{itemize}
%	\item \verb+PEAK_MONITOR_p=true+
%	\item \verb+NCO_DATA_WIDTH_p=16+
%	\item \verb+CHIPSCOPE_p=false+
%	\item \verb+INPUT_DATA_WIDTH_p=16+
%	\item \verb+CORDIC_STAGES_p=16+
%	\item \verb+ocpi_endian=little+
%	\item \verb+ocpi_debug=false+
%	\item \verb+VIVADO_ILA_p=false+
%\end{itemize}
%\begin{scriptsize}
%	\begin{table}[H]
%	\captionsetup{justification=raggedright,singlelinecheck=false}
%	\caption{Worker Build Configuration ``1''}
%	\begin{tabular}{|c|c|c|c|c|c|c|c|}
%		\hline
%		\rowcolor{blue}
%		OpenCPI Target & Tool    & Version & Device             & Registers & LUTs  & Fmax (MHz) & Memory/Special Functions                         \\
%		\hline
%		stratix4       & Quartus & 15.1.0  & EP4SGX230KF40C2    & 1468      & 1,926 & N/A        & \begin{tabular}{@{}c@{}}DSP18x18s=12\end{tabular}\\
%		\hline
%		virtex6        & ISE     & 14.7    & 6vlx240tff1156-1   & 1407      & 3319  & 165.044    & \begin{tabular}{@{}c@{}}DSP48E1s=8\end{tabular}  \\
%		\hline
%		zynq           & Vivado  & 2017.1  & xc7z020clg484-1    & 1452      & 3084  & 120.322    & \begin{tabular}{@{}c@{}}DSP48E1s=6\end{tabular}  \\
%		\hline
%		zynq\_ise      & ISE     & 14.7    & 7z020clg484-1      & 1407      & 3319  & 162.760    & \begin{tabular}{@{}c@{}}DSP48E1s=8\end{tabular}  \\
%		\hline
%	\end{tabular}
%	\end{table}
%\end{scriptsize}
%\textbf{Worker Build Configuration ``2'':}\\
%Table entries are a result of building the worker with the following parameter sets:\
%\begin{itemize}
%	\item \verb+PEAK_MONITOR_p=true+
%	\item \verb+NCO_DATA_WIDTH_p=12+
%	\item \verb+CHIPSCOPE_p=false+
%	\item \verb+INPUT_DATA_WIDTH_p=12+
%	\item \verb+CORDIC_STAGES_p=16+
%	\item \verb+ocpi_endian=little+
%	\item \verb+ocpi_debug=false+
%	\item \verb+VIVADO_ILA_p=true+
%\end{itemize}
%\begin{scriptsize}
%	\begin{table}[H]
%	\captionsetup{justification=raggedright,singlelinecheck=false}
%	\caption{Worker Build Configuration ``2''}
%	\begin{tabular}{|c|c|c|c|c|c|c|c|}
%		\hline
%		\rowcolor{blue}
%		OpenCPI Target & Tool    & Version & Device             & Registers & LUTs  & Fmax (MHz) & Memory/Special Functions                        \\
%		\hline
%		stratix4       & Quartus & 15.1.0  & EP4SGX230KF40C2    & 1268      & 1,742 & N/A        & \begin{tabular}{@{}c@{}}DSP18x18s=8\end{tabular}\\
%		\hline
%		virtex6        & ISE     & 14.7    & 6vlx240tff1156-1   & 1216      & 2820  & 222.207    & \begin{tabular}{@{}c@{}}DSP48E1s=6\end{tabular} \\
%		\hline
%		zynq           & Vivado  & 2017.1  & xc7z020clg484-1    & 1216      & 2566  & 129.232    & \begin{tabular}{@{}c@{}}DSP48E1s=6\end{tabular} \\
%		\hline
%		zynq\_ise      & ISE     & 14.7    & 7z020clg484-1      & 1216      & 2818  & 214.658    & \begin{tabular}{@{}c@{}}DSP48E1s=6\end{tabular} \\
%		\hline
%	\end{tabular}
%	\end{table}
%\end{scriptsize}
