%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Utilization Report for Worker "fifo"
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\textbf{Worker Build Configuration ``0'':}\\
Table entries are a result of building the worker with the following parameter sets:\
\begin{itemize}
	\item \verb+ocpi_endian=little+
	\item \verb+ocpi_debug=false+
	\item \verb+ZLM_WHEN_ONESHOT_DONE_p=false+
	\item \verb+FIFO_DEPTH_p=8192+
\end{itemize}
\begin{scriptsize}
	\begin{table}[H]
	\captionsetup{justification=raggedright,singlelinecheck=false}
	\caption{Worker Build Configuration ``0''}
	\begin{tabular}{|c|c|c|c|c|c|c|c|}
		\hline
		\rowcolor{blue}
		OpenCPI Target & Tool    & Version & Device          & Registers & LUTs & Fmax (MHz) & Memory/Special Functions                         \\
		\hline
		stratix4       & Quartus & 15.1.0  & N/A             & 291       & 350  & N/A        & N/A                                              \\
		\hline
		virtex6        & ISE     & 14.7    & 6vcx75tff484-2  & 191       & 8044 & 255.582    & \begin{tabular}{@{}c@{}}RAM64Ms=1536\end{tabular}\\
		\hline
		zynq           & Vivado  & 2017.1  & xc7z020clg400-3 & 373       & 7948 & 249.004    & N/A                                              \\
		\hline
		zynq\_ise      & ISE     & 14.7    & 7z010clg400-3   & 191       & 8028 & 307.390    & \begin{tabular}{@{}c@{}}RAM64Ms=1536\end{tabular}\\
		\hline
	\end{tabular}
	\end{table}
\end{scriptsize}
\textbf{Worker Build Configuration ``1'':}\\
Table entries are a result of building the worker with the following parameter sets:\
\begin{itemize}
	\item \verb+ocpi_endian=little+
	\item \verb+ocpi_debug=false+
	\item \verb+ZLM_WHEN_ONESHOT_DONE_p=true+
	\item \verb+FIFO_DEPTH_p=8192+
\end{itemize}
\begin{scriptsize}
	\begin{table}[H]
	\captionsetup{justification=raggedright,singlelinecheck=false}
	\caption{Worker Build Configuration ``1''}
	\begin{tabular}{|c|c|c|c|c|c|c|c|}
		\hline
		\rowcolor{blue}
		OpenCPI Target & Tool    & Version & Device          & Registers & LUTs & Fmax (MHz) & Memory/Special Functions                         \\
		\hline
		stratix4       & Quartus & 15.1.0  & N/A             & 291       & 342  & N/A        & N/A                                              \\
		\hline
		virtex6        & ISE     & 14.7    & 6vcx75tff484-2  & 191       & 8034 & 253.615    & \begin{tabular}{@{}c@{}}RAM64Ms=1536\end{tabular}\\
		\hline
		zynq           & Vivado  & 2017.1  & xc7z020clg400-3 & 372       & 7956 & 249.004    & N/A                                              \\
		\hline
		zynq\_ise      & ISE     & 14.7    & 7z010clg400-3   & 191       & 8031 & 307.390    & \begin{tabular}{@{}c@{}}RAM64Ms=1536\end{tabular}\\
		\hline
	\end{tabular}
	\end{table}
\end{scriptsize}
