//timer e contatore ore di funzionamento (salvataggio dell'ora ogni 1 minuto)
//Lettura di correnti, tensioni, sensore_pressione, temperatura_motore, rel� differenziale, tensione_logica
//lettura memoria EEPROM tramite SPI
//limitazione della pressione

//tipo intervento:
// 0 OFF senza intervento protezioni     
// 1 ON 

//senza arresto 
// 10 sovracorrente
// 11 sovratensione
// 12 sottotensione
// 13 mandata chiusa
// 14 funzionamento a secco
// 15 sovratemperatura della pompa
// 16 protezione differenziale
// 17 squilibrio di corrente
// 18 dissimmetria delle tensioni
// 19 pressione emergenza

//con arresto
// 20 sovracorrente
// 21 sovratensione
// 22 sottotensione
// 23 mandata chiusa
// 24 funzionamento a secco
// 25 sovratemperatura della pompa
// 26 protezione differenziale
// 27 squilibrio di corrente
// 28 dissimmetria delle tensioni
// 29 allarme sensore pressione
// 30 pressione emergenza
// 31 protezione Icc
//ogni valore diverso non viene considerato
 
//16 bytes trasmessi per un complesso di 8K 504 registrazioni
// 1 byte = tipo intervento  all'indirizzo binario xxxx xxxx xxxx 0000
// 4 bytes = ora_minuto_secondo  xxxxx.xx.xx  (viene registrata come numero totale di secondi di funzionamento)
// 2 bytes = tensione_media 0-500V
// 1 byte = I1_rms  0-25.5A
// 1 byte = I2_rms  0-25.5A
// 1 byte = I3_rms  0-25.5A
// 2 bytes = potenza  0-10000 W
// 2 bytes = pressione  -1.0 - +50.0 Bar
// 1 byte  = cosfi convenzionale =P/(P^2+Q^2) 0-.99
// 1 byte = temperatura  0-255 �C

//funzioni:
//col pulsante FUNC si leggono le funzioni in successione crescente
//coi pulsanti + e - si cambia il valore
// i comandi ON OFF vengono memorizzati e ripresi al ritorno della tensione

// presentazioni con motore in ON:
// tensione, potenza, pressione, corrente1, corrente2, corrente3,
// alternata con:
// temperatura, potenza, pressione, cosfi1, cosfi2, cosfi3,

const int
N_tabella=0,//0=trifase 5.5KW, 1=trifase 2.2KW, 2=monofase 2.2KW, 3=?
operazione_effettuata=11000,
lettura_tarature=10000,//da dare alla variabile allarme_in_lettura per la lettura delle tarature
lettura_misure=9000;//da dare alla variabile allarme_in_lettura per la lettura delle misure istantanee
const char
N_tentativi_motore_bloccato=5;

__interrupt void AD_conversion_completeISR(void);
__interrupt void timer1_overflowISR(void); //timer 100 us per relais
__interrupt void timer2_overflowISR(void); //1 ms
__interrupt void SPI1(void);
__interrupt void usb_it_handler(void);

void calcolo_tensione_relais(void);
void lettura_pulsanti(void);
void comando_del_display(void);
void media_quadrati_nel_periodo(unsigned long *quadrato, unsigned long dividendo, char divisore);
int media_potenza_nel_periodo(long dividendo, int fattore, char divisore);
void condizioni_iniziali(void);
void somma_quadrati(unsigned long *somma_pot, int var);
void somma_potenze(long *somma, int x, int y);
void calcolo_offset_misure(unsigned long *offset, int misura);
void calcolo_valor_medio_potenze(long *media_potenza, int potenza); 
void calcolo_medie_quadrati(unsigned long *media_quad, unsigned long quad);//160ms
void valore_efficace(int *efficace, unsigned long quad, int costante);
char calcolo_del_fattore_di_potenza(int attiva, int reattiva);
char calcolo_del_fattore_di_potenza_monofase(int attiva, int tensione, int corrente);
void presenta_menu(unsigned char *menu, char idioma, char lunghezza , char riga_inizio);
void modifica_unsigned(unsigned int *var, unsigned int min, unsigned int max, char velocita);
void presenta_unsigned(unsigned int val, char punto, char riga, char colonna, char n_cifre);
void presenta_signed(int val, char punto, char riga, char colonna, char n_cifre);
void presenta_scritta(char *stringa, char offset, char idioma, char lunghezza, char riga,char colonna, char N_caratteri);
void calcolo_delle_costanti(void);
void programmazione(void);
void misure_medie(void);
void Nallarme_ora_minuto_secondo(long secondi, char numero);
void messaggio_allarme(char indentificazione);
void presenta_stato_motore(void);
void protezione_termica(void);
void disinserzione_condensatore_avviamento(void);
void condizioni_di_allarme(void);
void marcia_arresto(void);
void trasmissione_misure_istantanee(void); //con allarme_in_lettura = 9000
void trasmissione_tarature(void);//con allarme_in_lettura == 10000

void salva_reset_default(void);
void salva_impostazioni(void);
void salva_allarme_in_eepprom(void);
void salva_conta_secondi_attivita(void);
void lettura_impostazioni(void);
void lettura_allarme_messo_in_buffer_USB(void);//con allarme_in_lettura <totale_indicazioni_fault

void USB_comm_process(void);
void cdc_process(void);
  
void main(void);

//#include <hidef.h> /* for EnableInterrupts macro */
//#include "derivative.h" /* include peripheral declarations */
#include <MC9S08JM60.h>
#include "USB_man.h"

const unsigned char
presentazione_iniziale[2][17]={
"USB PUMP CONTROL",
"BOX          V01"},

abilitazione_OFF[2][17]={
"Abilitazione OFF",
"Not enable      "},

accesso_password[2][17]={
"ACCESO ABILITATO",
"ACCES ENABLED   "},

comandi_da_effettuare[2][2][17]={
"FUNC: tarature  ",
"START:avviamento",

"F:functions Menu",//inglese
"START: pump ON  "},

dati_presentati_in_ON[4][2][17]={
"   V,     W    B",//monofase
' ',' ',' ',' ','A',' ',' ',' ',' ','P','F',' ',' ',' ',223,'C',0,
"   V,     W    B",//monofase alternato
' ',' ',' ',' ','A',' ',' ',' ',' ','P','F',' ',' ',' ',223,'C',0,

"   V,     W    B",//trifase 
"    A    A     A",
' ',' ',' ',223,'C',' ',' ',' ',' ',' ','W',' ',' ',' ',' ','B',0,//trifase alternato
".  PF .  PF.  PF"},

lettura_allarmi[2][25][17]={
"N:  ,     -  -  ",// numero allarme=2cifre, ora=5cifre minuto=2cifre secondo=2cifre
"      OFF       ",
"      ON        ",
"Sovracorrente ON",
"Sovratensione ON",
"Sottotensione ON",
"MandataChiusa ON",
"Funzion.Secco ON",
"SovraTemper.  ON",
"Prot.Differen ON",
"Squilibrio    ON",
"Dissimmetria  ON",
"SovraPression ON",
"SovracorrenteOFF",
"SovratensioneOFF",
"SottotensioneOFF",
"MandataChiusaOFF",
"Funzion.SeccoOFF",
"SovraTemper. OFF",
"Prot.DifferenOFF",
"Squilibrio   OFF",
"Dissimmetria OFF",
"SensorePress.OFF",
"SovraPressionOFF",
"MancataPartenOFF",

"N:  ,     -  -  ",// inglese
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
"ShortCircuit OFF"},

presenta_monofase_trifase[2][2][17]={
"  Monofase      ",
"  Trifase       ",

"Single Phase    ",
"Three Phase     "},

presenta_tipo_start_stop[2][2][17]={
"  Manuale       ",
"  Pressione     ",

"  Manual        ",
"Pressure  Switch"},

comando_reset[2][17]={
"   YES?         ",
"   NOT?         "},

in_salvataggio[2][17]={
"    Save        ",
"    Data        "},

comando_lingua[2][17]={
"Italic/Italiano ",
"English/Inglese "},

menu_principale[2][80][17]={//Menu' principale: elenco delle funzioni di taratura
"1) Number       ",//0 italiano
"Password:       ",
"2)Single/Three  ",//1
"                ",
"3)Nomin. Power  ",//2
"              kW",
"4)Nomin. voltage",//3
"               V",
"5)Nomin. Current",//4
"               A",
"6)Over Voltage  ",//5
"Limit:         %",
"7)Under Voltage ",//6
"Limit:         %",   
"8)MinimumVoltage",//7
"Restart:       %",
"9)Diss. Voltage ",//8
"Alarm:         %",
"10)Diss.Voltage ",//9
"stop           %",
"11)Error Voltage",//10
"time:          s",
"12)Over Current ",//11
"Limit:         %",
"13)Unbalance I  ",//12
"Alarm:         %",
"14)Unbalance I  ",//13
"Stop:          %",
"15)Error Current",//14
"time:          s",
"16)Leakage Curr.",//15
"limit:        mA",
"17)Leakage Curr.",//16
"time stop:     s",
"18)PressureSens.",//17
"                ",
"19)Max pressure ",//18
"Alarm:       Bar",
"20)Max Pressure ",//19
"Stop:        Bar",
"21)ReStart Pres-",//20
"sure:        Bar",
"22)ReStart delay",//21
"             min",
"23)MinFlow Power",//22
"stop:          %",    
"24)MinFlow Stop ",//23
"delay:         s",
"25)Delay MinFlow",//24
"Restart:       s",
"26)Min Power dry",//25
"working:       %",
"27)Delay DryWork",//26
"stop:          s",
"28)Pressure     ",//27
"Range:         B",
"29)Manual/Press.",//28
"                ",
"30)Id range:    ",//29
"           mA/mA",
"31)K temp motor ",//30
"               s",
"32)Range sensor ",//31
'T','e','m','p','e','r',':',' ',' ',' ',' ','m','V','/',223,'C',0,
"33)Ambient      ",//32
'T','e','m','p','e','r','a','t','u','r','e',' ',' ',' ',223,'C',0,
"34)Stop Limit   ",//33
'T','e','m','p','e','r','a','t','u','r','e',' ',' ',' ',223,'C',0,
"35)Alarm History",//34
"                ",
"36)Serial number",//35
"                ",
"37)Reset        ",//36
"                ",    
"38)Calib.I1:    ",//37
"I1:    ,I3:     ",    
"39)Calib.I3:    ",//38
"I1:    ,I3:     ",    
"40)Calib.I2:    ",//39
"I1:    ,I2:     ",   

"1) Number       ",//0 inglese
"Password:       ",
"2)Single/Three  ",//1
"                ",
"3)Nomin. Power  ",//2
"              kW",
"4)Nomin. voltage",//3
"               V",
"5)Nomin. Current",//4
"               A",
"6)Over Voltage  ",//5
"Limit:         %",
"7)Under Voltage ",//6
"Limit:         %",   
"8)MinimumVoltage",//7
"Restart:       %",
"9)Diss. Voltage ",//8
"Alarm:         %",
"10)Diss.Voltage ",//9
"stop           %",
"11)Error Voltage",//10
"time:          s",
"12)Over Current ",//11
"Limit:         %",
"13)Unbalance I  ",//12
"Alarm:         %",
"14)Unbalance I  ",//13
"Stop:          %",
"15)Error Current",//14
"time:          s",
"16)Leakage Curr.",//15
"limit:        mA",
"17)Leakage Curr.",//16
"time stop:     s",
"18)PressureSens.",//17
"                ",
"19)Max pressure ",//18
"Alarm:       Bar",
"20)Max Pressure ",//19
"Stop:        Bar",
"21)ReStart Pres-",//20
"sure:        Bar",
"22)ReStart delay",//21
"             min",
"23)MinFlow Power",//22
"stop:          %",    
"24)MinFlow Stop ",//23
"delay:         s",
"25)Delay MinFlow",//24
"Restart:       s",
"26)Min Power dry",//25
"working:       %",
"27)Delay DryWork",//26
"stop:          s",
"28)Pressure     ",//27
"Range:         B",
"29)Manual/Press.",//28
"                ",
"30)Id range:    ",//29
"           mA/mA",
"31)K temp motor ",//30
"               s",
"32)Range sensor ",//31
'T','e','m','p','e','r',':',' ',' ',' ',' ','m','V','/',223,'C',0,
"33)Ambient      ",//32
'T','e','m','p','e','r','a','t','u','r','e',' ',' ',' ',223,'C',0,
"34)Stop Limit   ",//33
'T','e','m','p','e','r','a','t','u','r','e',' ',' ',' ',223,'C',0,
"35)Alarm History",//34
"                ",
"36)Serial number",//35
"                ",
"37)Reset        ",//36
"                ",    
"38)Calib.I1:    ",//37
"I1:    ,I3:     ",    
"39)Calib.I3:    ",//38
"I1:    ,I3:     ",    
"40)Calib.I2:    ",//39
"I1:    ,I2:     "};   

const char comando_AD[23]={
0x40,0x42,0x44,0x41,0x43,0x44,
0x43,0x41,0x44,0x42,0x40,0x44,
0x40,0x43,0x45,0x41,0x42,0x46,
0x42,0x41,0x47,0x43,0x40},
filtro_pulsanti=21,
prepara[7]={0x01,0x02,0x06,0x0c,0x14,0x3c,0x01};

const long
primo_indirizzo_funzioni=0x1f44000,//128064 x 256,
indirizzo_conta_ore=0x1f48e00,//128142 x 256,//salva motore_on, numero_segnalazione, conta_ore
ultimo_indirizzo_funzioni=0x1f4a000;//128160 x 256;

const int
delta_Tn=80, //sovra_temperatura limite con corrente nominale = 80�C
delta_T_riavviamento=60, //�C
fattore_portata=1966,//16*.15/5*4096 DAC portata
durata_avviamento=2000, inizio_lettura_I_avviamento=1950,
durata_cc=3000,
tempo_eccitazione_relais=500,//.5s
chiave_ingresso=541, chiave_numero_serie=11,
totale_indicazioni_fault=8000,//totale delle registrazioni salvate
disturbo_I_pos=54,
disturbo_I_neg=-54,
emergenza_sensore_pressione=-20,//Bar*10
/*
organizzazione della eeprom
0-8039 salvataggio degli allarmi
8064-8160 dati di funzionamento
*/

dati_di_fabbrica[4][48]={
//0=trifase 5.5KW,
1,    //numero_serie,
1,    //monofase_trifase, 0=monofase, 1=trifase
550,  //potenza_nominale, W*10
400,  //tensione_nominale, V
115,  //limite_sovratensione %
85,   //limite_sottotensione %
90,   //tensione_restart %
10,   //limite_segnalazione_dissimmetria %
15,   //limite_intervento_dissimmetria %
30,   //timeout_protezione_tensione s
137,  //corrente_nominale A*10
115,  //limite_sovracorrente %
10,   //limite_segnalazione_squilibrio %
15,   //limite_intervento_squilibrio %
60,   //ritardo_protezione_squilibrio s
60,   //costante_tau_salita_temperatura s
20,   //taratura_temperatura_ambiente �C //
100,  //scala_temperatura_motore,//mV/�C
100,  //limite_intervento_temper_motore,//�C //35
100,  //scala_corrente_differenziale  mA in /mA out
300,  //limite_corrente_differenziale,//mA
30,   //ritardo_intervento_differenziale,//ms
10,   //ritardo_funzionamento_dopo_emergenza,//min
160,  //portata_sensore_pressione BAR*10
120,  //pressione_emergenza Bar*10
70,   //potenza_minima_mandata_chiusa %
20,   //ritardo_stop_mandata_chiusa //s
50,   //potenza_minima_funz_secco %
40,   //ritardo_stop_funzionemento_a_secco
10,   //ritardo_riaccensione_mandata_chiusa //s
0,    //modo_start_stop,//0=remoto o 1=pressione
30,   //pressione_accensione BAR*10
50,   //pressione_spegnimento BAR*10
1,    //lingua
20,   //temperatura_ambiente  DAC
0,    //abilita_sensore_pressione
128,  //calibrazione_I1,    
128,  //calibrazione_I2,    
128,  //calibrazione_I3,    
0,    //motore_on
0,    //numero_segnalazione
0,0,  //conta_ore[2]
0,0,  //conta_ore_funzionamento[2]   
0,0,0, //riserva[3]

//1=trifase 2.2KW,
1,    //numero_serie,
1,    //monofase_trifase, 0=monofase, 1=trifase
220,  //potenza_nominale, W*10
400,  //tensione_nominale, V
115,  //limite_sovratensione %
85,   //limite_sottotensione %
90,   //tensione_restart %
10,   //limite_segnalazione_dissimmetria %
15,   //limite_intervento_dissimmetria %
30,   //timeout_protezione_tensione s
56,   //corrente_nominale A*10
115,  //limite_sovracorrente %
10,   //limite_segnalazione_squilibrio %
15,   //limite_intervento_squilibrio %
60,   //ritardo_protezione_squilibrio s
60,   //costante_tau_salita_temperatura s
20,   //taratura_temperatura_ambiente �C //
100,  //scala_temperatura_motore,//mV/�C
100,  //limite_intervento_temper_motore,//�C //35
100,  //scala_corrente_differenziale  mA in /mA out
300,  //limite_corrente_differenziale,//mA
30,   //ritardo_intervento_differenziale,//ms
10,   //ritardo_funzionamento_dopo_emergenza,//min
160,  //portata_sensore_pressione BAR*10
120,  //pressione_emergenza Bar*10
70,   //potenza_minima_mandata_chiusa %
20,   //ritardo_stop_mandata_chiusa //s
50,   //potenza_minima_funz_secco %
40,   //ritardo_stop_funzionemento_a_secco
10,   //ritardo_riaccensione_mandata_chiusa //s
0,    //modo_start_stop,//0=remoto o 1=pressione
30,   //pressione_accensione BAR*10
50,   //pressione_spegnimento BAR*10
1,    //lingua
20,   //temperatura_ambiente  DAC
0,    //abilita_sensore_pressione
128,  //calibrazione_I1,    
128,  //calibrazione_I2,    
128,  //calibrazione_I3,    
0,    //motore_on
0,    //numero_segnalazione
0,0,  //conta_ore[2]
0,0,  //conta_ore_funzionamento[2]   
0,0,0, //riserva[3]

//2=monofase 2.2KW,
1,    //numero_serie,
0,    //monofase_trifase, 0=monofase, 1=trifase
220,  //potenza_nominale, W*10
230,  //tensione_nominale, V
115,  //limite_sovratensione %
85,   //limite_sottotensione %
90,   //tensione_restart %
10,   //limite_segnalazione_dissimmetria %
15,   //limite_intervento_dissimmetria %
30,   //timeout_protezione_tensione s
150,  //corrente_nominale A*10
115,  //limite_sovracorrente %
10,   //limite_segnalazione_squilibrio %
15,   //limite_intervento_squilibrio %
60,   //ritardo_protezione_squilibrio s
60,   //costante_tau_salita_temperatura s
20,   //taratura_temperatura_ambiente �C //
100,  //scala_temperatura_motore,//mV/�C
100,  //limite_intervento_temper_motore,//�C //35
100,  //scala_corrente_differenziale  mA in /mA out
300,  //limite_corrente_differenziale,//mA
30,   //ritardo_intervento_differenziale,//ms
10,   //ritardo_funzionamento_dopo_emergenza,//min
160,  //portata_sensore_pressione BAR*10
120,  //pressione_emergenza Bar*10
70,   //potenza_minima_mandata_chiusa %
20,   //ritardo_stop_mandata_chiusa //s
50,   //potenza_minima_funz_secco %
40,   //ritardo_stop_funzionemento_a_secco
10,   //ritardo_riaccensione_mandata_chiusa //s
0,    //modo_start_stop,//0=remoto o 1=pressione
30,   //pressione_accensione BAR*10
50,   //pressione_spegnimento BAR*10
1,    //lingua
20,   //temperatura_ambiente  DAC
0,    //abilita_sensore_pressione
128,  //calibrazione_I1,    
128,  //calibrazione_I2,    
128,  //calibrazione_I3,    
0,    //motore_on
0,    //numero_segnalazione
0,0,  //conta_ore[2]
0,0,  //conta_ore_funzionamento[2]   
0,0,0, //riserva[3]

//3=?
1,    //numero_serie,
1,    //monofase_trifase, 0=monofase, 1=trifase
550,  //potenza_nominale, W*10
400,  //tensione_nominale, V
115,  //limite_sovratensione %
85,   //limite_sottotensione %
90,   //tensione_restart %
10,   //limite_segnalazione_dissimmetria %
15,   //limite_intervento_dissimmetria %
30,   //timeout_protezione_tensione s
130,  //corrente_nominale A*10
115,  //limite_sovracorrente %
10,   //limite_segnalazione_squilibrio %
15,   //limite_intervento_squilibrio %
60,   //ritardo_protezione_squilibrio s
60,   //costante_tau_salita_temperatura s
20,   //taratura_temperatura_ambiente �C //
100,  //scala_temperatura_motore,//mV/�C
100,  //limite_intervento_temper_motore,//�C //35
100,  //scala_corrente_differenziale  mA in /mA out
300,  //limite_corrente_differenziale,//mA
30,   //ritardo_intervento_differenziale,//ms
10,   //ritardo_funzionamento_dopo_emergenza,//min
160,  //portata_sensore_pressione BAR*10
120,  //pressione_emergenza Bar*10
70,   //potenza_minima_mandata_chiusa %
20,   //ritardo_stop_mandata_chiusa //s
50,   //potenza_minima_funz_secco %
40,   //ritardo_stop_funzionemento_a_secco
10,   //ritardo_riaccensione_mandata_chiusa //s
0,    //modo_start_stop,//0=remoto o 1=pressione
30,   //pressione_accensione BAR*10
50,   //pressione_spegnimento BAR*10
1,    //lingua
20,   //temperatura_ambiente  DAC
0,    //abilita_sensore_pressione
128,  //calibrazione_I1,    
128,  //calibrazione_I2,    
128,  //calibrazione_I3,    
0,    //motore_on
0,    //numero_segnalazione
0,0,  //conta_ore[2]
0,0,  //conta_ore_funzionamento[2]   
0,0,0 //riserva[3]
},

limiti_inferiori[4][48]={
//0=trifase 5.5KW,
1,    //numero_serie,
0,    //monofase_trifase, 0=monofase, 1=trifase
75,   //potenza_nominale, W*10
110,  //tensione_nominale, V
100,  //limite_sovratensione %
60,   //limite_sottotensione %
60,   //tensione_restart %
5,    //limite_segnalazione_dissimmetria %
5,    //limite_intervento_dissimmetria %
2,    //timeout_protezione_tensione s
10,   //corrente_nominale A*10
105,  //limite_sovracorrente %
1,    //limite_segnalazione_squilibrio %
1,    //limite_intervento_squilibrio %
1,    //ritardo_protezione_squilibrio s
10,   //costante_tau_salita_temperatura s
5,    //taratura_temperatura_ambiente �C
2,    //scala_temperatura_motore,//mV/�C
70,   //limite_intervento_temper_motore,//�C
1,    //scala_corrente_differenziale  mA in /mA out
10,   //limite_corrente_differenziale,//mA
20,   //ritardo_intervento_differenziale,//ms
1,    //ritardo_funzionamento_dopo_emergenza,//min
40,   //portata_sensore_pressione BAR*10
40,   //pressione_emergenza Bar*10
10,   //potenza_minima_mandata_chiusa %
5,    //ritardo_stop_mandata_chiusa 
10,   //potenza_minima_funz_secco %
5,    //ritardo_stop_funzionemento_a_secco
3,    //ritardo_riaccensione_mandata_chiusa //s
0,    //modo_start_stop,//0=remoto o 1=pressione
5,    //pressione_accensione BAR*10
10,   //pressione_spegnimento BAR*10
0,    //lingua
0,    //temperatura_ambiente DAC
0,    //abilita_sensore_pressione
115,  //calibrazione_I1,    
115,  //calibrazione_I2,    
115,  //calibrazione_I3,    
0,    //motore_on
0,    //numero_segnalazione
0,0,  //conta_ore[2]   
0,0,  //conta_ore_funzionamento[2]   
0,0,0, //riserva[3]

//1=trifase 2.2KW,
1,    //numero_serie,
0,    //monofase_trifase, 0=monofase, 1=trifase
75,   //potenza_nominale, W*10
110,  //tensione_nominale, V
100,  //limite_sovratensione %
60,   //limite_sottotensione %
60,   //tensione_restart %
5,    //limite_segnalazione_dissimmetria %
5,    //limite_intervento_dissimmetria %
2,    //timeout_protezione_tensione s
10,   //corrente_nominale A*10
105,  //limite_sovracorrente %
1,    //limite_segnalazione_squilibrio %
1,    //limite_intervento_squilibrio %
1,    //ritardo_protezione_squilibrio s
10,   //costante_tau_salita_temperatura s
5,    //taratura_temperatura_ambiente �C
2,    //scala_temperatura_motore,//mV/�C
70,   //limite_intervento_temper_motore,//�C
1,    //scala_corrente_differenziale  mA in /mA out
10,   //limite_corrente_differenziale,//mA
20,   //ritardo_intervento_differenziale,//ms
1,    //ritardo_funzionamento_dopo_emergenza,//min
40,   //portata_sensore_pressione BAR*10
40,   //pressione_emergenza Bar*10
10,   //potenza_minima_mandata_chiusa %
5,    //ritardo_stop_mandata_chiusa 
10,   //potenza_minima_funz_secco %
5,    //ritardo_stop_funzionemento_a_secco
3,    //ritardo_riaccensione_mandata_chiusa //s
0,    //modo_start_stop,//0=remoto o 1=pressione
5,    //pressione_accensione BAR*10
10,   //pressione_spegnimento BAR*10
0,    //lingua
0,    //temperatura_ambiente DAC
0,    //abilita_sensore_pressione
115,  //calibrazione_I1,    
115,  //calibrazione_I2,    
115,  //calibrazione_I3,    
0,    //motore_on
0,    //numero_segnalazione
0,0,  //conta_ore[2]   
0,0,  //conta_ore_funzionamento[2]   
0,0,0, //riserva[3]

//2=monofase 2.2KW,
1,    //numero_serie,
0,    //monofase_trifase, 0=monofase, 1=trifase
75,   //potenza_nominale, W*10
110,  //tensione_nominale, V
100,  //limite_sovratensione %
60,   //limite_sottotensione %
60,   //tensione_restart %
5,    //limite_segnalazione_dissimmetria %
5,    //limite_intervento_dissimmetria %
2,    //timeout_protezione_tensione s
10,   //corrente_nominale A*10
105,  //limite_sovracorrente %
1,    //limite_segnalazione_squilibrio %
1,    //limite_intervento_squilibrio %
1,    //ritardo_protezione_squilibrio s
10,   //costante_tau_salita_temperatura s
5,    //taratura_temperatura_ambiente �C
2,    //scala_temperatura_motore,//mV/�C
70,   //limite_intervento_temper_motore,//�C
1,    //scala_corrente_differenziale  mA in /mA out
10,   //limite_corrente_differenziale,//mA
20,   //ritardo_intervento_differenziale,//ms
1,    //ritardo_funzionamento_dopo_emergenza,//min
40,   //portata_sensore_pressione BAR*10
40,   //pressione_emergenza Bar*10
10,   //potenza_minima_mandata_chiusa %
5,    //ritardo_stop_mandata_chiusa 
10,   //potenza_minima_funz_secco %
5,    //ritardo_stop_funzionemento_a_secco
3,    //ritardo_riaccensione_mandata_chiusa //s
0,    //modo_start_stop,//0=remoto o 1=pressione
5,    //pressione_accensione BAR*10
10,   //pressione_spegnimento BAR*10
0,    //lingua
0,    //temperatura_ambiente DAC
0,    //abilita_sensore_pressione
115,  //calibrazione_I1,    
115,  //calibrazione_I2,    
115,  //calibrazione_I3,    
0,    //motore_on
0,    //numero_segnalazione
0,0,  //conta_ore[2]   
0,0,  //conta_ore_funzionamento[2]   
0,0,0, //riserva[3]

//3=?
1,    //numero_serie,
0,    //monofase_trifase, 0=monofase, 1=trifase
75,   //potenza_nominale, W*10
110,  //tensione_nominale, V
100,  //limite_sovratensione %
60,   //limite_sottotensione %
60,   //tensione_restart %
5,    //limite_segnalazione_dissimmetria %
5,    //limite_intervento_dissimmetria %
2,    //timeout_protezione_tensione s
10,   //corrente_nominale A*10
105,  //limite_sovracorrente %
1,    //limite_segnalazione_squilibrio %
1,    //limite_intervento_squilibrio %
1,    //ritardo_protezione_squilibrio s
10,   //costante_tau_salita_temperatura s
5,    //taratura_temperatura_ambiente �C
2,    //scala_temperatura_motore,//mV/�C
70,   //limite_intervento_temper_motore,//�C
1,    //scala_corrente_differenziale  mA in /mA out
10,   //limite_corrente_differenziale,//mA
20,   //ritardo_intervento_differenziale,//ms
1,    //ritardo_funzionamento_dopo_emergenza,//min
40,   //portata_sensore_pressione BAR*10
40,   //pressione_emergenza Bar*10
10,   //potenza_minima_mandata_chiusa %
5,    //ritardo_stop_mandata_chiusa 
10,   //potenza_minima_funz_secco %
5,    //ritardo_stop_funzionemento_a_secco
3,    //ritardo_riaccensione_mandata_chiusa //s
0,    //modo_start_stop,//0=remoto o 1=pressione
5,    //pressione_accensione BAR*10
10,   //pressione_spegnimento BAR*10
0,    //lingua
0,    //temperatura_ambiente DAC
0,    //abilita_sensore_pressione
115,  //calibrazione_I1,    
115,  //calibrazione_I2,    
115,  //calibrazione_I3,    
0,    //motore_on
0,    //numero_segnalazione
0,0,  //conta_ore[2]   
0,0,  //conta_ore_funzionamento[2]   
0,0,0 //riserva[3]
},

