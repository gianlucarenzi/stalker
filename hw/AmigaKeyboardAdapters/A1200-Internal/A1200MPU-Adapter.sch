EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Amiga1200 U13 MPU Adapter"
Date "2022-11-10"
Rev "2.0"
Comp "RetroBit Lab"
Comment1 "Amiga1200 Internal Connector for STALKER"
Comment2 "From Kicad 4 to Kicad 5"
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L AmigaComponents:A1200U13_REV_PLCC44 U1
U 1 1 5BE895D2
P 5750 3650
F 0 "U1" H 6450 5500 50  0000 C CNN
F 1 "A1200U13_REV_PLCC44" H 5950 1850 50  0000 L CNN
F 2 "RetroBitLab:PLCC-44_THT-Socket_3D" H 5750 3650 60  0001 C CNN
F 3 "" H 5750 3650 60  0000 C CNN
	1    5750 3650
	1    0    0    -1  
$EndComp
Text GLabel 4750 2750 0    60   BiDi ~ 0
nVCC5V
Text GLabel 5750 1650 1    60   BiDi ~ 0
mVCC5V
$Comp
L Device:CP1_Small C1
U 1 1 5BE89BB8
P 4100 1400
F 0 "C1" H 4110 1470 50  0000 L CNN
F 1 "10uF" H 4110 1320 50  0000 L CNN
F 2 "Capacitor_Tantalum_SMD:CP_EIA-3216-12_Kemet-S_Pad1.58x1.35mm_HandSolder" H 4100 1400 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1811151641_Sunlord-TC212A106K016Y_C108529.pdf" H 4100 1400 50  0001 C CNN
F 4 "C108529" H 4100 1400 50  0001 C CNN "LCSC"
	1    4100 1400
	1    0    0    -1  
$EndComp
Wire Wire Line
	4100 1000 4100 1200
Wire Wire Line
	3850 1200 4100 1200
Wire Wire Line
	4700 1000 4700 1200
Connection ~ 4100 1200
Wire Wire Line
	4100 1500 4100 1600
Text GLabel 3850 1200 0    60   BiDi ~ 0
VCC5V
Text GLabel 3850 1600 0    60   BiDi ~ 0
GND
Text GLabel 4100 1000 1    60   BiDi ~ 0
mVCC5V
Text GLabel 4700 1000 1    60   BiDi ~ 0
nVCC5V
Connection ~ 4700 1200
Text GLabel 5750 5650 3    60   BiDi ~ 0
mGND
Text GLabel 4100 1800 3    60   BiDi ~ 0
mGND
$Comp
L power:GND #PWR01
U 1 1 5BE8A525
P 2500 1900
F 0 "#PWR01" H 2500 1650 50  0001 C CNN
F 1 "GND" H 2500 1750 50  0000 C CNN
F 2 "" H 2500 1900 50  0001 C CNN
F 3 "" H 2500 1900 50  0001 C CNN
	1    2500 1900
	1    0    0    -1  
$EndComp
Text GLabel 2500 1600 1    60   BiDi ~ 0
GND
Wire Wire Line
	2500 1600 2500 1750
$Comp
L power:PWR_FLAG #FLG02
U 1 1 5BE8A55F
P 2500 1750
F 0 "#FLG02" H 2500 1825 50  0001 C CNN
F 1 "PWR_FLAG" H 2500 1900 50  0000 C CNN
F 2 "" H 2500 1750 50  0001 C CNN
F 3 "" H 2500 1750 50  0001 C CNN
	1    2500 1750
	0    1    1    0   
$EndComp
Text GLabel 3000 1700 1    60   BiDi ~ 0
VCC5V
Wire Wire Line
	3000 1700 3000 1845
$Comp
L power:VCC #PWR03
U 1 1 5BE8A596
P 3000 1950
F 0 "#PWR03" H 3000 1800 50  0001 C CNN
F 1 "VCC" H 3000 2100 50  0000 C CNN
F 2 "" H 3000 1950 50  0001 C CNN
F 3 "" H 3000 1950 50  0001 C CNN
	1    3000 1950
	-1   0    0    1   
