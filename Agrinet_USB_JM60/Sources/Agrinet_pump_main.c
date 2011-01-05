//timer e contatore ore di funzionamento (salvataggio dell'ora ogni 10 minuti)
//Lettura di correnti, tensioni, sensore_pressione, temperatura_motore, relè differenziale, tensione_logica
//lettura memoria EEPROM tramite SPI
//limitazione della pressione

//intervento per:
//   corrente differenziale sopra i 300mA tempo 20ms
//   elevata temperatura motore
//   sovracorrente (I^2*t)
//   squilibrio delle correnti %
//   sovratensione di valore e tempo stabiliti
//   sottotensione di valore e tempo stabiliti
//   dissimmetria di valore e tempo stabiliti
//   mandata chiusa: pressione normale e potenza minima
//   funzionamento a secco: pressione bassa e potenza minima
//   pressione troppo alta
//   sensore_pressione  rotto

//24 bytes trasmessi per un complesso di 8K 335 registrazioni
// 1 byte = tipo intervento  all'indirizzo binario xxxx xxxx xxx0 0000 o xxxx xxxx xxx1 1000
// 4 bytes = ora_minuto_secondo  xxxxx.xx.xx  (viene registrata come numero totale di secondi di funzionamento)
// 2 bytes =tensione1
// 2 bytes =tensione2
// 2 bytes =tensione3
// 2 bytes = corrente1 
// 2 bytes = corrente2 
// 2 bytes = corrente3 
// 2 bytes = potenza  
// 2 bytes = pressione
// 1 byte  = cosfi convenzionale =P/(P^2+Q^2)
// 2 bytes = temperatura

//tipo intervento:
// 1 ON 
// 2 OFF senza intervento protezioni     

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
//ogni valore diverso non viene considerato
 
//funzioni:
//col pulsante FUNC si leggono le funzioni in successione crescente
//coi pulsanti + e - si cambia il valore
// i comandi ON OFF vengono memorizzati e ripresi al ritorno della tensione

//funzioni protette da chiave:
// 1) codice macchina registrato in indirizzo 8040
// 2) funzione di reset con conferma : azzera conta_ore, cancella gli interventi, inserisce parametri default
//funzioni non protette
// 1) monofase o trifase 
// 2) tensione nominale V
// 3) limite percentuale della sovratensione
// 4) limite percentuale della sottotensione
// 5) limite percentuale di dissimmetria delle tensioni
// 6) tempo limite per intervento protezione tensione s
// 7) corrente nominale A
// 8) limite percentuale della sovracorrente
// 9) limite percentuale segnalazione squilibrio delle correnti
// 10) limite percentuale intervento squilibrio delle correnti
// 11) costante di tempo (s) della salita di temperatura
// 12) sensore pressione (4-20mA) portata Bar
// 13) sensore temperatura: temperatura attuale
// 14) sensore temperatura: mV/°C
// 15) soglia intervento temperatura: °C
// 16) comando motore: remoto, o pressione
// 17) pressione accensione Bar*10 (bassa)
// 18) pressione arresto Bar*10 (alta)
// 19) pressione intervento emergenza Bar (alta)
// 20) potenza minima mandata chiusa (con pressione maggiore della pressione di accensione)
// 21) potenza minima funzionamento a secco (con pressione minore della pressione di accensione)
// 22) soglia di emergenza della corrente differenziale (mA)
// 23) ripresa funzionamento dopo intervento (s)

// presentazioni con motore in ON:
// tensione, potenza, pressione, corrente1, corrente2, corrente3,
// alternata con:
// temperatura, potenza, pressione, cosfi1, cosfi2, cosfi3,

__interrupt void AD_conversion_completeISR(void);
__interrupt void timer1_overflowISR(void); //timer 100 us per relais
__interrupt void timer2_overflowISR(void); //1 ms
__interrupt void SPI1(void);
//__interrupt void USB_status(void);

void calcolo_tensione_relais(void);
void lettura_pulsanti(void);
void comando_del_display(void);
long media_quadrati_nel_periodo(unsigned long dividendo, int divisore);
int media_potenza_nel_periodo(long dividendo, int divisore);
void condizioni_iniziali(void);
void somma_quadrati(unsigned long *somma, int var);
long somma_potenze(int x, int y);
void calcolo_offset_misure(unsigned long *offset, int misura);
void calcolo_valor_medio_potenze(long *media_potenza, int potenza); 
void calcolo_medie_quadrati(unsigned long *media_quad, unsigned long quad);//160ms
int valore_efficace(unsigned long quad, int costante);
void lettura_impostazioni(void);
void salva_conta_secondi_attivita(void);
void leggi_conta_secondi_attivita(void);
void salva_impostazioni(void);
void lettura_allarme(void);//con allarme_in_lettura <335
void scrittura_allarme_in_eeprom(void);
char calcolo_del_fattore_di_potenza(long attiva, long reattiva);
void presenta_menu(unsigned char *menu, char idioma, char lunghezza , char riga_inizio);
void modifica_unsigned(unsigned int *var, unsigned int min, unsigned int max);
void presenta_unsigned(unsigned int val, char punto, char riga, char colonna, char n_cifre);
void presenta_scritta(char *stringa, char offset, char idioma, char lunghezza, char riga,char colonna, char N_caratteri);
void funzione_reset_default(void);
void calcolo_delle_costanti(void);
void programmazione(void);
void misure_medie(void);
void presenta_stato_motore(void);
void protezione_termica(void);
void disinserzione_condensatore_avviamento(void);
void condizioni_di_allarme(void);
void marcia_arresto(void);
void trasmissione_misure_istantanee(void); //con allarme_in_lettura = 1000
void trasmissione_tarature(void);//con allarme_in_lettura == 2000

void main(void);

#include <hidef.h> /* for EnableInterrupts macro */
#include "derivative.h" /* include peripheral declarations */
#include "USB_man.h"


const unsigned char
presentazione_iniziale[2][17]={
"AGRINET PUMP    ",
"CONTROL V 01    "},

dati_presentati_in_ON[4][2][17]={
"   V     W     B",//monofase
"    A   PF    °C",
"   V     W     B",//monofase alternato
"    A   PF    °C",

"   V     W     B",//trifase 
"    A    A     A",
"   C     W     B",//trifase alternato
".  PF .  PF.  PF"},

lettura_allarmi[2][21][17]={
"N   ,     .  .  ",// numero allarme=3cifre, ora=5cifre minuto=2cifre secondo=2cifre
"      ON        ",
"      OFF       ",
"Sovracorrente ON",
"Sovratensione ON",
"Sottotensione ON",
"MandataChiusa ON",
"Funzion.Secco ON",
"SovraTemper.  ON",
"Prot.Differen ON",
"Squilibrio    ON",
"Dissimmetria  ON",
"SovracorrenteOFF",
"SovratensioneOFF",
"SottotensioneOFF",
"MandataChiusaOFF",
"Funzion.SeccoOFF",
"SovraTemper. OFF",
"Prot.DifferenOFF",
"Squilibrio   OFF",
"Dissimmetria OFF",

"N   ,     .  .  ",// numero allarme=3cifre, ora=5cifre minuto=2cifre secondo=2cifre
"      ON        ",
"      OFF       ",
"OverCurrent   ON",
"OverVoltage   ON",
"UnderVoltage  ON",
"MinimumFlow   ON",
"DryOperating  ON",
"OverTemperat. ON",
"Isolat.Fault  ON",
"CurrentDiff.  ON",
"VoltageDiff.  ON",
"OverCurrent  OFF",
"OverVoltage  OFF",
"UnderVoltage OFF",
"MinimumFlow  OFF",
"DryOperating OFF",
"OverTemperat.OFF",
"Isolat.Fault OFF",
"CurrentDiff. OFF",
"VoltageDiff. OFF"},

presenta_monofase_trifase[2][2][17]={
"  Monofase      ",
"  Trifase       ",

"Single Phase    ",
"Three Phase     "},

comando_reset[4][17]={
"  Restart?      ",
" No Restart?    ",
"Restart Begun   ",
"Restart Executed"},

comando_lingua[2][17]={
"Italic/Italiano ",
"English/Inglese "},

abilita_avanzate[2][2][17]={
"  abilitate     ",
"non abilitate   ",

"  enabled       ",
"not enabled     "},

menu_principale[2][64][17]={//Menu' principale: elenco delle funzioni di taratura
"Monofase/Trifase",
"                ",
"TensioneNominale",
"              V ",
"CorrenteNominale",
"              A ",
"LimiteSovraTens.",
"              % ",
"LimiteSottoTens.",
"              % ",
"SegnalazioneDis-",
"simetria:     % ",
"InterventoDissim",
"metria:       % ",
"TempoIntervento ",
"Err.Volt:      s",
"LimiteSovraCorr.",
"              % ",
"SegnalazioneSqui",
"librio:       % ",
"InterventoSquili",
"brio:         % ",
"TempoIntervento ",
"Err.Corr:      s",
"Limite Corrente ",
"Disp.:        mA",
"TempoIntervento ",
"Dispers.:      s",
"PressioneEmergen",
"za:          Bar",
"PressioneRiaccen",
"sione:       Bar",
"PressioneSpegni-",
"mento:       Bar",
"Ritardo Ripristi",
"no Em:         s",
"Minima Potenza  ",
"Chiusa:        W",
"RitardoStopMand.",
"Chiusa:        s",
"Minima Potenza  ",
"Secco:         W",
"RitardoStopFunz.",
"Secco:         s",
"RitardoRiaccens.",
"Secco:         s",
"FunzioniAvanzate",
"                ",
"Costante Tempo  ",//sotto chiave
"Rscaldam:      s",
"Portata Sensore ",
"Pressione      B",
"Scala Sensore   ",
"Temper:     mV/C",
"Temperatura     ",
"Attuale:      °C",
"LimiteIntervento",
"Temper.:      °C",
"Scala Sensore I ",
"diff:      mV/mA",
"Numero Serie:   ",
"                ",
"Dati Costruttore",
"                ",

"Monofase/Trifase",//inglese
"                ",
"TensioneNominale",
"              V ",
"CorrenteNominale",
"              A ",
"LimiteSovraTens.",
"              % ",
"LimiteSottoTens.",
"              % ",
"SegnalazioneDis-",
"simetria:     % ",
"InterventoDissim",
"metria:       % ",
"TempoIntervento ",
"Err.Volt:      s",
"LimiteSovraCorr.",
"              % ",
"SegnalazioneSqui",
"librio:       % ",
"InterventoSquili",
"brio:         % ",
"TempoIntervento ",
"Err.Corr:      s",
"Limite Corrente ",
"Disp.:        mA",
"TempoIntervento ",
"Dispers.:      s",
"PressioneEmergen",
"za:          Bar",
"PressioneRiaccen",
"sione:       Bar",
"PressioneSpegni-",
"mento:       Bar",
"Ritardo Ripristi",
"no Em:         s",
"Minima Potenza  ",
"Chiusa:        W",
"RitardoStopMand.",
"Chiusa:        s",
"Minima Potenza  ",
"Secco:         W",
"RitardoStopFunz.",
"Secco:         s",
"RitardoRiaccens.",
"Secco:         s",
"FunzioniAvanzate",
"                ",
"Costante Tempo  ",//sotto chiave
"Rscaldam:      s",
"Portata Sensore ",
"Pressione      B",
"Scala Sensore   ",
"Temper:     mV/C",
"Temperatura     ",
"Attuale:      °C",
"LimiteIntervento",
"Temper.:      °C",
"Scala Sensore I ",
"diff:      mV/mA",
"Numero Serie:   ",
"                ",
"Dati Costruttore",
"                "};

const char comando_AD[33]={
0x40,0x41,0x42,0x43,0x44,
0x40,0x41,0x42,0x43,0x44,
0x45,
0x40,0x41,0x42,0x43,0x44,
0x40,0x41,0x42,0x43,0x44,
0x46,
0x40,0x41,0x42,0x43,0x44,
0x40,0x41,0x42,0x43,0x44,
0x47},
prepara[7]={0x01,0x02,0x06,0x0c,0x14,0x3c,0x01};

const int
delta_Tn=80, //sovra_temperatura limite con corrente nominale = 80°C
fattore_portata=1966,//16*.15/5*4096 DAC portata
durata_avviamento=4000, inizio_lettura_I_avviamento=3900,
tempo_eccitazione_relais=500,//.5s
chiave_ingresso=123,
indirizzo_numero_serie=8040,
indirizzo_conta_secondi_attivita=8136,

/*
organizzazione della eeprom
0-8039 salvataggio degli allarmi
8040-8103 dati di funzionamento
8136-8168 salvataggio dei secondi di attività
*/

