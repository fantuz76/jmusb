//timer e contatore ore di funzionamento (salvataggio dell'ora ogni 1 minuto)
//Lettura di correnti, tensioni, sensore_pressione, temperatura_motore, relè differenziale, tensione_logica
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
// 16 
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
// 26 
// 27 squilibrio di corrente
// 28 dissimmetria delle tensioni
// 29 allarme sensore pressione
// 30 pressione emergenza
// 31 protezione Icc
//ogni valore diverso non viene considerato
 
//funzioni:
//col pulsante FUNC si leggono le funzioni in successione crescente
//coi pulsanti + e - si cambia il valore
// i comandi ON OFF vengono memorizzati e ripresi al ritorno della tensione

// presentazioni con motore in ON:
// tensione, potenza, pressione, corrente1, corrente2, corrente3,
// alternata con:
// temperatura, potenza, pressione, cosfi1, cosfi2, cosfi3,

// aggiunta del totalizzatore dell'energia in data 22-06-2011
// salvataggio e reset eeprom piu rapido in data 22-06-2011

const int
operazione_effettuata=11000,
lettura_tarature=10000,//da dare alla variabile allarme_in_lettura per la lettura delle tarature
lettura_satellitare=9000;//da dare alla variabile allarme_in_lettura per la lettura delle misure istantanee
const char
byte_tabella=127,//128-1
segnalazioni_senza_intervento=20,
N_tentativi_motore_bloccato=5;

__interrupt void AD_conversion_completeISR(void);
__interrupt void timer2_captureISR(void);
__interrupt void timer1_overflowISR(void); //timer 1ms
__interrupt void SPI1(void);
__interrupt void usb_it_handler(void);


void InviaSatellitare(void);
void calcolo_istante(void);
void calcolo_tensione_relay(void);
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
void presenta_PF(unsigned char val, char riga, char colonna);
void presenta_energia(unsigned long val, char punto, char riga, char colonna, char n_cifre);
void presenta_signed(int val, char punto, char riga, char colonna, char n_cifre);
void presenta_scritta(char *stringa, int offset, char idioma, char lunghezza, char riga,char colonna, char N_caratteri);
void calcolo_delle_costanti(void);
void sequenza_lettura_scrittura_ram(void);
void leggi_data_ora(void);
void presenta_data_ora(void);
void registra_nuova_data(char dato, char indirizzo);
void modifica_carattere(char *var, char min, char max);
void presenta_2_cifre(char val, char riga, char colonna);
void modifica_data_ora(void);
void programmazione(void);
void messaggio_allarme(char indentificazione);
void misure_medie(void);
void Nallarme_ora_minuto_secondo(long secondi, int conta_allarmi, char numero);
void presenta_stato_motore(void);
void protezione_termica(void);
void disinserzione_condensatore_avviamento(void);
void condizioni_di_allarme(void);
void marcia_arresto(void);
void trasmissione_misure_istantanee(void); //con allarme_in_lettura = 9000
void trasmissione_tarature(void);//con allarme_in_lettura == 10000
void misura_della_portata(void);

void salva_reset_default(void);
void presenta_cursore_eeprom(long var);
void salva_impostazioni(void);
void salva_segnalazione_fault(void);
void prepara_per_salvataggio_allarme(void);
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

extern const unsigned char
presentazione_iniziale[16][17],
abilitazione_OFF[17],
accesso_password[17],
dati_presentati_in_ON[4][2][17],
lettura_allarmi[25][17],
presenta_tipo_start_stop[2][17],
attivazione_comando_reset[2][17],
in_salvataggio[17],
menu_principale[32][17],//Menu' principale: elenco delle funzioni di taratura
dati_motore[4][17],
protezione_voltaggio[14][17],
protezione_corrente[12][17],
controllo_pressione[18][17],
controllo_potenza[12][17],
sensore_di_flusso[8][17],
sensore_temperatura[10][17],
contatore_energia[2][17],
contatore_litri[2][17],
presenta_data[2][17],
calibrazione[6][17];

const char
comando_AD[20]={
0x40, //I1letta   //0,2,1,3
0x42, //V31letta
0x41, //I3letta
0x43, //V32letta
0x44, //sensore_pressione
0x43, //V32letta  //3,1,2,0
0x41, //I3letta
0x42, //V31letta
0x40, //I1letta
0x45, //sensore_PT100
0x40, //I1letta   //0,3,1,2
0x43, //V32letta
0x41, //I3letta
0x42, //V31letta
0x46, //caduta_cavo_PT100
0x42, //V31letta  //2,1,3,0
0x41, //I3letta
0x43, //V32letta
0x40, //I1letta
0x47},//tensione_15V
N_letture_AD=20, N_letture_AD_men1=19,
N_parametri=64,
N_bytes_messaggio_fault=20,
filtro_pulsanti=21,
prepara[7]={0x01,0x02,0x06,0x0c,0x14,0x3c,0x01},
limite_segnalazioni=32;

const long
primo_indirizzo_funzioni=0x1ff0000;//511*256 * 256,

const int
delta_Tn=80, //sovra_temperatura limite con corrente nominale = 80°C
limitato_delta_T=100,//gradi
massimo_delta_T=120,//gradi
delta_T_riavviamento=60, //°C
fattore_PT100=18016,
fattore_portata_sensore_pressione=1966,//16*.15/5*4096 DAC portata
durata_avviamento=1000, inizio_lettura_I_avviamento=950,
durata_cc=3000,
tempo_eccitazione_relay=500,//.5s
chiave_ingresso=11, chiave_ingresso2=541,
totale_indicazioni_fault=6539,//totale delle registrazioni di 20 bytes salvate in 129280 bytes
disturbo_I_pos=54,
disturbo_I_neg=-54,
emergenza_sensore_pressione=-20,//Bar*10
tot_righe_menu=32;

extern const unsigned int
tabella_potenza_nominale[15],//KW*100

tabella_ritardo_protezione_squilibrio[3],//s
tabella_ritardo_protezione_tensione[3],//s
tabella_ritardo_riaccensione_da_emergenza_V[3],//minuti
tabella_ritardo_riaccensione_da_emergenza_I[3],//minuti
tabella_ritardo_stop_mandata_chiusa[3],//s
tabella_ritardo_stop_funzionamento_a_secco[3],//s
tabella_ritardo_riaccensione_mandata_chiusa[3],//s
tabella_ritardo_riaccensione_funzionamento_a_secco[3],//minuti

tabella_timer_ritorno_da_emergenza_sensore[3],//s
tabella_portata_sensore_pressione[3],//Bar*10
tabella_corrente_minima_sensore[3],//mA*10
tabella_corrente_massima_sensore[3],//mA*10
tabella_scala_sensore_di_flusso[3],//litri*1000/impulso
tabella_tipo_sonda_PT100[3],//numero dei fili
tabella_resistenza_PT100_a_0gradi[3],//Ohm*10
tabella_resistenza_PT100_a_100gradi[3],//Ohm*10
tabella_limite_intervento_temper_motore[3],//°C
tabella_pressione_emergenza[3],//Bar*10
tabella_pressione_spegnimento[3],//Bar*10
tabella_pressione_accensione[3],//Bar*10
tabella_limite_minimum_flow[3],//litri/minuto
tabella_limite_maximum_flow[3],//litri/minuto
tabella_potenza_minima_mandata_chiusa[3],//%
tabella_potenza_minima_funz_secco[3],//%
tabella_K_di_tempo_riscaldamento[3],//s

tabella_calibrazione[3],//dei sensori di corrente

tabella_limite_segnalazione_dissimmetria[3][9],//%
tabella_limite_intervento_dissimmetria[3][9],//%
tabella_limite_segnalazione_squilibrio[3][9],//%
tabella_limite_intervento_squilibrio[3][9],//%

tabella_tensione_nominale[3][15],//V
tabella_corrente_nominale[3][15],//A*10
tabella_limite_sovratensione[3][15],//%
tabella_limite_sottotensione[3][15],//%
tabella_tensione_restart[3][15],//%
tabella_limite_sovracorrente[3][15];//%
   
struct
{
int //dai di funzionamento a partire dall'indirizzo 8040  della eeprom
numero_serie,//1-65535

N_tabella_potenza,//trifase .37 - 5.5KW, monofase .37 - 2.2 KW

potenza_nominale,// W*10

ritardo_protezione_squilibrio,// s
ritardo_protezione_tensione,// s
ritardo_riaccensione_da_emergenza_V,//s
ritardo_riaccensione_da_emergenza_I,//s
ritardo_stop_mandata_chiusa, //s
ritardo_stop_funzionamento_a_secco, //s
ritardo_riaccensione_mandata_chiusa, //s
ritardo_riaccensione_funzionamento_a_secco, //min

timer_ritorno_da_emergenza_sensore,//s
portata_sensore_pressione,// BAR*10
corrente_minima_sensore,//mA*10
corrente_massima_sensore,//mA*10
scala_sensore_di_flusso,//litri*1000/impulso
tipo_sonda_PT100,//fili
resistenza_PT100_a_0gradi,//Ohm*10
resistenza_PT100_a_100gradi,//Ohm*10
limite_intervento_temper_motore,//°C
pressione_emergenza,// Bar*10
pressione_spegnimento,// BAR*10
pressione_accensione,// BAR*10
limite_minimum_flow,//litri/min
limite_maximum_flow,//litri/min
potenza_minima_mandata_chiusa,// %
potenza_minima_funz_secco,// %
K_di_tempo_riscaldamento,//s

calibrazione_I1,
calibrazione_I2,    
calibrazione_I3,    

limite_segnalazione_dissimmetria,// %
limite_intervento_dissimmetria,// %
limite_segnalazione_squilibrio,// %
limite_intervento_squilibrio,// %

tensione_nominale,// V
corrente_nominale,// A*10
limite_sovratensione,// %
limite_sottotensione,// %
tensione_restart,// %
limite_sovracorrente,// %

abilita_sensore_pressione,//0-1
abilita_sensore_flusso,//0-1
abilita_sensore_temperatura,//0-1
modo_start_stop,//0=remoto o 1=pressione

motore_on,//45 //0-1
numero_segnalazione,//46 //0-6539
conta_ore[2],//47
conta_ore_funzionamento[2],//49 //s
energia[2],//51 //KWh
conta_litri_funzionamento[2],//53
riserva[9];//55
}set;

struct
{
int //ingressi analogici
I1letta,//A*.004*5/.512* 4096/5 = A*32
I3letta,//A*.004*5/.512* 4096/5 = A*32
V31letta,//V* 33/680/6 /2 *4096/5 = V*3.313
V32letta,//V* 33/680/6 /2 *4096/5 = V*3.313
sensore_pressione,//V*4096/5 = mA*.15*4096/5 = mA*122.88
lettura_PT100,
caduta_cavo_PT100,
tensione_15V; //V*4.7/(33+4.7) *4096/5 = V*102
}DAC0;

struct
{
int //ingressi analogici
I1letta,//A*.004*5/.512* 4096/5 = A*32
I3letta,//A*.004*5/.512* 4096/5 = A*32
V31letta,//V* 33/680/6 /2 *4096/5 = V*3.313
V32letta,//V* 33/680/6 /2 *4096/5 = V*3.313
sensore_pressione,//V*4096/5 = mA*.15*4096/5 = mA*122.88
lettura_PT100,
caduta_cavo_PT100,
tensione_15V; //V*4.7/(33+4.7) *4096/5 = V*102
}DAC;

char
mono_tri_fase,
conta_data,
data_letta,
modificato,
Giorno, Mese, Anno, Ora, Minuto, Secondo,
giorno[2], mese[2], anno[2], ora[2], minuto[2], secondo[2];

char //memoria con orologio
indirizzo_ram,
contatore_ram,
dato_ram,
leggi_ram,
scrivi_ram;

char
display[2][17],
lampeggio_da, N_cifre_lampeggianti,
cifre[16], 
buffer_USB[128],//contiene i 24 bytes degli errori
cursore_menu, cursore_sottomenu,
comando_display;

unsigned char
impulso_flusso,
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
tot_misure,
Tot_misure,
contatore_display,
contatore_AD;

unsigned char
start, enter, 
salita_start, salita_enter, toggle_start, toggle_enter,
stop, esc,
salita_stop, salita_esc, 
func,
salita_func, toggle_func,
meno,
salita_meno,
piu,
salita_piu, 
remoto,
salita_remoto,
sequenza_fasi,
segno_V12,
segno_V13,
anticipo_V12_su_V13,
attesa_invio,
prima_segnalazione,
alternanza_presentazione,
relay_alimentazione,
relay_avviamento,
tentativi_avviamento,
tentativi_avviamento_a_secco;

int 
I1,//A*.004*5/.512* 4096/5 = A*32
I2,
I3,//A*.004*5/.512* 4096/5 = A*32
V31,//V* 33/680/6 /2 *4096/5 = V*3.313
V32,//V* 33/680/6 /2 *4096/5 = V*3.313
V1, V2, V3;//V*3.313 * 3

unsigned int //timer
timer_presenta_data,
timer_1_ora,
timer_ritorno_da_emergenza_V,
timer_ritorno_da_emergenza_I,
timer_ritorno_da_emergenza_funzionamento_a_secco,
timer_lampeggio_LED_emergenza,
timer_attesa_segnalazione_fault,
timer_eccitazione_relay,
timer_commuta_presentazione,
timer_aggiorna_misure,
timer_aggiorna_menu,
timer_avviamento_monofase,
timer_allarme_avviamento,
timer_inc_dec,
timer_piu_meno,
timer_flusso, timer_flusso_nullo;

unsigned long //offset delle letture
offset_I1letta,
offset_I3letta,
offset_V31letta,
offset_V32letta;

long //letture sommate
timer_riavviamento,
incremento_volume,//litri*65536/impulso
conta_secondi, conta_secondi_attivita,
istante,//s dall'orologio
delta_T,//sovra_temperatura calcolata
somma_quad_I1,
somma_quad_I2,
somma_quad_I3,
somma_quad_V12,
somma_quad_V23,
somma_quad_V31,
Somma_quad_I1,
Somma_quad_I2,
Somma_quad_I3,
Somma_quad_V12,
Somma_quad_V23,
Somma_quad_V31,
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
somma_energia_attiva,//J*50
quad_I1,
quad_I2,
quad_I3,
quad_V12,
quad_V23,
quad_V31;

long //valori medi
media_temperatura,//°C
media_pressione,//Bar*10
media_flusso,//litri/minuto
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
soglia_tensione, soglia_tensione_neg,//V*3.313
potenzaI1xV13,//W
potenzaI1xV1,//W
potenzaI2xV2,
potenzaI3xV3,
reattivaI1xV23,//VAR
reattivaI2xV31,
reattivaI3xV12,
conta_litri[3],
misura_energia[3],

delta_PT100_0_100gradi,
flusso,//litri/minuto
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
indirizzo_scrivi_eeprom,//indirizza il byte in fase di scrittura
ultimo_indirizzo_scrittura,//0-129280 in bytes
primo_indirizzo_lettura;//in bytes  mettere primo_indirizzo<ultimo_indirizzo e leggi_eeprom=1;

unsigned int //eeprom
ultimo_dato_in_lettura,//in bytes
buffer_eeprom[128];

unsigned char //eeprom
buffer_fault[20], primo_elemento_buffer,
contatore_scrivi_eeprom,
lunghezza_salvataggio,//numero bytes da salvare 0-255
contatore_leggi_eeprom,//indirizza il byte in fase di lettura
scrivi_eeprom,
leggi_eeprom,
eeprom_impegnata,
reset_default, 
prepara_chiave,//0-1

segnalazione_, precedente_segnalazione, 
comando_reset,
salvataggio_funzioni, //!=0 solo durante il salvataggio delle funzioni o reset eeprom
salvataggio_allarme,
salva_conta_secondi,

leggi_impostazioni;

unsigned int
fattore_I2xT,
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
int V12, V21, V23, V13;
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
 CMP N_letture_AD_men1
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
calcolo_offset_misure((unsigned long*)&offset_V31letta, DAC.V31letta);
calcolo_offset_misure((unsigned long*)&offset_V32letta, DAC.V32letta);
asm
 { 
 LDA offset_I1letta:1
 SUB DAC.I1letta:1
 STA I1:1
 LDA offset_I1letta
 SBC DAC.I1letta
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
 LDA offset_I3letta:1
 SUB DAC.I3letta:1
 STA I3:1
 LDA offset_I3letta
 SBC DAC.I3letta
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
 
 LDA DAC.V32letta:1
 SUB offset_V32letta:1
 STA V32:1
 LDA DAC.V32letta
 SBC offset_V32letta
 STA V32

 LDA DAC.V31letta:1
 SUB offset_V31letta:1
 STA V31:1
 LDA DAC.V31letta
 SBC offset_V31letta
 STA V31

//tensioni di fase 
;V13=-V31
;V23=-V32;
;V12=V13-V23;
;V21=-V12

;V1 = V12+V13  
;V2 = V21+V23
;V3 = V31+V32

 CLRA
 SUB V31:1
 STA V13:1
 CLRA
 SBC V31
 STA V13

 CLRA
 SUB V32:1
 STA V23:1
 CLRA
 SBC V32
 STA V23

 LDA V13:1
 SUB V23:1
 STA V12:1
 LDA V13
 SBC V23
 STA V12

 CLRA
 SUB V12:1
 STA V21:1
 CLRA
 SBC V12
 STA V21

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

 LDA segno_V12
 BMI commuta_a_positivi
 CMP #1
 BEQ vedi_salita_V13
 
 LDHX V12
 BPL per_somme_quad
 CPHX soglia_tensione_neg
 BCC per_somme_quad
 LDA #-1
 STA segno_V12
 BRA per_somme_quad
commuta_a_positivi: 
 LDHX V12
 BMI per_somme_quad
 CPHX soglia_tensione
 BCS per_somme_quad
 LDA #1
 STA segno_V12
 CLRA
 STA segno_V13
 
vedi_salita_V13:
 LDA anticipo_V12_su_V13
 INCA
 STA anticipo_V12_su_V13

 LDA segno_V13
 BMI Commuta_a_positivi
 CMP #1
 BEQ per_somme_quad
 
 LDHX V13
 BPL per_somme_quad
 CPHX soglia_tensione_neg
 BCC per_somme_quad
 LDA #-1
 STA segno_V13
 BRA per_somme_quad
Commuta_a_positivi: 
 LDHX V13
 BMI per_somme_quad
 CPHX soglia_tensione
 BCS per_somme_quad
 LDA #1
 STA segno_V13
 CLRA
 STA segno_V12

 LDA anticipo_V12_su_V13
 CMP #24
 BCS assegna_0_sequenza_fasi
 LDA #1
 STA sequenza_fasi
 CLRA
 STA anticipo_V12_su_V13
 BRA per_somme_quad
assegna_0_sequenza_fasi:
 CLRA 
 STA sequenza_fasi
 STA anticipo_V12_su_V13
 
per_somme_quad:
 }
somma_quadrati((unsigned long*)&somma_quad_I1,I1);
somma_quadrati((unsigned long*)&somma_quad_I3,I3);
somma_quadrati((unsigned long*)&somma_quad_V31,V31);
somma_potenze((long*)&somma_potenzaI1xV13,I1,V13);
if(mono_tri_fase) //trifase
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
 CPX N_letture_AD
 BCS per_comando_conversione
 CLRX
per_comando_conversione:
 STX contatore_AD
 CLRH
 LDA @comando_AD,X
 STA ADCSC1
fine: 
 }
}

void lettura_pulsanti(void)
{
asm
 {
 BRSET 0,_PTAD,azzera_func
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
 LDA start
 CMP #20
 BNE start_1
 LDA #1
 STA salita_start
 STA salita_enter
start_1:
 LDA start
 CMP filtro_pulsanti
 BCC vedi_stop
 INCA
 STA start
 STA enter
 BRA vedi_stop
azzera_start:
 CLRA
 STA start
 STA enter

vedi_stop:
 BRSET 4,_PTAD,azzera_stop
 LDA stop
 CMP #20
 BNE stop_1
 LDA #1
 STA salita_stop
 STA salita_esc
stop_1: 
 LDA stop
 CMP filtro_pulsanti
 BCC vedi_remoto
 INCA
 STA stop
 STA esc
 BRA vedi_remoto
azzera_stop:
 CLRA
 STA stop
 STA esc

vedi_remoto:
 BRCLR 5,_PTAD,azzera_remoto
 LDA remoto
 CMP #20
 BNE remoto_1
 LDA #1
 STA salita_remoto
remoto_1:
 LDA remoto
 CMP filtro_pulsanti
 BCC fine
 INCA
 STA remoto
 BRA fine
azzera_remoto:
 CLRA
 STA remoto

fine:
 }   
}

