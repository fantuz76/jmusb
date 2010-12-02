#ifndef __PROTOCOLH
#define __PROTOCOLH


#define MAX_LEN_CMD   40


void comm_init(int (*putch_)(char), int (*getch_)(void), int(*kbhit_)(void));
void comm_process(void);


#endif