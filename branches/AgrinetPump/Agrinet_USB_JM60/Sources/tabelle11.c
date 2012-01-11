const unsigned char
presentazione_iniziale[5][17]={
"PCM-PCT   V11   ",
"    PCM3.0      ",
"ThreePhase    KW",
"SinglePhase   KW",
"SinglePh   KW+C2"},

abilitazione_OFF[17]={
"Not Enable      "},

accesso_password[17]={
"ACCES ENABLED   "},

exit_program[2][17]={
"    EXIT        ",
" PROGRAMMING?   "}, 

scritta_numero_serie[17]={
"SERIAL N:       "},

dati_presentati_in_ON_monofase[2][17]={
"   V     W     B",//monofase alternato
"    A .  PF    C"},

dati_presentati_in_ON_trifase[4][17]={
"   V     W     B",//trifase 
"               A",
"   C     W     L",//trifase alternato
"PF.   .   .   S "},

posizione_allarme[32]={ //bit 7=1, 6=0 ON,  bit 7=0 6=1 OFF, bit 7=0 6=0 nessuna aggiunta
0x40,//00
0x80,//01
0x01,//02
0x02,//03
0x43,//04
0x83,//05
0x84,//06
0x44,//07 

0x4b,//08 
0x4e,//09 

0x85,//10 
0x86,//11 
0x87,//12 
0x88,//13 
0x89,//14 
0x8a,//15 
0x8b,//16 
0x8c,//17 
0x8d,//18 
0x8e,//19 

0x45,//20 
0x46,//21 
0x47,//22 
0x48,//23 
0x49,//24 
0x4a,//25 
0x4b,//26 
0x4c,//27 
0x4d,//28 
0x4e,//29 

0x4f,//30 
0x50 //31 
},

lettura_allarmi[17][17]={
//"Manual       OFF", //allarme 0
"Manual        ON", //0,allarme 1
"Nom.P.V.I.Change", //1,allarme 2
"Nom.P.V.I.Change", //2,allarme 3
"Remote       OFF", //3,allarme 4
//"Remote        ON", //allarme 5
"automatic     ON", //4,allarme 6
//"automatic    OFF",//allarme 7
//"Maximum Flow OFF",//allarme 8
//"Over PressureOFF",//allarme 9

"Over Current  ON",//5,allarme 10,20
"Over Voltage  ON",//6,allarme 11,21
"Under Voltage ON",//7,allarme 12,22
"Minimum Flow  ON",//8,allarme 13,23
"Dry Working   ON",//9,allarme 14,24
"Over Temperat.ON",//10,allarme 15,25
"Maximum Flow  ON",//11,allarme 16,26
"Unbal.Current ON",//12,allarme 17,27
"Dissym.Voltag.ON",//13,allarme 18,28
"Over Pressure ON",//14,allarme 19,29

//"OverCurrent  OFF",20
//"OverVoltage  OFF",21
//"UnderVoltage OFF",22
//"MinimumFlow  OFF",23
//"Dry Working  OFF",24
//"OverTemperat OFF",25
//"MaximumFlow  OFF",26
//"Unbal.CurrentOFF",27
//"Dissym.VoltagOFF",28
//"OverPressure OFF",29

"PressureSens OFF", //15,allarme 30
"ShortCircuit OFF"},//16,allarme 31

presenta_tipo_start_stop[2][17]={
"  Manual        ",
"Pressure  Switch"},

attivazione_comando_reset[2][17]={
"   YES?         ",
"   NOT?         "},

in_salvataggio[17]=
" Save Data      ",

chiamata_reset[17]=
"COUNTER RESET?  ",

reset_eseguito[17]=
"RESET EXECUTED  ",

attendere[17]=
"WAIT DISCHARGE  ",