limiti_superiori[4][48]={
//0=trifase 5.5KW,
65535,//numero_serie,
1,    //monofase_trifase, 0=monofase, 1=trifase
550,  //potenza_nominale, W*10
440,  //tensione_nominale, V
125,  //limite_sovratensione %
95,   //limite_sottotensione %
100,  //tensione_restart %
20,   //limite_segnalazione_dissimmetria %
25,   //limite_intervento_dissimmetria %
120,  //timeout_protezione_tensione s
160,  //corrente_nominale A*10
140,  //limite_sovracorrente %
20,   //limite_segnalazione_squilibrio %
25,   //limite_intervento_squilibrio %
120,  //ritardo_protezione_squilibrio s
180,  //costante_tau_salita_temperatura s
40,   //taratura_temperatura_ambiente �C
250,  //scala_temperatura_motore,//mV/�C
150,  //limite_intervento_temper_motore,//�C
250,  //scala_corrente_differenziale  mA in /mA out
500,  //limite_corrente_differenziale,//mA
200,  //ritardo_intervento_differenziale,//ms
99,   //ritardo_funzionamento_dopo_emergenza,//min
500,  //portata_sensore_pressione BAR*10
450,  //pressione_emergenza Bar*10
100,  //potenza_minima_mandata_chiusa %
120,  //ritardo_stop_mandata_chiusa 
100,  //potenza_minima_funz_secco %
120,  //ritardo_stop_funzionemento_a_secco
999,  //ritardo_riaccensione_mandata_chiusa //s
1,    //modo_start_stop,//0=remoto o 1=pressione
400,  //pressione_accensione BAR*10
400,  //pressione_spegnimento BAR*10
1,    //lingua
1000, //temperatura_ambiente DAC
1,    //abilita_sensore_pressione
141,  //calibrazione_I1,    
141,  //calibrazione_I2,    
141,  //calibrazione_I3,    
1,    //motore_on
503,  //numero_segnalazione
65535,65535,  //conta_ore[2]   
65535,65535,  //conta_ore_funzionamento[2]   
0,0,0, //riserva[3]

//1=trifase 2.2KW,
65535,//numero_serie,
1,    //monofase_trifase, 0=monofase, 1=trifase
550,  //potenza_nominale, W*10
440,  //tensione_nominale, V
125,  //limite_sovratensione %
95,   //limite_sottotensione %
100,  //tensione_restart %
20,   //limite_segnalazione_dissimmetria %
25,   //limite_intervento_dissimmetria %
120,  //timeout_protezione_tensione s
160,  //corrente_nominale A*10
140,  //limite_sovracorrente %
20,   //limite_segnalazione_squilibrio %
25,   //limite_intervento_squilibrio %
120,  //ritardo_protezione_squilibrio s
180,  //costante_tau_salita_temperatura s
40,   //taratura_temperatura_ambiente �C
250,  //scala_temperatura_motore,//mV/�C
150,  //limite_intervento_temper_motore,//�C
250,  //scala_corrente_differenziale  mA in /mA out
500,  //limite_corrente_differenziale,//mA
200,  //ritardo_intervento_differenziale,//ms
99,   //ritardo_funzionamento_dopo_emergenza,//min
500,  //portata_sensore_pressione BAR*10
450,  //pressione_emergenza Bar*10
100,  //potenza_minima_mandata_chiusa %
120,  //ritardo_stop_mandata_chiusa 
100,  //potenza_minima_funz_secco %
120,  //ritardo_stop_funzionemento_a_secco
999,  //ritardo_riaccensione_mandata_chiusa //s
1,    //modo_start_stop,//0=remoto o 1=pressione
400,  //pressione_accensione BAR*10
400,  //pressione_spegnimento BAR*10
1,    //lingua
1000, //temperatura_ambiente DAC
1,    //abilita_sensore_pressione
141,  //calibrazione_I1,    
141,  //calibrazione_I2,    
141,  //calibrazione_I3,    
1,    //motore_on
503,  //numero_segnalazione
65535,65535,  //conta_ore[2]   
65535,65535,  //conta_ore_funzionamento[2]   
0,0,0, //riserva[3]

//2=monofase 2.2KW,
65535,//numero_serie,
1,    //monofase_trifase, 0=monofase, 1=trifase
550,  //potenza_nominale, W*10
440,  //tensione_nominale, V
125,  //limite_sovratensione %
95,   //limite_sottotensione %
100,  //tensione_restart %
20,   //limite_segnalazione_dissimmetria %
25,   //limite_intervento_dissimmetria %
120,  //timeout_protezione_tensione s
160,  //corrente_nominale A*10
140,  //limite_sovracorrente %
20,   //limite_segnalazione_squilibrio %
25,   //limite_intervento_squilibrio %
120,  //ritardo_protezione_squilibrio s
180,  //costante_tau_salita_temperatura s
40,   //taratura_temperatura_ambiente �C
250,  //scala_temperatura_motore,//mV/�C
150,  //limite_intervento_temper_motore,//�C
250,  //scala_corrente_differenziale  mA in /mA out
500,  //limite_corrente_differenziale,//mA
200,  //ritardo_intervento_differenziale,//ms
99,   //ritardo_funzionamento_dopo_emergenza,//min
500,  //portata_sensore_pressione BAR*10
450,  //pressione_emergenza Bar*10
100,  //potenza_minima_mandata_chiusa %
120,  //ritardo_stop_mandata_chiusa 
100,  //potenza_minima_funz_secco %
120,  //ritardo_stop_funzionemento_a_secco
999,  //ritardo_riaccensione_mandata_chiusa //s
1,    //modo_start_stop,//0=remoto o 1=pressione
400,  //pressione_accensione BAR*10
400,  //pressione_spegnimento BAR*10
1,    //lingua
1000, //temperatura_ambiente DAC
1,    //abilita_sensore_pressione
141,  //calibrazione_I1,    
141,  //calibrazione_I2,    
141,  //calibrazione_I3,    
1,    //motore_on
503,  //numero_segnalazione
65535,65535,  //conta_ore[2]   
65535,65535,  //conta_ore_funzionamento[2]   
0,0,0, //riserva[3]

//3=?
65535,//numero_serie,
1,    //monofase_trifase, 0=monofase, 1=trifase
550,  //potenza_nominale, W*10
440,  //tensione_nominale, V
125,  //limite_sovratensione %
95,   //limite_sottotensione %
100,  //tensione_restart %
20,   //limite_segnalazione_dissimmetria %
25,   //limite_intervento_dissimmetria %
120,  //timeout_protezione_tensione s
160,  //corrente_nominale A*10
140,  //limite_sovracorrente %
20,   //limite_segnalazione_squilibrio %
25,   //limite_intervento_squilibrio %
120,  //ritardo_protezione_squilibrio s
180,  //costante_tau_salita_temperatura s
40,   //taratura_temperatura_ambiente �C
250,  //scala_temperatura_motore,//mV/�C
150,  //limite_intervento_temper_motore,//�C
250,  //scala_corrente_differenziale  mA in /mA out
500,  //limite_corrente_differenziale,//mA
200,  //ritardo_intervento_differenziale,//ms
99,   //ritardo_funzionamento_dopo_emergenza,//min
500,  //portata_sensore_pressione BAR*10
450,  //pressione_emergenza Bar*10
100,  //potenza_minima_mandata_chiusa %
120,  //ritardo_stop_mandata_chiusa 
100,  //potenza_minima_funz_secco %
120,  //ritardo_stop_funzionemento_a_secco
999,  //ritardo_riaccensione_mandata_chiusa //s
1,    //modo_start_stop,//0=remoto o 1=pressione
400,  //pressione_accensione BAR*10
400,  //pressione_spegnimento BAR*10
1,    //lingua
1000, //temperatura_ambiente DAC
1,    //abilita_sensore_pressione
141,  //calibrazione_I1,    
141,  //calibrazione_I2,    
141,  //calibrazione_I3,    
1,    //motore_on
503,  //numero_segnalazione
65535,65535,  //conta_ore[2]   
65535,65535,  //conta_ore_funzionamento[2]   
0,0,0 //riserva[3]
};

struct
{
int //dai di funzionamento a partire dall'indirizzo 8040  della eeprom
numero_serie, //0
monofase_trifase,//0-1
potenza_nominale,// W*10
tensione_nominale,//V
limite_sovratensione,//%
limite_sottotensione,//%
tensione_restart,// %
limite_segnalazione_dissimmetria,//%
limite_intervento_dissimmetria,//%
timeout_protezione_tensione,//s
corrente_nominale,//A*10
limite_sovracorrente,//%
limite_segnalazione_squilibrio,//%
limite_intervento_squilibrio,//%
ritardo_protezione_squilibrio,//s
costante_tau_salita_temperatura,//s
taratura_temperatura_ambiente,//da modificare per la taratura
scala_temperatura_motore,//mV/�C
limite_intervento_temper_motore,//�C
scala_corrente_differenziale, //mA in /mA out
limite_corrente_differenziale,//mA
ritardo_intervento_differenziale,//ms
ritardo_funzionamento_dopo_emergenza,//min
portata_sensore_pressione,//Bar*10
pressione_emergenza,//Bar*10
potenza_minima_mandata_chiusa,//%
ritardo_stop_mandata_chiusa, //s
potenza_minima_funz_secco, //%
ritardo_stop_funzionemento_a_secco, //s
ritardo_riaccensione_mandata_chiusa,//s
modo_start_stop,//remoto o pressione
pressione_accensione,//Bar*10
pressione_spegnimento,//Bar*10
lingua,//0-1
temperatura_ambiente,//�C
abilita_sensore_pressione,//0-1
calibrazione_I1,    
calibrazione_I2,    
calibrazione_I3,    
motore_on,//0-1         
numero_segnalazione,//0-503
conta_ore[2],
conta_ore_funzionamento[2],//s
riserva[3];
}set;

struct
{
int //ingressi analogici
I3letta,//A*.004*5/.512* 4096/5 = A*32
I1letta,//A*.004*5/.512* 4096/5 = A*32
V13letta,//V* 33/680/6 /2 *4096/5 = V*3.313
V12letta,//V* 33/680/6 /2 *4096/5 = V*3.313
Idletta,//corrente differenziale
sensore_pressione,//V*4096/5 = mA*.15*4096/5 = mA*122.88
temperatura_letta,
tensione_15V; //V*4.7/(33+4.7) *4096/5 = V*102
}DAC0;

struct
{
int //ingressi analogici
I3letta,//A*.004*5/.512* 4096/5 = A*32
I1letta,//A*.004*5/.512* 4096/5 = A*32
V13letta,//V* 33/680/6 /2 *4096/5 = V*3.313
V12letta,//V* 33/680/6 /2 *4096/5 = V*3.313
Idletta,//corrente differenziale
sensore_pressione,//V*4096/5 = mA*.15*4096/5 = mA*122.88
temperatura_letta,
tensione_15V; //V*4.7/(33+4.7) *4096/5 = V*102
}DAC;

char
display[34],
cifre[16], 
buffer_USB[96],//contiene i 24 bytes degli errori
cursore_menu,
comando_display;

unsigned char
timer_1min,
timer_1s,
timer_20ms,
timer_mandata_chiusa,
timer_attesa_secco,
timer_attesa_tensione,
timer_attesa_squilibrio,
timer_reset_display,
timer_lampeggio,
timer_rinfresco_display,
timer_reset,//attesa per esecuzione reset della eeprom
timer_rele_avviamento,
tot_misure,
Tot_misure,
contatore_display,
contatore_AD;

unsigned char
start,
salita_start,
stop,
salita_stop,
toggle_stop,
func,
salita_func,
toggle_func,
meno,
salita_meno,
piu,
salita_piu, 
remoto,
salita_remoto,
sequenza_fasi,
segno_V12,
commutazione_V12,
segno_V13,
commutazione_V13,
attesa_invio,
prima_segnalazione,
alternanza_presentazione,
relais_alimentazione,
relais_avviamento,
tentativi_avviamento,
precedente_segnalazione;

int 
I1,//A*.004*5/.512* 4096/5 = A*32
I3,//A*.004*5/.512* 4096/5 = A*32
V12,//V* 33/680/6 /2 *4096/5 = V*3.313
V13,//V* 33/680/6 /2 *4096/5 = V*3.313
Id,//corrente differenziale
V1, V2, V3,//V*3.313 * 3
I2;

unsigned int //timer
timer_1_ora,
timer_ritorno_da_emergenza,
timer_lampeggio_LED_emergenza,
timer_attesa_segnalazione_fault,
timer_eccitazione_relais,
timer_riavviamento,
timer_attesa_differenziale,
timer_commuta_presentazione,
timer_aggiorna_misure,
timer_avviamento_monofase,
timer_allarme_avviamento,
timer_inc_dec,
timer_piu_meno,
timer_rilascio;//dei pulsanti start e stop

unsigned long //offset delle letture
offset_I1letta,
offset_I3letta,
offset_V13letta,
offset_V12letta,
offset_Idletta;

long //letture sommate
conta_secondi, conta_secondi_attivita,
delta_T,//sovra_temperatura calcolata
quad_Id,
somma_quad_Id,
somma_quad_I1,
somma_quad_I2,
somma_quad_I3,
somma_quad_V12,
somma_quad_V23,
somma_quad_V31,
somma_potenzaI1xV13,
somma_potenzaI1xV1,
somma_potenzaI2xV2,
somma_potenzaI3xV3,
somma_reattivaI1xV23,
somma_reattivaI2xV31,
somma_reattivaI3xV12,
Somma_potenzaI1xV13,
Somma_potenzaI1xV1,
Somma_potenzaI2xV2,
Somma_potenzaI3xV3,
Somma_reattivaI1xV23,
Somma_reattivaI2xV31,
Somma_reattivaI3xV12,
quad_I1,
quad_I2,
quad_I3,
quad_V12,
quad_V23,
quad_V31;

long //valori medi
media_temperatura,//�C
media_pressione,//Bar*10
media_quad_Id,
media_quad_I1,//(A*.004*5/.512* 4096/5)^2 = A^2*1024
media_quad_I2,
media_quad_I3,
media_quad_V12,
media_quad_V23,
media_quad_V31,
media_potenzaI1xV13,//W
media_potenzaI1xV1,//W
media_potenzaI2xV2,
media_potenzaI3xV3,
media_reattivaI1xV23,//VAR
media_reattivaI2xV31,
media_reattivaI3xV12;

int
Id_rms,
I1_rms,//A*10
I2_rms,
I3_rms,
V12_rms,//V
V23_rms,
V31_rms,//V
potenzaI1xV13,//W
potenzaI1xV1,//W
potenzaI2xV2,
potenzaI3xV3,
reattivaI1xV23,//VAR
reattivaI2xV31,
reattivaI3xV12,

PWM_relais,
potenza_media,
reattiva_media,
tensione_media,
corrente_media,
picco_corrente_avviamento,
corrente_test,
sovratensione_consentita,//V
sottotensione_consentita,//V
ripresa_per_tensione_consentita,//V
dissimmetria,
dissimmetria_tollerata,
dissimmetria_emergenza,
squilibrio,
squilibrio_tollerato,
squilibrio_emergenza,
limitazione_I_avviamento,//A*10
potenza_a_secco,
potenza_mandata_chiusa;

unsigned long //utilizzati i bytes 0,1,2 per indirizzi a 24 bit
indirizzo_scrivi_eeprom,//0-131071 indirizza il byte in fase di scrittura
ultimo_indirizzo_scrittura,//0-131071 in bytes
primo_indirizzo_lettura;//in bytes  mettere primo_indirizzo<ultimo_indirizzo e leggi_eeprom=1;
unsigned int
contatore_leggi_eeprom,//indirizza il byte in fase di lettura
ultimo_dato_in_lettura,//in bytes
buffer_eeprom[48];

unsigned char //eeprom
scrivi_eeprom,
leggi_eeprom,
eeprom_impegnata,

segnalazione_,
reset_default,
salvataggio_funzioni, //!=0 solo durante il salvataggio delle funzioni o reset eeprom
salvataggio_allarme,
salva_conta_secondi,

lunghezza_salvataggio,//numero bytes da salvare 0-32
leggi_impostazioni,
indirizzo_buffer_eeprom,
conta_dati_scrittura_eeprom;

unsigned int
fattore_I2xT,
fattore_Id,
sovraccarico_moderato, sovraccarico,
precedente_lettura_allarme,
numero_ingresso,
allarme_in_lettura; //variabile scritta tramite USB

unsigned char
cosfi[4],
pronto_alla_risposta;//tramite USB

void somma_quadrati(unsigned long *somma, int var)
{
long prod, sommatoria;
asm
 {
 LDHX somma
 LDA 0,X
 STA sommatoria
 LDA 1,X
 STA sommatoria:1
 LDA 2,X
 STA sommatoria:2
 LDA 3,X
 STA sommatoria:3

 LDA var
 BPL al_quadrato
 CLRA
 SUB var:1
 STA var:1
 CLRA
 SBC var
 STA var
al_quadrato: 
 LDA var:1
 TAX
 MUL
 STA prod:3
 STX prod:2
 LDA var:1
 LDX var
 MUL
 LSLA
 ROLX
 CLR prod
 ROL prod
 ADD prod:2
 STA prod:2
 TXA
 ADC #0
 STA prod:1
 LDA prod
 ADC #0
 STA prod
 LDA var
 TAX
 MUL
 ADD prod:1
 STA prod:1
 TXA
 ADC prod
 STA prod
 LDA sommatoria:3
 ADD prod:3
 STA sommatoria:3
 LDA sommatoria:2
 ADC prod:2
 STA sommatoria:2
 LDA sommatoria:1
 ADC prod:1
 STA sommatoria:1
 LDA sommatoria
 ADC prod
 STA sommatoria

 LDHX somma
 LDA sommatoria
 STA 0,X
 LDA sommatoria:1
 STA 1,X
 LDA sommatoria:2
 STA 2,X
 LDA sommatoria:3
 STA 3,X
 }
}

void somma_potenze(long *somma_pot, int x, int y)
{
char segno;
long prod, somma;
asm
 {
 LDHX somma_pot
 LDA 0,X
 STA somma
 LDA 1,X
 STA somma:1
 LDA 2,X
 STA somma:2
 LDA 3,X
 STA somma:3

 CLR segno
 LDA x
 BPL segno_di_y
 COM segno
 CLRA
 SUB x:1
 STA x:1
 CLRA
 SBC x
 STA x
segno_di_y: 
 LDA y
 BPL al_prodotto
 COM segno
 CLRA
 SUB y:1
 STA y:1
 CLRA
 SBC y
 STA y
al_prodotto: 
 
 LDA x:1
 LDX y:1
 MUL
 STA prod:3
 STX prod:2
 LDA x:1
 LDX y
 MUL
 ADD prod:2
 STA prod:2
 TXA
 ADC #0
 STA prod:1
 LDA x
 LDX y:1
 MUL
 ADD prod:2
 STA prod:2
 TXA
 ADC prod:1
 STA prod:1
 CLR prod
 ROL prod
 LDA x
 LDX y
 MUL
 ADD prod:1
 STA prod:1
 TXA
 ADC prod
 STA prod
 
 LDA segno
 BNE per_sottrazione
 LDA somma:3
 ADD prod:3
 STA somma:3
 LDA somma:2
 ADC prod:2
 STA somma:2
 LDA somma:1
 ADC prod:1
 STA somma:1
 LDA somma
 ADC prod
 STA somma
 BRA fine
per_sottrazione: 
 LDA somma:3
 SUB prod:3
 STA somma:3
 LDA somma:2
 SBC prod:2
 STA somma:2
 LDA somma:1
 SBC prod:1
 STA somma:1
 LDA somma
 SBC prod
 STA somma

fine: 
 LDHX somma_pot
 LDA somma
 STA 0,X
 LDA somma:1
 STA 1,X
 LDA somma:2
 STA 2,X
 LDA somma:3
 STA 3,X
 }
}

void calcolo_offset_misure(unsigned long *offset, int misura)
{
long media, delta;
asm
 {
 LDHX offset
 LDA 0,X
 STA media
 LDA 1,X
 STA media:1
 LDA 2,X
 STA media:2
 LDA 3,X
 STA media:3
 
 LDA misura:1
 SUB media:1
 STA delta:3
 LDA misura
 SBC media
 STA delta:2
 CLRA
 SBC #0
 STA delta:1
 STA delta

 LDA media:3
 ADD delta:3
 STA media:3
 LDA media:2
 ADC delta:2
 STA media:2
 LDA media:1
 ADC delta:1
 STA media:1
 LDA media:0
 ADC delta:0
 STA media:0
 
 LDHX offset
 LDA media
 STA 0,X
 LDA media:1
 STA 1,X
 LDA media:2
 STA 2,X
 LDA media:3
 STA 3,X
 }
}

void calcolo_valor_medio_potenze(long *media_potenza, int potenza) //tau = 1s
{
long media, delta;
char k;
asm
 {
 CLR k
 CLR media
 LDHX media_potenza
 LDA 2,X
 STA media:3
 LDA 1,X
 STA media:2
 LDA 0,X
 STA media:1
 BPL vedi_potenza
 COM media
vedi_potenza:
 LDA potenza
 BPL per_delta
 COM k

per_delta:
 LDA potenza:1
 SUB media:2
 STA delta:2
 LDA potenza
 SBC media:1
 STA delta:1
 LDA k
 SBC media
 STA delta
 ROL delta:2 
 ROL delta:1 
 ROL delta 
 ROL delta:2 
 ROL delta:1 
 ROL delta 
 ROL delta:2 
 ROL delta:1 
 ROL delta 

 LDA media:3
 ADD delta:2
 STA media:3
 LDA media:2
 ADC delta:1
 STA media:2
 LDA media:1
 ADC delta:0
 STA media:1

 LDHX media_potenza
 LDA media:1
 STA 0,X
 LDA media:2
 STA 1,X
 LDA media:3
 STA 2,X
 }
}

void calcolo_medie_quadrati(unsigned long *media_quad, unsigned long quad)
{
long media, delta;
asm
 {
 LDHX media_quad
 LDA 0,X
 STA media:0
 LDA 1,X
 STA media:1
 LDA 2,X
 STA media:2
 LDA 3,X
 STA media:3
 
 LDA quad:3
 SUB media:3
 STA delta:3
 LDA quad:2
 SBC media:2
 STA delta:2
 LDA quad:1
 SBC media:1
 STA delta:1
 LDA quad
 SBC media
 STA delta

 ASR delta
 ROR delta:1
 ROR delta:2
 ROR delta:3
 ASR delta
 ROR delta:1
 ROR delta:2
 ROR delta:3
 ASR delta
 ROR delta:1
 ROR delta:2
 ROR delta:3
 
 LDA media:3
 ADD delta:3
 STA media:3
 LDA media:2
 ADC delta:2
 STA media:2
 LDA media:1
 ADC delta:1
 STA media:1
 LDA media:0
 ADC delta:0
 STA media:0
 
 LDHX media_quad
 LDA media
 STA 0,X
 LDA media:1
 STA 1,X
 LDA media:2
 STA 2,X
 LDA media:3
 STA 3,X
 }
}

