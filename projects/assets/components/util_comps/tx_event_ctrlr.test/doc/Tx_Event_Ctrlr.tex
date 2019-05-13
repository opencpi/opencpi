\documentclass{article}
\iffalse
This file is protected by Copyright. Please refer to the COPYRIGHT file
distributed with this source distribution.

This file is part of OpenCPI <http://www.opencpi.org>

OpenCPI is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.
\fi

\author{} % Force author to be blank
%----------------------------------------------------------------------------------------
% Paper size, orientation and margins
%----------------------------------------------------------------------------------------
\usepackage{geometry}
\geometry{
	letterpaper,			% paper type
	portrait,				% text direction
	left=.75in,				% left margin
	top=.75in,				% top margin
	right=.75in,			% right margin
	bottom=.75in			% bottom margin
 }
%----------------------------------------------------------------------------------------
% Header/Footer
%----------------------------------------------------------------------------------------
\usepackage{fancyhdr} \pagestyle{fancy} % required for fancy headers
\renewcommand{\headrulewidth}{0.5pt}
\renewcommand{\footrulewidth}{0.5pt}
%----------------------------------------------------------------------------------------
% Appendix packages
%----------------------------------------------------------------------------------------
\usepackage[toc,page]{appendix}
%----------------------------------------------------------------------------------------
% Defined Commands & Renamed Commands
%----------------------------------------------------------------------------------------
\renewcommand{\contentsname}{Table of Contents}
\renewcommand{\listfigurename}{List of Figures}
\renewcommand{\listtablename}{List of Tables}
\newcommand{\todo}[1]{\textcolor{red}{TODO: #1}\PackageWarning{TODO:}{#1}} % To do notes
\newcommand{\code}[1]{\texttt{#1}} % For inline code snippet or command line
%----------------------------------------------------------------------------------------
% Various pacakges
%----------------------------------------------------------------------------------------
\usepackage{hyperref} % for linking urls and lists
\usepackage{graphicx} % for including pictures by file
\usepackage{listings} % for coding language styles
\usepackage{rotating} % for sideways table
\usepackage{pifont}   % for sideways table
\usepackage{pdflscape} % for landscape view
\usepackage{longtable}
%----------------------------------------------------------------------------------------
% Table packages
%----------------------------------------------------------------------------------------
\usepackage{tabularx} % c=center,l=left,r=right,X=fill
\usepackage{float}
\floatstyle{plaintop}
\usepackage[tableposition=top]{caption}
\newcolumntype{P}[1]{>{\centering\arraybackslash}p{#1}}
\newcolumntype{M}[1]{>{\centering\arraybackslash}m{#1}}
%----------------------------------------------------------------------------------------
% Block Diagram / FSM Drawings
%----------------------------------------------------------------------------------------
\usepackage{tikz}
\usetikzlibrary{shapes,arrows,fit,positioning}
\usetikzlibrary{automata} % used for the fsm
%----------------------------------------------------------------------------------------
% Colors Used
%----------------------------------------------------------------------------------------
\usepackage{colortbl}
\definecolor{blue}{rgb}{.7,.8,.9}
\definecolor{ceruleanblue}{rgb}{0.16, 0.32, 0.75}
\definecolor{drkgreen}{rgb}{0,0.6,0}
\definecolor{deepmagenta}{rgb}{0.8, 0.0, 0.8}
\definecolor{cyan}{rgb}{0.0,0.6,0.6}
\definecolor{maroon}{rgb}{0.5,0,0}
%----------------------------------------------------------------------------------------
% Update the docTitle and docVersion per document
%----------------------------------------------------------------------------------------
\def\docTitle{Component Data Sheet}
\def\docVersion{1.5}
%----------------------------------------------------------------------------------------
\date{OpenCPI Version \docVersion} % Force date to be blank and override date with version
\title{\docTitle}
\lhead{\small{\docTitle}}

\def\comp{tx\_event\_ctrlr}
\edef\ecomp{tx\_event\_ctrlr}
\def\Comp{TX Event Controller}
\graphicspath{ {figures/} }

\begin{document}

\section*{Summary - \Comp}
\begin{longtable}{|p{\dimexpr0.5\textwidth-2\tabcolsep\relax}
                  |p{\dimexpr0.5\textwidth-2\tabcolsep\relax}|}
	\hline
	\rowcolor{blue}
	                  &                                                    \\
	\hline
	Name              & \comp                                              \\
	\hline
	Worker Type       & Application                                        \\
	\hline
	Latest OpenCPI Version & v\docVersion ~ (released 4/2019) \\
	\hline
	Component Library & ocpi.assets.util\_comps                             \\
	\hline
	Workers           & \comp.rcc (version 2)                               \\
	\hline
	Tested Platforms  & CentOS7 \\
	\hline
\end{longtable}

\begin{center}
  \textit{\textbf{Revision History}}
\end{center}
\begin{longtable}{|p{\dimexpr0.15\textwidth-2\tabcolsep\relax}
                  |p{\dimexpr0.65\textwidth-2\tabcolsep\relax}
                  |p{\dimexpr0.2\textwidth-2\tabcolsep\relax}|}
  \hline
  \rowcolor{blue}
  \textbf{Revision} & \textbf{Description of Change} & \textbf{Date} \\
  \hline
  v1.4 & Initial release. & 9/2018 \\
  \hline
  v1.5 & Updated \comp.rcc to version 2 (OWD change only). & 4/2019 \\
  \hline
\end{longtable}

\section*{Worker Implementation Details}
When writing a value to the txen property that is different from its
       current value, there will be some finite delay before that write action's
       correspond ZLM is sent to the output port. Because the value read from
       the txen property is indicative of the \textit{current} state (i.e. the \textit{last}
       ZLM sent), the readback value of txen may differ from the value
       previously written until some amount of time has passed. If multiple
       property writes somehow occur before their corresponding ZLMs can be
       sent, they are queued for processing. The following table
       gives an example.

  \begin{center}
	\begin{tabular}{|c|c|}
		\hline
		\rowcolor{blue}
		Action & Time \\
		\hline
      write txen=false                & 0    sec \\
      txOff ZLM sent                  & 0.1  sec \\
      readback value of txen is false & 1    sec \\
      write txen=true                 & 2    sec \\
      readback value of txen is false & 2.1  sec \\
      txOn ZLM sent                   & 2.1  sec \\
      readback value of txen is true  & 2.2  sec \\
      readback value of txen is true  & 2.4  sec \\
      readback value of txen is true  & 2.6  sec \\
      readback value of txen is true  & 2.8  sec \\
		\hline
	\end{tabular}
  \end{center}

\section*{Block Diagrams}
\begin{verbatim}
<!--                                                                         -->
<!--         txen property                                                   -->
<!--              |                                                          -->
<!--              |                                                          -->
<!--       _______V_______                                                   -->
<!--      +       .       +                                                  -->
<!--      |       .       |                                                  -->
<!--      |       ........|-> out port sends ZLMs w/ txOn or txOff opcode    -->
<!--      |               |                                                  -->
<!--      +_______________+                                                  -->
<!--                                                                         -->
\end{verbatim}

\begin{landscape}
	\section*{Component Spec Properties}
	\begin{scriptsize}
		\begin{tabular}{|p{3cm}|p{1.5cm}|c|c|c|p{1.5cm}|p{1cm}|p{7.15cm}|}
			\hline
			\rowcolor{blue}
			Name               & Type   & SequenceLength & ArrayDimensions & Accessibility      & Valid Range & Default & Usage                                                                         \\
			\hline
			\verb+txen+        & Bool   & -              & -               & Writable, Volatile & Standard    & -       & TX enable.                                                                    \\
			\hline

		\end{tabular}
	\end{scriptsize}

	\section*{Worker Properties}
	\subsection*{\comp.rcc}
	\begin{scriptsize}
		\begin{tabular}{|p{1.5cm}|p{2.5cm}|p{1cm}|c|c|c|p{2cm}|p{1cm}|p{4cm}|}
			\hline
			\rowcolor{blue}
			Type     & Name                      & Type  & SequenceLength & ArrayDimensions & Accessibility       & Valid Range & Default & Usage                                      \\
			\hline
			Property & \verb+txen+               & Bool  & -              & -               & Writable, WriteSync, ReadSync & Standard    & -       & TX enable.                                 \\
			\hline
		\end{tabular}
	\end{scriptsize}

	\section*{Component Ports}
	\begin{scriptsize}
		\begin{tabular}{|M{2cm}|M{1.5cm}|M{4cm}|c|c|M{11.5cm}|}
			\hline
			\rowcolor{blue}
			Name & Producer & Protocol           & Optional & Advanced & Usage                  \\
			\hline
			out  & true     & tx\_event-prot     & false    & -        & \\
			\hline
		\end{tabular}
	\end{scriptsize}

\end{landscape}

\end{document}
