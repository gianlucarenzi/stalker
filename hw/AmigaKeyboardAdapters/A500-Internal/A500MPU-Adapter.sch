EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Amiga 500 MPU Adapter"
Date "2022-11-30"
Rev "2.0"
Comp "Retrobit Lab"
Comment1 "J13 Adapter on Amiga 500 Motherboard Lobster"
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Wire Wire Line
	7600 3050 7400 3050
Wire Wire Line
	7600 3150 7400 3150
Wire Wire Line
	7600 3250 7400 3250
Wire Wire Line
	7600 3350 7400 3350
Wire Wire Line
	7600 3450 7400 3450
Wire Wire Line
	7600 3550 7500 3550
Wire Wire Line
	7600 3650 7400 3650
Wire Wire Line
	7600 3750 7400 3750
Text GLabel 7400 3050 0    39   BiDi ~ 0
CLOCK
Text GLabel 7400 3150 0    39   BiDi ~ 0
DATA
Text GLabel 7400 3250 0    39   BiDi ~ 0
RESET
Text GLabel 7400 3650 0    39   BiDi ~ 0
STATUS
Text GLabel 7400 3750 0    39   BiDi ~ 0
INUSE
NoConn ~ 7400 3450
$Comp
L power:GND #PWR01
U 1 1 5BE8B22F
P 7500 3850
F 0 "#PWR01" H 7500 3600 50  0001 C CNN
F 1 "GND" H 7500 3700 50  0000 C CNN
F 2 "" H 7500 3850 50  0001 C CNN
F 3 "" H 7500 3850 50  0001 C CNN
	1    7500 3850
	1    0    0    -1  
$EndComp
Wire Wire Line
	7500 3850 7500 3550
Connection ~ 7500 3550
Text GLabel 7400 3550 0    39   BiDi ~ 0
GND
Text GLabel 7400 3350 0    39   BiDi ~ 0
VCC
$Comp
L power:VCC #PWR02
U 1 1 5BE8B275
P 7600 4150
F 0 "#PWR02" H 7600 4000 50  0001 C CNN
F 1 "VCC" H 7600 4300 50  0000 C CNN
F 2 "" H 7600 4150 50  0001 C CNN
F 3 "" H 7600 4150 50  0001 C CNN
	1    7600 4150
	-1   0    0    1   
$EndComp
Wire Wire Line
	8950 3050 8750 3050
Wire Wire Line
	8950 3150 8750 3150
Wire Wire Line
	8950 3250 8750 3250
Wire Wire Line
	8950 3350 8750 3350
Wire Wire Line
	8950 3450 8750 3450
Wire Wire Line
	8950 3550 8850 3550
Wire Wire Line
	8950 3650 8750 3650
Wire Wire Line
	8950 3750 8750 3750
Text GLabel 8750 3050 0    39   BiDi ~ 0
CLOCK
Text GLabel 8750 3150 0    39   BiDi ~ 0
DATA
Text GLabel 8750 3250 0    39   BiDi ~ 0
RESET
Text GLabel 8750 3650 0    39   BiDi ~ 0
STATUS
Text GLabel 8750 3750 0    39   BiDi ~ 0
INUSE
NoConn ~ 8750 3450
$Comp
L power:GND #PWR03
U 1 1 5BE8B42D
P 8850 3850
F 0 "#PWR03" H 8850 3600 50  0001 C CNN
F 1 "GND" H 8850 3700 50  0000 C CNN
F 2 "" H 8850 3850 50  0001 C CNN
F 3 "" H 8850 3850 50  0001 C CNN
	1    8850 3850
	1    0    0    -1  
$EndComp
Wire Wire Line
	8850 3850 8850 3550