void valore_efficace(int *efficace, unsigned long quad, int costante)
{
long prod;
int radice, fattore;
char j;
radice=0;
asm
 {
 LDA quad:3 //3x1
 LDX costante:1
 MUL
 STX prod:3
 LDA quad:3 //3x0
 LDX costante
 MUL
 ADD prod:3
 STA prod:3
 TXA
 ADC #0
 STA prod:2
 LDA quad:2 //2x1
 LDX costante:1
 MUL
 ADD prod:3
 STA prod:3
 TXA
 ADC prod:2
 STA prod:2
 CLR prod:1
 ROL prod:1
 LDA quad:2 //2x0
 LDX costante
 MUL
 ADD prod:2
 STA prod:2
 TXA
 ADC prod:1
 STA prod:1
 LDA quad:1 //1x1
 LDX costante:1
 MUL
 ADD prod:2
 STA prod:2
 TXA
 ADC prod:1
 STA prod:1
 CLR prod
 ROL prod
 LDA quad:1 //1x0
 LDX costante
 MUL
 ADD prod:1
 STA prod:1
 TXA
 ADC prod
 STA prod

 LSL prod:3
 ROL prod:2
 ROL prod:1
 ROL prod
 LSL prod:3
 ROL prod:2
 ROL prod:1
 ROL prod
 
 LDA prod:2
 STA prod:3
 LDA prod:1
 STA prod:2
 LDA prod
 STA prod:1
 CLR prod
 
//radice 
 LDA #12
 STA j
 LDHX #$0800 
 STHX fattore
al_quadrato_trag:
 LDA radice:1
 ORA fattore:1
 STA radice:1
 LDA radice
 ORA fattore
 STA radice
 
 LDA radice:1
 TAX
 MUL
 STA quad:3
 STX quad:2
 LDA radice:1
 LDX radice
 MUL
 LSLA
 ROLX
 ADD quad:2
 STA quad:2
 TXA
 ADC #0
 STA quad:1
 LDA radice
 TAX
 MUL
 ADD quad:1
 STA quad:1
 TXA
 ADC #0
 STA quad
 LDHX quad
 CPHX prod
 BEQ vedi_basso
 BCS alla_rotazione
 LDA radice:1
 EOR fattore:1
 STA radice:1
 LDA radice
 EOR fattore
 STA radice
 BRA alla_rotazione
vedi_basso:
 LDHX quad:2
 CPHX prod:2
 BEQ fine
 BCS alla_rotazione
 LDA radice:1
 EOR fattore:1
 STA radice:1
 LDA radice
 EOR fattore
 STA radice
alla_rotazione:
 LSR fattore
 ROR fattore:1
 DBNZ j,al_quadrato_trag
fine: 
 LDA radice:1 //arrotondamento
 ADD #1
 STA radice:1
 LDA radice
 ADC #0
 STA radice
 LSR radice
 ROR radice:1

 LDHX efficace
 LDA radice
 STA 0,X
 LDA radice:1
 STA 1,X
 }
}

void media_quadrati_nel_periodo(unsigned long *quadrato, unsigned long dividendo, char divisore)
{
asm
 {
//diviso 4
 LSR dividendo
 ROR dividendo:1
 ROR dividendo:2
 ROR dividendo:3
 LSR dividendo
 ROR dividendo:1
 ROR dividendo:2
 ROR dividendo:3
 LSR dividendo
 ROR dividendo:1
 ROR dividendo:2
 ROR dividendo:3
 LSR dividendo
 ROR dividendo:1
 ROR dividendo:2
 ROR dividendo:3
 CLRH
 LDX divisore
 LDA dividendo
 DIV
 STA dividendo
 LDA dividendo:1
 DIV
 STA dividendo:1
 LDA dividendo:2
 DIV
 STA dividendo:2
 LDA dividendo:3
 DIV
 STA dividendo:3
 LDHX quadrato
 STA 3,X
 LDA dividendo:2
 STA 2,X
 LDA dividendo:1
 STA 1,X
 LDA dividendo:0
 STA 0,X
 }
}

int media_potenza_nel_periodo(long dividendo, int fattore, char divisore)//W
{
//A*.004*5/.512* 4096/5  *  V*33/680/6 /2 *4096/5 =  W * 106.01*16
//potenza (W) = potenza*9891>>24
char segno, prod[5];
int risultato;
asm
 {
 CLR segno
 LDA dividendo
 BPL al_prodotto
 COM segno
 COM dividendo:3 
 COM dividendo:2
 COM dividendo:1 
 COM dividendo
al_prodotto: 
 LDA dividendo:3//3x0
 LDX fattore
 MUL
 STA prod:4
 STX prod:3
 LDA dividendo:2 //2x1
 LDX fattore:1
 MUL
 ADD prod:4
 STA prod:4
 TXA
 ADC prod:3
 STA prod:3
 CLR prod:2
 ROL prod:2
 LDA dividendo:2 //2x0
 LDX fattore
 MUL
 ADD prod:3
 STA prod:3
 TXA
 ADC prod:2
 STA prod:2
 LDA dividendo:1 //1x1
 LDX fattore:1
 MUL
 ADD prod:3
 STA prod:3
 TXA
 ADC prod:2
 STA prod:2
 CLR prod:1
 ROL prod:1
 LDA dividendo:1 //1x0
 LDX fattore
 MUL
 ADD prod:2
 STA prod:2
 TXA
 ADC prod:1
 STA prod:1
 LDA dividendo   //0x1
 LDX fattore:1
 MUL
 ADD prod:2
 STA prod:2
 TXA
 ADC prod:1
 STA prod:1
 CLR prod
 ROL prod
 LDA dividendo   //0x0
 LDX fattore
 MUL
 ADD prod:1
 STA prod:1
 TXA
 ADC prod
 STA prod

 CLRH
 LDX divisore
 LDA prod
 DIV
 STA prod
 LDA prod:1
 DIV
 STA prod:1
 LDA prod:2
 DIV
 STA prod:2

 LDA segno
 BEQ fine
 CLRA
 SUB prod:2
 STA prod:2
 CLRA
 SBC prod:1
 STA prod:1
fine: 
 LDHX prod:1
 STHX risultato 
 }
return risultato; 
}

__interrupt void AD_conversion_completeISR(void)
{
char segno;
long k;
int V21, V31, V23, V32;
asm
 {
 SEI
 CLRH
 LDX contatore_AD
 LDA @comando_AD,X
 AND #$07
 LSLA
 TAX
 LDA ADCRL
 ADD @DAC0:1,X ;//letture analogiche
 STA @DAC0:1,X
 LDA ADCRH
 ADC @DAC0,X
 STA @DAC0,X
 
 LDA contatore_AD
 CMP #22
 BNE comando_di_conversione

 LDHX #16
cancellazione: 
 LDA @DAC0:-1,X
 STA @DAC:-1,X
 CLRA
 STA @DAC0:-1,X
 DBNZX cancellazione

 LDA tot_misure
 INCA
 STA tot_misure
//calcolo dell'offset
 }
calcolo_offset_misure((unsigned long*)&offset_I1letta, DAC.I1letta);
calcolo_offset_misure((unsigned long*)&offset_I3letta, DAC.I3letta);
calcolo_offset_misure((unsigned long*)&offset_V12letta, DAC.V12letta);
calcolo_offset_misure((unsigned long*)&offset_V13letta, DAC.V13letta);
calcolo_offset_misure((unsigned long*)&offset_Idletta, DAC.Idletta);
asm
 { 
 LDA DAC.Idletta:1
 SUB offset_Idletta:1
 STA Id:1
 LDA DAC.Idletta
 SBC offset_Idletta
 STA Id

 LDA DAC.I1letta:1
 SUB offset_I1letta:1
 STA I1:1
 LDA DAC.I1letta
 SBC offset_I1letta
 STA I1

 CLR segno
 LDHX I1
 BMI elimina_disturbo_neg
 CPHX disturbo_I_pos
 BCC per_calibrazione_I1
 LDHX #0
 STHX I1
 BRA vedi_I3
elimina_disturbo_neg:
 CPHX disturbo_I_neg
 BCS per_calibrazione_I1_con_segno
 LDHX #0
 STHX I1
 BRA vedi_I3
per_calibrazione_I1_con_segno:
 COM segno
 CLRA
 SUB I1:1
 STA I1:1 
 CLRA
 SBC I1
 STA I1 
per_calibrazione_I1:
 LDA I1:1
 LDX set.calibrazione_I1:1
 MUL
 STA k:2
 STX k:1
 LDA I1
 LDX set.calibrazione_I1:1
 MUL
 ADD k:1
 STA k:1
 TXA
 ADC #0
 STA k
 LSL k:2
 ROL k:1
 ROL k
 LDA segno
 BEQ assegna_I1
 CLRA
 SUB k:1
 STA k:1 
 CLRA
 SBC k
 STA k 
assegna_I1:
 LDHX k
 STHX I1
 
vedi_I3: 
 LDA DAC.I3letta:1
 SUB offset_I3letta:1
 STA I3:1
 LDA DAC.I3letta
 SBC offset_I3letta
 STA I3

 CLR segno
 LDHX I3
 BMI Elimina_disturbo_neg
 CPHX disturbo_I_pos
 BCC per_calibrazione_I3
 LDHX #0
 STHX I3
 BRA vedi_I2
Elimina_disturbo_neg:
 CPHX disturbo_I_neg
 BCS per_calibrazione_I3_con_segno
 LDHX #0
 STHX I3
 BRA vedi_I2
per_calibrazione_I3_con_segno:
 COM segno
 CLRA
 SUB I3:1
 STA I3:1 
 CLRA
 SBC I3
 STA I3 
per_calibrazione_I3:
 LDA I3:1
 LDX set.calibrazione_I3:1
 MUL
 STA k:2
 STX k:1
 LDA I3
 LDX set.calibrazione_I3:1
 MUL
 ADD k:1
 STA k:1
 TXA
 ADC #0
 STA k
 LSL k:2
 ROL k:1
 ROL k
 LDA segno
 BEQ assegna_I3
 CLRA
 SUB k:1
 STA k:1 
 CLRA
 SBC k
 STA k 
assegna_I3:
 LDHX k
 STHX I3

vedi_I2:
;I2 = -I1-I3;
 CLR segno
 CLRA
 SUB I1:1
 STA k:1
 CLRA
 SBC I1
 STA k
 LDA k:1
 SUB I3:1
 STA I2:1
 LDA k
 SBC I3
 STA I2
 BPL per_calibrazione_I2
 COM segno
 CLRA
 SUB I2:1
 STA I2:1 
 CLRA
 SBC I2
 STA I2 
per_calibrazione_I2:
 LDA I2:1
 LDX set.calibrazione_I2:1
 MUL
 STA k:2
 STX k:1
 LDA I2
 LDX set.calibrazione_I2:1
 MUL
 ADD k:1
 STA k:1
 TXA
 ADC #0
 STA k
 LSL k:2
 ROL k:1
 ROL k
 LDA segno
 BEQ assegna_I2
 CLRA
 SUB k:1
 STA k:1 
 CLRA
 SBC k
 STA k 
assegna_I2:
 LDHX k
 STHX I2
 
 LDA DAC.V12letta:1
 SUB offset_V12letta:1
 STA V12:1
 LDA DAC.V12letta
 SBC offset_V12letta
 STA V12

 LDA DAC.V13letta:1
 SUB offset_V13letta:1
 STA V13:1
 LDA DAC.V13letta
 SBC offset_V13letta
 STA V13

//tensioni di fase 
;V21=-V12
;V31=-V13
;V23=V21-V31;
;V32=-V23;

;V1 = V12+V13  
;V2 = V21+V23
;V3 = V31+V32

 CLRA
 SUB V12:1
 STA V21:1
 CLRA
 SBC V12
 STA V21

 CLRA
 SUB V13:1
 STA V31:1
 CLRA
 SBC V13
 STA V31

 LDA V21:1
 SUB V31:1
 STA V23:1
 LDA V21
 SBC V31
 STA V23

 CLRA
 SUB V23:1
 STA V32:1
 CLRA
 SBC V23
 STA V32

 LDA V12:1
 ADD V13:1
 STA V1:1
 LDA V12
 ADC V13
 STA V1
 
 LDA V21:1
 ADD V23:1
 STA V2:1
 LDA V21
 ADC V23
 STA V2
 
 LDA V31:1
 ADD V32:1
 STA V3:1
 LDA V31
 ADC V32
 STA V3

 LDA V12
 BPL commuta_a_positivi
 LDA #-1
 STA segno_V12
 BRA vedi_commutazione_V13
commuta_a_positivi: 
 LDA segno_V12
 BPL vedi_commutazione_V13
 LDA #1
 STA segno_V12
 LDA tot_misure
 STA commutazione_V12
 
vedi_commutazione_V13:
 LDA V13
 BPL Commuta_a_positivi
 LDA #-1
 STA segno_V13
 BRA per_somme_quad
Commuta_a_positivi: 
 LDA segno_V13
 BPL per_somme_quad
 LDA #1
 STA segno_V13
 LDA tot_misure
 STA commutazione_V13

per_somme_quad:
 }
somma_quadrati((unsigned long*)&somma_quad_Id,Id);
somma_quadrati((unsigned long*)&somma_quad_I1,I1);
somma_quadrati((unsigned long*)&somma_quad_I3,I3);
somma_quadrati((unsigned long*)&somma_quad_V31,V31);
somma_potenze((long*)&somma_potenzaI1xV13,I1,V13);
if(set.monofase_trifase) 
 {
 somma_quadrati((unsigned long*)&somma_quad_I2,I2);
 somma_quadrati((unsigned long*)&somma_quad_V12,V12);
 somma_quadrati((unsigned long*)&somma_quad_V23,V23);

 somma_potenze((long*)&somma_potenzaI1xV1,I1,V1);
 somma_potenze((long*)&somma_potenzaI2xV2,I2,V2);
 somma_potenze((long*)&somma_potenzaI3xV3,I3,V3);
 somma_potenze((long*)&somma_reattivaI1xV23,I1,V23);
 somma_potenze((long*)&somma_reattivaI2xV31,I2,V31);
 somma_potenze((long*)&somma_reattivaI3xV12,I3,V12);
 }
asm
 { 
comando_di_conversione:
 LDX contatore_AD
 INCX
 CPX #23
 BCS per_comando_conversione
 CLRX
per_comando_conversione:
 STX contatore_AD
 CLRH
 LDA @comando_AD,X
 STA ADCSC1
 }
}

void lettura_pulsanti(void)
{
asm
 {
 BRSET 0,_PTAD,azzera_func
 LDHX #500
 STHX timer_rilascio 
 LDA func
 CMP #20
 BNE func_1
 LDA #1
 STA salita_func
func_1: 
 LDA func
 CMP filtro_pulsanti
 BCC vedi_meno
 INCA
 STA func
 BRA vedi_meno
azzera_func:
 CLRA
 STA func

vedi_piu:
 BRSET 1,_PTAD,azzera_piu
 LDHX #500
 STHX timer_rilascio 
 LDA piu
 CMP #20
 BNE piu_1
 LDA #1
 STA salita_piu
piu_1: 
 LDA piu
 CMP filtro_pulsanti
 BCC vedi_meno 
 INCA
 STA piu
 BRA vedi_meno 
azzera_piu:
 CLRA
 STA piu

vedi_meno:
 BRSET 2,_PTAD,azzera_meno
 LDHX #500
 STHX timer_rilascio 
 LDA meno
 CMP #20
 BNE meno_1
 LDA #1
 STA salita_meno
meno_1: 
 LDA meno
 CMP filtro_pulsanti
 BCC vedi_start 
 INCA
 STA meno
 BRA vedi_start 
azzera_meno:
 CLRA
 STA meno

vedi_start:
 BRSET 3,_PTAD,azzera_start
 LDHX #500
 STHX timer_rilascio 
 LDA start
 CMP #20
 BNE start_1
 LDA #1
 STA salita_start
start_1:
 LDA start
 CMP filtro_pulsanti
 BCC vedi_stop
 INCA
 STA start
 BRA vedi_stop
azzera_start:
 CLRA
 STA start

vedi_stop:
 BRSET 4,_PTAD,azzera_stop
 LDHX #500
 STHX timer_rilascio 
 LDA stop
 CMP #20
 BNE stop_1
 LDA #1
 STA salita_stop
stop_1: 
 LDA stop
 CMP filtro_pulsanti
 BCC vedi_remoto
 INCA
 STA stop
 BRA vedi_remoto
azzera_stop:
 CLRA
 STA stop

vedi_remoto:
 BRCLR 5,_PTAD,azzera_remoto
 LDHX #500
 STHX timer_rilascio 
 LDA remoto
 CMP #20
 BNE remoto_1
 LDA #1
 STA salita_remoto
remoto_1:
 LDA remoto
 CMP filtro_pulsanti
 BCC vedi_timer_rilascio
 INCA
 STA remoto
 BRA vedi_timer_rilascio
azzera_remoto:
 CLRA
 STA remoto

vedi_timer_rilascio:
 LDHX timer_rilascio
 BNE per_dec
 CLRA
 STA start
 STA salita_start
 STA stop
 STA salita_stop
 STA func
 STA salita_func
 STA meno
 STA salita_meno
 STA piu 
 STA salita_piu 
 STA remoto
 STA salita_remoto
 BRA fine
per_dec:
 LDA timer_rilascio:1
 SUB #1
 STA timer_rilascio:1
 LDA timer_rilascio
 SBC #0
 STA timer_rilascio
fine:
 }   
}

void comando_del_display(void)
{
char Display;
asm
 {
;//PTD0-7 = dato
;//RS=PTG2, E=PTG3
;if(comando_display) //nuovo controllo
; {
; PTDD=prepara[contatore_display>>1];
; if(contatore_display & 0x0001) BCLR 2,_PTGD; else BSET 3,_PTGD; //E=1, RS=0
; contatore_display++;
; if(contatore_display>13)
;  {
;  comando_display=0;
;  contatore_display=0;
;  }  
; }
 LDA timer_reset_display
 BEQ per_comando_display
 DECA
 STA timer_reset_display 
 BNE fine 
 CLRA
 STA contatore_display
 LDHX #34
primo_messaggio:
 LDA @presentazione_iniziale:-1,X
 STA @display:-1,X
 DBNZX primo_messaggio
 BRA fine

per_comando_display: 
 LDA comando_display
 BEQ scrivi_display
 LDX contatore_display
 LSRX
 CLRH
 LDA @prepara,X
 STA _PTDD
 LDA contatore_display
 AND #1
 BEQ setta_1_E
 BCLR 2,_PTGD;//RS=0
 BCLR 3,_PTGD;//E=0
 BRA inc_contatore_display
setta_1_E:
 BSET 3,_PTGD;//E=1
inc_contatore_display:
 LDA contatore_display
 INCA
 STA contatore_display
 CMP #14
 BCS fine
 CLRA
 STA comando_display
 STA contatore_display
 STA timer_lampeggio

scrivi_display:
;    else //normale scrittura
;     {
;     if(contatore_display==0)
;      {
;      PTGD &=0xf3; //E=0, RS=0;
;      PTGD |=0x08; //E=1
;      Display=0x80;
;      }
;     else if(contatore_display==1)
;      {
;      PTGD &=0xf3; //E=0, RS=0;
;      }
;     else if(contatore_display<34)
;      {
;      if(contatore_display & 0x0001)
;       {
;       PTGD &=0xf3; //RS=0, E=0
;       PTGD |=0x04; //RS=1, E=0
;       }
;      else
;       {
;       PTGD |=0x0c; //RS=1, E=1
;       Display=display[(contatore_display-2)>>1];
;       } 
;      }
;     else if(contatore_display==34)
;      {
;      PTGD &=0xf3; //E=0, RS=0;
;      PTGD |=0x08; //E=1
;      Display=0xc0;
;      }
;     else if(contatore_display==35)
;      {
;      PTGD &=0xf3; //E=0, RS=0;
;      Display=0xc0; //
;      }
;     else if(contatore_display<68)
;      {
;      if(contatore_display & 0x0001)
;       {
;       PTGD |=0x04; //RS=1, E=0
;       }
;      else
;       {
;       PTGD |=0x0c; //RS=1, E=1
;       Display=display[(contatore_display-2)>>1];
;       } 
;      }
;     else contatore_display=0xff;
;     PTDD=Display;
;     contatore_display++;
;     }
 LDA contatore_display
 BNE vedi_d1
 BCLR 2,_PTGD;//RS=0;
 BSET 3,_PTGD;//E=1;
 LDA #$80;//comando per prima riga
 STA Display;// STA SPID
 BRA increm_contatore
vedi_d1:
 CMP #1
 BNE vedi_d2
 BCLR 3,_PTGD;//E=0
 BRA increm_contatore
vedi_d2: 
 CMP #34
 BEQ vedi_d34
 BCC vedi_d35
 LDA contatore_display
 AND #1
 BEQ assegna_carattere
 BSET 2,_PTGD;//RS=1
 BCLR 3,_PTGD;//E=0
 BRA increm_contatore
assegna_carattere: 
 BSET 2,_PTGD;//RS
 BSET 3,_PTGD;//E
 LDX contatore_display
 DECX
 DECX
 LSRX
 CLRH
 LDA @display,X
 STA Display
 BRA increm_contatore

vedi_d34:
 BCLR 2,_PTGD;//RS
 BSET 3,_PTGD;//E
 LDA #$c0;//comando per seconda riga
 STA Display;
 BRA increm_contatore
vedi_d35:
 CMP #35
 BNE vedi_d36
 BCLR 3,_PTGD;//E
 BRA increm_contatore
vedi_d36:
 CMP #68
 BCC vedi_d68
 LDA contatore_display
 AND #1
 BEQ Assegna_carattere
 BSET 2,_PTGD;//RS
 BCLR 3,_PTGD;//E=0
 BRA increm_contatore
Assegna_carattere:
 BSET 2,_PTGD;//RS
 BSET 3,_PTGD;//E
 LDA toggle_stop
 BEQ Presenta_carattere
 LDA timer_lampeggio
 CMP #3
 BCC Presenta_carattere
 LDA #' '
 STA Display
 BRA increm_contatore
Presenta_carattere: 
 LDX contatore_display
 DECX
 DECX
 LSRX
 CLRH
 LDA @display,X
 STA Display
 BRA increm_contatore

vedi_d68:
 LDA #$0ff
 STA contatore_display
 LDA timer_lampeggio
 BEQ reset_timer_lampeggio
 DECA
 STA timer_lampeggio
 BRA per_timer_rinfresco_display
reset_timer_lampeggio: 
 LDA #10 
 STA timer_lampeggio

per_timer_rinfresco_display: 
 LDA timer_rinfresco_display
 BEQ per_rinfresco
 DECA
 STA timer_rinfresco_display
 BRA increm_contatore
per_rinfresco:
 LDA #255//ogni 16 s
 STA timer_rinfresco_display
 LDA #1
 STA comando_display

increm_contatore:
 LDA Display
 STA _PTDD
 LDA contatore_display
 INCA
 STA contatore_display
fine:
 }
}

const int mille=1000;
char calcolo_del_fattore_di_potenza_monofase(int attiva, int tensione, int corrente)
{
long prod, app;
int fattore;
char j;
asm
 {
 LDA attiva:1
 LDX mille:1
 MUL 
 STA prod:3
 STX prod:2
 LDA attiva:1
 LDX mille
 MUL 
 ADD prod:2
 STA prod:2
 TXA
 ADC #0
 STA prod:1
 LDA attiva
 LDX mille:1
 MUL 
 ADD prod:2
 STA prod:2
 TXA
 ADC prod:1
 STA prod:1
 CLR prod
 ROL prod
 LDA attiva
 LDX mille
 MUL 
 ADD prod:1
 STA prod:1
 TXA
 ADC prod
 STA prod

 LDA tensione:1
 LDX corrente:1
 MUL
 STA app:2
 STX app:1
 LDA tensione:1
 LDX corrente
 MUL
 ADD app:1
 STA app:1
 TXA
 ADC #0
 STA app
 LDA tensione
 LDX corrente:1
 MUL
 ADD app:1
 STA app:1
 TXA
 ADC app
 STA app
 LDA tensione
 LDX corrente
 MUL
 ADD app
 STA app

 LDA #9
 STA j
ripeti_diff:
 LDA prod:2
 SUB app:2
 STA fattore:1
 LDA prod:1
 SBC app:1
 STA fattore
 LDA prod
 SBC app
 BCS alla_rotazione
 STA prod
 LDHX fattore
 STHX prod:1
alla_rotazione:
 ROL prod:3 
 ROL prod:2 
 ROL prod:1 
 ROL prod 
 DBNZ j,ripeti_diff
 COM prod:3 //cosfi
 LDA prod:3
 STA j
 }
return j; 
}