dati_di_fabbrica[48]={
1,    //numero_serie,
1,    //monofase_trifase, 0=monofase, 1=trifase
400,  //tensione_nominale, V
115,  //limite_sovratensione %
85,   //limite_sottotensione %
10,   //limite_segnalazione_dissimmetria %
15,   //limite_intervento_dissimmetria %
10,   //timeout_protezione_tensione s
60,   //corrente_nominale A*10
120,  //limite_sovracorrente %
10,   //limite_segnalazione_squilibrio %
15,   //limite_intervento_squilibrio %
10,   //timeout_protezione_squilibrio s
120,  //costante_tau_salita_temperatura s
20,   //taratura_temperatura_ambiente °C
10,   //scala_temperatura_motore,//mV/°C
100,  //limite_intervento_temper_motore,//°C
100,  //scala_corrente_differenziale  mA in /mA out
300,  //limite_corrente_differenziale,//mA
20,   //ritardo_intervento_differenziale,//ms
10,   //ritardo_funzionamento_dopo_emergenza,//s
160,  //portata_sensore_pressione BAR*10
100,  //limite_intervento_pressione Bar*10
1000, //potenza_minima_mandata_chiusa W
12,   //ritardo_stop_mandata_chiusa 
800,  //potenza_minima_funz_secco W
40,   //ritardo_stop_funzionemento_a_secco
900,  //ritardo_riaccensione_a_secco
0,    //modo_start_stop,//0=remoto o 1=pressione
30,   //pressione_accensione BAR*10
40,   //pressione_spegnimento BAR*10
1,    //lingua
20,   //temperatura_ambiente
0,    //motore_on
0,    //numero_segnalazione;//31
0,0,0,0,0,0,0,0,0,0,0,0,0 //riserva[13]
},

limiti_inferiori[48]={
1,    //numero_serie,
0,    //monofase_trifase, 0=monofase, 1=trifase
110,  //tensione_nominale, V
100,  //limite_sovratensione %
60,   //limite_sottotensione %
5,    //limite_segnalazione_dissimmetria %
5,    //limite_intervento_dissimmetria %
2,    //timeout_protezione_tensione s
10,   //corrente_nominale A*10
105,  //limite_sovracorrente %
5,    //limite_segnalazione_squilibrio %
5,    //limite_intervento_squilibrio %
1,    //timeout_protezione_squilibrio s
20,   //costante_tau_salita_temperatura s
5,    //taratura_temperatura_ambiente °C
2,    //scala_temperatura_motore,//mV/°C
70,   //limite_intervento_temper_motore,//°C
1,    //scala_corrente_differenziale  mA in /mA out
10,   //limite_corrente_differenziale,//mA
5,    //ritardo_intervento_differenziale,//ms
2,    //ritardo_funzionamento_dopo_emergenza,//s
40,   //portata_sensore_pressione BAR*10
40,   //limite_intervento_pressione Bar*10
50,   //potenza_minima_mandata_chiusa W
5,    //ritardo_stop_mandata_chiusa 
100,  //potenza_minima_funz_secco W
5,    //ritardo_stop_funzionemento_a_secco
60,   //ritardo_riaccensione_a_secco
0,    //modo_start_stop,//0=remoto o 1=pressione
5,    //pressione_accensione BAR*10
10,   //pressione_spegnimento BAR*10
0,    //lingua
0,    //temperatura_ambiente
0,    //motore_on
0,    //numero_segnalazione;//31
0,0,0,0,0,0,0,0,0,0,0,0,0 //riserva[13]
},

limiti_superiori[48]={
65535,//numero_serie,
1,    //monofase_trifase, 0=monofase, 1=trifase
440,  //tensione_nominale, V
125,  //limite_sovratensione %
95,   //limite_sottotensione %
20,   //limite_segnalazione_dissimmetria %
25,   //limite_intervento_dissimmetria %
120,  //timeout_protezione_tensione s
200,  //corrente_nominale A*10
140,  //limite_sovracorrente %
20,   //limite_segnalazione_squilibrio %
25,   //limite_intervento_squilibrio %
120,  //timeout_protezione_squilibrio s
600,  //costante_tau_salita_temperatura s
40,   //taratura_temperatura_ambiente °C
100,  //scala_temperatura_motore,//mV/°C
130,  //limite_intervento_temper_motore,//°C
250,  //scala_corrente_differenziale  mA in /mA out
500,  //limite_corrente_differenziale,//mA
100,  //ritardo_intervento_differenziale,//ms
120,  //ritardo_funzionamento_dopo_emergenza,//s
500,  //portata_sensore_pressione BAR*10
450,  //limite_intervento_pressione Bar*10
3000, //potenza_minima_mandata_chiusa W
120,  //ritardo_stop_mandata_chiusa 
2500, //potenza_minima_funz_secco W
120,  //ritardo_stop_funzionemento_a_secco
2000, //ritardo_riaccensione_a_secco
1,    //modo_start_stop,//0=remoto o 1=pressione
400,  //pressione_accensione BAR*10
440,  //pressione_spegnimento BAR*10
1,    //lingua
50,   //temperatura_ambiente
1,    //motore_on
334,  //numero_segnalazione;//31
0,0,0,0,0,0,0,0,0,0,0,0,0 //riserva[13]
};

struct
{
int //dai di funzionamento a partire dall'indirizzo 8040  della eeprom
numero_serie, //0
monofase_trifase,
tensione_nominale,
limite_sovratensione,
limite_sottotensione,
limite_segnalazione_dissimmetria,
limite_intervento_dissimmetria,
timeout_protezione_tensione,
corrente_nominale,
limite_sovracorrente,
limite_segnalazione_squilibrio,
limite_intervento_squilibrio,
timeout_protezione_squilibrio,
costante_tau_salita_temperatura,
taratura_temperatura_ambiente,//da modificare per la taratura
scala_temperatura_motore,//mV/°C
limite_intervento_temper_motore,//°C
scala_corrente_differenziale, //mA in /mA out
limite_corrente_differenziale,//mA
ritardo_intervento_differenziale,//ms
ritardo_funzionamento_dopo_emergenza,//s
portata_sensore_pressione,
limite_intervento_pressione,
potenza_minima_mandata_chiusa,
ritardo_stop_mandata_chiusa, //s
potenza_minima_funz_secco, //W
ritardo_stop_funzionemento_a_secco, //s
ritardo_riaccensione_a_secco,//s
modo_start_stop,//remoto o pressione
pressione_accensione,
pressione_spegnimento,
lingua,
temperatura_ambiente,
motore_on,
numero_segnalazione,
riserva[13];
}set;

struct
{
int //ingressi analogici
I1letta,//A*.004*5/.512* 4096/5 = A*32
I3letta,//A*.004*5/.512* 4096/5 = A*32
V12letta,//V* 33/680/6 /2 *4096/5 = V*3.313
V13letta,//V* 33/680/6 /2 *4096/5 = V*3.313
Idletta,//corrente differenziale
sensore_pressione,//V*4096/5 = mA*.15*4096/5 = mA*122.88
temperatura_letta,
tensione_15V; //V*4.7/(33+4.7) *4096/5 = V*102
}DAC;

char
display[34],
cifre[16], 
buffer_USB[32],//contiene i 24 bytes degli errori
timer_reset_display,
cursore_menu,
funzioni_avanzate,
comando_display,
contatore_display,
contatore_AD;

char
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
intervento_allarme,
prima_accensione,
attesa_invio,
relais_alimentazione,
relais_avviamento,
precedente_segnalazione;

int 
I1,//A*.004*5/.512* 4096/5 = A*32
I3,//A*.004*5/.512* 4096/5 = A*32
V12,//V* 33/680/6 /2 *4096/5 = V*3.313
V13,//V* 33/680/6 /2 *4096/5 = V*3.313
Id,//corrente differenziale
V1, V2, V3,//V*3.313 * 3
I2,
tot_misure;

unsigned int //timer
timer_eccitazione_relais,
timer_rinfresco_display,
timer_riavviamento,
timer_attesa_squilibrio,
timer_attesa_tensione,
timer_mandata_chiusa,
timer_attesa_secco,
timer_attesa_differenziale,
timer_commuta_presentazione,
timer_aggiorna_misure,
timer_avviamento_monofase,
timer_lampeggio,
timer_20ms,
timer_1s,
timer_10min,
timer_inc_dec,
timer_piu_meno,
timer_rilascio;//dei pulsanti start e stop

unsigned long //offset delle letture
offset_I1letta,
offset_I3letta,
offset_V12letta,
offset_V13letta,
offset_Idletta;

long //letture sommate
conta_secondi_attivita,
delta_T,//sovra_temperatura calcolata
quad_Id,
somma_quad_I1,
somma_quad_I2,
somma_quad_I3,
somma_quad_V1,
somma_quad_V2,
somma_quad_V3,
somma_potenzaI1xV1,
somma_potenzaI2xV2,
somma_potenzaI3xV3,
somma_reattivaI1xV23,
somma_reattivaI2xV31,
somma_reattivaI3xV12,
quad_I1,
quad_I2,
quad_I3,
quad_V1,
quad_V2,
quad_V3;

long //valori medi
media_temperatura,//°C
media_pressione,//Bar*10
media_quad_Id,
media_quad_I1,//(A*.004*5/.512* 4096/5)^2 = A^2*1024
media_quad_I2,
media_quad_I3,
media_quad_V1,
media_quad_V2,
media_quad_V3,
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
V1_rms,//V
V2_rms,
V3_rms,
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
dissimmetria,
squilibrio;

struct//dati buffer allarmi
{
char segnalazione;//tipo intervento:
             // 1 ON 
             // 2 OFF senza intervento protezioni     

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
unsigned long ora_minuto_secondo;
int 
tensione[3],
corrente[3],
potenza,
pressione;
char cosfi;
int temperatura;
}registrazione;

unsigned int
indirizzo_scrivi_eeprom,//0-8192 indirizza il byte in fase di scrittura
indirizzo_leggi_eeprom,//0-8192 indirizza il byte in fase di lettura
ultimo_indirizzo_scrittura,//in bytes
primo_indirizzo_lettura,//in bytes  mettere primo_indirizzo<ultimo_indirizzo e leggi_eeprom=1;
ultimo_indirizzo_lettura,//in bytes
buffer_eeprom[48];

unsigned char //eeprom
scrivi_eeprom,
salvataggio_in_corso,
leggi_eeprom,
leggi_impostazioni,
salva_conta_secondi,
leggi_conta_secondi,
conta_dati_scrittura_eeprom;

unsigned int
fattore_I2xT,
sovraccarico_moderato, sovraccarico,
precedente_lettura_allarme,
numero_ingresso,
allarme_in_lettura; //variabile scritta tramite USB

unsigned char
reset_default,
abilita_reset,
pronto_alla_risposta,//tramite USB
cosfi[3];


//__interrupt void USB_status(void) //vedi Fantuzzi
//{
//}


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
 ADD prod:2
 STA prod:2
 TXA
 ADC #0
 STA prod:1
 LDA var
 TAX
 MUL
 ADD prod:1
 STA prod:1
 TXA
 ADC #0
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

long somma_potenze(int x, int y)
{
char segno;
long prod, somma;
asm
 {
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
 LDA x
 LDA y
 MUL
 ADD prod:1
 STA prod:1
 TXA
 ADC #0
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
 }
return somma; 
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
char j, k;
asm
 {
 CLR j
 CLR k
 LDHX media_potenza
 LDA 3,X
 STA media:3
 LDA 2,X
 STA media:2
 LDA 1,X
 STA media:1
 LDA 0,X
 STA media
 BPL vedi_potenza
 COM j
vedi_potenza:
 LDA potenza
 BPL per_delta
 COM k

per_delta:
 LDA potenza:1
 SUB media:1
 STA delta:2
 LDA potenza
 SBC media
 STA delta:1
 LDA k
 SBC j
 STA delta
 ROL delta:2 
 ROL delta:1 
 ROL delta 
 ROL delta:2 
 ROL delta:1 
 ROL delta 

 LDA media:2
 ADD delta:2
 STA media:2
 LDA media:1
 ADC delta:1
 STA media:1
 LDA media:0
 ADC delta:0
 STA media:0

 LDHX media_potenza
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

int valore_efficace(unsigned long quad, int costante)
{
long prod;
unsigned int radice, fattore;
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
 }
return radice; 
}

long media_quadrati_nel_periodo(unsigned long dividendo, int divisore)
{
char j, k;
asm
 {
 CLR k
 LDA #25
 STA j
ripeti_diff:
 LDA dividendo
 SUB divisore:1
 TAX
 LDA k
 SBC divisore
 BCS per_scorrimenti
 STA k
 STX dividendo
per_scorrimenti:
 ROL dividendo:3 
 ROL dividendo:2 
 ROL dividendo:1 
 ROL dividendo:0
 ROL k 
 DBNZ j,ripeti_diff
 COM dividendo:3 
 COM dividendo:2 
 COM dividendo:1 
 CLR dividendo
 }
return dividendo; 
}

int media_potenza_nel_periodo(long dividendo, int divisore)//W
{
//potenza = A*.004*5/.512* 4096/5 * V*33/680/6 /2 *4096/5 *3 = V*A*318 
//potenza (W) = potenza/318 = potenza*206>>16
char j, segno;
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
 LDA dividendo:3
 LDX #206
 MUL
 STX dividendo:3
 LDA dividendo:2
 LDX #206
 MUL
 ADD dividendo:3
 STA dividendo:3
 TXA
 ADC #0
 STA dividendo:2
 LDA dividendo:1
 LDX #206
 MUL
 ADD dividendo:2
 STA dividendo:2
 TXA
 ADC #0
 STA dividendo:1
 LDA dividendo
 LDX #206
 MUL
 ADD dividendo:1
 STA dividendo:1
 TXA
 ADC #0
 STA dividendo

 LDA dividendo:2
 STA dividendo:3
 LDA dividendo:1
 STA dividendo:2
 LDA dividendo
 STA dividendo:1
 CLR dividendo

 LDA #17
 STA j
ripeti_diff:
 LDA dividendo:1
 SUB divisore:1
 TAX
 LDA dividendo
 SBC divisore
 BCS per_scorrimenti
 STA dividendo
 STX dividendo:1
per_scorrimenti:
 ROL dividendo:3 
 ROL dividendo:2 
 ROL dividendo:1 
 ROL dividendo:0
 DBNZ j,ripeti_diff
 LDA segno
 BNE fine 
 COM dividendo:3 
 COM dividendo:2
fine: 
 LDHX dividendo:2
 STHX risultato 
 }
return risultato; 
}

