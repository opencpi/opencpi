% see original utilization.inc and component datasheet appendix

% It is best to wrap this table in \begin{landscape} and \end{landscape} in its including doc
\begin{tiny}
    \begin{longtable}[l]{* {10}{|c}|}
    \captionsetup{justification=raggedright,singlelinecheck=false}
    \caption{Resource Utilization Table for worker: ad9361\_adc}\\
        \hline
        \rowcolor{blue}
            Configuration & OCPI Target & Tool    & Version & Device           & Registers       & LUTs       & \multicolumn{2}{c|}{Fmax (MHz) (Typ)} & Memory/Special           \\
        \rowcolor{blue}
                          &             &         &         &                  & (Typ)           & (Typ)      & \multicolumn{2}{c|}{}               & Functions                \\
            \hline
        \rowcolor{blue}
                          &             &         &         &                  &                 &            & control plane & dev\_adc.adc\_clk &                          \\
        \rowcolor{blue}
                          &             &         &         &                  &                 &            & clock         & clock             &                          \\
            \hline
            0             & zynq        & Vivado  & 2017.1  & xc7z020clg484-1  & 188             & 133        & 202 \textsuperscript{\ref{abc}} & 339 \textsuperscript{\ref{abc}} & \begin{tabular}{@{}l@{}}RAMB18: 1\end{tabular}\\
            \hline
            0             & stratix4    & Quartus & 17.1.0  & EP4SGX230KF40C2  & 187             & 185        & N/A              & N/A & N/A \\
            \hline
            0             & virtex6     & ISE     & 14.7    & 6vlx240tff1156-1 & 208             & 258        & 335.373          & 369 & \begin{tabular}{@{}l@{}}RAM64M: 8\end{tabular} \\
            \hline
        \end{longtable}
\end{tiny}