const int diecimila=10000;
char calcolo_del_fattore_di_potenza(int attiva, int reattiva)//cosfi*100
{
long quad_attiva, quad_reattiva;
unsigned int fattore, quad;
char j, radice, prod[6];
radice=0;
asm
 {
 LDA attiva
 BPL al_quadrato
 LDHX #0
 STHX attiva
al_quadrato: 
 LDA attiva:1
 TAX
 MUL
 STA quad_attiva:3
 STX quad_attiva:2
 LDA attiva
 LDX attiva:1
 MUL
 LSLA
 ROLX
 CLR j
 ROL j
 ADD quad_attiva:2
 STA quad_attiva:2
 TXA
 ADC #0
 STA quad_attiva:1
 LDA j
 ADC #0
 STA quad_attiva
 LDA attiva
 TAX
 MUL
 ADD quad_attiva:1
 STA quad_attiva:1
 TXA
 ADC quad_attiva
 STA quad_attiva

 LDA reattiva
 BPL Al_quadrato
 CLRA
 SUB reattiva:1
 STA reattiva:1
 CLRA
 SBC reattiva
 STA reattiva
Al_quadrato: 
 LDA reattiva:1
 TAX
 MUL
 STA quad_reattiva:3
 STX quad_reattiva:2
 LDA reattiva
 LDX reattiva:1
 MUL
 LSLA
 ROLX
 CLR j
 ROL j
 ADD quad_reattiva:2
 STA quad_reattiva:2
 TXA
 ADC #0
 STA quad_reattiva:1
 LDA j
 ADC #0
 STA quad_reattiva
 LDA reattiva
 TAX
 MUL
 ADD quad_reattiva:1
 STA quad_reattiva:1
 TXA
 ADC quad_reattiva
 STA quad_reattiva

 LDA quad_attiva:3
 ADD quad_reattiva:3
 STA quad_reattiva:3
 LDA quad_attiva:2
 ADC quad_reattiva:2
 STA quad_reattiva:2
 LDA quad_attiva:1
 ADC quad_reattiva:1
 STA quad_reattiva:1
 LDA quad_attiva:0
 ADC quad_reattiva:0
 STA quad_reattiva:0
 
 LDA quad_attiva:3//3x1
 LDX diecimila:1
 MUL
 STA prod:5
 STX prod:4
 LDA quad_attiva:3//3x0
 LDX diecimila
 MUL
 ADD prod:4
 STA prod:4
 TXA 
 ADC #0
 STA prod:3
 LDA quad_attiva:2//2x1
 LDX diecimila:1
 MUL
 ADD prod:4
 STA prod:4
 TXA 
 ADC prod:3
 STA prod:3
 CLR prod:2
 ROL prod:2
 LDA quad_attiva:2//2x0
 LDX diecimila
 MUL
 ADD prod:3
 STA prod:3
 TXA 
 ADC prod:2
 STA prod:2
 LDA quad_attiva:1//1x1
 LDX diecimila:1
 MUL
 ADD prod:3
 STA prod:3
 TXA 
 ADC prod:2
 STA prod:2
 CLR prod:1
 ROL prod:1
 LDA quad_attiva:1//1x0
 LDX diecimila
 MUL
 ADD prod:2
 STA prod:2
 TXA 
 ADC prod:1
 STA prod:1
 LDA quad_attiva//0x1
 LDX diecimila:1
 MUL
 ADD prod:2
 STA prod:2
 TXA 
 ADC prod:1
 STA prod:1
 CLR prod
 ROL prod
 LDA quad_attiva//0x0
 LDX diecimila
 MUL
 ADD prod:1
 STA prod:1
 TXA 
 ADC prod
 STA prod

 LDA #17
 STA j
ripeti_diff:
 LDA prod:3
 SUB quad_reattiva:3
 STA fattore:1
 LDA prod:2
 SBC quad_reattiva:2
 STA fattore
 LDA prod:1
 SBC quad_reattiva:1
 TAX
 LDA prod
 SBC quad_reattiva
 BCS alla_rotazione
 STA prod
 STX prod:1
 LDHX fattore
 STHX prod:2
alla_rotazione:
 ROL prod:5 
 ROL prod:4 
 ROL prod:3 
 ROL prod:2 
 ROL prod:1 
 ROL prod 
 DBNZ j,ripeti_diff
 COM prod:4 //cosfi^2
 COM prod:5 

//radice 
 LDA #8
 STA j
 LDA #$80 
 STA fattore
al_quadrato_trag:
 LDA radice
 ORA fattore
 STA radice
 LDA radice
 TAX
 MUL
 STA quad:1
 STX quad
 LDHX quad
 CPHX prod:4
 BEQ fine
 BCS Alla_rotazione
 LDA radice
 EOR fattore
 STA radice
Alla_rotazione:
 LSR fattore
 DBNZ j,al_quadrato_trag
fine: 
 }
return radice; 
}

__interrupt void timer1_overflowISR(void) //timer 100 us per relais periodo=1200
{
asm
 {
 SEI  /*disabilita interrupt*/
 LDA _TPM1SC
 BCLR 7,_TPM1SC

 LDA relais_alimentazione
 BEQ spegni_relais_alim
 LDA timer_rele_avviamento
 BEQ accendi_relais_alim
 DECA
 STA timer_rele_avviamento
 BRA spegni_relais_alim
accendi_relais_alim:
 LDHX PWM_relais
 STHX _TPM1C2V//rel� di marcia acceso
 LDHX #$7fff
 STHX _TPM1C5V //LED di marcia acceso
 BRA vedi_relais_avviam 
spegni_relais_alim: 
 LDHX #-1
 STHX _TPM1C5V//LED di marcia spento
 STHX _TPM1C2V//rel� di marcia spento
vedi_relais_avviam: 
 LDA relais_avviamento
 BEQ spegni_relais_avviam
 LDHX PWM_relais
 STHX _TPM1C3V//rel� di avviamento acceso
 BRA fine
spegni_relais_avviam:
 LDHX #-1
 STHX _TPM1C3V  
fine:
 LDHX #$7fff
 STHX _TPM1C4V//Led presenza tensione
 }
}

const int
alimentazione12V=5745,//12/(33+4.7)*4.7 /5 *4096 *1200/256
alimentazione8V=3830;//8/(33+4.7)*4.7 /5 *4096 *1200/256 
void calcolo_tensione_relais(void)
{
char j;
long numeratore;
asm
 {
 CLR numeratore
 CLR numeratore:3
 LDHX timer_eccitazione_relais
 BEQ eccitazione_ridotta
 LDA timer_eccitazione_relais:1
 SUB #1
 STA timer_eccitazione_relais:1
 LDA timer_eccitazione_relais
 SBC #0
 STA timer_eccitazione_relais
 LDHX alimentazione12V
 STHX numeratore:1
 BRA calcolo_PWM
 
eccitazione_ridotta: 
 LDHX alimentazione8V
 STHX numeratore:1

calcolo_PWM:
// PWM_relais = 1200*alimentazione12V/DAC.tensione_15V
 LDA #17
 STA j
ripetizione:
 LDA numeratore:1
 SUB DAC.tensione_15V:1
 TAX
 LDA numeratore
 SBC DAC.tensione_15V
 BCS per_rotazione
 STA numeratore
 STX numeratore:1
per_rotazione: 
 ROL numeratore:3
 ROL numeratore:2
 ROL numeratore:1
 ROL numeratore:0
 DBNZ j,ripetizione 
 COM numeratore:3
 COM numeratore:2
 LDHX numeratore:2
 STHX PWM_relais
 }
}

const int duemila507=2507;
__interrupt void timer2_overflowISR(void) //timer 1 ms
{
long delta;
int temperatura, pressione, attiva, reattiva;
char j, segno;
unsigned char ritardo;
asm
 {
 SEI  /*disabilita interrupt*/
 LDA _TPM2SC
 BCLR 7,_TPM2SC
 LDA timer_20ms
 CMP #19
 BCC per_reset_timer_20ms
 INCA
 STA timer_20ms
 BRA per_timer_1ms
per_reset_timer_20ms:
 CLRA
 STA timer_20ms

 LDA timer_1s
 CMP #49
 BCC per_reset_timer_1s
 INCA
 STA timer_1s
 BRA per_timer_1ms
per_reset_timer_1s: 
 CLRA
 STA timer_1s

//funzioni ripetute ogni secondo
 JSR protezione_termica

 LDA timer_reset
 BEQ per_timer_1_ora
 DECA
 STA timer_reset

per_timer_1_ora:
 LDHX timer_1_ora
 BEQ per_incremento_contatore_emer
 LDA timer_1_ora:1
 SUB #1
 STA timer_1_ora:1
 LDA timer_1_ora
 SBC #0
 STA timer_1_ora
 BRA per_timer_riavviamento
per_incremento_contatore_emer:
 LDHX #3600
 STHX timer_1_ora
 LDA tentativi_avviamento  //contatore emergenza per sovracorrente
 BEQ per_timer_riavviamento
 CMP N_tentativi_motore_bloccato
 BCC per_timer_riavviamento
 INCA
 STA tentativi_avviamento
 
per_timer_riavviamento: 
 LDHX timer_riavviamento
 BEQ per_timer_attesa_secco
 LDA timer_riavviamento:1
 SUB #1
 STA timer_riavviamento:1
 LDA timer_riavviamento
 SBC #0
 STA timer_riavviamento
 
per_timer_attesa_secco: 
 LDA timer_attesa_secco
 BEQ per_timer_mandata_chiusa
 DECA
 STA timer_attesa_secco
 
per_timer_mandata_chiusa: 
 LDA timer_mandata_chiusa
 BEQ per_timer_attesa_tensione
 DECA
 STA timer_mandata_chiusa
 
per_timer_attesa_tensione: 
 LDA timer_attesa_tensione
 BEQ per_timer_attesa_squilibrio
 DECA
 STA timer_attesa_tensione
 
per_timer_attesa_squilibrio: 
 LDA timer_attesa_squilibrio
 BEQ per_conta_secondi
 DECA
 STA timer_attesa_squilibrio
 
per_conta_secondi: 
 LDA conta_secondi:3
 ADD #1
 STA conta_secondi:3
 LDA conta_secondi:2
 ADC #0
 STA conta_secondi:2
 LDA conta_secondi:1
 ADC #0
 STA conta_secondi:1
 LDA conta_secondi
 ADC #0
 STA conta_secondi
 LDA relais_alimentazione
 BEQ per_timer_1min
 LDA conta_secondi_attivita:3
 ADD #1
 STA conta_secondi_attivita:3
 LDA conta_secondi_attivita:2
 ADC #0
 STA conta_secondi_attivita:2
 LDA conta_secondi_attivita:1
 ADC #0
 STA conta_secondi_attivita:1
 LDA conta_secondi_attivita
 ADC #0
 STA conta_secondi_attivita

per_timer_1min:
 LDA timer_1min
 CMP #59
 BCC per_reset_timer_1min
 INCA
 STA timer_1min
 BRA per_timer_1ms
per_reset_timer_1min: 
 CLRA
 STA timer_1min
 LDA #1
 STA salva_conta_secondi

per_timer_1ms: 
 LDHX #0
 STHX temperatura
 STHX pressione;
 }
switch(timer_20ms)
 {
 case 0:
  {
  Tot_misure=tot_misure;
  tot_misure=0;
  media_quadrati_nel_periodo((unsigned long*)&quad_Id,somma_quad_Id,Tot_misure);
  media_quadrati_nel_periodo((unsigned long*)&quad_I1,somma_quad_I1,Tot_misure);
  media_quadrati_nel_periodo((unsigned long*)&quad_I3,somma_quad_I3,Tot_misure);
  if(set.monofase_trifase)//trifase
   {
   media_quadrati_nel_periodo((unsigned long*)&quad_I2,somma_quad_I2,Tot_misure);
   media_quadrati_nel_periodo((unsigned long*)&quad_V12,somma_quad_V12,Tot_misure);
   media_quadrati_nel_periodo((unsigned long*)&quad_V23,somma_quad_V23,Tot_misure);
   media_quadrati_nel_periodo((unsigned long*)&quad_V31,somma_quad_V31,Tot_misure);
   ritardo=commutazione_V12-commutazione_V13;
   asm
    {
    LDX Tot_misure
    CLRH
    LDA ritardo
    DIV
    CLRA
    DIV
    STA ritardo
    }
   if(ritardo>127) sequenza_fasi=1; else sequenza_fasi=0;
   somma_quad_I2=0;
   somma_quad_V12=0;
   somma_quad_V23=0;
   somma_quad_V31=0;
   Somma_potenzaI1xV1=somma_potenzaI1xV1;
   Somma_potenzaI2xV2=somma_potenzaI2xV2;
   Somma_potenzaI3xV3=somma_potenzaI3xV3;
   Somma_reattivaI1xV23=somma_reattivaI1xV23;
   Somma_reattivaI2xV31=somma_reattivaI2xV31;
   Somma_reattivaI3xV12=somma_reattivaI3xV12;
   somma_potenzaI1xV1=0;
   somma_potenzaI2xV2=0;
   somma_potenzaI3xV3=0;
   somma_reattivaI1xV23=0;
   somma_reattivaI2xV31=0;
   somma_reattivaI3xV12=0;
   }
  else media_quadrati_nel_periodo((unsigned long*)&quad_V31,somma_quad_V31,Tot_misure);
  somma_quad_Id=0;
  somma_quad_I1=0;
  somma_quad_I3=0;
  somma_quad_V31=0;

  Somma_potenzaI1xV13=somma_potenzaI1xV13;
  somma_potenzaI1xV13=0;
  } break;
 case 1:
  {
  if(set.monofase_trifase)//trifase
   {
   potenzaI1xV1=media_potenza_nel_periodo(Somma_potenzaI1xV1,3297,Tot_misure);//W
   potenzaI2xV2=media_potenza_nel_periodo(Somma_potenzaI2xV2,3297,Tot_misure);//W
   potenzaI3xV3=media_potenza_nel_periodo(Somma_potenzaI3xV3,3297,Tot_misure);//W
   reattivaI1xV23=media_potenza_nel_periodo(Somma_reattivaI1xV23,5710,Tot_misure);//VAR
   reattivaI2xV31=media_potenza_nel_periodo(Somma_reattivaI2xV31,5710,Tot_misure);//VAR
   reattivaI3xV12=media_potenza_nel_periodo(Somma_reattivaI3xV12,5710,Tot_misure);//VAR
   if(sequenza_fasi) //senso ciclico invertito
    {
    reattivaI1xV23=-reattivaI1xV23;
    reattivaI2xV31=-reattivaI2xV31;
    reattivaI3xV12=-reattivaI3xV12;
    }
   }
  else potenzaI1xV13=media_potenza_nel_periodo(Somma_potenzaI1xV13,9891,Tot_misure);//W
  } break;
 case 2:
  {
  calcolo_medie_quadrati((unsigned long*)&media_quad_Id,quad_Id);
  valore_efficace((int*)&Id_rms,media_quad_Id,fattore_Id);
  valore_efficace((int*)&I1_rms,media_quad_I1,6400); //(A*.004*5/.512* 4096/5)^2 = A^2*1024  = (A*10)^2*10.24 = (A*10)^2*6400>>16
  calcolo_medie_quadrati((unsigned long*)&media_quad_I1,quad_I1);
  } break;
 case 3:
  {
  if(set.monofase_trifase)//trifase
   {
   calcolo_medie_quadrati((unsigned long*)&media_quad_I2,quad_I2);
   valore_efficace((int*)&I2_rms,media_quad_I2,6400); 
   }
  } break;
 case 4:
  {
  calcolo_medie_quadrati((unsigned long*)&media_quad_I3,quad_I3);
  valore_efficace((int*)&I3_rms,media_quad_I3,6400); 
  } break;
 case 5:
  {
  if(set.monofase_trifase)//trifase
   {
   calcolo_medie_quadrati((unsigned long*)&media_quad_V12,quad_V12);
   valore_efficace((int*)&V12_rms,media_quad_V12,6210);//(V *33/680/6 /2 *4096/5)^2 = V^2*10.9755 = V^2*5971>>16 *(correzione=1.04)
   }
  } break;
 case 6:
  {
  if(set.monofase_trifase)//trifase
   {
   calcolo_medie_quadrati((unsigned long*)&media_quad_V23,quad_V23);
   valore_efficace((int*)&V23_rms,media_quad_V23,6210); 
   }
  } break;
 case 7:
  {
  calcolo_medie_quadrati((unsigned long*)&media_quad_V31,quad_V31);
  valore_efficace((int*)&V31_rms,media_quad_V31,6210); 
  } break;
 case 8:
  {
  if(set.monofase_trifase) calcolo_valor_medio_potenze((long*)&media_potenzaI1xV1,potenzaI1xV1);
  else calcolo_valor_medio_potenze((long*)&media_potenzaI1xV13,potenzaI1xV13);
  } break;
 case 9:
  {
  if(set.monofase_trifase) calcolo_valor_medio_potenze((long*)&media_potenzaI2xV2,potenzaI2xV2);
  } break;
 case 10:
  {
  if(set.monofase_trifase) calcolo_valor_medio_potenze((long*)&media_potenzaI3xV3,potenzaI3xV3);
  } break;
 case 11:
  {
  if(set.monofase_trifase) calcolo_valor_medio_potenze((long*)&media_reattivaI1xV23,reattivaI1xV23);
  } break;
 case 12:
  {
  if(set.monofase_trifase) calcolo_valor_medio_potenze((long*)&media_reattivaI2xV31,reattivaI2xV31);
  } break;
 case 13:
  {
  if(set.monofase_trifase) calcolo_valor_medio_potenze((long*)&media_reattivaI3xV12,reattivaI3xV12);
  } break;
 case 14:
  {
  asm
   {
//calcolo della tenperatura motore    
;temperatura=(set.temperatura_ambiente+DAC.temperatura_letta)/4096*5/4.7*37.7*1000/set.scala_temperatura_motore-set.taratura_temperatura_ambiente,//da modificare per la taratura
;temperatura=(set.temperatura_ambiente+DAC.temperatura_letta)*2507/256/set.scala_temperatura_motore-set.taratura_temperatura_ambiente,//da modificare per la taratura
   LDA DAC.temperatura_letta:1
   ADD set.temperatura_ambiente:1
   STA temperatura:1
   LDA DAC.temperatura_letta
   ADC set.temperatura_ambiente
   STA temperatura

   LDA temperatura:1
   LDX duemila507:1
   MUL
   STX delta:2
   LDA temperatura
   LDX duemila507:1
   MUL
   ADD delta:2
   STA delta:2
   TXA
   ADC #0
   STA delta:1
   LDA temperatura:1
   LDX duemila507
   MUL
   ADD delta:2
   STA delta:2
   TXA
   ADC delta:1
   STA delta:1
   CLR delta
   ROL delta
   LDA temperatura
   LDX duemila507
   MUL
   ADD delta:1
   STA delta:1
   TXA
   ADC delta
   STA delta

   LDHX set.scala_temperatura_motore
   LDA delta
   DIV
   STA delta
   LDA delta:1
   DIV
   STA delta:1
   LDA delta:2
   DIV
   STA delta:2
   SUB set.taratura_temperatura_ambiente:1
   STA temperatura:1
   LDA delta:1
   SBC set.taratura_temperatura_ambiente
   STA temperatura
   BCC uscita_temperatura
   CLR temperatura
   CLR temperatura:1
  uscita_temperatura: 
   }
  calcolo_valor_medio_potenze((long*)&media_temperatura,temperatura);
  if(media_temperatura<0) media_temperatura=0;
  } break;
 case 15:
  {
  asm
   {
//calcolo della pressione
;pressione=(DAC.sensore_pressione-491)*set.portata_sensore_pressione/fattore_portata;
   CLR segno
   LDA DAC.sensore_pressione:1
   SUB #235
   STA pressione:1
   LDA DAC.sensore_pressione
   SBC #1
   STA pressione
   BCC per_prodotto_portata
   COM segno
   CLRA
   SUB pressione:1
   STA pressione:1
   CLRA
   SBC pressione
   STA pressione
  per_prodotto_portata:
   LDA set.portata_sensore_pressione:1
   LDX pressione:1
   MUL
   STA delta:3
   STX delta:2
   LDA set.portata_sensore_pressione:1
   LDX pressione
   MUL
   ADD delta:2
   STA delta:2
   TXA
   ADC #0
   STA delta:1
   LDA set.portata_sensore_pressione
   LDX pressione:1
   MUL
   ADD delta:2
   STA delta:2
   TXA
   ADC delta:1
   STA delta:1
   LDA set.portata_sensore_pressione
   LDX pressione
   MUL
   ADD delta:1
   STA delta:1
   CLR delta

   LDA #17
   STA j
  ripeti_diff:
   LDA delta:1
   SBC fattore_portata:1
   TAX
   LDA delta
   SBC fattore_portata
   BCS alla_rotazione
   STA delta
   STX delta:1
  alla_rotazione:
   ROL delta:3 
   ROL delta:2 
   ROL delta:1 
   ROL delta 
   DBNZ j,ripeti_diff
   COM delta:2
   COM delta:3 
   LDA segno
   BEQ assegna_pressione
   CLRA
   SUB delta:3
   STA delta:3
   CLRA
   SBC delta:2
   STA delta:2
  assegna_pressione: 
   LDHX delta:2
   STHX pressione
   }
  calcolo_valor_medio_potenze((long*)&media_pressione,pressione);
  } break;
 case 16:
  {
  asm
   {
   LDHX media_reattivaI1xV23
   STHX reattiva
   LDHX media_potenzaI1xV1
   STHX attiva
   BPL per_calcolo_cosfi0
   CLR attiva
   CLR attiva:1
  per_calcolo_cosfi0: 
   }
  if((set.monofase_trifase)&&(set.monofase_trifase)&&(relais_alimentazione)) cosfi[0]=calcolo_del_fattore_di_potenza(attiva,reattiva); else cosfi[0]=0;//cosfi*100
  if(cosfi[0]>99) cosfi[0]=99;
  } break;
 case 17:
  {
  asm
   {
   LDHX media_reattivaI2xV31
   STHX reattiva
   LDHX media_potenzaI2xV2
   STHX attiva
   BPL per_calcolo_cosfi1
   CLR attiva
   CLR attiva:1
  per_calcolo_cosfi1: 
   }
  if((set.monofase_trifase)&&(set.monofase_trifase)&&(relais_alimentazione)) cosfi[1]=calcolo_del_fattore_di_potenza(attiva,reattiva); else cosfi[1]=0;//cosfi*100
  if(cosfi[1]>99) cosfi[1]=99;
  } break;
 case 18:
  {
  asm
   {
   LDHX media_reattivaI3xV12
   STHX reattiva
   LDHX media_potenzaI3xV3
   STHX attiva
   BPL per_calcolo_cosfi2
   CLR attiva
   CLR attiva:1
  per_calcolo_cosfi2: 
   }
  if((set.monofase_trifase)&&(set.monofase_trifase)&&(relais_alimentazione)) cosfi[2]=calcolo_del_fattore_di_potenza(attiva,reattiva); else cosfi[2]=0;//cosfi*100
  if(cosfi[2]>99) cosfi[2]=99;
  } break;
 case 19:
  {
  if(relais_alimentazione==0) cosfi[3]=0; else
   {
   if(set.monofase_trifase) cosfi[3]=calcolo_del_fattore_di_potenza(potenza_media,reattiva_media);//cosfi*100
   else cosfi[3]=calcolo_del_fattore_di_potenza_monofase(potenza_media,tensione_media,corrente_media);//cosfi*100
   if(cosfi[3]>99) cosfi[3]=99;
   }
  } break;
 }
asm 
 {
//funzioni ripetute ogni ms
 JSR lettura_pulsanti
 JSR comando_del_display
 JSR disinserzione_condensatore_avviamento
 JSR calcolo_tensione_relais
 
 LDHX timer_inc_dec
 BEQ vedi_timer_piu_meno
 LDA timer_inc_dec:1
 SUB #1
 STA timer_inc_dec:1
 LDA timer_inc_dec
 SBC #0
 STA timer_inc_dec
 
vedi_timer_piu_meno: 
 LDHX timer_piu_meno
 BEQ per_timer_attesa_differenziale
 LDA timer_piu_meno:1
 SUB #1
 STA timer_piu_meno:1
 LDA timer_piu_meno
 SBC #0
 STA timer_piu_meno
 
per_timer_attesa_differenziale:
 LDHX timer_attesa_differenziale
 BEQ per_timer_allarme_avviamento
 LDA timer_attesa_differenziale:1
 SUB #1
 STA timer_attesa_differenziale:1
 LDA timer_attesa_differenziale
 SBC #0
 STA timer_attesa_differenziale

per_timer_allarme_avviamento:
 LDHX timer_allarme_avviamento
 BEQ per_timer_aggiorna_misure
 LDA timer_allarme_avviamento:1
 SUB #1
 STA timer_allarme_avviamento:1
 LDA timer_allarme_avviamento
 SBC #0
 STA timer_allarme_avviamento
  
per_timer_aggiorna_misure:
 LDHX timer_aggiorna_misure
 BEQ per_timer_attesa_segnalazione_fault
 LDA timer_aggiorna_misure:1
 SUB #1
 STA timer_aggiorna_misure:1
 LDA timer_aggiorna_misure
 SBC #0
 STA timer_aggiorna_misure
 
per_timer_attesa_segnalazione_fault:
 LDHX timer_attesa_segnalazione_fault
 BEQ per_timer_commuta_presentazione
 LDA timer_attesa_segnalazione_fault:1
 SUB #1
 STA timer_attesa_segnalazione_fault:1
 LDA timer_attesa_segnalazione_fault
 SBC #0
 STA timer_attesa_segnalazione_fault
 
per_timer_commuta_presentazione:
 LDHX timer_commuta_presentazione
 BEQ reset_timer_commuta_presentazione
 LDA timer_commuta_presentazione:1
 SUB #1
 STA timer_commuta_presentazione:1
 LDA timer_commuta_presentazione
 SBC #0
 STA timer_commuta_presentazione
 BRA per_LED
reset_timer_commuta_presentazione:
 LDA alternanza_presentazione
 CMP #$20
 BNE set_alternanza
 LDA #$81
 STA alternanza_presentazione
 LDHX #2500
 STHX timer_commuta_presentazione 
 BRA per_LED
set_alternanza:
 CMP #$80
 BNE presenta_allarme
 LDA #$41
 STA alternanza_presentazione
 LDHX #2500
 STHX timer_commuta_presentazione 
 BRA per_LED
presenta_allarme:
 LDA remoto
 BEQ presentazione_stato
 LDA segnalazione_
 CMP #10
 BCS per_prima_presentazione
presentazione_stato: 
 LDA #$21
 STA alternanza_presentazione
 LDHX #1000
 STHX timer_commuta_presentazione 
 BRA per_LED
per_prima_presentazione: 
 LDA #$81
 STA alternanza_presentazione
 LDHX #2500
 STHX timer_commuta_presentazione 
  
per_LED:
 LDHX timer_lampeggio_LED_emergenza
 BEQ reset_timer_lampeggio
 LDA timer_lampeggio_LED_emergenza:1
 SUB #1
 STA timer_lampeggio_LED_emergenza:1
 LDA timer_lampeggio_LED_emergenza
 SBC #0
 STA timer_lampeggio_LED_emergenza
 BRA vedi_LED_rosso
reset_timer_lampeggio:
 LDHX #250
 STHX timer_lampeggio_LED_emergenza

vedi_LED_rosso:  
 LDA segnalazione_
 CMP #10
 BCS spegni_LED_rosso
 CMP #20
 BCS per_lampeggio
 LDHX #$7fff
 STHX TPM2C0V
 BRA fine
per_lampeggio:
 LDHX timer_lampeggio_LED_emergenza
 CPHX #125
 BCC spegni_LED_rosso
 LDHX #$7fff
 STHX TPM2C0V
 BRA fine
spegni_LED_rosso:
 LDHX #24
 STHX TPM2C0V
fine:
 }
}
 