__interrupt void AD_conversion_completeISR(void)
{
int k, V23, V31;
asm
 {
 SEI
 CLRH
 LDX contatore_AD
 LDA @comando_AD,X
 AND #$07
 TAX
 LDA ADCRL
 STA @DAC.I1letta:1,X ;//letture analogiche
 LDA ADCRH
 STA @DAC.I1letta,X
 CPX #4
 BEQ calcolo_potenze
 CPX #9
 BEQ calcolo_potenze
 CPX #15
 BEQ calcolo_potenze
 CPX #20
 BNE comando_di_conversione
calcolo_potenze:
 LDA tot_misure:1
 ADD #1
 STA tot_misure:1
 LDA tot_misure
 ADC #0
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
 LDA DAC.I1letta:1
 SUB offset_I1letta:1
 STA I1:1
 LDA DAC.I1letta
 SBC offset_I1letta
 STA I1

 LDA DAC.I3letta:1
 SUB offset_I3letta:1
 STA I3:1
 LDA DAC.I3letta
 SBC offset_I3letta
 STA I3

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

 LDA DAC.Idletta:1
 SUB offset_Idletta:1
 STA Id:1
 LDA DAC.Idletta
 SBC offset_Idletta
 STA Id

//tensioni di fase 
;V1 = V12+V13  
;V31 = -V13; 
;V23 =V21-V31 = -V12+V13
;V2 = V21+V23 = -V12+V23 = -2*V12+V13;  
;V3 = V31+V32 = -V13-V23 = -2*V13+V12;
;I2 = -I1-I3;
 
 LDA I1:1
 ADD I3:1
 STA I2:1
 LDA I1
 ADC I3
 STA I2
 CLRA
 SUB I2:1
 STA I2:1
 CLRA
 SBC I2
 STA I2
 
 LDA V12:1
 ADD V13:1
 STA V1:1
 LDA V12
 ADC V13
 STA V1
 
 CLRA
 SUB V13:1
 STA V31:1
 CLRA
 SBC V13
 STA V31
 
 LDA V13:1
 SUB V12:1
 STA V23:1
 LDA V13
 SBC V12
 STA V23
 
 LDHX V12
 STHX k
 LSL k:1
 ROL k
 LDA V13:1
 SUB k:1
 STA V2:1
 LDA V13
 SBC k
 STA V2
 
 LDHX V13
 STHX k
 LSL k:1
 ROL k
 LDA V12:1
 SUB k:1
 STA V3:1
 LDA V12
 SBC k
 STA V3
 
 LDA Id
 BPL al_quadrato_Id
 CLRA
 SUB Id:1
 STA Id:1
 CLRA
 SBC Id
 STA Id
al_quadrato_Id: 
 LDA Id:1
 TAX
 MUL
 STA quad_Id:3
 STX quad_Id:2
 LDA Id:1
 LDX Id
 MUL
 LSLA
 ROLX
 ADD quad_Id:2
 STA quad_Id:2
 TXA
 ADC #0
 STA quad_Id:1
 LDA Id
 TAX
 MUL
 ADD quad_Id:1
 STA quad_Id:1
 TXA
 ADC #0
 STA quad_Id
 }
somma_quadrati((unsigned long*)&somma_quad_I1,I1);
somma_quadrati((unsigned long*)&somma_quad_I2,I2);
somma_quadrati((unsigned long*)&somma_quad_I3,I3);
somma_quadrati((unsigned long*)&somma_quad_V1,V1);
somma_quadrati((unsigned long*)&somma_quad_V2,V2);
somma_quadrati((unsigned long*)&somma_quad_V3,V3);

somma_potenzaI1xV1=somma_potenze(I1,V1);
somma_potenzaI2xV2=somma_potenze(I2,V2);
somma_potenzaI3xV3=somma_potenze(I3,V3);
somma_reattivaI1xV23=somma_potenze(I1,V23);
somma_reattivaI2xV31=somma_potenze(I2,V31);
somma_reattivaI3xV12=somma_potenze(I3,V12);
asm
 { 
comando_di_conversione:
 CLRH
 LDX contatore_AD
 INCX
 CPX #33
 BCS per_comando_conversione
 CLRX
per_comando_conversione:
 STX contatore_AD
 LDA @comando_AD,X
 STA ADCSC1
 }
}

void lettura_pulsanti(void)
{
asm
 {
 BRSET 0,_PTAD,azzera_start
 LDA #500
 STA timer_rilascio 
 LDA start
 CMP #10
 BNE start_1
 LDA #1
 STA salita_start
start_1:
 LDA start
 CMP #11
 BCC vedi_meno
 INCA
 STA start
 BRA vedi_meno
azzera_start:
 CLRA
 STA start

vedi_meno:
 BRSET 1,_PTAD,azzera_meno
 LDA #500
 STA timer_rilascio 
 LDA meno
 CMP #10
 BNE meno_1
 LDA #1
 STA salita_meno
meno_1: 
 LDA meno
 CMP #11
 BCC vedi_func
 INCA
 STA meno
 BRA vedi_func
azzera_meno:
 CLRA
 STA meno

vedi_func:
 BRSET 2,_PTAD,azzera_func
 LDA #500
 STA timer_rilascio 
 LDA func
 CMP #10
 BNE func_1
 LDA #1
 STA salita_func
func_1: 
 LDA func
 CMP #11
 BCC vedi_stop
 INCA
 STA func
 BRA vedi_stop
azzera_func:
 CLRA
 STA func

vedi_stop:
 BRSET 3,_PTAD,azzera_stop
 LDA #500
 STA timer_rilascio 
 LDA stop
 CMP #10
 BNE stop_1
 LDA #1
 STA salita_stop
stop_1: 
 LDA stop
 CMP #11
 BCC vedi_piu
 INCA
 STA stop
 BRA vedi_piu
azzera_stop:
 CLRA
 STA stop

vedi_piu:
 BRSET 4,_PTAD,azzera_piu
 LDA #500
 STA timer_rilascio 
 LDA piu
 CMP #10
 BNE piu_1
 LDA #1
 STA salita_piu
piu_1: 
 LDA piu
 CMP #11
 BCC vedi_remoto
 INCA
 STA piu
 BRA vedi_remoto
azzera_piu:
 CLRA
 STA piu

vedi_remoto:
 BRSET 5,_PTAD,azzera_remoto
 LDA #500
 STA timer_rilascio 
 LDA remoto
 CMP #10
 BNE remoto_1
 LDA #1
 STA salita_remoto
remoto_1:
 LDA remoto
 CMP #11
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
 CLRA
 STA contatore_display
 STA start
 STA salita_start
 STA stop
 STA salita_stop
 STA toggle_stop
 STA func
 STA salita_func
 STA toggle_func
 STA meno
 STA salita_meno
 STA piu
 STA salita_piu 
 STA remoto
 STA salita_remoto
 STA reset_default
 STA timer_rilascio
 STA timer_rilascio:1
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
 LDA toggle_stop
 BEQ presenta_carattere
 LDA timer_lampeggio
 CMP #4
 BCC presenta_carattere
 LDA #' '
 STA Display
 BRA increm_contatore
presenta_carattere: 
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
 CMP #4
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
 BRA increm_contatore
reset_timer_lampeggio: 
 LDA #15 
 STA timer_lampeggio
 
increm_contatore:
 LDA Display
 STA _PTDD
 LDA contatore_display
 INCA
 STA contatore_display
fine:
 }
}

char calcolo_del_fattore_di_potenza(long attiva, long reattiva)//cosfi*100
{
long quad_attiva, quad_reattiva;
unsigned int fattore, quad;
char j, radice, prod[5];
radice=0;
asm
 {
 LDA attiva
 BPL al_quadrato
 CLRA
 SUB attiva:1
 STA attiva:1
 CLRA
 SBC attiva
 STA attiva
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
 ADD quad_attiva:2
 STA quad_attiva:2
 TXA
 ADC #0
 STA quad_attiva:1
 LDA attiva
 TAX
 MUL
 ADD quad_attiva:1
 STA quad_attiva:1
 TXA
 ADC #0
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
 ADD quad_reattiva:2
 STA quad_reattiva:2
 TXA
 ADC #0
 STA quad_reattiva:1
 LDA reattiva
 TAX
 MUL
 ADD quad_reattiva:1
 STA quad_reattiva:1
 TXA
 ADC #0
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
 
 LDA quad_attiva:3
 LDX #39
 MUL
 STA prod:4
 STX prod:3
 LDA quad_attiva:2
 LDX #39
 MUL
 ADD prod:3
 STA prod:3
 TXA 
 ADC #0
 STA prod:2
 LDA quad_attiva:1
 LDX #39
 MUL
 ADD prod:2
 STA prod:2
 TXA 
 ADC #0
 STA prod:1
 LDA quad_attiva
 LDX #39
 MUL
 ADD prod:1
 STA prod:1
 TXA 
 ADC #0
 STA prod

 LDA #17
 STA j
ripeti_diff:
 LDA prod:2
 SUB quad_reattiva:2
 STA fattore:1
 LDA prod:1
 SBC quad_reattiva:1
 STA fattore
 LDA prod
 SBC quad_reattiva
 BCS alla_rotazione
 STA prod
 LDHX fattore
 STHX prod:1
alla_rotazione:
 ROL prod:4 
 ROL prod:3 
 ROL prod:2 
 ROL prod:1 
 ROL prod 
 DBNZ j,ripeti_diff
 COM prod:4 //cosfi^2
 COM prod:3 

//radice 
 LDA #12
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
 CPHX prod
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
 BEQ spegni_relais
 LDHX PWM_relais
 STHX _TPM1C2V
 BRA vedi_relais_avviam 
spegni_relais: 
 LDHX #24
 STHX _TPM1C2V  
vedi_relais_avviam: 
 LDA relais_avviamento
 BEQ spegni_relais_avviam
 LDHX PWM_relais
 STHX _TPM1C3V
 LDHX #$7fff
 STHX _TPM1C4V
 STHX _TPM1C5V
 BRA fine
spegni_relais_avviam:
 LDHX #24
 STHX _TPM1C3V  
 LDHX #-1
 STHX _TPM1C5V
 LDHX #$7fff
 STHX _TPM1C4V
fine:   
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
 LDHX numeratore
 STHX PWM_relais
 }
}

