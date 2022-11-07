
#include "pico.h"

#ifndef CORE_1_H_ 
#define CORE_1_H_


/*---------------------------------------*/
//              Readability              //
/*---------------------------------------*/
void wait_readable_fifo();

uint8_t read_fifo();

void write_fifo(uint8_t core_flag);

/*---------------------------------------*/
//           Core 1 Management           //
/*---------------------------------------*/

void start_core1(uint process);

void stop_core1();

void force_stop_core1();

void busy_core();

#endif