Connection ~ 8850 3550
Text GLabel 8750 3550 0    39   BiDi ~ 0
GND
Text GLabel 8750 3350 0    39   BiDi ~ 0
VCC
$Comp
L power:VCC #PWR04
U 1 1 5BE8B438
P 8950 4150
F 0 "#PWR04" H 8950 4000 50  0001 C CNN
F 1 "VCC" H 8950 4300 50  0000 C CNN
F 2 "" H 8950 4150 50  0001 C CNN
F 3 "" H 8950 4150 50  0001 C CNN
	1    8950 4150
	-1   0    0    1   
$EndComp
$Comp
L power:VCC #PWR05
U 1 1 5BE8B527
P 9800 2750
F 0 "#PWR05" H 9800 2600 50  0001 C CNN
F 1 "VCC" H 9800 2900 50  0000 C CNN
F 2 "" H 9800 2750 50  0001 C CNN
F 3 "" H 9800 2750 50  0001 C CNN
	1    9800 2750
	1    0    0    -1  
$EndComp
Wire Wire Line
	9800 2750 9800 2850
Text GLabel 9800 2950 3    39   BiDi ~ 0
VCC
$Comp
L power:GND #PWR06
U 1 1 5BE8B566
P 10050 2750
F 0 "#PWR06" H 10050 2500 50  0001 C CNN
F 1 "GND" H 10050 2600 50  0000 C CNN
F 2 "" H 10050 2750 50  0001 C CNN
F 3 "" H 10050 2750 50  0001 C CNN
	1    10050 2750
	-1   0    0    1   
$EndComp
Text GLabel 10050 2950 3    39   BiDi ~ 0
GND
Wire Wire Line
	10050 2750 10050 2850
$Comp
L power:PWR_FLAG #FLG07
U 1 1 5BE8B59C
P 10050 2850
F 0 "#FLG07" H 10050 2925 50  0001 C CNN
F 1 "PWR_FLAG" H 10050 3000 50  0000 C CNN
F 2 "" H 10050 2850 50  0001 C CNN
F 3 "" H 10050 2850 50  0001 C CNN
	1    10050 2850
	0    1    1    0   
$EndComp
$Comp
L power:PWR_FLAG #FLG08
U 1 1 5BE8B5B8
P 9800 2850
F 0 "#FLG08" H 9800 2925 50  0001 C CNN
F 1 "PWR_FLAG" H 9800 3000 50  0000 C CNN
F 2 "" H 9800 2850 50  0001 C CNN
F 3 "" H 9800 2850 50  0001 C CNN
	1    9800 2850
	0    -1   -1   0   
$EndComp
Connection ~ 10050 2850
Connection ~ 9800 2850
Text GLabel 6250 2100 2    39   BiDi ~ 0
STATUS
Wire Wire Line
	5650 2100 5900 2100
$Comp
L power:GND #PWR09
U 1 1 5BE8B7FF
P 5900 2700
F 0 "#PWR09" H 5900 2450 50  0001 C CNN
F 1 "GND" H 5900 2550 50  0000 C CNN
F 2 "" H 5900 2700 50  0001 C CNN
F 3 "" H 5900 2700 50  0001 C CNN
	1    5900 2700
	1    0    0    -1  
$EndComp
Text GLabel 6250 3025 2    39   BiDi ~ 0
INUSE
$Comp
L Device:R_Small R2
U 1 1 5BE8B8C9
P 5900 3225
F 0 "R2" H 5750 3225 50  0000 L CNN
F 1 "330R" H 5955 3220 50  0000 L CNN
F 2 "Resistor_SMD:R_1206_3216Metric_Pad1.30x1.75mm_HandSolder" H 5900 3225 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1811141443_Ever-Ohms-Tech-CR1206F330RP05Z_C245520.pdf" H 5900 3225 50  0001 C CNN
F 4 "C245520" H 5900 3225 50  0001 C CNN "LCSC"
	1    5900 3225
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR010
U 1 1 5BE8B8D5
P 5900 3625
F 0 "#PWR010" H 5900 3375 50  0001 C CNN
F 1 "GND" H 5900 3475 50  0000 C CNN
F 2 "" H 5900 3625 50  0001 C CNN
F 3 "" H 5900 3625 50  0001 C CNN
	1    5900 3625
	1    0    0    -1  
