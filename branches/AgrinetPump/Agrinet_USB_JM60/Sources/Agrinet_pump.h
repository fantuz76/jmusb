#ifndef __AGRINET_PUMPH
#define __AGRINET_PUMPH



// Prototipi
void lettura_allarme_messo_in_buffer_USB(void);
void trasmissione_misure_istantanee(void);
void trasmissione_tarature(void);


// Var esportate all'esterno
extern unsigned int allarme_in_lettura;
extern unsigned char pronto_alla_risposta;
extern char buffer_USB[32];

extern const totale_indicazioni_fault;
#endif