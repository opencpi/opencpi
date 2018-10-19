% see original utilization.inc and component datasheet appendix

% It is best to wrap this table in \begin{landscape} and \end{landscape} in its including doc
\begin{tiny}
    \begin{longtable}[l]{* {10}{|c}|}
    \captionsetup{justification=raggedright,singlelinecheck=false}
    \caption{Resource Utilization Table for worker: ad9361\_dac\_sub}\\
        \hline
        \rowcolor{blue}
            Configuration & OCPI Target & Tool    & Version & Device           & Registers       & LUTs       & \multicolumn{2}{c|}{Fmax (MHz) (Typ)} & Memory/Special           \\
        \rowcolor{blue}
                          &             &         &         &                  & (Typ)           & (Typ)      & \multicolumn{2}{c|}{}                 & Functions                \\
            \hline
        \rowcolor{blue}
                          &             &         &         &                  &                 &            & control plane & dev\_data\_clk    &                          \\
        \rowcolor{blue}
                          &             &         &         &                  &                 &            & clock         & clock             &                          \\
            \hline
            0             & zynq        & Vivado & 2017.1  & xc7z020clg484-1  & 94              & 89         & 477 \textsuperscript{\ref{abc}}             & 267 \textsuperscript{\ref{abc}}& \begin{tabular}{@{}l@{}}ODDR: 7 \\ BUFR: 1\end{tabular} \\
            \hline
            0             & virtex6     & ISE    & 14.7    & 6vlx240tff1156-1 & 101             & 113        & 689.655          & 632.511 & \begin{tabular}{@{}l@{}}ODDR: 7 \\ BUFR: 1\end{tabular} \\
            \hline
            1             & zynq        & Vivado & 2017.1  & xc7z020clg484-1  & 24              & 36         & 477 \textsuperscript{\ref{abc}}             & 297 \textsuperscript{\ref{abc}}& \begin{tabular}{@{}l@{}}ODDR: 7\end{tabular} \\
            \hline
            1             & virtex6     & ISE    & 14.7    & 6vlx240tff1156-1 & 29              & 44         & 689.655          & 496.416 & \begin{tabular}{@{}l@{}}ODDR: 7\end{tabular} \\
            \hline
        \end{longtable}
\end{tiny}