$EndComp
Text GLabel 6220 4045 2    39   BiDi ~ 0
VCC
$Comp
L power:GND #PWR011
U 1 1 5BE8BAB0
P 5870 4645
F 0 "#PWR011" H 5870 4395 50  0001 C CNN
F 1 "GND" H 5870 4495 50  0000 C CNN
F 2 "" H 5870 4645 50  0001 C CNN
F 3 "" H 5870 4645 50  0001 C CNN
	1    5870 4645
	1    0    0    -1  
$EndComp
Text GLabel 4650 2910 2    39   BiDi ~ 0
DATA
Text GLabel 4150 2910 0    39   BiDi ~ 0
CLOCK
Text GLabel 4150 3110 0    39   BiDi ~ 0
RESET
Text GLabel 4650 3110 2    39   BiDi ~ 0
VCC
Text GLabel 4150 3010 0    39   BiDi ~ 0
GND
Text GLabel 4650 3010 2    39   BiDi ~ 0
GND
$Comp
L power:GND #PWR012
U 1 1 5BE8BFEA
P 5350 2100
F 0 "#PWR012" H 5350 1850 50  0001 C CNN
F 1 "GND" H 5350 1950 50  0000 C CNN
F 2 "" H 5350 2100 50  0001 C CNN
F 3 "" H 5350 2100 50  0001 C CNN
	1    5350 2100
	-1   0    0    1   
$EndComp
Connection ~ 5900 2100
Wire Wire Line
	5900 2100 5900 2200
Wire Wire Line
	5650 3025 5900 3025
$Comp
L power:GND #PWR013
U 1 1 5BE8C240
P 5350 3025
F 0 "#PWR013" H 5350 2775 50  0001 C CNN
F 1 "GND" H 5350 2875 50  0000 C CNN
F 2 "" H 5350 3025 50  0001 C CNN
F 3 "" H 5350 3025 50  0001 C CNN
	1    5350 3025
	-1   0    0    1   
$EndComp
Wire Wire Line
	5900 3125 5900 3025
Connection ~ 5900 3025
Wire Wire Line
	6220 4045 5870 4045
Wire Wire Line
	5870 4045 5870 4145
Text GLabel 4500 4350 2    39   BiDi ~ 0
VCC
$Comp
L power:GND #PWR014
U 1 1 5C1ED552
P 4150 4850
F 0 "#PWR014" H 4150 4600 50  0001 C CNN
F 1 "GND" H 4150 4700 50  0000 C CNN
F 2 "" H 4150 4850 50  0001 C CNN
F 3 "" H 4150 4850 50  0001 C CNN
	1    4150 4850
	1    0    0    -1  
$EndComp
Wire Wire Line
	3850 4350 4150 4350
Wire Wire Line
	4150 4350 4150 4450
$Comp
L Device:C_Small C2
U 1 1 5C1ED589
P 4150 4550
F 0 "C2" H 4160 4620 50  0000 L CNN
F 1 "100nF" H 4160 4470 50  0000 L CNN
F 2 "Capacitor_SMD:C_1206_3216Metric_Pad1.33x1.80mm_HandSolder" H 4150 4550 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1810221109_Samsung-Electro-Mechanics-CL31B104KBCNNNC_C24497.pdf" H 4150 4550 50  0001 C CNN
F 4 "C24497" H 4150 4550 50  0001 C CNN "LCSC"
	1    4150 4550
	1    0    0    -1  
$EndComp
$Comp
L Device:CP1_Small C1
U 1 1 5C1ED5CC
P 3850 4550
F 0 "C1" H 3860 4620 50  0000 L CNN
F 1 "10uF" H 3860 4470 50  0000 L CNN
F 2 "Capacitor_Tantalum_SMD:CP_EIA-3216-12_Kemet-S_Pad1.58x1.35mm_HandSolder" H 3850 4550 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1811151641_Sunlord-TC212A106K016Y_C108529.pdf" H 3850 4550 50  0001 C CNN
F 4 "C108529" H 3850 4550 50  0001 C CNN "LCSC"
	1    3850 4550
	1    0    0    -1  