__interrupt void timer2_overflowISR(void) //timer 1 ms
{
long delta;
int temperatura, pressione;
char j;
asm
 {
 SEI  /*disabilita interrupt*/
 LDA _TPM2SC
 BCLR 7,_TPM2SC
 LDA timer_20ms:1
 BEQ per_reset_timer_20ms
 DECA
 STA timer_20ms:1
 BRA per_timer_1ms
per_reset_timer_20ms:
 LDA #20
 STA timer_20ms:1
 }
//funzioni ripetute ogni 20ms
quad_I1=media_quadrati_nel_periodo(somma_quad_I1,tot_misure);
quad_I2=media_quadrati_nel_periodo(somma_quad_I2,tot_misure);
quad_I3=media_quadrati_nel_periodo(somma_quad_I3,tot_misure);
quad_V1=media_quadrati_nel_periodo(somma_quad_V1,tot_misure);
quad_V2=media_quadrati_nel_periodo(somma_quad_V2,tot_misure);
quad_V3=media_quadrati_nel_periodo(somma_quad_V3,tot_misure);
potenzaI1xV1=media_potenza_nel_periodo(somma_potenzaI1xV1,tot_misure);//W
potenzaI2xV2=media_potenza_nel_periodo(somma_potenzaI2xV2,tot_misure);//W
potenzaI3xV3=media_potenza_nel_periodo(somma_potenzaI3xV3,tot_misure);//W
reattivaI1xV23=media_potenza_nel_periodo(somma_reattivaI1xV23,tot_misure);//VAR
reattivaI2xV31=media_potenza_nel_periodo(somma_reattivaI2xV31,tot_misure);//VAR
reattivaI3xV12=media_potenza_nel_periodo(somma_reattivaI3xV12,tot_misure);//VAR
somma_quad_I1=0;
somma_quad_I2=0;
somma_quad_I3=0;
somma_quad_V1=0;
somma_quad_V2=0;
somma_quad_V3=0;
somma_potenzaI1xV1=0;
somma_potenzaI2xV2=0;
somma_potenzaI3xV3=0;
somma_reattivaI1xV23=0;
somma_reattivaI2xV31=0;
somma_reattivaI3xV12=0;
tot_misure=0;
USB_time_sw();
asm
 {
 LDA timer_1s:1
 BEQ per_reset_timer_1s
 DECA
 STA timer_1s:1
 BRA per_timer_1ms
per_reset_timer_1s: 
 LDA #50
 STA timer_1s:1

//funzioni ripetute ogni secondo
 JSR protezione_termica

 LDA timer_rinfresco_display:1
 BEQ per_rinfresco
 DECA
 STA timer_rinfresco_display:1
 BRA per_timer_riavviamento
per_rinfresco:
 LDA #30//ogni 30 s
 STA timer_rinfresco_display:1
 LDA #1
 STA comando_display

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
 LDA timer_attesa_secco:1
 BEQ per_timer_mandata_chiusa
 DECA
 STA timer_attesa_secco:1
 
per_timer_mandata_chiusa: 
 LDA timer_mandata_chiusa:1
 BEQ per_timer_attesa_tensione
 DECA
 STA timer_mandata_chiusa:1
 
per_timer_attesa_tensione: 
 LDA timer_attesa_tensione:1
 BEQ per_timer_attesa_squilibrio
 DECA
 STA timer_attesa_tensione:1
 
per_timer_attesa_squilibrio: 
 LDA timer_attesa_squilibrio:1
 BEQ per_conta_secondi
 DECA
 STA timer_attesa_squilibrio:1
 
per_conta_secondi: 
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
 LDHX timer_10min
 BEQ per_reset_timer_10min
 LDA timer_10min:1
 SUB #1
 STA timer_10min:1
 LDA timer_10min
 SBC #0
 STA timer_10min
 BRA per_timer_1ms
per_reset_timer_10min: 
 LDHX #600
 STHX timer_10min
//funzioni ripetute ogni 10 minuti
 LDA #1
 STA salva_conta_secondi

per_timer_1ms: 
 LDA timer_20ms
 CMP #4
 BNE vedi_media_quad_V
 }
calcolo_medie_quadrati((unsigned long*)&media_quad_I1,quad_I1); //tau = 160 ms
I1_rms=valore_efficace(media_quad_I1,6400); //(A*.004*5/.512* 4096/5)^2 = A^2*1024  = (A*10)^2*10.24 = (A*10)^2*6400>>16
calcolo_medie_quadrati((unsigned long*)&media_quad_I2,quad_I2);
I2_rms=valore_efficace(media_quad_I2,6400); 
calcolo_medie_quadrati((unsigned long*)&media_quad_I3,quad_I3);
I3_rms=valore_efficace(media_quad_I3,6400); 
asm
 {
vedi_media_quad_V:
 LDA timer_20ms
 CMP #8
 BNE vedi_media_potenze
 }
calcolo_medie_quadrati((unsigned long*)&media_quad_V1,quad_V1);
V1_rms=valore_efficace(media_quad_V1,663);//(V *3*33/680/6 /2 *4096/5)^2 = V^2*98.78 = V^2*663>>16
calcolo_medie_quadrati((unsigned long*)&media_quad_V2,quad_V2);
V2_rms=valore_efficace(media_quad_V2,663); 
calcolo_medie_quadrati((unsigned long*)&media_quad_V3,quad_V3);
V3_rms=valore_efficace(media_quad_V3,663); 
asm
 {
vedi_media_potenze:
 LDA timer_20ms
 CMP #12
 BNE vedi_calcolo_temperatura
 }
calcolo_valor_medio_potenze((long*)&media_potenzaI1xV1,potenzaI1xV1);
calcolo_valor_medio_potenze((long*)&media_potenzaI2xV2,potenzaI2xV2);
calcolo_valor_medio_potenze((long*)&media_potenzaI3xV3,potenzaI3xV3);
calcolo_valor_medio_potenze((long*)&media_reattivaI1xV23,reattivaI1xV23);
calcolo_valor_medio_potenze((long*)&media_reattivaI2xV31,reattivaI2xV31);
calcolo_valor_medio_potenze((long*)&media_reattivaI3xV12,reattivaI3xV12);
asm
 {
vedi_calcolo_temperatura:
 LDA timer_20ms
 CMP #14
 BNE vedi_calcolo_cosfi
//calcolo della tenperatura motore    
;temperatura=(set.temperatura_ambiente+DAC.temperatura_letta)*5000/4096/set.scala_temperatura_motore-set.taratura_temperatura_ambiente,//da modificare per la taratura
;temperatura=(set.temperatura_ambiente+DAC.temperatura_letta)*625/512/set.scala_temperatura_motore-set.taratura_temperatura_ambiente,//da modificare per la taratura
 LDA DAC.temperatura_letta:1
 ADD set.temperatura_ambiente:1
 STA temperatura:1
 LDA DAC.temperatura_letta
 ADC set.temperatura_ambiente
 STA temperatura

 LDA temperatura:1
 LDX #113
 MUL
 STX delta:2
 LDA temperatura
 LDX #113
 MUL
 ADD delta:2
 STA delta:2
 TXA
 ADC #0
 STA delta:1
 LDA temperatura:1
 LDX #2
 MUL
 ADD delta:2
 STA delta:2
 TXA
 ADC delta:1
 STA delta:1
 LDA temperatura
 LDX #2
 MUL
 ADD delta:1
 STA delta:1
 TXA
 ADC delta
 STA delta

 LDHX set.scala_temperatura_motore
 DIV
 LDA delta:1
 DIV
 STA delta:1
 LDA delta:2
 DIV
 SUB set.taratura_temperatura_ambiente:1
 STA temperatura:1
 LDA delta:1
 SBC set.taratura_temperatura_ambiente
 STA temperatura

//calcolo della pressione
;pressione=(DAC.sensore_pressione-491)*set.portata_sensore_pressione/fattore_portata;
 LDA DAC.sensore_pressione:1
 SUB #235
 STA pressione:1
 LDA DAC.sensore_pressione
 SBC #1
 STA pressione
 BCC per_prodotto_portata
 CLR pressione
 CLR pressione:1
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
 LDHX delta:2
 STHX pressione
 }
calcolo_valor_medio_potenze((long*)&media_temperatura,temperatura);
calcolo_valor_medio_potenze((long*)&media_pressione,pressione);
asm
 {
vedi_calcolo_cosfi:
 LDA timer_20ms
 CMP #16
 BNE vedi_media_quad_Id
 }
cosfi[0]=calcolo_del_fattore_di_potenza(media_potenzaI1xV1,media_reattivaI1xV23);//cosfi*100
cosfi[1]=calcolo_del_fattore_di_potenza(media_potenzaI2xV2,media_reattivaI2xV31);//cosfi*100
cosfi[2]=calcolo_del_fattore_di_potenza(media_potenzaI3xV3,media_reattivaI3xV12);//cosfi*100
registrazione.cosfi=calcolo_del_fattore_di_potenza(media_potenzaI1xV1+media_potenzaI2xV2+media_potenzaI3xV3,
                                                   media_reattivaI1xV23+media_reattivaI2xV31+media_reattivaI3xV12);//cosfi*100
asm 
 {
vedi_media_quad_Id: 
//funzioni ripetute ogni ms
 LDA quad_Id:1
 SUB media_quad_Id:1
 STA delta:2
 LDA quad_Id
 SBC media_quad_Id
 STA delta:1
 CLRA
 SBC #0
 STA delta
 ROL delta:2
 ROL delta:1
 ROL delta:0
 ROL delta:2
 ROL delta:1
 ROL delta:0
 LDA media_quad_Id:2
 ADD delta:2
 STA media_quad_Id:2
 LDA media_quad_Id:1
 ADC delta:1
 STA media_quad_Id:1
 LDA media_quad_Id
 ADC delta
 STA media_quad_Id

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
 BEQ per_timer_aggiorna_misure
 LDA timer_attesa_differenziale:1
 SUB #1
 STA timer_attesa_differenziale:1
 LDA timer_attesa_differenziale
 SBC #0
 STA timer_attesa_differenziale
 
per_timer_aggiorna_misure:
 LDHX timer_aggiorna_misure
 BEQ per_timer_avviamento_monofase
 LDA timer_aggiorna_misure:1
 SUB #1
 STA timer_aggiorna_misure:1
 LDA timer_aggiorna_misure
 SBC #0
 STA timer_aggiorna_misure
 
per_timer_avviamento_monofase:
 LDHX timer_avviamento_monofase
 BEQ spegni_relais_avviamento
 LDA timer_avviamento_monofase:1
 SUB #1
 STA timer_avviamento_monofase:1
 LDA timer_avviamento_monofase
 SBC #0
 STA timer_avviamento_monofase
 BRA per_timer_commuta_presentazione
spegni_relais_avviamento:
 CLRA
 STA relais_avviamento
 
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
 LDHX #5000
 STHX timer_commuta_presentazione 

per_LED:
 LDHX #24
 STHX TPM2C1V;//2us
 LDA intervento_allarme
 BEQ spegni_LED_rosso
 LDHX #$7fff
 STHX TPM2C0V
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
;  PTED &=0xfb; //EEPROM abilitata
;  conta_dati_scrittura_eeprom=0;
;  dato_scritto=0x06;//abilita scrittura
;  scrivi_eeprom++;
;  }
; else if(scrivi_eeprom==2) //spazio di disabilatazione e riabilitazione
;  {
;  PTED |=0x04; //EEPROM disabilitata
;  scrivi_eeprom++;
;  }
; else if(scrivi_eeprom==3) //spazio di disabilatazione e riabilitazione
;  {
;  PTED &=0xfb; //EEPROM abilitata
;  dato_scritto=0x02;//codice di scrittura
;  scrivi_eeprom++;
;  }

; else if(scrivi_eeprom==4) //prima abilitazione e indirizzo alto
;  {
;  dato_scritto=indirizzo_scrivi_eeprom>>8;
;  scrivi_eeprom++;
;  }
; else if(scrivi_eeprom==5) //scrivi parte bassa indirizzo
;  {
;  dato_scritto=indirizzo_scrivi_eeprom;
;  scrivi_eeprom++;
;  }
; else if(conta_dati_scrittura_eeprom<16)
;  {
;  if(scrivi_eeprom==6) //scrivi parte alta dato
;   {
;   dato_scritto=buffer_eeprom[indirizzo_scrivi_eeprom+conta_dati_scrittura_eeprom]>>8;
;   scrivi_eeprom++;
;   }
;  else if(scrivi_eeprom==7) //scrivi parte bassa dato
;   {
;   dato_scritto=buffer_eeprom[indirizzo_scrivi_eeprom+conta_dati_scrittura_eeprom];
;   conta_dati_scrittura_eeprom++;
;   scrivi_eeprom=6;
;   }
;  }
; else if(scrivi_eeprom<20)
;  {
;  PTED |=0x04; //EEPROM disabilitata
;  scrivi_eeprom++;
;  }
; else if(scrivi_eeprom==20)
;  {
;  PTED &=0xfb; //EEPROM abilitata
;  dato_scritto=0x04; //codice di blocco della scrittura
;  scrivi_eeprom++;
;  }
; else//if(scrivi_eeprom==21)
;  {
;  PTED |=0x04; //EEPROM disabilitata
;  scrivi_eeprom=0;
;  }
; }
 LDA scrivi_eeprom
 BEQ lettura_EE
 CMP #1
 BNE spazio_abilitazione