void comando_del_display(void)
{
char posizione_carattere, Display;
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
 JSR presenta_data_ora
 BRA fine

per_comando_display: 
 LDA comando_display
 BEQ scrittura
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

scrittura:
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
 DECA
 DECA
 LSRA
 STA posizione_carattere

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
 LDA N_cifre_lampeggianti
 BEQ presenta_carattere
 LDA lampeggio_da
 CMP #16
 BCC presenta_carattere
 LDA posizione_carattere
 CMP lampeggio_da
 BCS presenta_carattere
 LDA lampeggio_da
 ADD N_cifre_lampeggianti
 CMP posizione_carattere
 BCS presenta_carattere
 LDA timer_lampeggio
 CMP #3
 BCC presenta_carattere
 LDA #' '
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
 LDA N_cifre_lampeggianti
 BEQ presenta_carattere
 LDA lampeggio_da
 CMP #17
 BCS presenta_carattere
 CMP #33
 BCC presenta_carattere
 LDA posizione_carattere
 CMP lampeggio_da
 BCS presenta_carattere
 LDA lampeggio_da
 ADD N_cifre_lampeggianti
 CMP posizione_carattere
 BCS presenta_carattere
 LDA timer_lampeggio
 CMP #3
 BCC presenta_carattere
 LDA #' '
 STA Display
 BRA increm_contatore

presenta_carattere: 
 LDX posizione_carattere
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

__interrupt  void timer2_captureISR(void)
{
asm
 {
 SEI  /*disabilita interrupt*/
 LDA _TPM2C0SC
 BCLR 7,_TPM2C0SC
 LDHX _TPM2C0V//sensore di flusso
 STHX timer_flusso
 LDHX #650
 STHX timer_flusso_nullo
 LDA #1
 STA impulso_flusso
 }
}

const int
alimentazione8V=38298;//8/(33+4.7)*4.7 /5 *4096 *12000/256
void calcolo_tensione_relay(void)
{
char j;
int PWM_relay_8V, PWM_relay_12V;
long numeratore;
asm
 {
 CLR numeratore
 LDHX alimentazione8V
 STHX numeratore:1

// PWM_relay_8V = alimentazione8V/DAC.tensione_15V
 LDA #$11
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
 STHX PWM_relay_8V
// PWM_relay_12V = alimentazione8V*3/2;
 LSR numeratore:2
 ROR numeratore:3
 LDA numeratore:3
 LDX #3
 MUL
 STA PWM_relay_12V:1
 STX PWM_relay_12V
 LDA numeratore:2
 LDX #3
 MUL
 ADD PWM_relay_12V
 STA PWM_relay_12V
 
 LDA relay_avviamento
 BEQ spegni_relay_avviam
 LDHX PWM_relay_12V
 STHX _TPM1C3V//relè di avviam acceso
 BRA per_timer_eccitazione_relay_linea
spegni_relay_avviam:
 LDHX #-1
 STHX _TPM1C3V//relè di avviam spento
 
per_timer_eccitazione_relay_linea: 
 LDA relay_alimentazione
 BEQ spegni_relay_alim
 LDHX timer_eccitazione_relay
 BEQ eccitazione_ridotta_relay_linea
 LDA timer_eccitazione_relay:1
 SUB #1
 STA timer_eccitazione_relay:1
 LDA timer_eccitazione_relay
 SBC #0
 STA timer_eccitazione_relay
 LDHX PWM_relay_12V
 STHX _TPM1C2V//relè di marcia acceso
 LDHX #$7fff
 STHX _TPM1C5V//Led presenza tensione
 BRA fine
eccitazione_ridotta_relay_linea:
 LDHX PWM_relay_8V
 STHX _TPM1C2V//relè di marcia ridotto
 LDHX #$7fff
 STHX _TPM1C5V//Led presenza tensione
 BRA fine
spegni_relay_alim:
 LDHX #-1
 STHX _TPM1C2V//relè di marcia spento
 STHX _TPM1C5V//Led presenza tensione
fine: 
 }
}

__interrupt void timer1_overflowISR(void) //timer 1 ms
{
long delta;
int temperatura, pressione, attiva, reattiva;
char j, segno;
asm
 {
 SEI  /*disabilita interrupt*/
 LDA _TPM1SC
 BCLR 7,_TPM1SC

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

per_timer_1min:
 LDA timer_1min
 CMP #59
 BCC per_reset_timer_1min
 INCA
 STA timer_1min
 BRA per_calcolo_istante
per_reset_timer_1min: 
 CLRA
 STA timer_1min
 LDA #1
 STA salva_conta_secondi

per_calcolo_istante:
//per ogni secondo
 JSR calcolo_istante
 
 JSR InviaSatellitare

//calocolo dell'energia
 LDA relay_alimentazione
 BEQ per_protezione_termica
 LDA somma_energia_attiva
 BMI per_protezione_termica
 LDA misura_energia:5
 ADD somma_energia_attiva:3
 STA misura_energia:5
 LDA misura_energia:4
 ADC somma_energia_attiva:2
 STA misura_energia:4
 LDA misura_energia:3
 ADC somma_energia_attiva:1
 STA misura_energia:3
 LDA misura_energia:2
 ADC somma_energia_attiva
 STA misura_energia:2
 LDA misura_energia:1
 ADC #0
 STA misura_energia:1
 LDA misura_energia
 ADC #0
 STA misura_energia

per_protezione_termica: 
 LDHX #0
 STHX somma_energia_attiva
 STHX somma_energia_attiva:2
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
//non in funzionamento a secco
 INCA
 STA tentativi_avviamento
 
per_timer_riavviamento: 
 LDHX timer_riavviamento
 BNE per_dec_timer_riavviamento
 LDHX timer_riavviamento:2
 BEQ per_timer_attesa_secco
per_dec_timer_riavviamento: 
 LDA timer_riavviamento:3
 SUB #1
 STA timer_riavviamento:3
 LDA timer_riavviamento:2
 SBC #0
 STA timer_riavviamento:2
 LDA timer_riavviamento:1
 SBC #0
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
 LDA relay_alimentazione
 BEQ per_timer_1ms
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

per_timer_1ms:
//per ogni ms 
 LDHX #0
 STHX temperatura
 STHX pressione;
 }

switch(timer_20ms)
 {
 case 0:
  {
  asm
   {
   SEI
   }
  Tot_misure=tot_misure;
  Somma_quad_I1=somma_quad_I1;
  Somma_quad_I2=somma_quad_I2;
  Somma_quad_I3=somma_quad_I3;
  Somma_quad_V12=somma_quad_V12;
  Somma_quad_V23=somma_quad_V23;
  Somma_quad_V31=somma_quad_V31;
  Somma_potenzaI1xV1=somma_potenzaI1xV1;
  Somma_potenzaI2xV2=somma_potenzaI2xV2;
  Somma_potenzaI3xV3=somma_potenzaI3xV3;
  Somma_reattivaI1xV23=somma_reattivaI1xV23;
  Somma_reattivaI2xV31=somma_reattivaI2xV31;
  Somma_reattivaI3xV12=somma_reattivaI3xV12;
  Somma_potenzaI1xV13=somma_potenzaI1xV13;
   
  tot_misure=0;
  somma_quad_I1=0;
  somma_quad_I2=0;
  somma_quad_I3=0;
  somma_quad_V12=0;
  somma_quad_V23=0;
  somma_quad_V31=0;
  somma_potenzaI1xV1=0;
  somma_potenzaI2xV2=0;
  somma_potenzaI3xV3=0;
  somma_reattivaI1xV23=0;
  somma_reattivaI2xV31=0;
  somma_reattivaI3xV12=0;
  somma_potenzaI1xV13=0;
  asm
   {
   CLI
   }
  media_quadrati_nel_periodo((unsigned long*)&quad_I1,Somma_quad_I1,Tot_misure);
  media_quadrati_nel_periodo((unsigned long*)&quad_I3,Somma_quad_I3,Tot_misure);
  if(mono_tri_fase)//trifase
   {
   media_quadrati_nel_periodo((unsigned long*)&quad_I2,Somma_quad_I2,Tot_misure);
   media_quadrati_nel_periodo((unsigned long*)&quad_V12,Somma_quad_V12,Tot_misure);
   media_quadrati_nel_periodo((unsigned long*)&quad_V23,Somma_quad_V23,Tot_misure);
   media_quadrati_nel_periodo((unsigned long*)&quad_V31,Somma_quad_V31,Tot_misure);
   }
  else media_quadrati_nel_periodo((unsigned long*)&quad_V31,Somma_quad_V31,Tot_misure);
  } break;
 case 1:
  {
  if(mono_tri_fase)//trifase
   {
   potenzaI1xV1=media_potenza_nel_periodo(Somma_potenzaI1xV1,3297,Tot_misure);//W
   potenzaI2xV2=media_potenza_nel_periodo(Somma_potenzaI2xV2,3297,Tot_misure);//W
   potenzaI3xV3=media_potenza_nel_periodo(Somma_potenzaI3xV3,3297,Tot_misure);//W
   attiva=potenzaI1xV1+potenzaI2xV2+potenzaI3xV3;
   somma_energia_attiva +=(long)attiva;
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
  else
   {
   potenzaI1xV13=media_potenza_nel_periodo(Somma_potenzaI1xV13,9891,Tot_misure);//W
   somma_energia_attiva +=(long)potenzaI1xV13;
   }
  } break;
 case 2:
  {
  valore_efficace((int*)&I1_rms,media_quad_I1,6400); //(A*.004*5/.512* 4096/5)^2 = A^2*1024  = (A*10)^2*10.24 = (A*10)^2*6400>>16
  calcolo_medie_quadrati((unsigned long*)&media_quad_I1,quad_I1);
  } break;
 case 3:
  {
  if(mono_tri_fase)//trifase
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
  if(mono_tri_fase)//trifase
   {
   calcolo_medie_quadrati((unsigned long*)&media_quad_V12,quad_V12);
   valore_efficace((int*)&V12_rms,media_quad_V12,6210);//(V *33/680/6 /2 *4096/5)^2 = V^2*10.9755 = V^2*5971>>16 *(correzione=1.04)
   }
  } break;
 case 6:
  {
  if(mono_tri_fase)//trifase
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
  if(mono_tri_fase) calcolo_valor_medio_potenze((long*)&media_potenzaI1xV1,potenzaI1xV1);
  else calcolo_valor_medio_potenze((long*)&media_potenzaI1xV13,potenzaI1xV13);
  } break;
 case 9:
  {
  if(mono_tri_fase) calcolo_valor_medio_potenze((long*)&media_potenzaI2xV2,potenzaI2xV2);
  } break;
 case 10:
  {
  if(mono_tri_fase) calcolo_valor_medio_potenze((long*)&media_potenzaI3xV3,potenzaI3xV3);
  } break;
 case 11:
  {
  if(mono_tri_fase) calcolo_valor_medio_potenze((long*)&media_reattivaI1xV23,reattivaI1xV23);
  } break;
 case 12:
  {
  if(mono_tri_fase) calcolo_valor_medio_potenze((long*)&media_reattivaI2xV31,reattivaI2xV31);
  } break;
 case 13:
  {
  if(mono_tri_fase) calcolo_valor_medio_potenze((long*)&media_reattivaI3xV12,reattivaI3xV12);
  } break;
 case 14:
  {
  asm
   {
//calcolo della tenperatura motore  
   
   LDA set.tipo_sonda_PT100:1
   CMP #3
   BCS senza_compensazione
   BEQ compensazione_a_3_fili
  //per sonda a 4 fili 
;temperatura=(DAC.lettura_PT100/4096*5 /10 /(5/1126) *10 - set.resistenza_PT100_a_0gradi)*100/(set.resistenza_PT100_a_0gradi-set.resistenza_PT100_a_100gradi)
;temperatura=(DAC.lettura_PT100*18016>>16 - set.resistenza_PT100_a_0gradi)*100/(set.resistenza_PT100_a_0gradi-set.resistenza_PT100_a_100gradi)
;temperatura=(DAC.lettura_PT100*fattore_PT100>>16 - set.resistenza_PT100_a_0gradi)*100/(set.resistenza_PT100_a_0gradi-set.resistenza_PT100_a_100gradi)
   LDHX DAC.lettura_PT100
   STHX temperatura
   BRA uscita_temperatura
  
  compensazione_a_3_fili: 
;temperatura=((DAC.lettura_PT100-DAC.caduta_cavo_PT100)*fattore_PT100>>16 - set.resistenza_PT100_a_0gradi)*100/(set.resistenza_PT100_a_0gradi-set.resistenza_PT100_a_100gradi)
   LDA DAC.lettura_PT100:1
   SUB DAC.caduta_cavo_PT100:1
   STA temperatura:1
   LDA DAC.lettura_PT100
   SBC DAC.caduta_cavo_PT100
   STA temperatura
   BPL uscita_temperatura
   CLR temperatura
   CLR temperatura:1
   BRA uscita_temperatura
  
  senza_compensazione:
;temperatura=(DAC.lettura_PT100*fattore_PT100>>16 - set.resistenza_PT100_a_0gradi)*100/(set.resistenza_PT100_a_0gradi-set.resistenza_PT100_a_100gradi)
   LDHX DAC.lettura_PT100
   STHX temperatura
  
  uscita_temperatura:  
   LDA temperatura:1
   LDX fattore_PT100
   MUL
   STA delta:2
   STX delta:1
   LDA temperatura
   LDX fattore_PT100:1
   MUL
   ADC delta:2
   STA delta:2
   TXA 
   ADC delta:1
   STA delta:1
   CLR delta
   ROL delta
   LDA temperatura
   LDX fattore_PT100
   MUL
   ADC delta:1
   STA temperatura:1
   TXA 
   ADC delta
   STA temperatura
   
   LDA temperatura:1
   SUB set.resistenza_PT100_a_0gradi:1
   STA temperatura:1
   LDA temperatura
   SBC set.resistenza_PT100_a_0gradi
   STA temperatura
   BPL per_rapporto_limiti
   CLR temperatura
   CLR temperatura:1

  per_rapporto_limiti:   
   LDA temperatura:1
   LDX #100
   MUL
   STA delta:2
   STX delta:1
   LDA temperatura
   LDX #100
   MUL
   ADD delta:1
   STA delta:1
   TXA
   ADC #0
   STA delta

   LDA #$09
   STA j
  ripeti_div:
   LDA delta:1
   SUB delta_PT100_0_100gradi:1
   TAX
   LDA delta
   SBC delta_PT100_0_100gradi
   BCS alla_rotazione
   STA delta
   STX delta:1
alla_rotazione:
   ROL delta:2  
   ROL delta:1  
   ROL delta  
   DBNZ j,ripeti_div 
   COM delta:2  
   LDA delta:2
   STA temperatura
   }
  calcolo_valor_medio_potenze((long*)&media_temperatura,temperatura);
  if(media_temperatura<0) media_temperatura=0;
  } break;
 case 15:
  {
  asm
   {
//calcolo della pressione
;pressione=(DAC.sensore_pressione-491)*set.portata_sensore_pressione/fattore_portata_sensore_pressione;
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
   SBC fattore_portata_sensore_pressione:1
   TAX
   LDA delta
   SBC fattore_portata_sensore_pressione
   BCS alla_rotazione_delta
   STA delta
   STX delta:1
  alla_rotazione_delta:
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
  if((mono_tri_fase)&&(relay_alimentazione)) cosfi[0]=calcolo_del_fattore_di_potenza(attiva,reattiva); else cosfi[0]=0;//cosfi*100
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
  if((mono_tri_fase)&&(relay_alimentazione)) cosfi[1]=calcolo_del_fattore_di_potenza(attiva,reattiva); else cosfi[1]=0;//cosfi*100
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
  if((mono_tri_fase)&&(relay_alimentazione)) cosfi[2]=calcolo_del_fattore_di_potenza(attiva,reattiva); else cosfi[2]=0;//cosfi*100
  if(cosfi[2]>99) cosfi[2]=99;
  } break;
 case 19:
  {
  if(relay_alimentazione==0) cosfi[3]=0; else
   {
   if(mono_tri_fase) cosfi[3]=calcolo_del_fattore_di_potenza(potenza_media,reattiva_media);//cosfi*100
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
 JSR calcolo_tensione_relay

 LDA flusso:1
 SUB media_flusso:1
 STA delta:2
 LDA flusso
 SBC media_flusso
 STA delta:1
 CLRA
 SBC #0
 STA delta
 LDA media_flusso:2
 ADD delta:2
 STA media_flusso:2
 LDA media_flusso:1
 ADC delta:1
 STA media_flusso:1
 LDA media_flusso
 ADC delta
 STA media_flusso
 
 LDHX timer_flusso_nullo
 BEQ per_timer_presenta_data
 LDA timer_flusso_nullo:1
 SUB #1
 STA timer_flusso_nullo:1
 LDA timer_flusso_nullo
 SBC #0
 STA timer_flusso_nullo
 
per_timer_presenta_data:
 LDHX timer_presenta_data
 BEQ per_timer_inc_dec 
 LDA timer_presenta_data:1
 SUB #1
 STA timer_presenta_data:1
 LDA timer_presenta_data
 SBC #0
 STA timer_presenta_data
 
per_timer_inc_dec:
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
 BEQ per_timer_allarme_avviamento
 LDA timer_piu_meno:1
 SUB #1
 STA timer_piu_meno:1
 LDA timer_piu_meno
 SBC #0
 STA timer_piu_meno
 
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
 BEQ per_timer_aggiorna_menu
 LDA timer_aggiorna_misure:1
 SUB #1
 STA timer_aggiorna_misure:1
 LDA timer_aggiorna_misure
 SBC #0
 STA timer_aggiorna_misure

per_timer_aggiorna_menu:
 LDHX timer_aggiorna_menu
 BEQ per_timer_attesa_segnalazione_fault
 LDA timer_aggiorna_menu:1
 SUB #1
 STA timer_aggiorna_menu:1
 LDA timer_aggiorna_menu
 SBC #0
 STA timer_aggiorna_menu
  
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
 STHX TPM1C0V
 BRA per_sequenza_lettura_
per_lampeggio:
 LDHX timer_lampeggio_LED_emergenza
 CPHX #125
 BCC spegni_LED_rosso
 LDHX #$7fff
 STHX TPM1C0V
 BRA per_sequenza_lettura_
spegni_LED_rosso:
 LDHX #-1
 STHX TPM1C0V
per_sequenza_lettura_:
 JSR sequenza_lettura_scrittura_ram
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
 LDA #$ff
 STA dato_scritto
 
;if(scrivi_eeprom)
; {
; eeprom_impegnata=1;
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
;  contatore_scrivi_eeprom=0;
;  scrivi_eeprom++;
;  }
; else if(contatore_scrivi_eeprom<=lunghezza_salvataggio)
;  {
;  dato_scritto=buffer_eeprom[contatore_scrivi_eeprom];
;  if(contatore_scrivi_eeprom<lunghezza_salvataggio) contatore_scrivi_eeprom++; else goto chiusura_scrittura;
;  }
; else if(scrivi_eeprom<80) //pausa salvataggio
;  {
;  PTED |=0x80; //EEPROM disabilitata
;  scrivi_eeprom++;
;  }
; else if(scrivi_eeprom==80)
;  {
;  PTED &=0x7f; //EEPROM abilitata
;  dato_scritto=0x04; //codice di blocco della scrittura
;  scrivi_eeprom++;
;  }
; else//if(scrivi_eeprom==81)
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
 BNE scrittura_del_dato
; else if(scrivi_eeprom==6) //scrivi parte bassa indirizzo
;  {
;  dato_scritto=indirizzo_scrivi_eeprom:2;
;  contatore_scrivi_eeprom=0;
;  scrivi_eeprom++;
;  }
 LDA indirizzo_scrivi_eeprom:2
 STA dato_scritto//_SPID
 CLRA
 STA contatore_scrivi_eeprom
 LDA scrivi_eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

scrittura_del_dato: 
; else if(contatore_scrivi_eeprom<=lunghezza_salvataggio)
;  {
;  dato_scritto=buffer_eeprom[contatore_scrivi_eeprom];
;  if(contatore_scrivi_eeprom<lunghezza_salvataggio) contatore_scrivi_eeprom++; else goto chiusura_scrittura;
;  }
 LDX contatore_scrivi_eeprom
 CLRH
 LDA @buffer_eeprom,X
 STA dato_scritto//_SPID
 LDA contatore_scrivi_eeprom
 CMP lunghezza_salvataggio
 BCC chiusura_scrittura
 INCA
 STA contatore_scrivi_eeprom
 BRA fine_SPID

chiusura_scrittura:
 LDA scrivi_eeprom
 CMP #80 
 BEQ comando_disabilitazione
 BCC per_disabilitazione
; else if(scrivi_eeprom<80) //pausa salvataggio
;  {
;  PTED |=0x80; //EEPROM disabilitata
;  scrivi_eeprom++;
;  }
 BSET 7,_PTED //disabilita eeprom
 INCA
 STA scrivi_eeprom
 BRA fine_SPID

comando_disabilitazione:
; else if(scrivi_eeprom==80)
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
; else//if(scrivi_eeprom==81)
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
; eeprom_impegnata=1;
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
;  contatore_leggi_eeprom=0;
;  }
; else if(contatore_leggi_eeprom<=ultimo_dato_in_lettura)
;  {
;  buffer_eeprom[contatore_leggi_eeprom]=dato_letto;
;  if(contatore_leggi_eeprom<=ultimo_dato_in_lettura) contatore_leggi_eeprom++; else goto fine_lettura;
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
 CLRA
 STA contatore_leggi_eeprom
 BRA fine_SPID

lettura_dati:
; else if(contatore_leggi_eeprom<=ultimo_dato_in_lettura)
;  {
;  buffer_eeprom[contatore_leggi_eeprom]=dato_letto;
;  if(contatore_leggi_eeprom<=ultimo_dato_in_lettura) contatore_leggi_eeprom++; else goto fine_lettura;
;  }
 LDX contatore_leggi_eeprom
 CLRH
 LDA dato_letto 
 STA @buffer_eeprom,X
 LDA contatore_leggi_eeprom
 CMP ultimo_dato_in_lettura
 BCC fine_lettura
 INCA
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
Fine: 
 } 
}

void trasmissione_misure_istantanee(void) //con allarme_in_lettura == 9000
{
asm
 {
//31 bytes nell'ordine:
//1       @
//1       0x10
//1       0x30
//1       0x01
//2       24 (lunghezza)      
//2       V12,
//2       V13,
//2       V23,
//2        I1,
//2        I2,
//2        I3,
//1       cosfi1,
//1       cosfi2,
//1       cosfi3,
//1      cosfi_medio,
//2       potenza,
//2       pressione,
//2      flusso  litri/minuto
//1      temperatura
//1      allarme
//1      CRC (somma)
 LDHX allarme_in_lettura
 CPHX lettura_satellitare
 BNE fine
 LDHX operazione_effettuata
 STHX allarme_in_lettura
 LDA #'@'
 STA buffer_USB
 LDA #$10
 STA buffer_USB:1
 LDA #$30
 STA buffer_USB:2
 LDA #$01
 STA buffer_USB:3
 LDHX #24
 STHX buffer_USB:4
 
 LDHX V12_rms
 STHX buffer_USB:6
 LDHX V23_rms
 STHX buffer_USB:8
 LDHX V31_rms
 STHX buffer_USB:10
 LDHX I1_rms
 STHX buffer_USB:12
 LDHX I2_rms
 STHX buffer_USB:14
 LDHX I3_rms
 STHX buffer_USB:16
 LDHX cosfi
 STHX buffer_USB:18
 LDHX cosfi:2
 STHX buffer_USB:20
 LDHX potenza_media
 STHX buffer_USB:22
 LDHX media_pressione
 STHX buffer_USB:24
 LDHX media_flusso
 STHX buffer_USB:26
 LDA media_temperatura:1
 STA buffer_USB:28
 LDA segnalazione_
 STA buffer_USB:29
 
 CLRA
 LDHX #30 
somma:
 ADD @buffer_USB:-1,X 
 DBNZX somma
 STA buffer_USB:30
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
 LDX byte_tabella
 CLRH
assegna_buffer:
 LDA @set,X
 STA @buffer_USB,X
 DBNZX assegna_buffer
 LDA set
 STA buffer_USB
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

void presenta_scritta(char *stringa, int offset, char idioma, char lunghezza, char riga,char colonna, char N_caratteri)
{
unsigned char j, k;
int indirizzo;
asm
 { 
; indirizzo = stringa + offset + idioma*lunghezza*17;
 LDA idioma
 LDX lunghezza
 MUL
 LDX #17
 MUL
 STA indirizzo:1
 STX indirizzo
 
 LDA indirizzo:1
 ADD offset:1
 STA indirizzo:1
 LDA indirizzo
 ADC offset
 STA indirizzo

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
 LDA riga
 LDX #17
 MUL
 ADD colonna
 STA lampeggio_da
 
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
 LDHX #10
 LDA val
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

void presenta_PF(unsigned char val, char riga, char colonna)
{
char inizio;
asm
 {
 LDA #99
 CMP val
 BCC per_posizione
 STA val

per_posizione: 
 LDA colonna
 ADD #1
 CMP #16
 BCC fine
 LDA riga
 CMP #2
 BCC fine
 LDX #17
 MUL
 ADD colonna
 STA inizio
 
 LDA #'.'
 STA cifre
 LDHX #10
 LDA val
 DIV
 STA val
 ORA #$30
 STA cifre:1
 PSHH
 PULA
 ORA #$30
 STA cifre:2
  
 LDX inizio
 CLRH
 LDA cifre
 STA @display,X
 LDA cifre:1
 STA @display:1,X
 LDA cifre:2
 STA @display:2,X
fine:
 }
}

void presenta_energia(unsigned long val, char punto, char riga, char colonna, char n_cifre)
{
char j, k, inizio, s, KWh[7];
int divisore_energia;
asm
 {
 //KWh=val*256^2/50/3600000*1000 =val*256/703
 LDA punto
 CMP #2
 BNE con_3_decimali
 LDHX #7031
 STHX divisore_energia
 BRA assegna_KWh
con_3_decimali:
 LDHX #703
 STHX divisore_energia

assegna_KWh: 
 LDHX val:2
 STHX KWh:4
 LDHX val
 STHX KWh:2
 CLRA
 STA KWh
 STA KWh:1
 STA KWh:6
 LDA #$29
 STA j
ripeti_div:
 LDA KWh:1
 SUB divisore_energia:1
 TAX
 LDA KWh
 SBC divisore_energia
 BCS per_rotazione
 STA KWh
 STX KWh:1
per_rotazione:
 ROL KWh:6
 ROL KWh:5
 ROL KWh:4
 ROL KWh:3
 ROL KWh:2
 ROL KWh:1
 ROL KWh
 DBNZ j,ripeti_div
 COM KWh:3 
 COM KWh:4 
 COM KWh:5
 COM KWh:6

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
 LDHX #10
 LDA KWh:3
 DIV
 STA KWh:3
 LDA KWh:4
 DIV
 STA KWh:4
 LDA KWh:5
 DIV
 STA KWh:5
 LDA KWh:6
 DIV
 STA KWh:6
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

const int fattore_portata=2812;
void misura_della_portata(void)
{
char j, prod[5], delta[2];
long timer;
asm
 {
 LDA impulso_flusso
 BEQ fine
 CLRA
 STA impulso_flusso
 LDHX timer_flusso_nullo
 BNE per_calcolo_flusso
 LDHX #0
 STHX flusso
 BRA fine
per_calcolo_flusso:

// conta_litri +=scala_sensore_di_flusso[litri*1000/impulso]/1000*65536;
// conta_litri +=incremento_volume;
 LDA conta_litri:5
 ADD incremento_volume:3
 STA conta_litri:5
 LDA conta_litri:4
 ADC incremento_volume:2
 STA conta_litri:4
 LDA conta_litri:3
 ADC incremento_volume:1
 STA conta_litri:3
 LDA conta_litri:2
 ADC incremento_volume
 STA conta_litri:2
 LDA conta_litri:1
 ADC #0
 STA conta_litri:1
 LDA conta_litri
 ADC #0
 STA conta_litri
 
//portata[l/min] = scala_sensore_di_flusso[litri*1000/impulso]/1000 *60*1000000/10.66667/timer_flusso;
//portata[l/min] = scala_sensore_di_flusso[litri*1000/impulso]*5625/timer_flusso;
//portata[l/min] = scala_sensore_di_flusso[litri*1000/impulso]*2812/(timer_flusso>>1);
 LDHX timer_flusso
 STHX timer
 CLR timer:2
 LSR timer
 ROR timer:1
 ROR timer:2
 LDA set.scala_sensore_di_flusso:1
 LDX fattore_portata:1
 MUL 
 STA prod:3
 STX prod:2
 LDA set.scala_sensore_di_flusso:1
 LDX fattore_portata
 MUL 
 ADD prod:2
 STA prod:2
 TXA
 ADC #0
 STA prod:1
 LDA set.scala_sensore_di_flusso
 LDX fattore_portata:1
 MUL 
 ADD prod:2
 STA prod:2
 TXA
 ADC prod:1
 STA prod:1
 CLR prod
 ROL prod
 LDA set.scala_sensore_di_flusso
 LDX fattore_portata
 MUL 
 ADD prod:1
 STA prod:1
 TXA
 ADC prod
 STA prod

 LDA #$11
 STA j
ripeti:
 LDA prod:2
 SUB timer:2
 STA delta:1
 LDA prod:1
 SBC timer:1
 STA delta
 LDA prod
 SBC timer
 BCS alla_rotazione
 STA prod
 LDHX delta
 STHX prod:1
alla_rotazione:
 ROL prod:4 
 ROL prod:3 
 ROL prod:2 
 ROL prod:1 
 ROL prod 
 DBNZ j,ripeti
 COM prod:4
 COM prod:3
 LDHX prod:3
 STHX flusso
fine: 
 }
}


void salva_reset_default(void)
{
char j;
int funzioni_non_modif[4], k, var;
asm
 {
 LDA eeprom_impegnata
 BNE fine
 LDA reset_default
 BEQ fine

 CLRA
 STA N_cifre_lampeggianti
 LDHX indirizzo_scrivi_eeprom
 CPHX ultimo_indirizzo_scrittura
 BCC per_default_parametri

 LDA #255
 STA lunghezza_salvataggio
 LDHX #255
ripeti_assegnazione:
 STA @buffer_eeprom,X
 DBNZX ripeti_assegnazione 
 STA buffer_eeprom
 LDA indirizzo_scrivi_eeprom:1
 ADD #1
 STA indirizzo_scrivi_eeprom:1
 LDA indirizzo_scrivi_eeprom
 ADC #0
 STA indirizzo_scrivi_eeprom
 LDA #1
 STA eeprom_impegnata 
 STA scrivi_eeprom
 BSET 7,_PTED;//disabilitazione eeprom
 MOV #$ff,SPI1D
 BRA fine

per_default_parametri:
 CLRA
 STA reset_default
 STA toggle_func
 STA salita_start
 STA toggle_enter
 STA toggle_start
 STA cursore_menu
 
 LDHX set.numero_serie
 STHX funzioni_non_modif

 LDHX set.calibrazione_I1
 CPHX tabella_calibrazione:4
 BEQ cal_accettata
 BCC assegna_defa
 CPHX tabella_calibrazione:2
 BCC cal_accettata
assegna_defa:
 LDHX tabella_calibrazione
cal_accettata: 
 STHX funzioni_non_modif:2

 LDHX set.calibrazione_I2
 CPHX tabella_calibrazione:4
 BEQ Cal_accettata
 BCC Assegna_defa
 CPHX tabella_calibrazione:2
 BCC Cal_accettata
Assegna_defa:
 LDHX tabella_calibrazione
Cal_accettata: 
 STHX funzioni_non_modif:4

 LDHX set.calibrazione_I3
 CPHX tabella_calibrazione:4
 BEQ CAl_accettata
 BCC ASsegna_defa
 CPHX tabella_calibrazione:2
 BCC CAl_accettata
ASsegna_defa:
 LDHX tabella_calibrazione
CAl_accettata: 
 STHX funzioni_non_modif:6

 LDHX #0 
 STHX set.numero_segnalazione
 STHX set.abilita_sensore_pressione
 STHX set.abilita_sensore_flusso
 STHX set.abilita_sensore_temperatura
 STHX set.modo_start_stop
 STHX set.motore_on
 
 LDHX set.N_tabella_potenza
 CPHX #15
 BCS per_successive_limitazioni
 LDHX #8 //motore trifase 5.5KW
 STHX set.N_tabella_potenza

per_successive_limitazioni:
 LDA #1
 LDHX set.N_tabella_potenza
 CPHX #9
 BCC versione_trifase
 CLRA
versione_trifase: 
 STA mono_tri_fase
 LSLX
 LDA @tabella_potenza_nominale,X
 STA set.potenza_nominale
 LDA @tabella_potenza_nominale:1,X
 STA set.potenza_nominale:1

 CLR j
 CLR k
 CLR k:1
ripeti_ass_default:
 LDHX k
 LDA @tabella_ritardo_protezione_squilibrio,X
 STA var
 LDA @tabella_ritardo_protezione_squilibrio:1,X
 CLRH
 LDX j
 LSLX
 STA @ set.ritardo_protezione_squilibrio:1,X
 LDA var
 STA @ set.ritardo_protezione_squilibrio,X
 LDA k:1
 ADD #6
 STA k:1
 LDA k
 ADC #0
 STA k
 INC j
 LDA j
 CMP #25
 BCS ripeti_ass_default
  
 LDA mono_tri_fase
 BEQ per_funz_comuni
 CLR j
 LDHX set.N_tabella_potenza
 LSL k:1
 ROL k
Ripeti_ass_default:
 LDHX k
 LDA @tabella_limite_segnalazione_dissimmetria,X
 STA var
 LDA @tabella_limite_segnalazione_dissimmetria:1,X
 CLRH
 LDX j
 LSLX
 STA @ set.limite_segnalazione_dissimmetria:1,X
 LDA var
 STA @ set.limite_segnalazione_dissimmetria,X
 LDA k:1
 ADD #54 //6x9
 STA k:1
 LDA k
 ADC #0
 STA k
 INC j
 LDA j
 CMP #4
 BCS Ripeti_ass_default

per_funz_comuni:
 CLR j
 LDHX set.N_tabella_potenza
 LSL k:1
 ROL k
ripeti_Ass_default:
 LDHX k
 LDA @tabella_tensione_nominale,X
 STA var
 LDA @tabella_tensione_nominale:1,X
 CLRH
 LDX j
 LSLX
 STA @ set.tensione_nominale:1,X
 LDA var
 STA @ set.tensione_nominale,X
 LDA k:1
 ADD #90 //6x15
 STA k:1
 LDA k
 ADC #0
 STA k
 INC j
 LDA j
 CMP #4
 BCS ripeti_Ass_default

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
 STA set.conta_ore
 STA set.conta_ore:1
 STA set.conta_ore:2
 STA set.conta_ore:3
 STA set.conta_ore_funzionamento
 STA set.conta_ore_funzionamento:1
 STA set.conta_ore_funzionamento:2
 STA set.conta_ore_funzionamento:3
 STA set.energia
 STA set.energia:1
 STA set.energia:2
 STA set.energia:3
 STA set.conta_litri_funzionamento
 STA set.conta_litri_funzionamento:1
 STA set.conta_litri_funzionamento:2
 STA set.conta_litri_funzionamento:3
 STA conta_secondi
 STA conta_secondi:1
 STA conta_secondi:2
 STA conta_secondi:3
 STA conta_secondi_attivita
 STA conta_secondi_attivita:1
 STA conta_secondi_attivita:2
 STA conta_secondi_attivita:3
 
 LDX byte_tabella
 CLRH
metti_in_buffer_eeprom:
 LDA @set,X
 STA @buffer_eeprom,X
 DBNZX metti_in_buffer_eeprom
 LDA set
 STA buffer_eeprom
  
 JSR calcolo_delle_costanti
 
 LDA #1
 STA salvataggio_funzioni
fine:
 }
}

void presenta_cursore_eeprom(long var)
{
char j;
asm
 {
 LDHX #32
 LDA var
 DIV
 LDA var:1
 DIV
 INCA
 STA j
 
 LDX #16
 CLRH
 LDA #' '
ripeti:
 DECX
 CPX j
 BCS tacche
 STA @display:17,X
 BRA ripeti

tacche:
 LDX j
 LDA #$ff//tacche temporizzatore salvataggio  (0xfd)
Ripeti:
 STA @display:16,X
 DBNZX Ripeti 
 }
}

void salva_impostazioni(void)
{
asm
 {
 LDA eeprom_impegnata
 BNE fine
 LDA salvataggio_funzioni
 BEQ fine
 LDA reset_default
 BNE fine
 LDA salvataggio_allarme
 BNE fine
 CLRA
 STA salvataggio_funzioni 
  
 LDA byte_tabella
 STA lunghezza_salvataggio
 LDHX primo_indirizzo_funzioni
 STHX indirizzo_scrivi_eeprom
 LDA primo_indirizzo_funzioni:2
 STA indirizzo_scrivi_eeprom:2
 LDX byte_tabella
 CLRH
ripeti_trasferimento: 
 LDA @set,X
 STA @buffer_eeprom,X
 DBNZX ripeti_trasferimento
 LDA set
 STA buffer_eeprom
 LDA #1
 STA eeprom_impegnata 
 STA scrivi_eeprom
 BSET 7,_PTED;//disabilitazione eeprom
 MOV #$ff,SPI1D
fine: 
 }
}

void salva_segnalazione_fault(void)
{
char j;
asm
 {
 LDA salvataggio_allarme
 BEQ fine
 LDA eeprom_impegnata
 BNE fine
 LDA reset_default
 BNE fine
 LDA salvataggio_funzioni
 BNE fine
  
 LDA salvataggio_allarme
 CMP #1
 BNE secondo_salvataggio 
 LDA indirizzo_scrivi_eeprom:2
 ADD N_bytes_messaggio_fault
 STA ultimo_indirizzo_scrittura:2
 LDA indirizzo_scrivi_eeprom:1
 ADC #0
 STA ultimo_indirizzo_scrittura:1
 LDA indirizzo_scrivi_eeprom
 ADC #0
 STA ultimo_indirizzo_scrittura

 LDHX ultimo_indirizzo_scrittura
 CPHX indirizzo_scrivi_eeprom
 BCC semplice_salvataggio
 LDA #2
 STA salvataggio_allarme
 BRA primo_salvataggio
semplice_salvataggio:
 CLRA
 STA salvataggio_allarme
 LDA N_bytes_messaggio_fault
 DECA
 STA lunghezza_salvataggio
 TAX
 CLRH
metti_in_buffer_eeprom:
 LDA @buffer_fault,X
 STA @buffer_eeprom,X
 DBNZX metti_in_buffer_eeprom
 LDA buffer_fault
 STA buffer_eeprom
 LDA #1
 STA eeprom_impegnata 
 STA scrivi_eeprom
 BSET 7,_PTED;//disabilitazione eeprom
 MOV #$ff,SPI1D
 BRA fine
 
primo_salvataggio:
 LDA indirizzo_scrivi_eeprom:2
 COMA
 STA primo_elemento_buffer
 STA lunghezza_salvataggio
 TAX
 CLRH
metti_fault_buffer_eeprom:
 LDA @buffer_fault,X
 STA @buffer_eeprom,X
 DBNZX metti_fault_buffer_eeprom
 LDA buffer_fault
 STA buffer_eeprom
 LDA #1
 STA eeprom_impegnata 
 STA scrivi_eeprom
 BSET 7,_PTED;//disabilitazione eeprom
 MOV #$ff,SPI1D
 BRA fine
 
secondo_salvataggio:
 LDHX ultimo_indirizzo_scrittura
 STHX indirizzo_scrivi_eeprom
 CLRA
 STA indirizzo_scrivi_eeprom:2
 LDA ultimo_indirizzo_scrittura:2
 DECA
 STA lunghezza_salvataggio
 CLR j
 CLRH
metti_Fault_buffer_eeprom:
 LDX primo_elemento_buffer
 INCX
 STX primo_elemento_buffer
 LDA @buffer_fault,X
 LDX j
 STA @buffer_eeprom,X
 INC j
 LDA j
 CMP lunghezza_salvataggio
 BCS metti_Fault_buffer_eeprom
 BEQ metti_Fault_buffer_eeprom
 CLRA
 STA salvataggio_allarme
 LDA #1
 STA eeprom_impegnata 
 STA scrivi_eeprom
 BSET 7,_PTED;//disabilitazione eeprom
 MOV #$ff,SPI1D

fine: 
 }
}

const long
secondi_per_anno=35942400,//13*32*24*3600
secondi_per_mese=2764800,//32*24*3600
secondi_per_giorno=86400;//24*3600
const int
secondi_per_ora=3600,
secondi_per_minuto=60;
void calcolo_istante(void)
{
long prod;
asm
 {
 LDA secondi_per_anno:3
 LDX Anno
 MUL
 STA istante:3
 STX istante:2    
 LDA secondi_per_anno:2
 LDX Anno
 MUL
 ADD istante:2
 STA istante:2
 TXA
 ADC #0
 STA istante:1
 LDA secondi_per_anno:1
 LDX Anno
 MUL
 ADD istante:1
 STA istante:1
 TXA
 ADC #0
 STA istante
 LDA secondi_per_anno
 LDX Anno
 MUL
 ADD istante
 STA istante

 LDA secondi_per_mese:3
 LDX Mese
 MUL
 STA prod:3
 STX prod:2    
 LDA secondi_per_mese:2
 LDX Mese
 MUL
 ADD prod:2
 STA prod:2
 TXA
 ADC #0
 STA prod:1
 LDA secondi_per_mese:1
 LDX Mese
 MUL
 ADD prod:1
 STA prod:1
 TXA
 ADC #0
 STA prod
 
 LDA istante:3
 ADD prod:3
 STA istante:3
 LDA istante:2
 ADC prod:2
 STA istante:2
 LDA istante:1
 ADC prod:1
 STA istante:1
 LDA istante
 ADC prod
 STA istante

 LDA secondi_per_giorno:3
 LDX Giorno
 MUL
 STA prod:3
 STX prod:2    
 LDA secondi_per_giorno:2
 LDX Giorno
 MUL
 ADD prod:2
 STA prod:2
 TXA
 ADC #0
 STA prod:1
 LDA secondi_per_giorno:1
 LDX Giorno
 MUL
 ADD prod:1
 STA prod:1
 TXA
 ADC #0
 STA prod
 
 LDA istante:3
 ADD prod:3
 STA istante:3
 LDA istante:2
 ADC prod:2
 STA istante:2
 LDA istante:1
 ADC prod:1
 STA istante:1
 LDA istante
 ADC prod
 STA istante

 LDA secondi_per_ora:1
 LDX Ora
 MUL
 STA prod:3
 STX prod:2    
 LDA secondi_per_ora
 LDX Ora
 MUL
 ADD prod:2
 STA prod:2
 TXA
 ADC #0
 STA prod:1
 
 LDA istante:3
 ADD prod:3
 STA istante:3
 LDA istante:2
 ADC prod:2
 STA istante:2
 LDA istante:1
 ADC prod:1
 STA istante:1
 LDA istante
 ADC #0
 STA istante

 LDA secondi_per_minuto:1
 LDX Minuto
 MUL
 STA prod:3
 STX prod:2    
 LDA prod:3
 ADD Secondo
 STA prod:3
 LDA prod:2
 ADC #0
 STA prod:2

 LDA istante:3
 ADD prod:3
 STA istante:3
 LDA istante:2
 ADC prod:2
 STA istante:2
 LDA istante:1
 ADC #0
 STA istante:1
 LDA istante
 ADC #0
 STA istante
 }
}

void prepara_per_salvataggio_allarme(void)
{
long data;
asm
 {
 LDA eeprom_impegnata
 BNE fine
 LDA reset_default
 BNE fine
 LDA salvataggio_funzioni
 BNE fine
 LDA segnalazione_
 CMP segnalazioni_senza_intervento
 BCC per_registrazione
 LDHX timer_attesa_segnalazione_fault
 BNE fine
per_registrazione:   
 LDA segnalazione_
 CMP precedente_segnalazione
 BEQ fine
 STA precedente_segnalazione

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
//nuova versione settembre -2011
//20 bytes trasmessi per un complesso di 6539 registrazioni  per 129510 bytes
// 1 byte = tipo intervento 
// 4 bytes = tempo = secondo + minuto*60 + ora*3600 + giorno*24*3600 + mese*32*24*3600 + anno*13*32*24*3600   ; valido fino al 2119
//   processo inverso:
//   anno(a partire dal 2000) = tempo/(13*32*24*3600); resto_anno = tempo - anno*(13*32*24*3600);
//   mese = resto_anno/(32*24*3600); resto_mese = resto_anno - mese*(32*24*3600)
//   giorno = resto_mese/(24*3600);  resto_giorno = resto_mese - giorno*(24*3600)
//   ora = resto_giorno/3600;  resto_ora = resto_giorno - ora*3600;
//   minuto = resto_ora/60;  resto_minuto = secondo = resto_ora - minuto*60;
// 4 bytes per le 3 tensioni: tensioni = (((V12_rms<<10)+V13_rms)<<10)+V23_rms; campo 0-1023V
// 4 bytes per le 3 correnti = (((I1<<10)+I2)<<10)+I3;  I= 0-102.3A, oppure 0-1023A
// 4 bytes = potenza_pressione_cosfi = (((potenza<<10) + pressione)<<7) +cosfi; potenza 0-327.67KW, pressione 0-102.3Bar, cosfi= 0-99
// 2 bytes = flusso
// 1 byte = temperatura  0-255 °C

 LDA segnalazione_;//tipo intervento:
 STA buffer_fault

 LDHX istante
 STHX buffer_fault:1
 LDHX istante:2
 STHX buffer_fault:3

 LDHX V12_rms
 STHX data
 LDHX V23_rms
 STHX data:2
 LDX #6
rotazione_6_volte: 
 LSL data:3
 ROL data:2 
 DBNZX rotazione_6_volte
 LDX #4
rotazione_4_volte: 
 LSL data:3
 ROL data:2 
 ROL data:1 
 ROL data 
 DBNZX rotazione_4_volte
 LDA V31_rms
 ORA data:2
 STA data:2
 LDA V31_rms:1
 STA data:3

 LDHX data
 STHX buffer_fault:5
 LDHX data:2
 STHX buffer_fault:7

 LDHX I1_rms
 STHX data
 LDHX I2_rms
 STHX data:2
 LDX #6
rotazione_6_Volte: 
 LSL data:3
 ROL data:2 
 DBNZX rotazione_6_Volte
 LDX #4
rotazione_4_Volte: 
 LSL data:3
 ROL data:2 
 ROL data:1 
 ROL data 
 DBNZX rotazione_4_Volte
 LDA I3_rms
 ORA data:2
 STA data:2
 LDA I3_rms:1
 STA data:3

 LDHX data
 STHX buffer_fault:9
 LDHX data:2
 STHX buffer_fault:11

 LDHX potenza_media
 STHX data
 LDHX media_pressione
 STHX data:2
 LDX #6
Rotazione_6_volte: 
 LSL data:3
 ROL data:2 
 DBNZX Rotazione_6_volte
 LSL data:3
 ROL data:2 
 ROL data:1 
 ROL data 
 LDA cosfi:3
 STA data:3

 LDHX data
 STHX buffer_fault:13
 LDHX data:2
 STHX buffer_fault:15

 LDHX media_flusso
 STHX buffer_fault:17

 LDA media_temperatura:1
 STA buffer_fault:19

 CLRA
 LDA set.numero_segnalazione:1
 LDX N_bytes_messaggio_fault
 MUL
 STA indirizzo_scrivi_eeprom:2
 STX indirizzo_scrivi_eeprom:1
 LDA set.numero_segnalazione
 LDX N_bytes_messaggio_fault
 MUL
 ADD indirizzo_scrivi_eeprom:1
 STA indirizzo_scrivi_eeprom:1
 TXA
 ADC #0
 STA indirizzo_scrivi_eeprom
 
 LDA #1
 STA salvataggio_allarme
fine:
 } 
}

void salva_conta_secondi_attivita(void)//ogni 1 minuti
{
asm
 {
 LDA salva_conta_secondi
 BEQ fine
 LDA eeprom_impegnata
 BNE fine
 LDA reset_default
 BNE fine
 LDA salvataggio_funzioni
 BNE fine
 LDA salvataggio_allarme
 BNE fine
 CLRA
 STA salva_conta_secondi

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

 LDHX misura_energia
 STHX set.energia
 STHX buffer_eeprom:12
 LDHX misura_energia:2
 STHX set.energia:2
 STHX buffer_eeprom:14

 LDHX conta_litri
 STHX set.conta_litri_funzionamento
 STHX buffer_eeprom:16
 LDHX conta_litri:2
 STHX set.conta_litri_funzionamento:2
 STHX buffer_eeprom:18

 LDHX set.riserva
 STHX buffer_eeprom:20
 LDHX set.riserva:2
 STHX buffer_eeprom:22
 LDHX set.riserva:4
 STHX buffer_eeprom:24
 LDHX set.riserva:6
 STHX buffer_eeprom:26

 LDA #1
 STA salvataggio_funzioni 
fine: 
 }
}

void lettura_impostazioni(void)
{
char j;
int k, var, defa, min, max;
asm
 {
 LDA leggi_impostazioni
 BEQ fine
 LDA eeprom_impegnata
 BNE fine
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
 LDA byte_tabella
 STA ultimo_dato_in_lettura
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

 LDX byte_tabella
 CLRH
trasferimento: 
 LDA @buffer_eeprom,X
 STA @set,X
 DBNZX trasferimento
 LDA buffer_eeprom
 STA set
 
 CLRA
 LDHX set.numero_segnalazione
 CPHX totale_indicazioni_fault
 BCS per_abilitazioni
 STA set.numero_segnalazione
 STA set.numero_segnalazione:1
 
per_abilitazioni:
 LDHX set.N_tabella_potenza
 CPHX #15
 BCS per_tabella_potenza
 LDHX #8
 STHX set.N_tabella_potenza
 BRA per_trifase
per_tabella_potenza: 
 LDHX set.N_tabella_potenza
 LDA @tabella_potenza_nominale,X
 STA set.potenza_nominale
 LDA @tabella_potenza_nominale:1,X
 STA set.potenza_nominale:1
 CPHX #9
 BCS per_trifase
 CLRA
 STA mono_tri_fase
 BRA per_monofase_trifase
per_trifase: 
 LDA #1
 STA mono_tri_fase

per_monofase_trifase:
 LDHX set.calibrazione_I1
 CPHX tabella_calibrazione:4
 BEQ cal_accettata
 BCC assegna_defa
 CPHX tabella_calibrazione:2
 BCC cal_accettata
assegna_defa:
 LDHX tabella_calibrazione
 STHX set.calibrazione_I1

cal_accettata: 
 LDHX set.calibrazione_I2
 CPHX tabella_calibrazione:4
 BEQ Cal_accettata
 BCC Assegna_defa
 CPHX tabella_calibrazione:2
 BCC Cal_accettata
Assegna_defa:
 LDHX tabella_calibrazione
 STHX set.calibrazione_I2

Cal_accettata: 
 LDHX set.calibrazione_I3
 CPHX tabella_calibrazione:4
 BEQ CAl_accettata
 BCC ASsegna_defa
 CPHX tabella_calibrazione:2
 BCC CAl_accettata
ASsegna_defa:
 LDHX tabella_calibrazione
 STHX set.calibrazione_I3

CAl_accettata: 
 CLR j
 CLR k
 CLR k:1
ripeti_ass_default:
 LDHX k
 LDA @tabella_ritardo_protezione_squilibrio,X
 STA defa
 LDA @tabella_ritardo_protezione_squilibrio:1,X
 STA defa:1
 LDA @tabella_ritardo_protezione_squilibrio:2,X
 STA min
 LDA @tabella_ritardo_protezione_squilibrio:3,X
 STA min:1
 LDA @tabella_ritardo_protezione_squilibrio:4,X
 STA max
 LDA @tabella_ritardo_protezione_squilibrio:5,X
 STA max:1
 CLRH
 LDX j
 LSLX
 LDA @ set.ritardo_protezione_squilibrio:1,X
 STA var:1
 LDA @ set.ritardo_protezione_squilibrio,X
 STA var
 LDHX var
 CPHX max
 BEQ procedi_oltre
 BCC per_default
 CPHX min
 BCC procedi_oltre
per_default:
 LDHX defa
 STHX var
procedi_oltre:
 CLRH
 LDX j
 LSLX
 LDA var:1
 STA @ set.ritardo_protezione_squilibrio:1,X
 LDA var
 STA @ set.ritardo_protezione_squilibrio,X
 LDA k:1
 ADD #6
 STA k:1
 LDA k
 ADC #0
 STA k
 INC j
 LDA j
 CMP #25
 BCS ripeti_ass_default
  
 LDA mono_tri_fase
 BEQ per_funz_comuni
 CLR j
 LDHX set.N_tabella_potenza
 LSL k:1
 ROL k
Ripeti_ass_default:
 LDHX k
 LDA @tabella_limite_segnalazione_dissimmetria,X
 STA defa
 LDA @tabella_limite_segnalazione_dissimmetria:1,X
 STA defa:1
 LDA @tabella_limite_segnalazione_dissimmetria:2,X
 STA min
 LDA @tabella_limite_segnalazione_dissimmetria:3,X
 STA min:1
 LDA @tabella_limite_segnalazione_dissimmetria:4,X
 STA max
 LDA @tabella_limite_segnalazione_dissimmetria:5,X
 STA max:1
 CLRH
 LDX j
 LSLX
 LDA @ set.limite_segnalazione_dissimmetria:1,X
 STA var:1
 LDA @ set.limite_segnalazione_dissimmetria,X
 STA var
 LDHX var
 CPHX max
 BEQ Procedi_oltre
 BCC Per_default
 CPHX min
 BCC Procedi_oltre
Per_default:
 LDHX defa
 STHX var
Procedi_oltre:
 CLRH
 LDX j
 LSLX
 LDA var:1
 STA @ set.limite_segnalazione_dissimmetria:1,X
 LDA var
 STA @ set.limite_segnalazione_dissimmetria,X
 LDA k:1
 ADD #54 //6x9
 STA k:1
 LDA k
 ADC #0
 STA k
 INC j
 LDA j
 CMP #4
 BCS Ripeti_ass_default

per_funz_comuni:
 CLR j
 LDHX set.N_tabella_potenza
 LSL k:1
 ROL k
ripeti_Ass_default:
 LDHX k
 LDA @tabella_tensione_nominale,X
 STA defa
 LDA @tabella_tensione_nominale:1,X
 STA defa:1
 LDA @tabella_tensione_nominale:2,X
 STA min
 LDA @tabella_tensione_nominale:3,X
 STA min:1
 LDA @tabella_tensione_nominale:4,X
 STA max
 LDA @tabella_tensione_nominale:5,X
 STA max:1
 CLRH
 LDX j
 LSLX
 LDA @ set.tensione_nominale:1,X
 STA var:1
 LDA @ set.tensione_nominale,X
 STA var
 LDHX var
 CPHX max
 BEQ PRocedi_oltre
 BCC PEr_default
 CPHX min
 BCC PRocedi_oltre
PEr_default:
 LDHX defa
 STHX var
PRocedi_oltre:
 CLRH
 LDX j
 LSLX
 LDA var:1
 STA @ set.tensione_nominale:1,X
 LDA var
 STA @ set.tensione_nominale,X
 LDA k:1
 ADD #90 //6x15
 STA k:1
 LDA k
 ADC #0
 STA k
 INC j
 LDA j
 CMP #4
 BCS ripeti_Ass_default
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
 LDA reset_default
 BNE fine
 LDA leggi_impostazioni
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
 LDX N_bytes_messaggio_fault
 MUL
 STA primo_indirizzo_lettura:2
 STX primo_indirizzo_lettura:1
 LDA allarme_in_lettura
 LDX N_bytes_messaggio_fault
 MUL
 ADD primo_indirizzo_lettura:1
 STA primo_indirizzo_lettura:1
 TXA
 ADC #0
 STA primo_indirizzo_lettura
 
 LDA N_bytes_messaggio_fault
 DECA
 STA ultimo_dato_in_lettura
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
 LDX N_bytes_messaggio_fault
 CLRH
ripeti_assegnazione:
 LDA @buffer_eeprom:-1,X
 STA @buffer_USB:-1,X
 DBNZX ripeti_assegnazione
 LDA #1
 STA pronto_alla_risposta
fine:
 } 
}

const int seicento=600;
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
 
//soglia_tensione = tensione_nominale *1.414*3.313/2 = tensione_nominale * 600/256
 LDA set.tensione_nominale:1
 LDX seicento:1
 MUL
 STX soglia_tensione:1
 LDA set.tensione_nominale
 LDX seicento:1
 MUL
 ADD soglia_tensione:1
 STA soglia_tensione:1
 TXA
 ADC #0
 STA soglia_tensione
 LDA set.tensione_nominale:1
 LDX seicento
 MUL
 ADD soglia_tensione:1
 STA soglia_tensione:1
 TXA
 ADC soglia_tensione
 STA soglia_tensione
 LDA set.tensione_nominale
 LDX seicento
 MUL
 ADD soglia_tensione
 STA soglia_tensione
 CLRA
 SUB soglia_tensione:1
 STA soglia_tensione_neg:1
 CLRA
 SBC soglia_tensione
 STA soglia_tensione_neg
 
//timer_ritorno_da_emergenza=set.ritardo_riaccensione_dopo_emergenza*60;
 LDA set.ritardo_riaccensione_da_emergenza_V:1
 LDX #60
 MUL
 STA timer_ritorno_da_emergenza_V:1
 STX timer_ritorno_da_emergenza_V
 LDA set.ritardo_riaccensione_da_emergenza_V
 LDX #60
 MUL
 ADD timer_ritorno_da_emergenza_V
 STA timer_ritorno_da_emergenza_V
 
//timer_ritorno_da_emergenza=set.ritardo_riaccensione_funzionamento_a_secco*60;
 LDA set.ritardo_riaccensione_funzionamento_a_secco:1
 LDX #60
 MUL
 STA timer_ritorno_da_emergenza_funzionamento_a_secco:1
 STX timer_ritorno_da_emergenza_funzionamento_a_secco
 LDA set.ritardo_riaccensione_funzionamento_a_secco
 LDX #60
 MUL
 ADD timer_ritorno_da_emergenza_funzionamento_a_secco
 STA timer_ritorno_da_emergenza_funzionamento_a_secco
 
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
;//incremento = (corrente_media^2 * delta_Tn / corrente_nominale^2 - delta_T)/set.K_di_tempo_riscaldamento
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

//set.resistenza_PT100_a_0gradi-set.resistenza_PT100_a_100gradi
 LDA set.resistenza_PT100_a_0gradi:1
 SUB set.resistenza_PT100_a_100gradi:1
 STA delta_PT100_0_100gradi:1
 LDA set.resistenza_PT100_a_0gradi
 SBC set.resistenza_PT100_a_100gradi
 STA delta_PT100_0_100gradi

//incremento_volume = scala_sensore_di_flusso[litri*1000/impulso]/1000*65536;
 LDHX #125
 LDA set.scala_sensore_di_flusso
 DIV
 STA prod
 LDA set.scala_sensore_di_flusso:1
 DIV
 STA prod:1
 CLRA
 DIV
 STA prod:2
 CLRA
 DIV
 STA prod:3
 LSR prod
 ROR prod:1
 ROR prod:2
 ROR prod:3
 LSR prod
 ROR prod:1
 ROR prod:2
 ROR prod:3
 LSR prod
 ROR prod:1
 ROR prod:2
 ROR prod:3
 LDHX prod
 STHX incremento_volume
 LDHX prod:2
 STHX incremento_volume:2
 }
}

//----------------------lettura data e ora----------------------------------
void sequenza_lettura_scrittura_ram(void)
{
asm
 {
;//----------scrivi e leggi la ram dell'orologio---------
;/*read write
;  81   80       CH(7) secondi (stop con CH=1)
;  83   82       minuti 
;  85   84       ore
;  87   86       giorno 
;  89   88       mese
;  8d   8c       anno
;  
;  c1   c0       inizio ram
;  ------------------------    
;  fd   fc       fine ram
;scrivi: dato pronto con clock in salita
;commuta da scrittura a lettura con clock in salita ed I/O alto
;leggi: dato commutato con clock in salita, pronto con clock in discesa

;PTC2 I/O ram                      ;GPIOF_DR.1 //I/O
;PTC3 clock ram                    ;GPIOF_DR.2 //clock
;PTC4 abilita ram con livello alto ;GPIOF_DR.3 //reset
;*/
;if(leggi_ram)
; {
; if(leggi_ram==1)//abbassa clock, I/O out=1
;  {
;  PTCD &=0xfe;//abbassa clock
;  PTCDD |=0x02;//dato in uscita
;  PTCD |=0x02;//I/O
;  PTCD |=0x04;//reset
;  leggi_ram++;
;  }
; else if(leggi_ram==2)//alza clock
;  {
;  contatore_ram=0x02;
;  indirizzo_ram |=0x80;
;  PTCD |=0x01;//alza clock
;  leggi_ram++;
;  }
; else if(leggi_ram<15)
;  {
;  if(leggi_ram & 0x01)//abbassa clock, indirizzo
;   {
;   if(indirizzo_ram & contatore_ram) PTCD |=0x02; else PTCD &=0xfd;
;   contatore_ram <<=1;
;   PTCD &=0xfe;//abbassa clock
;   }
;  else//alza clock
;   {
;   PTCD |=0x01;
;   } 
;  leggi_ram++;
;  }
; else if(leggi_ram==15)
;  {
;  PTCD |=0x02;//alza I/O
;  PTCD &=0xfe;//abbassa clock
;  leggi_ram++;
;  }   
; else if(leggi_ram==16)
;  {
;  PTCD |=0x01;//alza clock
;  leggi_ram++;
;  }   
; else if(leggi_ram==17)
;  {
;  PTCD &=0xfe;//abbassa clock
;  PTCDD &=0xfd;//inversione dati
;  contatore_ram=0x01;
;  dato_ram=0;
;  leggi_ram++;
;  }   
; else if(leggi_ram<33)
;  {
;  if(leggi_ram & 0x01) //abbassa clock
;   {
;   PTCD &=0xfe;
;   }
;  else//leggi dato e alza clock
;   {
;   if(PTCD & 0x02) dato_ram |=contatore_ram;
;   contatore_ram <<=1;
;   PTCD |=0x01;//alza clock
;   } 
;  leggi_ram++;
;  }
; else
;  {
;  leggi_ram=0;
;  PTCD &=0xfa;//reset basso, clock basso
;  }
; }

  LDA leggi_ram ;//lettura ram
  BEQ per_scrittura_ram
  CMP #1
  BNE alza_clock1
;  PTCD &=0xfe;//abbassa clock
;  PTCDD |=0x02;//dato in uscita
;  PTCD |=0x02;//I/O
;  PTCD |=0x04;//reset
;  leggi_ram++;
  BCLR 0,_PTCD//abbassa clock
  BSET 1,_PTCDD
  BSET 1,_PTCD
  BSET 2,_PTCD
  LDA leggi_ram
  INCA
  STA leggi_ram 
  BRA fine
 alza_clock1:
  CMP #2
  BNE indirizzo_lettura
;  contatore_ram=0x02;
;  indirizzo_ram |=0x80;
;  PTCD |=0x01;//alza clock
;  leggi_ram++;
  LDA #$02
  STA contatore_ram
  LDA indirizzo_ram
  ORA #$80
  STA indirizzo_ram
  BSET 0,_PTCD//alza clock
  LDA leggi_ram
  INCA
  STA leggi_ram 
  BRA fine
 indirizzo_lettura: 
  CMP #15
  BCC chiusura_indirizzo
;  if(leggi_ram & 0x01)//abbassa clock, indirizzo
;   {
;   if(indirizzo_ram & contatore_ram) PTCD |=0x02; else PTCD &=0xfd;
;   contatore_ram <<=1;
;   PTCD &=0xfe;//abbassa clock
;   }
;  else//alza clock
;   {
;   PTCD |=0x01;
;   } 
;  leggi_ram++;
  LDA leggi_ram
  AND #$01
  BEQ alza_clock2
  LDA indirizzo_ram
  AND contatore_ram
  BEQ uscita_bassa
  BSET 1,_PTCD
  BRA scorri_contatore_ram
 uscita_bassa:
  BCLR 1,_PTCD 
 scorri_contatore_ram:
  LDA contatore_ram
  LSLA
  STA contatore_ram 
  BCLR 0,_PTCD//abbassa clock
 alza_clock2:  
  BSET 0,_PTCD//alza clock
 increm_leggi_ram:
  LDA leggi_ram
  INCA
  STA leggi_ram 
  BRA fine
 chiusura_indirizzo:
  BNE precede_inversione_IO
;  PTCD |=0x02;//alza I/O
;  PTCD &=0xfe;//abbassa clock
;  leggi_ram++;
  BSET 1,_PTCD 
  BCLR 0,_PTCD//abbassa clock
  LDA leggi_ram
  INCA
  STA leggi_ram 
  BRA fine
  
 precede_inversione_IO:
  CMP #16
  BNE inversione_IO
;  PTCD |=0x01;//alza clock
;  leggi_ram++;
  BSET 0,_PTCD//alza clock
  LDA leggi_ram
  INCA
  STA leggi_ram 
  BRA fine

inversione_IO:
  CMP #17
  BNE lettura_dato
;  PTCD &=0xfe;//abbassa clock
;  PTCDD &=0xfd;//inversione dati
;  contatore_ram=0x01;
;  dato_ram=0;
;  leggi_ram++;
  BCLR 0,_PTCD//abbassa clock
  BCLR 1,_PTCDD
  LDA #$01
  STA contatore_ram
  CLRA
  STA dato_ram
  LDA leggi_ram
  INCA
  STA leggi_ram 
  BRA fine
 lettura_dato: 
  CMP #33
  BCC completamento_dato
;  if(leggi_ram & 0x01) //abbassa clock
;   {
;   PTCD &=0xfe;
;   }
;  else//leggi dato e alza clock
;   {
;   if(PTCD & 0x02) dato_ram |=contatore_ram;
;   contatore_ram <<=1;
;   PTCD |=0x01;//alza clock
;   } 
;  leggi_ram++;
  LDA leggi_ram
  AND #$01
  BEQ alza_clock3
  BCLR 0,_PTCD//abbassa clock
  BRA inc_leggi_ram
 alza_clock3: 
  BRCLR 1,_PTCD,Scorri_contatore_ram
  LDA dato_ram
  ORA contatore_ram
  STA dato_ram
 Scorri_contatore_ram: 
  LDA contatore_ram
  LSLA
  STA contatore_ram
  BSET 0,_PTCD//alza clock
 inc_leggi_ram:
  LDA leggi_ram
  INCA
  STA leggi_ram 
  BRA fine

 completamento_dato:
  CLRA
  STA leggi_ram
  BCLR 0,_PTCD//abbassa clock
  BCLR 2,_PTCD//reset
  BRA fine
  
  
 per_scrittura_ram://scrittura ram
;else if(scrivi_ram)
; {
; if(scrivi_ram==1)//abbassa clock, I/O out=0
;  {
;  PTCD &=0xfe;//abbassa clock
;  PTCDD |=0x02;//dato in uscita
;  PTCD &=0xfd;//I/O
;  PTCD |=0x04;//reset
;  scrivi_ram++;
;  }
; else if(scrivi_ram==2)//alza clock
;  {
;  contatore_ram=0x02;
;  indirizzo_ram |=0x80;
;  PTCD |=0x01; //alza clock
;  scrivi_ram++;
;  }
; else if(scrivi_ram<17)
;  {
;  if(leggi_ram & 0x01)//abbassa clock, indirizzo
;   {
;   if(indirizzo_ram & contatore_ram) PTCD |=0x04; else PTCD &=0xfb;
;   contatore_ram <<=1;
;   PTCD &=0xfe;//abbassa clock
;   }
;  else//alza clock
;   {
;   PTCD |=0x01;
;   } 
;  leggi_ram++;
;  }
; else if(scrivi_ram==17)
;  {
;  PTCD &=0xfe;//abbassa clock
;  contatore_ram=0x01;
;  if(dato_ram & contatore_ram) PTCD |=0x02; else PTCD &=0xfd;
;  scrivi_ram++;
;  }
; else if(scrivi_ram<33)
;  {
;  if(scrivi_ram & 0x01)// abbassa clock
;   {
;   PTCD &=0xfe;//abbassa clock
;   contatore_ram <<=1;
;   if(dato_ram & contatore_ram) PTCD |=0x02; else PTCD &=0xfd;
;   scrivi_ram++;
;   }
;  else // alza clock
;   {
;   PTCD |=0x01;
;   scrivi_ram++;
;   }
;  }   
; else
;  {
;  scrivi_ram=0; 
;  PTCD &=0xfa;//reset basso, clock basso
;  }
; } 
  LDA scrivi_ram
  BEQ fine
  CMP #1
  BNE alza_clock4
;  PTCD &=0xfe;//abbassa clock
;  PTCDD |=0x02;//dato in uscita
;  PTCD &=0xfd;//I/O
;  PTCD |=0x04;//reset
;  scrivi_ram++;
  BCLR 0,_PTCD//abbassa clock
  BSET 1,_PTCDD
  BCLR 1,_PTCD
  BSET 2,_PTCD
  LDA scrivi_ram
  INCA
  STA scrivi_ram
  BRA fine
 alza_clock4: 
  CMP #2
  BNE indirizzo_scrittura
;  contatore_ram=0x02;
;  indirizzo_ram |=0x80;
;  PTCD |=0x01; //alza clock
;  scrivi_ram++;
  LDA indirizzo_ram
  ORA #$80
  STA indirizzo_ram
  LDA #$02
  STA contatore_ram
  BSET 0,_PTCD//alza clock
  LDA scrivi_ram
  INCA
  STA scrivi_ram
  BRA fine
 indirizzo_scrittura:
  CMP #17
  BCC last_bit_addr
;  if(leggi_ram & 0x01)//abbassa clock, indirizzo
;   {
;   if(indirizzo_ram & contatore_ram) PTCD |=0x02; else PTCD &=0xfd;
;   contatore_ram <<=1;
;   PTCD &=0xfe;//abbassa clock
;   }
;  else//alza clock
;   {
;   PTCD |=0x01;
;   } 
;  leggi_ram++;
  LDA scrivi_ram
  AND #$01
  BEQ alza_clock5
  LDA indirizzo_ram
  AND contatore_ram
  BEQ uscita_Bassa
  BSET 1,_PTCD
  BRA scorri_conta_ram
 uscita_Bassa:
  BCLR 1,_PTCD
 scorri_conta_ram:
  LDA contatore_ram
  LSLA
  STA contatore_ram  
  BCLR 0,_PTCD//abbassa clock
  BRA inc_scrivi_ram
 alza_clock5:
  BSET 0,_PTCD//alza clock
 inc_scrivi_ram:
  LDA scrivi_ram
  INCA
  STA scrivi_ram
  BRA fine
 last_bit_addr:
  BNE scorri_dato
;  PTCD &=0xfe;
;  contatore_ram=0x01;
;  if(dato_ram & contatore_ram) PTCD |=0x02; else PTCD &=0xfd;
;  scrivi_ram++;
  BCLR 0,_PTCD//abbassa clock
  LDA #$01 ;//#$02
  STA contatore_ram
  LDA dato_ram
  AND contatore_ram
  BEQ daTo_basso
  BSET 1,_PTCD
  BRA incremento_conta
 daTo_basso:
  BCLR 1,_PTCD 
 incremento_conta:
  LDA scrivi_ram
  INCA
  STA scrivi_ram
  BRA fine

 scorri_dato: 
  CMP #33
  BCC conclusione_scrittura  
;  if(scrivi_ram & 0x01)// abbassa clock
;   {
;   PTCD &=0xfe;
;   contatore_ram <<=1;
;   if(dato_ram & contatore_ram) PTCD |=0x02; else PTCD &=0xfd;
;   scrivi_ram++;
;   }
;  else // alza clock
;   {
;   PTCD |=0x01;
;   scrivi_ram++;
;   }
  LDA scrivi_ram
  AND #$01
  BEQ secondo_step
  BCLR 0,_PTCD//abbassa clock
  LDA contatore_ram
  LSLA
  STA contatore_ram
  LDA dato_ram
  AND contatore_ram
  BEQ daTo_Basso
  BSET 1,_PTCD
  BRA scorrimento_conta
 daTo_Basso:
  BCLR 1,_PTCD 
 scorrimento_conta:
  LDA scrivi_ram
  INCA
  STA scrivi_ram
  BRA fine
 secondo_step:
  BSET 0,_PTCD//alza clock
  LDA scrivi_ram
  INCA
  STA scrivi_ram
  BRA fine
conclusione_scrittura:  
  CLRA
  STA scrivi_ram
  BCLR 0,_PTCD//abbassa clock
  BCLR 2,_PTCD
fine:
 }
}

//---------------------------------------------------------------
void leggi_data_ora(void)
{
/*
 if(conta_data==0)//giorno
  {
  indirizzo_ram=0x0086;
  leggi_ram=1;
  conta_data++;
  }
 else if(conta_data==1)//assegna giorno
  {
  if(leggi_ram==0)
   {
   giorno[0]=((dato_ram & 0x03f)>>4);
   giorno[1]=(dato_ram & 0x0f);
   Giorno=dato_ram/16*10 + (dato_ram & 0x0f);
   indirizzo_ram=0x0088;
   leggi_ram=1;
   conta_data++;
   }
  }
 else if(conta_data==2)//assegna mese
  {
  if(leggi_ram==0)
   {
   mese[0]=((dato_ram & 0x01f)>>4);
   mese[1]=(dato_ram & 0x0f);
   Mese=dato_ram/16*10 + (dato_ram & 0x0f);
   indirizzo_ram=0x008c;
   leggi_ram=1;
   conta_data++;
   }
  }
 else if(conta_data==3)//assegna anno
  {
  if(leggi_ram==0)
   {
   anno[0]=((dato_ram & 0x03f)>>4);
   anno[1]=(dato_ram & 0x0f);
   Anno=dato_ram/16*10 + (dato_ram & 0x0f);
   indirizzo_ram=0x0084;
   leggi_ram=1;
   conta_data++;
   }
  }
 else if(conta_data==4)//assegna ore
  {
  if(leggi_ram==0)
   {
   ora[0]=0x30 | ((dato_ram & 0x03f)>>4);
   ora[1]=0x30 | (dato_ram & 0x0f);
   Ora=dato_ram/16*10 + (dato_ram & 0x0f);
   indirizzo_ram=0x0082;
   leggi_ram=1;
   conta_data++;
   }
  }
 else if(conta_data==5)//assegna minuti
  {
  if(leggi_ram==0)
   {
   minuto[0]=((dato_ram & 0x07f)>>4);
   minuto[1]=(dato_ram & 0x0f);
   Minuto=dato_ram/16*10 + (dato_ram & 0x0f);
   indirizzo_ram=0x0080;
   leggi_ram=1;
   conta_data++;
   }
  }
 else //assegna secondi
  {
  if(leggi_ram==0)
   {
   secondo[0]=((dato_ram & 0x07f)>>4);
   secondo[1]=(dato_ram & 0x0f);
   Secondo=dato_ram/16*10 + (dato_ram & 0x0f);
   conta_data=0;
   data_letta=1;
   timer_presenta_data=950;
   }
  }
  81   80       CH(7) secondi (stop con CH=1)
  83   82       minuti 
  85   84       ore
  87   86       giorno 
  89   88       mese
  8d   8c       anno
*/
asm
 {
 LDHX timer_presenta_data
 BNE fine
 LDA leggi_ram
 BNE fine
 LDA scrivi_ram
 BNE fine
 
 LDA conta_data
 BNE presenta_giorno
 LDA #$87
 STA indirizzo_ram 
 LDA #1
 STA leggi_ram
 LDA conta_data
 INCA
 STA conta_data
 BRA fine
presenta_giorno: 
 CMP #1
 BNE presenta_mese
;   giorno[0]=((dato_ram & 0x03f)>>4);
;   giorno[1]=(dato_ram & 0x0f);
;   Giorno=dato_ram/16*10 + (dato_ram & 0x0f);
 LDA dato_ram
 AND #$3f
 LSRA
 LSRA
 LSRA
 LSRA
 STA giorno
 LDX #10
 MUL
 STA Giorno
 LDA dato_ram
 AND #$0f
 STA giorno:1
 ADD Giorno
 STA Giorno
  
 LDA #$89
 STA indirizzo_ram 
 LDA #1
 STA leggi_ram
 LDA conta_data
 INCA
 STA conta_data
 BRA fine
presenta_mese: 
 CMP #2
 BNE presenta_anno
;   mese[0]=((dato_ram & 0x01f)>>4);
;   mese[1]=(dato_ram & 0x0f);
 LDA dato_ram
 AND #$1f
 LSRA
 LSRA
 LSRA
 LSRA
 STA mese
 LDX #10
 MUL
 STA Mese
 LDA dato_ram
 AND #$0f
 STA mese:1
 ADD Mese
 STA Mese
 
 LDA #$8d
 STA indirizzo_ram 
 LDA #1
 STA leggi_ram
 LDA conta_data
 INCA
 STA conta_data
 BRA fine
presenta_anno:
 CMP #3
 BNE presenta_ora
;  anno[0]=((dato_ram & 0x03f)>>4);
;  anno[1]=(dato_ram & 0x0f);
 LDA dato_ram
 AND #$3f
 LSRA
 LSRA
 LSRA
 LSRA
 STA anno
 LDX #10
 MUL
 STA Anno
 LDA dato_ram
 AND #$0f
 STA anno:1
 ADD Anno
 STA Anno
 
 LDA #$85;//indirizzo ora
 STA indirizzo_ram 
 LDA #1
 STA leggi_ram
 LDA conta_data
 INCA
 STA conta_data
 BRA fine
presenta_ora: 
 CMP #4
 BNE presenta_minuto
;   ora[0]=((dato_ram & 0x03f)>>4);
;   ora[1]=(dato_ram & 0x0f);
 LDA dato_ram
 AND #$3f
 LSRA
 LSRA
 LSRA
 LSRA
 STA ora
 LDX #10
 MUL
 STA Ora
 LDA dato_ram
 AND #$0f
 STA ora:1
 ADD Ora
 STA Ora
 
 LDA #$83
 STA indirizzo_ram 
 LDA #1
 STA leggi_ram
 LDA conta_data
 INCA
 STA conta_data
 BRA fine
presenta_minuto:
 CMP #5
 BNE presenta_secondo
;   minuto[0]=((dato_ram & 0x07f)>>4);
;   minuto[1]=(dato_ram & 0x0f);
 LDA dato_ram
 AND #$7f
 LSRA
 LSRA
 LSRA
 LSRA
 STA minuto
 LDX #10
 MUL
 STA Minuto
 LDA dato_ram
 AND #$0f
 STA minuto:1
 ADD Minuto
 STA Minuto
 
 LDA #$81
 STA indirizzo_ram 
 LDA #1
 STA leggi_ram
 LDA conta_data
 INCA
 STA conta_data
 BRA fine
presenta_secondo:
;   secondo[0]=((dato_ram & 0x07f)>>4);
;   secondo[1]=(dato_ram & 0x0f);
 LDA dato_ram
 AND #$7f
 LSRA
 LSRA
 LSRA
 LSRA
 STA secondo
 LDX #10
 MUL
 STA Secondo
 LDA dato_ram
 AND #$0f
 STA secondo:1
 ADD Secondo
 STA Secondo
 
 LDHX #950
 STHX timer_presenta_data
 LDA #1
 STA data_letta 
 CLRA
 STA conta_data
fine:
 }
}

void presenta_data_ora(void)
{
unsigned char j;
asm
 {
 LDA data_letta
 BEQ fine
 CLRA
 STA data_letta

 CLR j
 CLRH
primo_messaggio:
 LDX j
 LDA @presentazione_iniziale,X
 STA @display,X
 INC j
 LDA j
 CMP #17
 BCS primo_messaggio
 
 LDA giorno
 ORA #$30
 STA cifre
 LDA giorno:1
 ORA #$30
 STA cifre:1
 LDA #'-'
 STA cifre:2
 LDA mese
 ORA #$30
 STA cifre:3
 LDA mese:1
 ORA #$30
 STA cifre:4
 LDA #' '
 STA cifre:5
 STA cifre:6
 STA cifre:7
 LDA ora
 ORA #$30
 STA cifre:8
 LDA ora:1
 ORA #$30
 STA cifre:9 
 LDA #':'
 STA cifre:10
 LDA minuto
 ORA #$30
 STA cifre:11
 LDA minuto:1
 ORA #$30
 STA cifre:12
 LDA #':'
 STA cifre:13
 LDA secondo
 ORA #$30
 STA cifre:14
 LDA secondo:1
 ORA #$30
 STA cifre:15
 
;for(j=0; j<16; j++)
; {
; display[riga][j]=cifre[j];
; }
 
 CLR j
Ripeti: 
 CLRH
 LDX j
 LDA @cifre,X
 STA @display:17,X
 INC j
 LDA j
 CMP #16
 BNE Ripeti
fine:  
 }
}

//----------------nuova data------------------------------------------------------------
void registra_nuova_data(char dato, char indirizzo)
{
/*read write
  81   80       CH(7) secondi (stop con CH=1)
  83   82       minuti 
  85   84       ore
  87   86       giorno 
  89   88       mese
  8d   8c       anno*/
asm
 {
 LDA scrivi_ram
 BNE fine
 LDA leggi_ram
 BNE fine
 LDA modificato
 BEQ fine
 CLRA
 STA modificato
 
 LDA indirizzo
 STA indirizzo_ram
 
 LDHX #10
 LDA dato
 DIV
 PSHH
 LDX #16
 MUL
 STA dato_ram
 PULA
 ORA dato_ram
 STA dato_ram
 LDA #1
 STA scrivi_ram
fine:
 }
}

void modifica_carattere(char *var, char min, char max)
{
char variab;
asm 
 {
 LDHX var
 LDA ,X
 STA variab
 
 LDA salita_piu
 BEQ vedi_meno
 CLRA
 STA salita_piu
 LDA max
 CMP variab
 BEQ fine
 BCC aggiorna
 STA variab
 BRA fine
aggiorna: 
 INC variab
 LDA #1
 STA modificato
 BRA fine
vedi_meno: 
 LDA salita_meno
 BEQ fine
 CLRA
 STA salita_meno
 LDA min
 CMP variab
 BCS Aggiorna
 STA variab
 BRA fine
Aggiorna: 
 DEC variab
 LDA #1
 STA modificato
fine:
 LDHX var
 LDA variab
 STA ,X
 }
}

void presenta_2_cifre(char val, char riga, char colonna)
{
char j, inizio;
asm
 {
 LDA colonna
 CMP #15
 BCC fine
 LDA riga
 CMP #2
 BCC fine
 LDX #17
 MUL
 ADD colonna
 STA inizio
 
 CLRA
 STA cifre 
 LDA #2
 STA j
ripeti: 
 CLRH
 LDA val
 LDX #10
 DIV
 STA val
 PSHH
 PULA
 LDX j
 CLRH
 STA @cifre,X
 DBNZ j,ripeti
  
 LDA #1
 STA j
RIpeti:
 CLRH
 LDX j
 LDA @cifre,X
 ORA #$30
 LDX inizio
 CLRH
 STA @display,X
 INC inizio
 INC j
 LDA j
 CMP #3
 BNE RIpeti 
fine: 
 }
}

void modifica_data_ora(void)
{
if(salita_func)
 {
 salita_func=0;
 switch(cursore_sottomenu)
  {
  case 10: cursore_sottomenu=13; break;
  case 13: cursore_sottomenu=17; break;
  case 17: cursore_sottomenu=24; break;
  case 24: cursore_sottomenu=27; break;
  case 27: cursore_sottomenu=30; break;
  case 30: cursore_sottomenu=10; break;
  default: cursore_sottomenu=10; break;
  }
 }
switch(cursore_sottomenu)
 {
 case 10:
  {
  modifica_carattere((char*)&Anno,0,99);
  registra_nuova_data(Anno,0x8c);
  } break;
 case 13:
  {
  modifica_carattere((char*)&Mese,1,12);
  registra_nuova_data(Mese,0x88);
  } break; 
 case 17:
  {
  modifica_carattere((char*)&Giorno,1,31);
  registra_nuova_data(Giorno,0x86);
  } break; 
 case 24:
  {
  modifica_carattere((char*)&Ora,0,23);
  registra_nuova_data(Ora,0x84);
  } break; 
 case 27:
  {
  modifica_carattere((char*)&Minuto,0,59);
  registra_nuova_data(Minuto,0x82);
  } break; 
 case 30:
  {
  modifica_carattere((char*)&Secondo,0,59);
  registra_nuova_data(Secondo,0x80);
  } break;
 }
//"13)date:20  -   ",//35
//"  -      :  :   ",
asm
 {
 LDA #'2'
 STA display:8
 LDA #'0'
 STA display:9
 LDA #'-'
 STA display:12
 STA display:15
 LDA #':'
 STA display:26
 STA display:29
 STA display:32
 }
presenta_2_cifre(Anno,0,10);
presenta_2_cifre(Mese,0,13);
presenta_2_cifre(Giorno,1,0);
presenta_2_cifre(Ora,1,7);
presenta_2_cifre(Minuto,1,10);
presenta_2_cifre(Secondo,1,13);
}


//-----------------------------------------------------------------------------------------
void programmazione(void)
{
long secondi, KWh;
if(toggle_func==0)
 {
 if(salita_func)
  {
  salita_func=0;
  toggle_func=1;
  timer_reset=5;
  allarme_in_lettura=0;
  cursore_menu=0;
  salita_piu=0;
  salita_meno=0;
  salita_start=0;
  toggle_start=0;
  toggle_enter=0;
  start=0;
  segnalazione_=0;
  timer_aggiorna_menu=0;
  if(numero_ingresso==chiave_ingresso)
   {
   prepara_chiave=0;
   cursore_menu=1;
   }
  else if(numero_ingresso==chiave_ingresso2)
   {
   prepara_chiave=0;
   cursore_menu=14;
   }
  else
   {
   prepara_chiave=1;
   cursore_menu=0;
   }
  presenta_menu((unsigned char*)&menu_principale,0,tot_righe_menu,cursore_menu<<1);
  }
 }
 
if(salita_esc)
 {
 salita_esc=0;
 if(toggle_func)
  {
  N_cifre_lampeggianti=0;
  if(toggle_enter)
   {
   toggle_enter=0;
   }
  else
   {
   salita_piu=0;
   salita_meno=0;
   cursore_sottomenu=0;
   if(toggle_start)//spegne motore
    {
    toggle_start=0;
    salita_stop=0;
    segnalazione_=0;
    timer_riavviamento=2;//attesa minima per riavviamento
    tentativi_avviamento=N_tentativi_motore_bloccato;
    tentativi_avviamento_a_secco=N_tentativi_motore_bloccato;
    } 
   else 
    {
    toggle_func=0;
    toggle_enter=0;
    timer_reset=5;
    calcolo_delle_costanti();
  //comando di salvataggio in eeprom
    salvataggio_funzioni=1;
    presenta_data_ora();//presenta_menu((unsigned char*)&comandi_da_effettuare,0,2,0);
    }
   }
  }
 }
 
if(toggle_func)//lettura e modifica delle funzioni
 {
 if(toggle_enter==0)//presenta menu_principale
  {
  if(salita_enter)
   {
   salita_enter=0;
   if((cursore_menu==1)||(cursore_menu==13)||(cursore_menu==15)) toggle_enter=2; else toggle_enter=1;
   } 
  set.motore_on=0;
  cursore_sottomenu=0;
  if(prepara_chiave)
   {
   modifica_unsigned((unsigned int*)&numero_ingresso,0,9999,0);
   presenta_unsigned(numero_ingresso,0,1,10,4);
   N_cifre_lampeggianti=4;
   } 
  else if(timer_aggiorna_menu==0) //attesa per presentazione accesso abilitato
   {
   N_cifre_lampeggianti=0;
   if(salita_meno)
    {
    if(numero_ingresso==chiave_ingresso2)
     {
     if(cursore_menu<15) cursore_menu++; else cursore_menu=1;
     } 
    else if(numero_ingresso==chiave_ingresso)
     {
     if(cursore_menu<12) cursore_menu++; else cursore_menu=1;
     }
    salita_meno=0;
    }
   else if(salita_piu)
    {
    if(numero_ingresso==chiave_ingresso2)
     {
     if(cursore_menu>1) cursore_menu--; else cursore_menu=15;
     } 
    else if(numero_ingresso==chiave_ingresso)
     {
     if(cursore_menu>1) cursore_menu--; else cursore_menu=12;
     }
    salita_piu=0;
    }
   if(cursore_menu==1) //potenza motore
    {
    presenta_scritta((unsigned char*)&menu_principale,34,0,tot_righe_menu,0,0,16);
    presenta_scritta((unsigned char*)&presentazione_iniziale,(set.N_tabella_potenza+1)*17,0,0,1,0,16);
    } 
   else if(cursore_menu==13) //nunmero di serie
    {
    presenta_scritta((unsigned char*)&menu_principale,(int)cursore_menu*34,0,tot_righe_menu,0,0,16);
    presenta_scritta((unsigned char*)&menu_principale,(int)cursore_menu*34+17,0,tot_righe_menu,1,0,7);
    presenta_unsigned(set.numero_serie,0,1,7,5);
    } 
   else if(cursore_menu==15)
    {
    presenta_scritta((unsigned char*)&menu_principale,(int)cursore_menu*34,0,tot_righe_menu,0,0,16);//RESET
    presenta_scritta((char*)&attivazione_comando_reset,17,0,1,1,0,16);//NOT
    }
   else presenta_menu((unsigned char*)&menu_principale,0,tot_righe_menu,cursore_menu<<1);
   }
  }

 else if(toggle_enter==1)//presenta sottomenu
  {
  if(salita_enter)
   {
   salita_enter=0;
   toggle_enter=2;
   } 
  N_cifre_lampeggianti=0;
  if(timer_aggiorna_menu==0)
   {
   timer_aggiorna_menu=200;
   switch(cursore_menu)
    {
    case 0://chiave
     {
     toggle_enter=0;
     if(numero_ingresso==chiave_ingresso)
      {
      prepara_chiave=0;
      cursore_menu=1;
      presenta_scritta((char*)&accesso_password,0,0,0,1,0,16);
      timer_aggiorna_menu=1000;
      }
     else if(numero_ingresso==chiave_ingresso2)
      {
      prepara_chiave=0;
      cursore_menu=1;
      presenta_scritta((char*)&accesso_password,0,0,0,1,0,16);
      timer_aggiorna_menu=1000;
      }
     else
      {
      prepara_chiave=0;
      cursore_menu=1;
      }
     } break;
     /*
    case 1://potenza motore
     {
     presenta_scritta((unsigned char*)&menu_principale,34,0,tot_righe_menu,0,0,16);
     presenta_scritta((unsigned char*)&presentazione_iniziale,(set.N_tabella_potenza+1)*17,0,0,1,0,16);
     } break;
     */
    case 2://dati_motore
     {
     if(salita_meno)
      {
      salita_piu=0;
      cursore_sottomenu=1;
      }
     else if(salita_piu)
      {
      salita_meno=0;
      cursore_sottomenu=0;
      }
     presenta_menu((unsigned char*)&dati_motore,0,4,cursore_sottomenu<<1);
     switch(cursore_sottomenu)
      {
      case 0: presenta_unsigned(set.tensione_nominale,0,1,11,3); break;
      case 1: presenta_unsigned(set.corrente_nominale,1,1,11,3); break;
      }
     } break;
    case 3://protezione_voltaggio
     {
     if(salita_meno)
      {
      salita_piu=0;
      if(cursore_sottomenu<6) cursore_sottomenu++;
      if(mono_tri_fase==0)
       {
       if((cursore_sottomenu==3)||(cursore_sottomenu==4)) cursore_sottomenu=5;
       }
      } 
     else if(salita_piu)
      {
      salita_meno=0;
      if(cursore_sottomenu>0) cursore_sottomenu--;
      if(mono_tri_fase==0)
       {
       if((cursore_sottomenu==3)||(cursore_sottomenu==4)) cursore_sottomenu=2;
       }
      }
     presenta_menu((unsigned char*)&protezione_voltaggio,0,14,cursore_sottomenu<<1);
     switch(cursore_sottomenu)
      {
      case 0: presenta_unsigned(set.limite_sovratensione,0,1,10,3); break;
      case 1: presenta_unsigned(set.limite_sottotensione,0,1,10,3); break;
      case 2: presenta_unsigned(set.tensione_restart,0,1,10,3); break;
      case 3: presenta_unsigned(set.limite_segnalazione_dissimmetria,0,1,10,3); break;
      case 4: presenta_unsigned(set.limite_intervento_dissimmetria,0,1,10,3); break;
      case 5: presenta_unsigned(set.ritardo_protezione_tensione,0,1,10,3); break;
      case 6: presenta_unsigned(set.ritardo_riaccensione_da_emergenza_V,0,1,10,3); break;
      }
     } break;
    case 4://protezione_corrente
     {
     if(salita_meno)
      {
      salita_piu=0;
      if(cursore_sottomenu<5) cursore_sottomenu++;
      if(mono_tri_fase==0)
       {
       if((cursore_sottomenu==2)||(cursore_sottomenu==3)) cursore_sottomenu=4;
       }
      } 
     else if(salita_piu)
      {
      salita_meno=0;
      if(cursore_sottomenu>0) cursore_sottomenu--;
      if(mono_tri_fase==0)
       {
       if((cursore_sottomenu==2)||(cursore_sottomenu==3)) cursore_sottomenu=1;
       }
      }
     presenta_menu((unsigned char*)&protezione_voltaggio,0,12,cursore_sottomenu<<1);
     switch(cursore_sottomenu)
      {
      case 0: presenta_unsigned(set.limite_sovracorrente,0,1,10,3); break;
      case 1: presenta_unsigned(set.limite_segnalazione_squilibrio,0,1,11,3); break;
      case 2:presenta_unsigned(set.limite_intervento_squilibrio,0,1,11,3); break;
      case 3: presenta_unsigned(set.ritardo_protezione_squilibrio,0,1,12,3); break;
      case 4: presenta_unsigned(set.ritardo_riaccensione_da_emergenza_I,0,1,12,3); break;
      }
     } break;
    case 5://controllo_pressione
     {
     if(salita_meno)
      {
      salita_piu=0;
      if((set.abilita_sensore_pressione)&&(cursore_sottomenu<8)) cursore_sottomenu++;
      if(set.modo_start_stop==0)
       {
       if((cursore_sottomenu==3)||(cursore_sottomenu==4)) cursore_sottomenu=5;
       }
      } 
     else if(salita_piu)
      {
      salita_meno=0;
      if(cursore_sottomenu>0) cursore_sottomenu--;
      if(set.modo_start_stop==0)
       {
       if((cursore_sottomenu==3)||(cursore_sottomenu==4)) cursore_sottomenu=2;
       }
      }
     presenta_menu((unsigned char*)&controllo_pressione,0,18,cursore_sottomenu<<1);
     switch(cursore_sottomenu)
      {
      case 0: if(set.abilita_sensore_pressione) presenta_scritta((char*)&lettura_allarmi,22,0,24,1,8,7); else presenta_scritta((char*)&lettura_allarmi,5,0,24,1,8,7); break;
      case 1: presenta_unsigned(set.pressione_emergenza,1,1,9,3); break;
      case 2: if(set.modo_start_stop) presenta_scritta((char*)&presenta_tipo_start_stop,17,0,2,1,0,16); else presenta_scritta((char*)&presenta_tipo_start_stop,0,0,2,1,0,16); break;
      case 3: presenta_unsigned(set.pressione_spegnimento,1,1,9,3); break;
      case 4: presenta_unsigned(set.pressione_accensione,1,1,9,3); break;
      case 5: presenta_unsigned(set.corrente_minima_sensore,1,1,10,3); break;
      case 6: presenta_unsigned(set.corrente_massima_sensore,1,1,10,3); break;
      case 7: presenta_unsigned(set.portata_sensore_pressione,1,1,10,3); break;
      case 8: presenta_unsigned(set.timer_ritorno_da_emergenza_sensore,0,1,10,3); break;
      }
     } break;
    case 6://controllo_potenza
     {
     if(salita_meno)
      {
      salita_piu=0;
      if(cursore_sottomenu<5) cursore_sottomenu++;
      } 
     else if(salita_piu)
      {
      salita_meno=0;
      if(cursore_sottomenu>0) cursore_sottomenu--;
      }
     presenta_menu((unsigned char*)&controllo_potenza,0,12,cursore_sottomenu<<1);
     switch(cursore_sottomenu)
      {
      case 0: presenta_unsigned(set.potenza_minima_mandata_chiusa,0,1,10,3); break;
      case 1: presenta_unsigned(set.ritardo_stop_mandata_chiusa,0,1,11,3); break;
      case 2: presenta_unsigned(set.ritardo_riaccensione_mandata_chiusa,0,1,11,3); break;
      case 3: presenta_unsigned(set.potenza_minima_funz_secco,0,1,10,3); break;
      case 4: presenta_unsigned(set.ritardo_stop_funzionamento_a_secco,0,1,11,3); break;
      case 5: presenta_unsigned(set.ritardo_riaccensione_funzionamento_a_secco,0,1,10,3); break;
      }
     } break;
    case 7://sensore_di_flusso
     {
     if(salita_meno)
      {
      salita_piu=0;
      if((set.abilita_sensore_flusso)&&(cursore_sottomenu<3)) cursore_sottomenu++;
      } 
     else if(salita_piu)
      {
      salita_meno=0;
      if(cursore_sottomenu>0) cursore_sottomenu--;
      }
     presenta_menu((unsigned char*)&sensore_di_flusso,0,8,cursore_sottomenu<<1);
     switch(cursore_sottomenu)
      {
      case 0: if(set.abilita_sensore_flusso) presenta_scritta((char*)&lettura_allarmi,22,0,24,1,8,7); else presenta_scritta((char*)&lettura_allarmi,5,0,24,1,8,7); break;
      case 1: presenta_unsigned(set.limite_minimum_flow,1,1,6,3); break;
      case 2: presenta_unsigned(set.limite_maximum_flow,1,1,6,4); break;
      case 3: presenta_unsigned(set.scala_sensore_di_flusso,3,1,6,5); break;
      }
     } break;
    case 8://sensore_temperatura
     {
     if(salita_meno)
      {
      salita_piu=0;
      if((set.abilita_sensore_temperatura)&&(cursore_sottomenu<3)) cursore_sottomenu++;
      } 
     else if(salita_piu)
      {
      salita_meno=0;
      if(cursore_sottomenu>0) cursore_sottomenu--;
      }
     presenta_menu((unsigned char*)&sensore_temperatura,0,8,cursore_sottomenu<<1);
     switch(cursore_sottomenu)
      {
      case 0: if(set.abilita_sensore_temperatura) presenta_scritta((char*)&lettura_allarmi,22,0,24,1,8,7); else presenta_scritta((char*)&lettura_allarmi,5,0,24,1,8,7); break;
      case 1: presenta_unsigned(set.tipo_sonda_PT100,0,1,10,1); break;
      case 2: presenta_unsigned(set.resistenza_PT100_a_0gradi,1,1,8,4); break;
      case 3: presenta_unsigned(set.resistenza_PT100_a_100gradi,1,1,8,4); break;
      }
     } break;
    case 9://storia emergenze
     {
     allarme_in_lettura=0;
     timer_aggiorna_menu=2000;
     asm
      {
      LDHX buffer_USB:1
      STHX secondi
      LDHX buffer_USB:3
      STHX secondi:2
      }
     Nallarme_ora_minuto_secondo(secondi,allarme_in_lettura,buffer_USB[0]);
     } break;
    case 10://contatore_energia
     {
     asm
      {
      LDHX misura_energia
      STHX KWh
      LDHX misura_energia:2
      STHX KWh:2
      }
     presenta_energia(KWh,3,1,0,8);
     display[1][9]=' ';
     display[1][10]='K';
     display[1][11]='W';
     display[1][12]='h';
     display[1][13]=' ';
     } break;
    case 11://contatore_litri
     {
     asm
      {
      LDHX #100
      LDA conta_litri
      DIV
      STA KWh
      LDA conta_litri:1
      DIV
      STA KWh:1
      LDA conta_litri:2
      DIV
      STA KWh:2
      LDA conta_litri:3
      DIV
      STA KWh:3
      }
     presenta_energia(KWh,1,1,0,8);
     display[1][9]=' ';
     display[1][10]='m';
     display[1][11]='^';
     display[1][12]='3';
     display[1][13]=' ';
     } break;
    case 12://data
     {
     modifica_data_ora();
     cursore_sottomenu=12;
     } break;
    /*
    case 13://numerio seriale
     {
     } break;
     */
    case 14://calibrazione
     {
     toggle_start=1;
     salita_start=0;
     timer_commuta_presentazione=0;
     if((relay_alimentazione==0)&&(remoto==filtro_pulsanti))
      {
      set.motore_on=1;
      segnalazione_=1;
      relay_alimentazione=1;
      prima_segnalazione=0;
      timer_eccitazione_relay=tempo_eccitazione_relay;
      picco_corrente_avviamento=0;
      corrente_test=200;//20A
      timer_allarme_avviamento=durata_cc;
      timer_avviamento_monofase=durata_avviamento;//3 s tempo massimo di avviamento di un motore monofase
      timer_mandata_chiusa=(unsigned char)set.ritardo_stop_mandata_chiusa;
      timer_attesa_secco=(unsigned char)set.ritardo_stop_funzionamento_a_secco;
      timer_attesa_squilibrio=(unsigned char)set.ritardo_protezione_squilibrio;
      timer_attesa_tensione=(unsigned char)set.ritardo_protezione_tensione;
      tentativi_avviamento=N_tentativi_motore_bloccato;
      tentativi_avviamento_a_secco=N_tentativi_motore_bloccato;
      }
     if(salita_piu)
      {
      salita_piu=0;
      if(cursore_sottomenu<2) cursore_sottomenu++;
      } 
     else if(salita_meno)
      {
      salita_meno=0;
      if(cursore_sottomenu>0) cursore_sottomenu--;
      }
     presenta_menu((unsigned char*)&calibrazione,0,tot_righe_menu,cursore_sottomenu<<1);
     } break;
     /*
    case 15://reset
     {
     presenta_scritta((char*)&attivazione_comando_reset,17,0,1,1,0,16);
     } break;
     */
    }
   } 
  }
  
 else if(toggle_enter==2) //modifica della funzione
  {
  if(salita_enter)
   {
   salita_enter=0;
   toggle_enter=1;
   } 
  switch(cursore_menu)
   {
   case 1://menu iniziale + potenza motore
    {
    modifica_unsigned((unsigned int*)&set.N_tabella_potenza,0,14,0);
    presenta_scritta((unsigned char*)&menu_principale,34,0,tot_righe_menu,0,0,16);
    presenta_scritta((unsigned char*)&presentazione_iniziale,(set.N_tabella_potenza+1)*17,0,0,1,0,16);
    set.potenza_nominale=tabella_potenza_nominale[set.N_tabella_potenza];
    if(set.N_tabella_potenza<9) mono_tri_fase=1; else mono_tri_fase=0;
    lampeggio_da=17;
    N_cifre_lampeggianti=16;
    } break;
   case 2://dati_motore
    {
    switch(cursore_sottomenu)
     {
     case 0:
      {
      modifica_unsigned((unsigned int*)&set.tensione_nominale,tabella_tensione_nominale[1][set.N_tabella_potenza],tabella_tensione_nominale[2][set.N_tabella_potenza],1);
      presenta_unsigned(set.tensione_nominale,0,1,11,3);
      N_cifre_lampeggianti=3;
      } break;
     case 1:
      {
      modifica_unsigned((unsigned int*)&set.corrente_nominale,tabella_corrente_nominale[1][set.N_tabella_potenza],tabella_corrente_nominale[2][set.N_tabella_potenza],1);
      presenta_unsigned(set.corrente_nominale,1,1,11,3);
      N_cifre_lampeggianti=4;
      } break;
     }
    } break;
   case 3://protezione_voltaggio
    {
    switch(cursore_sottomenu)
     {
     case 0:
      {
      modifica_unsigned((unsigned int*)&set.limite_sovratensione,tabella_limite_sovratensione[1][set.N_tabella_potenza],tabella_limite_sovratensione[2][set.N_tabella_potenza],1);
      presenta_unsigned(set.limite_sovratensione,0,1,10,3);
      N_cifre_lampeggianti=3;
      } break;
     case 1:
      {
      modifica_unsigned((unsigned int*)&set.limite_sottotensione,tabella_limite_sottotensione[1][set.N_tabella_potenza],tabella_limite_sottotensione[2][set.N_tabella_potenza],1);
      presenta_unsigned(set.limite_sottotensione,0,1,10,3);
      N_cifre_lampeggianti=3;
      } break;
     case 2:
      {
      modifica_unsigned((unsigned int*)&set.tensione_restart,tabella_tensione_restart[1][set.N_tabella_potenza],tabella_tensione_restart[2][set.N_tabella_potenza],1);
      presenta_unsigned(set.tensione_restart,0,1,10,3);
      N_cifre_lampeggianti=3;
      } break;
     case 3:
      {
      modifica_unsigned((unsigned int*)&set.limite_segnalazione_dissimmetria,tabella_limite_segnalazione_dissimmetria[1][set.N_tabella_potenza],tabella_limite_segnalazione_dissimmetria[2][set.N_tabella_potenza],1);
      presenta_unsigned(set.limite_segnalazione_dissimmetria,0,1,10,3);
      N_cifre_lampeggianti=3;
      } break;
     case 4:
      {
      modifica_unsigned((unsigned int*)&set.limite_intervento_dissimmetria,tabella_limite_intervento_dissimmetria[1][set.N_tabella_potenza],tabella_limite_intervento_dissimmetria[2][set.N_tabella_potenza],1);
      presenta_unsigned(set.limite_intervento_dissimmetria,0,1,10,3);
      N_cifre_lampeggianti=3;
      } break;
     case 5:
      {
      modifica_unsigned((unsigned int*)&set.ritardo_protezione_tensione,tabella_ritardo_protezione_tensione[1],tabella_ritardo_protezione_tensione[2],1);
      presenta_unsigned(set.ritardo_protezione_tensione,0,1,10,3);
      N_cifre_lampeggianti=3;
      } break;
     case 6:
      {
      modifica_unsigned((unsigned int*)&set.ritardo_riaccensione_da_emergenza_V,tabella_ritardo_riaccensione_da_emergenza_V[1],tabella_ritardo_riaccensione_da_emergenza_V[2],1);
      presenta_unsigned(set.ritardo_riaccensione_da_emergenza_V,0,1,10,3);
      N_cifre_lampeggianti=3;
      } break;
     } 
    } break;
   case 4://protezione_corrente
    {
    switch(cursore_sottomenu)
     {
     case 0:
      {
      modifica_unsigned((unsigned int*)&set.limite_sovracorrente,tabella_limite_sovracorrente[1][set.N_tabella_potenza],tabella_limite_sovracorrente[2][set.N_tabella_potenza],1);
      presenta_unsigned(set.limite_sovracorrente,0,1,10,3);
      N_cifre_lampeggianti=3;
      } break;
     case 1:
      {
      modifica_unsigned((unsigned int*)&set.limite_segnalazione_squilibrio,tabella_limite_segnalazione_squilibrio[1][set.N_tabella_potenza],tabella_limite_segnalazione_squilibrio[2][set.N_tabella_potenza],1);
      presenta_unsigned(set.limite_segnalazione_squilibrio,0,1,11,3);
      N_cifre_lampeggianti=3;
      } break;
     case 2:
      {
      modifica_unsigned((unsigned int*)&set.limite_intervento_squilibrio,tabella_limite_intervento_squilibrio[1][set.N_tabella_potenza],tabella_limite_intervento_squilibrio[2][set.N_tabella_potenza],1);
      presenta_unsigned(set.limite_intervento_squilibrio,0,1,11,3);
      N_cifre_lampeggianti=3;
      } break;
     case 3:
      {
      modifica_unsigned((unsigned int*)&set.ritardo_protezione_squilibrio,tabella_ritardo_protezione_squilibrio[1],tabella_ritardo_protezione_squilibrio[2],1);
      presenta_unsigned(set.ritardo_protezione_squilibrio,0,1,12,3);
      N_cifre_lampeggianti=3;
      } break;
     case 4:
      {
      modifica_unsigned((unsigned int*)&set.ritardo_riaccensione_da_emergenza_I,tabella_ritardo_riaccensione_da_emergenza_I[1],tabella_ritardo_riaccensione_da_emergenza_I[2],1);
      presenta_unsigned(set.ritardo_riaccensione_da_emergenza_I,0,1,12,3);
      N_cifre_lampeggianti=3;
      } break;
     }
    } break;
   case 5://controllo_pressione
    {
    switch(cursore_sottomenu)
     {
     case 0:
      {
      modifica_unsigned((unsigned int*)&set.abilita_sensore_pressione,0,1,0);
      if(set.abilita_sensore_pressione) presenta_scritta((char*)&lettura_allarmi,22,0,24,1,8,7); else presenta_scritta((char*)&lettura_allarmi,5,0,24,1,8,7);
      lampeggio_da=25;
      N_cifre_lampeggianti=3;
      } break;
     case 1:
      {
      modifica_unsigned((unsigned int*)&set.pressione_emergenza,tabella_pressione_emergenza[1],tabella_pressione_emergenza[2],1);
      presenta_unsigned(set.pressione_emergenza,1,1,8,3);
      N_cifre_lampeggianti=3;
      } break;
     case 2:
      {
      modifica_unsigned((unsigned int*)&set.modo_start_stop,0,1,0);
      if(set.modo_start_stop) presenta_scritta((char*)&presenta_tipo_start_stop,17,0,2,1,0,16); else presenta_scritta((char*)&presenta_tipo_start_stop,0,0,2,1,0,16);
      lampeggio_da=19;
      N_cifre_lampeggianti=6;
      } break;
     case 3:
      {
      modifica_unsigned((unsigned int*)&set.pressione_spegnimento,tabella_pressione_spegnimento[1],tabella_pressione_spegnimento[2],1);
      presenta_unsigned(set.pressione_spegnimento,1,1,9,3);
      N_cifre_lampeggianti=4;
      } break;
     case 4:
      {
      modifica_unsigned((unsigned int*)&set.pressione_accensione,tabella_pressione_accensione[1],tabella_pressione_accensione[2],1);
      presenta_unsigned(set.pressione_accensione,1,1,9,3);
      N_cifre_lampeggianti=4;
      } break;
     case 5:
      {
      modifica_unsigned((unsigned int*)&set.corrente_minima_sensore,tabella_corrente_minima_sensore[1],tabella_corrente_minima_sensore[2],1);
      presenta_unsigned(set.corrente_minima_sensore,1,1,10,3);
      N_cifre_lampeggianti=4;
      } break;
     case 6:
      {
      modifica_unsigned((unsigned int*)&set.corrente_massima_sensore,tabella_corrente_massima_sensore[1],tabella_corrente_massima_sensore[2],1);
      presenta_unsigned(set.corrente_massima_sensore,1,1,10,3);
      N_cifre_lampeggianti=4;
      } break;
     case 7:
      {
      modifica_unsigned((unsigned int*)&set.portata_sensore_pressione,tabella_portata_sensore_pressione[1],tabella_portata_sensore_pressione[2],1);
      presenta_unsigned(set.portata_sensore_pressione,1,1,10,3);
      N_cifre_lampeggianti=4;
      } break;
     case 8:
      {
      modifica_unsigned((unsigned int*)&set.timer_ritorno_da_emergenza_sensore,tabella_timer_ritorno_da_emergenza_sensore[1],tabella_timer_ritorno_da_emergenza_sensore[2],1);
      presenta_unsigned(set.timer_ritorno_da_emergenza_sensore,0,1,10,3);
      N_cifre_lampeggianti=3;
      } break;
     }
    } break;
   case 6://controllo_potenza
    {
    switch(cursore_sottomenu)
     {
     case 0:
      {
      modifica_unsigned((unsigned int*)&set.potenza_minima_mandata_chiusa,tabella_potenza_minima_mandata_chiusa[1],tabella_potenza_minima_mandata_chiusa[2],1);
      presenta_unsigned(set.potenza_minima_mandata_chiusa,0,1,10,3);
      N_cifre_lampeggianti=3;
      } break;
     case 1:
      {
      modifica_unsigned((unsigned int*)&set.ritardo_stop_mandata_chiusa,tabella_ritardo_stop_mandata_chiusa[1],tabella_ritardo_stop_mandata_chiusa[2],1);
      presenta_unsigned(set.ritardo_stop_mandata_chiusa,0,1,11,3);
      N_cifre_lampeggianti=3;
      } break;
     case 2:
      {
      modifica_unsigned((unsigned int*)&set.ritardo_riaccensione_mandata_chiusa,tabella_ritardo_riaccensione_mandata_chiusa[1],tabella_ritardo_riaccensione_mandata_chiusa[2],1);
      presenta_unsigned(set.ritardo_riaccensione_mandata_chiusa,0,1,11,3);
      N_cifre_lampeggianti=3;
      } break;
     case 3:
      {
      modifica_unsigned((unsigned int*)&set.potenza_minima_funz_secco,tabella_potenza_minima_funz_secco[1],tabella_potenza_minima_funz_secco[2],1);
      presenta_unsigned(set.potenza_minima_funz_secco,0,1,10,3);
      N_cifre_lampeggianti=3;
      } break;
     case 4:
      {
      modifica_unsigned((unsigned int*)&set.ritardo_stop_funzionamento_a_secco,tabella_ritardo_stop_funzionamento_a_secco[1],tabella_ritardo_stop_funzionamento_a_secco[2],1);
      presenta_unsigned(set.ritardo_stop_funzionamento_a_secco,0,1,11,3);
      N_cifre_lampeggianti=3;
      } break;
     case 5:
      {
      modifica_unsigned((unsigned int*)&set.ritardo_riaccensione_funzionamento_a_secco,tabella_ritardo_riaccensione_funzionamento_a_secco[1],tabella_ritardo_riaccensione_funzionamento_a_secco[2],1);
      presenta_unsigned(set.ritardo_riaccensione_funzionamento_a_secco,0,1,10,3);
      N_cifre_lampeggianti=3;
      } break;
     }
    } break;
   case 7://sensore_di_flusso
    {
    switch(cursore_sottomenu)
     {
     case 0:
      {
      modifica_unsigned((unsigned int*)&set.abilita_sensore_flusso,0,1,0);
      if(set.abilita_sensore_flusso) presenta_scritta((char*)&lettura_allarmi,22,0,24,1,8,7); else presenta_scritta((char*)&lettura_allarmi,5,0,24,1,8,7);
      lampeggio_da=26;
      N_cifre_lampeggianti=3;
      } break;
     case 1:
      {
      modifica_unsigned((unsigned int*)&set.limite_minimum_flow,tabella_limite_minimum_flow[1],tabella_limite_minimum_flow[2],1);
      presenta_unsigned(set.limite_minimum_flow,1,1,6,4);
      N_cifre_lampeggianti=5;
      } break;
     case 2:
      {
      modifica_unsigned((unsigned int*)&set.limite_maximum_flow,tabella_limite_maximum_flow[1],tabella_limite_maximum_flow[2],1);
      presenta_unsigned(set.limite_maximum_flow,1,1,6,4);
      N_cifre_lampeggianti=5;
      } break;
     case 3:
      {
      modifica_unsigned((unsigned int*)&set.scala_sensore_di_flusso,tabella_scala_sensore_di_flusso[1],tabella_scala_sensore_di_flusso[2],1);
      presenta_unsigned(set.scala_sensore_di_flusso,3,1,6,5);
      N_cifre_lampeggianti=6;
      } break;
     }
    } break;
   case 8://sensore_temperatura
    {
    switch(cursore_sottomenu)
     {
     case 0:
      {
      modifica_unsigned((unsigned int*)&set.abilita_sensore_temperatura,0,1,0);
      if(set.abilita_sensore_temperatura) presenta_scritta((char*)&lettura_allarmi,22,0,24,1,8,7); else presenta_scritta((char*)&lettura_allarmi,5,0,24,1,8,7);
      lampeggio_da=26;
      N_cifre_lampeggianti=3;
      } break;
     case 1:
      { 
      modifica_unsigned((unsigned int*)&set.tipo_sonda_PT100,tabella_tipo_sonda_PT100[1],tabella_tipo_sonda_PT100[2],1);
      presenta_unsigned(set.tipo_sonda_PT100,0,1,10,1);
      N_cifre_lampeggianti=1;
      } break;
     case 2:
      {
      modifica_unsigned((unsigned int*)&set.resistenza_PT100_a_0gradi,tabella_resistenza_PT100_a_0gradi[1],tabella_resistenza_PT100_a_0gradi[2],1);
      presenta_unsigned(set.resistenza_PT100_a_0gradi,1,1,8,4);
      N_cifre_lampeggianti=5;
      } break;
     case 3:
      {
      modifica_unsigned((unsigned int*)&set.resistenza_PT100_a_100gradi,tabella_resistenza_PT100_a_100gradi[1],tabella_resistenza_PT100_a_100gradi[2],1);
      presenta_unsigned(set.resistenza_PT100_a_100gradi,1,1,8,4);
      N_cifre_lampeggianti=5;
      } break;
     }
    } break;
   case 9://storia emergenze
    {
    if(pronto_alla_risposta)
     {
     if(salita_piu)
      {
      salita_piu=0;
      if(allarme_in_lettura<set.numero_segnalazione) 
       {
       allarme_in_lettura++;
       timer_aggiorna_menu=2000;
       pronto_alla_risposta=0;
       }
      } 
     else if(salita_meno)
      {
      salita_meno=0;
      if(allarme_in_lettura>0)
       {
       allarme_in_lettura--;
       timer_aggiorna_menu=2000;
       pronto_alla_risposta=0;
       }
      }
     if(buffer_USB[0]<=set.numero_segnalazione)
      {  
      asm
       {
       LDHX buffer_USB:1
       STHX secondi
       LDHX buffer_USB:3
       STHX secondi:2
       }
      Nallarme_ora_minuto_secondo(secondi,allarme_in_lettura,buffer_USB[0]);
      } 
     else Nallarme_ora_minuto_secondo(istante,allarme_in_lettura,99);//fine degli allarmi
     lampeggio_da=17;
     N_cifre_lampeggianti=16;
     }
    } break;
   case 10://contatore_energia
    {
    if(meno==filtro_pulsanti)
     {
     presenta_unsigned(timer_reset,0,1,14,2);
     if((timer_reset==0)&&(comando_reset==0))
      {
      N_cifre_lampeggianti=0;
      salita_start=0;
      start=0;
      comando_reset=1;
      misura_energia[0]=0;
      misura_energia[1]=0;
      misura_energia[2]=0;
      salvataggio_funzioni=1;
      }
     } 
    else
     {
     timer_reset=5;//5 s attesa reset
     comando_reset=0;
     }
    if(timer_aggiorna_menu==0)
     {
     timer_aggiorna_menu=250;
     presenta_menu((unsigned char*)&menu_principale,0,tot_righe_menu,cursore_menu<<1);
     asm
      {
      LDHX misura_energia
      STHX KWh
      LDHX misura_energia:2
      STHX KWh:2
      }
     presenta_energia(KWh,3,1,0,8);
     display[1][9]=' ';
     display[1][10]='K';
     display[1][11]='W';
     display[1][12]='h';
     display[1][13]=' ';
     }
    lampeggio_da=17;
    N_cifre_lampeggianti=9;
    } break;
   case 11://contatore_litri
    {
    if(meno==filtro_pulsanti)
     {
     presenta_unsigned(timer_reset,0,1,14,2);
     if((timer_reset==0)&&(comando_reset==0))
      {
      N_cifre_lampeggianti=0;
      salita_start=0;
      start=0;
      comando_reset=1;
      conta_litri[0]=0;
      conta_litri[1]=0;
      conta_litri[2]=0;
      salvataggio_funzioni=1;
      }
     } 
    else
     {
     timer_reset=5;//5 s attesa reset
     comando_reset=0;
     N_cifre_lampeggianti=0;
     }
    if(timer_aggiorna_menu==0)
     {
     timer_aggiorna_menu=250;
     presenta_menu((unsigned char*)&menu_principale,0,tot_righe_menu,cursore_menu<<1);
     asm
      {
      LDHX #100
      LDA conta_litri
      DIV
      STA KWh
      LDA conta_litri:1
      DIV
      STA KWh:1
      LDA conta_litri:2
      DIV
      STA KWh:2
      LDA conta_litri:3
      DIV
      STA KWh:3
      }
     presenta_energia(KWh,1,1,0,8);
     display[1][9]=' ';
     display[1][10]='m';
     display[1][11]='^';
     display[1][12]='3';
     display[1][13]=' ';
     }
    lampeggio_da=17;
    N_cifre_lampeggianti=9;
    } break;
   case 12://data
    {
    modifica_data_ora();
    lampeggio_da=cursore_sottomenu;
    N_cifre_lampeggianti=2;
    } break;
   case 13://numerio seriale
    {
    modifica_unsigned((unsigned int*)&set.numero_serie,1,65535,2);
    presenta_unsigned(set.numero_serie,0,1,7,5);
    N_cifre_lampeggianti=5;
    } break;
   case 14://calibrazione
    {
    switch(cursore_sottomenu)
     {
     case 0:
      {
      modifica_unsigned((unsigned int*)&set.calibrazione_I1,tabella_calibrazione[1],tabella_calibrazione[2],0);
      presenta_unsigned(set.calibrazione_I1,0,0,12,3);
      presenta_unsigned(I3_rms,1,1,11,3);
      presenta_unsigned(I1_rms,1,1,3,3);
      N_cifre_lampeggianti=3;
      } break;
     case 1:
      {
      modifica_unsigned((unsigned int*)&set.calibrazione_I3,tabella_calibrazione[1],tabella_calibrazione[2],0);
      presenta_unsigned(set.calibrazione_I3,0,0,12,3);
      presenta_unsigned(I1_rms,1,1,3,3);
      presenta_unsigned(I3_rms,1,1,11,3);
      N_cifre_lampeggianti=3;
      } break;
     case 2:
      {
      modifica_unsigned((unsigned int*)&set.calibrazione_I2,tabella_calibrazione[1],tabella_calibrazione[2],0);
      presenta_unsigned(set.calibrazione_I2,0,0,12,3);
      presenta_unsigned(I1_rms,1,1,3,3);
      presenta_unsigned(I2_rms,1,1,11,3);
      N_cifre_lampeggianti=3;
      } break;
     } 
    } break;
   case 15://reset
    {
    if(meno==filtro_pulsanti)
     {
     presenta_scritta((char*)&attivazione_comando_reset,0,0,1,1,0,16);
     presenta_unsigned(timer_reset,0,1,10,2);
     if((timer_reset==0)&&(comando_reset==0))
      {
      salita_start=0;
      start=0;
      comando_reset=1;
      set.motore_on=0;
      toggle_func=0;
      toggle_enter=0;
      reset_default=1;
      indirizzo_scrivi_eeprom=0;
      ultimo_indirizzo_scrittura=0x02000000;
      }
     } 
    else
     {
     timer_reset=5;//5 s attesa reset
     comando_reset=0;
     presenta_scritta((char*)&attivazione_comando_reset,17,0,1,1,0,16);
     }
    lampeggio_da=17; 
    N_cifre_lampeggianti=16;
    } break;
   }
  }
 } 
}

void messaggio_allarme(char indentificazione)
{
if(indentificazione<10) presenta_scritta((unsigned char *)&lettura_allarmi,(int)indentificazione*17,0,24,1,0,16);
else presenta_scritta((unsigned char *)&lettura_allarmi,(int)(indentificazione-8)*17,0,24,1,0,16);
} 

void misure_medie(void)
{
int primo, secondo, terzo, tensione, corrente;
long prod;
asm
 {
 LDA mono_tri_fase
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

void Nallarme_ora_minuto_secondo(long secondi, int conta_allarmi, char numero)
{
int indirizzo;
char j, anni, mesi, giorni, ore, minuti, sec;
asm
 {
//   anno(a partire dal 2000) = tempo/(13*32*24*3600); resto_anno = tempo - anno*(13*32*24*3600);
//   mese = resto_anno/(32*24*3600); resto_mese = resto_anno - mese*(32*24*3600)
//   giorno = resto_mese/(24*3600);  resto_giorno = resto_mese - giorno*(24*3600)
//   ora = resto_giorno/3600;  resto_ora = resto_giorno - ora*3600;
//   minuto = resto_ora/60;  resto_minuto = secondo = resto_ora - minuto*60;
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
 STA minuti

 LDHX #24
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
 STA ore

 LDHX #32
 LDA secondi:2
 DIV
 STA secondi:2
 LDA secondi:3
 DIV
 STA secondi:3
 PSHH
 PULA
 STA giorni

 LDHX #13
 LDA secondi:2
 DIV
 STA secondi:2
 LDA secondi:3
 DIV
 STA secondi:3
 PSHH
 PULA
 STA mesi

 LDA secondi:3
 STA anni

 LDHX timer_aggiorna_menu
 CPHX #1000
 BCS per_seconda_presentazione
 LDA #' '
 STA display:25
 STA display:28
 LDA #'C'
 STA display:26
 LDA #':'
 STA display:27
 
 LDHX #10
 LDA conta_allarmi
 DIV
 STA conta_allarmi
 LDA conta_allarmi:1
 DIV
 STA conta_allarmi:1
 PSHH
 PULA
 ORA #$30
 STA display:32
 LDHX #10
 LDA conta_allarmi
 DIV
 STA conta_allarmi
 LDA conta_allarmi:1
 DIV
 STA conta_allarmi:1
 PSHH
 PULA
 ORA #$30
 STA display:31
 LDHX #10
 LDA conta_allarmi:1
 DIV
 STA conta_allarmi:1
 ORA #$30
 STA display:29
 PSHH
 PULA
 ORA #$30
 STA display:30
 
 LDHX #10
 LDA sec
 DIV
 ORA #$30
 STA display:23
 PSHH
 PULA
 ORA #$30
 STA display:24
 
 LDA #':'
 STA display:22
 LDHX #10
 LDA minuti
 DIV
 ORA #$30
 STA display:20
 PSHH
 PULA
 ORA #$30
 STA display:21
 
 LDA #':'
 STA display:19
 LDHX #10
 LDA ore
 DIV
 ORA #$30
 STA display:17
 PSHH
 PULA
 ORA #$30
 STA display:18

per_seconda_presentazione:
 LDA #'-'
 STA display:15
 LDHX #10
 LDA giorni
 DIV
 ORA #$30
 STA display:13
 PSHH
 PULA
 ORA #$30
 STA display:14
 
 LDA #'-'
 STA display:12
 LDHX #10
 LDA mesi  
 DIV
 ORA #$30
 STA display:10
 PSHH
 PULA
 ORA #$30
 STA display:11
 
 LDA #'-'
 STA display:9
 LDHX #10
 LDA anni
 DIV
 ORA #$30
 STA display:7
 PSHH
 PULA
 ORA #$30
 STA display:8
 
 LDA #'0'
 STA display:6
 LDA #'2'
 STA display:5
 LDA #','
 STA display:4
 LDHX #10
 LDA numero
 DIV
 ORA #$30
 STA display:2
 PSHH
 PULA
 ORA #$30
 STA display:3
 LDA #':'
 STA display:1
 LDA #'N'
 STA display
 
 LDHX timer_aggiorna_menu
 CPHX #1000
 BCC fine

 LDA #32
 CMP numero
 BCC per_presentazione
 STA numero
per_presentazione:
 LDA numero
 CMP #10
 BCC per_indirizzo
 LDX #17
 MUL
 STA indirizzo:1
 STX indirizzo
 BRA lettura_messaggio
per_indirizzo: 
 SUB #8
 LDX #17
 MUL
 STA indirizzo:1
 STX indirizzo
lettura_messaggio:
 CLR j
ripeti_carattere: 
 LDHX indirizzo
 LDA @lettura_allarmi,X
 CLRH
 LDX j
 STA @display:17,X
 LDA indirizzo:1
 ADD #1
 STA indirizzo:1
 LDA indirizzo
 ADC #0
 STA indirizzo
 INC j
 LDA j
 CMP #17
 BCS ripeti_carattere
fine:
 }
}

void presenta_stato_motore(void)
{
int pressione, temperatura;
if(toggle_func==0)//presenta le letture
 {
 if(set.motore_on==1)
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
   if(mono_tri_fase==0) //con alimentazione monofase
    {
    if(alternanza_presentazione==0x21)
     {
     alternanza_presentazione=0x20;
     if(remoto==0)//abilitazione_OFF
      {
      presenta_scritta((unsigned char *)&abilitazione_OFF,0,0,1,1,0,16);
      } 
     else if(segnalazione_)
      {
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
     if(set.abilita_sensore_pressione) presenta_signed(pressione,1,0,11,3); else display[0][15]=' ';//elimina 'B'
     presenta_unsigned(corrente_media,1,1,0,3);
     presenta_unsigned(cosfi[3],2,1,5,3);
     if(temperatura>0) presenta_unsigned(temperatura,0,1,11,3); else display[1][14]=display[1][15]=' ';//elimina '°C'
     }
    else if(alternanza_presentazione==0x40)
     {
     presenta_unsigned(tensione_media,0,0,0,3);
     if(potenza_media<0) potenza_media=0;
     presenta_unsigned(potenza_media,0,0,5,5);
     if(set.abilita_sensore_pressione) presenta_signed(pressione,1,0,11,3); else display[0][15]=' ';//elimina 'B'
     presenta_unsigned(corrente_media,1,1,0,3);
     presenta_unsigned(cosfi[3],2,1,5,3);
     if(temperatura>0) presenta_unsigned(temperatura,0,1,11,3); else display[1][14]=display[1][15]=' ';//elimina '°C'
     } 
    } 
   else //alimentazione trifase
    {
    if(alternanza_presentazione==0x21)
     {
     alternanza_presentazione=0x20;
     if(remoto==0)//abilitazione_OFF
      {
      presenta_scritta((unsigned char *)&abilitazione_OFF,0,0,1,1,0,16);
      }
     else if(segnalazione_)
      {
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
     if(set.abilita_sensore_pressione) presenta_signed(pressione,1,0,11,3); else display[0][15]=' ';//elimina 'B'
     presenta_unsigned(I1_rms,1,1,0,3);//prova //I1_rms
     presenta_unsigned(I2_rms,1,1,5,3);
     presenta_unsigned(I3_rms,1,1,11,3);
     }
    else if(alternanza_presentazione==0x40)
     {
     if(temperatura>0) presenta_unsigned(temperatura,0,0,0,3);
     else //presenta tensione
      {
      display[0][3]='V';
      display[0][4]=',';
      presenta_unsigned(tensione_media,0,0,0,3);
      } 
     if(potenza_media<0) potenza_media=0;
     presenta_unsigned(potenza_media,0,0,5,5);
     if(set.abilita_sensore_pressione) presenta_signed(pressione,1,0,11,3); else display[0][15]=' ';//elimina 'B'
     display[1][0]='P';
     display[1][1]='F';
     presenta_PF(cosfi[0],1,2);
     presenta_PF(cosfi[1],1,6);
     presenta_PF(cosfi[2],1,10);
     display[1][14]='S';
     if(sequenza_fasi) display[1][15]='1'; else display[1][15]='0';
     } 
    }
   } 
  }
 else if((reset_default)||(scrivi_eeprom))
  {
  presenta_scritta((unsigned char*)&in_salvataggio,0,0,1,0,0,16);
  presenta_cursore_eeprom(indirizzo_scrivi_eeprom);
  }
 else presenta_data_ora();//presenta_menu((unsigned char*)&comandi_da_effettuare,0,2,0);
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
if(set.motore_on)//condizioni per riavviamento
 {
 if(relay_alimentazione==0)
  {
  if(timer_riavviamento==0)
   {
   if((temperatura<set.limite_intervento_temper_motore-10)
   &&(I2xT<delta_T_riavviamento) //temperatura calcolata < 60°C
   &&(tensione_media<sovratensione_consentita)
   &&(tensione_media>ripresa_per_tensione_consentita)
   &&(tentativi_avviamento)
   &&(tentativi_avviamento_a_secco)
   &&(remoto==filtro_pulsanti))
    {
    if((set.abilita_sensore_pressione==0)||
    ((set.abilita_sensore_pressione)&&(pressione<set.pressione_accensione)&&(pressione>emergenza_sensore_pressione)))
     {
     if((mono_tri_fase==0)||                               
     ((mono_tri_fase)&&(dissimmetria<dissimmetria_tollerata)))
      {
      prima_segnalazione=0;
      segnalazione_=1;
      relay_alimentazione=1;
      timer_eccitazione_relay=tempo_eccitazione_relay;
      picco_corrente_avviamento=0;
      corrente_test=200;//20A
      timer_allarme_avviamento=durata_cc;
      timer_avviamento_monofase=durata_avviamento;//3 s tempo massimo di avviamento di un motore monofase
      timer_mandata_chiusa=(unsigned char)set.ritardo_stop_mandata_chiusa;
      timer_attesa_secco=(unsigned char)set.ritardo_stop_funzionamento_a_secco;
      timer_attesa_squilibrio=(unsigned char)set.ritardo_protezione_squilibrio;
      timer_attesa_tensione=(unsigned char)set.ritardo_protezione_tensione;
      }
     } 
    }
   } 
  }
 if(relay_alimentazione)
  {
  if((remoto==0)||((set.modo_start_stop)&&(set.abilita_sensore_pressione)&&(pressione>set.pressione_spegnimento)))
   {
   relay_alimentazione=0;
   timer_riavviamento=2;//attesa minima per riavviamento
   }
  } 
 }
else if(toggle_func==0)//motore fermo
 {
 presenta_data_ora();
 }
  
if(toggle_func==0)//marcia normale
 {
 if(salita_start)
  {
  salita_start=0;
  timer_commuta_presentazione=0;
  if((relay_alimentazione==0)&&(remoto==filtro_pulsanti))
   {
   set.motore_on=1;
   salvataggio_funzioni=1; 
   segnalazione_=1;
   relay_alimentazione=1;
   prima_segnalazione=0;
   timer_eccitazione_relay=tempo_eccitazione_relay;
   picco_corrente_avviamento=0;
   corrente_test=200;//20A
   timer_allarme_avviamento=durata_cc;
   timer_avviamento_monofase=durata_avviamento;//3 s tempo massimo di avviamento di un motore monofase
   timer_mandata_chiusa=(unsigned char)set.ritardo_stop_mandata_chiusa;
   timer_attesa_secco=(unsigned char)set.ritardo_stop_funzionamento_a_secco;
   timer_attesa_squilibrio=(unsigned char)set.ritardo_protezione_squilibrio;
   timer_attesa_tensione=(unsigned char)set.ritardo_protezione_tensione;
   tentativi_avviamento=N_tentativi_motore_bloccato;
   tentativi_avviamento_a_secco=N_tentativi_motore_bloccato;
   }
  } 
 else if(salita_stop)
  {
  salita_stop=0;
  toggle_enter=0;
  segnalazione_=0;
  relay_alimentazione=0;
  set.motore_on=0;
  salvataggio_funzioni=1; 
  timer_riavviamento=2;//attesa minima per riavviamento
  presenta_data_ora();//presenta_menu((unsigned char*)&comandi_da_effettuare,0,2,0);
  tentativi_avviamento=N_tentativi_motore_bloccato;
  tentativi_avviamento_a_secco=N_tentativi_motore_bloccato;
  } 
 }
}

void protezione_termica(void)//calcolo della sovra_temperatura motore con integrale I^2*T
{
char segno, prod[6];
asm
 {
;//---- incremento di temperatura ad ogni secondo ----
;//incremento = (corrente_media^2 * delta_Tn / corrente_nominale^2 - delta_T)/set.K_di_tempo_riscaldamento
;//incremento = (corrente_media^2 * fattore_I2xT - delta_T)/set.K_di_tempo_riscaldamento
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
 LDHX set.K_di_tempo_riscaldamento
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
 LDA relay_alimentazione
 BEQ spegni_relay_avviamento
 LDHX timer_avviamento_monofase
 BEQ spegni_relay_avviamento
 LDA timer_avviamento_monofase:1
 SUB #1
 STA timer_avviamento_monofase:1
 LDA timer_avviamento_monofase
 SBC #0
 STA timer_avviamento_monofase
 LDA #1
 STA relay_avviamento
 
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
 
spegni_relay_avviamento:
 CLRA
 STA relay_avviamento
fine: 
 }
}

void condizioni_di_allarme(void)
{
char j;
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
if(relay_alimentazione) //alimentazione presente
 {
 //segnalazioni ed arresto
 if((set.abilita_sensore_pressione)&&(pressione<emergenza_sensore_pressione)) //allarme sensore
  {
  relay_alimentazione=0;
  segnalazione_=29;
  timer_riavviamento=set.timer_ritorno_da_emergenza_sensore;
  } 
 else if(I2xT>massimo_delta_T)//allarme protezione termica
  {
  if(tentativi_avviamento) tentativi_avviamento--;
  relay_alimentazione=0;
  segnalazione_=20;
  timer_riavviamento=10;//timer_ritorno_da_emergenza;
  } 
 else if(temperatura>set.limite_intervento_temper_motore) //temperatura misurata
  {
  relay_alimentazione=0;
  segnalazione_=25;
  timer_riavviamento=10;//timer_ritorno_da_emergenza;
  } 
 else if((set.abilita_sensore_pressione)&&(pressione>set.pressione_emergenza))//pressione emergenza
  {
  relay_alimentazione=0;
  segnalazione_=30;
  timer_riavviamento=10;//timer_ritorno_da_emergenza;
  } 
 else
  {
  if((I1_rms>limitazione_I_avviamento)||(I2_rms>limitazione_I_avviamento)||(I3_rms>limitazione_I_avviamento)) //protezione per avviamento mancato
   {
   if(timer_allarme_avviamento==0)
    {
    if(tentativi_avviamento) tentativi_avviamento--;
    relay_alimentazione=0;
    segnalazione_=31;
    timer_riavviamento=10;//timer_ritorno_da_emergenza;
    }
   }
  else //emergenza tensione
   {
   timer_allarme_avviamento=durata_cc;
   if(tensione_media<sottotensione_consentita) //tensione insufficiente
    {
    if(timer_attesa_tensione==0)
     {
     relay_alimentazione=0;
     segnalazione_=22;
     timer_riavviamento=timer_ritorno_da_emergenza_V;
     }
    } 
   else if(tensione_media>sovratensione_consentita) //tensione eccessiva
    {
    if(timer_attesa_tensione==0)
     {
     relay_alimentazione=0;
     segnalazione_=21;
     timer_riavviamento=timer_ritorno_da_emergenza_V;
     }
    }
   else if((dissimmetria>dissimmetria_emergenza)&&(mono_tri_fase)) //tensione dissimmetrica
    {
    if(timer_attesa_tensione==0)
     {
     relay_alimentazione=0;
     segnalazione_=28;
     timer_riavviamento=timer_ritorno_da_emergenza_V;
     }
    } 
   else
    {
    timer_attesa_tensione=(unsigned char)set.ritardo_protezione_tensione;
    if((squilibrio>squilibrio_emergenza)&&(mono_tri_fase)) //corrente squilibrata
     {
     if(timer_attesa_squilibrio==0)
      {
      relay_alimentazione=0;
      segnalazione_=27;
      timer_riavviamento=timer_ritorno_da_emergenza_I;
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
       relay_alimentazione=0;
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
        relay_alimentazione=0;
        segnalazione_=24;
        asm
         {
         LDA N_tentativi_motore_bloccato
         INCA
         SUB tentativi_avviamento_a_secco
         STA j
         LDX timer_ritorno_da_emergenza_funzionamento_a_secco:1
         MUL
         STA timer_riavviamento:3
         STX timer_riavviamento:2
         LDA j
         LDX timer_ritorno_da_emergenza_funzionamento_a_secco:1
         MUL
         ADD timer_riavviamento:2
         STA timer_riavviamento:2
         TXA
         ADC #0
         STA timer_riavviamento:1
         CLRA
         STA timer_riavviamento
         }
        if(tentativi_avviamento_a_secco) tentativi_avviamento_a_secco--;
        }
       } 
      else timer_attesa_secco=(unsigned char)set.ritardo_stop_funzionamento_a_secco;
      } 
     }
    } 
   } 
  }

 //segnalazioni senza arresto
 if(segnalazione_<segnalazioni_senza_intervento)
  {
  if(segnalazione_>1) segnalazione_=1;
  if((set.abilita_sensore_pressione)&&(pressione>set.pressione_emergenza-5)) segnalazione_=19;//segnalazione pressione_emergenza
  else
   {
   if(temperatura>temperatura_segnalazione) segnalazione_=15;//segnalazione con 5°C in anticipo
   else
    {
    if(I2xT>limitato_delta_T) segnalazione_=10;
    else
     {
     if((dissimmetria>dissimmetria_tollerata)&&(mono_tri_fase)) segnalazione_=18;
     else
      { 
      if((squilibrio>squilibrio_tollerato)&&(mono_tri_fase)) segnalazione_=17;
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
  if(prima_segnalazione==0)
   {
   prima_segnalazione=1;
   timer_attesa_segnalazione_fault=1000;
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
PTCDD=0x05; //0=clock, 1=dato, 2=reset
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
SPI1BR=0x74;
 
//-------uscite LED e PWM e timer 1ms--------------------
PTFDD=0x3d;//uscite LED e relè
PTFD=0;
TPM1CNT=0;
TPM1MOD=12000;//PWM a 1KHz
TPM1C0SC=0x08;//LED allarme
TPM1C2SC=0x08;//relè linea
TPM1C3SC=0x08;//relè avviamento
TPM1C4SC=0x08;//LED presenza tensione
TPM1C5SC=0x08;//LED motore
TPM1SC=0x68;

TPM1C0V=-1;
TPM1C2V=-1;
TPM1C3V=-1;
TPM1C4V=0x7fff;
TPM1C5V=-1;

//-------ingresso timer per sensore di flusso--------
TPM2C0SC=0x48;//abilita interruzione, con ingresso in discesa
TPM2CNT=0;
TPM2SC=0x0f;//unità = 10.666 us

//-----portG  per display  G2 = RSneg, G3 = E----
PTGDD=0x0c;
PTGD=0;

//--------inizializzazione delle variabili-------------
precedente_lettura_allarme=allarme_in_lettura=10000;
pronto_alla_risposta=0;
prima_segnalazione=0;
timer_flusso_nullo=0;
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
timer_mandata_chiusa=100;
timer_attesa_secco=100;
timer_1_ora=3600;

attesa_invio=0;
relay_alimentazione=0;
relay_avviamento=0;
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
comando_display=0;

indirizzo_scrivi_eeprom=0;
ultimo_indirizzo_scrittura=0;

timer_rinfresco_display=255;
alternanza_presentazione=0x81;
lampeggio_da=0;
N_cifre_lampeggianti=0;
asm
 {
 LDA #' '
 LDHX #34
reset_del_disply:
 STA @display:-1,X 
 DBNZX reset_del_disply
 }

USB_comm_init();
asm
 { 
 CLI//abilita interrupt
 }
scrivi_eeprom=0;
leggi_eeprom=0;
reset_default=0;
comando_reset=0;
salvataggio_funzioni=0;
salvataggio_allarme=0;
salva_conta_secondi=0;
lunghezza_salvataggio=0;
timer_reset_display=100;
timer_1min=0;
timer_aggiorna_misure=2000; 
timer_aggiorna_menu=0;
comando_display=1;
while(timer_reset_display)
 {
 __RESET_WATCHDOG(); /* feeds the dog */
 }
comando_display=1;
leggi_impostazioni=1;
timer_presenta_data=0;
data_letta=0;
while(leggi_impostazioni)
 {
 __RESET_WATCHDOG(); /* feeds the dog */
 leggi_data_ora();
 presenta_data_ora();
 lettura_impostazioni();
 }
calcolo_delle_costanti();
offset_I1letta=0x20000000; //valor medio ipotetico
offset_I3letta=0x20000000;
offset_V32letta=0x20000000;
offset_V31letta=0x20000000;
segno_V12=0;
segno_V13=0;
anticipo_V12_su_V13=0;
Id_rms=0;
I1_rms=0;
I2_rms=0;
I3_rms=0;
V12_rms=0;
V23_rms=0;
V31_rms=0;
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
media_flusso=0;
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
 LDHX set.energia
 STHX misura_energia
 LDHX set.energia:2
 STHX misura_energia:2
 LDHX #0
 STHX misura_energia:4
 LDHX set.conta_litri_funzionamento
 STHX conta_litri
 LDHX set.conta_litri_funzionamento:2
 STHX conta_litri:2
 LDHX #0
 STHX conta_litri:4
 }
somma_energia_attiva=0; 
sequenza_fasi=1;
while(timer_aggiorna_misure)
 {
 __RESET_WATCHDOG(); /* feeds the dog */
 leggi_data_ora();
 presenta_scritta((char*)&presentazione_iniziale,0,0,0,0,0,16);
 presenta_scritta((unsigned char*)&presentazione_iniziale,(set.N_tabella_potenza+1)*17,0,0,1,0,16);
 }
start=0;
enter=0;
salita_start=0;
salita_enter=0;
stop=0;
esc=0;
salita_stop=0;
salita_esc=0;
toggle_start=0;
toggle_enter=0;
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
timer_eccitazione_relay=tempo_eccitazione_relay;

precedente_segnalazione=segnalazione_=relay_alimentazione=(unsigned char)set.motore_on;

timer_attesa_squilibrio=(unsigned char)set.ritardo_protezione_squilibrio;
timer_attesa_tensione=(unsigned char)set.ritardo_protezione_tensione;
timer_mandata_chiusa=(unsigned char)set.ritardo_stop_mandata_chiusa;
timer_attesa_secco=(unsigned char)set.ritardo_stop_funzionamento_a_secco;
tentativi_avviamento=N_tentativi_motore_bloccato;
}

/***********************************************************************/

void main(void)
{
condizioni_iniziali();
for(;;)  /* loop forever */
 {
 __RESET_WATCHDOG(); /* feeds the dog */
 leggi_data_ora();
 marcia_arresto();

 salva_reset_default();//precedenza 1
 salva_impostazioni();//precedenza 2
 salva_segnalazione_fault();//precedenza 3
 salva_conta_secondi_attivita();//precedenza 4

 lettura_impostazioni();//precedenza 5
 lettura_allarme_messo_in_buffer_USB();//precedenza 6
 misura_della_portata();
 
 trasmissione_misure_istantanee();
 trasmissione_tarature();

 programmazione();
 misure_medie();
 presenta_stato_motore();
 if(toggle_func==0) condizioni_di_allarme();//non in taratura
 prepara_per_salvataggio_allarme();

 USB_comm_process();
 cdc_process();
 }
}
