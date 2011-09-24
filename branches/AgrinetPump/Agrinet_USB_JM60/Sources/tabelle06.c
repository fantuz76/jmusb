const unsigned char
presentazione_iniziale[16][17]={
"BOX PCT5.5   V06",
"ThreePhase0.37KW",
"ThreePhase0.55KW",
"ThreePhase0.75KW",
"ThreePhase1.10KW",
"ThreePhase1.50KW",
"ThreePhase2.20KW",
"ThreePhase3.00KW",
"ThreePhase4.00KW",
"ThreePhase5.50KW",
"SinglePhase.37KW",
"SinglePhase.55KW",
"SinglePhase.75KW",
"SinglePhase1.1KW",
"SinglePhase1.5KW",
"SinglePhase2.2KW"},

abilitazione_OFF[17]={
"Not enable      "},

accesso_password[17]={
"ACCES ENABLED   "},

dati_presentati_in_ON[4][2][17]={
"   V,     W    B",//monofase
' ',' ',' ',' ','A',' ',' ',' ',' ','P','F',' ',' ',' ',0xcd,'C',0,
"   V,     W    B",//monofase alternato
' ',' ',' ',' ','A',' ',' ',' ',' ','P','F',' ',' ',' ',0xcd,'C',0,

"   V,     W    B",//trifase 
"    A    A     A",
' ',' ',' ',0xcd,'C',' ',' ',' ',' ',' ','W',' ',' ',' ',' ','B',0,//trifase alternato
"PF.  ,.  ,.   S "},

lettura_allarmi[25][17]={
"      OFF       ",
"      ON        ",
"Over Current  ON",
"Over Voltage  ON",
"Under Voltage ON",
"Minimum Flow  ON",
"Dry Working   ON",
"Over Temperat.ON",
"Isolat.Fault  ON",
"Current Diff. ON",
"Voltage Diff. ON",
"Over Pressure ON",
"Over Current OFF",
"Over Voltage OFF",
"UnderVoltage OFF",
"Minimum Flow OFF",
"Dry Working  OFF",
"OverTemperat.OFF",
"Isolat.Fault OFF",
"Current Diff.OFF",
"Voltage Diff.OFF",
"PressureSens.OFF",
"OverPressure OFF",
"ShortCircuit OFF",
" END OF ALARM   "},

presenta_tipo_start_stop[2][17]={
"  Manual        ",
"Pressure  Switch"},

attivazione_comando_reset[2][17]={
"   YES?         ",
"   NOT?         "},

in_salvataggio[17]=
" Save Data      ",

menu_principale[32][17]={//Menu' principale: elenco delle funzioni di taratura
"1) NUMBER       ",
"PASSWORD:       ",
"2)PUMP POWER    ",
"                ",
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
"V.Alarm:       %",
"4.5)Dissimmetric",
"V.Stop:        %",
"4.6)ErrorV.delay",
"time:          s",
"4.7)ErrorV.delay",
"Restart:       s"},

protezione_corrente[12][17]={
"5.1)Over Current",
"Limit:         %",
"5.2)Unbalanced I",
"Alarm:         %",   
"5.3)Unbalanced I",
"Stop:          %",   
"5.4)ErrorI delay",
"Time:          %",
"5.5)RestartError",
"I delay:       %",
"5.6)K temperatu-",
"re Motor:      s"},

controllo_pressione[18][17]={
"6.1)Pressure    ",
"Sensor:         ",    
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
"range:        mA",
"6.9)DelayErr.Sen",
"Restart:       s"},

controllo_potenza[12][17]={
"7.1)Minimum Flow",
"PwrStop:       %",    
"7.2)Minimum Flow",
"StopDelay:     %",
"7.3)DelayMinimum",
"FlowRestart    s",
"7.4)MinimumPower",
"DryWork.:      %",
"7.5)DelayDryWork",
"Stop:          s",
"7.6)DelayDryWork",
"Restart:     min"},

sensore_di_flusso[8][17]={
"8.1)Flow Transdu", 
"cer:            ",    
"8.2)Minimum Flow",
"Limit:     l/min",
"8.3)Maximum Flow",
"Limit:     l/min",
"8.4)Flow Transd.",
"Const:      l/Pu"},

sensore_temperatura[10][17]={
"9.1)Tmperat.Tran", 
"ducer:          ",    
"9.2)Wires Number",
"Resitor:        ",
"9.3)Stop Limit  ", 
"Temper:       °C",   
"9.4)Resistance  ",
"0°C:         Ohm",
"9.5)Resistance  ",
"100°C:       Ohm"},

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
"I1:    ,I2:     "}; 

const unsigned int
tabella_potenza_nominale[15]=//KW*100
{   37,   55,   75,  110,  150,  220,  300,  400,  550,             37,   55,   75,  110,  150,  220},

tabella_ritardo_protezione_squilibrio[3]=//s
{       4,      1,    120},

tabella_ritardo_protezione_tensione[3]=//s
{      10,      1,    120},

tabella_ritardo_riaccensione_da_emergenza_V[3]=//minuti
{       4,      1,    999}, 

tabella_ritardo_riaccensione_da_emergenza_I[3]=//minuti
{       4,      1,    999}, 

tabella_ritardo_stop_mandata_chiusa[3]=//s
{      20,      1,    120}, 

tabella_ritardo_stop_funzionamento_a_secco[3]=//s
{      20,     10,    120}, 

