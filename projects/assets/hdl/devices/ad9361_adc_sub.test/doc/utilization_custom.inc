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
            0             & zynq        & Vivado & 2017.1  & xc7z020clg484-1  & 175             & 87         & 315 \textsuperscript{\ref{abc}} & 418 \textsuperscript{\ref{abc}} & \begin{tabular}{@{}l@{}}BUFR: 1\end{tabular} \\
            \hline
            0             & virtex6     & ISE    & 14.7    & 6vlx240tff1156-1 & 155             & 163        & 437.445          & & \begin{tabular}{@{}l@{}}BUFR: 1\end{tabular} \\
            \hline
            1             & zynq        & Vivado & 2017.1  & xc7z020clg484-1  & 175             & 87         & 315 \textsuperscript{\ref{abc}} & 442 \textsuperscript{\ref{abc}} & N/A \\
            \hline
            1             & virtex6     & ISE    & 14.7    & 6vlx240tff1156-1 & 155             & 163        & 437.445          & & N/A \\
            \hline
        \end{longtable}
\end{tiny}