__interrupt void SPI1(void)
{
char dato_letto, dato_scritto;
asm
 {
 SEI  /*disabilita interrupt*/
 LDA SPI1S
 LDA SPI1D
 STA dato_letto
 CLR dato_scritto
 
;if(scrivi_eeprom)
; {
; if(scrivi_eeprom==1)
;  {
;  PTED &=0x7f; //EEPROM abilitata
;  dato_scritto=0x06;//abilita scrittura
;  scrivi_eeprom++;
;  }
; else if(scrivi_eeprom==2) //spazio di disabilatazione e riabilitazione
;  {
;  PTED |=0x80; //EEPROM disabilitata
;  scrivi_eeprom++;
;  }
; else if(scrivi_eeprom==3) //spazio di disabilatazione e riabilitazione
;  {
;  PTED &=0x7f; //EEPROM abilitata
;  dato_scritto=0x02;//codice di scrittura
;  scrivi_eeprom++;
;  }

; else if(scrivi_eeprom==4) //prima abilitazione e indirizzo alto
;  {
;  dato_scritto=indirizzo_scrivi_eeprom>>16;
;  scrivi_eeprom++;
;  }
; else if(scrivi_eeprom==5) //prima abilitazione e indirizzo alto
;  {
;  dato_scritto=indirizzo_scrivi_eeprom>>8;
;  scrivi_eeprom++;
;  }
; else if(scrivi_eeprom==6) //scrivi parte bassa indirizzo
;  {
;  dato_scritto=indirizzo_scrivi_eeprom;
;  conta_dati_scrittura_eeprom=0;
;  scrivi_eeprom++;
;  }
; else if(conta_dati_scrittura_eeprom<lunghezza_salvataggio)
;  {
;  dato_scritto=buffer_eeprom[indirizzo_buffer_eeprom+conta_dati_scrittura_eeprom];
;  conta_dati_scrittura_eeprom++;
;  }
; else if(scrivi_eeprom<20)
;  {
;  PTED |=0x80; //EEPROM disabilitata
;  scrivi_eeprom++;
;  }
; else if(scrivi_eeprom==20)
;  {
;  PTED &=0x7f; //EEPROM abilitata
;  dato_scritto=0x04; //codice di blocco della scrittura
;  scrivi_eeprom++;
;  }
; else//if(scrivi_eeprom==21)
;  {
;  PTED |=0x80; //EEPROM disabilitata
;  scrivi_eeprom=0;
;  }
; }
 LDA scrivi_eeprom
 BEQ lettura_EE
 LDA #1
 STA eeprom_impegnata
 LDA scrivi_eeprom
 CMP #1
 BNE spazio_abilitazione
; if(scrivi_eeprom==1)
;  {
;  PTED &=0x7f; //EEPROM abilitata
;  dato_scritto=0x06;//abilita scrittura
;  scrivi_eeprom++;
;  }
 BCLR 7,_PTED //abilita eeprom
 CLRA
 STA conta_dati_scrittura_eeprom
 LDA #$06
 STA dato_scritto//,_SPID
 LDA scrivi_eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

spazio_abilitazione:
 CMP #2
 BNE spazio_abilitazione1
; else if(scrivi_eeprom==2) //spazio di disabilatazione e riabilitazione
;  {
;  PTED |=0x80; //EEPROM disabilitata
;  scrivi_eeprom++;
;  }
 BSET 7,_PTED
 LDA scrivi_eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

spazio_abilitazione1:
 CMP #3
 BNE parte_alta_indirizzo
; else if(scrivi_eeprom==3) //spazio di disabilatazione e riabilitazione
;  {
;  PTED &=0x7f; //EEPROM abilitata
;  dato_scritto=0x02;//codice di scrittura
;  scrivi_eeprom++;
;  }
 BCLR 7,_PTED
 LDA #$02
 STA dato_scritto//,_SPID
 LDA scrivi_eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

parte_alta_indirizzo:
 CMP #4
 BNE parte_intermedia_indirizzo
; else if(scrivi_eeprom==4) //prima abilitazione e indirizzo alto
;  {
;  dato_scritto=indirizzo_scrivi_eeprom>>16;
;  scrivi_eeprom++;
;  }
 LDA indirizzo_scrivi_eeprom
 STA dato_scritto//_SPID
 LDA scrivi_eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

parte_intermedia_indirizzo:
 CMP #5
 BNE parte_bassa_indirizzo
; else if(scrivi_eeprom==5)
;  {
;  dato_scritto=indirizzo_scrivi_eeprom>>8;
;  scrivi_eeprom++;
;  }
 LDA indirizzo_scrivi_eeprom:1
 STA dato_scritto//_SPID
 LDA scrivi_eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

parte_bassa_indirizzo:
 CMP #6
 BNE parte_alta_dato
; else if(scrivi_eeprom==6) //scrivi parte bassa indirizzo
;  {
;  dato_scritto=indirizzo_scrivi_eeprom:2;
;  conta_dati_scrittura_eeprom=0;
;  scrivi_eeprom++;
;  }
 LDA indirizzo_scrivi_eeprom:2
 STA dato_scritto//_SPID
 CLRA
 STA conta_dati_scrittura_eeprom
 LDA scrivi_eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

parte_alta_dato: 
 LDA conta_dati_scrittura_eeprom
 CMP lunghezza_salvataggio
 BCC chiusura_scrittura
; else if(conta_dati_scrittura_eeprom<lunghezza_salvataggio)
;  {
;  dato_scritto=buffer_eeprom[indirizzo_buffer_eeprom+conta_dati_scrittura_eeprom];
;  conta_dati_scrittura_eeprom++;
;  }
 LDA indirizzo_buffer_eeprom
 ADD conta_dati_scrittura_eeprom
 TAX
 CLRH
 LDA @buffer_eeprom,X
 STA dato_scritto//_SPID
 LDA conta_dati_scrittura_eeprom
 INCA
 STA conta_dati_scrittura_eeprom
 BRA fine_SPID

chiusura_scrittura:
 LDA scrivi_eeprom
 CMP #20 
 BEQ comando_disabilitazione
 BCC per_disabilitazione
; else if(scrivi_eeprom<20)
;  {
;  PTED |=0x80; //EEPROM disabilitata
;  scrivi_eeprom++;
;  }
 BSET 7,_PTED //disabilita eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

comando_disabilitazione:
; else if(scrivi_eeprom==20)
;  {
;  PTED &=0x7f; //EEPROM abilitata
;  dato_scritto=0x04; //codice di blocco della scrittura
;  scrivi_eeprom++;
;  }
 BCLR 7,_PTED //abilita eeprom
 LDA #$04
 STA dato_scritto//,_SPID
 LDA scrivi_eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

per_disabilitazione:
; else//if(scrivi_eeprom==21)
;  {
;  PTED |=0x80; //EEPROM disabilitata
;  scrivi_eeprom=0;
;  }
 BSET 7,_PTED //disabilita eeprom
 CLRA
 STA scrivi_eeprom
 BRA fine_SPID

;else if(leggi_eeprom)
; {
; if(leggi_eeprom==1)
;  {
;  PTED &=0x7f; //EEPROM abilitata
;  dato_scritto=0x03;//abilitazione lettura
;  leggi_eeprom++;
;  }
; else if(leggi_eeprom==2)
;  {
;  dato_scritto=primo_indirizzo_lettura>>16;//parte alta primo indirizzo
;  leggi_eeprom++;
;  }
; else if(leggi_eeprom==3)
;  {
;  dato_scritto=primo_indirizzo_lettura>>8;
;  leggi_eeprom++;
;  }
; else if(leggi_eeprom==4)
;  {
;  dato_scritto=primo_indirizzo_lettura;//parte bassa primo indirizzo
;  leggi_eeprom++;
;  }
; else if(leggi_eeprom==5)
;  {
;  leggi_eeprom++;
;  }
; else if(contatore_leggi_eeprom<ultimo_dato_in_lettura)
;  {
;  buffer_eeprom[contatore_leggi_eeprom]=dato_letto;
;  contatore_leggi_eeprom++;
;  }
; else
;  {
;  PTED |=0x80;
;  leggi_eeprom=0;
;  } 
; }
lettura_EE://per comandi lettura eeprom 
 LDA leggi_eeprom
 BEQ fine
 LDA #1
 STA eeprom_impegnata

 LDA leggi_eeprom
 CMP #1
 BNE parte_alta_indirizzo_lettura
; if(leggi_eeprom==1)
;  {
;  PTED &=0x7f; //EEPROM abilitata
;  dato_scritto=0x03;//abilitazione lettura
;  leggi_eeprom++;
;  }
 BCLR 7,_PTED
 LDA #$03
 STA dato_scritto//,_SPID
 LDA leggi_eeprom
 INCA
 STA leggi_eeprom
 BRA fine_SPID

parte_alta_indirizzo_lettura:
 CMP #2
 BNE parte_media_indirizzo_lettura
; else if(leggi_eeprom==2) //spazio di disabilatazione e riabilitazione
;  {
;  dato_scritto=primo_indirizzo_lettura>>16;//parte alta primo indirizzo
;  leggi_eeprom++;
;  }
 LDA primo_indirizzo_lettura
 STA dato_scritto  
 LDA leggi_eeprom
 INCA
 STA leggi_eeprom
 BRA fine_SPID

parte_media_indirizzo_lettura:
 CMP #3
 BNE parte_bassa_indirizzo_lettura
 LDA primo_indirizzo_lettura:1
 STA dato_scritto
 LDA leggi_eeprom
 INCA
 STA leggi_eeprom
 BRA fine_SPID

parte_bassa_indirizzo_lettura:
 CMP #4
 BNE primo_byte_letto
 LDA primo_indirizzo_lettura:2
 STA dato_scritto
 LDA leggi_eeprom
 INCA
 STA leggi_eeprom
 BRA fine_SPID

primo_byte_letto:
 CMP #5
 BNE lettura_dati
 LDA leggi_eeprom
 INCA
 STA leggi_eeprom
 BRA fine_SPID

lettura_dati:
 LDHX contatore_leggi_eeprom
 CPHX ultimo_dato_in_lettura
 BCC fine_lettura
; else if(contatore_leggi_eeprom<ultimo_dato_in_lettura)
;  {
;  buffer_eeprom[contatore_leggi_eeprom]=dato_letto;
;  contatore_leggi_eeprom++;
;  }
 LDA dato_letto
 STA @buffer_eeprom,X
 LDA contatore_leggi_eeprom:1
 ADD #1
 STA contatore_leggi_eeprom:1
 LDA contatore_leggi_eeprom
 ADC #0
 STA contatore_leggi_eeprom
 BRA fine_SPID

fine_lettura: 
 BSET 7,_PTED;//disabilita eepprom
 CLRA
 STA leggi_eeprom
 
fine_SPID:
 LDA dato_scritto
 STA SPI1D
 BRA Fine
 
fine:
 CLRA
 STA eeprom_impegnata 
 STA salvataggio_allarme
Fine: 
 } 
}

void trasmissione_misure_istantanee(void) //con allarme_in_lettura == 9000
{
asm
 {
//V1,V2,V3,I1,I2,I3,cosfi1,cosfi2,cosfi3,pressione,temperatura
 LDHX allarme_in_lettura
 CPHX lettura_misure
 BNE fine
 LDHX operazione_effettuata
 STHX allarme_in_lettura
 LDHX V12_rms
 STHX buffer_USB
 LDHX V23_rms
 STHX buffer_USB:2
 LDHX V31_rms
 STHX buffer_USB:4
 LDHX I1_rms
 STHX buffer_USB:6
 LDHX I2_rms
 STHX buffer_USB:8
 LDHX I3_rms
 STHX buffer_USB:10
 LDHX potenzaI1xV1
 STHX buffer_USB:12
 LDHX potenzaI1xV1
 STHX buffer_USB:14
 LDHX potenzaI1xV1
 STHX buffer_USB:16
 LDHX cosfi
 STHX buffer_USB:18
 LDHX cosfi:2
 STHX buffer_USB:20
 LDHX media_pressione
 STHX buffer_USB:22
 LDHX media_temperatura
 STHX buffer_USB:24
fine: 
 }
}
 
void trasmissione_tarature(void) //con allarme_in_lettura == lettura_tarature
{
asm
 {
 LDHX allarme_in_lettura
 CPHX lettura_tarature
 BNE fine
 LDHX operazione_effettuata
 STHX allarme_in_lettura
 LDHX #96
assegna_buffer:
 LDA @set:-1,X
 STA @buffer_USB:-1,X
 DBNZX assegna_buffer
fine: 
 }
}
 
void presenta_menu(unsigned char *menu, char idioma, char lunghezza , char riga_inizio)
{
//cursor da 0 in su
unsigned char j;
int indirizzo;
asm
 { 
; indirizzo = menu +riga_inizio*17 + idioma*lunghezza*17;
 LDA idioma
 LDX lunghezza
 MUL
 ADD riga_inizio
 LDX #17
 MUL
 STA indirizzo:1
 STX indirizzo
 
 LDA indirizzo:1
 ADD menu:1
 STA indirizzo:1
 LDA indirizzo
 ADC menu
 STA indirizzo
 
;for(j=0; j<34; j++)
; {
; display[j]=*((char*)(&indirizzo+j));
; }
 LDA #34
 STA j
ripeti:
 LDA indirizzo:1
 ADD j
 TAX
 LDA indirizzo
 ADC #0
 PSHA
 PULH
 LDA -1,X
 LDX j
 CLRH
 STA @display:-1,X
 DBNZ j,ripeti
 }
}

void presenta_scritta(char *stringa, char offset, char idioma, char lunghezza, char riga,char colonna, char N_caratteri)
{
unsigned char j, k;
int indirizzo;
asm
 { 
; indirizzo = stringa + (offset + idioma*lunghezza)*17;
 LDA idioma
 LDX lunghezza
 MUL
 ADD offset
 LDX #17
 MUL
 STA indirizzo:1
 STX indirizzo
 
 LDA indirizzo:1
 ADD stringa:1
 STA indirizzo:1
 LDA indirizzo
 ADC stringa
 STA indirizzo

 LDA riga
 LDX #17
 MUL
 ADD colonna
 STA k
  
;for(j=0; j<N_caratteri; j++)
; {
; if(k+j<34) display[k+j]=*((char*)(&indirizzo+j));
; }
 CLR j
ripeti:
 LDA k
 CMP #34
 BCC fine
 
 LDA indirizzo:1
 ADD j
 TAX
 LDA indirizzo
 ADC #0
 PSHA
 PULH
 LDA 0,X
 LDX k
 CLRH
 STA @display,X

 INC k
 INC j
 LDA j
 CMP N_caratteri
 BCS ripeti
fine: 
 }
}

void modifica_unsigned(unsigned int *var, unsigned int min, unsigned int max, char velocita)
{
unsigned int variab;
asm
 {
;if(salita_piu)
; {
; timer_inc_dec=0x1000;
; timer_piu_meno=500;
; if(*var>=max) *var=max; else *var++;
; salita_piu=0;
; }
;else if(salita_meno)
; {
; timer_inc_dec=0x1000;
; timer_piu_meno=500;
; if(*var<=min) *var=min; else *var--;
; salita_meno=0;
; }
;else if(timer_piu_meno==0)
; {
; if(piu==filtro_pulsanti)
;  {
;  if(*var>=max) *var=max; else *var++;
;  timer_piu_meno=timer_inc_dec>>4;
;  if(velocita==0)
;   {
;   if(timer_piu_meno<10) timer_inc_dec=10;
;   }
;  else if((velocita==2)&&(timer_piu_meno==0)) *var +=9;
;   {
;   if(*var>=max-9) *var +=9;
;   }
;  }
; else if(meno==filtro_pulsanti)
;  {
;  if(*var<=min) *var=min; else *var--;
;  timer_piu_meno=timer_inc_dec>>4;
;  if(velocita==0)
;   {
;   if(timer_piu_meno<10) timer_inc_dec=10;
;   }
;  else if((velocita==2)&&(timer_piu_meno==0))
;   {
;   if(*var<=min+9) *var -=9;
;   }
;  }
; } 
 LDHX var
 LDA 0,X
 STA variab
 LDA 1,X
 STA variab:1
 
 LDA salita_piu
 BEQ vedi_salita_meno
 LDHX #$1000
 STHX timer_inc_dec
 LDHX #500
 STHX timer_piu_meno
 CLRA
 STA salita_piu
; if(*var>=max) *var=max; else *var++;
 LDHX max
 CPHX variab
 BEQ fine
 BCC aggiungi
 STHX variab
 BRA fine
aggiungi:
 LDA variab:1
 ADD #1
 STA variab:1
 LDA variab
 ADC #0
 STA variab 
 BRA fine

vedi_salita_meno:
 LDA salita_meno
 BEQ vedi_timer
 LDHX #$1000
 STHX timer_inc_dec
 LDHX #500
 STHX timer_piu_meno
 CLRA
 STA salita_meno
; if(*var<=min) *var=min; else *var--;
 LDHX min
 CPHX variab
 BEQ fine
 BCS sottrai
 STHX variab
 BRA fine
sottrai:
 LDA variab:1
 SUB #1
 STA variab:1
 LDA variab
 SBC #0
 STA variab 
 BRA fine

vedi_timer:
 LDHX timer_piu_meno
 BNE fine
 LDA piu
 CMP filtro_pulsanti
 BNE vedi_meno
; if(*var>=max) *var=max; else *var++;
 LDHX max
 CPHX variab
 BEQ fine
 BCC Aggiungi
 STHX variab
 BRA fine
Aggiungi:
 LDA variab:1
 ADD #1
 STA variab:1
 LDA variab
 ADC #0
 STA variab 
 LDHX #16
 LDA timer_inc_dec
 DIV
 STA timer_piu_meno
 LDA timer_inc_dec:1
 DIV
 STA timer_piu_meno:1
 LDA velocita
 BNE vedi_velocita_massima
 LDHX #10
 CPHX timer_piu_meno
 BCS fine
 STHX timer_piu_meno
 BRA fine
vedi_velocita_massima:
 CMP #2
 BNE fine
 LDHX #65525
 CPHX variab
 BCS fine
 LDA variab:1
 ADD #9
 STA variab:1
 LDA variab
 ADC #0
 STA variab 
 BRA fine
 
vedi_meno:
 LDA meno
 CMP filtro_pulsanti
 BNE fine
; if(*var<=min) *var=min; else *var--;
 LDHX min
 CPHX variab
 BEQ fine
 BCS Sottrai
 STHX variab
 BRA fine
Sottrai:
 LDA variab:1
 SUB #1
 STA variab:1
 LDA variab
 SBC #0
 STA variab 
 LDHX #16
 LDA timer_inc_dec
 DIV
 STA timer_piu_meno
 LDA timer_inc_dec:1
 DIV
 STA timer_piu_meno:1
 LDA velocita
 BNE Vedi_velocita_massima
 LDHX #10
 CPHX timer_piu_meno
 BCS fine
 STHX timer_piu_meno
Vedi_velocita_massima:
 CMP #2
 BNE fine
 LDHX #10
 CPHX variab
 BCC fine
 LDA variab:1
 SUB #9
 STA variab:1
 LDA variab
 SBC #0
 STA variab 
 
fine:
 LDHX var
 LDA variab
 STA 0,X
 LDA variab:1
 STA 1,X
 }
}

void presenta_signed(int val, char punto, char riga, char colonna, char n_cifre)
{
char j, k, inizio, s;
asm
 {
 LDA n_cifre
 CMP #6
 BCS procedi
 LDA #5
 STA n_cifre
 
procedi: 
 LDA colonna
 ADD n_cifre
 DECA
 CMP #16
 BCC fine
 LDA riga
 CMP #2
 BCC fine
 LDX #17
 MUL
 ADD colonna
 STA inizio
 
 LDA val
 BPL procedi_presentazione
 LDA #'-'
 LDX inizio
 CLRH
 STA @display,X
 INC inizio
 DEC n_cifre
 CLRA
 SUB val:1
 STA val:1
 CLRA
 SBC val
 STA val
 
procedi_presentazione: 
 CLR j
ripeti: 
 CLRH
 LDA val
 LDX #10
 DIV
 STA val
 LDA val:1
 DIV
 STA val:1
 PSHH
 PULA
 LDX j
 CLRH
 ORA #$30
 STA @cifre,X;//cifra 0 la meno significativa
 INC j
 LDA j
 CMP n_cifre
 BNE ripeti
  
 LDA punto
 BEQ presenta
 LDA n_cifre
 SUB punto
 STA j
 LDA n_cifre
 STA k
Ripeti:
 CLRH
 LDX k
 LDA @cifre:-1,X
 STA @cifre,X
 LDA #'.'
 STA @cifre:-1,X
 DEC k
 DBNZ j,Ripeti
  
presenta:
 LDA n_cifre
 STA j
 LDA punto
 BEQ presenta_cifre
 INC j
presenta_cifre:
 CLR s
RIpeti:
 CLRH
 LDX j
 LDA @cifre:-1,X
 STA k
 CMP #$30
 BNE assegna_s
 LDA j
 CMP #1;//ultima cifra presentata
 BEQ scrivi_su_display
 LDA s
 BNE scrivi_su_display
 LDA #' '
 STA k
 BRA scrivi_su_display
assegna_s:
 LDA #1
 STA s 
scrivi_su_display: 
 LDA k
 LDX inizio
 CLRH
 STA @display,X
 INC inizio
 DBNZ j,RIpeti
fine: 
 }
}

void presenta_unsigned(unsigned int val, char punto, char riga, char colonna, char n_cifre)
{
char j, k, inizio, s;
asm
 {
 LDA n_cifre
 CMP #6
 BCS procedi
 LDA #5
 STA n_cifre
 
procedi: 
 LDA colonna
 ADD n_cifre
 DECA
 CMP #16
 BCC fine
 LDA riga
 CMP #2
 BCC fine
 LDX #17
 MUL
 ADD colonna
 STA inizio
 
 CLR j
ripeti: 
 CLRH
 LDA val
 LDX #10
 DIV
 STA val
 LDA val:1
 DIV
 STA val:1
 PSHH
 PULA
 LDX j
 CLRH
 ORA #$30
 STA @cifre,X;//cifra 0 la meno significativa
 INC j
 LDA j
 CMP n_cifre
 BNE ripeti
  
 LDA punto
 BEQ presenta
 LDA n_cifre
 SUB punto
 STA j
 LDA n_cifre
 STA k
Ripeti:
 CLRH
 LDX k
 LDA @cifre:-1,X
 STA @cifre,X
 LDA #'.'
 STA @cifre:-1,X
 DEC k
 DBNZ j,Ripeti
  
presenta:
 LDA n_cifre
 STA j
 LDA punto
 BEQ presenta_cifre
 INC j
presenta_cifre:
 CLR s
RIpeti:
 CLRH
 LDX j
 LDA @cifre:-1,X
 STA k
 CMP #$30
 BNE assegna_s
 LDA j
 CMP #1;//ultima cifra presentata
 BEQ scrivi_su_display
 LDA s
 BNE scrivi_su_display
 LDA #' '
 STA k
 BRA scrivi_su_display
assegna_s:
 LDA #1
 STA s 
scrivi_su_display: 
 LDA k
 LDX inizio
 CLRH
 STA @display,X
 INC inizio
 DBNZ j,RIpeti
fine: 
 }
}

void salva_reset_default(void)
{
char j;
int indirizzo_tabella, funzioni_non_modif[4];
asm
 {
 LDA eeprom_impegnata
 BNE fine
 LDA reset_default
 BEQ fine

 LDHX indirizzo_scrivi_eeprom
 CPHX ultimo_indirizzo_scrittura
 BCS continua_reset
 LDA indirizzo_scrivi_eeprom:2
 CMP ultimo_indirizzo_scrittura:2
 BCC per_default_parametri

continua_reset:
 LDA #32
 STA lunghezza_salvataggio
 LDA indirizzo_scrivi_eeprom:2
 ADD #32
 STA indirizzo_scrivi_eeprom:2
 LDA indirizzo_scrivi_eeprom:1
 ADC #0
 STA indirizzo_scrivi_eeprom:1
 LDA indirizzo_scrivi_eeprom
 ADC #0
 STA indirizzo_scrivi_eeprom
 LDA #1
 STA eeprom_impegnata 
 STA scrivi_eeprom
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
 BRA fine

per_default_parametri:
 LDHX #0 
 STHX set.motore_on
 STX toggle_func
 STX salita_start
 STX toggle_stop
 STX cursore_menu
 
 LDHX set.numero_serie
 STHX funzioni_non_modif
 LDHX set.calibrazione_I1
 STHX funzioni_non_modif:2
 LDHX set.calibrazione_I2
 STHX funzioni_non_modif:4
 LDHX set.calibrazione_I3
 STHX funzioni_non_modif:6
 
 LDA N_tabella:1
 LDX #96
 MUL 
 STA indirizzo_tabella:1
 STX indirizzo_tabella
 CLR j
assegna_parametri: //mette dati di fabbrica
 LDA indirizzo_tabella:1
 ADD j
 TAX
 LDA indirizzo_tabella
 ADC #0
 PSHA
 PULH
 LDA @dati_di_fabbrica,X
 CLRH
 LDX j
 STA @set,X
 STA @buffer_eeprom,X
 INC j
 LDA j
 CMP #96
 BCS assegna_parametri
 
//le tarature rimangono fisse insieme al numero di serie
 LDHX funzioni_non_modif
 STHX set.numero_serie
 LDHX funzioni_non_modif:2
 STHX set.calibrazione_I1
 LDHX funzioni_non_modif:4
 STHX set.calibrazione_I2
 LDHX funzioni_non_modif:6
 STHX set.calibrazione_I3

 CLRA
 STA indirizzo_buffer_eeprom
 STA reset_default 
 STA set.conta_ore
 STA set.conta_ore:1
 STA set.conta_ore:2
 STA set.conta_ore:3
 STA set.conta_ore_funzionamento
 STA set.conta_ore_funzionamento:1
 STA set.conta_ore_funzionamento:2
 STA set.conta_ore_funzionamento:3
 STA conta_secondi
 STA conta_secondi:1
 STA conta_secondi:2
 STA conta_secondi:3
 STA conta_secondi_attivita
 STA conta_secondi_attivita:1
 STA conta_secondi_attivita:2
 STA conta_secondi_attivita:3

 JSR calcolo_delle_costanti
 
 LDHX primo_indirizzo_funzioni
 STHX indirizzo_scrivi_eeprom
 LDA primo_indirizzo_funzioni:2
 STA indirizzo_scrivi_eeprom:2
 LDHX ultimo_indirizzo_funzioni
 STHX ultimo_indirizzo_scrittura
 LDA ultimo_indirizzo_funzioni:2
 STA ultimo_indirizzo_scrittura:2
 LDA #32
 STA lunghezza_salvataggio
 LDA #1
 STA salvataggio_funzioni
 STA eeprom_impegnata 
 STA scrivi_eeprom
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
fine:
 }
}