menu_principale[32][17]={//Menu' principale: elenco delle funzioni di taratura
"1)PUMP POWER    ",
"                ",
"2) NUMBER       ",
"PASSWORD:       ",
"3) MOTOR        ",
"   DATA         ",
"4) VOLTAGE      ",
"PROTECTIONS     ",
"5) CURRENT      ",
"PROTECTIONS     ",
"6) PRESSURE     ",
"   CONTROL      ",
"7) POWER        ",
"   CONTROL      ",
"8)FLOW DIRECT   ",
"   CONTROL      ",
"9)TEMPERATURE   ",
"   CONTROL      ",
"10) ALARM       ",
"   HISTORY      ",
"11) ENERGY      ",
"   COUNTER      ",
"12)FLOW COUNTER ",
"                ",
"13)DATE SETTING ",
"                ",
"14)SERIAL NUMBER",
"                ",
"15) CURRENT     ",
"   SETTING      ",
"16) RESET       ",
"                "},

dati_motore[4][17]={
"3.1)Nominal     ",
"Voltage:       V",
"3.2)Nominal     ",
"Current:       A"},

protezione_voltaggio[14][17]={
"4.1)Over Voltage",
"Limit:         %",
"4.2)UnderVoltage",
"Limit:         %",   
"4.3)MinimumVolt.",
"Restart:       %",
"4.4)Dissimmetric",
"Volt.Alarm:    %",
"4.5)Dissimmetric",
"Volt.Stop:     %",
"4.6)ErrorV.delay",
"time:          s",
"4.7)RestartError",
"V.delay:     min"},

protezione_corrente[12][17]={
"5.1)Over Current",
"Limit:         %",
"5.2)Unbalanced I",
"Alarm:         %",   
"5.3)Unbalanced I",
"Stop:          %",   
"5.4)Error.I Dela",
"y Time:        s",
"5.5)RestartError",
"I delay:     min",
"5.6)K temperatu-",
"re Motor:      s"},

controllo_pressione[18][17]={
"6.1)PressureTran",
"sducer:         ",    
"6.2)Max.Pressure",
"Alarm:       Bar",
"6.3)Manual/Press",
"SwitchMode:     ",
"6.4)Max.Pressure",
"Stop:        Bar",
"6.5)Restart Pres",
"sure:        Bar",
"6.6)Press.Trans.",
"Min.Out:      mA",
"6.7)Press.Trans.",
"Max.Out:      mA",
"6.8)Press.Trans.",
"range:       Bar",
"6.9)DelayErr.Sen",
"Restart:       s"},

controllo_potenza[12][17]={
"7.1)Minimum Flow",
"PwrStop:       %",    
"7.2)Minimum Flow",
"StopDelay:     s",
"7.3)DelayMinimum",
"FlowRestart    s",
"7.4)MinimumPower",
"DryWork.:      %",
"7.5)DelayDryWork",
"Stop:          s",
"7.6)DelayDryWork",
"Restart:     min"},

sensore_di_flusso[12][17]={
"8.1)Flow Sensor:", 
"                ",    
"8.2)Minimum Flow",
"Limit:     L/min",
"8.3)Maximum Flow",
"Limit:     L/min",
"8.4)Flow Sensor ",
"Const:       L/P",
"8.5)Count Down  ",
"Volume:         ",
"8.6)CountVolume:",
"               L"},

sensore_temperatura[10][17]={
"9.1)Tmperat.Tran", 
"sducer:         ",    
"9.2)Wires Number",
"Resitor:        ",
"9.3)Stop Limit  ", 
"Temper:        C",   
"9.4)Resistance  ",
"0 C:         Ohm",
"9.5)Resistance  ",
"100 C:       Ohm"},

contatore_energia[2][17]={
"KWh:            ",
"reset energia?  "},   

contatore_litri[2][17]={
"m^3:            ",
"reset volume?   "},
  
presenta_data[2][17]={
"Date:   20  -   ",
"  -      :  :   "},
  
calibrazione[6][17]={
"15.1)Cal.I1:    ",
"I1:    ,I3:     ",    
"15.2)Cal.I3:    ",
"I1:    ,I3:     ",    
"15.3)Cal.I2:    ",
"I1:    ,I2:     "}, 

tabella_calibrazione[3]=//dei sensori di corrente
{     128,    112,    144};