$EndComp
Wire Wire Line
	3850 4350 3850 4450
Connection ~ 4150 4350
Wire Wire Line
	4150 4650 4150 4750
Wire Wire Line
	4150 4750 3850 4750
Wire Wire Line
	3850 4750 3850 4650
Connection ~ 4150 4750
Wire Wire Line
	7500 3550 7400 3550
Wire Wire Line
	8850 3550 8750 3550
Wire Wire Line
	10050 2850 10050 2950
Wire Wire Line
	9800 2850 9800 2950
Wire Wire Line
	5900 2100 6250 2100
Wire Wire Line
	5900 3025 6250 3025
Wire Wire Line
	4150 4350 4500 4350
Wire Wire Line
	4150 4750 4150 4850
$Comp
L Connector_Generic:Conn_02x03_Odd_Even P3
U 1 1 6387440A
P 4350 3010
F 0 "P3" H 4390 3245 50  0000 C CNN
F 1 "Conn_02x03" H 4390 2770 50  0001 C CNN
F 2 "Connector_IDC:IDC-Header_2x03_P2.54mm_Vertical" H 4350 3010 50  0001 C CNN
F 3 "~" H 4350 3010 50  0001 C CNN
	1    4350 3010
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x08 P1
U 1 1 63877AAC
P 7800 3350
F 0 "P1" H 7740 3800 50  0000 L CNN
F 1 "Conn_01x08" H 7880 3251 50  0001 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical" H 7800 3350 50  0001 C CNN
F 3 "~" H 7800 3350 50  0001 C CNN
	1    7800 3350
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x08 P2
U 1 1 63879470
P 9150 3350
F 0 "P2" H 9085 3760 50  0000 L CNN
F 1 "Conn_01x08" H 9230 3251 50  0001 L CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical" H 9150 3350 50  0001 C CNN
F 3 "~" H 9150 3350 50  0001 C CNN
	1    9150 3350
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x02 P4
U 1 1 63888D90
P 5450 2420
F 0 "P4" V 5560 2315 50  0000 L CNN
F 1 "Conn_01x02" V 5413 2500 50  0001 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 5450 2420 50  0001 C CNN
F 3 "~" H 5450 2420 50  0001 C CNN
	1    5450 2420
	0    1    1    0   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 P5
U 1 1 6388A0FA
P 5450 3330
F 0 "P5" V 5560 3240 50  0000 L CNN
F 1 "Conn_01x02" V 5413 3410 50  0001 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 5450 3330 50  0001 C CNN
F 3 "~" H 5450 3330 50  0001 C CNN
	1    5450 3330
	0    1    1    0   
$EndComp
Wire Wire Line
	5450 3025 5450 3130
Wire Wire Line
	5350 3025 5350 3130
Wire Wire Line
	5450 2100 5450 2220
Wire Wire Line
	5350 2100 5350 2220
Wire Wire Line
	7600 3750 7600 4150
Connection ~ 7600 3750
Wire Wire Line
	8950 3750 8950 4150
Connection ~ 8950 3750
$Comp
L Device:R_Small R3
U 1 1 638B5FCA
P 5870 4245
F 0 "R3" H 5720 4245 50  0000 L CNN
F 1 "330R" H 5925 4240 50  0000 L CNN
F 2 "Resistor_SMD:R_1206_3216Metric_Pad1.30x1.75mm_HandSolder" H 5870 4245 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1811141443_Ever-Ohms-Tech-CR1206F330RP05Z_C245520.pdf" H 5870 4245 50  0001 C CNN
F 4 "C245520" H 5870 4245 50  0001 C CNN "LCSC"
	1    5870 4245
	1    0    0    -1  