void salva_impostazioni(void)
{
asm
 {
 LDA eeprom_impegnata
 BNE fine
 LDA reset_default
 BNE fine
 LDA salvataggio_funzioni
 BEQ fine
 CMP #1
 BNE continua_salvataggio
 LDA #2
 STA salvataggio_funzioni
 CLRA
 STA indirizzo_buffer_eeprom
 LDA #32
 STA lunghezza_salvataggio
 LDHX primo_indirizzo_funzioni
 STHX indirizzo_scrivi_eeprom
 LDA primo_indirizzo_funzioni:2
 STA indirizzo_scrivi_eeprom:2
 LDHX ultimo_indirizzo_funzioni
 STHX ultimo_indirizzo_scrittura
 LDA ultimo_indirizzo_funzioni:2
 STA ultimo_indirizzo_scrittura:2
 LDHX #96
ripeti_trasferimento: 
 LDA @set:-1,X
 STA @buffer_eeprom:-1,X
 DBNZX ripeti_trasferimento
 LDA #1
 STA eeprom_impegnata 
 STA scrivi_eeprom
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
 BRA fine

continua_salvataggio: 
 LDA #32
 STA lunghezza_salvataggio
 LDA indirizzo_buffer_eeprom
 ADD #32
 STA indirizzo_buffer_eeprom
 LDA indirizzo_scrivi_eeprom:2
 ADD #32
 STA indirizzo_scrivi_eeprom:2
 LDA indirizzo_scrivi_eeprom:1
 ADC #0
 STA indirizzo_scrivi_eeprom:1
 LDA indirizzo_scrivi_eeprom
 ADC #0
 STA indirizzo_scrivi_eeprom

 LDHX indirizzo_scrivi_eeprom
 CPHX ultimo_indirizzo_scrittura
 BCS continua_Salvataggio
 LDA indirizzo_scrivi_eeprom:2
 CMP ultimo_indirizzo_scrittura:2
 BCC salvataggio_terminato
continua_Salvataggio: 
 LDA #1
 STA eeprom_impegnata 
 STA scrivi_eeprom
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
 BRA fine
salvataggio_terminato:
 CLRA
 STA salvataggio_funzioni 
fine: 
 }
}

void salva_allarme_in_eepprom(void)
{
asm
 {
 LDA eeprom_impegnata
 BNE fine
 LDA reset_default
 BNE fine
 LDA salvataggio_funzioni
 BNE fine
 LDA segnalazione_
 CMP #20
 BCC per_registrazione
 LDHX timer_attesa_segnalazione_fault
 BNE fine
per_registrazione:   
 LDA segnalazione_
 CMP precedente_segnalazione
 BEQ fine
 STA precedente_segnalazione
 LDA #1
 STA salvataggio_allarme

 LDHX set.numero_segnalazione
;  if(set.numero_segnalazione<totale_indicazioni_fault) set.numero_segnalazione++; else set.numero_segnalazione=0;
 CPHX totale_indicazioni_fault
 BCC azzera_contatore
 LDA set.numero_segnalazione:1
 ADD #1
 STA set.numero_segnalazione:1
 LDA set.numero_segnalazione
 ADC #0
 STA set.numero_segnalazione
 BRA assegna_a_buffer_eeprom
azzera_contatore: 
 LDHX #0
 STHX set.numero_segnalazione

assegna_a_buffer_eeprom:
//16 bytes trasmessi per un complesso di 8K 504 registrazioni
// 1 byte = tipo intervento  all'indirizzo binario xxxx xxxx xxxx 0000
// 4 bytes = ora_minuto_secondo  xxxxx.xx.xx  (viene registrata come numero totale di secondi di funzionamento)
// 2 bytes = tensione_media 0-500V
// 1 byte = I1_rms  0-25.5A
// 1 byte = I2_rms  0-25.5A
// 1 byte = I3_rms  0-25.5A
// 2 bytes = potenza  0-10000 W
// 2 bytes = pressione  -1.0 - +50.0 Bar
// 1 byte  = cosfi convenzionale =P/(P^2+Q^2) 0-.99
// 1 byte = temperatura  0-255 �C
 LDA segnalazione_;//tipo intervento:
 STA buffer_eeprom
 LDHX conta_secondi
 STHX buffer_eeprom:1
 LDHX conta_secondi:2
 STHX buffer_eeprom:3
 LDHX tensione_media
 STHX buffer_eeprom:5
 LDHX I1_rms
 CPHX #255
 BCS assegna_I1_rms
 LDX #255
assegna_I1_rms:
 STX buffer_eeprom:7
 LDHX I2_rms
 CPHX #255
 BCS assegna_I2_rms
 LDX #255
assegna_I2_rms:
 STX buffer_eeprom:8
 LDHX I3_rms
 CPHX #255
 BCS assegna_I3_rms
 LDX #255
assegna_I3_rms:
 STX buffer_eeprom:9
 LDHX potenza_media
 STHX buffer_eeprom:10
 LDHX media_pressione
 STHX buffer_eeprom:12
 LDA cosfi:3
 STA buffer_eeprom:14
 LDA media_temperatura
 STA buffer_eeprom:15

 CLRA
 STA indirizzo_buffer_eeprom
 LDA set.numero_segnalazione:1
 LDX #16
 STX lunghezza_salvataggio
 MUL
 STA indirizzo_scrivi_eeprom:2
 STX indirizzo_scrivi_eeprom:1
 LDA set.numero_segnalazione
 LDX #16
 MUL
 ADD indirizzo_scrivi_eeprom:1
 STA indirizzo_scrivi_eeprom:1
 TXA
 ADC #0
 STA indirizzo_scrivi_eeprom
 
 LDA indirizzo_scrivi_eeprom:2
 ADD #16
 STA ultimo_indirizzo_scrittura:2
 LDA indirizzo_scrivi_eeprom:1
 ADC #0
 STA ultimo_indirizzo_scrittura:1
 LDA indirizzo_scrivi_eeprom
 ADC #0
 STA ultimo_indirizzo_scrittura
 LDA #1
 STA eeprom_impegnata 
 STA scrivi_eeprom
 STA salva_conta_secondi
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
fine:
 } 
}

void salva_conta_secondi_attivita(void)//ogni 1 minuti
{
asm
 {
 LDA eeprom_impegnata
 BNE fine
 LDA reset_default
 BNE fine
 LDA salvataggio_funzioni
 BNE fine
 LDA salvataggio_allarme
 BNE fine
 LDA salva_conta_secondi
 BEQ fine
 CLRA
 STA salva_conta_secondi
 LDHX indirizzo_conta_ore
 STHX indirizzo_scrivi_eeprom
 LDA indirizzo_conta_ore:2
 STA indirizzo_scrivi_eeprom:2
 LDHX set.motore_on
 STHX buffer_eeprom
 LDHX set.numero_segnalazione
 STHX buffer_eeprom:2
 LDHX conta_secondi
 STHX set.conta_ore
 STHX buffer_eeprom:4
 LDHX conta_secondi:2
 STHX set.conta_ore:2
 STHX buffer_eeprom:6
 LDHX conta_secondi_attivita
 STHX set.conta_ore_funzionamento
 STHX buffer_eeprom:8
 LDHX conta_secondi_attivita:2
 STHX set.conta_ore_funzionamento:2
 STHX buffer_eeprom:10
 CLRA
 STA indirizzo_buffer_eeprom
 LDA #16
 STA lunghezza_salvataggio
 LDA #1
 STA eeprom_impegnata 
 STA scrivi_eeprom
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
fine: 
 }
}

void lettura_impostazioni(void)
{
char j;
int indirizzo_tabella, var, def, min, max;
asm
 {
 LDA eeprom_impegnata
 BNE fine
 LDA leggi_impostazioni
 BEQ fine
 LDA reset_default
 BNE fine
 LDA salvataggio_funzioni
 BNE fine
 LDA salvataggio_allarme
 BNE fine
 LDA salva_conta_secondi
 BNE fine
 LDA #2
 CMP leggi_impostazioni
 BEQ vedi_lettura_ultimata
 STA leggi_impostazioni
 LDHX primo_indirizzo_funzioni
 STHX primo_indirizzo_lettura
 LDHX primo_indirizzo_funzioni:2
 STHX primo_indirizzo_lettura:2
 LDHX #0
 STHX contatore_leggi_eeprom
 LDHX #96
 STHX ultimo_dato_in_lettura
 LDA #1
 STA eeprom_impegnata 
 STA leggi_eeprom
 BSET 7,_PTED;//disabilitazione eeprom
 MOV #$ff,SPI1D
 BRA fine

vedi_lettura_ultimata:
 LDA leggi_impostazioni
 CMP #2
 BNE fine
 CLRA
 STA leggi_impostazioni

 LDA N_tabella:1
 LDX #96
 MUL 
 STA indirizzo_tabella:1
 STX indirizzo_tabella
 CLR j
assegna_parametri:
 LDA j
 LSLA
 ADD indirizzo_tabella:1
 TAX
 LDA indirizzo_tabella
 ADC #0
 PSHA
 PULH
 LDA @dati_di_fabbrica:1,X
 STA def:1
 LDA @dati_di_fabbrica,X
 STA def
 LDA @limiti_inferiori:1,X
 STA min:1
 LDA @limiti_inferiori,X
 STA min
 LDA @limiti_superiori:1,X
 STA max:1
 LDA @limiti_superiori,X
 STA max
 CLRH
 LDX j
 LSLX
 LDA @buffer_eeprom:1,X
 STA var:1
 LDA @buffer_eeprom,X
 STA var
 LDHX var
 CPHX min
 BCC vedi_max
 LDHX def
 STHX var
 BRA assegna
vedi_max: 
 LDHX var
 CPHX max
 BEQ assegna
 BCS assegna
 LDHX def
 STHX var
assegna:
 CLRH
 LDX j
 LSLX
 LDA var:1
 STA @set:1,X
 LDA var
 STA @set,X
 INC j
 LDA j
 CMP #48
 BCS assegna_parametri
fine:
 } 
}

void lettura_allarme_messo_in_buffer_USB(void)  //con allarme_in_lettura <totale_indicazioni_fault
{
//la richiesta viene effettuata tramite USB dando un valore a allarme_in_lettura < totale_indicazioni_fault
//dopo il tempo necessario alla lettura i dati vengono posti in buffer_USB e spediti
asm
 {
 LDA eeprom_impegnata
 BNE fine
 LDA leggi_impostazioni
 BNE fine
 LDA reset_default
 BNE fine
 LDA salvataggio_funzioni
 BNE fine
 LDA salva_conta_secondi
 BNE fine
 LDA salvataggio_allarme
 BNE fine
 LDA attesa_invio
 BNE vedi_fine_lettura_eeprom
 LDHX allarme_in_lettura
 CPHX totale_indicazioni_fault
 BCC fine
 CPHX precedente_lettura_allarme
 BEQ fine
 STHX precedente_lettura_allarme

 LDA allarme_in_lettura:1
 LDX #16
 MUL
 STA primo_indirizzo_lettura:2
 STX primo_indirizzo_lettura:1
 LDA allarme_in_lettura
 LDX #16
 MUL
 ADD primo_indirizzo_lettura:1
 STA primo_indirizzo_lettura:1
 TXA
 ADC #0
 STA primo_indirizzo_lettura
 
 LDHX #0
 STHX contatore_leggi_eeprom
 LDHX #16
 STHX ultimo_dato_in_lettura
 LDA #1
 STA eeprom_impegnata 
 STA attesa_invio
 STA leggi_eeprom
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
 BRA fine
 
vedi_fine_lettura_eeprom:
 LDA attesa_invio
 BEQ fine
 CLRA
 STA attesa_invio
 LDHX #16
ripeti_assegnazione:
 LDA @buffer_eeprom:-1,X
 STA @buffer_USB:-1,X
 DBNZX ripeti_assegnazione
 LDA #1
 STA pronto_alla_risposta
fine:
 } 
}

const int mille678=1678, mille515=1515;
void calcolo_delle_costanti(void)
{
char j;
int quad_In;
long prod, fattore;
asm
 { 
//limitazione_I_avviamento=2*set.corrente_nominale;
 LDA set.corrente_nominale:1
 LSLA
 STA limitazione_I_avviamento:1
 LDA set.corrente_nominale
 ROLA
 STA limitazione_I_avviamento
 
//timer_ritorno_da_emergenza=set.ritardo_funzionamento_dopo_emergenza*60;
 LDA set.ritardo_funzionamento_dopo_emergenza:1
 LDX #60
 MUL
 STA timer_ritorno_da_emergenza:1
 STX timer_ritorno_da_emergenza
 
//potenza_a_secco = set.potenza_minima_funz_secco*potenza_nominale/10; //W
 LDA set.potenza_nominale:1
 LDX set.potenza_minima_funz_secco:1
 MUL
 STA prod:2
 STX prod:1
 LDA set.potenza_nominale
 LDX set.potenza_minima_funz_secco:1
 MUL
 ADD prod:1
 STA prod:1
 TXA
 ADC #0
 STA prod
 LDHX #10
 DIV
 LDA prod:1
 DIV 
 STA prod:1
 LDA prod:2
 DIV 
 STA prod:2
 LDHX prod:1
 STHX potenza_a_secco

//potenza_mandata_chiusa = set.potenza_minima_mandata_chiusa*potenza_nominale/10; //W
 LDA set.potenza_nominale:1
 LDX set.potenza_minima_mandata_chiusa:1
 MUL
 STA prod:2
 STX prod:1
 LDA set.potenza_nominale
 LDX set.potenza_minima_mandata_chiusa:1
 MUL
 ADD prod:1
 STA prod:1
 TXA
 ADC #0
 STA prod
 LDHX #10
 DIV
 LDA prod:1
 DIV 
 STA prod:1
 LDA prod:2
 DIV 
 STA prod:2
 LDHX prod:1
 STHX potenza_mandata_chiusa

 //per il calcolo della sovratemperatura
;//incremento = (corrente_media^2 * delta_Tn / corrente_nominale^2 - delta_T)/costante_tau_salita_temperatura
;//fattore_I2xT = delta_Tn / corrente_nominale^2;
 LDA set.corrente_nominale:1
 TAX
 MUL
 STA quad_In:1
 STX quad_In

 LDHX delta_Tn
 STHX fattore
 CLR fattore:2
 CLR fattore:3
 
 LDA #17
 STA j
differenza: 
 LDA fattore:1
 SUB quad_In:1
 TAX
 LDA fattore
 SBC quad_In
 BCS scorrimento
 STA fattore
 STX fattore:1
scorrimento: 
 ROL fattore:3
 ROL fattore:2
 ROL fattore:1
 ROL fattore:0
 DBNZ j,differenza
 COM fattore:3
 COM fattore:2
 LDHX fattore:2
 STHX fattore_I2xT

//livello di sovraccarico ammissibile �C
;sovraccarico = delta_Tn*(set.limite_sovracorrente/100)^2;
;sovraccarico = delta_Tn*set.limite_sovracorrente^2*1678/256^3;
 LDA set.limite_sovracorrente:1
 TAX
 MUL
 STA prod:1
 STX prod
 LDA prod:1
 LDX delta_Tn:1
 MUL
 STA prod:2
 STX prod:1
 LDA prod
 LDX delta_Tn:1
 MUL
 ADD prod:1
 STA prod:1
 TXA
 ADC #0
 STA prod
 
 LDA prod:2
 LDX mille678:1
 MUL
 STX fattore:3
 LDA prod:2
 LDX mille678
 MUL
 ADD fattore:3
 STA fattore:3
 TXA
 ADC #0
 STA fattore:2
 LDA prod:1
 LDX mille678:1
 MUL
 ADD fattore:3
 STA fattore:3
 TXA
 ADC fattore:2
 STA fattore:2
 CLR fattore:1
 ROL fattore:1
 LDA prod:1
 LDX mille678
 MUL
 ADD fattore:2
 STA fattore:2
 TXA
 ADC fattore:1
 STA fattore:1
 LDA prod
 LDX mille678:1
 MUL
 ADD fattore:2
 STA fattore:2
 TXA
 ADC fattore:1
 STA fattore:1
 CLR fattore
 ROL fattore
 LDA prod
 LDX mille678
 MUL
 ADD fattore:1
 STA fattore:1
 TXA
 ADC fattore
 STA fattore
 LDHX fattore
 STHX sovraccarico

//livello di sovraccarico ammissibile �C
;sovraccarico_moderato = delta_Tn*(set.limite_sovracorrente/100)^2;
;sovraccarico_moderato = delta_Tn*set.limite_sovracorrente^2*1515/256^3;
 LDA set.limite_sovracorrente:1
 TAX
 MUL
 STA prod:1
 STX prod
 LDA prod:1
 LDX delta_Tn:1
 MUL
 STA prod:2
 STX prod:1
 LDA prod
 LDX delta_Tn:1
 MUL
 ADD prod:1
 STA prod:1
 TXA
 ADC #0
 STA prod
 
 LDA prod:2
 LDX mille515:1
 MUL
 STX fattore:3
 LDA prod:2
 LDX mille515
 MUL
 ADD fattore:3
 STA fattore:3
 TXA
 ADC #0
 STA fattore:2
 LDA prod:1
 LDX mille515:1
 MUL
 ADD fattore:3
 STA fattore:3
 TXA
 ADC fattore:2
 STA fattore:2
 CLR fattore:1
 ROL fattore:1
 LDA prod:1
 LDX mille515
 MUL
 ADD fattore:2
 STA fattore:2
 TXA
 ADC fattore:1
 STA fattore:1
 LDA prod
 LDX mille515:1
 MUL
 ADD fattore:2
 STA fattore:2
 TXA
 ADC fattore:1
 STA fattore:1
 CLR fattore
 ROL fattore
 LDA prod
 LDX mille515
 MUL
 ADD fattore:1
 STA fattore:1
 TXA
 ADC fattore
 STA fattore
 LDHX fattore
 STHX sovraccarico_moderato

//dissimmetria_tollerata, V
 LDA set.tensione_nominale:1
 LDX set.limite_segnalazione_dissimmetria:1
 MUL
 STA prod:1
 STX prod
 LDA set.tensione_nominale
 LDX set.limite_segnalazione_dissimmetria:1
 MUL
 ADD prod
 STA prod
 LDHX #100
 LDA prod
 DIV
 STA dissimmetria_tollerata
 LDA prod:1
 DIV
 STA dissimmetria_tollerata:1
//dissimmetria_emergenza, V
 LDA set.tensione_nominale:1
 LDX set.limite_intervento_dissimmetria:1
 MUL
 STA prod:1
 STX prod
 LDA set.tensione_nominale
 LDX set.limite_intervento_dissimmetria:1
 MUL
 ADD prod
 STA prod
 LDHX #100
 LDA prod
 DIV
 STA dissimmetria_emergenza
 LDA prod:1
 DIV
 STA dissimmetria_emergenza:1

//squilibrio_tollerato, A*10
 LDA set.corrente_nominale:1
 LDX set.limite_segnalazione_squilibrio:1
 MUL
 STA prod:1
 STX prod
 LDA set.corrente_nominale
 LDX set.limite_segnalazione_squilibrio:1
 MUL
 ADD prod
 STA prod
 LDHX #100
 LDA prod
 DIV
 STA squilibrio_tollerato
 LDA prod:1
 DIV
 STA squilibrio_tollerato:1
//squilibrio_emergenza; A*10
 LDA set.corrente_nominale:1
 LDX set.limite_intervento_squilibrio:1
 MUL
 STA prod:1
 STX prod
 LDA set.corrente_nominale
 LDX set.limite_intervento_squilibrio:1
 MUL
 ADD prod
 STA prod
 LDHX #100
 LDA prod
 DIV
 STA squilibrio_emergenza
 LDA prod:1
 DIV
 STA squilibrio_emergenza:1

// ;sovratensione_consentita = set.tensione_nominale*set.limite_sovratensione/100;
 LDA set.tensione_nominale:1
 LDX set.limite_sovratensione:1
 MUL
 STA prod:1
 STX prod
 LDA set.tensione_nominale
 LDX set.limite_sovratensione:1
 MUL
 ADD prod
 STA prod
 LDHX #100
 LDA prod
 DIV
 STA sovratensione_consentita
 LDA prod:1
 DIV
 STA sovratensione_consentita:1

//  ;sottotensione_consentita = set.tensione_nominale*set.limite_sottotensione/100;
 LDA set.tensione_nominale:1
 LDX set.limite_sottotensione:1
 MUL
 STA prod:1
 STX prod
 LDA set.tensione_nominale
 LDX set.limite_sottotensione:1
 MUL
 ADD prod
 STA prod
 LDHX #100
 LDA prod
 DIV
 STA sottotensione_consentita
 LDA prod:1
 DIV
 STA sottotensione_consentita:1

//  ;sottotensione_consentita = set.tensione_nominale*set.tensione_restart/100
 LDA set.tensione_nominale:1
 LDX set.tensione_restart:1
 MUL
 STA prod:1
 STX prod
 LDA set.tensione_nominale
 LDX set.tensione_restart:1
 MUL
 ADD prod
 STA prod
 LDHX #100
 LDA prod
 DIV
 STA ripresa_per_tensione_consentita
 LDA prod:1
 DIV
 STA ripresa_per_tensione_consentita:1

//(A*.004*5/.512* 4096/5)^2 = A^2*1024  = (A*10)^2*10.24 = (A*10)^2*6400>>16
;Id_rms = mA in = mA out * set.scala_corrente_differenziale /( .15/5*4096 *5) 
;Id_rms^2 = media_quad_Id^2 * set.scala_corrente_differenziale^2 /5.76 /2^16
;Id_rms = sqr(media_quad_Id^2 * set.scala_corrente_differenziale^2 *4/23 /2^16)
;Id_rms = sqr(media_quad_Id^2 * fattore_Id)
;fattore_Id = set.scala_corrente_differenziale^2 *4/23 /2^16
 LDA set.scala_corrente_differenziale:1
 TAX
 MUL
 STA prod:2
 STX prod:1
 LDA set.scala_corrente_differenziale:1
 LDX set.scala_corrente_differenziale
 MUL
 LSLA
 ROLX
 ADD prod:1
 STA prod:1
 TXA
 ADC #0
 STA prod
 LDA set.scala_corrente_differenziale
 TAX
 MUL
 ADD prod
 STA prod
 LSL prod:2
 ROL prod:1
 ROL prod
 LSL prod:2
 ROL prod:1
 ROL prod
 LDHX #23
 LDA prod
 DIV
 LDA prod:1
 DIV
 STA fattore_Id
 LDA prod:2
 DIV
 STA fattore_Id:1
 }
}

