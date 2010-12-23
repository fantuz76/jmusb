#ifndef __AGRINET_PUMPH
#define __AGRINET_PUMPH



// Prototipi
void lettura_allarme(void);
void trasmissione_misure_istantanee(void);
void trasmissione_tarature(void);


// Var esportate all'esterno
extern unsigned int allarme_in_lettura;
extern unsigned int buffer_USB[32];
#endif