$EndComp
$Comp
L Device:R_Small R1
U 1 1 638B6D68
P 5900 2300
F 0 "R1" H 5750 2300 50  0000 L CNN
F 1 "330R" H 5955 2295 50  0000 L CNN
F 2 "Resistor_SMD:R_1206_3216Metric_Pad1.30x1.75mm_HandSolder" H 5900 2300 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1811141443_Ever-Ohms-Tech-CR1206F330RP05Z_C245520.pdf" H 5900 2300 50  0001 C CNN
F 4 "C245520" H 5900 2300 50  0001 C CNN "LCSC"
	1    5900 2300
	1    0    0    -1  
$EndComp
$Comp
L Device:LED D2
U 1 1 638C83C9
P 5900 3475
F 0 "D2" V 5905 3575 50  0000 L CNN
F 1 "FLOPPY GREEN" V 5910 2780 50  0000 L CNN
F 2 "LED_SMD:LED_1206_3216Metric_Pad1.42x1.75mm_HandSolder" V 5900 3475 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1810261420_Everlight-Elec-15-21-S2C-AQ2R2B-2T_C183851.pdf" V 5900 3475 50  0001 C CNN
F 4 "C2980185" H 5900 3475 50  0001 C CNN "LCSC"
	1    5900 3475
	0    -1   -1   0   
$EndComp
$Comp
L Device:LED D3
U 1 1 638C8D6E
P 5870 4495
F 0 "D3" H 5820 4620 50  0000 L CNN
F 1 "READY RED" V 5870 3945 50  0000 L CNN
F 2 "LED_SMD:LED_1206_3216Metric_Pad1.42x1.75mm_HandSolder" V 5870 4495 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1811151241_Everlight-Elec-15-21-R6C-FQ1R1B-2T_C93133.pdf" V 5870 4495 50  0001 C CNN
F 4 "C93133" H 5870 4495 50  0001 C CNN "LCSC"
	1    5870 4495
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_Small R4
U 1 1 638E637E
P 5550 2100
F 0 "R4" V 5460 2050 50  0000 L CNN
F 1 "330R" V 5640 2050 50  0000 L CNN
F 2 "Resistor_SMD:R_1206_3216Metric_Pad1.30x1.75mm_HandSolder" H 5550 2100 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1811141443_Ever-Ohms-Tech-CR1206F330RP05Z_C245520.pdf" H 5550 2100 50  0001 C CNN
F 4 "C245520" H 5550 2100 50  0001 C CNN "LCSC"
	1    5550 2100
	0    1    1    0   
$EndComp
$Comp
L Device:R_Small R5
U 1 1 638E937F
P 5550 3025
F 0 "R5" V 5460 2975 50  0000 L CNN
F 1 "330R" V 5640 2975 50  0000 L CNN
F 2 "Resistor_SMD:R_1206_3216Metric_Pad1.30x1.75mm_HandSolder" H 5550 3025 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1811141443_Ever-Ohms-Tech-CR1206F330RP05Z_C245520.pdf" H 5550 3025 50  0001 C CNN
F 4 "C245520" H 5550 3025 50  0001 C CNN "LCSC"
	1    5550 3025
	0    1    1    0   
$EndComp
$Comp
L Device:LED D1
U 1 1 638EB651
P 5900 2550
F 0 "D1" V 5910 2630 50  0000 L CNN
F 1 "POWER ORANGE" V 5910 1855 50  0000 L CNN
F 2 "LED_SMD:LED_1206_3216Metric_Pad1.42x1.75mm_HandSolder" V 5900 2550 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1810261420_Everlight-Elec-15-21-S2C-AQ2R2B-2T_C183851.pdf" V 5900 2550 50  0001 C CNN
F 4 "C183851" H 5900 2550 50  0001 C CNN "LCSC"
	1    5900 2550
	0    -1   -1   0   
$EndComp
$EndSCHEMATC