void programmazione(void)
{
long secondi;
if(salita_func)
 {
 salita_func=0;
 timer_reset=5;
 allarme_in_lettura=0;
 salita_piu=0;
 salita_meno=0;
 salita_start=0;
 start=0;
 set.motore_on=0;
 if(numero_ingresso==chiave_ingresso) toggle_stop=0; else toggle_stop=1;
 if(toggle_func)
  {
  calcolo_delle_costanti();
  //comando di salvataggio in eeprom
  salvataggio_funzioni=1;
  presenta_menu((unsigned char*)&comandi_da_effettuare,(char)set.lingua,2,0);
  toggle_func=0;
  }
 else
  { 
  toggle_func=1;
  segnalazione_=0;
  relais_alimentazione=0;
  if(numero_ingresso==chiave_ingresso) cursore_menu=1; else cursore_menu=0;
  presenta_menu((unsigned char*)&menu_principale,(char)set.lingua,80,cursore_menu<<1);
  }
 }
 
if(toggle_func)//lettura e modifica delle funzioni
 {
 if(salita_stop)
  {
  salita_stop=0;
  salita_piu=0;
  salita_meno=0;
  timer_reset=5;
  salita_start=0;
  start=0;
  if(cursore_menu==34) //presentazione dell'elenco emergenze
   {
   if(allarme_in_lettura>=totale_indicazioni_fault) allarme_in_lettura=0; else allarme_in_lettura++;
   toggle_stop=1;
   } 
  else 
   {
   if(toggle_stop) toggle_stop=0; else toggle_stop=1;
   }
  }
 
 if(toggle_stop==0)//modifica della funzione
  {
  if(salita_meno)
   {
   salita_meno=0;
   if(cursore_menu<40) cursore_menu++; else if(numero_ingresso==chiave_ingresso) cursore_menu=1; else cursore_menu=0;
   if(numero_ingresso==chiave_numero_serie)
    {
    if(cursore_menu==40) cursore_menu=35;
    } 
   else// if(numero_ingresso!=chiave_numero_serie)
    {
    if(cursore_menu>35)
     {
     if(numero_ingresso==chiave_ingresso) cursore_menu=1; else cursore_menu=0; //non presenta le tarature
     }
    }
   if(set.monofase_trifase==0)
    {
    if(cursore_menu==8) cursore_menu++; //solo trifase
    if(cursore_menu==9) cursore_menu++; //solo trifase
    if(cursore_menu==12) cursore_menu++; //solo trifase
    if(cursore_menu==13) cursore_menu++; //solo trifase
    }
   presenta_menu((unsigned char*)&menu_principale,(char)set.lingua,80,cursore_menu<<1);
   }
  else if(salita_piu)
   {
   salita_piu=0;
   if(numero_ingresso==chiave_ingresso)
    {
    if(cursore_menu>1) cursore_menu--; else cursore_menu=39;
    } 
   else
    {
    if(cursore_menu>0) cursore_menu--; else cursore_menu=39;
    }
   if(numero_ingresso==chiave_numero_serie)
    {
    if(cursore_menu<35) cursore_menu=39;
    } 
   else// if(numero_ingresso!=chiave_numero_serie)
    {
    if(cursore_menu>35) cursore_menu=35; //non presenta le tarature
    }
   if(set.monofase_trifase==0)
    {
    if(cursore_menu==13) cursore_menu--; //solo trifase
    if(cursore_menu==12) cursore_menu--; //solo trifase
    if(cursore_menu==9) cursore_menu--; //solo trifase
    if(cursore_menu==8) cursore_menu--; //solo trifase
    }
   presenta_menu((unsigned char*)&menu_principale,(char)set.lingua,80,cursore_menu<<1);
   }
  switch(cursore_menu)
   {
   case 0://abilitazione delle funzioni
    {
    if(numero_ingresso==chiave_ingresso) presenta_scritta((char *)&accesso_password,0,(char)set.lingua,1,0,0,16);
    presenta_unsigned(numero_ingresso,0,1,10,3);
    } break;
   case 1:
    {
    if(set.monofase_trifase) presenta_scritta((char*)&presenta_monofase_trifase,1,(char)set.lingua,2,1,0,16);
    else presenta_scritta((char*)&presenta_monofase_trifase,0,(char)set.lingua,2,1,0,16);
    } break;
   case 2: presenta_unsigned(set.potenza_nominale,2,1,10,3); break;
   case 3: presenta_unsigned(set.tensione_nominale,0,1,10,3); break;
   case 4: presenta_unsigned(set.corrente_nominale,1,1,10,3); break;
   case 5: presenta_unsigned(set.limite_sovratensione,0,1,10,3); break;
   case 6: presenta_unsigned(set.limite_sottotensione,0,1,10,3); break;
   case 7: presenta_unsigned(set.tensione_restart,0,1,10,3); break;
   case 8: presenta_unsigned(set.limite_segnalazione_dissimmetria,0,1,10,3); break;
   case 9: presenta_unsigned(set.limite_intervento_dissimmetria,0,1,10,3); break;
   case 10: presenta_unsigned(set.timeout_protezione_tensione,0,1,12,3); break;
   case 11: presenta_unsigned(set.limite_sovracorrente,0,1,10,3); break;
   case 12: presenta_unsigned(set.limite_segnalazione_squilibrio,0,1,11,3); break;
   case 13:presenta_unsigned(set.limite_intervento_squilibrio,0,1,11,3); break;
   case 14: presenta_unsigned(set.ritardo_protezione_squilibrio,0,1,12,3); break;
   case 15: presenta_unsigned(set.limite_corrente_differenziale,0,1,10,3); break;
   case 16: presenta_unsigned(set.ritardo_intervento_differenziale,0,1,11,3); break;
   case 17:
    {
    if(set.abilita_sensore_pressione) presenta_scritta((char*)&lettura_allarmi,2,0,25,1,0,16);
    else presenta_scritta((char*)&lettura_allarmi,1,0,25,1,0,16);
    } break;
   case 18: presenta_unsigned(set.pressione_emergenza,1,1,9,3); break;
   case 19: presenta_unsigned(set.pressione_spegnimento,1,1,9,3); break;
   case 20: presenta_unsigned(set.pressione_accensione,1,1,9,3); break;
   case 21: presenta_unsigned(set.ritardo_funzionamento_dopo_emergenza,0,1,10,2); break;
   case 22: presenta_unsigned(set.potenza_minima_mandata_chiusa,0,1,10,3); break;
   case 23: presenta_unsigned(set.ritardo_stop_mandata_chiusa,0,1,11,3); break;
   case 24: presenta_unsigned(set.ritardo_riaccensione_mandata_chiusa,0,1,11,3); break;
   case 25: presenta_unsigned(set.potenza_minima_funz_secco,0,1,10,3); break;
   case 26: presenta_unsigned(set.ritardo_stop_funzionemento_a_secco,0,1,11,3); break;
   case 27: presenta_unsigned(set.portata_sensore_pressione,1,1,10,3); break;
   case 28:
    {
    if(set.modo_start_stop) presenta_scritta((char*)&presenta_tipo_start_stop,1,(char)set.lingua,2,1,0,16);
    else presenta_scritta((char*)&presenta_tipo_start_stop,0,(char)set.lingua,2,1,0,16);
    } break;
   case 29: presenta_unsigned(set.scala_corrente_differenziale,0,1,7,3); break;
   case 30: presenta_unsigned(set.costante_tau_salita_temperatura,0,1,10,3); break;
   case 31: presenta_unsigned(set.scala_temperatura_motore,0,1,8,3); break;
   case 32: presenta_unsigned(set.taratura_temperatura_ambiente,0,1,11,3); break;
   case 33: presenta_unsigned(set.limite_intervento_temper_motore,0,1,11,3); break;

   case 35: presenta_unsigned(set.numero_serie,0,1,7,5); break;
   case 36: presenta_scritta((char*)&comando_reset,1,0,1,1,0,16); break;
   case 37: presenta_unsigned(set.calibrazione_I1,0,0,12,3); break;
   case 38: presenta_unsigned(set.calibrazione_I3,0,0,12,3); break;
   case 39: presenta_unsigned(set.calibrazione_I2,0,0,12,3); break;
   }
  }
 //if(toggle_stop) modifica del valore 
 else if(cursore_menu==0)  //abilitazione delle funzioni
  {
  modifica_unsigned((unsigned int*)&numero_ingresso,0,999,0);
  presenta_unsigned(numero_ingresso,0,1,10,3);
  }
 else if(cursore_menu==35) //numero serie
  {
  if(numero_ingresso==chiave_numero_serie) //solo presentazione con chiave non corretta
   {
   modifica_unsigned((unsigned int*)&set.numero_serie,1,65535,2);
   presenta_unsigned(set.numero_serie,0,1,7,5);
   }
  } 
 else if(cursore_menu==36) //reset
  {
  if(piu==filtro_pulsanti)
   {
   presenta_scritta((char*)&comando_reset,0,0,1,1,0,9);
   presenta_unsigned(timer_reset,0,1,10,2);
   if((timer_reset==0)&&(reset_default==0))
    {
    salita_start=0;
    start=0;
    set.motore_on=0;
    reset_default=1;
    toggle_func=0;
    toggle_stop=0;
    indirizzo_scrivi_eeprom=0;
    indirizzo_buffer_eeprom=0;
    lunghezza_salvataggio=32;
    ultimo_indirizzo_scrittura=primo_indirizzo_funzioni;
    asm
     {
     LDHX #96
    ripeti_Trasferimento: 
     LDA #$ff
     STA @buffer_eeprom:-1,X
     DBNZX ripeti_Trasferimento
     LDA #1
     STA scrivi_eeprom
     STA salvataggio_funzioni
     BSET 7,_PTED;//abilitazione eeprom
     MOV #$ff,SPI1D
     }
    presenta_scritta((char*)&comando_reset,2,0,1,1,0,16);//reset iniziato
    }
   } 
  else
   {
   timer_reset=5;//5 s attesa reset
   presenta_scritta((char*)&comando_reset,1,0,1,1,0,16);
   }
  }
 else if(cursore_menu==37)
  {
  modifica_unsigned((unsigned int*)&set.calibrazione_I1,115,141,0);
  presenta_unsigned(set.calibrazione_I1,0,0,12,3);
  presenta_unsigned(I1_rms,1,1,3,3);
  presenta_unsigned(I3_rms,1,1,11,3);
  } 
 else if(cursore_menu==38)
  {
  modifica_unsigned((unsigned int*)&set.calibrazione_I3,115,141,0);
  presenta_unsigned(set.calibrazione_I3,0,0,12,3);
  presenta_unsigned(I1_rms,1,1,3,3);
  presenta_unsigned(I3_rms,1,1,11,3);
  } 
 else if(cursore_menu==39)
  {
  modifica_unsigned((unsigned int*)&set.calibrazione_I2,115,141,0);
  presenta_unsigned(set.calibrazione_I2,0,0,12,3);
  presenta_unsigned(I1_rms,1,1,3,3);
  presenta_unsigned(I2_rms,1,1,11,3);
  } 
 else if(numero_ingresso==chiave_ingresso) 
  {
  switch(cursore_menu)
   {
   case 1:
    {
    modifica_unsigned((unsigned int*)&set.monofase_trifase,0,1,0);
    if(set.monofase_trifase) presenta_scritta((char*)&presenta_monofase_trifase,1,(char)set.lingua,2,1,0,16);
    else presenta_scritta((char*)&presenta_monofase_trifase,0,(char)set.lingua,2,1,0,16);
    } break;
   case 2:
    {
    modifica_unsigned((unsigned int*)&set.potenza_nominale,75,550,0);
    presenta_unsigned(set.potenza_nominale,2,1,10,3);
    } break;
   case 3:
    {
    modifica_unsigned((unsigned int*)&set.tensione_nominale,110,440,0);
    presenta_unsigned(set.tensione_nominale,0,1,10,3);
    } break;
   case 4:
    {
    modifica_unsigned((unsigned int*)&set.corrente_nominale,10,160,0);
    presenta_unsigned(set.corrente_nominale,1,1,10,3);
    } break;
   case 5:
    {
    modifica_unsigned((unsigned int*)&set.limite_sovratensione,100,125,0);
    presenta_unsigned(set.limite_sovratensione,0,1,10,3);
    } break;
   case 6:
    {
    modifica_unsigned((unsigned int*)&set.limite_sottotensione,60,95,0);
    presenta_unsigned(set.limite_sottotensione,0,1,10,3);
    } break;
   case 7:
    {
    modifica_unsigned((unsigned int*)&set.tensione_restart,set.limite_sottotensione,100,0);
    presenta_unsigned(set.tensione_restart,0,1,10,3);
    } break;
   case 8:
    {
    modifica_unsigned((unsigned int*)&set.limite_segnalazione_dissimmetria,5,20,0);
    presenta_unsigned(set.limite_segnalazione_dissimmetria,0,1,10,3);
    } break;
   case 9:
    {
    modifica_unsigned((unsigned int*)&set.limite_intervento_dissimmetria,5,25,0);
    presenta_unsigned(set.limite_intervento_dissimmetria,0,1,10,3);
    } break;
   case 10:
    {
    modifica_unsigned((unsigned int*)&set.timeout_protezione_tensione,2,120,0);
    presenta_unsigned(set.timeout_protezione_tensione,0,1,12,3);
    } break;
   case 11:
    {
    modifica_unsigned((unsigned int*)&set.limite_sovracorrente,105,140,0);
    presenta_unsigned(set.limite_sovracorrente,0,1,10,3);
    } break;
   case 12:
    {
    modifica_unsigned((unsigned int*)&set.limite_segnalazione_squilibrio,1,20,0);
    presenta_unsigned(set.limite_segnalazione_squilibrio,0,1,11,3);
    } break;
   case 13:
    {
    modifica_unsigned((unsigned int*)&set.limite_intervento_squilibrio,1,25,0);
    presenta_unsigned(set.limite_intervento_squilibrio,0,1,11,3);
    } break;
   case 14:
    {
    modifica_unsigned((unsigned int*)&set.ritardo_protezione_squilibrio,1,120,0);
    presenta_unsigned(set.ritardo_protezione_squilibrio,0,1,12,3);
    } break;
   case 15:
    {
    modifica_unsigned((unsigned int*)&set.limite_corrente_differenziale,10,500,0);
    presenta_unsigned(set.limite_corrente_differenziale,0,1,10,3);
    } break;
   case 16:
    {
    modifica_unsigned((unsigned int*)&set.ritardo_intervento_differenziale,20,200,0);
    presenta_unsigned(set.ritardo_intervento_differenziale,0,1,11,3);
    } break;
   case 17:
    {
    modifica_unsigned((unsigned int*)&set.abilita_sensore_pressione,0,1,0);
    if(set.abilita_sensore_pressione) presenta_scritta((char*)&lettura_allarmi,2,0,25,1,0,16);
    else presenta_scritta((char*)&lettura_allarmi,1,0,25,1,0,16);
    } break;
   case 18:
    {
    modifica_unsigned((unsigned int*)&set.pressione_emergenza,set.pressione_spegnimento,set.portata_sensore_pressione,0);
    presenta_unsigned(set.pressione_emergenza,1,1,9,3);
    } break;
   case 19:
    {
    modifica_unsigned((unsigned int*)&set.pressione_spegnimento,set.pressione_accensione,set.portata_sensore_pressione,0);
    presenta_unsigned(set.pressione_spegnimento,1,1,9,3);
    } break;
   case 20:
    {
    modifica_unsigned((unsigned int*)&set.pressione_accensione,5,set.pressione_spegnimento,0);
    presenta_unsigned(set.pressione_accensione,1,1,9,3);
    } break;
   case 21:
    {
    modifica_unsigned((unsigned int*)&set.ritardo_funzionamento_dopo_emergenza,1,99,0);
    presenta_unsigned(set.ritardo_funzionamento_dopo_emergenza,0,1,10,2);
    } break;
   case 22:
    {
    modifica_unsigned((unsigned int*)&set.potenza_minima_mandata_chiusa,10,100,0);
    presenta_unsigned(set.potenza_minima_mandata_chiusa,0,1,10,3);
    } break;
   case 23:
    {
    modifica_unsigned((unsigned int*)&set.ritardo_stop_mandata_chiusa,5,120,0);
    presenta_unsigned(set.ritardo_stop_mandata_chiusa,0,1,11,3);
    } break;
   case 24:
    {
    modifica_unsigned((unsigned int*)&set.ritardo_riaccensione_mandata_chiusa,3,999,0);
    presenta_unsigned(set.ritardo_riaccensione_mandata_chiusa,0,1,11,3);
    } break;
   case 25:
    {
    modifica_unsigned((unsigned int*)&set.potenza_minima_funz_secco,10,100,0);
    presenta_unsigned(set.potenza_minima_funz_secco,0,1,10,3);
    } break;
   case 26:
    {
    modifica_unsigned((unsigned int*)&set.ritardo_stop_funzionemento_a_secco,5,120,0);
    presenta_unsigned(set.ritardo_stop_funzionemento_a_secco,0,1,11,3);
    } break;
   case 27:
    {
    modifica_unsigned((unsigned int*)&set.portata_sensore_pressione,40,500,0);
    presenta_unsigned(set.portata_sensore_pressione,1,1,10,3);
    } break;
   case 28:
    {
    modifica_unsigned((unsigned int*)&set.modo_start_stop,0,1,0);
    if(set.modo_start_stop)
     {
     presenta_scritta((char*)&presenta_tipo_start_stop,1,(char)set.lingua,2,1,0,16);
     set.abilita_sensore_pressione=1;
     }
    else presenta_scritta((char*)&presenta_tipo_start_stop,0,(char)set.lingua,2,1,0,16);
    } break;
   case 29:
    {
    modifica_unsigned((unsigned int*)&set.scala_corrente_differenziale,1,250,0);
    presenta_unsigned(set.scala_corrente_differenziale,0,1,7,3);
    } break;
   case 30:
    {
    modifica_unsigned((unsigned int*)&set.costante_tau_salita_temperatura,10,180,0);
    presenta_unsigned(set.costante_tau_salita_temperatura,0,1,10,3);
    } break;
   case 31:
    {
    modifica_unsigned((unsigned int*)&set.scala_temperatura_motore,2,250,0);
    presenta_unsigned(set.scala_temperatura_motore,0,1,8,3);
    } break;
   case 32:
    {
    modifica_unsigned((unsigned int*)&set.taratura_temperatura_ambiente,5,40,0);
    if(salita_piu | salita_meno) set.temperatura_ambiente=DAC.temperatura_letta;
    presenta_unsigned(set.taratura_temperatura_ambiente,0,1,10,3);
    } break;
   case 33:
    {
    modifica_unsigned((unsigned int*)&set.limite_intervento_temper_motore,70,150,0);
    presenta_unsigned(set.limite_intervento_temper_motore,0,1,10,3);
    } break;
   case 34:
    {
    if(pronto_alla_risposta)
     {
     pronto_alla_risposta=0;
     if(buffer_USB[0]<29)
      {  
      asm
       {
       LDHX buffer_USB:1
       STHX secondi
       LDHX buffer_USB:3
       STHX secondi:2
       }
      presenta_scritta((unsigned char *)&lettura_allarmi,0,(char)set.lingua,25,0,0,16);
      Nallarme_ora_minuto_secondo(secondi,buffer_USB[0]);
      messaggio_allarme(buffer_USB[0]);
      }
     }
    } break;
   }
  }
 } 
}

void messaggio_allarme(char indentificazione)
{
switch(indentificazione)
 {
 case 0:
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,1,(char)set.lingua,25,1,0,16);
  } break; 
 case 1:
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,2,(char)set.lingua,25,1,0,16);
  } break; 
 case 10://I2xT
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,3,(char)set.lingua,25,1,0,16);
  } break;
 case 11://sovratensione
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,4,(char)set.lingua,25,1,0,16);
  } break;
 case 12://sottotensione
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,5,(char)set.lingua,25,1,0,16);
  } break;
 case 13://mandata chiusa
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,6,(char)set.lingua,25,1,0,16);
  } break;
 case 14://funzionamento a secco
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,7,(char)set.lingua,25,1,0,16);
  } break;
 case 15://sovratemperatura motore
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,8,(char)set.lingua,25,1,0,16);
  } break;
 case 16://protezione differenziale
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,9,(char)set.lingua,25,1,0,16);
  } break;
 case 17://squilibrio correnti
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,10,(char)set.lingua,25,1,0,16);
  } break;
 case 18://dissimmetria tensioni
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,11,(char)set.lingua,25,1,0,16);
  } break;
 case 19://Pressione emergenza  
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,12,(char)set.lingua,25,1,0,16);
  } break;
 case 20://I2xT
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,13,(char)set.lingua,25,1,0,16);
  } break;
 case 21://sovratensione
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,14,(char)set.lingua,25,1,0,16);
  } break;
 case 22://sottotensione
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,15,(char)set.lingua,25,1,0,16);
  } break;
 case 23://mandata chiusa
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,16,(char)set.lingua,25,1,0,16);
  } break;
 case 24://funzionamento a secco
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,17,(char)set.lingua,25,1,0,16);
  } break;
 case 25://sovratemperatura motore
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,18,(char)set.lingua,25,1,0,16);
  } break;
 case 26://protezione differenziale
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,19,(char)set.lingua,25,1,0,16);
  } break;
 case 27://squilibrio correnti
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,20,(char)set.lingua,25,1,0,16);
  } break;
 case 28://dissimmetria tensioni
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,21,(char)set.lingua,25,1,0,16);
  } break;
 case 29://sensore pressione      
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,22,(char)set.lingua,25,1,0,16);
  } break;
 case 30://pressione emergenza    
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,23,(char)set.lingua,25,1,0,16);
  } break;
 case 31://motore non partito    
  {
  presenta_scritta((unsigned char *)&lettura_allarmi,24,(char)set.lingua,25,1,0,16);
  } break;
 }
} 

void misure_medie(void)
{
int primo, secondo, terzo, tensione, corrente;
long prod;
asm
 {
 LDA set.monofase_trifase:1
 BNE versione_trifase
 LDHX V31_rms
 STHX tensione
 LDHX I1_rms
 STHX corrente

 CLR prod:2
 CLR prod:3
 LDHX media_potenzaI1xV13
 STHX prod
 BPL assegna_potenze_medie
 CLR prod
 CLR prod:1
 BRA assegna_potenze_medie

versione_trifase: 
 LDA V12_rms:1
 ADD V23_rms:1
 STA tensione:1
 LDA V12_rms
 ADC V23_rms
 STA tensione
 LDA tensione:1
 ADD V31_rms:1
 STA tensione:1
 LDA tensione
 ADC V31_rms
 STA tensione
 LDHX #3
 DIV
 STA tensione 
 LDA tensione:1
 DIV
 STA tensione:1
 
 LDA I1_rms:1
 ADD I2_rms:1
 STA corrente:1
 LDA I1_rms
 ADC I2_rms
 STA corrente
 LDA corrente:1
 ADD I3_rms:1
 STA corrente:1
 LDA corrente
 ADC I3_rms
 STA corrente
 LDHX #3
 LDA corrente
 DIV
 STA corrente
 LDA corrente:1
 DIV
 STA corrente:1

 LDA media_potenzaI1xV1:1
 ADD media_potenzaI2xV2:1
 STA prod:1
 LDA media_potenzaI1xV1
 ADC media_potenzaI2xV2
 STA prod
 LDA prod:1
 ADD media_potenzaI3xV3:1
 STA prod:1
 LDA prod
 ADC media_potenzaI3xV3
 STA prod
 BPL per_reattiva
 CLR prod
 CLR prod:1
 
per_reattiva: 
 LDA media_reattivaI1xV23:1
 ADD media_reattivaI2xV31:1
 STA prod:3
 LDA media_reattivaI1xV23
 ADC media_reattivaI2xV31
 STA prod:2
 LDA prod:3
 ADD media_reattivaI3xV12:1
 STA prod:3
 LDA prod:2
 ADC media_reattivaI3xV12
 STA prod:2
 
assegna_potenze_medie:
 LDHX prod
 SEI
 STHX potenza_media
 LDHX prod:2
 STHX reattiva_media
 LDHX tensione
 STHX tensione_media
 LDHX corrente
 STHX corrente_media
 CLI
  
//-----dissimetria---------
 LDHX V12_rms
 CPHX V23_rms
 BCS V2_maggiore_V1
 STHX secondo
 LDHX V23_rms
 STHX terzo
 BRA vedi_secondo_V3
V2_maggiore_V1:
 STHX terzo
 LDHX V23_rms
 STHX secondo

vedi_secondo_V3:
 LDHX secondo
 CPHX V31_rms
 BCS V3_maggiore
 STHX primo
 LDHX V31_rms
 STHX secondo
 BRA spareggio
V3_maggiore:
 LDHX V31_rms
 STHX primo

spareggio:
 LDHX secondo
 CPHX terzo
 BCC per_differenza
 STHX terzo

per_differenza:
 LDA primo:1
 SUB terzo:1
 STA dissimmetria:1
 LDA primo
 SBC terzo
 STA dissimmetria

//------squilibrio----------
 LDHX I1_rms
 CPHX I2_rms
 BCS I2_maggiore_I1
 STHX secondo
 LDHX I2_rms
 STHX terzo
 BRA vedi_secondo_I3
I2_maggiore_I1:
 STHX terzo
 LDHX I2_rms
 STHX secondo

vedi_secondo_I3:
 LDHX secondo
 CPHX I3_rms
 BCS I3_maggiore
 STHX primo
 LDHX I3_rms
 STHX secondo
 BRA Spareggio
I3_maggiore:
 LDHX I3_rms
 STHX primo

Spareggio:
 LDHX secondo
 CPHX terzo
 BCC Per_differenza
 STHX terzo

Per_differenza:
 LDA primo:1
 SUB terzo:1
 STA squilibrio:1
 LDA primo
 SBC terzo
 STA squilibrio
 }
}

void Nallarme_ora_minuto_secondo(long secondi, char numero)
{
int ore;
char min, sec;
asm
 {
 LDHX #60
 LDA secondi
 DIV
 STA secondi
 LDA secondi:1
 DIV
 STA secondi:1
 LDA secondi:2
 DIV
 STA secondi:2
 LDA secondi:3
 DIV
 STA secondi:3
 PSHH
 PULA
 STA sec
 LDHX #60
 LDA secondi
 DIV
 STA secondi
 LDA secondi:1
 DIV
 STA secondi:1
 LDA secondi:2
 DIV
 STA secondi:2
 LDA secondi:3
 DIV
 STA secondi:3
 PSHH
 PULA
 STA min
 LDHX secondi:2
 STHX ore
 
 LDHX #10
 LDA sec
 DIV
 ORA #$30
 STA display:14
 PSHH
 PULA
 ORA #$30
 STA display:15
 
 LDHX #10
 LDA min
 DIV
 ORA #$30
 STA display:11
 PSHH
 PULA
 ORA #$30
 STA display:12
 
 LDHX #10
 LDA ore
 DIV
 STA ore
 LDA ore:1
 DIV
 STA ore:1
 PSHH
 PULA
 ORA #$30
 STA display:9
 
 LDHX #10
 LDA ore
 DIV
 STA ore
 LDA ore:1
 DIV
 STA ore:1
 PSHH
 PULA
 ORA #$30
 STA display:8
 
 LDHX #10
 LDA ore
 DIV
 STA ore
 LDA ore:1
 DIV
 STA ore:1
 PSHH
 PULA
 ORA #$30
 STA display:7
 
 LDHX #10
 LDA ore
 DIV
 STA ore
 LDA ore:1
 DIV
 ORA #$30
 STA display:5
 PSHH
 PULA
 ORA #$30
 STA display:6
 
 LDHX #10
 LDA numero
 DIV
 ORA #$30
 STA display:2
 PSHH
 PULA
 ORA #$30
 STA display:3
 }
}