; if(scrivi_eeprom==1)
;  {
;  PTED &=0xfb; //EEPROM abilitata
;  conta_dati_scrittura_eeprom=0;
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
;  PTED |=0x04; //EEPROM disabilitata
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
;  PTED &=0xfb; //EEPROM abilitata
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
 BNE parte_bassa_indirizzo
; else if(scrivi_eeprom==4) //prima abilitazione e indirizzo alto
;  {
;  dato_scritto=indirizzo_scrivi_eeprom>>8;
;  scrivi_eeprom++;
;  }
 LDA indirizzo_scrivi_eeprom
 STA dato_scritto//_SPID
 LDA scrivi_eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

parte_bassa_indirizzo:
 CMP #5
 BNE parte_alta_dato
; else if(scrivi_eeprom==5) //scrivi parte bassa indirizzo
;  {
;  dato_scritto=indirizzo_scrivi_eeprom;
;  scrivi_eeprom++;
;  }
 LDA indirizzo_scrivi_eeprom:1
 STA dato_scritto//_SPID
 LDA scrivi_eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

parte_alta_dato: 
 LDA conta_dati_scrittura_eeprom
 CMP #16
 BCC chiusura_scrittura
;  if(scrivi_eeprom==6) //scrivi parte alta dato
;   {
;   dato_scritto=buffer_eeprom[indirizzo_scrivi_eeprom+conta_dati_scrittura_eeprom]>>8;
;   scrivi_eeprom++;
;   }
;  else if(scrivi_eeprom==7) //scrivi parte bassa dato
;   {
;   dato_scritto=buffer_eeprom[indirizzo_scrivi_eeprom+conta_dati_scrittura_eeprom];
;   conta_dati_scrittura_eeprom++;
;   scrivi_eeprom=6;
;   }
 LDA scrivi_eeprom
 CMP #6
 BNE parte_bassa_dato
 LDA conta_dati_scrittura_eeprom
 LSLA
 ADD indirizzo_scrivi_eeprom:1
 TAX
 CLRH
 LDA @buffer_eeprom,X
 STA dato_scritto//_SPID
 LDA scrivi_eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

parte_bassa_dato: 
 LDA conta_dati_scrittura_eeprom
 LSLA
 ADD indirizzo_scrivi_eeprom:1
 TAX
 CLRH
 LDA @buffer_eeprom:1,X
 STA dato_scritto//_SPID
 LDA conta_dati_scrittura_eeprom
 INCA
 STA conta_dati_scrittura_eeprom
 LDA #6
 STA scrivi_eeprom
 BRA fine_SPID

chiusura_scrittura:
 LDA scrivi_eeprom
 CMP #20 
 BEQ comando_disabilitazione
 BCC per_disabilitazione
; else if(scrivi_eeprom<20)
;  {
;  PTED |=0x04; //EEPROM disabilitata
;  scrivi_eeprom++;
;  }
 BSET 7,_PTED //disabilita eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

comando_disabilitazione:
; else if(scrivi_eeprom==20)
;  {
;  PTED &=0xfb; //EEPROM abilitata
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
;  PTED |=0x04; //EEPROM disabilitata
;  scrivi_eeprom=0;
;  }
 BSET 7,_PTED //disabilita eeprom
 CLRA
 STA scrivi_eeprom
 BRA fine_SPID

;else if((leggi_eeprom)&&(salvataggio_in_corso==0))
; {
; if(leggi_eeprom==1)
;  {
;  PTED &=0xfb; //EEPROM abilitata
;  dato_scritto=0x03;//abilitazione lettura
;  indirizzo_leggi_eeprom=primo_indirizzo_lettura;
;  leggi_eeprom++;
;  }
; else if(leggi_eeprom==2) //spazio di disabilatazione e riabilitazione
;  {
;  dato_scritto=indirizzo_leggi_eeprom>>8;//parte alta primo indirizzo
;  leggi_eeprom++;
;  }
; else if(leggi_eeprom==3) //spazio di disabilatazione e riabilitazione
;  {
;  dato_scritto=indirizzo_leggi_eeprom;//parte bassa primo indirizzo
;  leggi_eeprom++;
;  }
; else if(leggi_eeprom==4)
;  {
;  dato_scritto=0;
;  indirizzo_leggi_eeprom++;
;  leggi_eeprom++;
;  }
; else if(indirizzo_leggi_eeprom<ultimo_indirizzo_lettura)
;  {
;  j=indirizzo_leggi_eeprom-primo_indirizzo_lettura-1;
;  buffer_eeprom[j]=dato_letto;
;  dato_scritto=0;
;  indirizzo_leggi_eeprom++;
;  }
; else
;  {
;  PTED |=0x04;
;  leggi_eeprom=0;
;  } 
; }
lettura_EE://per comandi lettura eeprom 
 LDA leggi_eeprom
 BEQ fine
 LDA salvataggio_in_corso
 BNE fine_SPID
 LDA leggi_eeprom
 CMP #1
 BNE parte_alta_indirizzo_lettura
; if(leggi_eeprom==1)
;  {
;  PTED &=0xfb; //EEPROM abilitata
;  dato_scritto=0x03;//abilitazione lettura
;  indirizzo_leggi_eeprom=primo_indirizzo_lettura;
;  leggi_eeprom++;
;  }
 BCLR 7,_PTED
 LDA #$03
 STA dato_scritto//,_SPID
 LDHX primo_indirizzo_lettura
 STHX indirizzo_leggi_eeprom
 LDA leggi_eeprom
 INCA
 STA leggi_eeprom
 BRA fine_SPID

parte_alta_indirizzo_lettura:
 CMP #2
 BNE parte_bassa_indirizzo_lettura
; else if(leggi_eeprom==2) //spazio di disabilatazione e riabilitazione
;  {
;  dato_scritto=indirizzo_leggi_eeprom>>8;//parte alta primo indirizzo
;  leggi_eeprom++;
;  }
 LDA indirizzo_leggi_eeprom
 STA dato_scritto  
 LDA leggi_eeprom
 INCA
 STA leggi_eeprom
 BRA fine_SPID

parte_bassa_indirizzo_lettura:
 CMP #3
 BNE primo_byte_letto
 LDA indirizzo_leggi_eeprom:1
 STA dato_scritto
 LDA leggi_eeprom
 INCA
 STA leggi_eeprom
 BRA fine_SPID

primo_byte_letto:
 CMP #4
 BNE lettura_dati
 LDA indirizzo_leggi_eeprom:1
 ADD #1
 STA indirizzo_leggi_eeprom:1
 LDA indirizzo_leggi_eeprom
 ADC #0
 STA indirizzo_leggi_eeprom
 LDA leggi_eeprom
 INCA
 STA leggi_eeprom
 BRA fine_SPID

lettura_dati:
 LDHX indirizzo_leggi_eeprom
 CPHX ultimo_indirizzo_lettura
 BCC fine_lettura
; else if(indirizzo_leggi_eeprom<ultimo_indirizzo_lettura)
;  {
;  j=indirizzo_leggi_eeprom-primo_indirizzo_lettura;
;  buffer_eeprom[j-1]=dato_letto;
;  dato_scritto=0;
;  indirizzo_leggi_eeprom++;
;  }
 LDA indirizzo_leggi_eeprom:1
 SUB primo_indirizzo_lettura:1
 TAX
 CLRH
 LDA dato_letto
 STA @buffer_eeprom:-1,X
 LDA leggi_eeprom
 INCA
 STA leggi_eeprom
 BRA fine_SPID

fine_lettura: 
 BSET 7,_PTED;//disabilita eepprom
 CLRA
 STA leggi_eeprom

fine_SPID:
 LDA dato_scritto
 STA SPI1D
fine: 
 } 
}


void salva_conta_secondi_attivita(void)//ogni 10 minuti
{
asm
 {
 LDA salva_conta_secondi
 BEQ fine
 LDA scrivi_eeprom
 BNE fine
 LDA leggi_eeprom
 BNE fine
 CLRA
 STA salva_conta_secondi
 LDHX indirizzo_conta_secondi_attivita
 STHX indirizzo_scrivi_eeprom
 LDHX conta_secondi_attivita
 STHX buffer_eeprom
 LDHX conta_secondi_attivita:2
 STHX buffer_eeprom:2
 LDA #1
 STA scrivi_eeprom
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
fine: 
 }
}

void leggi_conta_secondi_attivita(void)//all'accensione
{
asm
 {
 LDA leggi_conta_secondi
 BEQ fine
 LDA scrivi_eeprom
 BNE fine
 LDA leggi_eeprom
 BNE fine
 CLRA
 STA leggi_conta_secondi
 LDHX indirizzo_conta_secondi_attivita
 STHX indirizzo_scrivi_eeprom
 LDA #1
 STA leggi_eeprom
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
fine:
 } 
}

void salva_impostazioni(void)
{
asm
 {
 LDA reset_default
 BNE fine
 LDHX indirizzo_scrivi_eeprom
 CPHX ultimo_indirizzo_scrittura
 BCC salvataggio_terminato
 LDA scrivi_eeprom
 BNE fine
 LDA leggi_eeprom
 BNE fine
 LDA #1
 STA salvataggio_in_corso

 LDHX #96
ripeti_trasferimento: 
 LDA @set.numero_serie:-1,X
 STA @buffer_eeprom:-1,X
 DBNZX ripeti_trasferimento
 
 LDA indirizzo_scrivi_eeprom:1
 ADD #32
 STA indirizzo_scrivi_eeprom:1
 LDA indirizzo_scrivi_eeprom
 ADC #0
 STA indirizzo_scrivi_eeprom
 LDA #1
 STA scrivi_eeprom
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
 BRA fine
salvataggio_terminato:
 CLRA
 STA salvataggio_in_corso 
fine: 
 }
}

void lettura_impostazioni(void)
{
char j;
int var, def, min, max;
asm
 {
 LDA scrivi_eeprom
 BNE fine
 LDA leggi_eeprom
 BNE fine
 LDA leggi_impostazioni
 CMP #1
 BNE vedi_lettura_ultimata
 LDA #2
 STA leggi_impostazioni
 LDHX indirizzo_numero_serie
 STHX primo_indirizzo_lettura
 LDA primo_indirizzo_lettura:1
 ADD #96
 STA ultimo_indirizzo_lettura:1
 LDA primo_indirizzo_lettura
 ADC #0
 STA ultimo_indirizzo_lettura
 LDA #1
 STA leggi_eeprom
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
 BRA fine

vedi_lettura_ultimata:
 CMP #2
 BNE fine
 CLRA
 STA leggi_impostazioni
 LDA #48
 STA j
assegna_parametri:
 CLRH
 LDX j
 LSLX
 LDA @dati_di_fabbrica:-1,X
 STA def:1
 LDA @dati_di_fabbrica:-2,X
 STA def
 LDA @limiti_inferiori:-1,X
 STA min:1
 LDA @limiti_inferiori:-2,X
 STA min
 LDA @limiti_superiori:-1,X
 STA max:1
 LDA @limiti_superiori:-2,X
 STA max
 LDA @buffer_eeprom:-1,X
 STA var:1
 LDA @buffer_eeprom:-2,X
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
 LDA var
 STA @set.numero_serie:-1,X
 LDA var:1
 STA @set.numero_serie:-2,X
 DBNZ j,assegna_parametri

 LDA registrazione.segnalazione
 STA precedente_segnalazione
 JSR calcolo_delle_costanti
fine:
 } 
}

void lettura_allarme(void)  //con allarme_in_lettura <335
{
//la richiesta viene effettuata tramite USB dando un valore a allarme_in_lettura < 335
//dopo il tempo necessario alla lettura i dati vengono posti in buffer_USB e spediti
asm
 {
 LDA scrivi_eeprom
 BNE fine
 LDA leggi_eeprom
 BNE fine
 LDA attesa_invio
 BNE vedi_fine_lettura_eeprom
 LDHX allarme_in_lettura
 CPHX #335
 BCC fine
 CPHX precedente_lettura_allarme
 BEQ fine
 STHX precedente_lettura_allarme

 LDA allarme_in_lettura:1
 LDX #24
 MUL
 STA primo_indirizzo_lettura:1
 STX primo_indirizzo_lettura
 LDA allarme_in_lettura
 LDX #24
 MUL
 ADD primo_indirizzo_lettura
 STA primo_indirizzo_lettura
 LDA primo_indirizzo_lettura:1
 ADD #24
 STA ultimo_indirizzo_lettura:1
 LDA primo_indirizzo_lettura
 ADC #0
 STA ultimo_indirizzo_lettura
 LDA #1
 STA attesa_invio
 STA leggi_eeprom
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
 BRA fine
 
vedi_fine_lettura_eeprom:
 LDA attesa_invio
 BEQ fine
 LDHX #24
ripeti_assegnazione:
 LDA @buffer_eeprom:-1,X
 STA @buffer_USB:-1,X
 DBNZX ripeti_assegnazione
 LDA #1
 STA pronto_alla_risposta
fine:
 } 
}

void trasmissione_misure_istantanee(void) //con allarme_in_lettura == 1000
{
asm
 {
//V1,V2,V3,I1,I2,I3,cosfi1,cosfi2,cosfi3,pressione,temperatura
 LDHX allarme_in_lettura
 CPHX #1000
 BNE fine
 LDHX #1001
 STHX allarme_in_lettura
 LDHX V1_rms
 STHX buffer_USB
 LDHX V2_rms
 STHX buffer_USB:2
 LDHX V3_rms
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
 LDHX cosfi:4
 STHX buffer_USB:22
 LDHX media_pressione
 STHX buffer_USB:24
 LDHX media_temperatura
 STHX buffer_USB:26
fine: 
 }
}
 
void trasmissione_tarature(void) //con allarme_in_lettura == 2000
{
asm
 {
 LDHX allarme_in_lettura
 CPHX #2000
 BNE fine
 LDHX #2001
 STHX allarme_in_lettura
 LDHX #64
assegna_buffer:
 LDA @set.numero_serie:-1,X
 STA @buffer_USB:-1,X
 DBNZX assegna_buffer
fine: 
 }
}
 