const unsigned int
tabella_ritardo_protezione_squilibrio[3]=//s
{      10,      1,    120},

tabella_ritardo_protezione_tensione[3]=//s
{      10,      2,    160},

tabella_ritardo_riaccensione_da_emergenza_V[3]=//minuti
{       4,      1,    999}, 

tabella_ritardo_riaccensione_da_emergenza_I[3]=//minuti
{       4,      1,    999}, 

tabella_ritardo_stop_mandata_chiusa[3]=//s
{      20,      5,    120}, 

tabella_ritardo_stop_funzionamento_a_secco[3]=//s
{      10,      5,    120}, 

tabella_ritardo_riaccensione_mandata_chiusa[3]=//s
{     240,      3,    999}, 

tabella_ritardo_riaccensione_funzionamento_a_secco[3]=//minuti
{      10,      1,    100},

tabella_timer_ritorno_da_emergenza_sensore[3]=//s
{      10,      1,    999}, 

tabella_portata_sensore_pressione[3]=//Bar*10
{     160,     30,    500},

tabella_corrente_minima_sensore[3]=//mA*10
{      40,     10,    100},

tabella_corrente_massima_sensore[3]=//mA*10
{     200,    120,    250},

tabella_tipo_sonda_PT100[3]=//numero dei fili
{       4,      2,      4},

tabella_resistenza_PT100_a_0gradi[3]=//Ohm*10
{    1000,    100,   2000},

tabella_resistenza_PT100_a_100gradi[3]=//Ohm*10
{    1385,    200,   3000},

tabella_limite_intervento_temper_motore[3]=//�C
{     100,     60,    150},

tabella_pressione_emergenza[3]=//Bar*10
{     160,     30,    450},
   
tabella_pressione_spegnimento[3]=//Bar*10
{      60,     10,    400},
   
tabella_pressione_accensione[3]=//Bar*10
{      40,      5,    400},
   
tabella_limite_minimum_flow[3]=//litri/minuto
{       0,      0,   1000},
   
tabella_limite_maximum_flow[3]=//litri/minuto
{    1000,      1,   9999},
   
tabella_potenza_minima_mandata_chiusa[3]=//%
{      70,     10,    100},
   
tabella_potenza_minima_funz_secco[3]=//%
{      50,     10,    100},
   
tabella_K_di_tempo_riscaldamento[3]=//s
{      60,     10,    180},
   
tabella_limite_sovratensione[3]=//%
{     107,    100,    125},
   
tabella_limite_sottotensione[3]=//%
{      88,     60,     95},

tabella_tensione_restart[3]=//%
{      92,     60,    100},

tabella_limite_sovracorrente[3]=//%
{     115,    105,    140},
   
tabella_limite_segnalazione_dissimmetria[3]=//%
{      12,      5,     20},
   
tabella_limite_segnalazione_squilibrio[3]=//%
{      12,      5,     20},
   
tabella_limite_intervento_dissimmetria[3]=//%
{      15,      5,     25},
   
tabella_limite_intervento_squilibrio[3]=//%
{      15,      5,     25},

tabella_tensione_nominale_3fasi[3]=//V
{     400,    380,    440},

tabella_tensione_nominale_2fasi[3]=//V
{     230,    200,    440},

tabella_corrente_nominale[3][16]=//A*10
{   16,   19,   23,   31,   40,   56,   74,   98,  137,             25,   50,   62,   81,  104,  150,  227,//iniziali
    13,   15,   18,   25,   32,   45,   59,   78,  110,             20,   40,   50,   65,   83,  120,  182,//minimi
    19,   23,   28,   37,   48,   67,   89,  118,  164,             45,   60,   74,   97,  125,  180,  272};//massimi

const char
tabella_potenza_nominale[16]=//KW*100
{   37,   55,   75,  11,  15,  22,  30,  40,  55,             37,   55,   75,  11,  15,  22, 30};

const long
tabella_scala_sensore_di_flusso[3]=//litri*100/impulso
{    100,       1,   100000},

tabella_volume_massimo[3]=//litri 0-1000000
{    1000,      1,  1000000};