void presenta_stato_motore(void)
{
int pressione, temperatura;
if(toggle_func==0)//presenta le letture
 {
 if(set.motore_on)
  {
  if(timer_aggiorna_misure==0)
   {
   timer_aggiorna_misure=250;
   asm
    {
    LDHX media_pressione
    STHX pressione
    LDHX media_temperatura
    STHX temperatura
    }
   if(set.monofase_trifase==0) //con alimentazione monofase
    {
    if(alternanza_presentazione==0x21)
     {
     alternanza_presentazione=0x20;
     if(remoto==0)//abilitazione_OFF
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,0,(char)set.lingua,25,0,0,16);
      Nallarme_ora_minuto_secondo(conta_secondi,0);
      presenta_scritta((unsigned char *)&abilitazione_OFF,0,(char)set.lingua,1,1,0,16);
      } 
     else if(segnalazione_)
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,0,(char)set.lingua,25,0,0,16);
      Nallarme_ora_minuto_secondo(conta_secondi,segnalazione_);
      messaggio_allarme(segnalazione_);
      } 
     }
    else if(alternanza_presentazione==0x81)
     {
     alternanza_presentazione=0x80;
     presenta_menu((unsigned char *)&dati_presentati_in_ON,0,0,0);
     }
    else if(alternanza_presentazione==0x41)
     {
     alternanza_presentazione=0x40;
     presenta_menu((unsigned char *)&dati_presentati_in_ON,0,0,0);
     }

    if(alternanza_presentazione==0x80)
     {
     presenta_unsigned(tensione_media,0,0,0,3);
     if(potenza_media<0) potenza_media=0;
     presenta_unsigned(potenza_media,0,0,5,5);
     if(set.abilita_sensore_pressione) presenta_signed(pressione,1,0,11,3); else display[15]=' ';//elimina 'B'
     presenta_unsigned(corrente_media,1,1,0,3);
     presenta_unsigned(cosfi[3],2,1,5,3);
     if(temperatura>0) presenta_unsigned(temperatura,0,1,11,3); else display[31]=display[32]=' ';//elimina '�C'
     }
    else if(alternanza_presentazione==0x40)
     {
     presenta_unsigned(tensione_media,0,0,0,3);
     if(potenza_media<0) potenza_media=0;
     presenta_unsigned(potenza_media,0,0,5,5);
     if(set.abilita_sensore_pressione) presenta_signed(pressione,1,0,11,3); else display[15]=' ';//elimina 'B'
     presenta_unsigned(corrente_media,1,1,0,3);
     presenta_unsigned(cosfi[3],2,1,5,3);
     if(temperatura>0) presenta_unsigned(temperatura,0,1,11,3); else display[31]=display[32]=' ';//elimina '�C'
     } 
    } 
   else //alimentazione trifase
    {
    if(alternanza_presentazione==0x21)
     {
     alternanza_presentazione=0x20;
     if(remoto==0)//abilitazione_OFF
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,0,(char)set.lingua,25,0,0,16);
      Nallarme_ora_minuto_secondo(conta_secondi,0);
      presenta_scritta((unsigned char *)&abilitazione_OFF,0,(char)set.lingua,1,1,0,16);
      }
     else if(segnalazione_)
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,0,(char)set.lingua,25,0,0,16);
      Nallarme_ora_minuto_secondo(conta_secondi,segnalazione_);
      messaggio_allarme(segnalazione_);
      }
     }
    else if(alternanza_presentazione==0x81)
     {
     alternanza_presentazione=0x80;
     presenta_menu((unsigned char *)&dati_presentati_in_ON,0,0,4);
     }
    else if(alternanza_presentazione==0x41)
     {
     alternanza_presentazione=0x40;
     presenta_menu((unsigned char *)&dati_presentati_in_ON,0,0,6);
     }

    if(alternanza_presentazione==0x80)
     {
     presenta_unsigned(tensione_media,0,0,0,3);
     if(potenza_media<0) potenza_media=0;
     presenta_unsigned(potenza_media,0,0,5,5);
     if(set.abilita_sensore_pressione) presenta_signed(pressione,1,0,11,3); else display[15]=' ';//elimina 'B'
     presenta_unsigned(I1_rms,1,1,0,3);//prova //I1_rms
     presenta_unsigned(I2_rms,1,1,5,3);
     presenta_unsigned(I3_rms,1,1,11,3);
     }
    else if(alternanza_presentazione==0x40)
     {
     if(temperatura>0) presenta_unsigned(temperatura,0,0,0,3);
     else //presenta tensione
      {
      display[3]='V';
      display[4]=',';
      presenta_unsigned(tensione_media,0,0,0,3);
      } 
     if(potenza_media<0) potenza_media=0;
     presenta_unsigned(potenza_media,0,0,5,5);
     if(set.abilita_sensore_pressione) presenta_signed(pressione,1,0,11,3); else display[15]=' ';//elimina 'B'
     presenta_unsigned(cosfi[0],2,1,0,3);
     presenta_unsigned(cosfi[1],2,1,5,3);
     presenta_unsigned(cosfi[2],2,1,10,3);
     display[21]=display[26]=display[31]='P';
     display[22]=display[27]=display[32]='F';
     } 
    }
   } 
  }
 else if(eeprom_impegnata) presenta_menu((unsigned char*)&in_salvataggio,0,2,0);
 else presenta_menu((unsigned char*)&comandi_da_effettuare,(char)set.lingua,2,0);
 }
}

void marcia_arresto(void)
{
int I2xT, pressione, temperatura;
asm
 {
 LDHX media_pressione
 STHX pressione
 LDHX media_temperatura
 STHX temperatura
 LDHX delta_T
 STHX I2xT
 } 

if((toggle_func==0)||(cursore_menu==37)||(cursore_menu==38)||(cursore_menu==39)) //partenza possibile in taratura
 {
 if(salita_start)
  {
  salita_start=0;
  timer_commuta_presentazione=0;
  if((relais_alimentazione==0)&&(remoto==filtro_pulsanti))
   {
   set.motore_on=1;
   segnalazione_=1;
   timer_rele_avviamento=100;
   relais_alimentazione=1;
   prima_segnalazione=0;
   timer_eccitazione_relais=tempo_eccitazione_relais;
   picco_corrente_avviamento=0;
   corrente_test=200;//20A
   timer_allarme_avviamento=durata_cc;
   timer_avviamento_monofase=durata_avviamento;//3 s tempo massimo di avviamento di un motore monofase
   timer_mandata_chiusa=(unsigned char)set.ritardo_stop_mandata_chiusa;
   timer_attesa_secco=(unsigned char)set.ritardo_stop_funzionemento_a_secco;
   timer_attesa_squilibrio=(unsigned char)set.ritardo_protezione_squilibrio;
   timer_attesa_tensione=(unsigned char)set.timeout_protezione_tensione;
   timer_attesa_differenziale=timer_ritorno_da_emergenza;
   tentativi_avviamento=N_tentativi_motore_bloccato;
   }
  }
 }

if(toggle_func==0)
 {
 if(salita_stop)
  {
  salita_stop=0;
  toggle_stop=0;
  segnalazione_=0;
  relais_alimentazione=0;
  set.motore_on=0;
  timer_riavviamento=2;//attesa minima per riavviamento
  presenta_menu((unsigned char*)&comandi_da_effettuare,(char)set.lingua,2,0);
  tentativi_avviamento=N_tentativi_motore_bloccato;
  }
 if(set.motore_on)//condizioni per riavviamento
  {
  if(relais_alimentazione==0)
   {
   if(timer_riavviamento==0)
    {
    if((Id_rms<set.limite_corrente_differenziale)
    &&(temperatura<set.limite_intervento_temper_motore-10)
    &&(I2xT<delta_T_riavviamento) //temperatura calcolata < 60�C
    &&(tensione_media<sovratensione_consentita)
    &&(tensione_media>ripresa_per_tensione_consentita)
    &&(tentativi_avviamento)
    &&(remoto==filtro_pulsanti))
     {
     if((set.abilita_sensore_pressione==0)||
     ((set.abilita_sensore_pressione)&&(pressione<set.pressione_accensione)&&(pressione>emergenza_sensore_pressione)))
      {
      if((set.monofase_trifase==0)||                               
      ((set.monofase_trifase)&&(dissimmetria<dissimmetria_tollerata)))
       {
       prima_segnalazione=0;
       segnalazione_=1;
       timer_rele_avviamento=100;
       relais_alimentazione=1;
       timer_eccitazione_relais=tempo_eccitazione_relais;
       picco_corrente_avviamento=0;
       corrente_test=200;//20A
       timer_allarme_avviamento=durata_cc;
       timer_avviamento_monofase=durata_avviamento;//3 s tempo massimo di avviamento di un motore monofase
       timer_mandata_chiusa=(unsigned char)set.ritardo_stop_mandata_chiusa;
       timer_attesa_secco=(unsigned char)set.ritardo_stop_funzionemento_a_secco;
       timer_attesa_squilibrio=(unsigned char)set.ritardo_protezione_squilibrio;
       timer_attesa_tensione=(unsigned char)set.timeout_protezione_tensione;
       timer_attesa_differenziale=timer_ritorno_da_emergenza;
       }
      }
     }
    }
   }
  }
 if(relais_alimentazione)
  {
  if((remoto==0)||((set.modo_start_stop)&&(set.abilita_sensore_pressione)&&(pressione>set.pressione_spegnimento)))
   {
   relais_alimentazione=0;
   timer_riavviamento=2;//attesa minima per riavviamento
   }
  } 
 } 
}

void protezione_termica(void)//calcolo della sovra_temperatura motore con integrale I^2*T
{
char segno, prod[6];
asm
 {
;//---- incremento di temperatura ad ogni secondo ----
;//incremento = (corrente_media^2 * delta_Tn / corrente_nominale^2 - delta_T)/costante_tau_salita_temperatura
;//incremento = (corrente_media^2 * fattore_I2xT - delta_T)/costante_tau_salita_temperatura
 LDA corrente_media:1
 TAX
 MUL
 STA prod:2
 STX prod:1
 LDA corrente_media:1
 LDX corrente_media
 MUL
 LSLA
 ROLX
 ADD prod:1
 STA prod:1
 TXA
 ADC #0
 STA prod
 LDA corrente_media
 TAX
 MUL
 ADD prod
 STA prod
 
 LDA prod:2
 LDX fattore_I2xT:1 //2x1
 MUL
 STA prod:5
 STX prod:4
 LDA prod:2
 LDX fattore_I2xT:0 //2x0
 MUL
 ADD prod:4
 STA prod:4
 TXA
 ADC #0
 STA prod:3
 LDA prod:1
 LDX fattore_I2xT:1 //1x1
 MUL
 ADD prod:4
 STA prod:4
 TXA
 ADC prod:3
 STA prod:3
 CLR prod:2
 ROL prod:2
 LDA prod:1
 LDX fattore_I2xT:0 //1x0
 MUL
 ADD prod:3
 STA prod:3
 TXA
 ADC prod:2
 STA prod:2
 LDA prod:0
 LDX fattore_I2xT:1 //0x1
 MUL
 ADD prod:3
 STA prod:3
 TXA
 ADC prod:2
 STA prod:2
 CLR prod:1
 ROL prod:1
 LDA prod:0
 LDX fattore_I2xT:0 //0x0
 MUL
 ADD prod:2
 STA prod:2
 TXA
 ADC prod:1
 STA prod:1
 CLR prod

 CLR segno
 LDA prod:5
 SUB delta_T:3
 STA prod:5
 LDA prod:4
 SBC delta_T:2
 STA prod:4
 LDA prod:3
 SBC delta_T:1
 STA prod:3
 LDA prod:2
 SBC delta_T
 STA prod:2
 LDA prod:1
 SBC #0
 STA prod:1
 LDA prod:0
 SBC #0
 STA prod:0
 BCC per_rapporto_ct
 COM segno
 COM prod
 COM prod:1
 COM prod:2
 COM prod:3
 COM prod:4
 COM prod:5
 
per_rapporto_ct:
 LDHX set.costante_tau_salita_temperatura
 LDA prod
 DIV
 STA prod
 LDA prod:1
 DIV
 STA prod:1
 LDA prod:2
 DIV
 STA prod:2
 LDA prod:3
 DIV
 STA prod:3
 LDA prod:4
 DIV
 STA prod:4
 LDA prod:5
 DIV
 STA prod:5
  
 LDA segno
 BNE sottrai_
 LDA delta_T:3
 ADD prod:5
 STA delta_T:3
 LDA delta_T:2
 ADC prod:4
 STA delta_T:2
 LDA delta_T:1
 ADC prod:3
 STA delta_T:1
 LDA delta_T
 ADC prod:2
 STA delta_T
 BRA fine
sottrai_:
 LDA delta_T:3
 SUB prod:5
 STA delta_T:3
 LDA delta_T:2
 SBC prod:4
 STA delta_T:2
 LDA delta_T:1
 SBC prod:3
 STA delta_T:1
 LDA delta_T
 SBC prod:2
 STA delta_T
 BPL fine
 LDHX #0
 STHX delta_T
 STHX delta_T:2

fine:
 }
}

void disinserzione_condensatore_avviamento(void)
{
asm
 {
 LDA set.monofase_trifase:1 //solo monofase
 BNE spegni_relais_avviamento
 LDA relais_alimentazione
 BEQ spegni_relais_avviamento
 LDHX timer_avviamento_monofase
 BEQ spegni_relais_avviamento
 LDA timer_avviamento_monofase:1
 SUB #1
 STA timer_avviamento_monofase:1
 LDA timer_avviamento_monofase
 SBC #0
 STA timer_avviamento_monofase
 LDA #1
 STA relais_avviamento
 
 LDHX timer_avviamento_monofase
 CPHX inizio_lettura_I_avviamento
 BCC fine
 LDHX corrente_media
 CPHX #20
 BCS fine
 CPHX picco_corrente_avviamento
 BCS per_test_partenza_avvenuta
 STHX picco_corrente_avviamento
 LDA picco_corrente_avviamento:1
 LDX #179//70%
 MUL
 STX corrente_test:1
 LDA picco_corrente_avviamento
 LDX #179//70%
 MUL
 ADD corrente_test:1
 STA corrente_test:1
 TXA
 ADC #0
 STA corrente_test
 BRA fine
  
per_test_partenza_avvenuta:
 CPHX corrente_test
 BCC fine
 LDHX #0
 STHX timer_avviamento_monofase
 
spegni_relais_avviamento:
 CLRA
 STA relais_avviamento
fine: 
 }
}

void condizioni_di_allarme(void)
{
int I2xT, temperatura, temperatura_segnalazione, pressione;
asm
 {
 LDHX media_pressione
 STHX pressione
 LDHX media_temperatura
 STHX temperatura
 LDHX delta_T
 STHX I2xT
 
 LDA set.limite_intervento_temper_motore:1
 SUB #5
 STA temperatura_segnalazione:1
 LDA set.limite_intervento_temper_motore
 SBC #0
 STA temperatura_segnalazione
 }
if(relais_alimentazione) //alimentazione presente
 {
 //segnalazioni ed arresto
 if((set.abilita_sensore_pressione)&&(pressione<emergenza_sensore_pressione)) //allarme sensore
  {
  relais_alimentazione=0;
  segnalazione_=29;
  timer_riavviamento=timer_ritorno_da_emergenza;
  } 
 else if(I2xT>sovraccarico)//allarme protezione termica
  {
  if(tentativi_avviamento) tentativi_avviamento--;
  relais_alimentazione=0;
  segnalazione_=20;
  timer_riavviamento=timer_ritorno_da_emergenza;
  } 
 else if(temperatura>set.limite_intervento_temper_motore) //temperatura misurata
  {
  relais_alimentazione=0;
  segnalazione_=25;
  timer_riavviamento=timer_ritorno_da_emergenza;
  } 
 else if((set.abilita_sensore_pressione)&&(pressione>set.pressione_emergenza))//pressione emergenza
  {
  relais_alimentazione=0;
  segnalazione_=30;
  timer_riavviamento=timer_ritorno_da_emergenza;
  } 
 else if(Id_rms>set.limite_corrente_differenziale)//protezione per correnti disperse
  {
  if(timer_attesa_differenziale==0)
   {
   relais_alimentazione=0;
   segnalazione_=26;
   timer_riavviamento=timer_ritorno_da_emergenza;
   }
  } 
 else
  {
  timer_attesa_differenziale=timer_ritorno_da_emergenza;
  if((I1_rms>limitazione_I_avviamento)||(I2_rms>limitazione_I_avviamento)||(I3_rms>limitazione_I_avviamento)) //protezione per avviamento mancato
   {
   if(timer_allarme_avviamento==0)
    {
    if(tentativi_avviamento) tentativi_avviamento--;
    relais_alimentazione=0;
    segnalazione_=31;
    timer_riavviamento=timer_ritorno_da_emergenza;
    }
   }
  else //emergenza tensione
   {
   timer_allarme_avviamento=durata_cc;
   if(tensione_media<sottotensione_consentita) //tensione insufficiente
    {
    if(timer_attesa_tensione==0)
     {
     relais_alimentazione=0;
     segnalazione_=22;
     timer_riavviamento=timer_ritorno_da_emergenza;
     }
    } 
   else if(tensione_media>sovratensione_consentita) //tensione eccessiva
    {
    if(timer_attesa_tensione==0)
     {
     relais_alimentazione=0;
     segnalazione_=21;
     timer_riavviamento=timer_ritorno_da_emergenza;
     }
    }
   else if((dissimmetria>dissimmetria_emergenza)&&(set.monofase_trifase)) //tensione dissimmetrica
    {
    if(timer_attesa_tensione==0)
     {
     relais_alimentazione=0;
     segnalazione_=28;
     timer_riavviamento=timer_ritorno_da_emergenza;
     }
    } 
   else
    {
    timer_attesa_tensione=(unsigned char)set.timeout_protezione_tensione;
    if((squilibrio>squilibrio_emergenza)&&(set.monofase_trifase)) //coorente squilibrata
     {
     if(timer_attesa_squilibrio==0)
      {
      relais_alimentazione=0;
      segnalazione_=27;
      timer_riavviamento=timer_ritorno_da_emergenza;
      }
     } 
    else
     {
     timer_attesa_squilibrio=(unsigned char)set.ritardo_protezione_squilibrio;
     if((set.abilita_sensore_pressione)
     &&(set.modo_start_stop==0)
     &&(potenza_media<potenza_mandata_chiusa)&&(pressione>set.pressione_spegnimento))//mandata chiusa
      {
      if(timer_mandata_chiusa==0)
       {
       relais_alimentazione=0;
       segnalazione_=23;
       timer_riavviamento=set.ritardo_riaccensione_mandata_chiusa;
       }
      } 
     else
      {
      timer_mandata_chiusa=(unsigned char)set.ritardo_stop_mandata_chiusa;
      if(potenza_media<potenza_a_secco) //funzionamento a secco
       {
       if(timer_attesa_secco==0)
        {
        relais_alimentazione=0;
        segnalazione_=24;
        timer_riavviamento=timer_ritorno_da_emergenza;
        }
       } 
      else timer_attesa_secco=(unsigned char)set.ritardo_stop_funzionemento_a_secco;
      } 
     }
    } 
   } 
  }

 //segnalazioni senza arresto
 if(segnalazione_<20)
  {
  if(segnalazione_>1) segnalazione_=1;
  if((set.abilita_sensore_pressione)&&(pressione>set.pressione_emergenza-5)) segnalazione_=19;//segnalazione pressione_emergenza
  else
   {
   if(temperatura>temperatura_segnalazione) segnalazione_=15;//segnalazione con 5�C in anticipo
   else
    {
    if(I2xT>sovraccarico_moderato) segnalazione_=10;
    else
     {
     if((dissimmetria>dissimmetria_tollerata)&&(set.monofase_trifase)) segnalazione_=18;
     else
      { 
      if((squilibrio>squilibrio_tollerato)&&(set.monofase_trifase)) segnalazione_=17;
      else
       {
       if(tensione_media>sovratensione_consentita) segnalazione_=11;
       else
        {
        if(tensione_media<sottotensione_consentita) segnalazione_=12;
        else
         {
         if((set.abilita_sensore_pressione)
         &&(set.modo_start_stop==0)
         &&(potenza_media<potenza_mandata_chiusa)&&(pressione>set.pressione_spegnimento)) segnalazione_=13;
         else
          {
          if(segnalazione_==13) segnalazione_=1;
          if(potenza_media<potenza_a_secco) segnalazione_=14; else segnalazione_=1;
          }
         } 
        } 
       } 
      } 
     } 
    } 
   } 
  if((segnalazione_>1)&&(prima_segnalazione==0))
   {
   prima_segnalazione=1;
   timer_attesa_segnalazione_fault=2500;
   timer_lampeggio_LED_emergenza=3000;
   }
  } 
 }
}

void condizioni_iniziali(void)
{
asm
 {
 SEI
 }
 
MCGC1=0x00;
MCGC2=0x37;
MCGC3=0x43;

//----portA--------
PTADD=0; //pulsanti ed abilitazione
PTAD=0;

//-----ingressi AD------
PTBDD=0;
PTBD=0;
APCTL1=0xff;//solo ingreei analogici
APCTL2=0;
ADCSC1=0;
ADCSC2=0;
ADCCFG=0x05;
ADCCVL=0xff;
contatore_AD=0;
ADCSC1=0x40;

//-----portC non usato---------
PTCDD=0;
PTCD=0;

//-----portD bus display-----------
PTDDD=0xff;//solo uscita
PTDD=0;

//-----port E per SPI----------
PTEDD=0x80;//uscita abilitazione eeprom
PTED=0;
asm
 {
 LDA SPI1S
 LDA SPI1D
 }
SPI1C1=0xd0;//interruzione abilitata
SPI1C2=0x00;
SPI1BR=0x77;
 
//-------uscite LED e PWM--------------------
PTFDD=0;//uscite LED e rel�
PTFD=0;
TPM1C2SC=0x08;//uscite rel�
TPM1C3SC=0x08;
TPM1C4SC=0x08;//uscite LED
TPM1C5SC=0x08;
TPM1MOD=1200;//PWM a 10KHz
TPM1C2V=24;
TPM1C3V=24;
TPM1C4V=24;
TPM1C5V=24;
TPM2C0SC=0x08;
TPM2C1SC=0x08;
TPM2MOD=12000;//PWM a 1KHz
TPM2C0V=24;//LED rosso
TPM2C1V=24;//2us solo segnale 

TPM1CNT=0;
TPM2CNT=0;
TPM1SC=0x68;
TPM2SC=0x68;

//-----portG  per display  G2 = RSneg, G3 = E----
PTGDD=0x0c;
PTGD=0;

//--------inizializzazione delle variabili-------------
precedente_lettura_allarme=allarme_in_lettura=10000;
pronto_alla_risposta=0;
prima_segnalazione=0;
timer_20ms=0;
timer_1s=0;
timer_inc_dec=0;
timer_piu_meno=0;
timer_lampeggio=0;
timer_lampeggio_LED_emergenza=0;
timer_attesa_segnalazione_fault=0;
timer_commuta_presentazione=0;
timer_attesa_squilibrio=100;
timer_attesa_tensione=100;
timer_attesa_differenziale=5000;
timer_mandata_chiusa=100;
timer_attesa_secco=100;
timer_1_ora=3600;

PWM_relais=0;
attesa_invio=0;
timer_rele_avviamento=0;
relais_alimentazione=0;
relais_avviamento=0;
contatore_display=0;
cursore_menu=0;
numero_ingresso=0;
potenza_media=0;
reattiva_media=0;
tensione_media=0;
corrente_media=0;
picco_corrente_avviamento=0;
corrente_test=200;//20A
dissimmetria=0;
squilibrio=0;
comando_display=1;
timer_rinfresco_display=255;
alternanza_presentazione=0x81;

USB_comm_init();
asm
 { 
 CLI//abilita interrupt
 }
scrivi_eeprom=0;
leggi_eeprom=0;
timer_rilascio=0;

reset_default=0;
salvataggio_funzioni=0;
salvataggio_allarme=0;
salva_conta_secondi=0;
lunghezza_salvataggio=0;
leggi_impostazioni=1;
timer_reset_display=100;
timer_1min=60;
timer_aggiorna_misure=2000; 
while(timer_reset_display)
 {
 __RESET_WATCHDOG(); /* feeds the dog */
 }
while(leggi_impostazioni)
 {
 __RESET_WATCHDOG(); /* feeds the dog */
 lettura_impostazioni();
 }
calcolo_delle_costanti();
offset_I1letta=0x20000000; //valor medio ipotetico
offset_I3letta=0x20000000;
offset_V12letta=0x20000000;
offset_V13letta=0x20000000;
offset_Idletta=0x20000000;
Id_rms=0;
I1_rms=0;
I2_rms=0;
I3_rms=0;
V12_rms=0;
V23_rms=0;
V31_rms=0;
quad_Id=0;
somma_quad_Id=0;
somma_quad_I1=0;
somma_quad_I2=0;
somma_quad_I3=0;
somma_quad_V12=0;
somma_quad_V23=0;
somma_quad_V31=0;
somma_potenzaI1xV13=0;
somma_potenzaI1xV1=0;
somma_potenzaI2xV2=0;
somma_potenzaI3xV3=0;
somma_reattivaI1xV23=0;
somma_reattivaI2xV31=0;
somma_reattivaI3xV12=0;
Somma_potenzaI1xV13=0;
Somma_potenzaI1xV1=0;
Somma_potenzaI2xV2=0;
Somma_potenzaI3xV3=0;
Somma_reattivaI1xV23=0;
Somma_reattivaI2xV31=0;
Somma_reattivaI3xV12=0;
media_temperatura=0;
media_pressione=0;
quad_I1=0;
quad_I2=0;
quad_I3=0;
quad_V12=0;
quad_V23=0;
quad_V31=0;
potenzaI1xV1=0;
potenzaI2xV2=0;
potenzaI3xV3=0;
reattivaI1xV23=0;
reattivaI2xV31=0;
reattivaI3xV12=0;
media_quad_Id=0;
media_quad_I1=0;
media_quad_I2=0;
media_quad_I3=0;
media_quad_V31=0;
media_quad_V12=0;
media_quad_V23=0;
media_quad_V31=0;
media_potenzaI1xV13=0;
media_potenzaI1xV1=0;
media_potenzaI2xV2=0;
media_potenzaI3xV3=0;
media_reattivaI1xV23=0;
media_reattivaI2xV31=0;
media_reattivaI3xV12=0;
delta_T=0;
remoto=filtro_pulsanti;
salita_remoto=1;
asm  //inizializza il conta_secondi
 {
 LDHX set.conta_ore:2
 STHX conta_secondi:2
 LDHX set.conta_ore
 STHX conta_secondi
 LDHX set.conta_ore_funzionamento:2
 STHX conta_secondi_attivita:2
 LDHX set.conta_ore_funzionamento
 STHX conta_secondi_attivita
 }
sequenza_fasi=0;
while(timer_aggiorna_misure)
 {
 __RESET_WATCHDOG(); /* feeds the dog */
 }
timer_rilascio=0;
start=0;
salita_start=0;
stop=0;
salita_stop=0;
toggle_stop=0;
func=0;
salita_func=0;
toggle_func=0;
meno=0;
salita_meno=0;
piu=0;
salita_piu=0; 
timer_riavviamento=0;
timer_avviamento_monofase=durata_avviamento;
timer_allarme_avviamento=durata_cc;
timer_eccitazione_relais=tempo_eccitazione_relais;
precedente_segnalazione=segnalazione_=(unsigned char)set.motore_on;

indirizzo_scrivi_eeprom=ultimo_indirizzo_scrittura=ultimo_indirizzo_funzioni;

timer_attesa_squilibrio=(unsigned char)set.ritardo_protezione_squilibrio;
timer_attesa_tensione=(unsigned char)set.timeout_protezione_tensione;
timer_attesa_differenziale=timer_ritorno_da_emergenza;
timer_mandata_chiusa=(unsigned char)set.ritardo_stop_mandata_chiusa;
timer_attesa_secco=(unsigned char)set.ritardo_stop_funzionemento_a_secco;
tentativi_avviamento=N_tentativi_motore_bloccato;
}

/***********************************************************************/

void main(void)
{
condizioni_iniziali();
for(;;)  /* loop forever */
 {
 __RESET_WATCHDOG(); /* feeds the dog */
 marcia_arresto();

 salva_reset_default();//precedenza 1
 salva_impostazioni();//precedenza 2
 salva_allarme_in_eepprom();//precedenza 3
 salva_conta_secondi_attivita();//precedenza 4

 lettura_impostazioni();//precedenza 5
 lettura_allarme_messo_in_buffer_USB();//precedenza 6

 trasmissione_misure_istantanee(); //con allarme_in_lettura = 1000
 trasmissione_tarature(); //con allarme_in_lettura == 2000

 programmazione();
 misure_medie();
 presenta_stato_motore();
 if(toggle_func==0) condizioni_di_allarme();//non in taratura

 USB_comm_process();
 cdc_process();
 }
}