void scrittura_allarme_in_eeprom(void)
{
asm
 {
 LDA scrivi_eeprom
 BNE fine
 LDA leggi_eeprom
 BNE fine
 LDA registrazione.segnalazione
 CMP precedente_segnalazione
 BEQ fine
 STA precedente_segnalazione

 LDHX set.numero_segnalazione
;  if(set.numero_segnalazione<334) set.numero_segnalazione++; else set.numero_segnalazione=0;
 CPHX #334
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
 LDA registrazione.segnalazione;//tipo intervento:
 STA buffer_eeprom
 LDHX registrazione.ora_minuto_secondo;
 STHX buffer_eeprom:1
 LDHX registrazione.ora_minuto_secondo:2;
 STHX buffer_eeprom:3
 LDHX registrazione.tensione;
 STHX buffer_eeprom:5
 LDHX registrazione.tensione:2
 STHX buffer_eeprom:7
 LDHX registrazione.tensione:4
 STHX buffer_eeprom:9
 LDHX registrazione.corrente
 STHX buffer_eeprom:11
 LDHX registrazione.corrente:2
 STHX buffer_eeprom:13
 LDHX registrazione.corrente:4
 STHX buffer_eeprom:15
 LDHX registrazione.potenza
 STHX buffer_eeprom:17
 LDHX registrazione.pressione
 STHX buffer_eeprom:19
 LDA registrazione.cosfi
 STA buffer_eeprom:21
 LDHX registrazione.temperatura
 STHX buffer_eeprom:22

 LDA set.numero_segnalazione:1
 LDX #24
 MUL
 STA indirizzo_scrivi_eeprom:1
 STX indirizzo_scrivi_eeprom
 LDA set.numero_segnalazione
 LDX #24
 MUL
 ADD indirizzo_scrivi_eeprom
 STA indirizzo_scrivi_eeprom
 LDA #1
 STA scrivi_eeprom
 STA salvataggio_in_corso
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
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

void modifica_unsigned(unsigned int *var, unsigned int min, unsigned int max)
{
unsigned int variab;
asm
 {
;if(salita_piu | salita_meno)
; {
; timer_inc_dec=0x1000;
; timer_piu_meno=200;
; salita_piu=0;
; salita_meno=0;
; }
;if(timer_piu_meno==0)
; {
; if(piu)
;  {
;  if(*var>=max) *var=max; else *var++;
;  timer_piu_meno=timer_inc_dec>>4;
;  }
; else if(meno)
;  {
;  if(*var<=min) *var=min; else *var--;
;  timer_piu_meno=timer_inc_dec>>4;
;  }
; } 
 LDHX var
 LDA 0,X
 STA variab
 LDA 1,X
 STA variab:1
 
 LDA salita_piu
 ORA salita_meno
 BEQ vedi_timer
 LDHX #$1000
 STHX timer_inc_dec
 LDHX #200
 STHX timer_piu_meno
 CLRA
 STA salita_meno
 STA salita_piu

vedi_timer:
 LDHX timer_piu_meno
 BNE fine
 LDA piu
 BEQ vedi_meno
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
 BRA fine

vedi_meno:
 LDA meno
 BEQ fine
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
 
fine:
 LDHX var
 LDA variab
 STA 0,X
 LDA variab:1
 STA 1,X
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

void funzione_reset_default(void)
{
char j;
asm
 {
 LDA reset_default
 BEQ fine
 CMP #1
 BNE per_default_parametri

 LDHX indirizzo_scrivi_eeprom
 CPHX ultimo_indirizzo_scrittura
 BCC salvataggio_terminato
 LDA scrivi_eeprom
 BNE fine
 LDA leggi_eeprom
 BNE fine
 LDA #1
 STA salvataggio_in_corso

 LDHX #32
ripeti_trasferimento: 
 LDA #$ff
 STA @buffer_eeprom:-1,X
 DBNZX ripeti_trasferimento
 
 LDA indirizzo_scrivi_eeprom:1
 ADD #32
 STA indirizzo_scrivi_eeprom:1
 LDA indirizzo_scrivi_eeprom
 ADC #0
 STA indirizzo_scrivi_eeprom
 LDA #1
 STA scrivi_eeprom
 BSET 7,_PTED;//abilitazione eeprom
 MOV #$ff,SPI1D
 BRA fine
salvataggio_terminato:
 CLRA
 STA salvataggio_in_corso 
 LDA #2
 STA reset_default
 BRA fine

per_default_parametri: 
 CMP #2
 BNE fine
 LDA #48
 STA j
assegna_parametri:
 CLRH
 LDX j
 LSLX
 LDA @dati_di_fabbrica:-1,X
 STA @set.numero_serie:-1,X
 LDA @dati_di_fabbrica:-2,X
 STA @set.numero_serie:-2,X
 DBNZ j,assegna_parametri
 CLRA
 STA conta_secondi_attivita
 STA conta_secondi_attivita:1
 STA conta_secondi_attivita:2
 STA conta_secondi_attivita:3
 STA abilita_reset
 JSR calcolo_delle_costanti
 }
presenta_scritta((char*)&comando_reset,3,0,1,1,0,16);//reset eseguito
asm
 {
fine:
 }
}

const int seimila400=6400, mille678=1678, mille515=1515;
void calcolo_delle_costanti(void)
{
char j;
int quad_In;
long prod, fattore;
asm
 { 
 //per il calcolo della sovratemperatura
 LDA set.corrente_nominale:1
 TAX
 MUL
 STA quad_In:1
 STX quad_In

 LDA seimila400:1
 LDX delta_Tn:1
 MUL
 STA fattore:3
 STX fattore:2
 LDA seimila400
 LDX delta_Tn:1
 MUL
 ADD fattore:2
 STA fattore:2
 TXA
 ADC #0
 STA fattore:1
 CLR fattore
 
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

//livello di sovraccarico ammissibile °C
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

//livello di sovraccarico ammissibile °C
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
 }
}

void programmazione(void)
{
static char aumenta;
if(salita_start)//uscita da programmazione
 {
 if(toggle_func)
  {
  calcolo_delle_costanti();
  //comando di salvataggio in eeprom
  indirizzo_scrivi_eeprom=indirizzo_numero_serie;
  ultimo_indirizzo_scrittura=indirizzo_conta_secondi_attivita;
  toggle_func=0;
  }
 }

if(salita_func)
 {
 salita_func=0;
 if(toggle_func)
  {
  calcolo_delle_costanti();
  //comando di salvataggio in eeprom
  indirizzo_scrivi_eeprom=indirizzo_numero_serie;
  ultimo_indirizzo_scrittura=indirizzo_conta_secondi_attivita;
  toggle_func=0;
  }
 else
  { 
  toggle_func=1;
  }
 }
 
if(toggle_func)//lettura e modifica delle funzioni
 {
 if(salita_stop)
  {
  salita_stop=0;
  if(toggle_stop) toggle_stop=0; else toggle_stop=1;
  }
 if(toggle_stop==0)//modifica della funzione
  {
  if(salita_meno)
   {
   salita_meno=0;
   aumenta=1;
   if(cursore_menu<23) cursore_menu++;
   else if((funzioni_avanzate)&&(cursore_menu<31)) cursore_menu++; else cursore_menu=0;
   }
  else if(salita_piu)
   {
   salita_piu=0;
   aumenta=0;
   if(cursore_menu>0) cursore_menu--;
   else if(funzioni_avanzate) cursore_menu=31; else cursore_menu=23;
   }
  presenta_menu((unsigned char*)&menu_principale,(char)set.lingua,64,cursore_menu<<1);
  if(numero_ingresso==chiave_ingresso) //presenta abilitazione alle funzioni avanzate
   {
   funzioni_avanzate=1;
   if(cursore_menu==20) presenta_scritta((char*)&abilita_avanzate,0,(char)set.lingua,2,1,0,16);
   }
  else
   {
   funzioni_avanzate=0;
   if(cursore_menu==20) presenta_scritta((char*)&abilita_avanzate,1,(char)set.lingua,2,1,0,16);
   }
  if((abilita_reset)&&(reset_default==0)&&(cursore_menu==28))
   {
   reset_default=1;
   indirizzo_scrivi_eeprom=0;
   ultimo_indirizzo_scrittura=8160;
   presenta_scritta((char*)&comando_reset,2,0,1,1,0,16);//reset iniziato
   }
  }
 else //if(toggle_stop) modifica del valore
  {
  switch(cursore_menu)
   {
   case 0:
    {
    if(piu) set.monofase_trifase=1; else if(meno) set.monofase_trifase=0;
    if(set.monofase_trifase) presenta_scritta((char*)&presenta_monofase_trifase,1,(char)set.lingua,2,1,0,16);
    else presenta_scritta((char*)&presenta_monofase_trifase,0,(char)set.lingua,2,1,0,16);
    } break;
   case 1:
    {
    modifica_unsigned((unsigned int*)&set.tensione_nominale,110,440);
    presenta_unsigned(set.tensione_nominale,0,1,6,3);
    } break;
   case 2:
    {
    modifica_unsigned((unsigned int*)&set.corrente_nominale,10,200);
    presenta_unsigned(set.corrente_nominale,1,1,6,3);
    } break;
   case 3:
    {
    modifica_unsigned((unsigned int*)&set.limite_sovratensione,100,125);
    presenta_unsigned(set.limite_sovratensione,0,1,6,3);
    } break;
   case 4:
    {
    modifica_unsigned((unsigned int*)&set.limite_sottotensione,60,95);
    presenta_unsigned(set.limite_sottotensione,0,1,6,3);
    } break;
   case 5:
    {
    if(set.monofase_trifase)
     {
     modifica_unsigned((unsigned int*)&set.limite_segnalazione_dissimmetria,5,20);
     presenta_unsigned(set.limite_segnalazione_dissimmetria,0,1,9,3);
     }
    else if(aumenta) cursore_menu=7; else cursore_menu=4;
    } break;
   case 6:
    {
    if(set.monofase_trifase)
     {
     modifica_unsigned((unsigned int*)&set.limite_intervento_dissimmetria,5,25);
     presenta_unsigned(set.limite_intervento_dissimmetria,0,1,9,3);
     }
    else if(aumenta) cursore_menu=7; else cursore_menu=4;
    } break;
   case 7:
    {
    modifica_unsigned((unsigned int*)&set.timeout_protezione_tensione,2,120);
    presenta_unsigned(set.timeout_protezione_tensione,0,1,10,3);
    } break;
   case 8:
    {
    modifica_unsigned((unsigned int*)&set.limite_sovracorrente,105,140);
    presenta_unsigned(set.limite_sovracorrente,0,1,10,3);
    } break;
   case 9:
    {
    if(set.monofase_trifase)
     {
     modifica_unsigned((unsigned int*)&set.limite_segnalazione_squilibrio,5,20);
     presenta_unsigned(set.limite_segnalazione_squilibrio,0,1,10,3);
     }
    else if(aumenta) cursore_menu=12; else cursore_menu=8;
    } break;
   case 10:
    {
    modifica_unsigned((unsigned int*)&set.limite_intervento_squilibrio,5,25);
    presenta_unsigned(set.limite_intervento_squilibrio,0,1,10,3);
    } break;
   case 11:
    {
    if(set.monofase_trifase)
     {
     modifica_unsigned((unsigned int*)&set.timeout_protezione_squilibrio,1,120);
     presenta_unsigned(set.timeout_protezione_squilibrio,0,1,10,3);
     }
    else if(aumenta) cursore_menu=12; else cursore_menu=8;
    } break;
   case 12:
    {
    modifica_unsigned((unsigned int*)&set.limite_corrente_differenziale,10,500);
    presenta_unsigned(set.limite_corrente_differenziale,0,1,10,3);
    } break;
   case 13:
    {
    modifica_unsigned((unsigned int*)&set.ritardo_intervento_differenziale,5,100);
    presenta_unsigned(set.ritardo_intervento_differenziale,0,1,10,3);
    } break;
   case 14:
    {
    modifica_unsigned((unsigned int*)&set.limite_intervento_pressione,40,450);
    presenta_unsigned(set.limite_intervento_pressione,1,1,9,3);
    } break;
   case 15:
    {
    modifica_unsigned((unsigned int*)&set.pressione_accensione,5,400);
    presenta_unsigned(set.pressione_accensione,1,1,9,3);
    } break;
   case 16:
    {
    modifica_unsigned((unsigned int*)&set.pressione_spegnimento,10,440);
    presenta_unsigned(set.pressione_spegnimento,1,1,9,3);
    } break;
   case 17:
    {
    modifica_unsigned((unsigned int*)&set.ritardo_funzionamento_dopo_emergenza,2,120);
    presenta_unsigned(set.ritardo_funzionamento_dopo_emergenza,0,1,10,3);
    } break;
   case 18:
    {
    modifica_unsigned((unsigned int*)&set.potenza_minima_mandata_chiusa,50,3000);
    presenta_unsigned(set.potenza_minima_mandata_chiusa,0,1,9,4);
    } break;
   case 19:
    {
    modifica_unsigned((unsigned int*)&set.ritardo_stop_mandata_chiusa,5,120);
    presenta_unsigned(set.potenza_minima_funz_secco,0,1,9,3);
    } break;
   case 20:
    {
    modifica_unsigned((unsigned int*)&set.potenza_minima_funz_secco,40,2500);
    presenta_unsigned(set.potenza_minima_funz_secco,0,1,9,4);
    } break;
   case 21:
    {
    modifica_unsigned((unsigned int*)&set.ritardo_stop_funzionemento_a_secco,5,120);
    presenta_unsigned(set.potenza_minima_funz_secco,0,1,9,3);
    } break;
   case 22:
    {
    modifica_unsigned((unsigned int*)&set.ritardo_riaccensione_a_secco,60,2000);
    presenta_unsigned(set.potenza_minima_funz_secco,0,1,9,4);
    } break;
   case 23: //abilitazione delle funzioni_avanzate
    {
    modifica_unsigned((unsigned int*)&numero_ingresso,0,999);
    presenta_unsigned(numero_ingresso,0,1,6,3);
    } break;
   case 24:
    {
    modifica_unsigned((unsigned int*)&set.costante_tau_salita_temperatura,20,600);
    presenta_unsigned(set.costante_tau_salita_temperatura,0,1,10,3);
    } break;
   case 25:
    {
    modifica_unsigned((unsigned int*)&set.portata_sensore_pressione,40,500);
    presenta_unsigned(set.portata_sensore_pressione,0,1,10,3);
    } break;
   case 26:
    {
    modifica_unsigned((unsigned int*)&set.scala_temperatura_motore,2,100);
    presenta_unsigned(set.scala_temperatura_motore,0,1,10,3);
    } break;
   case 27:
    {
    modifica_unsigned((unsigned int*)&set.taratura_temperatura_ambiente,5,40);
    if(piu | meno) set.temperatura_ambiente=DAC.temperatura_letta;
    presenta_unsigned(set.taratura_temperatura_ambiente,0,1,10,3);
    } break;
   case 28:
    {
    modifica_unsigned((unsigned int*)&set.limite_intervento_temper_motore,70,130);
    presenta_unsigned(set.limite_intervento_temper_motore,0,1,10,3);
    } break;
   case 29:
    {
    modifica_unsigned((unsigned int*)&set.scala_corrente_differenziale,1,250);
    presenta_unsigned(set.scala_corrente_differenziale,0,1,6,3);
    } break;
   case 30:
    {
    modifica_unsigned((unsigned int*)&set.numero_serie,1,65535);
    presenta_unsigned(set.numero_serie,0,1,6,5);
    } break;
   case 31:
    {
    if(piu) abilita_reset=1; else if(meno) abilita_reset=0;
    if(abilita_reset) presenta_scritta((char*)&comando_reset,0,0,1,1,0,16);
    else presenta_scritta((char*)&comando_reset,1,0,1,1,0,16);
    } break;
   }
  }
 } 
}

void misure_medie(void)
{
int delta, primo, secondo, terzo;
long prod;
char j;
asm
 {
 LDA media_potenzaI1xV1:1
 ADD media_potenzaI2xV2:1
 STA potenza_media:1
 LDA media_potenzaI1xV1
 ADC media_potenzaI2xV2
 STA potenza_media
 LDA potenza_media:1
 ADD media_potenzaI3xV3:1
 STA potenza_media:1
 LDA potenza_media
 ADC media_potenzaI3xV3
 STA potenza_media

 LDA media_reattivaI1xV23:1
 ADD media_reattivaI2xV31:1
 STA reattiva_media:1
 LDA media_reattivaI1xV23
 ADC media_reattivaI2xV31
 STA reattiva_media
 LDA reattiva_media:1
 ADD media_reattivaI3xV12:1
 STA reattiva_media:1
 LDA reattiva_media
 ADC media_reattivaI3xV12
 STA reattiva_media
  
 LDA V1_rms:1
 ADD V2_rms:1
 STA tensione_media:1
 LDA V1_rms
 ADC V2_rms
 STA tensione_media
 LDA tensione_media:1
 ADD V3_rms:1
 STA tensione_media:1
 LDA tensione_media
 ADC V3_rms
 STA tensione_media
 LDHX #3
 LDA tensione_media
 DIV
 STA tensione_media
 LDA tensione_media:1
 DIV
 STA tensione_media:1
  
 LDA I1_rms:1
 ADD I2_rms:1
 STA corrente_media:1
 LDA I1_rms
 ADC I2_rms
 STA corrente_media
 LDA corrente_media:1
 ADD I3_rms:1
 STA corrente_media:1
 LDA corrente_media
 ADC I3_rms
 STA corrente_media
 LDHX #3
 LDA corrente_media
 DIV
 STA corrente_media
 LDA corrente_media:1
 DIV
 STA corrente_media:1

//-----dissimetria---------
 LDHX V1_rms
 CPHX V2_rms
 BCS V2_maggiore_V1
 STHX secondo
 LDHX V2_rms
 STHX terzo
 BRA vedi_secondo_V3
V2_maggiore_V1:
 STHX terzo
 LDHX V2_rms
 STHX secondo

vedi_secondo_V3:
 LDHX secondo
 CPHX V3_rms
 BCS V3_maggiore
 STHX primo
 LDHX V3_rms
 STHX secondo
 BRA spareggio
V3_maggiore:
 LDHX V3_rms
 STHX primo

spareggio:
 LDHX secondo
 CPHX terzo
 BCC per_differenza
 STHX terzo

per_differenza:
 LDA primo:1
 SUB terzo:1
 STA delta:1
 LDA primo
 SBC terzo
 STA delta
 LDA delta:1
 LDX #100
 MUL
 STA prod:2
 STX prod:1
 LDA delta
 LDX #100
 MUL
 ADD prod:1
 STA prod:1
 TAX 
 ADC #0
 STA prod

 LDA #9
 STA j
ripeti:
 LDA prod:1
 SUB tensione_media:1
 TAX
 LDA prod
 SUB tensione_media
 BCS allo_scorrimento
 STA prod
 STX prod:1
allo_scorrimento:
 ROL prod:2
 ROL prod:1
 ROL prod 
 DBNZ j,ripeti
 COM prod:2
 LDA prod:2
 STA dissimmetria:1
 CLRA
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
 STA delta:1
 LDA primo
 SBC terzo
 STA delta
 LDA delta:1
 LDX #100
 MUL
 STA prod:2
 STX prod:1
 LDA delta
 LDX #100
 MUL
 ADD prod:1
 STA prod:1
 TAX 
 ADC #0
 STA prod

 LDA #9
 STA j
Ripeti:
 LDA prod:1
 SUB corrente_media:1
 TAX
 LDA prod
 SUB corrente_media
 BCS Allo_scorrimento
 STA prod
 STX prod:1
Allo_scorrimento:
 ROL prod:2
 ROL prod:1
 ROL prod 
 DBNZ j,Ripeti
 COM prod:2
 LDA prod:2
 STA squilibrio:1
 CLRA
 STA squilibrio
 }
}

void presenta_stato_motore(void)
{
int pressione, temperatura;
if(toggle_func==0)//presenta le letture
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
   if(timer_commuta_presentazione>4990)
    {
    timer_commuta_presentazione=4990;
    presenta_menu((unsigned char *)&dati_presentati_in_ON,0,4,0);
    }
   else if((timer_commuta_presentazione>2490)&&(timer_commuta_presentazione<2501))
    {
    timer_commuta_presentazione=2490;
    presenta_menu((unsigned char *)&dati_presentati_in_ON,0,4,0);
    }

   if(timer_commuta_presentazione<2500)
    {
    presenta_unsigned(tensione_media,0,0,0,3);
    presenta_unsigned(potenza_media,0,0,5,4);
    presenta_unsigned(pressione,1,0,11,3);
    presenta_unsigned(corrente_media,1,1,0,3);
    presenta_unsigned(registrazione.cosfi,2,1,5,2);
    presenta_unsigned(temperatura,0,1,11,3);
    }
   else if(registrazione.segnalazione)
    {
    presenta_unsigned(tensione_media,0,0,0,3);
    presenta_unsigned(potenza_media,0,0,5,4);
    presenta_unsigned(pressione,1,0,11,3);
    switch(registrazione.segnalazione)
     {
     case 1://motore spento
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,2,(char)set.lingua,21,1,0,16);
      } break;
     case 10://I2xT
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,3,(char)set.lingua,21,1,0,16);
      } break;
     case 11://sovratensione
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,4,(char)set.lingua,21,1,0,16);
      } break;
     case 12://sottotensione
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,5,(char)set.lingua,21,1,0,16);
      } break;
     case 13://mandata chiusa
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,6,(char)set.lingua,21,1,0,16);
      } break;
     case 14://funzionamento a secco
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,7,(char)set.lingua,21,1,0,16);
      } break;
     case 15://sovratemperatura motore
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,8,(char)set.lingua,21,1,0,16);
      } break;
     case 20://I2xT
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,12,(char)set.lingua,21,1,0,16);
      } break;
     case 21://sovratensione
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,13,(char)set.lingua,21,1,0,16);
      } break;
     case 22://sottotensione
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,14,(char)set.lingua,21,1,0,16);
      } break;
     case 23://mandata chiusa
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,15,(char)set.lingua,21,1,0,16);
      } break;
     case 24://funzionamento a secco
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,16,(char)set.lingua,21,1,0,16);
      } break;
     case 25://sovratemperatura motore
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,17,(char)set.lingua,21,1,0,16);
      } break;
     case 26://protezione differenziale
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,18,(char)set.lingua,21,1,0,16);
      } break;
     }
    } 
   else 
    {
    presenta_unsigned(tensione_media,0,0,0,3);
    presenta_unsigned(potenza_media,0,0,5,4);
    presenta_unsigned(pressione,1,0,11,3);
    presenta_unsigned(corrente_media,1,1,0,3);
    presenta_unsigned(registrazione.cosfi,2,1,5,2);
    presenta_unsigned(temperatura,0,1,11,3);
    } 
   } 
  else //alimentazione trifase
   {
   if(timer_commuta_presentazione>4990)
    {
    timer_commuta_presentazione=4990;
    presenta_menu((unsigned char *)&dati_presentati_in_ON,0,4,4);
    }
   else if((timer_commuta_presentazione>2490)&&(timer_commuta_presentazione<2501))
    {
    timer_commuta_presentazione=2490;
    presenta_menu((unsigned char *)&dati_presentati_in_ON,0,4,6);
    }

   if(timer_commuta_presentazione<2500)
    {
    presenta_unsigned(tensione_media,0,0,0,3);
    presenta_unsigned(potenza_media,0,0,5,4);
    presenta_unsigned(pressione,1,0,11,3);
    presenta_unsigned(I1_rms,1,1,0,3);
    presenta_unsigned(I2_rms,1,1,5,3);
    presenta_unsigned(I3_rms,1,1,11,3);
    }
   else if(registrazione.segnalazione)
    {
    presenta_unsigned(temperatura,0,0,0,3);
    presenta_unsigned(potenza_media,0,0,5,4);
    presenta_unsigned(pressione,1,0,11,3);
    switch(registrazione.segnalazione)
     {
     case 1://motore spento
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,2,(char)set.lingua,21,1,0,16);
      } break;
     case 10://I2xT
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,3,(char)set.lingua,21,1,0,16);
      } break;
     case 11://sovratensione
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,4,(char)set.lingua,21,1,0,16);
      } break;
     case 12://sottotensione
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,5,(char)set.lingua,21,1,0,16);
      } break;
     case 13://mandata chiusa
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,6,(char)set.lingua,21,1,0,16);
      } break;
     case 14://funzionamento a secco
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,7,(char)set.lingua,21,1,0,16);
      } break;
     case 15://sovratemperatura motore
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,8,(char)set.lingua,21,1,0,16);
      } break;
     case 17://squilibrio correnti
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,10,(char)set.lingua,21,1,0,16);
      } break;
     case 18://dissimmetria tensioni
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,11,(char)set.lingua,21,1,0,16);
      } break;
     case 20://I2xT
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,12,(char)set.lingua,21,1,0,16);
      } break;
     case 21://sovratensione
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,13,(char)set.lingua,21,1,0,16);
      } break;
     case 22://sottotensione
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,14,(char)set.lingua,21,1,0,16);
      } break;
     case 23://mandata chiusa
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,15,(char)set.lingua,21,1,0,16);
      } break;
     case 24://funzionamento a secco
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,16,(char)set.lingua,21,1,0,16);
      } break;
     case 25://sovratemperatura motore
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,17,(char)set.lingua,21,1,0,16);
      } break;
     case 26://protezione differenziale
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,18,(char)set.lingua,21,1,0,16);
      } break;
     case 27://squilibrio correnti
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,19,(char)set.lingua,21,1,0,16);
      } break;
     case 28://dissimmetria tensioni
      {
      presenta_scritta((unsigned char *)&lettura_allarmi,20,(char)set.lingua,21,1,0,16);
      } break;
     }
    } 
   else 
    {
    presenta_unsigned(temperatura,0,0,0,3);
    presenta_unsigned(potenza_media,0,0,5,4);
    presenta_unsigned(pressione,1,0,11,3);
    presenta_unsigned(cosfi[0],2,1,0,2);
    presenta_unsigned(cosfi[1],2,1,6,2);
    presenta_unsigned(cosfi[2],2,1,11,2);
    } 
   }
  asm //LED
   {
  fine:
   }
  } 
 }
}

