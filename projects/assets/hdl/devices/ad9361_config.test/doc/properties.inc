\begin{longtable}{|p{\dimexpr0.25\linewidth-2\tabcolsep\relax}
                  |p{\dimexpr0.05\linewidth-2\tabcolsep\relax}
                  |p{\dimexpr0.1\linewidth-2\tabcolsep\relax}
                  |p{\dimexpr0.1\linewidth-2\tabcolsep\relax}
                  |p{\dimexpr0.1\linewidth-2\tabcolsep\relax}
                  |p{\dimexpr0.1\linewidth-2\tabcolsep\relax}
                  |p{\dimexpr0.1\linewidth-2\tabcolsep\relax}
                  |p{\dimexpr0.2\linewidth-2\tabcolsep\relax}|}
\hline
\rowcolor{blue}
Name                 & Type   & SequenceLength & ArrayDimensions & Accessibility       & Valid Range & Default & Description
\\
\hline
other\_present & bool  & - & - & Readable & -  &- & -
\\
\hline
force\_reset & bool  & - & - & Readable, Writeable & -  &0 & Forces reset pin low (active low). Reset pin is otherwise the same level as the OpenCPI control plane reset signal.
\\
\hline
qadc0\_is\_present & bool  & - & - & Volatile & -  &- & Inidicates whether or not bitstream was built with qadc0, which supports first RX channel
\\
\hline
qadc1\_is\_present & bool  & - & - & Volatile & -  &- & Inidicates whether or not bitstream was built with qadc1, which supports second RX channel
\\
\hline
qdac0\_is\_present & bool  & - & - & Volatile & -  &- & Inidicates whether or not bitstream was built with qdac0, which supports first TX channel
\\
\hline
qdac1\_is\_present & bool  & - & - & Volatile & -  &- & Inidicates whether or not bitstream was built with qdac1, which supports second TX channel
\\
\hline
rx\_frame\_usage & enum  & - & - & Volatile & -  &- & enable : Register 0x010 bit D3 is 0, meaning Rx frame goes high coincident with the first valid receive sample. It stays high., toggle: Register 0x010 bit D3 is 1, meaning the Rx frame signal toggles with a duty cycle of 50%.
\\
\hline
data\_bus\_index\_direction & enum  & - & - & Volatile & -  &- & normal : Register 0x010 bit D1 is 0, meaning each RX sample's bit index direction is normal, i.e. [11:0], reverse: Register 0x010 bit D1 is 1, meaning each RX sample's bit direction is inverted, i.e. [0:11].
\\
\hline
data\_clk\_is\_inverted & bool  & - & - & Volatile & -  &- & false : Register 0x010 bit D0 is 0, meaning that the DATA\_CLK follows the DATA\_CLK\_P signal in the UG570 timing diagrams, true : Register 0x010 bit D0 is 1, meaning that the DATA\_CLK follows the DATA\_CLK\_N signal in the UG570 timing diagrams
\\
\hline
rx\_frame\_is\_inverted & bool  & - & - & Volatile & -  &- & false : Register 0x011 bit D2 is 0, meaning that the RX\_FRAME follows the RX\_FRAME\_P signal in the UG570 timing diagrams, true : Register 0x011 bit D2 is 1, meaning that the RX\_FRAME follows the RX\_FRAME\_N signal in the UG570 timing diagrams
\\
\hline
LVDS & bool  & - & - & Volatile & -  &- & Value is true if bitstream was built to use LVDS mode for Data/clock/frame signals, and false if CMOS mode was used.
\\
\hline
single\_port & bool  & - & - & Volatile & -  &- & Value is true if bitstream was built to use single port, and false if dual ports.
\\
\hline
swap\_ports & bool  & - & - & Volatile & -  &- & Value is true if bitstream was built to swap Port 0 and Port 1, and false if there was no swap.
\\
\hline
half\_duplex & bool  & - & - & Volatile & -  &- & Value is true if bitstream was built to use half duplex mode, and false if full duplex mode.
\\
\hline
data\_rate\_config & enum  & - & - & Volatile & -  &- & Value indicates which data rate mode (SDR/DDR) the bitstream was built to use.
\\
\hline
data\_configs\_are\_valid & bool  & - & - & Volatile & -  &- & Value is false if bitstream was built using erroneous combination of LVDS/single port/half duplex/data rate config modes (takes into account build configurations for both ad9361\_adc\_sub and ad9361\_dac\_sub workers).
\\
\hline
config\_is\_two\_r & bool  & - & - & Readable, Writeable & -  &- & Used to tell the ad9361\_adc\_sub and ad9361\_dac\_sub workers what data paths are enabled. Note that, just because a qadc or qdac worker is present in the bitstream, that doesn't mean it is enabled.
\\
\hline
config\_is\_two\_t & bool  & - & - & Readable, Writeable & -  &- & Used to tell the ad9361\_dac\_sub worker what data paths are enabled. Note that, just because a qdac worker is present in the bitstream, that doesn't mean it is enabled.
\\
\hline
force\_two\_r\_two\_t\_timing & bool  & - & - & Readable, Writeable & -  &- & Used to force the ad9361\_dac\_sub worker to use the 2R2T timing diagram regardless of what TX channels are enabled. This property is expected to correspond to the D2 bit of the Parallel Port Configuration 1 register at SPI address 0x010.
\\
\hline
Half\_Duplex\_Mode & bool  & - & - & Writeable & -  &false & -
\\
\hline
ENSM\_Pin\_Control & bool  & - & - & Writeable & -  &true & Intended to match AD9361 register 0x014 bit D4.
\\
\hline
Level\_Mode & bool  & - & - & Writeable & -  &false & Intended to match AD9361 register 0x014 bit D3.
\\
\hline
FDD\_External\_Control\_Enable & bool  & - & - & Writeable & -  &false & Intended to match AD9361 register 0x014 bit D7.
\\
\hline
ENABLE\_force\_set & bool  & - & - & Writeable & -  &false & Forces set of AD9361 ENABLE pin
\\
\hline
TXNRX\_force\_set & bool  & - & - & Writeable & -  &false & Forces set of AD9361 TXNRX pin
\\
\hline
general\_spi\_conf & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d0\_0x0000   Table 1:  CHIP LEVEL SETUP: SPI Configuration
\\
\hline
general\_multichip\_sync\_and\_tx\_mon\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d1\_0x0001   Table 1:  CHIP LEVEL SETUP: Multichip Sync and Tx Mon Control
\\
\hline
general\_tx\_enable\_filter\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d2\_0x0002   Table 1:  CHIP LEVEL SETUP: Tx Enable \& Filter Control
\\
\hline
general\_rx\_enable\_filter\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d3\_0x0003   Table 1:  CHIP LEVEL SETUP: Rx Enable \& Filter Control
\\
\hline
general\_input\_select & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d4\_0x0004   Table 1:  CHIP LEVEL SETUP: Input Select
\\
\hline
general\_rfpll\_dividers & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d5\_0x0005   Table 1:  CHIP LEVEL SETUP: RFPLL Dividers
\\
\hline
general\_rx\_clock\_data\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d6\_0x0006   Table 1:  CHIP LEVEL SETUP: Rx Clock and Data  Delay
\\
\hline
general\_tx\_clock\_data\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d7\_0x0007   Table 1:  CHIP LEVEL SETUP: Tx Clock and Data Delay
\\
\hline
ocpi\_pad\_008 & uchar  & - & - &  & -  &- & -
\\
\hline
clock\_enable & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d9\_0x0009   Table 8:  CLOCK CONTROL: Clock Enable
\\
\hline
clock\_bbpll & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d10\_0x000a  Table 8:  CLOCK CONTROL: BBPLL
\\
\hline
temp\_offset & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d11\_0x000b  Table 10: TEMPERATURE SENSOR: Offset
\\
\hline
temp\_start\_reading & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d12\_0x000c  Table 10: TEMPERATURE SENSOR: Start Temp Reading
\\
\hline
temp\_sense2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d13\_0x000d  Table 10: TEMPERATURE SENSOR: Temp Sense2
\\
\hline
temp\_temperature & uchar  & - & - & Volatile & -  &- & reg\_addr\_d14\_0x000e  Table 10: TEMPERATURE SENSOR: Temperature
\\
\hline
temp\_sensor\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d15\_0x000f  Table 10: TEMPERATURE SENSOR: Temp Sensor Config
\\
\hline
parallel\_port\_conf\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d16\_0x0010  Table 11: PARALLEL PORT CONFIGURATION: Parallel Port Configuration 1
\\
\hline
parallel\_port\_conf\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d17\_0x0011  Table 11: PARALLEL PORT CONFIGURATION: Parallel Port Configuration 2
\\
\hline
parallel\_port\_conf\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d18\_0x0012  Table 11: PARALLEL PORT CONFIGURATION: Parallel Port Configuration 3
\\
\hline
ensm\_mode & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d19\_0x0013  Table 12: ENABLE STATE MACHINE: ENSM Mode
\\
\hline
ensm\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d20\_0x0014  Table 12: ENABLE STATE MACHINE: ENSM Config 1
\\
\hline
ensm\_config\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d21\_0x0015  Table 12: ENABLE STATE MACHINE: ENSM Config 2
\\
\hline
ensm\_calibration\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d22\_0x0016  Table 12: ENABLE STATE MACHINE: Calibration Control
\\
\hline
ensm\_state & uchar  & - & - & Volatile & -  &- & reg\_addr\_d23\_0x0017  Table 12: ENABLE STATE MACHINE: State
\\
\hline
auxdac\_1\_word & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d24\_0x0018  Table 15: AUXDAC: AuxDAC 1 Word
\\
\hline
auxdac\_2\_word & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d25\_0x0019  Table 15: AUXDAC: AuxDAC 2 Word
\\
\hline
auxdac\_1\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d26\_0x001a  Table 15: AUXDAC: AuxDAC 1 Config
\\
\hline
auxdac\_2\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d27\_0x001b  Table 15: AUXDAC: AuxDAC 2 Config
\\
\hline
auxadc\_clock\_divider & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d28\_0x001c  Table 17: AUXILARYADC: AuxADC Clock Divider
\\
\hline
auxadc\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d29\_0x001d  Table 17: AUXILARYADC: Aux ADC Config
\\
\hline
auxadc\_word\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d30\_0x001e  Table 17: AUXILARYADC: AuxADC Word MSB
\\
\hline
auxadc\_world\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d31\_0x001f  Table 17: AUXILARYADC: AuxADC Word LSB
\\
\hline
misc\_auto\_gpo & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d32\_0x0020  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: Auto GPO
\\
\hline
misc\_agc\_gain\_lock\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d33\_0x0021  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AGC Gain Lock Delay
\\
\hline
misc\_agc\_attack\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d34\_0x0022  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AGC Attack Delay
\\
\hline
misc\_auxdac\_enable\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d35\_0x0023  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AuxDAC Enable Control
\\
\hline
misc\_rx\_load\_synth\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d36\_0x0024  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: RX Load Synth Delay
\\
\hline
misc\_tx\_load\_synth\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d37\_0x0025  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: TX Load Synth Delay
\\
\hline
misc\_external\_lna\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d38\_0x0026  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: External LNA control
\\
\hline
misc\_gpo\_force\_and\_init & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d39\_0x0027  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO Force and Init
\\
\hline
misc\_gpo0\_rx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d40\_0x0028  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO0 Rx delay
\\
\hline
misc\_gpo1\_rx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d41\_0x0029  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO1 Rx delay
\\
\hline
misc\_gpo2\_rx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d42\_0x002a  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO2 Rx delay
\\
\hline
misc\_gpo3\_rx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d43\_0x002b  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO3 Rx delay
\\
\hline
misc\_gpo0\_tx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d44\_0x002c  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO0 Tx Delay
\\
\hline
misc\_gpo1\_tx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d45\_0x002d  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO1 Tx Delay
\\
\hline
misc\_gpo2\_tx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d46\_0x002e  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO2 Tx Delay
\\
\hline
misc\_gpo3\_tx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d47\_0x002f  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO3 Tx Delay
\\
\hline
misc\_auxdac1\_rx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d48\_0x0030  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AuxDAC1 Rx Delay
\\
\hline
misc\_auxdac1\_tx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d49\_0x0031  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AuxDAC1 Tx Delay
\\
\hline
misc\_auxdac2\_rx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d50\_0x0032  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AuxDAC2 Rx Delay
\\
\hline
misc\_auxdac2\_tx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d51\_0x0033  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AuxDAC2 Tx Delay
\\
\hline
ocpi\_pad\_034 & uchar  & - & - &  & -  &- & -
\\
\hline
ctrl\_output\_pointer & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d53\_0x0035  Table 19: CONTROL OUTPUT: Control Output Pointer
\\
\hline
ctrl\_output\_enable & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d54\_0x0036  Table 19: CONTROL OUTPUT: Control Output Enable
\\
\hline
product\_id & uchar  & - & - & Volatile & -  &- & reg\_addr\_d55\_0x0037  Table 20: PRODUCT ID: Product ID
\\
\hline
ocpi\_pad\_038 & uchar  & - & - &  & -  &- & -
\\
\hline
reference\_clock\_cycles & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d58\_0x003a  Table 22: REFERENCE CLOCK CYCLES: Reference Clock Cycles
\\
\hline
digital\_io\_digital\_io\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d59\_0x003b  Table 23: DIGITAL IO CONTROL: Digital I/O Control
\\
\hline
digital\_io\_lvds\_bias\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d60\_0x003c  Table 23: DIGITAL IO CONTROL: LVDS Bias control
\\
\hline
digital\_io\_lvds\_invert\_ctrl1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d61\_0x003d  Table 23: DIGITAL IO CONTROL: LVDS Invert control1
\\
\hline
digital\_io\_lvds\_invert\_ctrl2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d62\_0x003e  Table 23: DIGITAL IO CONTROL: LVDS Invert control2
\\
\hline
bbpll\_ctrl\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d63\_0x003f  Table 25: BBPLL CONTROL: BPLL Control 1
\\
\hline
bbpll\_mustbe0x00 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d64\_0x0040  Table 25: BBPLL CONTROL: Must be 0
\\
\hline
bbpll\_fract\_bb\_freq\_word\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d65\_0x0041  Table 25: BBPLL CONTROL: Fractional BB Freq Word 1
\\
\hline
bbpll\_fract\_bb\_freq\_word\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d66\_0x0042  Table 25: BBPLL CONTROL: Fractional BB Freq Word 2
\\
\hline
bbpll\_fract\_bb\_freq\_word\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d67\_0x0043  Table 25: BBPLL CONTROL: Fractional BB Freq Word 3
\\
\hline
bbpll\_integer\_bb\_freq\_word & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d68\_0x0044  Table 25: BBPLL CONTROL: Integer BB Freq Word
\\
\hline
bbpll\_ref\_clock\_scaler & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d69\_0x0045  Table 25: BBPLL CONTROL: Ref Clock Scaler
\\
\hline
bbpll\_cp\_current & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d70\_0x0046  Table 25: BBPLL CONTROL: CP Current
\\
\hline
bbpll\_msc\_scale & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d71\_0x0047  Table 25: BBPLL CONTROL: MSC Scale
\\
\hline
bbpll\_loop\_filter\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d72\_0x0048  Table 25: BBPLL CONTROL: Loop Filter 1
\\
\hline
bbpll\_loop\_filter\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d73\_0x0049  Table 25: BBPLL CONTROL: Loop Filter 2
\\
\hline
bbpll\_loop\_filter\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d74\_0x004a  Table 25: BBPLL CONTROL: Loop Filter 3
\\
\hline
bbpll\_vco\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d75\_0x004b  Table 25: BBPLL CONTROL: VCO Control
\\
\hline
bbpll\_mustbe0x86 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d76\_0x004c  Table 25: BBPLL CONTROL: Must be\_0x86
\\
\hline
bpll\_control\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d77\_0x004d  Table 25: BBPLL CONTROL: BPLL Control 2
\\
\hline
bpll\_control\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d78\_0x004e  Table 25: BBPLL CONTROL: BPLL Control 3
\\
\hline
ocpi\_pad\_04f & uchar  & - & - &  & -  &- & -
\\
\hline
power\_down\_override\_rx\_synth & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d80\_0x0050  Table 26: POWER DOWN OVERRIDE: Rx Synth Power Down Override
\\
\hline
power\_down\_override\_tx\_synth & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d81\_0x0051  Table 26: POWER DOWN OVERRIDE: TX Synth Power Down Override
\\
\hline
power\_down\_override\_rx\_control\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d82\_0x0052  Table 26: POWER DOWN OVERRIDE: Control 0
\\
\hline
power\_down\_override\_mustbe0x00 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d83\_0x0053  Table 26: POWER DOWN OVERRIDE: Must be 0
\\
\hline
power\_down\_override\_rx1\_adc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d84\_0x0054  Table 26: POWER DOWN OVERRIDE: Rx1 ADC Power Down Override
\\
\hline
power\_down\_override\_rx2\_adc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d85\_0x0055  Table 26: POWER DOWN OVERRIDE: Rx2 ADC Power Down Override
\\
\hline
power\_down\_override\_tx\_analog & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d86\_0x0056  Table 26: POWER DOWN OVERRIDE: Tx Analog Power Down Override 1
\\
\hline
power\_down\_override\_analog & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d87\_0x0057  Table 26: POWER DOWN OVERRIDE: Analog Power Down Override
\\
\hline
power\_down\_override\_misc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d88\_0x0058  Table 26: POWER DOWN OVERRIDE: Misc Power Down Override
\\
\hline
ocpi\_pad\_059 & uchar  & - & - &  & -  &- & -
\\
\hline
overflow\_ch\_1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d94\_0x005e  Table 27: OVERFLOW: CH 1 Overflow
\\
\hline
overflow\_ch\_2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d95\_0x005f  Table 27: OVERFLOW: CH 2 Overflow
\\
\hline
tx\_filter\_coef\_addr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d96\_0x0060  Table 28: Tx PROGRAMMABLE FIR FILTER: TX Filter Coefficient Address
\\
\hline
tx\_filter\_coef\_write\_data\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d97\_0x0061  Table 28: Tx PROGRAMMABLE FIR FILTER: TX Filter Coefficient Write Data 1
\\
\hline
tx\_filter\_coef\_write\_data\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d98\_0x0062  Table 28: Tx PROGRAMMABLE FIR FILTER: TX Filter Coefficient Write Data 2
\\
\hline
tx\_filter\_coef\_read\_data\_1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d99\_0x0063  Table 28: Tx PROGRAMMABLE FIR FILTER: TX Filter Coefficient Read Data 1
\\
\hline
tx\_filter\_coef\_read\_data\_2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d100\_0x0064 Table 28: Tx PROGRAMMABLE FIR FILTER: TX Filter Coefficient Read Data 2
\\
\hline
tx\_filter\_conf & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d101\_0x0065 Table 28: Tx PROGRAMMABLE FIR FILTER: TX Filter Configuration
\\
\hline
ocpi\_pad\_066 & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_mon\_low\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d103\_0x0067 Table 29: Tx MONITOR: Tx Mon Low Gain
\\
\hline
tx\_mon\_high\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d104\_0x0068 Table 29: Tx MONITOR: Tx Mon High Gain
\\
\hline
tx\_mon\_delay\_counter & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d105\_0x0069 Table 29: Tx MONITOR: Tx Mon Delay Counter
\\
\hline
tx\_mon\_level\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d106\_0x006a Table 29: Tx MONITOR: Tx Level Threshold
\\
\hline
tx\_mon\_rssi1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d107\_0x006b Table 29: Tx MONITOR: TX RSSI1
\\
\hline
tx\_mon\_rssi2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d108\_0x006c Table 29: Tx MONITOR: TX RSSI2
\\
\hline
tx\_mon\_rssi\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d109\_0x006d Table 29: Tx MONITOR: TX RSSI LSB
\\
\hline
tx\_mon\_tpm\_mode\_enable & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d110\_0x006e Table 29: Tx MONITOR: TPM Mode Enable
\\
\hline
tx\_mon\_temp\_gain\_coef & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d111\_0x006f Table 29: Tx MONITOR: Temp Gain Coefficient
\\
\hline
tx\_mon\_1\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d112\_0x0070 Table 29: Tx MONITOR: Tx Mon 1 Config
\\
\hline
tx\_mon\_2\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d113\_0x0071 Table 29: Tx MONITOR: Tx Mon 2 Config
\\
\hline
ocpi\_pad\_072 & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_pwr\_atten\_tx1\_atten\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d115\_0x0073 Table 31: Tx POWER CONTROL AND ATTENUATION: Tx1 Atten 0
\\
\hline
tx\_pwr\_atten\_tx1\_atten\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d116\_0x0074 Table 31: Tx POWER CONTROL AND ATTENUATION: Tx1 Atten 1
\\
\hline
tx\_pwr\_atten\_tx2\_atten\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d117\_0x0075 Table 31: Tx POWER CONTROL AND ATTENUATION: Tx2 Atten 0
\\
\hline
tx\_pwr\_atten\_tx2\_atten\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d118\_0x0076 Table 31: Tx POWER CONTROL AND ATTENUATION: Tx2 Atten 1
\\
\hline
tx\_pwr\_atten\_tx\_atten\_offset & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d119\_0x0077 Table 31: Tx POWER CONTROL AND ATTENUATION: Tx Atten Offset
\\
\hline
tx\_pwr\_atten\_tx\_atten\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d120\_0x0078 Table 31: Tx POWER CONTROL AND ATTENUATION: Tx Atten Threshold
\\
\hline
tx\_pwr\_atten\_set\_tx1\_tx2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d121\_0x0079 Table 31: Tx POWER CONTROL AND ATTENUATION: Set Tx1/Tx2
\\
\hline
ocpi\_pad\_07a & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_pwr\_atten\_immediate\_update & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d124\_0x007c Table 31: Tx POWER CONTROL AND ATTENUATION: Immediate Update
\\
\hline
ocpi\_pad\_07d & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_pgo\_phase\_corr\_tx1\_out1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d142\_0x008e Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 1 Phase Corr
\\
\hline
tx\_pgo\_gain\_corr\_tx1\_out1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d143\_0x008f Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 1 Gain Corr
\\
\hline
tx\_pgo\_phase\_corr\_tx2\_out1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d144\_0x0090 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 1 Phase Corr
\\
\hline
tx\_pgo\_gain\_corr\_tx2\_out1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d145\_0x0091 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 1 Gain Corr
\\
\hline
tx\_pgo\_offset\_corr\_tx1\_out1\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d146\_0x0092 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 1 Offset I
\\
\hline
tx\_pgo\_offset\_corr\_tx1\_out1\_q & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d147\_0x0093 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 1 Offset Q
\\
\hline
tx\_pgo\_offset\_corr\_tx2\_out1\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d148\_0x0094 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 1 Offset I
\\
\hline
tx\_pgo\_offset\_corr\_tx2\_out1\_q & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d149\_0x0095 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 1 Offset Q
\\
\hline
tx\_pgo\_phase\_corr\_tx1\_out2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d150\_0x0096 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 2 Phase Corr
\\
\hline
tx\_pgo\_gain\_corr\_tx1\_out2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d151\_0x0097 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 2 Gain Corr
\\
\hline
tx\_pgo\_phase\_corr\_tx2\_out2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d152\_0x0098 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 2 Phase Corr
\\
\hline
tx\_pgo\_gain\_corr\_tx2\_out2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d153\_0x0099 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 2 Gain Corr
\\
\hline
tx\_pgo\_offset\_corr\_tx1\_out2\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d154\_0x009a Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 2 Offset I
\\
\hline
tx\_pgo\_offset\_corr\_tx1\_out2\_q & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d155\_0x009b Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 2 Offset Q
\\
\hline
tx\_pgo\_offset\_corr\_tx2\_out2\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d156\_0x009c Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 2 Offset I
\\
\hline
tx\_pgo\_offset\_corr\_tx2\_out2\_q & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d157\_0x009d Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 2 Offset Q
\\
\hline
ocpi\_pad\_09e & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_quad\_cal\_pgo\_force\_bits & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d159\_0x009f Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Force Bits
\\
\hline
tx\_quad\_cal\_nco\_freq\_phase\_offset & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d160\_0x00a0 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Quad Cal NCO Freq \& Phase Offset
\\
\hline
tx\_quad\_cal\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d161\_0x00a1 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Quad Cal Control
\\
\hline
tx\_quad\_cal\_kexp\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d162\_0x00a2 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Kexp 1
\\
\hline
tx\_quad\_cal\_kexp\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d163\_0x00a3 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Kexp 2
\\
\hline
tx\_quad\_cal\_settle\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d164\_0x00a4 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: QUAD Settle count
\\
\hline
tx\_quad\_cal\_mag\_ftest\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d165\_0x00a5 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Mag. Ftest Thresh
\\
\hline
tx\_quad\_cal\_mag\_ftest\_thresh\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d166\_0x00a6 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Mag. Ftest Thresh 2
\\
\hline
tx\_quad\_cal\_status\_tx1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d167\_0x00a7 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Quad cal status Tx1
\\
\hline
tx\_quad\_cal\_status\_tx2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d168\_0x00a8 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Quad cal status Tx2
\\
\hline
tx\_quad\_cal\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d169\_0x00a9 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Quad cal Count
\\
\hline
tx\_quad\_cal\_full\_lmt\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d170\_0x00aa Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Tx Quad Full/LMT Gain
\\
\hline
tx\_quad\_cal\_squarer\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d171\_0x00ab Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Squarer Config
\\
\hline
tx\_quad\_cal\_atten & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d172\_0x00ac Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: TX Quad Cal Atten
\\
\hline
tx\_quad\_cal\_thresh\_accum & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d173\_0x00ad Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Thresh Accum
\\
\hline
tx\_quad\_cal\_lpf\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d174\_0x00ae Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Tx Quad LPF Gain
\\
\hline
ocpi\_pad\_0af & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_bbf\_r1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d194\_0x00c2 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF R1
\\
\hline
tx\_bbf\_r2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d195\_0x00c3 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF R2
\\
\hline
tx\_bbf\_r3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d196\_0x00c4 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF R3
\\
\hline
tx\_bbf\_r4 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d197\_0x00c5 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF R4
\\
\hline
tx\_bbf\_rp & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d198\_0x00c6 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF RP
\\
\hline
tx\_bbf\_c1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d199\_0x00c7 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF C1
\\
\hline
tx\_bbf\_c2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d200\_0x00c8 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF C2
\\
\hline
tx\_bbf\_cp & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d201\_0x00c9 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF CP
\\
\hline
tx\_bbf\_tuner\_pd & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d202\_0x00ca Table 34: Tx BASEBAND FILTER REGISTERS: Tx Tuner PD
\\
\hline
tx\_bbf\_r2b & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d203\_0x00cb Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF R2b
\\
\hline
ocpi\_pad\_0cc & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_secondf\_config0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d208\_0x00d0 Table 35: Tx SECONDARY FILTER REGISTERS: Config0
\\
\hline
tx\_secondf\_resistor & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d209\_0x00d1 Table 35: Tx SECONDARY FILTER REGISTERS: Resistor
\\
\hline
tx\_secondf\_capacitor & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d210\_0x00d2 Table 35: Tx SECONDARY FILTER REGISTERS: Capacitor
\\
\hline
tx\_secondf\_mustbe0x60 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d211\_0x00d3 Table 35: Tx SECONDARY FILTER REGISTERS: Must be 0x60
\\
\hline
ocpi\_pad\_0d4 & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_bbf\_tune\_divider & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d214\_0x00d6 Table 38: Tx BBF TUNER CONFIGURATION: TX BBF Tune Divider
\\
\hline
tx\_bbf\_tune\_mode & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d215\_0x00d7 Table 38: Tx BBF TUNER CONFIGURATION: TX BBF Tune Mode
\\
\hline
ocpi\_pad\_0d8 & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_filter\_coef\_addr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d240\_0x00f0 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Coeff Addr
\\
\hline
rx\_filter\_coef\_write\_data\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d241\_0x00f1 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Coeff Data 1
\\
\hline
rx\_filter\_coef\_write\_data\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d242\_0x00f2 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Coeff Data 2
\\
\hline
rx\_filter\_coef\_read\_data\_1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d243\_0x00f3 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Coeff Read Data 1
\\
\hline
rx\_filter\_coef\_read\_data\_2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d244\_0x00f4 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Coeff Read Data 2
\\
\hline
rx\_filter\_conf & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d245\_0x00f5 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Configuration
\\
\hline
rx\_filter\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d246\_0x00f6 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Gain
\\
\hline
ocpi\_pad\_0f7 & uchar  & - & - &  & -  &- & -
\\
\hline
gain\_agc\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d250\_0x00fa Table 42: GAIN CONTROL SETUP: AGC Config1
\\
\hline
gain\_agc\_config\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d251\_0x00fb Table 42: GAIN CONTROL SETUP: AGC config2
\\
\hline
gain\_agc\_config\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d252\_0x00fc Table 42: GAIN CONTROL SETUP: AGC Config3
\\
\hline
gain\_max\_lmt\_full\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d253\_0x00fd Table 42: GAIN CONTROL SETUP: Max LMT/Full Gain
\\
\hline
gain\_peak\_wait\_time & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d254\_0x00fe Table 42: GAIN CONTROL SETUP: Peak Wait Time
\\
\hline
ocpi\_pad\_0ff & uchar  & - & - &  & -  &- & -
\\
\hline
gain\_digital\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d256\_0x0100 Table 42: GAIN CONTROL SETUP: Digital Gain
\\
\hline
gain\_agc\_lock\_level & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d257\_0x0101 Table 42: GAIN CONTROL SETUP: AGC Lock Level
\\
\hline
ocpi\_pad\_102 & uchar  & - & - &  & -  &- & -
\\
\hline
gain\_gain\_stp\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d259\_0x0103 Table 42: GAIN CONTROL SETUP: Gain Step Config 1
\\
\hline
gain\_adc\_small\_overload\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d260\_0x0104 Table 42: GAIN CONTROL SETUP: ADC Small Overload Threshold
\\
\hline
gain\_adc\_large\_overload\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d261\_0x0105 Table 42: GAIN CONTROL SETUP: ADC Large Overload Threshold
\\
\hline
gain\_stp\_config\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d262\_0x0106 Table 42: GAIN CONTROL SETUP: Gain Step Config 2
\\
\hline
gain\_small\_lmt\_overload\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d263\_0x0107 Table 42: GAIN CONTROL SETUP: Small LMT Overload Threshold
\\
\hline
gain\_large\_lmt\_overload\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d264\_0x0108 Table 42: GAIN CONTROL SETUP: Large LMT Overload Threshold
\\
\hline
gain\_rx1\_manual\_lmt\_full\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d265\_0x0109 Table 42: GAIN CONTROL SETUP: Rx1 Manual LMT/Full Gain
\\
\hline
gain\_rx1\_manual\_lpf\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d266\_0x010a Table 42: GAIN CONTROL SETUP: Rx1 Manual LPF gain
\\
\hline
gain\_rx1\_manual\_digitalforced\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d267\_0x010b Table 42: GAIN CONTROL SETUP: Rx1 Manual Digital/Forced Gain
\\
\hline
gain\_rx2\_manual\_lmt\_full\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d268\_0x010c Table 42: GAIN CONTROL SETUP: Rx2 Manual LMT/Full Gain
\\
\hline
gain\_rx2\_manual\_lpf\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d269\_0x010d Table 42: GAIN CONTROL SETUP: Rx2 Manual LPF Gain
\\
\hline
gain\_rx2\_manual\_digitalforced\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d270\_0x010e Table 42: GAIN CONTROL SETUP: Rx2 Manual Digital/Forced Gain
\\
\hline
ocpi\_pad\_10f & uchar  & - & - &  & -  &- & -
\\
\hline
fast\_agc\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d272\_0x0110 Table 44: FAST ATTACK AGC SETUP: Config 1
\\
\hline
fast\_agc\_config\_2\_settling\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d273\_0x0111 Table 44: FAST ATTACK AGC SETUP: Config 2 \& Settling Delay
\\
\hline
fast\_agc\_energy\_lost\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d274\_0x0112 Table 44: FAST ATTACK AGC SETUP: Energy Lost Threshold
\\
\hline
fast\_agc\_stronger\_signal\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d275\_0x0113 Table 44: FAST ATTACK AGC SETUP: Stronger Signal Threshold
\\
\hline
fast\_agc\_low\_power\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d276\_0x0114 Table 44: FAST ATTACK AGC SETUP: Low Power Threshold
\\
\hline
fast\_agc\_strong\_signal\_freeze & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d277\_0x0115 Table 44: FAST ATTACK AGC SETUP: Strong Signal Freeze
\\
\hline
fast\_agc\_final\_over\_range\_and\_opt\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d278\_0x0116 Table 44: FAST ATTACK AGC SETUP: Final Over Range and Opt Gain
\\
\hline
fast\_agc\_energy\_detect\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d279\_0x0117 Table 44: FAST ATTACK AGC SETUP: Energy Detect Count
\\
\hline
fast\_agc\_agcll\_upper\_limit & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d280\_0x0118 Table 44: FAST ATTACK AGC SETUP: AGCLL Upper Limit
\\
\hline
fast\_agc\_gain\_lock\_exit\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d281\_0x0119 Table 44: FAST ATTACK AGC SETUP: Gain Lock Exit Count
\\
\hline
fast\_agc\_initial\_lmt\_gain\_limit & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d282\_0x011a Table 44: FAST ATTACK AGC SETUP: Initial LMT Gain Limit
\\
\hline
fast\_agc\_increment\_time & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d283\_0x011b Table 44: FAST ATTACK AGC SETUP: Increment Time
\\
\hline
ocpi\_pad\_11c & uchar  & - & - &  & -  &- & -
\\
\hline
slowhybrid\_agc\_inner\_low\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d288\_0x0120 Table 45: SLOW ATTACK AND HYBRID AGC: AGC Inner Low Threshold
\\
\hline
slowhybrid\_agc\_lmt\_overload\_counters & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d289\_0x0121 Table 45: SLOW ATTACK AND HYBRID AGC: LMT Overload Counters
\\
\hline
slowhybrid\_agc\_adc\_overload\_counters & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d290\_0x0122 Table 45: SLOW ATTACK AND HYBRID AGC: ADC Overload Counters
\\
\hline
slowhybrid\_agc\_gain\_stp1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d291\_0x0123 Table 45: SLOW ATTACK AND HYBRID AGC: Gain Step1
\\
\hline
slowhybrid\_agc\_gain\_update\_counter1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d292\_0x0124 Table 45: SLOW ATTACK AND HYBRID AGC: Gain Update Counter1
\\
\hline
slowhybrid\_agc\_gain\_update\_counter2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d293\_0x0125 Table 45: SLOW ATTACK AND HYBRID AGC: Gain Update Counter2
\\
\hline
ocpi\_pad\_126 & uchar  & - & - &  & -  &- & -
\\
\hline
slowhybrid\_agc\_digital\_sat\_counter & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d296\_0x0128 Table 45: SLOW ATTACK AND HYBRID AGC: Digital Sat Counter
\\
\hline
slowhybrid\_agc\_outer\_power\_threshs & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d297\_0x0129 Table 45: SLOW ATTACK AND HYBRID AGC: Outer Power Thresholds
\\
\hline
slowhybrid\_agc\_gain\_stp\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d298\_0x012a Table 45: SLOW ATTACK AND HYBRID AGC: Gain Step 2
\\
\hline
ocpi\_pad\_12b & uchar  & - & - &  & -  &- & -
\\
\hline
ext\_lna\_high\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d300\_0x012c Table 46: EXTERNAL LNA GAIN WORD: Ext LNA High Gain
\\
\hline
ext\_lna\_low\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d301\_0x012d Table 46: EXTERNAL LNA GAIN WORD: Ext LNA Low Gain
\\
\hline
ocpi\_pad\_12e & uchar  & - & - &  & -  &- & -
\\
\hline
gain\_table & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d304\_0x0130 Table 47: AGC GAIN TABLE: Gain Table Address
\\
\hline
gain\_table\_write\_data1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d305\_0x0131 Table 47: AGC GAIN TABLE: Gain Table Write Data1
\\
\hline
gain\_table\_write\_data2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d306\_0x0132 Table 47: AGC GAIN TABLE: Gain Table Write Data2
\\
\hline
gain\_table\_write\_data3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d307\_0x0133 Table 47: AGC GAIN TABLE: Gain Table Write Data 3
\\
\hline
gain\_table\_read\_data1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d308\_0x0134 Table 47: AGC GAIN TABLE: Gain Table Read Data 1
\\
\hline
gain\_table\_read\_data2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d309\_0x0135 Table 47: AGC GAIN TABLE: Gain Table Read Data 2
\\
\hline
gain\_table\_read\_data3 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d310\_0x0136 Table 47: AGC GAIN TABLE: Gain Table Read Data 3
\\
\hline
gain\_table\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d311\_0x0137 Table 47: AGC GAIN TABLE: Gain Table Config
\\
\hline
mixer\_subtable & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d312\_0x0138 Table 48: MIXER SUBTABLE: Mixer Subtable Address
\\
\hline
mixer\_subtable\_gain\_write & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d313\_0x0139 Table 48: MIXER SUBTABLE: Mixer Subtable Gain Word Write
\\
\hline
mixer\_subtable\_bias\_write & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d314\_0x013a Table 48: MIXER SUBTABLE: Mixer Subtable Bias Word Write
\\
\hline
mixer\_subtable\_ctrl\_write & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d315\_0x013b Table 48: MIXER SUBTABLE: Mixer Subtable Control Word Write
\\
\hline
mixer\_subtable\_gain\_read & uchar  & - & - & Volatile & -  &- & reg\_addr\_d316\_0x013c Table 48: MIXER SUBTABLE: Mixer Subtable Gain Word Read
\\
\hline
mixer\_subtable\_bias\_read & uchar  & - & - & Volatile & -  &- & reg\_addr\_d317\_0x013d Table 48: MIXER SUBTABLE: Mixer Subtable Bias Word Read
\\
\hline
mixer\_subtable\_ctrl\_read & uchar  & - & - & Volatile & -  &- & reg\_addr\_d318\_0x013e Table 48: MIXER SUBTABLE: Mixer Subtable Control Word Read
\\
\hline
mixer\_subtable\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d319\_0x013f Table 48: MIXER SUBTABLE: Mixer Subtable Config
\\
\hline
calib\_gain\_table\_word & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d320\_0x0140 Table 49: CALIBRATION GAIN TABLE: Word\_Address
\\
\hline
calib\_gain\_table\_diff\_worderror\_write & uchar  & - & - & Writeable & -  &- & reg\_addr\_d321\_0x0141 Table 49: CALIBRATION GAIN TABLE: Gain Diff Word/Error Write
\\
\hline
calib\_gain\_table\_gain\_error\_read & uchar  & - & - & Volatile & -  &- & reg\_addr\_d322\_0x0142 Table 49: CALIBRATION GAIN TABLE: Gain Error Read
\\
\hline
calib\_gain\_table\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d323\_0x0143 Table 49: CALIBRATION GAIN TABLE: Config
\\
\hline
calib\_gain\_table\_lna\_diff\_read\_back & uchar  & - & - & Volatile & -  &- & reg\_addr\_d324\_0x0144 Table 49: CALIBRATION GAIN TABLE: LNA Gain Diff Read Back
\\
\hline
gen\_calib\_max\_mixer\_gain\_index & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d325\_0x0145 Table 50: GENERAL CALIBRATION: Max Mixer Calibration Gain Index
\\
\hline
gen\_calib\_temp\_gain\_coef & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d326\_0x0146 Table 50: GENERAL CALIBRATION: Temp Gain Coefficient
\\
\hline
gen\_calib\_settle\_time & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d327\_0x0147 Table 50: GENERAL CALIBRATION: Settle Time
\\
\hline
gen\_calib\_measure\_duration & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d328\_0x0148 Table 50: GENERAL CALIBRATION: Measure Duration
\\
\hline
gen\_calib\_cal\_temp\_sensor\_word & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d329\_0x0149 Table 50: GENERAL CALIBRATION: Cal Temp sensor word
\\
\hline
ocpi\_pad\_14a & uchar  & - & - &  & -  &- & -
\\
\hline
rssi\_measure\_duration\_01 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d336\_0x0150 Table 51: RSSI MEASUREMENT CONFIGURATION: Measure Duration 0,1
\\
\hline
rssi\_measure\_duration\_23 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d337\_0x0151 Table 51: RSSI MEASUREMENT CONFIGURATION: Measure Duration 2,3
\\
\hline
rssi\_weight\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d338\_0x0152 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI Weight 0
\\
\hline
rssi\_weight\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d339\_0x0153 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI Weight 1
\\
\hline
rssi\_weight\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d340\_0x0154 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI Weight 2
\\
\hline
rssi\_weight\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d341\_0x0155 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI Weight 3
\\
\hline
rssi\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d342\_0x0156 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI delay
\\
\hline
rssi\_wait\_time & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d343\_0x0157 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI wait time
\\
\hline
rssi\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d344\_0x0158 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI Config
\\
\hline
ocpi\_pad\_159 & uchar  & - & - &  & -  &- & -
\\
\hline
rssi\_dec\_power\_duration\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d348\_0x015c Table 51: RSSI MEASUREMENT CONFIGURATION: Dec Power Duration
\\
\hline
rssi\_lna\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d349\_0x015d Table 51: RSSI MEASUREMENT CONFIGURATION: LNA Gain
\\
\hline
ocpi\_pad\_15e & uchar  & - & - &  & -  &- & -
\\
\hline
power\_ch1\_rx\_filter\_power & uchar  & - & - & Volatile & -  &- & reg\_addr\_d353\_0x0161 Table 53: POWER WORD: CH1 Rx filter Power
\\
\hline
ocpi\_pad\_162 & uchar  & - & - &  & -  &- & -
\\
\hline
power\_ch2\_rx\_filter\_power & uchar  & - & - & Volatile & -  &- & reg\_addr\_d355\_0x0163 Table 53: POWER WORD: CH2 Rx filter Power
\\
\hline
ocpi\_pad\_164 & uchar  & - & - &  & -  &- & -
\\
\hline
calibration\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d361\_0x0169 Table 54: Rx QUADRATURE CALIBRATION: Calibration Config 1
\\
\hline
calibration\_mustbe0x75 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d362\_0x016a Table 54: Rx QUADRATURE CALIBRATION: Must be 0x75
\\
\hline
calibration\_mustbe0x95 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d363\_0x016b Table 54: Rx QUADRATURE CALIBRATION: Must be 0x95
\\
\hline
ocpi\_pad\_16c & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_pgo\_phase\_corr\_rx1\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d368\_0x0170 Table 55: Rx PHASE AND GAIN CORRECTION: Rx1A Phase Corr
\\
\hline
rx\_pgo\_gain\_corr\_rx1\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d369\_0x0171 Table 55: Rx PHASE AND GAIN CORRECTION: Rx1A Gain Corr
\\
\hline
rx\_pgo\_phase\_corr\_rx2\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d370\_0x0172 Table 55: Rx PHASE AND GAIN CORRECTION: Rx2A Phase Corr
\\
\hline
rx\_pgo\_gain\_corr\_rx2\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d371\_0x0173 Table 55: Rx PHASE AND GAIN CORRECTION: Rx2A Gain Corr
\\
\hline
rx\_pgo\_offset\_corr\_rx1\_ina\_q & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d372\_0x0174 Table 55: Rx PHASE AND GAIN CORRECTION: Rx1A Q Offset
\\
\hline
rx\_pgo\_offset\_corr\_rx1\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d373\_0x0175 Table 55: Rx PHASE AND GAIN CORRECTION: Rx1A Offset
\\
\hline
rx\_pgo\_offset\_corr\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d374\_0x0176 Table 55: Rx PHASE AND GAIN CORRECTION: Input A Offsets
\\
\hline
rx\_pgo\_offset\_corr\_rx2\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d375\_0x0177 Table 55: Rx PHASE AND GAIN CORRECTION: Rx2A Offset
\\
\hline
rx\_pgo\_offset\_corr\_rx2\_ina\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d376\_0x0178 Table 55: Rx PHASE AND GAIN CORRECTION: Rx2A I Offset
\\
\hline
rx\_pgo\_phase\_corr\_rx1\_inbc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d377\_0x0179 Table 55: Rx PHASE AND GAIN CORRECTION: Rx1B/C Phase Corr
\\
\hline
rx\_pgo\_gain\_corr\_rx1\_inbc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d378\_0x017a Table 55: Rx PHASE AND GAIN CORRECTION: Rx1B/C Gain Corr
\\
\hline
rx\_pgo\_phase\_corr\_rx2\_inbc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d379\_0x017b Table 55: Rx PHASE AND GAIN CORRECTION: Rx2B/C Phase Corr
\\
\hline
rx\_pgo\_gain\_corr\_rx2\_inbc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d380\_0x017c Table 55: Rx PHASE AND GAIN CORRECTION: Rx2B/C Gain Corr
\\
\hline
rx\_pgo\_offset\_corr\_rx1\_inbc\_q & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d381\_0x017d Table 55: Rx PHASE AND GAIN CORRECTION: Rx1B/C Q Offset
\\
\hline
rx\_pgo\_offset\_corr\_rx1\_inbc\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d382\_0x017e Table 55: Rx PHASE AND GAIN CORRECTION: Rx1B/C I Offset
\\
\hline
rx\_pgo\_offset\_corr\_inpbc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d383\_0x017f Table 55: Rx PHASE AND GAIN CORRECTION: Input B/C Offsets
\\
\hline
rx\_pgo\_offset\_corr\_rx2\_inbc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d384\_0x0180 Table 55: Rx PHASE AND GAIN CORRECTION: Rx2 B/C Offset
\\
\hline
rx\_pgo\_offset\_corr\_rx2\_inbc\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d385\_0x0181 Table 55: Rx PHASE AND GAIN CORRECTION: Rx2 B/C I Offset
\\
\hline
rx\_pgo\_force\_bits & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d386\_0x0182 Table 55: Rx PHASE AND GAIN CORRECTION: Force Bits
\\
\hline
ocpi\_pad\_183 & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_dc\_offset\_wait\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d389\_0x0185 Table 56: Rx DC OFFSET CONTROL: Wait Count
\\
\hline
rx\_dc\_offset\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d390\_0x0186 Table 56: Rx DC OFFSET CONTROL: RF DC Offset Count
\\
\hline
rx\_dc\_offset\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d391\_0x0187 Table 56: Rx DC OFFSET CONTROL: RF DC Offset Config 1
\\
\hline
rx\_dc\_offset\_atten & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d392\_0x0188 Table 56: Rx DC OFFSET CONTROL: RF DC Offset Attenuation
\\
\hline
rx\_dc\_offset\_mustbe0x30 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d393\_0x0189 Table 56: Rx DC OFFSET CONTROL: Must be 0x30
\\
\hline
ocpi\_pad\_18a & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_dc\_offset\_config2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d395\_0x018b Table 56: Rx DC OFFSET CONTROL: DC Offset Config2
\\
\hline
rx\_dc\_offset\_rf\_cal\_gain\_index & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d396\_0x018c Table 56: Rx DC OFFSET CONTROL: RF Cal Gain Index
\\
\hline
rx\_dc\_offset\_soi\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d397\_0x018d Table 56: Rx DC OFFSET CONTROL: SOI Threshold
\\
\hline
ocpi\_pad\_18e & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_dc\_offset\_bb\_shift & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d400\_0x0190 Table 56: Rx DC OFFSET CONTROL: BB DC Offset Shift
\\
\hline
rx\_dc\_offset\_bb\_fast\_settle\_shift & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d401\_0x0191 Table 56: Rx DC OFFSET CONTROL: BB DC Offset Fast Settle Shift
\\
\hline
rx\_dc\_offset\_bb\_fast\_settle\_dur & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d402\_0x0192 Table 56: Rx DC OFFSET CONTROL: BB Fast Settle Dur
\\
\hline
rx\_dc\_offset\_bb\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d403\_0x0193 Table 56: Rx DC OFFSET CONTROL: BB DC Offset Count
\\
\hline
rx\_dc\_offset\_bb\_atten & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d404\_0x0194 Table 56: Rx DC OFFSET CONTROL: BB DC Offset Attenuation
\\
\hline
ocpi\_pad\_195 & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_bb\_dc\_offset\_rx1\_word\_i\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d410\_0x019a Table 60: Rx BB DC OFFSET: RX1 BB DC word I MSB
\\
\hline
rx\_bb\_dc\_offset\_rx1\_word\_i\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d411\_0x019b Table 60: Rx BB DC OFFSET: RX1 BB DC word I LSB
\\
\hline
rx\_bb\_dc\_offset\_rx1\_word\_q\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d412\_0x019c Table 60: Rx BB DC OFFSET: RX1 BB DC word Q MSB
\\
\hline
rx\_bb\_dc\_offset\_rx1\_word\_q\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d413\_0x019d Table 60: Rx BB DC OFFSET: RX1 BB DC word Q LSB
\\
\hline
rx\_bb\_dc\_offset\_rx2\_word\_i\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d414\_0x019e Table 60: Rx BB DC OFFSET: RX2 BB DC word I MSB
\\
\hline
rx\_bb\_dc\_offset\_rx2\_word\_i\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d415\_0x019f Table 60: Rx BB DC OFFSET: RX2 BB DC word I LSB
\\
\hline
rx\_bb\_dc\_offset\_rx2\_word\_q\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d416\_0x01a0 Table 60: Rx BB DC OFFSET: RX2 BB DC word Q MSB
\\
\hline
rx\_bb\_dc\_offset\_rx2\_word\_q\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d417\_0x01a1 Table 60: Rx BB DC OFFSET: RX2 BB DC word Q LSB
\\
\hline
rx\_bb\_dc\_offset\_track\_corr\_word\_i\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d418\_0x01a2 Table 60: Rx BB DC OFFSET: BB Track corr word I MSB
\\
\hline
rx\_bb\_dc\_offset\_track\_corr\_word\_i\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d419\_0x01a3 Table 60: Rx BB DC OFFSET: BB Track corr word I LSB
\\
\hline
rx\_bb\_dc\_offset\_track\_corr\_word\_q\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d420\_0x01a4 Table 60: Rx BB DC OFFSET: BB Track corr word Q MSB
\\
\hline
rx\_bb\_dc\_offset\_track\_corr\_word\_q\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d421\_0x01a5 Table 60: Rx BB DC OFFSET: BB Track corr word Q LSB
\\
\hline
ocpi\_pad\_1a6 & uchar  & - & - &  & -  &- & -
\\
\hline
rssi\_readback\_rx1\_symbol & uchar  & - & - & Volatile & -  &- & reg\_addr\_d423\_0x01a7 Table 61: RSSI READBACK: Rx1 RSSI Symbol
\\
\hline
rssi\_readback\_rx1\_preamble & uchar  & - & - & Volatile & -  &- & reg\_addr\_d424\_0x01a8 Table 61: RSSI READBACK: Rx1 RSSI preamble
\\
\hline
rssi\_readback\_rx2\_symbol & uchar  & - & - & Volatile & -  &- & reg\_addr\_d425\_0x01a9 Table 61: RSSI READBACK: Rx2 RSSI symbol
\\
\hline
rssi\_readback\_rx2\_preamble & uchar  & - & - & Volatile & -  &- & reg\_addr\_d426\_0x01aa Table 61: RSSI READBACK: Rx2 RSSI preamble
\\
\hline
rssi\_readback\_symbol\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d427\_0x01ab Table 61: RSSI READBACK: Symbol LSB
\\
\hline
rssi\_readback\_preamble\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d428\_0x01ac Table 61: RSSI READBACK: Preamble LSB
\\
\hline
ocpi\_pad\_1ad & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_tia\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d475\_0x01db Table 62: Rx TIA: Rx TIA Config
\\
\hline
rx\_tia\_1\_c\_lsb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d476\_0x01dc Table 62: Rx TIA: TIA1 C LSB
\\
\hline
rx\_tia\_1\_c\_msb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d477\_0x01dd Table 62: Rx TIA: TIA1 C MSB
\\
\hline
rx\_tia\_2\_c\_lsb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d478\_0x01de Table 62: Rx TIA: TIA2 C LSB
\\
\hline
rx\_tia\_2\_c\_msb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d479\_0x01df Table 62: Rx TIA: TIA2 C MSB
\\
\hline
rx\_bbf\_rx1\_r1a & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d480\_0x01e0 Table 65: Rx BFF: Rx1 BBF R1A
\\
\hline
rx\_bbf\_rx2\_r1a & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d481\_0x01e1 Table 65: Rx BFF: Rx2 BBF R1A
\\
\hline
rx\_bff\_rx1\_tune\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d482\_0x01e2 Table 65: Rx BFF: Rx1 Tune Control
\\
\hline
rx\_bff\_rx2\_tune\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d483\_0x01e3 Table 65: Rx BFF: Rx2 Tune Control
\\
\hline
rx\_bff\_rx1\_bbf\_r5 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d484\_0x01e4 Table 65: Rx BFF: Rx1 BBF R5
\\
\hline
rx\_bff\_rx2\_bbf\_r5 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d485\_0x01e5 Table 65: Rx BFF: Rx2 BBF R5
\\
\hline
rx\_bbf\_r2346 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d486\_0x01e6 Table 65: Rx BFF: Rx BBF R2346
\\
\hline
rx\_bbf\_c1\_msb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d487\_0x01e7 Table 65: Rx BFF: Rx BBF C1 MSB
\\
\hline
rx\_bbf\_c1\_lsb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d488\_0x01e8 Table 65: Rx BFF: Rx BBF C1 LSB
\\
\hline
rx\_bbf\_c2\_msb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d489\_0x01e9 Table 65: Rx BFF: Rx BBF C2 MSB
\\
\hline
rx\_bbf\_c2\_lsb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d490\_0x01ea Table 65: Rx BFF: Rx BBF C2 LSB
\\
\hline
rx\_bbf\_c3\_msb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d491\_0x01eb Table 65: Rx BFF: Rx BBF C3 MSB
\\
\hline
rx\_bbf\_c3\_lsb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d492\_0x01ec Table 65: Rx BFF: Rx BBF C3 LSB
\\
\hline
rx\_bbf\_cc1\_ctr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d493\_0x01ed Table 65: Rx BFF: Rx BBF CC1 Ctr
\\
\hline
rx\_bbf\_mustbe0x60 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d494\_0x01ee Table 65: Rx BFF: Must be 0x60
\\
\hline
rx\_bbf\_cc2\_ctr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d495\_0x01ef Table 65: Rx BFF: Rx BBF CC2 Ctr
\\
\hline
rx\_bbf\_pow\_rz\_byte1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d496\_0x01f0 Table 65: Rx BFF: Rx BBF Pow Rz Byte1
\\
\hline
rx\_bbf\_cc3\_ctr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d497\_0x01f1 Table 65: Rx BFF: Rx BBF CC3 Ctr
\\
\hline
rx\_bbf\_r5\_tune & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d498\_0x01f2 Table 65: Rx BFF: Rx BBF R5 Tune
\\
\hline
rx\_bbf\_tune & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d499\_0x01f3 Table 65: Rx BFF: Rx BBF Tune
\\
\hline
rx\_bff\_rx1\_bbf\_man\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d500\_0x01f4 Table 65: Rx BFF: Rx1 BBF Man Gain
\\
\hline
rx\_bff\_rx2\_bbf\_man\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d501\_0x01f5 Table 65: Rx BFF: Rx2 BBF Man Gain
\\
\hline
ocpi\_pad\_1f6 & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_bbf\_tune\_config\_divide & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d504\_0x01f8 Table 66: Rx BBF TUNER CONFIGURATION: RX BBF Tune Divide
\\
\hline
rx\_bbf\_tune\_config\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d505\_0x01f9 Table 66: Rx BBF TUNER CONFIGURATION: RX BBF Tune Config
\\
\hline
rx\_bbf\_tune\_config\_mustbe0x01 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d506\_0x01fa Table 66: Rx BBF TUNER CONFIGURATION: Must be 0x01
\\
\hline
rx\_bbf\_tune\_config\_rx\_bbbw\_mhz & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d507\_0x01fb Table 66: Rx BBF TUNER CONFIGURATION: Rx BBBW MHz
\\
\hline
rx\_bbf\_tune\_config\_rx\_bbbw\_khz & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d508\_0x01fc Table 66: Rx BBF TUNER CONFIGURATION: Rx BBBW kHz
\\
\hline
ocpi\_pad\_1fd & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_synth\_disable\_vco\_cal & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d560\_0x0230 Table 67: Rx SYNTHESIZER: Disable VCO Cal
\\
\hline
rx\_synth\_integer\_byte\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d561\_0x0231 Table 67: Rx SYNTHESIZER: RX Integer Byte 0
\\
\hline
rx\_synth\_integer\_byte\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d562\_0x0232 Table 67: Rx SYNTHESIZER: RX Integer Byte 1
\\
\hline
rx\_synth\_fract\_byte\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d563\_0x0233 Table 67: Rx SYNTHESIZER: RX Fractional Byte 0
\\
\hline
rx\_synth\_fract\_byte\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d564\_0x0234 Table 67: Rx SYNTHESIZER: RX Fractional Byte 1
\\
\hline
rx\_synth\_fract\_byte\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d565\_0x0235 Table 67: Rx SYNTHESIZER: RX Fractional Byte 2
\\
\hline
rx\_synth\_force\_alc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d566\_0x0236 Table 67: Rx SYNTHESIZER: RX Force ALC
\\
\hline
rx\_synth\_force\_vco\_tune\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d567\_0x0237 Table 67: Rx SYNTHESIZER: RX Force VCO Tune 0
\\
\hline
rx\_synth\_force\_vco\_tune\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d568\_0x0238 Table 67: Rx SYNTHESIZER: RX Force VCO Tune 1
\\
\hline
rx\_synth\_alc\_varactor & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d569\_0x0239 Table 67: Rx SYNTHESIZER: RX ALC/Varactor
\\
\hline
rx\_synth\_vco\_output & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d570\_0x023a Table 67: Rx SYNTHESIZER: RX VCO Output
\\
\hline
rx\_synth\_cp\_current & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d571\_0x023b Table 67: Rx SYNTHESIZER: RX CP Current
\\
\hline
rx\_synth\_cp\_offset & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d572\_0x023c Table 67: Rx SYNTHESIZER: RX CP Offset
\\
\hline
rx\_synth\_cp\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d573\_0x023d Table 67: Rx SYNTHESIZER: RX CP Config
\\
\hline
rx\_synth\_loop\_filter\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d574\_0x023e Table 67: Rx SYNTHESIZER: RX Loop Filter 1
\\
\hline
rx\_synth\_loop\_filter\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d575\_0x023f Table 67: Rx SYNTHESIZER: RX Loop Filter 2
\\
\hline
rx\_synth\_loop\_filter\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d576\_0x0240 Table 67: Rx SYNTHESIZER: RX Loop Filter 3
\\
\hline
rx\_synth\_dithercp\_cal & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d577\_0x0241 Table 67: Rx SYNTHESIZER: RX Dither/CP Cal
\\
\hline
rx\_synth\_vco\_bias\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d578\_0x0242 Table 67: Rx SYNTHESIZER: RX VCO Bias 1
\\
\hline
rx\_synth\_mustbe0x0d & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d579\_0x0243 Table 67: Rx SYNTHESIZER: Must be 0x0D
\\
\hline
rx\_synth\_cal\_status & uchar  & - & - & Volatile & -  &- & reg\_addr\_d580\_0x0244 Table 67: Rx SYNTHESIZER: RX Cal Status
\\
\hline
rx\_synth\_mustbe0x00 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d581\_0x0245 Table 67: Rx SYNTHESIZER: Must be 0x00
\\
\hline
rx\_synth\_mustbe0x02 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d582\_0x0246 Table 67: Rx SYNTHESIZER: Set to 0x02 (Must be 0x02)
\\
\hline
rx\_synth\_cp\_ovrg\_vco\_lock & uchar  & - & - & Volatile & -  &- & reg\_addr\_d583\_0x0247 Table 67: Rx SYNTHESIZER: RX CP Ovrg/VCO Lock
\\
\hline
rx\_synth\_mustbe0x0b & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d584\_0x0248 Table 67: Rx SYNTHESIZER: Set to 0x0B (Must be 0x0B)
\\
\hline
rx\_synth\_vco\_cal & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d585\_0x0249 Table 67: Rx SYNTHESIZER: RX VCO Cal
\\
\hline
rx\_synth\_lock\_detect\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d586\_0x024a Table 67: Rx SYNTHESIZER: RX Lock Detect Config
\\
\hline
rx\_synth\_mustbe0x17 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d587\_0x024b Table 67: Rx SYNTHESIZER: Must be 0x17
\\
\hline
rx\_synth\_mustbe0x00\_also & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d588\_0x024c Table 67: Rx SYNTHESIZER: Must be 0x00
\\
\hline
rx\_synth\_mustbe0x00\_also\_also & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d589\_0x024d Table 67: Rx SYNTHESIZER: Must be 0x00
\\
\hline
ocpi\_pad\_24e & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_synth\_must\_be\_0x70 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d592\_0x0250 Table 67: Rx SYNTHESIZER: Set to 0x70 (Must be 0x70)
\\
\hline
rx\_synth\_vco\_varactor\_ctrl\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d593\_0x0251 Table 67: Rx SYNTHESIZER: RX VCO Varactor Control 1
\\
\hline
ocpi\_pad\_252 & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_fast\_lock\_setup & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d602\_0x025a Table 71: Rx FAST LOCK: Rx Fast Lock Setup
\\
\hline
rx\_fast\_lock\_setup\_init\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d603\_0x025b Table 71: Rx FAST LOCK: Rx Fast Lock Setup Init Delay
\\
\hline
rx\_fast\_lock\_program\_addr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d604\_0x025c Table 71: Rx FAST LOCK: Rx Fast Lock Program Address
\\
\hline
rx\_fast\_lock\_program\_data & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d605\_0x025d Table 71: Rx FAST LOCK: Rx Fast Lock Program Data
\\
\hline
rx\_fast\_lock\_program\_read & uchar  & - & - & Volatile & -  &- & reg\_addr\_d606\_0x025e Table 71: Rx FAST LOCK: Rx Fast Lock Program Read
\\
\hline
rx\_fast\_lock\_program\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d607\_0x025f Table 71: Rx FAST LOCK: Rx Fast Lock Program Control
\\
\hline
ocpi\_pad\_260 & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_lo\_gen\_power\_mode & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d609\_0x0261 Table 72: Rx LO GENERATION: Rx LO Gen Power Mode
\\
\hline
ocpi\_pad\_262 & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_synth\_disable\_vco\_cal & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d624\_0x0270 Table 73: Tx SYNTHESIZER: Disable VCO Cal
\\
\hline
tx\_synth\_integer\_byte\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d625\_0x0271 Table 73: Tx SYNTHESIZER: Integer Byte 0
\\
\hline
tx\_synth\_integer\_byte\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d626\_0x0272 Table 73: Tx SYNTHESIZER: Integer Byte 1
\\
\hline
tx\_synth\_fract\_byte\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d627\_0x0273 Table 73: Tx SYNTHESIZER: Fractional Byte 0
\\
\hline
tx\_synth\_fract\_byte\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d628\_0x0274 Table 73: Tx SYNTHESIZER: Fractional Byte 1
\\
\hline
tx\_synth\_fract\_byte\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d629\_0x0275 Table 73: Tx SYNTHESIZER: Fractional Byte 2
\\
\hline
tx\_synth\_force\_alc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d630\_0x0276 Table 73: Tx SYNTHESIZER: Force ALC
\\
\hline
tx\_synth\_force\_vco\_tune\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d631\_0x0277 Table 73: Tx SYNTHESIZER: Force VCO Tune 0
\\
\hline
tx\_synth\_force\_vco\_tune\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d632\_0x0278 Table 73: Tx SYNTHESIZER: Force VCO Tune 1
\\
\hline
tx\_synth\_alcvaract\_or & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d633\_0x0279 Table 73: Tx SYNTHESIZER: ALC/Varactor
\\
\hline
tx\_synth\_vco\_output & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d634\_0x027a Table 73: Tx SYNTHESIZER: VCO Output
\\
\hline
tx\_synth\_cp\_current & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d635\_0x027b Table 73: Tx SYNTHESIZER: CP Current
\\
\hline
tx\_synth\_cp\_offset & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d636\_0x027c Table 73: Tx SYNTHESIZER: CP Offset
\\
\hline
tx\_synth\_cp\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d637\_0x027d Table 73: Tx SYNTHESIZER: CP Config
\\
\hline
tx\_synth\_loop\_filter\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d638\_0x027e Table 73: Tx SYNTHESIZER: Loop Filter 1
\\
\hline
tx\_synth\_loop\_filter\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d639\_0x027f Table 73: Tx SYNTHESIZER: Loop Filter 2
\\
\hline
tx\_synth\_loop\_filter\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d640\_0x0280 Table 73: Tx SYNTHESIZER: Loop Filter 3
\\
\hline
tx\_synth\_dithercp\_cal & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d641\_0x0281 Table 73: Tx SYNTHESIZER: Dither/CP Cal
\\
\hline
tx\_synth\_vco\_bias\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d642\_0x0282 Table 73: Tx SYNTHESIZER: VCO Bias 1
\\
\hline
tx\_synth\_mustbe0x0d & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d643\_0x0283 Table 73: Tx SYNTHESIZER: Must be 0x0D
\\
\hline
tx\_synth\_cal\_status & uchar  & - & - & Volatile & -  &- & reg\_addr\_d644\_0x0284 Table 73: Tx SYNTHESIZER: Cal Status
\\
\hline
tx\_synth\_mustbe0x00 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d645\_0x0285 Table 73: Tx SYNTHESIZER: Must be 0x00
\\
\hline
tx\_synth\_mustbe0x02 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d646\_0x0286 Table 73: Tx SYNTHESIZER: Set to 0x02 (Must be 0x02)
\\
\hline
tx\_synth\_cp\_overrange\_vco\_lock & uchar  & - & - & Volatile & -  &- & reg\_addr\_d647\_0x0287 Table 73: Tx SYNTHESIZER: CP Over Range/VCO Lock
\\
\hline
tx\_synth\_mustbe0x0b & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d648\_0x0288 Table 73: Tx SYNTHESIZER: Set to 0x0B (Must be 0x0B)
\\
\hline
tx\_synth\_vco\_cal & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d649\_0x0289 Table 73: Tx SYNTHESIZER: VCO Cal
\\
\hline
tx\_synth\_lock\_detect\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d650\_0x028a Table 73: Tx SYNTEHSIZER: Lock Detect Config
\\
\hline
tx\_synth\_mustbe0x17 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d651\_0x028b Table 73: Tx SYNTEHSIZER: Must be 0x17
\\
\hline
tx\_synth\_mustbe0x00\_also & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d652\_0x028c Table 73: Tx SYNTEHSIZER: Must be 0x00
\\
\hline
tx\_synth\_mustbe0x00\_also\_also & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d653\_0x028d Table 73: Tx SYNTEHSIZER: Must be 0x00
\\
\hline
ocpi\_pad\_28e & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_synth\_mustbe0x70 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d656\_0x0290 Table 73: Tx SYNTEHSIZER: Set to 0x70 (Must be 0x70)
\\
\hline
tx\_synth\_vco\_varactor\_ctrl\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d657\_0x0291 Table 73: Tx SYNTEHSIZER: VCO Varactor Control 1
\\
\hline
dcxo\_coarse\_tune & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d658\_0x0292 Table 74: DCXO: DCXO Coarse Tune
\\
\hline
dcxo\_fine\_tune\_high & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d659\_0x0293 Table 74: DCXO: DCXO Fine Tune2
\\
\hline
dcxo\_fine\_tune\_low & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d660\_0x0294 Table 74: DCXO: DCXO Fine Tune1
\\
\hline
ocpi\_pad\_295 & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_fast\_lock\_setup & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d666\_0x029a Table 65: Tx SYNTH FAST LOCK: Tx Fast Lock Setup
\\
\hline
tx\_fast\_lock\_setup\_init\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d667\_0x029b Table 65: Tx SYNTH FAST LOCK: Tx Fast Lock Setup Init Delay
\\
\hline
tx\_fast\_lock\_program\_addr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d668\_0x029c Table 65: Tx SYNTH FAST LOCK: Tx Fast Lock Program Addr
\\
\hline
tx\_fast\_lock\_program\_data & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d669\_0x029d Table 65: Tx SYNTH FAST LOCK: Tx Fast Lock Program Data
\\
\hline
tx\_fast\_lock\_program\_read & uchar  & - & - & Volatile & -  &- & reg\_addr\_d670\_0x029e Table 65: Tx SYNTH FAST LOCK: Tx Fast Lock Program Read
\\
\hline
tx\_fast\_lock\_program\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d671\_0x029f Table 65: Tx SYNTH FAST LOCK: Tx Fast Lock Program Ctrl
\\
\hline
ocpi\_pad\_2a0 & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_lo\_gen\_power\_mode & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d673\_0x02a1 Table 76: Tx LO GENERATION: Tx LO Gen Power Mode
\\
\hline
ocpi\_pad\_2a2 & uchar  & - & - &  & -  &- & -
\\
\hline
bandgap\_mustbe0x0e & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d678\_0x02a6 Table 77: MASTER BIAS AND BANDGAP CONFIGURATION: Set to 0x0E (Must be 0x0E)
\\
\hline
ocpi\_pad\_2a7 & uchar  & - & - &  & -  &- & -
\\
\hline
bandgap\_mustbe0x0e\_also & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d680\_0x02a8 Table 77: MASTER BIAS AND BANDGAP CONFIGURATION: Set to 0x0E (Must be 0x0E)
\\
\hline
ocpi\_pad\_2a9 & uchar  & - & - &  & -  &- & -
\\
\hline
ref\_divide\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d683\_0x02ab Table 78: REFERENCE DIVIDER: Ref Divide Config 1
\\
\hline
ref\_divide\_config\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d684\_0x02ac Table 78: REFERENCE DIVIDER: Ref Divide Config 2
\\
\hline
ocpi\_pad\_2ad & uchar  & - & - &  & -  &- & -
\\
\hline
gain\_readback\_gain\_rx1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d688\_0x02b0 Table 80: Rx GAIN READ BACK: Gain Rx1
\\
\hline
gain\_readback\_lpf\_gain\_rx1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d689\_0x02b1 Table 80: Rx GAIN READ BACK: LPF Gain Rx1
\\
\hline
gain\_readback\_dig\_gain\_rx1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d690\_0x02b2 Table 80: Rx GAIN READ BACK: Dig gain Rx1
\\
\hline
gain\_readback\_fast\_attack\_state & uchar  & - & - & Volatile & -  &- & reg\_addr\_d691\_0x02b3 Table 80: Rx GAIN READ BACK: Fast Attack State
\\
\hline
gain\_readback\_slow\_loop\_state & uchar  & - & - & Volatile & -  &- & reg\_addr\_d692\_0x02b4 Table 80: Rx GAIN READ BACK: Slow Loop State
\\
\hline
gain\_readback\_gain\_rx2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d693\_0x02b5 Table 80: Rx GAIN READ BACK: Gain Rx2
\\
\hline
gain\_readback\_lpf\_gain\_rx2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d694\_0x02b6 Table 80: Rx GAIN READ BACK: LPF Gain Rx2
\\
\hline
gain\_readback\_dig\_gain\_rx2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d695\_0x02b7 Table 80: Rx GAIN READ BACK: Dig Gain Rx2
\\
\hline
gain\_readback\_ovrg\_sigs\_rx1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d696\_0x02b8 Table 80: Rx GAIN READ BACK: Ovrg Sigs Rx1
\\
\hline
gain\_readback\_ovrg\_sigs\_rx2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d697\_0x02b9 Table 80: Rx GAIN READ BACK: Ovrg Sigs Rx2
\\
\hline
ocpi\_pad\_2ba & uchar  & - & - &  & -  &- & -
\\
\hline
ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d991\_0x03df Table 83: CONTROL: Control
\\
\hline
ocpi\_pad\_3e0 & uchar  & - & - &  & -  &- & -
\\
\hline
test\_bist\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d1012\_0x03f4 Table 84: DIGITAL TEST: BIST Config
\\
\hline
test\_observe\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d1013\_0x03f5 Table 84: DIGITAL TEST: Observe Config
\\
\hline
test\_bist\_and\_data\_port\_test\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d1014\_0x03f6 Table 84: DIGITAL TEST: BIST and Data Port Test Config
\\
\hline
pin\_control\_p & bool  & - & - &  & -  &- &        Whether RX/TX powerdown via pin control is possible.
\\
\hline
other\_present & bool  & - & - & Readable & -  &- & -
\\
\hline
force\_reset & bool  & - & - & Readable, Writeable & -  &0 & Forces reset pin low (active low). Reset pin is otherwise the same level as the OpenCPI control plane reset signal.
\\
\hline
qadc0\_is\_present & bool  & - & - & Volatile & -  &- & Inidicates whether or not bitstream was built with qadc0, which supports first RX channel
\\
\hline
qadc1\_is\_present & bool  & - & - & Volatile & -  &- & Inidicates whether or not bitstream was built with qadc1, which supports second RX channel
\\
\hline
qdac0\_is\_present & bool  & - & - & Volatile & -  &- & Inidicates whether or not bitstream was built with qdac0, which supports first TX channel
\\
\hline
qdac1\_is\_present & bool  & - & - & Volatile & -  &- & Inidicates whether or not bitstream was built with qdac1, which supports second TX channel
\\
\hline
rx\_frame\_usage & enum  & - & - & Volatile & -  &- & enable : Register 0x010 bit D3 is 0, meaning Rx frame goes high coincident with the first valid receive sample. It stays high., toggle: Register 0x010 bit D3 is 1, meaning the Rx frame signal toggles with a duty cycle of 50%.
\\
\hline
data\_bus\_index\_direction & enum  & - & - & Volatile & -  &- & normal : Register 0x010 bit D1 is 0, meaning each RX sample's bit index direction is normal, i.e. [11:0], reverse: Register 0x010 bit D1 is 1, meaning each RX sample's bit direction is inverted, i.e. [0:11].
\\
\hline
data\_clk\_is\_inverted & bool  & - & - & Volatile & -  &- & false : Register 0x010 bit D0 is 0, meaning that the DATA\_CLK follows the DATA\_CLK\_P signal in the UG570 timing diagrams, true : Register 0x010 bit D0 is 1, meaning that the DATA\_CLK follows the DATA\_CLK\_N signal in the UG570 timing diagrams
\\
\hline
rx\_frame\_is\_inverted & bool  & - & - & Volatile & -  &- & false : Register 0x011 bit D2 is 0, meaning that the RX\_FRAME follows the RX\_FRAME\_P signal in the UG570 timing diagrams, true : Register 0x011 bit D2 is 1, meaning that the RX\_FRAME follows the RX\_FRAME\_N signal in the UG570 timing diagrams
\\
\hline
LVDS & bool  & - & - & Volatile & -  &- & Value is true if bitstream was built to use LVDS mode for Data/clock/frame signals, and false if CMOS mode was used.
\\
\hline
single\_port & bool  & - & - & Volatile & -  &- & Value is true if bitstream was built to use single port, and false if dual ports.
\\
\hline
swap\_ports & bool  & - & - & Volatile & -  &- & Value is true if bitstream was built to swap Port 0 and Port 1, and false if there was no swap.
\\
\hline
half\_duplex & bool  & - & - & Volatile & -  &- & Value is true if bitstream was built to use half duplex mode, and false if full duplex mode.
\\
\hline
data\_rate\_config & enum  & - & - & Volatile & -  &- & Value indicates which data rate mode (SDR/DDR) the bitstream was built to use.
\\
\hline
data\_configs\_are\_valid & bool  & - & - & Volatile & -  &- & Value is false if bitstream was built using erroneous combination of LVDS/single port/half duplex/data rate config modes (takes into account build configurations for both ad9361\_adc\_sub and ad9361\_dac\_sub workers).
\\
\hline
config\_is\_two\_r & bool  & - & - & Readable, Writeable & -  &- & Used to tell the ad9361\_adc\_sub and ad9361\_dac\_sub workers what data paths are enabled. Note that, just because a qadc or qdac worker is present in the bitstream, that doesn't mean it is enabled.
\\
\hline
config\_is\_two\_t & bool  & - & - & Readable, Writeable & -  &- & Used to tell the ad9361\_dac\_sub worker what data paths are enabled. Note that, just because a qdac worker is present in the bitstream, that doesn't mean it is enabled.
\\
\hline
force\_two\_r\_two\_t\_timing & bool  & - & - & Readable, Writeable & -  &- & Used to force the ad9361\_dac\_sub worker to use the 2R2T timing diagram regardless of what TX channels are enabled. This property is expected to correspond to the D2 bit of the Parallel Port Configuration 1 register at SPI address 0x010.
\\
\hline
Half\_Duplex\_Mode & bool  & - & - & Writeable & -  &false & -
\\
\hline
ENSM\_Pin\_Control & bool  & - & - & Writeable & -  &true & Intended to match AD9361 register 0x014 bit D4.
\\
\hline
Level\_Mode & bool  & - & - & Writeable & -  &false & Intended to match AD9361 register 0x014 bit D3.
\\
\hline
FDD\_External\_Control\_Enable & bool  & - & - & Writeable & -  &false & Intended to match AD9361 register 0x014 bit D7.
\\
\hline
ENABLE\_force\_set & bool  & - & - & Writeable & -  &false & Forces set of AD9361 ENABLE pin
\\
\hline
TXNRX\_force\_set & bool  & - & - & Writeable & -  &false & Forces set of AD9361 TXNRX pin
\\
\hline
general\_spi\_conf & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d0\_0x0000   Table 1:  CHIP LEVEL SETUP: SPI Configuration
\\
\hline
general\_multichip\_sync\_and\_tx\_mon\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d1\_0x0001   Table 1:  CHIP LEVEL SETUP: Multichip Sync and Tx Mon Control
\\
\hline
general\_tx\_enable\_filter\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d2\_0x0002   Table 1:  CHIP LEVEL SETUP: Tx Enable \& Filter Control
\\
\hline
general\_rx\_enable\_filter\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d3\_0x0003   Table 1:  CHIP LEVEL SETUP: Rx Enable \& Filter Control
\\
\hline
general\_input\_select & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d4\_0x0004   Table 1:  CHIP LEVEL SETUP: Input Select
\\
\hline
general\_rfpll\_dividers & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d5\_0x0005   Table 1:  CHIP LEVEL SETUP: RFPLL Dividers
\\
\hline
general\_rx\_clock\_data\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d6\_0x0006   Table 1:  CHIP LEVEL SETUP: Rx Clock and Data  Delay
\\
\hline
general\_tx\_clock\_data\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d7\_0x0007   Table 1:  CHIP LEVEL SETUP: Tx Clock and Data Delay
\\
\hline
ocpi\_pad\_008 & uchar  & - & - &  & -  &- & -
\\
\hline
clock\_enable & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d9\_0x0009   Table 8:  CLOCK CONTROL: Clock Enable
\\
\hline
clock\_bbpll & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d10\_0x000a  Table 8:  CLOCK CONTROL: BBPLL
\\
\hline
temp\_offset & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d11\_0x000b  Table 10: TEMPERATURE SENSOR: Offset
\\
\hline
temp\_start\_reading & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d12\_0x000c  Table 10: TEMPERATURE SENSOR: Start Temp Reading
\\
\hline
temp\_sense2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d13\_0x000d  Table 10: TEMPERATURE SENSOR: Temp Sense2
\\
\hline
temp\_temperature & uchar  & - & - & Volatile & -  &- & reg\_addr\_d14\_0x000e  Table 10: TEMPERATURE SENSOR: Temperature
\\
\hline
temp\_sensor\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d15\_0x000f  Table 10: TEMPERATURE SENSOR: Temp Sensor Config
\\
\hline
parallel\_port\_conf\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d16\_0x0010  Table 11: PARALLEL PORT CONFIGURATION: Parallel Port Configuration 1
\\
\hline
parallel\_port\_conf\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d17\_0x0011  Table 11: PARALLEL PORT CONFIGURATION: Parallel Port Configuration 2
\\
\hline
parallel\_port\_conf\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d18\_0x0012  Table 11: PARALLEL PORT CONFIGURATION: Parallel Port Configuration 3
\\
\hline
ensm\_mode & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d19\_0x0013  Table 12: ENABLE STATE MACHINE: ENSM Mode
\\
\hline
ensm\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d20\_0x0014  Table 12: ENABLE STATE MACHINE: ENSM Config 1
\\
\hline
ensm\_config\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d21\_0x0015  Table 12: ENABLE STATE MACHINE: ENSM Config 2
\\
\hline
ensm\_calibration\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d22\_0x0016  Table 12: ENABLE STATE MACHINE: Calibration Control
\\
\hline
ensm\_state & uchar  & - & - & Volatile & -  &- & reg\_addr\_d23\_0x0017  Table 12: ENABLE STATE MACHINE: State
\\
\hline
auxdac\_1\_word & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d24\_0x0018  Table 15: AUXDAC: AuxDAC 1 Word
\\
\hline
auxdac\_2\_word & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d25\_0x0019  Table 15: AUXDAC: AuxDAC 2 Word
\\
\hline
auxdac\_1\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d26\_0x001a  Table 15: AUXDAC: AuxDAC 1 Config
\\
\hline
auxdac\_2\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d27\_0x001b  Table 15: AUXDAC: AuxDAC 2 Config
\\
\hline
auxadc\_clock\_divider & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d28\_0x001c  Table 17: AUXILARYADC: AuxADC Clock Divider
\\
\hline
auxadc\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d29\_0x001d  Table 17: AUXILARYADC: Aux ADC Config
\\
\hline
auxadc\_word\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d30\_0x001e  Table 17: AUXILARYADC: AuxADC Word MSB
\\
\hline
auxadc\_world\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d31\_0x001f  Table 17: AUXILARYADC: AuxADC Word LSB
\\
\hline
misc\_auto\_gpo & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d32\_0x0020  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: Auto GPO
\\
\hline
misc\_agc\_gain\_lock\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d33\_0x0021  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AGC Gain Lock Delay
\\
\hline
misc\_agc\_attack\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d34\_0x0022  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AGC Attack Delay
\\
\hline
misc\_auxdac\_enable\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d35\_0x0023  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AuxDAC Enable Control
\\
\hline
misc\_rx\_load\_synth\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d36\_0x0024  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: RX Load Synth Delay
\\
\hline
misc\_tx\_load\_synth\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d37\_0x0025  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: TX Load Synth Delay
\\
\hline
misc\_external\_lna\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d38\_0x0026  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: External LNA control
\\
\hline
misc\_gpo\_force\_and\_init & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d39\_0x0027  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO Force and Init
\\
\hline
misc\_gpo0\_rx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d40\_0x0028  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO0 Rx delay
\\
\hline
misc\_gpo1\_rx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d41\_0x0029  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO1 Rx delay
\\
\hline
misc\_gpo2\_rx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d42\_0x002a  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO2 Rx delay
\\
\hline
misc\_gpo3\_rx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d43\_0x002b  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO3 Rx delay
\\
\hline
misc\_gpo0\_tx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d44\_0x002c  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO0 Tx Delay
\\
\hline
misc\_gpo1\_tx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d45\_0x002d  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO1 Tx Delay
\\
\hline
misc\_gpo2\_tx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d46\_0x002e  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO2 Tx Delay
\\
\hline
misc\_gpo3\_tx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d47\_0x002f  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: GPO3 Tx Delay
\\
\hline
misc\_auxdac1\_rx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d48\_0x0030  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AuxDAC1 Rx Delay
\\
\hline
misc\_auxdac1\_tx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d49\_0x0031  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AuxDAC1 Tx Delay
\\
\hline
misc\_auxdac2\_rx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d50\_0x0032  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AuxDAC2 Rx Delay
\\
\hline
misc\_auxdac2\_tx\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d51\_0x0033  Table 18: GPO, AUXDAC, AGC DELAY, AND SYNTH DELOY CONTROL: AuxDAC2 Tx Delay
\\
\hline
ocpi\_pad\_034 & uchar  & - & - &  & -  &- & -
\\
\hline
ctrl\_output\_pointer & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d53\_0x0035  Table 19: CONTROL OUTPUT: Control Output Pointer
\\
\hline
ctrl\_output\_enable & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d54\_0x0036  Table 19: CONTROL OUTPUT: Control Output Enable
\\
\hline
product\_id & uchar  & - & - & Volatile & -  &- & reg\_addr\_d55\_0x0037  Table 20: PRODUCT ID: Product ID
\\
\hline
ocpi\_pad\_038 & uchar  & - & - &  & -  &- & -
\\
\hline
reference\_clock\_cycles & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d58\_0x003a  Table 22: REFERENCE CLOCK CYCLES: Reference Clock Cycles
\\
\hline
digital\_io\_digital\_io\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d59\_0x003b  Table 23: DIGITAL IO CONTROL: Digital I/O Control
\\
\hline
digital\_io\_lvds\_bias\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d60\_0x003c  Table 23: DIGITAL IO CONTROL: LVDS Bias control
\\
\hline
digital\_io\_lvds\_invert\_ctrl1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d61\_0x003d  Table 23: DIGITAL IO CONTROL: LVDS Invert control1
\\
\hline
digital\_io\_lvds\_invert\_ctrl2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d62\_0x003e  Table 23: DIGITAL IO CONTROL: LVDS Invert control2
\\
\hline
bbpll\_ctrl\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d63\_0x003f  Table 25: BBPLL CONTROL: BPLL Control 1
\\
\hline
bbpll\_mustbe0x00 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d64\_0x0040  Table 25: BBPLL CONTROL: Must be 0
\\
\hline
bbpll\_fract\_bb\_freq\_word\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d65\_0x0041  Table 25: BBPLL CONTROL: Fractional BB Freq Word 1
\\
\hline
bbpll\_fract\_bb\_freq\_word\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d66\_0x0042  Table 25: BBPLL CONTROL: Fractional BB Freq Word 2
\\
\hline
bbpll\_fract\_bb\_freq\_word\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d67\_0x0043  Table 25: BBPLL CONTROL: Fractional BB Freq Word 3
\\
\hline
bbpll\_integer\_bb\_freq\_word & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d68\_0x0044  Table 25: BBPLL CONTROL: Integer BB Freq Word
\\
\hline
bbpll\_ref\_clock\_scaler & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d69\_0x0045  Table 25: BBPLL CONTROL: Ref Clock Scaler
\\
\hline
bbpll\_cp\_current & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d70\_0x0046  Table 25: BBPLL CONTROL: CP Current
\\
\hline
bbpll\_msc\_scale & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d71\_0x0047  Table 25: BBPLL CONTROL: MSC Scale
\\
\hline
bbpll\_loop\_filter\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d72\_0x0048  Table 25: BBPLL CONTROL: Loop Filter 1
\\
\hline
bbpll\_loop\_filter\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d73\_0x0049  Table 25: BBPLL CONTROL: Loop Filter 2
\\
\hline
bbpll\_loop\_filter\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d74\_0x004a  Table 25: BBPLL CONTROL: Loop Filter 3
\\
\hline
bbpll\_vco\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d75\_0x004b  Table 25: BBPLL CONTROL: VCO Control
\\
\hline
bbpll\_mustbe0x86 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d76\_0x004c  Table 25: BBPLL CONTROL: Must be\_0x86
\\
\hline
bpll\_control\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d77\_0x004d  Table 25: BBPLL CONTROL: BPLL Control 2
\\
\hline
bpll\_control\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d78\_0x004e  Table 25: BBPLL CONTROL: BPLL Control 3
\\
\hline
ocpi\_pad\_04f & uchar  & - & - &  & -  &- & -
\\
\hline
power\_down\_override\_rx\_synth & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d80\_0x0050  Table 26: POWER DOWN OVERRIDE: Rx Synth Power Down Override
\\
\hline
power\_down\_override\_tx\_synth & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d81\_0x0051  Table 26: POWER DOWN OVERRIDE: TX Synth Power Down Override
\\
\hline
power\_down\_override\_rx\_control\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d82\_0x0052  Table 26: POWER DOWN OVERRIDE: Control 0
\\
\hline
power\_down\_override\_mustbe0x00 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d83\_0x0053  Table 26: POWER DOWN OVERRIDE: Must be 0
\\
\hline
power\_down\_override\_rx1\_adc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d84\_0x0054  Table 26: POWER DOWN OVERRIDE: Rx1 ADC Power Down Override
\\
\hline
power\_down\_override\_rx2\_adc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d85\_0x0055  Table 26: POWER DOWN OVERRIDE: Rx2 ADC Power Down Override
\\
\hline
power\_down\_override\_tx\_analog & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d86\_0x0056  Table 26: POWER DOWN OVERRIDE: Tx Analog Power Down Override 1
\\
\hline
power\_down\_override\_analog & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d87\_0x0057  Table 26: POWER DOWN OVERRIDE: Analog Power Down Override
\\
\hline
power\_down\_override\_misc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d88\_0x0058  Table 26: POWER DOWN OVERRIDE: Misc Power Down Override
\\
\hline
ocpi\_pad\_059 & uchar  & - & - &  & -  &- & -
\\
\hline
overflow\_ch\_1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d94\_0x005e  Table 27: OVERFLOW: CH 1 Overflow
\\
\hline
overflow\_ch\_2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d95\_0x005f  Table 27: OVERFLOW: CH 2 Overflow
\\
\hline
tx\_filter\_coef\_addr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d96\_0x0060  Table 28: Tx PROGRAMMABLE FIR FILTER: TX Filter Coefficient Address
\\
\hline
tx\_filter\_coef\_write\_data\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d97\_0x0061  Table 28: Tx PROGRAMMABLE FIR FILTER: TX Filter Coefficient Write Data 1
\\
\hline
tx\_filter\_coef\_write\_data\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d98\_0x0062  Table 28: Tx PROGRAMMABLE FIR FILTER: TX Filter Coefficient Write Data 2
\\
\hline
tx\_filter\_coef\_read\_data\_1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d99\_0x0063  Table 28: Tx PROGRAMMABLE FIR FILTER: TX Filter Coefficient Read Data 1
\\
\hline
tx\_filter\_coef\_read\_data\_2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d100\_0x0064 Table 28: Tx PROGRAMMABLE FIR FILTER: TX Filter Coefficient Read Data 2
\\
\hline
tx\_filter\_conf & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d101\_0x0065 Table 28: Tx PROGRAMMABLE FIR FILTER: TX Filter Configuration
\\
\hline
ocpi\_pad\_066 & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_mon\_low\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d103\_0x0067 Table 29: Tx MONITOR: Tx Mon Low Gain
\\
\hline
tx\_mon\_high\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d104\_0x0068 Table 29: Tx MONITOR: Tx Mon High Gain
\\
\hline
tx\_mon\_delay\_counter & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d105\_0x0069 Table 29: Tx MONITOR: Tx Mon Delay Counter
\\
\hline
tx\_mon\_level\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d106\_0x006a Table 29: Tx MONITOR: Tx Level Threshold
\\
\hline
tx\_mon\_rssi1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d107\_0x006b Table 29: Tx MONITOR: TX RSSI1
\\
\hline
tx\_mon\_rssi2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d108\_0x006c Table 29: Tx MONITOR: TX RSSI2
\\
\hline
tx\_mon\_rssi\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d109\_0x006d Table 29: Tx MONITOR: TX RSSI LSB
\\
\hline
tx\_mon\_tpm\_mode\_enable & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d110\_0x006e Table 29: Tx MONITOR: TPM Mode Enable
\\
\hline
tx\_mon\_temp\_gain\_coef & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d111\_0x006f Table 29: Tx MONITOR: Temp Gain Coefficient
\\
\hline
tx\_mon\_1\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d112\_0x0070 Table 29: Tx MONITOR: Tx Mon 1 Config
\\
\hline
tx\_mon\_2\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d113\_0x0071 Table 29: Tx MONITOR: Tx Mon 2 Config
\\
\hline
ocpi\_pad\_072 & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_pwr\_atten\_tx1\_atten\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d115\_0x0073 Table 31: Tx POWER CONTROL AND ATTENUATION: Tx1 Atten 0
\\
\hline
tx\_pwr\_atten\_tx1\_atten\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d116\_0x0074 Table 31: Tx POWER CONTROL AND ATTENUATION: Tx1 Atten 1
\\
\hline
tx\_pwr\_atten\_tx2\_atten\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d117\_0x0075 Table 31: Tx POWER CONTROL AND ATTENUATION: Tx2 Atten 0
\\
\hline
tx\_pwr\_atten\_tx2\_atten\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d118\_0x0076 Table 31: Tx POWER CONTROL AND ATTENUATION: Tx2 Atten 1
\\
\hline
tx\_pwr\_atten\_tx\_atten\_offset & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d119\_0x0077 Table 31: Tx POWER CONTROL AND ATTENUATION: Tx Atten Offset
\\
\hline
tx\_pwr\_atten\_tx\_atten\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d120\_0x0078 Table 31: Tx POWER CONTROL AND ATTENUATION: Tx Atten Threshold
\\
\hline
tx\_pwr\_atten\_set\_tx1\_tx2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d121\_0x0079 Table 31: Tx POWER CONTROL AND ATTENUATION: Set Tx1/Tx2
\\
\hline
ocpi\_pad\_07a & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_pwr\_atten\_immediate\_update & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d124\_0x007c Table 31: Tx POWER CONTROL AND ATTENUATION: Immediate Update
\\
\hline
ocpi\_pad\_07d & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_pgo\_phase\_corr\_tx1\_out1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d142\_0x008e Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 1 Phase Corr
\\
\hline
tx\_pgo\_gain\_corr\_tx1\_out1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d143\_0x008f Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 1 Gain Corr
\\
\hline
tx\_pgo\_phase\_corr\_tx2\_out1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d144\_0x0090 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 1 Phase Corr
\\
\hline
tx\_pgo\_gain\_corr\_tx2\_out1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d145\_0x0091 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 1 Gain Corr
\\
\hline
tx\_pgo\_offset\_corr\_tx1\_out1\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d146\_0x0092 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 1 Offset I
\\
\hline
tx\_pgo\_offset\_corr\_tx1\_out1\_q & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d147\_0x0093 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 1 Offset Q
\\
\hline
tx\_pgo\_offset\_corr\_tx2\_out1\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d148\_0x0094 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 1 Offset I
\\
\hline
tx\_pgo\_offset\_corr\_tx2\_out1\_q & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d149\_0x0095 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 1 Offset Q
\\
\hline
tx\_pgo\_phase\_corr\_tx1\_out2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d150\_0x0096 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 2 Phase Corr
\\
\hline
tx\_pgo\_gain\_corr\_tx1\_out2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d151\_0x0097 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 2 Gain Corr
\\
\hline
tx\_pgo\_phase\_corr\_tx2\_out2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d152\_0x0098 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 2 Phase Corr
\\
\hline
tx\_pgo\_gain\_corr\_tx2\_out2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d153\_0x0099 Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 2 Gain Corr
\\
\hline
tx\_pgo\_offset\_corr\_tx1\_out2\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d154\_0x009a Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 2 Offset I
\\
\hline
tx\_pgo\_offset\_corr\_tx1\_out2\_q & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d155\_0x009b Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx1 Out 2 Offset Q
\\
\hline
tx\_pgo\_offset\_corr\_tx2\_out2\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d156\_0x009c Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 2 Offset I
\\
\hline
tx\_pgo\_offset\_corr\_tx2\_out2\_q & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d157\_0x009d Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Tx2 Out 2 Offset Q
\\
\hline
ocpi\_pad\_09e & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_quad\_cal\_pgo\_force\_bits & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d159\_0x009f Table 32: Tx QUADRATURE CALIBRATION, PHASE, GAIN, AND OFFSET CORRECTION: Force Bits
\\
\hline
tx\_quad\_cal\_nco\_freq\_phase\_offset & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d160\_0x00a0 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Quad Cal NCO Freq \& Phase Offset
\\
\hline
tx\_quad\_cal\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d161\_0x00a1 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Quad Cal Control
\\
\hline
tx\_quad\_cal\_kexp\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d162\_0x00a2 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Kexp 1
\\
\hline
tx\_quad\_cal\_kexp\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d163\_0x00a3 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Kexp 2
\\
\hline
tx\_quad\_cal\_settle\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d164\_0x00a4 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: QUAD Settle count
\\
\hline
tx\_quad\_cal\_mag\_ftest\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d165\_0x00a5 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Mag. Ftest Thresh
\\
\hline
tx\_quad\_cal\_mag\_ftest\_thresh\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d166\_0x00a6 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Mag. Ftest Thresh 2
\\
\hline
tx\_quad\_cal\_status\_tx1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d167\_0x00a7 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Quad cal status Tx1
\\
\hline
tx\_quad\_cal\_status\_tx2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d168\_0x00a8 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Quad cal status Tx2
\\
\hline
tx\_quad\_cal\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d169\_0x00a9 Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Quad cal Count
\\
\hline
tx\_quad\_cal\_full\_lmt\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d170\_0x00aa Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Tx Quad Full/LMT Gain
\\
\hline
tx\_quad\_cal\_squarer\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d171\_0x00ab Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Squarer Config
\\
\hline
tx\_quad\_cal\_atten & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d172\_0x00ac Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: TX Quad Cal Atten
\\
\hline
tx\_quad\_cal\_thresh\_accum & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d173\_0x00ad Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Thresh Accum
\\
\hline
tx\_quad\_cal\_lpf\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d174\_0x00ae Table 33: Tx QUADRATE CALIBRATION CONFIGURATION: Tx Quad LPF Gain
\\
\hline
ocpi\_pad\_0af & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_bbf\_r1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d194\_0x00c2 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF R1
\\
\hline
tx\_bbf\_r2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d195\_0x00c3 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF R2
\\
\hline
tx\_bbf\_r3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d196\_0x00c4 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF R3
\\
\hline
tx\_bbf\_r4 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d197\_0x00c5 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF R4
\\
\hline
tx\_bbf\_rp & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d198\_0x00c6 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF RP
\\
\hline
tx\_bbf\_c1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d199\_0x00c7 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF C1
\\
\hline
tx\_bbf\_c2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d200\_0x00c8 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF C2
\\
\hline
tx\_bbf\_cp & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d201\_0x00c9 Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF CP
\\
\hline
tx\_bbf\_tuner\_pd & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d202\_0x00ca Table 34: Tx BASEBAND FILTER REGISTERS: Tx Tuner PD
\\
\hline
tx\_bbf\_r2b & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d203\_0x00cb Table 34: Tx BASEBAND FILTER REGISTERS: Tx BBF R2b
\\
\hline
ocpi\_pad\_0cc & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_secondf\_config0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d208\_0x00d0 Table 35: Tx SECONDARY FILTER REGISTERS: Config0
\\
\hline
tx\_secondf\_resistor & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d209\_0x00d1 Table 35: Tx SECONDARY FILTER REGISTERS: Resistor
\\
\hline
tx\_secondf\_capacitor & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d210\_0x00d2 Table 35: Tx SECONDARY FILTER REGISTERS: Capacitor
\\
\hline
tx\_secondf\_mustbe0x60 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d211\_0x00d3 Table 35: Tx SECONDARY FILTER REGISTERS: Must be 0x60
\\
\hline
ocpi\_pad\_0d4 & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_bbf\_tune\_divider & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d214\_0x00d6 Table 38: Tx BBF TUNER CONFIGURATION: TX BBF Tune Divider
\\
\hline
tx\_bbf\_tune\_mode & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d215\_0x00d7 Table 38: Tx BBF TUNER CONFIGURATION: TX BBF Tune Mode
\\
\hline
ocpi\_pad\_0d8 & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_filter\_coef\_addr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d240\_0x00f0 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Coeff Addr
\\
\hline
rx\_filter\_coef\_write\_data\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d241\_0x00f1 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Coeff Data 1
\\
\hline
rx\_filter\_coef\_write\_data\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d242\_0x00f2 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Coeff Data 2
\\
\hline
rx\_filter\_coef\_read\_data\_1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d243\_0x00f3 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Coeff Read Data 1
\\
\hline
rx\_filter\_coef\_read\_data\_2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d244\_0x00f4 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Coeff Read Data 2
\\
\hline
rx\_filter\_conf & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d245\_0x00f5 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Configuration
\\
\hline
rx\_filter\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d246\_0x00f6 Table 39: Rx PROGRAMMABLE FIR FILTER: Rx Filter Gain
\\
\hline
ocpi\_pad\_0f7 & uchar  & - & - &  & -  &- & -
\\
\hline
gain\_agc\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d250\_0x00fa Table 42: GAIN CONTROL SETUP: AGC Config1
\\
\hline
gain\_agc\_config\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d251\_0x00fb Table 42: GAIN CONTROL SETUP: AGC config2
\\
\hline
gain\_agc\_config\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d252\_0x00fc Table 42: GAIN CONTROL SETUP: AGC Config3
\\
\hline
gain\_max\_lmt\_full\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d253\_0x00fd Table 42: GAIN CONTROL SETUP: Max LMT/Full Gain
\\
\hline
gain\_peak\_wait\_time & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d254\_0x00fe Table 42: GAIN CONTROL SETUP: Peak Wait Time
\\
\hline
ocpi\_pad\_0ff & uchar  & - & - &  & -  &- & -
\\
\hline
gain\_digital\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d256\_0x0100 Table 42: GAIN CONTROL SETUP: Digital Gain
\\
\hline
gain\_agc\_lock\_level & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d257\_0x0101 Table 42: GAIN CONTROL SETUP: AGC Lock Level
\\
\hline
ocpi\_pad\_102 & uchar  & - & - &  & -  &- & -
\\
\hline
gain\_gain\_stp\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d259\_0x0103 Table 42: GAIN CONTROL SETUP: Gain Step Config 1
\\
\hline
gain\_adc\_small\_overload\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d260\_0x0104 Table 42: GAIN CONTROL SETUP: ADC Small Overload Threshold
\\
\hline
gain\_adc\_large\_overload\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d261\_0x0105 Table 42: GAIN CONTROL SETUP: ADC Large Overload Threshold
\\
\hline
gain\_stp\_config\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d262\_0x0106 Table 42: GAIN CONTROL SETUP: Gain Step Config 2
\\
\hline
gain\_small\_lmt\_overload\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d263\_0x0107 Table 42: GAIN CONTROL SETUP: Small LMT Overload Threshold
\\
\hline
gain\_large\_lmt\_overload\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d264\_0x0108 Table 42: GAIN CONTROL SETUP: Large LMT Overload Threshold
\\
\hline
gain\_rx1\_manual\_lmt\_full\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d265\_0x0109 Table 42: GAIN CONTROL SETUP: Rx1 Manual LMT/Full Gain
\\
\hline
gain\_rx1\_manual\_lpf\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d266\_0x010a Table 42: GAIN CONTROL SETUP: Rx1 Manual LPF gain
\\
\hline
gain\_rx1\_manual\_digitalforced\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d267\_0x010b Table 42: GAIN CONTROL SETUP: Rx1 Manual Digital/Forced Gain
\\
\hline
gain\_rx2\_manual\_lmt\_full\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d268\_0x010c Table 42: GAIN CONTROL SETUP: Rx2 Manual LMT/Full Gain
\\
\hline
gain\_rx2\_manual\_lpf\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d269\_0x010d Table 42: GAIN CONTROL SETUP: Rx2 Manual LPF Gain
\\
\hline
gain\_rx2\_manual\_digitalforced\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d270\_0x010e Table 42: GAIN CONTROL SETUP: Rx2 Manual Digital/Forced Gain
\\
\hline
ocpi\_pad\_10f & uchar  & - & - &  & -  &- & -
\\
\hline
fast\_agc\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d272\_0x0110 Table 44: FAST ATTACK AGC SETUP: Config 1
\\
\hline
fast\_agc\_config\_2\_settling\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d273\_0x0111 Table 44: FAST ATTACK AGC SETUP: Config 2 \& Settling Delay
\\
\hline
fast\_agc\_energy\_lost\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d274\_0x0112 Table 44: FAST ATTACK AGC SETUP: Energy Lost Threshold
\\
\hline
fast\_agc\_stronger\_signal\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d275\_0x0113 Table 44: FAST ATTACK AGC SETUP: Stronger Signal Threshold
\\
\hline
fast\_agc\_low\_power\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d276\_0x0114 Table 44: FAST ATTACK AGC SETUP: Low Power Threshold
\\
\hline
fast\_agc\_strong\_signal\_freeze & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d277\_0x0115 Table 44: FAST ATTACK AGC SETUP: Strong Signal Freeze
\\
\hline
fast\_agc\_final\_over\_range\_and\_opt\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d278\_0x0116 Table 44: FAST ATTACK AGC SETUP: Final Over Range and Opt Gain
\\
\hline
fast\_agc\_energy\_detect\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d279\_0x0117 Table 44: FAST ATTACK AGC SETUP: Energy Detect Count
\\
\hline
fast\_agc\_agcll\_upper\_limit & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d280\_0x0118 Table 44: FAST ATTACK AGC SETUP: AGCLL Upper Limit
\\
\hline
fast\_agc\_gain\_lock\_exit\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d281\_0x0119 Table 44: FAST ATTACK AGC SETUP: Gain Lock Exit Count
\\
\hline
fast\_agc\_initial\_lmt\_gain\_limit & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d282\_0x011a Table 44: FAST ATTACK AGC SETUP: Initial LMT Gain Limit
\\
\hline
fast\_agc\_increment\_time & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d283\_0x011b Table 44: FAST ATTACK AGC SETUP: Increment Time
\\
\hline
ocpi\_pad\_11c & uchar  & - & - &  & -  &- & -
\\
\hline
slowhybrid\_agc\_inner\_low\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d288\_0x0120 Table 45: SLOW ATTACK AND HYBRID AGC: AGC Inner Low Threshold
\\
\hline
slowhybrid\_agc\_lmt\_overload\_counters & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d289\_0x0121 Table 45: SLOW ATTACK AND HYBRID AGC: LMT Overload Counters
\\
\hline
slowhybrid\_agc\_adc\_overload\_counters & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d290\_0x0122 Table 45: SLOW ATTACK AND HYBRID AGC: ADC Overload Counters
\\
\hline
slowhybrid\_agc\_gain\_stp1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d291\_0x0123 Table 45: SLOW ATTACK AND HYBRID AGC: Gain Step1
\\
\hline
slowhybrid\_agc\_gain\_update\_counter1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d292\_0x0124 Table 45: SLOW ATTACK AND HYBRID AGC: Gain Update Counter1
\\
\hline
slowhybrid\_agc\_gain\_update\_counter2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d293\_0x0125 Table 45: SLOW ATTACK AND HYBRID AGC: Gain Update Counter2
\\
\hline
ocpi\_pad\_126 & uchar  & - & - &  & -  &- & -
\\
\hline
slowhybrid\_agc\_digital\_sat\_counter & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d296\_0x0128 Table 45: SLOW ATTACK AND HYBRID AGC: Digital Sat Counter
\\
\hline
slowhybrid\_agc\_outer\_power\_threshs & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d297\_0x0129 Table 45: SLOW ATTACK AND HYBRID AGC: Outer Power Thresholds
\\
\hline
slowhybrid\_agc\_gain\_stp\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d298\_0x012a Table 45: SLOW ATTACK AND HYBRID AGC: Gain Step 2
\\
\hline
ocpi\_pad\_12b & uchar  & - & - &  & -  &- & -
\\
\hline
ext\_lna\_high\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d300\_0x012c Table 46: EXTERNAL LNA GAIN WORD: Ext LNA High Gain
\\
\hline
ext\_lna\_low\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d301\_0x012d Table 46: EXTERNAL LNA GAIN WORD: Ext LNA Low Gain
\\
\hline
ocpi\_pad\_12e & uchar  & - & - &  & -  &- & -
\\
\hline
gain\_table & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d304\_0x0130 Table 47: AGC GAIN TABLE: Gain Table Address
\\
\hline
gain\_table\_write\_data1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d305\_0x0131 Table 47: AGC GAIN TABLE: Gain Table Write Data1
\\
\hline
gain\_table\_write\_data2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d306\_0x0132 Table 47: AGC GAIN TABLE: Gain Table Write Data2
\\
\hline
gain\_table\_write\_data3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d307\_0x0133 Table 47: AGC GAIN TABLE: Gain Table Write Data 3
\\
\hline
gain\_table\_read\_data1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d308\_0x0134 Table 47: AGC GAIN TABLE: Gain Table Read Data 1
\\
\hline
gain\_table\_read\_data2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d309\_0x0135 Table 47: AGC GAIN TABLE: Gain Table Read Data 2
\\
\hline
gain\_table\_read\_data3 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d310\_0x0136 Table 47: AGC GAIN TABLE: Gain Table Read Data 3
\\
\hline
gain\_table\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d311\_0x0137 Table 47: AGC GAIN TABLE: Gain Table Config
\\
\hline
mixer\_subtable & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d312\_0x0138 Table 48: MIXER SUBTABLE: Mixer Subtable Address
\\
\hline
mixer\_subtable\_gain\_write & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d313\_0x0139 Table 48: MIXER SUBTABLE: Mixer Subtable Gain Word Write
\\
\hline
mixer\_subtable\_bias\_write & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d314\_0x013a Table 48: MIXER SUBTABLE: Mixer Subtable Bias Word Write
\\
\hline
mixer\_subtable\_ctrl\_write & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d315\_0x013b Table 48: MIXER SUBTABLE: Mixer Subtable Control Word Write
\\
\hline
mixer\_subtable\_gain\_read & uchar  & - & - & Volatile & -  &- & reg\_addr\_d316\_0x013c Table 48: MIXER SUBTABLE: Mixer Subtable Gain Word Read
\\
\hline
mixer\_subtable\_bias\_read & uchar  & - & - & Volatile & -  &- & reg\_addr\_d317\_0x013d Table 48: MIXER SUBTABLE: Mixer Subtable Bias Word Read
\\
\hline
mixer\_subtable\_ctrl\_read & uchar  & - & - & Volatile & -  &- & reg\_addr\_d318\_0x013e Table 48: MIXER SUBTABLE: Mixer Subtable Control Word Read
\\
\hline
mixer\_subtable\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d319\_0x013f Table 48: MIXER SUBTABLE: Mixer Subtable Config
\\
\hline
calib\_gain\_table\_word & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d320\_0x0140 Table 49: CALIBRATION GAIN TABLE: Word\_Address
\\
\hline
calib\_gain\_table\_diff\_worderror\_write & uchar  & - & - & Writeable & -  &- & reg\_addr\_d321\_0x0141 Table 49: CALIBRATION GAIN TABLE: Gain Diff Word/Error Write
\\
\hline
calib\_gain\_table\_gain\_error\_read & uchar  & - & - & Volatile & -  &- & reg\_addr\_d322\_0x0142 Table 49: CALIBRATION GAIN TABLE: Gain Error Read
\\
\hline
calib\_gain\_table\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d323\_0x0143 Table 49: CALIBRATION GAIN TABLE: Config
\\
\hline
calib\_gain\_table\_lna\_diff\_read\_back & uchar  & - & - & Volatile & -  &- & reg\_addr\_d324\_0x0144 Table 49: CALIBRATION GAIN TABLE: LNA Gain Diff Read Back
\\
\hline
gen\_calib\_max\_mixer\_gain\_index & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d325\_0x0145 Table 50: GENERAL CALIBRATION: Max Mixer Calibration Gain Index
\\
\hline
gen\_calib\_temp\_gain\_coef & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d326\_0x0146 Table 50: GENERAL CALIBRATION: Temp Gain Coefficient
\\
\hline
gen\_calib\_settle\_time & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d327\_0x0147 Table 50: GENERAL CALIBRATION: Settle Time
\\
\hline
gen\_calib\_measure\_duration & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d328\_0x0148 Table 50: GENERAL CALIBRATION: Measure Duration
\\
\hline
gen\_calib\_cal\_temp\_sensor\_word & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d329\_0x0149 Table 50: GENERAL CALIBRATION: Cal Temp sensor word
\\
\hline
ocpi\_pad\_14a & uchar  & - & - &  & -  &- & -
\\
\hline
rssi\_measure\_duration\_01 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d336\_0x0150 Table 51: RSSI MEASUREMENT CONFIGURATION: Measure Duration 0,1
\\
\hline
rssi\_measure\_duration\_23 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d337\_0x0151 Table 51: RSSI MEASUREMENT CONFIGURATION: Measure Duration 2,3
\\
\hline
rssi\_weight\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d338\_0x0152 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI Weight 0
\\
\hline
rssi\_weight\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d339\_0x0153 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI Weight 1
\\
\hline
rssi\_weight\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d340\_0x0154 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI Weight 2
\\
\hline
rssi\_weight\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d341\_0x0155 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI Weight 3
\\
\hline
rssi\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d342\_0x0156 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI delay
\\
\hline
rssi\_wait\_time & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d343\_0x0157 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI wait time
\\
\hline
rssi\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d344\_0x0158 Table 51: RSSI MEASUREMENT CONFIGURATION: RSSI Config
\\
\hline
ocpi\_pad\_159 & uchar  & - & - &  & -  &- & -
\\
\hline
rssi\_dec\_power\_duration\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d348\_0x015c Table 51: RSSI MEASUREMENT CONFIGURATION: Dec Power Duration
\\
\hline
rssi\_lna\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d349\_0x015d Table 51: RSSI MEASUREMENT CONFIGURATION: LNA Gain
\\
\hline
ocpi\_pad\_15e & uchar  & - & - &  & -  &- & -
\\
\hline
power\_ch1\_rx\_filter\_power & uchar  & - & - & Volatile & -  &- & reg\_addr\_d353\_0x0161 Table 53: POWER WORD: CH1 Rx filter Power
\\
\hline
ocpi\_pad\_162 & uchar  & - & - &  & -  &- & -
\\
\hline
power\_ch2\_rx\_filter\_power & uchar  & - & - & Volatile & -  &- & reg\_addr\_d355\_0x0163 Table 53: POWER WORD: CH2 Rx filter Power
\\
\hline
ocpi\_pad\_164 & uchar  & - & - &  & -  &- & -
\\
\hline
calibration\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d361\_0x0169 Table 54: Rx QUADRATURE CALIBRATION: Calibration Config 1
\\
\hline
calibration\_mustbe0x75 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d362\_0x016a Table 54: Rx QUADRATURE CALIBRATION: Must be 0x75
\\
\hline
calibration\_mustbe0x95 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d363\_0x016b Table 54: Rx QUADRATURE CALIBRATION: Must be 0x95
\\
\hline
ocpi\_pad\_16c & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_pgo\_phase\_corr\_rx1\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d368\_0x0170 Table 55: Rx PHASE AND GAIN CORRECTION: Rx1A Phase Corr
\\
\hline
rx\_pgo\_gain\_corr\_rx1\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d369\_0x0171 Table 55: Rx PHASE AND GAIN CORRECTION: Rx1A Gain Corr
\\
\hline
rx\_pgo\_phase\_corr\_rx2\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d370\_0x0172 Table 55: Rx PHASE AND GAIN CORRECTION: Rx2A Phase Corr
\\
\hline
rx\_pgo\_gain\_corr\_rx2\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d371\_0x0173 Table 55: Rx PHASE AND GAIN CORRECTION: Rx2A Gain Corr
\\
\hline
rx\_pgo\_offset\_corr\_rx1\_ina\_q & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d372\_0x0174 Table 55: Rx PHASE AND GAIN CORRECTION: Rx1A Q Offset
\\
\hline
rx\_pgo\_offset\_corr\_rx1\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d373\_0x0175 Table 55: Rx PHASE AND GAIN CORRECTION: Rx1A Offset
\\
\hline
rx\_pgo\_offset\_corr\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d374\_0x0176 Table 55: Rx PHASE AND GAIN CORRECTION: Input A Offsets
\\
\hline
rx\_pgo\_offset\_corr\_rx2\_ina & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d375\_0x0177 Table 55: Rx PHASE AND GAIN CORRECTION: Rx2A Offset
\\
\hline
rx\_pgo\_offset\_corr\_rx2\_ina\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d376\_0x0178 Table 55: Rx PHASE AND GAIN CORRECTION: Rx2A I Offset
\\
\hline
rx\_pgo\_phase\_corr\_rx1\_inbc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d377\_0x0179 Table 55: Rx PHASE AND GAIN CORRECTION: Rx1B/C Phase Corr
\\
\hline
rx\_pgo\_gain\_corr\_rx1\_inbc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d378\_0x017a Table 55: Rx PHASE AND GAIN CORRECTION: Rx1B/C Gain Corr
\\
\hline
rx\_pgo\_phase\_corr\_rx2\_inbc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d379\_0x017b Table 55: Rx PHASE AND GAIN CORRECTION: Rx2B/C Phase Corr
\\
\hline
rx\_pgo\_gain\_corr\_rx2\_inbc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d380\_0x017c Table 55: Rx PHASE AND GAIN CORRECTION: Rx2B/C Gain Corr
\\
\hline
rx\_pgo\_offset\_corr\_rx1\_inbc\_q & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d381\_0x017d Table 55: Rx PHASE AND GAIN CORRECTION: Rx1B/C Q Offset
\\
\hline
rx\_pgo\_offset\_corr\_rx1\_inbc\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d382\_0x017e Table 55: Rx PHASE AND GAIN CORRECTION: Rx1B/C I Offset
\\
\hline
rx\_pgo\_offset\_corr\_inpbc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d383\_0x017f Table 55: Rx PHASE AND GAIN CORRECTION: Input B/C Offsets
\\
\hline
rx\_pgo\_offset\_corr\_rx2\_inbc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d384\_0x0180 Table 55: Rx PHASE AND GAIN CORRECTION: Rx2 B/C Offset
\\
\hline
rx\_pgo\_offset\_corr\_rx2\_inbc\_i & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d385\_0x0181 Table 55: Rx PHASE AND GAIN CORRECTION: Rx2 B/C I Offset
\\
\hline
rx\_pgo\_force\_bits & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d386\_0x0182 Table 55: Rx PHASE AND GAIN CORRECTION: Force Bits
\\
\hline
ocpi\_pad\_183 & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_dc\_offset\_wait\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d389\_0x0185 Table 56: Rx DC OFFSET CONTROL: Wait Count
\\
\hline
rx\_dc\_offset\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d390\_0x0186 Table 56: Rx DC OFFSET CONTROL: RF DC Offset Count
\\
\hline
rx\_dc\_offset\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d391\_0x0187 Table 56: Rx DC OFFSET CONTROL: RF DC Offset Config 1
\\
\hline
rx\_dc\_offset\_atten & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d392\_0x0188 Table 56: Rx DC OFFSET CONTROL: RF DC Offset Attenuation
\\
\hline
rx\_dc\_offset\_mustbe0x30 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d393\_0x0189 Table 56: Rx DC OFFSET CONTROL: Must be 0x30
\\
\hline
ocpi\_pad\_18a & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_dc\_offset\_config2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d395\_0x018b Table 56: Rx DC OFFSET CONTROL: DC Offset Config2
\\
\hline
rx\_dc\_offset\_rf\_cal\_gain\_index & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d396\_0x018c Table 56: Rx DC OFFSET CONTROL: RF Cal Gain Index
\\
\hline
rx\_dc\_offset\_soi\_thresh & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d397\_0x018d Table 56: Rx DC OFFSET CONTROL: SOI Threshold
\\
\hline
ocpi\_pad\_18e & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_dc\_offset\_bb\_shift & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d400\_0x0190 Table 56: Rx DC OFFSET CONTROL: BB DC Offset Shift
\\
\hline
rx\_dc\_offset\_bb\_fast\_settle\_shift & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d401\_0x0191 Table 56: Rx DC OFFSET CONTROL: BB DC Offset Fast Settle Shift
\\
\hline
rx\_dc\_offset\_bb\_fast\_settle\_dur & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d402\_0x0192 Table 56: Rx DC OFFSET CONTROL: BB Fast Settle Dur
\\
\hline
rx\_dc\_offset\_bb\_count & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d403\_0x0193 Table 56: Rx DC OFFSET CONTROL: BB DC Offset Count
\\
\hline
rx\_dc\_offset\_bb\_atten & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d404\_0x0194 Table 56: Rx DC OFFSET CONTROL: BB DC Offset Attenuation
\\
\hline
ocpi\_pad\_195 & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_bb\_dc\_offset\_rx1\_word\_i\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d410\_0x019a Table 60: Rx BB DC OFFSET: RX1 BB DC word I MSB
\\
\hline
rx\_bb\_dc\_offset\_rx1\_word\_i\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d411\_0x019b Table 60: Rx BB DC OFFSET: RX1 BB DC word I LSB
\\
\hline
rx\_bb\_dc\_offset\_rx1\_word\_q\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d412\_0x019c Table 60: Rx BB DC OFFSET: RX1 BB DC word Q MSB
\\
\hline
rx\_bb\_dc\_offset\_rx1\_word\_q\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d413\_0x019d Table 60: Rx BB DC OFFSET: RX1 BB DC word Q LSB
\\
\hline
rx\_bb\_dc\_offset\_rx2\_word\_i\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d414\_0x019e Table 60: Rx BB DC OFFSET: RX2 BB DC word I MSB
\\
\hline
rx\_bb\_dc\_offset\_rx2\_word\_i\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d415\_0x019f Table 60: Rx BB DC OFFSET: RX2 BB DC word I LSB
\\
\hline
rx\_bb\_dc\_offset\_rx2\_word\_q\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d416\_0x01a0 Table 60: Rx BB DC OFFSET: RX2 BB DC word Q MSB
\\
\hline
rx\_bb\_dc\_offset\_rx2\_word\_q\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d417\_0x01a1 Table 60: Rx BB DC OFFSET: RX2 BB DC word Q LSB
\\
\hline
rx\_bb\_dc\_offset\_track\_corr\_word\_i\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d418\_0x01a2 Table 60: Rx BB DC OFFSET: BB Track corr word I MSB
\\
\hline
rx\_bb\_dc\_offset\_track\_corr\_word\_i\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d419\_0x01a3 Table 60: Rx BB DC OFFSET: BB Track corr word I LSB
\\
\hline
rx\_bb\_dc\_offset\_track\_corr\_word\_q\_msb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d420\_0x01a4 Table 60: Rx BB DC OFFSET: BB Track corr word Q MSB
\\
\hline
rx\_bb\_dc\_offset\_track\_corr\_word\_q\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d421\_0x01a5 Table 60: Rx BB DC OFFSET: BB Track corr word Q LSB
\\
\hline
ocpi\_pad\_1a6 & uchar  & - & - &  & -  &- & -
\\
\hline
rssi\_readback\_rx1\_symbol & uchar  & - & - & Volatile & -  &- & reg\_addr\_d423\_0x01a7 Table 61: RSSI READBACK: Rx1 RSSI Symbol
\\
\hline
rssi\_readback\_rx1\_preamble & uchar  & - & - & Volatile & -  &- & reg\_addr\_d424\_0x01a8 Table 61: RSSI READBACK: Rx1 RSSI preamble
\\
\hline
rssi\_readback\_rx2\_symbol & uchar  & - & - & Volatile & -  &- & reg\_addr\_d425\_0x01a9 Table 61: RSSI READBACK: Rx2 RSSI symbol
\\
\hline
rssi\_readback\_rx2\_preamble & uchar  & - & - & Volatile & -  &- & reg\_addr\_d426\_0x01aa Table 61: RSSI READBACK: Rx2 RSSI preamble
\\
\hline
rssi\_readback\_symbol\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d427\_0x01ab Table 61: RSSI READBACK: Symbol LSB
\\
\hline
rssi\_readback\_preamble\_lsb & uchar  & - & - & Volatile & -  &- & reg\_addr\_d428\_0x01ac Table 61: RSSI READBACK: Preamble LSB
\\
\hline
ocpi\_pad\_1ad & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_tia\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d475\_0x01db Table 62: Rx TIA: Rx TIA Config
\\
\hline
rx\_tia\_1\_c\_lsb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d476\_0x01dc Table 62: Rx TIA: TIA1 C LSB
\\
\hline
rx\_tia\_1\_c\_msb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d477\_0x01dd Table 62: Rx TIA: TIA1 C MSB
\\
\hline
rx\_tia\_2\_c\_lsb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d478\_0x01de Table 62: Rx TIA: TIA2 C LSB
\\
\hline
rx\_tia\_2\_c\_msb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d479\_0x01df Table 62: Rx TIA: TIA2 C MSB
\\
\hline
rx\_bbf\_rx1\_r1a & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d480\_0x01e0 Table 65: Rx BFF: Rx1 BBF R1A
\\
\hline
rx\_bbf\_rx2\_r1a & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d481\_0x01e1 Table 65: Rx BFF: Rx2 BBF R1A
\\
\hline
rx\_bff\_rx1\_tune\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d482\_0x01e2 Table 65: Rx BFF: Rx1 Tune Control
\\
\hline
rx\_bff\_rx2\_tune\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d483\_0x01e3 Table 65: Rx BFF: Rx2 Tune Control
\\
\hline
rx\_bff\_rx1\_bbf\_r5 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d484\_0x01e4 Table 65: Rx BFF: Rx1 BBF R5
\\
\hline
rx\_bff\_rx2\_bbf\_r5 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d485\_0x01e5 Table 65: Rx BFF: Rx2 BBF R5
\\
\hline
rx\_bbf\_r2346 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d486\_0x01e6 Table 65: Rx BFF: Rx BBF R2346
\\
\hline
rx\_bbf\_c1\_msb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d487\_0x01e7 Table 65: Rx BFF: Rx BBF C1 MSB
\\
\hline
rx\_bbf\_c1\_lsb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d488\_0x01e8 Table 65: Rx BFF: Rx BBF C1 LSB
\\
\hline
rx\_bbf\_c2\_msb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d489\_0x01e9 Table 65: Rx BFF: Rx BBF C2 MSB
\\
\hline
rx\_bbf\_c2\_lsb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d490\_0x01ea Table 65: Rx BFF: Rx BBF C2 LSB
\\
\hline
rx\_bbf\_c3\_msb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d491\_0x01eb Table 65: Rx BFF: Rx BBF C3 MSB
\\
\hline
rx\_bbf\_c3\_lsb & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d492\_0x01ec Table 65: Rx BFF: Rx BBF C3 LSB
\\
\hline
rx\_bbf\_cc1\_ctr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d493\_0x01ed Table 65: Rx BFF: Rx BBF CC1 Ctr
\\
\hline
rx\_bbf\_mustbe0x60 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d494\_0x01ee Table 65: Rx BFF: Must be 0x60
\\
\hline
rx\_bbf\_cc2\_ctr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d495\_0x01ef Table 65: Rx BFF: Rx BBF CC2 Ctr
\\
\hline
rx\_bbf\_pow\_rz\_byte1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d496\_0x01f0 Table 65: Rx BFF: Rx BBF Pow Rz Byte1
\\
\hline
rx\_bbf\_cc3\_ctr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d497\_0x01f1 Table 65: Rx BFF: Rx BBF CC3 Ctr
\\
\hline
rx\_bbf\_r5\_tune & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d498\_0x01f2 Table 65: Rx BFF: Rx BBF R5 Tune
\\
\hline
rx\_bbf\_tune & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d499\_0x01f3 Table 65: Rx BFF: Rx BBF Tune
\\
\hline
rx\_bff\_rx1\_bbf\_man\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d500\_0x01f4 Table 65: Rx BFF: Rx1 BBF Man Gain
\\
\hline
rx\_bff\_rx2\_bbf\_man\_gain & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d501\_0x01f5 Table 65: Rx BFF: Rx2 BBF Man Gain
\\
\hline
ocpi\_pad\_1f6 & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_bbf\_tune\_config\_divide & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d504\_0x01f8 Table 66: Rx BBF TUNER CONFIGURATION: RX BBF Tune Divide
\\
\hline
rx\_bbf\_tune\_config\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d505\_0x01f9 Table 66: Rx BBF TUNER CONFIGURATION: RX BBF Tune Config
\\
\hline
rx\_bbf\_tune\_config\_mustbe0x01 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d506\_0x01fa Table 66: Rx BBF TUNER CONFIGURATION: Must be 0x01
\\
\hline
rx\_bbf\_tune\_config\_rx\_bbbw\_mhz & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d507\_0x01fb Table 66: Rx BBF TUNER CONFIGURATION: Rx BBBW MHz
\\
\hline
rx\_bbf\_tune\_config\_rx\_bbbw\_khz & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d508\_0x01fc Table 66: Rx BBF TUNER CONFIGURATION: Rx BBBW kHz
\\
\hline
ocpi\_pad\_1fd & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_synth\_disable\_vco\_cal & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d560\_0x0230 Table 67: Rx SYNTHESIZER: Disable VCO Cal
\\
\hline
rx\_synth\_integer\_byte\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d561\_0x0231 Table 67: Rx SYNTHESIZER: RX Integer Byte 0
\\
\hline
rx\_synth\_integer\_byte\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d562\_0x0232 Table 67: Rx SYNTHESIZER: RX Integer Byte 1
\\
\hline
rx\_synth\_fract\_byte\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d563\_0x0233 Table 67: Rx SYNTHESIZER: RX Fractional Byte 0
\\
\hline
rx\_synth\_fract\_byte\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d564\_0x0234 Table 67: Rx SYNTHESIZER: RX Fractional Byte 1
\\
\hline
rx\_synth\_fract\_byte\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d565\_0x0235 Table 67: Rx SYNTHESIZER: RX Fractional Byte 2
\\
\hline
rx\_synth\_force\_alc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d566\_0x0236 Table 67: Rx SYNTHESIZER: RX Force ALC
\\
\hline
rx\_synth\_force\_vco\_tune\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d567\_0x0237 Table 67: Rx SYNTHESIZER: RX Force VCO Tune 0
\\
\hline
rx\_synth\_force\_vco\_tune\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d568\_0x0238 Table 67: Rx SYNTHESIZER: RX Force VCO Tune 1
\\
\hline
rx\_synth\_alc\_varactor & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d569\_0x0239 Table 67: Rx SYNTHESIZER: RX ALC/Varactor
\\
\hline
rx\_synth\_vco\_output & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d570\_0x023a Table 67: Rx SYNTHESIZER: RX VCO Output
\\
\hline
rx\_synth\_cp\_current & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d571\_0x023b Table 67: Rx SYNTHESIZER: RX CP Current
\\
\hline
rx\_synth\_cp\_offset & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d572\_0x023c Table 67: Rx SYNTHESIZER: RX CP Offset
\\
\hline
rx\_synth\_cp\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d573\_0x023d Table 67: Rx SYNTHESIZER: RX CP Config
\\
\hline
rx\_synth\_loop\_filter\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d574\_0x023e Table 67: Rx SYNTHESIZER: RX Loop Filter 1
\\
\hline
rx\_synth\_loop\_filter\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d575\_0x023f Table 67: Rx SYNTHESIZER: RX Loop Filter 2
\\
\hline
rx\_synth\_loop\_filter\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d576\_0x0240 Table 67: Rx SYNTHESIZER: RX Loop Filter 3
\\
\hline
rx\_synth\_dithercp\_cal & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d577\_0x0241 Table 67: Rx SYNTHESIZER: RX Dither/CP Cal
\\
\hline
rx\_synth\_vco\_bias\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d578\_0x0242 Table 67: Rx SYNTHESIZER: RX VCO Bias 1
\\
\hline
rx\_synth\_mustbe0x0d & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d579\_0x0243 Table 67: Rx SYNTHESIZER: Must be 0x0D
\\
\hline
rx\_synth\_cal\_status & uchar  & - & - & Volatile & -  &- & reg\_addr\_d580\_0x0244 Table 67: Rx SYNTHESIZER: RX Cal Status
\\
\hline
rx\_synth\_mustbe0x00 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d581\_0x0245 Table 67: Rx SYNTHESIZER: Must be 0x00
\\
\hline
rx\_synth\_mustbe0x02 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d582\_0x0246 Table 67: Rx SYNTHESIZER: Set to 0x02 (Must be 0x02)
\\
\hline
rx\_synth\_cp\_ovrg\_vco\_lock & uchar  & - & - & Volatile & -  &- & reg\_addr\_d583\_0x0247 Table 67: Rx SYNTHESIZER: RX CP Ovrg/VCO Lock
\\
\hline
rx\_synth\_mustbe0x0b & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d584\_0x0248 Table 67: Rx SYNTHESIZER: Set to 0x0B (Must be 0x0B)
\\
\hline
rx\_synth\_vco\_cal & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d585\_0x0249 Table 67: Rx SYNTHESIZER: RX VCO Cal
\\
\hline
rx\_synth\_lock\_detect\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d586\_0x024a Table 67: Rx SYNTHESIZER: RX Lock Detect Config
\\
\hline
rx\_synth\_mustbe0x17 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d587\_0x024b Table 67: Rx SYNTHESIZER: Must be 0x17
\\
\hline
rx\_synth\_mustbe0x00\_also & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d588\_0x024c Table 67: Rx SYNTHESIZER: Must be 0x00
\\
\hline
rx\_synth\_mustbe0x00\_also\_also & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d589\_0x024d Table 67: Rx SYNTHESIZER: Must be 0x00
\\
\hline
ocpi\_pad\_24e & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_synth\_must\_be\_0x70 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d592\_0x0250 Table 67: Rx SYNTHESIZER: Set to 0x70 (Must be 0x70)
\\
\hline
rx\_synth\_vco\_varactor\_ctrl\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d593\_0x0251 Table 67: Rx SYNTHESIZER: RX VCO Varactor Control 1
\\
\hline
ocpi\_pad\_252 & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_fast\_lock\_setup & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d602\_0x025a Table 71: Rx FAST LOCK: Rx Fast Lock Setup
\\
\hline
rx\_fast\_lock\_setup\_init\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d603\_0x025b Table 71: Rx FAST LOCK: Rx Fast Lock Setup Init Delay
\\
\hline
rx\_fast\_lock\_program\_addr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d604\_0x025c Table 71: Rx FAST LOCK: Rx Fast Lock Program Address
\\
\hline
rx\_fast\_lock\_program\_data & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d605\_0x025d Table 71: Rx FAST LOCK: Rx Fast Lock Program Data
\\
\hline
rx\_fast\_lock\_program\_read & uchar  & - & - & Volatile & -  &- & reg\_addr\_d606\_0x025e Table 71: Rx FAST LOCK: Rx Fast Lock Program Read
\\
\hline
rx\_fast\_lock\_program\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d607\_0x025f Table 71: Rx FAST LOCK: Rx Fast Lock Program Control
\\
\hline
ocpi\_pad\_260 & uchar  & - & - &  & -  &- & -
\\
\hline
rx\_lo\_gen\_power\_mode & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d609\_0x0261 Table 72: Rx LO GENERATION: Rx LO Gen Power Mode
\\
\hline
ocpi\_pad\_262 & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_synth\_disable\_vco\_cal & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d624\_0x0270 Table 73: Tx SYNTHESIZER: Disable VCO Cal
\\
\hline
tx\_synth\_integer\_byte\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d625\_0x0271 Table 73: Tx SYNTHESIZER: Integer Byte 0
\\
\hline
tx\_synth\_integer\_byte\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d626\_0x0272 Table 73: Tx SYNTHESIZER: Integer Byte 1
\\
\hline
tx\_synth\_fract\_byte\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d627\_0x0273 Table 73: Tx SYNTHESIZER: Fractional Byte 0
\\
\hline
tx\_synth\_fract\_byte\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d628\_0x0274 Table 73: Tx SYNTHESIZER: Fractional Byte 1
\\
\hline
tx\_synth\_fract\_byte\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d629\_0x0275 Table 73: Tx SYNTHESIZER: Fractional Byte 2
\\
\hline
tx\_synth\_force\_alc & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d630\_0x0276 Table 73: Tx SYNTHESIZER: Force ALC
\\
\hline
tx\_synth\_force\_vco\_tune\_0 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d631\_0x0277 Table 73: Tx SYNTHESIZER: Force VCO Tune 0
\\
\hline
tx\_synth\_force\_vco\_tune\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d632\_0x0278 Table 73: Tx SYNTHESIZER: Force VCO Tune 1
\\
\hline
tx\_synth\_alcvaract\_or & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d633\_0x0279 Table 73: Tx SYNTHESIZER: ALC/Varactor
\\
\hline
tx\_synth\_vco\_output & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d634\_0x027a Table 73: Tx SYNTHESIZER: VCO Output
\\
\hline
tx\_synth\_cp\_current & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d635\_0x027b Table 73: Tx SYNTHESIZER: CP Current
\\
\hline
tx\_synth\_cp\_offset & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d636\_0x027c Table 73: Tx SYNTHESIZER: CP Offset
\\
\hline
tx\_synth\_cp\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d637\_0x027d Table 73: Tx SYNTHESIZER: CP Config
\\
\hline
tx\_synth\_loop\_filter\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d638\_0x027e Table 73: Tx SYNTHESIZER: Loop Filter 1
\\
\hline
tx\_synth\_loop\_filter\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d639\_0x027f Table 73: Tx SYNTHESIZER: Loop Filter 2
\\
\hline
tx\_synth\_loop\_filter\_3 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d640\_0x0280 Table 73: Tx SYNTHESIZER: Loop Filter 3
\\
\hline
tx\_synth\_dithercp\_cal & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d641\_0x0281 Table 73: Tx SYNTHESIZER: Dither/CP Cal
\\
\hline
tx\_synth\_vco\_bias\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d642\_0x0282 Table 73: Tx SYNTHESIZER: VCO Bias 1
\\
\hline
tx\_synth\_mustbe0x0d & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d643\_0x0283 Table 73: Tx SYNTHESIZER: Must be 0x0D
\\
\hline
tx\_synth\_cal\_status & uchar  & - & - & Volatile & -  &- & reg\_addr\_d644\_0x0284 Table 73: Tx SYNTHESIZER: Cal Status
\\
\hline
tx\_synth\_mustbe0x00 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d645\_0x0285 Table 73: Tx SYNTHESIZER: Must be 0x00
\\
\hline
tx\_synth\_mustbe0x02 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d646\_0x0286 Table 73: Tx SYNTHESIZER: Set to 0x02 (Must be 0x02)
\\
\hline
tx\_synth\_cp\_overrange\_vco\_lock & uchar  & - & - & Volatile & -  &- & reg\_addr\_d647\_0x0287 Table 73: Tx SYNTHESIZER: CP Over Range/VCO Lock
\\
\hline
tx\_synth\_mustbe0x0b & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d648\_0x0288 Table 73: Tx SYNTHESIZER: Set to 0x0B (Must be 0x0B)
\\
\hline
tx\_synth\_vco\_cal & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d649\_0x0289 Table 73: Tx SYNTHESIZER: VCO Cal
\\
\hline
tx\_synth\_lock\_detect\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d650\_0x028a Table 73: Tx SYNTEHSIZER: Lock Detect Config
\\
\hline
tx\_synth\_mustbe0x17 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d651\_0x028b Table 73: Tx SYNTEHSIZER: Must be 0x17
\\
\hline
tx\_synth\_mustbe0x00\_also & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d652\_0x028c Table 73: Tx SYNTEHSIZER: Must be 0x00
\\
\hline
tx\_synth\_mustbe0x00\_also\_also & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d653\_0x028d Table 73: Tx SYNTEHSIZER: Must be 0x00
\\
\hline
ocpi\_pad\_28e & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_synth\_mustbe0x70 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d656\_0x0290 Table 73: Tx SYNTEHSIZER: Set to 0x70 (Must be 0x70)
\\
\hline
tx\_synth\_vco\_varactor\_ctrl\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d657\_0x0291 Table 73: Tx SYNTEHSIZER: VCO Varactor Control 1
\\
\hline
dcxo\_coarse\_tune & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d658\_0x0292 Table 74: DCXO: DCXO Coarse Tune
\\
\hline
dcxo\_fine\_tune\_high & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d659\_0x0293 Table 74: DCXO: DCXO Fine Tune2
\\
\hline
dcxo\_fine\_tune\_low & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d660\_0x0294 Table 74: DCXO: DCXO Fine Tune1
\\
\hline
ocpi\_pad\_295 & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_fast\_lock\_setup & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d666\_0x029a Table 65: Tx SYNTH FAST LOCK: Tx Fast Lock Setup
\\
\hline
tx\_fast\_lock\_setup\_init\_delay & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d667\_0x029b Table 65: Tx SYNTH FAST LOCK: Tx Fast Lock Setup Init Delay
\\
\hline
tx\_fast\_lock\_program\_addr & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d668\_0x029c Table 65: Tx SYNTH FAST LOCK: Tx Fast Lock Program Addr
\\
\hline
tx\_fast\_lock\_program\_data & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d669\_0x029d Table 65: Tx SYNTH FAST LOCK: Tx Fast Lock Program Data
\\
\hline
tx\_fast\_lock\_program\_read & uchar  & - & - & Volatile & -  &- & reg\_addr\_d670\_0x029e Table 65: Tx SYNTH FAST LOCK: Tx Fast Lock Program Read
\\
\hline
tx\_fast\_lock\_program\_ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d671\_0x029f Table 65: Tx SYNTH FAST LOCK: Tx Fast Lock Program Ctrl
\\
\hline
ocpi\_pad\_2a0 & uchar  & - & - &  & -  &- & -
\\
\hline
tx\_lo\_gen\_power\_mode & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d673\_0x02a1 Table 76: Tx LO GENERATION: Tx LO Gen Power Mode
\\
\hline
ocpi\_pad\_2a2 & uchar  & - & - &  & -  &- & -
\\
\hline
bandgap\_mustbe0x0e & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d678\_0x02a6 Table 77: MASTER BIAS AND BANDGAP CONFIGURATION: Set to 0x0E (Must be 0x0E)
\\
\hline
ocpi\_pad\_2a7 & uchar  & - & - &  & -  &- & -
\\
\hline
bandgap\_mustbe0x0e\_also & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d680\_0x02a8 Table 77: MASTER BIAS AND BANDGAP CONFIGURATION: Set to 0x0E (Must be 0x0E)
\\
\hline
ocpi\_pad\_2a9 & uchar  & - & - &  & -  &- & -
\\
\hline
ref\_divide\_config\_1 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d683\_0x02ab Table 78: REFERENCE DIVIDER: Ref Divide Config 1
\\
\hline
ref\_divide\_config\_2 & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d684\_0x02ac Table 78: REFERENCE DIVIDER: Ref Divide Config 2
\\
\hline
ocpi\_pad\_2ad & uchar  & - & - &  & -  &- & -
\\
\hline
gain\_readback\_gain\_rx1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d688\_0x02b0 Table 80: Rx GAIN READ BACK: Gain Rx1
\\
\hline
gain\_readback\_lpf\_gain\_rx1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d689\_0x02b1 Table 80: Rx GAIN READ BACK: LPF Gain Rx1
\\
\hline
gain\_readback\_dig\_gain\_rx1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d690\_0x02b2 Table 80: Rx GAIN READ BACK: Dig gain Rx1
\\
\hline
gain\_readback\_fast\_attack\_state & uchar  & - & - & Volatile & -  &- & reg\_addr\_d691\_0x02b3 Table 80: Rx GAIN READ BACK: Fast Attack State
\\
\hline
gain\_readback\_slow\_loop\_state & uchar  & - & - & Volatile & -  &- & reg\_addr\_d692\_0x02b4 Table 80: Rx GAIN READ BACK: Slow Loop State
\\
\hline
gain\_readback\_gain\_rx2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d693\_0x02b5 Table 80: Rx GAIN READ BACK: Gain Rx2
\\
\hline
gain\_readback\_lpf\_gain\_rx2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d694\_0x02b6 Table 80: Rx GAIN READ BACK: LPF Gain Rx2
\\
\hline
gain\_readback\_dig\_gain\_rx2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d695\_0x02b7 Table 80: Rx GAIN READ BACK: Dig Gain Rx2
\\
\hline
gain\_readback\_ovrg\_sigs\_rx1 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d696\_0x02b8 Table 80: Rx GAIN READ BACK: Ovrg Sigs Rx1
\\
\hline
gain\_readback\_ovrg\_sigs\_rx2 & uchar  & - & - & Volatile & -  &- & reg\_addr\_d697\_0x02b9 Table 80: Rx GAIN READ BACK: Ovrg Sigs Rx2
\\
\hline
ocpi\_pad\_2ba & uchar  & - & - &  & -  &- & -
\\
\hline
ctrl & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d991\_0x03df Table 83: CONTROL: Control
\\
\hline
ocpi\_pad\_3e0 & uchar  & - & - &  & -  &- & -
\\
\hline
test\_bist\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d1012\_0x03f4 Table 84: DIGITAL TEST: BIST Config
\\
\hline
test\_observe\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d1013\_0x03f5 Table 84: DIGITAL TEST: Observe Config
\\
\hline
test\_bist\_and\_data\_port\_test\_config & uchar  & - & - & Volatile, Writeable & -  &- & reg\_addr\_d1014\_0x03f6 Table 84: DIGITAL TEST: BIST and Data Port Test Config
\\
\hline
pin\_control\_p & bool & - & - & Parameter & -  &- & Whether RX/TX powerdown via pin control is possible.
\\
\hline
\end{longtable}
