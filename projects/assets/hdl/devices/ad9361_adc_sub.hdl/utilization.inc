\begin{landscape}
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Utilization Report for Worker "ad9361_adc_sub"
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%\textbf{Worker Build Configuration ``0'':}\\
%Table entries are a result of building the worker with the following parameter sets:\
%\begin{itemize}
%	\item \verb+DATA_RATE_CONFIG_p=DDR+
%	\item \verb+SINGLE_PORT_p=false+
%	\item \verb+HALF_DUPLEX_p=false+
%	\item \verb+LVDS_p=true+
%\end{itemize}
%\begin{scriptsize}
%	\begin{table}[H]
%	\captionsetup{justification=raggedright,singlelinecheck=false}
%	\caption{Worker Build Configuration ``0''}
%	\begin{tabular}{|c|c|c|c|c|c|c|c|}
%		\hline
%		\rowcolor{blue}
%		OpenCPI Target & Tool   & Version & Device          & Registers & LUTs & Fmax (MHz) & Memory/Special Functions\\
%		\hline
%		zynq           & Vivado & 2017.1  & xc7z020clg484-1 & 175       & 87   & 251.256\textsuperscript{\ref{abc}} & \begin{tabular}{@{}c@{}}BUFRs=2\\IDDRs=7\end{tabular}                     \\
%		\hline
%	\end{tabular}
%	\end{table}
%\end{scriptsize}
\textbf{Worker Build Configuration ``1'':}\\
Table entries are a result of building the worker with the following parameter sets:\
\begin{itemize}
	\item \verb+DATA_RATE_CONFIG_p=DDR+
	\item \verb+SINGLE_PORT_p=true+
	\item \verb+HALF_DUPLEX_p=false+
	\item \verb+LVDS_p=false+
\end{itemize}
\begin{scriptsize}
	\begin{table}[H]
	\captionsetup{justification=raggedright,singlelinecheck=false}
	\caption{Worker Build Configuration ``1''}
	\begin{tabular}{|c|c|c|c|c|c|c|c|c|}
		\hline
		\rowcolor{blue}
		OpenCPI Target & Tool   & Version & Device          & Registers & LUTs & \multicolumn{2}{c|}{Worker Fmax (typical)} & Memory/Special Functions\\
		\hline
		\rowcolor{blue}
	                       &        &         &                 &           &      & cp clock & dev\_data\_clk & \\
		\hline
		zynq           & Vivado & 2017.1  & xc7z020clg484-1 & 175       & 87   & 315.060\textsuperscript{\ref{abc}} & ?\textsuperscript{\ref{bad}} & \begin{tabular}{@{}c@{}}BUFR=1\\IDDRs=7\end{tabular}\\
		\hline
	\end{tabular}
	\end{table}
\end{scriptsize}
\textbf{Worker Build Configuration ``2'':}\\
Table entries are a result of building the worker with the following parameter sets:\
\begin{itemize}
	\item \verb+DATA_RATE_CONFIG_p=DDR+
	\item \verb+SINGLE_PORT_p=false+
	\item \verb+HALF_DUPLEX_p=false+
	\item \verb+LVDS_p=false+
\end{itemize}
\begin{scriptsize}
	\begin{table}[H]
	\captionsetup{justification=raggedright,singlelinecheck=false}
	\caption{Worker Build Configuration ``2''}
	\begin{tabular}{|c|c|c|c|c|c|c|c|c|}
		\hline
		\rowcolor{blue}
		OpenCPI Target & Tool   & Version & Device          & Registers & LUTs & \multicolumn{2}{c|}{Worker Fmax (typical)} & Memory/Special Functions\\
		\hline
		\rowcolor{blue}
	                       &        &         &                 &           &      & cp clock & dev\_data\_clk & \\
		\hline
		zynq           & Vivado & 2017.1  & xc7z020clg484-1 & 125       & 82   & 315.060\textsuperscript{\ref{abc}} & ?\textsuperscript{\ref{bad}} & \begin{tabular}{@{}c@{}}BUFRs=1\\IDDRs=13\end{tabular}\\
		\hline
	\end{tabular}
	\end{table}
\end{scriptsize}
\footnotetext[1]{\label{abc}These measurements were the result of a Vivado timing analysis which was different from the Vivado analysis performed by default for OpenCPI worker builds. For more info see Appendix \ref{appendix}}
\footnotetext[2]{\label{bad}Could not determine dev\_data\_clk Fmax using the analysis methods recommended in \ref{appendix}. The report\_timing command for clk2 reports ``No timing paths found.''}
\end{landscape}