void marcia_arresto(void)
{
if(salita_start)
 {
 salita_start=0;
 if(intervento_allarme==0)
  {
  set.motore_on=1;
  relais_alimentazione=1;
  relais_avviamento=1;
  timer_eccitazione_relais=tempo_eccitazione_relais;
  picco_corrente_avviamento=0;
  corrente_test=200;//20A
  timer_avviamento_monofase=durata_avviamento;//4 s tempo massimo di avviamento di un motore monofase
  registrazione.segnalazione=1;
  }
 }
else if(toggle_func==0)
 {
 if(salita_stop)
  {
  salita_stop=0;
  intervento_allarme=0;
  relais_alimentazione=0;
  relais_avviamento=0;
  set.motore_on=0;
  registrazione.segnalazione=0;
  }
 }  
else if((set.motore_on)&&(intervento_allarme==0)&&(prima_accensione))
 {
 prima_accensione=0;
 relais_alimentazione=1;
 relais_avviamento=1;
 timer_eccitazione_relais=tempo_eccitazione_relais;
 picco_corrente_avviamento=0;
 corrente_test=200;//20A
 timer_avviamento_monofase=durata_avviamento;
 }
}

void protezione_termica(void)//calcolo della sovra_temperatura motore con integrale I^2*T
{
char j, segno, prod[6];
asm
 {
;//---- incremento di temperatura ad ogni secondo ----
;//incremento = ((media_quad_I1+media_quad_I2+media_quad_I3) * delta_Tn *6400 / corrente_nominale^2 - delta_T)/costante_tau_salita_temperatura
 
 LDA media_quad_I1:2
 ADD media_quad_I2:2
 STA prod:2
 LDA media_quad_I1:1
 ADC media_quad_I2:1
 STA prod:1
 LDA media_quad_I1
 ADC media_quad_I2
 STA prod
 LDA media_quad_I3:2
 ADD prod:2
 STA prod:2
 LDA media_quad_I3:1
 ADC prod:1
 STA prod:1
 LDA media_quad_I3
 ADC prod
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
 LDA #$21
 STA j
ciClo_dIv:
 LDA prod:1
 SUB set.costante_tau_salita_temperatura:1
 TAX
 LDA prod
 SBC set.costante_tau_salita_temperatura
 BCS scorrim_QuoTo
 STA prod
 STX prod:1
scorrim_QuoTo:
 ROL prod:5
 ROL prod:4
 ROL prod:3
 ROL prod:2
 ROL prod:1
 ROL prod:0
 DBNZ j,ciClo_dIv 
 COM prod:5
 COM prod:4
 COM prod:3
 COM prod:2

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
 LDA set.monofase_trifase //solo monofase
 BNE fine
 LDHX timer_avviamento_monofase
 BEQ fine
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
 
fine: 
 }
}

