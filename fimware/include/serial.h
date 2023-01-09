#ifndef SERIAL_H_
#define SERIAL_H_

#include "pico.h"


/*---------------------------------------*/
//          Generic Serial Stuff         //
/*---------------------------------------*/

void init_serial_uart();

void refresh_serial_uart();

/*---------------------------------------*/
//           Serial Mouse Funcs          //
/*---------------------------------------*/

void serial_putc(uint8_t *buffer, int size);

void serialMouseNego();

void postSerialMouse();

/*---------------------------------------*/
//             Terminal Stuff            //
/*---------------------------------------*/

int16_t term_getc(uart_inst_t * comport, uint8_t * buffer, int16_t num);

bool serial_terminal(uart_inst_t * com, uint64_t ddd);

void post_settings_saved(uart_inst_t * com, uint64_t ddd);

#endif // SERIAL_H_