tabella_ritardo_riaccensione_mandata_chiusa[3]=//s
{      10,      1,    120}, 

tabella_ritardo_riaccensione_funzionamento_a_secco[3]=//minuti
{       4,      1,    100},

tabella_timer_ritorno_da_emergenza_sensore[3]=//s
{      10,      1,    999}, 

tabella_portata_sensore_pressione[3]=//Bar*10
{     160,     50,    500},

tabella_corrente_minima_sensore[3]=//mA*10
{      40,     10,    100},

tabella_corrente_massima_sensore[3]=//mA*10
{     200,    100,    250},

tabella_scala_sensore_di_flusso[3]=//litri*1000/impulso
{     100,      1,  10000},

tabella_tipo_sonda_PT100[3]=//numero dei fili
{       4,      2,      4},

tabella_resistenza_PT100_a_0gradi[3]=//Ohm*10
{    1000,  10000,    100},

tabella_resistenza_PT100_a_100gradi[3]=//Ohm*10
{    1385,  20000,    200},

tabella_limite_intervento_temper_motore[3]=//°C
{     100,     80,    150},

tabella_pressione_emergenza[3]=//Bar*10
{  160,   50,  250},
   
tabella_pressione_spegnimento[3]=//Bar*10
{   60,   10,  200},
   
tabella_pressione_accensione[3]=//Bar*10
{   40,   10,  200},
   
tabella_limite_minimum_flow[3]=//litri/minuto
{   10,    0,  100},
   
tabella_limite_maximum_flow[3]=//litri/minuto
{  100,   10, 1000},
   
tabella_potenza_minima_mandata_chiusa[3]=//%
{   70,   10,  100},
   
tabella_potenza_minima_funz_secco[3]=//%
{   50,   10,   80},
   
tabella_K_di_tempo_riscaldamento[3]=//s
{   60,    2,  200},
   
tabella_calibrazione[3]=//dei sensori di corrente
{     128,    112,    144},

tabella_limite_segnalazione_dissimmetria[3][9]=//%
{   12,   12,   12,   12,   12,   12,   12,   12,   12,
     8,    8,    8,    8,    8,    8,    8,    8,    8,
    20,   20,   20,   20,   20,   20,   20,   20,   20},
   
tabella_limite_intervento_dissimmetria[3][9]=//%
{   15,   15,   15,   15,   15,   15,   15,   15,   15,
    12,   12,   12,   12,   12,   12,   12,   12,   12,
    25,   25,   25,   25,   25,   25,   25,   25,   25},
   
tabella_limite_segnalazione_squilibrio[3][9]=//%
{   12,   12,   12,   12,   12,   12,   12,   12,   12,
     8,    8,    8,    8,    8,    8,    8,    8,    8,
    20,   20,   20,   20,   20,   20,   20,   20,   20},
   
tabella_limite_intervento_squilibrio[3][9]=//%
{   15,   15,   15,   15,   15,   15,   15,   15,   15,
    12,   12,   12,   12,   12,   12,   12,   12,   12,
    25,   25,   25,   25,   25,   25,   25,   25,   25},

tabella_tensione_nominale[3][15]=//V
{  400,  400,  400,  400,  400,  400,  400,  400,  400,            230,  230,  230,  230,  230,  230,
   220,  220,  220,  220,  220,  220,  220,  220,  220,            110,  110,  110,  110,  110,  110,
   460,  460,  460,  460,  460,  460,  460,  460,  460,            260,  260,  260,  260,  260,  260},

tabella_corrente_nominale[3][15]=//A*10
{   10,   20,   25,   30,   34,   55,   70,   95,  140,             25,   50,   65,   80,  100,  150,
     5,   10,   12,   20,   20,   30,   50,   60,  100,             20,   30,   50,   65,   80,  110,
    20,   30,   40,   50,   55,   80,  110,  150,  200,             60,  100,  130,  160,  200,  300},

tabella_limite_sovratensione[3][15]=//%
{  115,  115,  115,  115,  115,  115,  115,  115,  115,            115,  115,  115,  115,  115,  115,
   106,  106,  106,  106,  106,  106,  106,  106,  106,            106,  106,  106,  106,  106,  106,
   125,  125,  125,  125,  125,  125,  125,  125,  125,            125,  125,  125,  125,  125,  125},
   
tabella_limite_sottotensione[3][15]=//%
{   88,   88,   88,   88,   88,   88,   88,   88,   88,             88,   88,   88,   88,   88,   88,
    80,   80,   80,   80,   80,   80,   80,   80,   80,             80,   80,   80,   80,   80,   80,
    90,   90,   90,   90,   90,   90,   90,   90,   90,             90,   90,   90,   90,   90,   90},
   
tabella_tensione_restart[3][15]=//%
{   92,   92,   92,   92,   92,   92,   92,   92,   92,             92,   92,   92,   92,   92,   92,
    80,   80,   80,   80,   80,   80,   80,   80,   80,             80,   80,   80,   80,   80,   80,
   100,  100,  100,  100,  100,  100,  100,  100,  100,            100,  100,  100,  100,  100,  100},
   
tabella_limite_sovracorrente[3][15]=//%
{  115,  115,  115,  115,  115,  115,  115,  115,  115,            115,  115,  115,  115,  115,  115,
   106,  106,  106,  106,  106,  106,  106,  106,  106,            106,  106,  106,  106,  106,  106,
   125,  125,  125,  125,  125,  125,  125,  125,  125,            125,  125,  125,  125,  125,  125};
   
   