void condizioni_di_allarme(void)
{
int I2xT, sovratensione, sottotensione, temperatura, pressione, fattore_Id;
long prod;
asm
 {
 ;sovratensione = set.tensione_nominale*set.limite_sovratensione/100;
 LDA set.tensione_nominale:1
 LDX set.limite_sovratensione:1
 MUL
 STA prod:1
 STX prod
 LDHX #100
 LDA prod
 DIV
 STA sovratensione
 LDA prod:1
 DIV
 STA sovratensione:1
  ;sottotensione = set.tensione_nominale*set.limite_sottotensione/100;
 LDA set.tensione_nominale:1
 LDX set.limite_sottotensione:1
 MUL
 STA prod:1
 STX prod
 LDHX #100
 LDA prod
 DIV
 STA sottotensione
 LDA prod:1
 DIV
 STA sottotensione:1
 
 LDHX media_pressione
 STHX pressione
 LDHX media_temperatura
 STHX temperatura
 LDHX delta_T
 STHX I2xT
 
//corrente_differenziale  
//(A*.004*5/.512* 4096/5)^2 = A^2*1024  = (A*10)^2*10.24 = (A*10)^2*6400>>16
;Id_rms = mA in = mA out * set.scala_corrente_differenziale /( .15/5*4096 *5) 
;quad_Id = mAout^2 * set.scala_corrente_differenziale^2 /5.76 /2^16
;quad_Id = mAout^2 * set.scala_corrente_differenziale^2 *4/23 /2^16
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
Id_rms=valore_efficace(media_quad_Id,fattore_Id); 
if(relais_alimentazione) //alimentazione presente
 {
 //segnalazioni ed arresto
 if(Id_rms>set.limite_corrente_differenziale)
  {
  if(timer_attesa_differenziale==0)
   {
   relais_alimentazione=0;
   relais_avviamento=0;
   intervento_allarme=registrazione.segnalazione=26;
   timer_riavviamento=set.ritardo_funzionamento_dopo_emergenza;
   }
  } 
 else
  {
  timer_attesa_differenziale=set.ritardo_funzionamento_dopo_emergenza;
  if(temperatura>set.limite_intervento_temper_motore)
   {
   relais_alimentazione=0;
   relais_avviamento=0;
   intervento_allarme=registrazione.segnalazione=25;
   timer_riavviamento=set.ritardo_funzionamento_dopo_emergenza;
   } 
  else if(I2xT>sovraccarico)
   {
   relais_alimentazione=0;
   relais_avviamento=0;
   intervento_allarme=registrazione.segnalazione=20;
   timer_riavviamento=set.ritardo_funzionamento_dopo_emergenza;
   } 
  else if(squilibrio>set.limite_intervento_squilibrio)
   {
   if(timer_attesa_squilibrio==0)
    {
    relais_alimentazione=0;
    relais_avviamento=0;
    intervento_allarme=registrazione.segnalazione=27;
    timer_riavviamento=set.ritardo_funzionamento_dopo_emergenza;
    }
   } 
  else
   {
   timer_attesa_squilibrio=set.timeout_protezione_squilibrio;
   if((V1_rms>sovratensione)||(V2_rms>sovratensione)||(V3_rms>sovratensione))
    {
    if(timer_attesa_tensione==0)
     {
     relais_alimentazione=0;
     relais_avviamento=0;
     intervento_allarme=registrazione.segnalazione=21;
     timer_riavviamento=set.ritardo_funzionamento_dopo_emergenza;
     }
    }
   else if((V1_rms<sottotensione)||(V2_rms<sottotensione)||(V3_rms<sottotensione))
    {
    if(timer_attesa_tensione==0)
     {
     relais_alimentazione=0;
     relais_avviamento=0;
     intervento_allarme=registrazione.segnalazione=22;
     timer_riavviamento=set.ritardo_funzionamento_dopo_emergenza;
     }
    }
   else if(dissimmetria>set.limite_intervento_dissimmetria)
    {
    if(timer_attesa_tensione==0)
     {
     relais_alimentazione=0;
     relais_avviamento=0;
     intervento_allarme=registrazione.segnalazione=28;
     timer_riavviamento=set.ritardo_funzionamento_dopo_emergenza;
     }
    } 
   else
    {
    timer_attesa_tensione=set.timeout_protezione_tensione;
    if((potenza_media<set.potenza_minima_mandata_chiusa)&&(pressione>set.pressione_accensione+3))//soglia di 3/10 di Bar
     {
     if(timer_mandata_chiusa==0)
      {
      relais_alimentazione=0;
      relais_avviamento=0;
      intervento_allarme=registrazione.segnalazione=23;
      timer_riavviamento=0;
      }
     } 
    else
     {
     timer_mandata_chiusa=set.ritardo_stop_mandata_chiusa;
     if((potenza_media<set.potenza_minima_funz_secco)&&(pressione<set.pressione_accensione))
      {
      if(timer_attesa_secco==0)
       {
       relais_alimentazione=0;
       relais_avviamento=0;
       intervento_allarme=registrazione.segnalazione=24;
       timer_riavviamento=set.ritardo_riaccensione_a_secco;
       }
      } 
     else timer_attesa_secco=set.ritardo_stop_funzionemento_a_secco;
     }
    } 
   } 
  }

 //segnalazioni senza arresto
 if(intervento_allarme==0)
  {
  if(temperatura>set.limite_intervento_temper_motore-5) registrazione.segnalazione=15;//segnalazione con 5°C in anticipo
  else if(I2xT>sovraccarico_moderato) registrazione.segnalazione=10;
  else if(squilibrio>set.limite_intervento_squilibrio) registrazione.segnalazione=17;
  else if((V1_rms>sovratensione)||(V2_rms>sovratensione)||(V3_rms>sovratensione)) registrazione.segnalazione=11;
  else if((V1_rms<sottotensione)||(V2_rms<sottotensione)||(V3_rms<sottotensione)) registrazione.segnalazione=12;
  else if((potenza_media<set.potenza_minima_mandata_chiusa)&&(pressione>set.pressione_accensione+3)) registrazione.segnalazione=13;//soglia di 3/10 di Bar
  else if((potenza_media<set.potenza_minima_funz_secco)&&(pressione<set.pressione_accensione)) registrazione.segnalazione=14;
  else if(dissimmetria>set.limite_intervento_dissimmetria) registrazione.segnalazione=18;
  } 
 }
else //relais_alimentazione==0
if(set.motore_on) //alimentazione assente
 {
 if(timer_riavviamento==0)
  {
  if((Id_rms<set.limite_corrente_differenziale)
   &&(temperatura<set.limite_intervento_temper_motore-10)&&(I2xT<delta_Tn)&&(dissimmetria<5)
   &&(V1_rms<sovratensione)&&(V2_rms<sovratensione)&&(V3_rms<sovratensione)
   &&(V1_rms>sottotensione)&&(V2_rms>sottotensione)&&(V3_rms>sottotensione)
   &&(pressione<set.pressione_accensione))
   {
   intervento_allarme=0;
   relais_alimentazione=1;
   relais_avviamento=1;
   timer_eccitazione_relais=tempo_eccitazione_relais;
   picco_corrente_avviamento=0;
   corrente_test=200;//20A
   timer_avviamento_monofase=durata_avviamento;//4 s tempo massimo di avviamento di un motore monofase
   registrazione.segnalazione=1;
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
ADCCFG=0x05;
ADCCVL=0xff;
ADCCVL=0xff;
//fantuzADCSC1=0x40;

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
//fantuzSPI1C1=0xd0;//interruzione abilitata
SPI1C2=0x00;
SPI1BR=0x43;
 
//-------uscite LED e PWM--------------------
PTFDD=0;//uscite LED e relè
PTFD=0;
TPM1C2SC=0x08;//uscite relè
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
TPM2C1V=24;//2us

TPM1CNT=0;
TPM2CNT=0;
//fantuzTPM1SC=0x68;
//fantuzTPM2SC=0x68;

//-----portG  per display  G2 = RSneg, G3 = E----
PTGDD=0x0c;
PTGD=0;

//--------inizializzazione delle variabili-------------
offset_I1letta=0x08000000; //valor medio ipotetico
offset_I3letta=0x08000000;
offset_V12letta=0x08000000;
offset_V13letta=0x08000000;
offset_Idletta=0x08000000;
precedente_lettura_allarme=allarme_in_lettura=3000;
pronto_alla_risposta=0;
leggi_impostazioni=1;
salva_conta_secondi=0;
leggi_conta_secondi=1;
timer_20ms=0;
timer_1s=0;
timer_10min=0;
timer_rilascio=0;
timer_inc_dec=0;
timer_piu_meno=0;
timer_lampeggio=0;
timer_commuta_presentazione=0;
timer_aggiorna_misure=0;
timer_avviamento_monofase=0;
timer_attesa_squilibrio=0;
timer_attesa_tensione=0;
timer_mandata_chiusa=0;
timer_attesa_secco=0;
timer_attesa_differenziale=0;
timer_riavviamento=0;
timer_eccitazione_relais=0;
PWM_relais=0;
quad_Id=0;
somma_quad_I1=0;
somma_quad_I2=0;
somma_quad_I3=0;
somma_quad_V1=0;
somma_quad_V2=0;
somma_quad_V3=0;
somma_potenzaI1xV1=0;
somma_potenzaI2xV2=0;
somma_potenzaI3xV3=0;
somma_reattivaI1xV23=0;
somma_reattivaI2xV31=0;
somma_reattivaI3xV12=0;
media_temperatura=0;
media_pressione=0;
quad_I1=0;
quad_I2=0;
quad_I3=0;
quad_V1=0;
quad_V2=0;
quad_V3=0;
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
media_quad_V1=0;
media_quad_V2=0;
media_quad_V3=0;
media_potenzaI1xV1=0;
media_potenzaI2xV2=0;
media_potenzaI3xV3=0;
media_reattivaI1xV23=0;
media_reattivaI2xV31=0;
media_reattivaI3xV12=0;
delta_T=0;
I1_rms=0;
I2_rms=0;
I3_rms=0;
V1_rms=0;
V2_rms=0;
V3_rms=0;
attesa_invio=0;
intervento_allarme=0;
relais_alimentazione=0;
relais_avviamento=0;
contatore_display=0;
contatore_AD=0;
cursore_menu=0;
funzioni_avanzate=0;
numero_ingresso=0;
abilita_reset=0;
potenza_media=0;
reattiva_media=0;
tensione_media=0;
corrente_media=0;
picco_corrente_avviamento=0;
corrente_test=200;//20A
dissimmetria=0;
squilibrio=0;
prima_accensione=1;
comando_display=1;
timer_reset_display=100;
timer_rinfresco_display=30;
precedente_segnalazione=registrazione.segnalazione;
indirizzo_scrivi_eeprom=ultimo_indirizzo_scrittura=indirizzo_conta_secondi_attivita;

asm
 { 
 CLI//abilita interrupt
 }
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
remoto=0;
salita_remoto=0;
timer_rilascio=0;
reset_default=0;
}

/***********************************************************************/

void main(void)
{
condizioni_iniziali();
SOPT1_COPT=0;
  
  _irq_restore(0);
  
USB_comm_init();

for(;;)  /* loop forever */
 {
 __RESET_WATCHDOG(); /* feeds the dog */
/* leggi_conta_secondi_attivita();
 lettura_impostazioni();
// salva_impostazioni();

 lettura_allarme(); //con allarme_in_lettura <335
 trasmissione_misure_istantanee(); //con allarme_in_lettura = 1000
 trasmissione_tarature(); //con allarme_in_lettura == 2000

// scrittura_allarme_in_eeprom();
 salva_conta_secondi_attivita();
 programmazione();
 funzione_reset_default();
 misure_medie();
// presenta_stato_motore();
 marcia_arresto();
// condizioni_di_allarme();  */
 
 USB_comm_process();
 cdc_process();
 }
}