$EndComp
$Comp
L power:PWR_FLAG #FLG04
U 1 1 5BE8A5B2
P 3000 1845
F 0 "#FLG04" H 3000 1920 50  0001 C CNN
F 1 "PWR_FLAG" H 3000 1995 50  0000 C CNN
F 2 "" H 3000 1845 50  0001 C CNN
F 3 "" H 3000 1845 50  0001 C CNN
	1    3000 1845
	0    1    1    0   
$EndComp
$Comp
L power:VCC #PWR05
U 1 1 5BE8ABEA
P 8200 3650
F 0 "#PWR05" H 8200 3500 50  0001 C CNN
F 1 "VCC" H 8200 3800 50  0000 C CNN
F 2 "" H 8200 3650 50  0001 C CNN
F 3 "" H 8200 3650 50  0001 C CNN
	1    8200 3650
	0    1    1    0   
$EndComp
Text GLabel 6750 2750 2    60   BiDi ~ 0
mKBD_DATA
Text GLabel 8200 3450 2    60   BiDi ~ 0
mKBD_DATA
Text GLabel 6750 4500 2    60   BiDi ~ 0
mKBD_CLOCK
Text GLabel 7700 3450 0    60   BiDi ~ 0
mKBD_CLOCK
Text GLabel 4750 4500 0    60   BiDi ~ 0
mKBD_RESET
Text GLabel 7700 3650 0    60   BiDi ~ 0
mKBD_RESET
Wire Wire Line
	8550 3550 8550 3850
Wire Wire Line
	8550 3850 6950 3850
Wire Wire Line
	6950 3850 6950 3550
Wire Wire Line
	6950 3550 7700 3550
$Comp
L power:GND #PWR06
U 1 1 5BE8AFDB
P 6950 3850
F 0 "#PWR06" H 6950 3600 50  0001 C CNN
F 1 "GND" H 6950 3700 50  0000 C CNN
F 2 "" H 6950 3850 50  0001 C CNN
F 3 "" H 6950 3850 50  0001 C CNN
	1    6950 3850
	1    0    0    -1  
$EndComp
Connection ~ 6950 3850
$Comp
L Device:R_Small R1
U 1 1 5BE8AFFD
P 8085 1600
F 0 "R1" H 7945 1605 50  0000 L CNN
F 1 "330R" H 8155 1610 50  0000 L CNN
F 2 "Resistor_SMD:R_1206_3216Metric_Pad1.30x1.75mm_HandSolder" H 8085 1600 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1811141443_Ever-Ohms-Tech-CR1206F330RP05Z_C245520.pdf" H 8085 1600 50  0001 C CNN
F 4 "C245520" H 8085 1600 50  0001 C CNN "LCSC"
	1    8085 1600
	-1   0    0    1   
$EndComp
$Comp
L power:GND #PWR07
U 1 1 5BE8B087
P 8085 2000
F 0 "#PWR07" H 8085 1750 50  0001 C CNN
F 1 "GND" H 8085 1850 50  0000 C CNN
F 2 "" H 8085 2000 50  0001 C CNN
F 3 "" H 8085 2000 50  0001 C CNN
	1    8085 2000
	1    0    0    -1  
$EndComp
$Comp
L power:VCC #PWR08
U 1 1 5BE8B0A7
P 8085 1500
F 0 "#PWR08" H 8085 1350 50  0001 C CNN
F 1 "VCC" H 8085 1650 50  0000 C CNN
F 2 "" H 8085 1500 50  0001 C CNN
F 3 "" H 8085 1500 50  0001 C CNN
	1    8085 1500
	1    0    0    -1  
$EndComp
Wire Wire Line
	3850 1600 4100 1600
