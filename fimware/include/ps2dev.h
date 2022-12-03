/*
* A Dirty Port of PS2DEV 
* Origin GitHub: https://github.com/Harvie/ps2dev
*/

#include "pico.h"


#ifndef ps2dev_h
#define ps2dev_h


    // Din port queries
    bool din_dead();      
    bool din_idle();      
    bool din_available();



    // -edited
    void initps2pins();
    bool din_init();
    int8_t pool_din_kbd();

    int8_t at_write(unsigned char data);
    int8_t at_read(unsigned char * data);

    uint8_t keyboard_make( unsigned char usbhidcode );
    uint8_t keyboard_break( unsigned char usbhidcode ); 


    
    int keyboard_press_printscreen();
    int keyboard_release_printscreen();





#if PS2DEV_C_



#else

//extern uint8_t const USB2PS2Make[][2][2];
//extern uint8_t const USB2PS2SMake[][2][2]

//extern uint8_t const USB2PS2Break[][2][3]
//extern uint8_t const USB2PS2SBreak[][2][3]


//extern enum CommandSetCodes CommandSetCodes;

#endif

#endif /* ps2dev_h */