Connection ~ 4100 1600
$Comp
L Device:C_Small C3
U 1 1 5BE8B488
P 4350 1400
F 0 "C3" H 4360 1470 50  0000 L CNN
F 1 "100nF" H 4360 1320 50  0000 L CNN
F 2 "Capacitor_SMD:C_1206_3216Metric_Pad1.33x1.80mm_HandSolder" H 4350 1400 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1810221109_Samsung-Electro-Mechanics-CL31B104KBCNNNC_C24497.pdf" H 4350 1400 50  0001 C CNN
F 4 "C24497" H 4350 1400 50  0001 C CNN "LCSC"
	1    4350 1400
	1    0    0    -1  
$EndComp
Wire Wire Line
	4950 1200 4950 1300
Wire Wire Line
	4950 1600 4950 1500
Connection ~ 4700 1600
Wire Wire Line
	4350 1300 4350 1200
Connection ~ 4350 1200
Wire Wire Line
	4350 1500 4350 1600
Connection ~ 4350 1600
Wire Wire Line
	4100 1200 4100 1300
Wire Wire Line
	4100 1200 4350 1200
Wire Wire Line
	4700 1200 4700 1300
Wire Wire Line
	4700 1200 4950 1200
Wire Wire Line
	4100 1600 4100 1800
Wire Wire Line
	4100 1600 4350 1600
Wire Wire Line
	4700 1600 4950 1600
Wire Wire Line
	4350 1200 4700 1200
Wire Wire Line
	4350 1600 4700 1600
$Comp
L Connector_Generic:Conn_02x03_Odd_Even P1
U 1 1 636CBEED
P 7900 3550
F 0 "P1" H 7950 3775 50  0000 C CNN
F 1 "Conn_02x03_Odd_Even" H 7950 3776 50  0001 C CNN
F 2 "Connector_IDC:IDC-Header_2x03_P2.54mm_Vertical" H 7900 3550 50  0001 C CNN
F 3 "~" H 7900 3550 50  0001 C CNN
	1    7900 3550
	1    0    0    -1  
$EndComp
Wire Wire Line
	8200 3550 8550 3550
$Comp
L Device:LED D1
U 1 1 636D4F4F
P 8085 1850
F 0 "D1" V 8095 1675 50  0000 L CNN
F 1 "RED" V 8075 1915 50  0000 L CNN
F 2 "LED_SMD:LED_1206_3216Metric_Pad1.42x1.75mm_HandSolder" H 8085 1850 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1811151241_Everlight-Elec-15-21-R6C-FQ1R1B-2T_C93133.pdf" H 8085 1850 50  0001 C CNN
F 4 "C93133" V 8085 1850 50  0001 C CNN "LCSC"
	1    8085 1850
	0    -1   -1   0   
$EndComp
Wire Wire Line
	4700 1500 4700 1600
$Comp
L Device:CP1_Small C2
U 1 1 636E678B
P 4700 1400
F 0 "C2" H 4710 1470 50  0000 L CNN
F 1 "10uF" H 4710 1320 50  0000 L CNN
F 2 "Capacitor_Tantalum_SMD:CP_EIA-3216-12_Kemet-S_Pad1.58x1.35mm_HandSolder" H 4700 1400 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1811151641_Sunlord-TC212A106K016Y_C108529.pdf" H 4700 1400 50  0001 C CNN
F 4 "C108529" H 4700 1400 50  0001 C CNN "LCSC"
	1    4700 1400
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C4
U 1 1 636E7056
P 4950 1400
F 0 "C4" H 4960 1470 50  0000 L CNN
F 1 "100nF" H 4960 1320 50  0000 L CNN
F 2 "Capacitor_SMD:C_1206_3216Metric_Pad1.33x1.80mm_HandSolder" H 4950 1400 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1810221109_Samsung-Electro-Mechanics-CL31B104KBCNNNC_C24497.pdf" H 4950 1400 50  0001 C CNN
F 4 "C24497" H 4950 1400 50  0001 C CNN "LCSC"
	1    4950 1400
	1    0    0    -1  
$EndComp
Connection ~ 3000 1845
Wire Wire Line
	3000 1845 3000 1950
Connection ~ 2500 1750
Wire Wire Line
	2500 1750 2500 1900
$EndSCHEMATC
