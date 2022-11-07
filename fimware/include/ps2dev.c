/*
Special thanks to:

PS2DEV GitHub: https://github.com/Harvie/ps2dev

This article by Adam Chipweske
https://www.tayloredge.com/reference/Interface/atkeyboard.pdf

*/

#include <stdio.h>
#include <stdlib.h>
#include "bsp/board.h"
#include "hardware/gpio.h"


#include "utils.h"
#include "ctypes.h"
#include "ps2dev.h"
//#include "hid_app.h"
#include "din_lookups.h"
#include <default_config.h>

// ========== From PS2 dev ==========
//since for the device side we are going to be in charge of the clock,
//the two defines below are how long each _phase_ of the clock cycle is
#define CLKFULL 40
// we make changes in the middle of a phase, this how long from the
// start of phase to the when we drive the data line
#define CLKHALF 20
// Delay between bytes
// I've found i need at least 400us to get this working at all,
// but even more is needed for reliability, so i've put 1000us
#define BYTEWAIT 1000

// ========== XT variants of vars above ==========
#define XT_CLKSTART 120
#define XT_CLKINIT 50
#define XT_CLKFULL 66
#define XT_CLKHALF 30

// numbers from IBM model F
//#define XT_CLKSTART 107
//#define XT_CLKINIT 200
//#define XT_CLKFULL 61
//#define XT_CLKHALF 27

// Timeout if computer not sending for 30ms
#define TIMEOUT 30

static absolute_time_t BYTEWAIT_TIMESTAMP;


//--------------------------------------------------------------------+
//          Din port handling functions
//--------------------------------------------------------------------+

// Set the state of the data line of the din port
void setDATA(bool state) {
    // Because the data and clock lines are toggled low by turning on an NPN, it was a little obscure to put a 0 to set high.
    if ( state == 1 ) {
        gpio_put( PS2_DATA_OUT, 0 );
    } else {
        gpio_put( PS2_DATA_OUT, 1 );
    }

    return;
}

// Set the state of the clock line of the din port
void setCLOCK(bool state) {
    // Because the data and clock lines are toggled low by turning on an NPN, it was a little obscure to put a 0 to set high.
    if ( state == 1 ) {
        gpio_put( PS2_CLOCK_OUT, 0 );
    } else {
        gpio_put( PS2_CLOCK_OUT, 1 );
    }

    return;
}

// Is either Clock or data lines high
bool din_available() {
  return ( !gpio_get(PS2_DATA_IN) || !gpio_get(PS2_CLOCK_IN) );
}

// Din port isn't doing anything
bool din_idle() {
    return ( gpio_get(PS2_DATA_IN) && gpio_get(PS2_CLOCK_IN) );
}

// Are Clock and Data lines are low, it could mean that the pc is off
bool din_dead() {
    return ( !gpio_get(PS2_DATA_IN) && !gpio_get(PS2_CLOCK_IN) );
}

// Try initalising with the computers keyboard contrller
bool din_init() {   
    
    // Attempt to connect 5 times before returning as a failure
    for ( uint8_t i = 0 ; i < 5 ; i++ ) {
        
        int8_t ret = at_write(0xAA);

         // If we can connect set ret to true
        if (  ret == 0 ) { 
            return true;           }

        busy_wait_ms(10);
    }
    return false;
}


//--------------------------------------------------------------------+
//          Functions for communicating with XT keyboard controllers
//--------------------------------------------------------------------+

// ---- XT KBC Writing ----
// Send data over din port using XT protocol
int8_t xt_write(unsigned char keycode)
{   
    // Do NO-OP Commands while waiting for our timestamp    
    while ( !time_reached(BYTEWAIT_TIMESTAMP) ) { tight_loop_contents(); }
    
    // Clock Line Fail
    if ( !gpio_get(PS2_CLOCK_IN) )                              { return DIN_RET_CFAIL; }
    // Data Line Fail
    if ( !gpio_get(PS2_DATA_IN) )                               { return DIN_RET_DFAIL; }

    // Get the computers attention
    setDATA(0);                 // Pull Data Low
    busy_wait_us(XT_CLKINIT);   // Wait a bit

    // Start bit TODO: one or two start bits, curr 2 start bits
    setCLOCK(0);                // Set Clock Low
    setDATA(1);                 // Set Data High
    busy_wait_us(XT_CLKFULL);   // Wait for two start bits
    
    setCLOCK(1);                // Pull Up the clock
    
    // Transmit 8 bits of data,
    for(uint8_t bitcount = 0 ; bitcount < 9 ; bitcount++ ) {

        if ( bitcount == 0 ) { 
            setDATA(1);                     // The first data bit is always one. 
        } else {
            setDATA( (keycode & 0x01) );    // Set State of our data line
            keycode = keycode >> 1;         // Bitwise op to move onto the "next" bit
        }

        busy_wait_us(XT_CLKFULL);       // Wait while the clock line is high
        setCLOCK(0);                    // Set Clock Low     
        busy_wait_us(XT_CLKHALF);       // Mimick slower clocking to help compatibility
        setCLOCK(1);                    // Set Clock High
    }

    // Set our timestamp for the next soonest time we'll send a keycode 
    BYTEWAIT_TIMESTAMP = delayed_by_us( get_absolute_time(), BYTEWAIT);

    return 0;
}

// ---- XT KBC Reading ----
int8_t xt_read(unsigned char * value) { //todo
    }

// ---- XT KBC Polling ----
// At some point, the XT host will pull the keyboard clock low for 20 mS or more.
// When this happens, we need to simulate a keyboard reset.
void xt_poll() {

    // If clock line is high, return
    if ( gpio_get(PS2_CLOCK_IN) ) { return; }

    // With out clock line low, we need to time how long it remains low.
    absolute_time_t clowtime = get_absolute_time();
    while ( !gpio_get(PS2_CLOCK_IN) ) { tight_loop_contents(); }

    // If Clock wasn't low for at least 10 ms, return
    if ( 10000 > absolute_time_diff_us( clowtime, get_absolute_time() ) ) { return; }

    // Post diagnostic byte
    xt_write(ATXT_OKAY);

    // to do;
    //UpdateLED();

    return;
}

// ---- XT Posting the pause key ----
// Pause keys lack a release code and instead have one long ar of codes. 
// More memory effient to use a function instead of translation table
uint8_t xt_pause_press() {
    xt_write(0xE1);
    xt_write(0x1D);
    xt_write(0x45);
    xt_write(0xE1);
    xt_write(0x9D);
    xt_write(0xC5);
    return 0;
}


//--------------------------------------------------------------------+
//          Functions for communicating with AT/PS2 keyboard controllers
//--------------------------------------------------------------------+

// ---- AT KBC Writing ----
// Send data over din port using AT protocol
int8_t at_write(unsigned char keycode)
{
    // Do NO-OP Commands while waiting for our timestamp    
    while ( !time_reached(BYTEWAIT_TIMESTAMP) ) { tight_loop_contents(); }

    unsigned char parity = 1;
    
    // Clock and Data Line fail
    //if ( !gpio_get(PS2_CLOCK_IN) && !gpio_get(PS2_DATA_IN) )    { return DIN_RET_DCFAIL; }
    // Clock Line Fail
    if ( !gpio_get(PS2_CLOCK_IN) )                              { return DIN_RET_CFAIL; }
    // Data Line Fail
    if ( !gpio_get(PS2_DATA_IN) )                               { return DIN_RET_DFAIL; }

     
    for (uint8_t bitcount=0 ; bitcount <= 10 ; bitcount++) {
        switch (bitcount) {
            case 0:     // Start bit
                setDATA( 0 );
                break;
            case 9:     // Parity bit
                setDATA( parity );
                break;
            case 10:    // Stop bit
                setDATA ( 1 );
                break;
            default:    // Our Data bits
                setDATA( (keycode & 0x01) );   // Set our data line

                parity ^= (keycode & 0x01);    // Parity Tracker
                keycode >>= 1;                 // "move" onto the next keycode bit
                break;
        }

        // Jiggle the clock
        busy_wait_us(CLKHALF);  // Wait a bit
        setCLOCK( 0 );          // device sends on falling clock
        busy_wait_us(CLKFULL);  // Wait a bit
        setCLOCK( 1 );          // Raise our clock
        busy_wait_us(CLKHALF);  // Wait a bit.

    }

    // Set our timestamp for the next soonest time we'll send a keycode 
    BYTEWAIT_TIMESTAMP = delayed_by_us( get_absolute_time(), BYTEWAIT);


    #if DEBUG > 0
        printf("sent %x\n", keycode);
    #endif

    return 0;
}

// ---- AT KBC Reading ----
// Read data from the host computers KBC using the AT protocol
int8_t at_read(unsigned char * value) {
    uint data = 0x00;
    uint bit = 0x01;
    uint8_t cal_parity = 1;
    uint8_t rec_parity = 0;
    absolute_time_t ps2_read_conn_timeout = delayed_by_ms( get_absolute_time(), TIMEOUT);

    // wait for data line to go low and clock line to go high (or timeout)
    while( gpio_get(PS2_DATA_IN) || !gpio_get(PS2_CLOCK_IN) ) { 
        if ( time_reached(ps2_read_conn_timeout) )  { return DIN_RET_TOUT; }
    }    

    for (uint8_t bitcount=0 ; bitcount < 10 ; bitcount++) {
        switch (bitcount) {
            case 0:     // Start bit
                break;
            case 9:     // Parity bit
                 rec_parity = gpio_get(PS2_DATA_IN);
                break;
            default:    // Data we're getting from the pc
                data        |= ( gpio_get(PS2_DATA_IN) ) ? bit : 0x00;  // Our data tracker 
                cal_parity  ^= (int) gpio_get(PS2_DATA_IN);             // Parity Tacker

                bit <<= 1;                                              // shift the bit
                break;
        }
    
        // Jiggle the clock line
        busy_wait_us(CLKHALF); /**/ setCLOCK( 0 ); /**/ busy_wait_us(CLKFULL);/**/ setCLOCK( 1 );/**/ busy_wait_us(CLKHALF);
    }

    // Stop bit
    busy_wait_us(CLKHALF);
    setDATA( 0 );
    setCLOCK( 0 );
    busy_wait_us(CLKFULL);
    setCLOCK( 1 );
    busy_wait_us(CLKHALF);
    setDATA( 1 );

    *value = data & 0x00FF;

    #if DEBUG > 0
        printf("received data %x", *value);
        printf(" received parity %c", rec_parity);
        printf(" calculated parity %c\n",rec_parity);
    #endif

    // If the partity doesn't match, return parity fail error
    if (rec_parity != cal_parity) { return DIN_RET_PRFAIL; }
    
    // return successful read
    return DIN_RET_SUCC;
}

// ---- AT Posting the pause key ----
// Pause keys lack a release code and instead have one long ar of codes. 
// More memory effient to use a function instead of translation table
uint8_t at_pause_press() {
    at_write(0xe1);
    at_write(0x14);
    at_write(0x77);
    at_write(0xe1);
    at_write(0xf0);
    at_write(0x14);
    at_write(0xe0);
    at_write(0x77);
    return 0;
}



void ps2ack()
{
  while(at_write(0xFA));
  return;
}

int keyboard_reply(unsigned char cmd, unsigned char *leds)
{
    unsigned char val;

    switch (cmd) {

    case CMD_RESET: //reset
        //== Causes keyboard to enter "Reset" mode
        ps2ack();

        kbd_data.cmd_set.kbd_enabled = true;    // Enable the keyboard
        kbd_data.cmd_set.key_mkbktm = 0x07;     // Set Default All Keys state
        kbd_data.cmd_set.tm_delay = 500;        // Set Default Typematic delay of 0.5 seconds
        kbd_data.cmd_set.tm_rate = 100;         // Set Default Typematic rate of 10.9cps
        kbd_data.cmd_set.tm_key = 0x00;         // Blank the Typematic key  

        //the while loop lets us wait for the host to be ready
        while(at_write(ATXT_OKAY)!=0);

        #if DEBUG > 0
        printf("AT Command Set: Reset\n");
        fflush(stdout);
        #endif 

        break;

    case CMD_RESEND:
        ps2ack();

        #if DEBUG > 0
        printf("AT Command Set: RESEND\n");
        fflush(stdout);
        #endif 

        break;

    case SET_KEY_TYPE_MAKE: //##== PS/2 Only
        //== Allows the host to specify a key that is to send only make codes.
        //== This key will not send break codes or typematic repeat. 
        //== This key is specified by its set 3 scan code.
        ps2ack();

        #if DEBUG > 0
        printf("AT Command Set: Set Key Type Make\n");
        fflush(stdout);
        #endif 

        break;

    case SET_KEY_TYPE_MAKE_BREAK: //##== PS/2 Only
        //==  Similar to "Set Key Type Make", but both make codes and break codes are enabled (typematic is disabled.)
        ps2ack();
        
        #if DEBUG > 0
        printf("AT Command Set: Set Key Type Make Break\n");
        fflush(stdout);
        #endif 

        break;
    
    case SET_KEY_TYPE_TYPEMATIC: //##== PS/2 Only
        //== Similar to previous two commands, except make and typematic is enabled; break codes are disabled.
        ps2ack();

        #if DEBUG > 0
        printf("AT Command Set: Set Key Type Typematic\n");
        fflush(stdout);
        #endif 

        break;

    case SET_ALL_KEYS_TYPEMATIC_MAKE_BREAK: //##== PS/2 Only
        //== Default setting. Make codes, break codes, and typematic repeat enabled for all keys (except "Print Screen" key)
        kbd_data.cmd_set.key_mkbktm = 0x07;
        ps2ack();

        #if DEBUG > 0
        printf("AT Command Set: Set all keys typematic make break\n");
        fflush(stdout);
        #endif 

        break;

    case SET_ALL_KEYS_MAKE: //##== PS/2 Only
        //== Causes only make codes to be sent; break codes and typematic repeat are disabled for all keys.
        kbd_data.cmd_set.key_mkbktm = 0x04;
        ps2ack();

        #if DEBUG > 0
        printf("AT Command Set: Set All Keys Make\n");
        fflush(stdout);
        #endif 

        break;

    case SET_ALL_KEYS_MAKE_BREAK: //##== PS/2 Only
        //== Similar to previous two commands, except only typematic repeat is disabled.
        kbd_data.cmd_set.key_mkbktm = 0x06;
        ps2ack();

        #if DEBUG > 0
        printf("AT Command Set: Set All Keys Make Break\n");
        fflush(stdout);
        #endif 

        break;

    case SET_ALL_KEYS_TYPEMATIC: //##== PS/2 Only
        //== Similar to previous three commands, except only break codes are disabled. Make codes and typematic repeat are enabled.
        kbd_data.cmd_set.key_mkbktm = 0x05;
        ps2ack();

        #if DEBUG > 0
        printf("AT Command Set: Set All Keys Typematic\n");
        fflush(stdout);
        #endif 

        break;

    case SET_DEFAULT: 
        //== Load default typematic rate/delay (10.9cps / 500ms), key types (all keys typematic/make/break), and scan code set (2).

        kbd_data.cmd_set.kbd_enabled = true;    // Enable the keyboard
        kbd_data.cmd_set.key_mkbktm = 0x07;     // Set Default All Keys state
        kbd_data.cmd_set.tm_delay = 500;        // Set Default Typematic delay of 0.5 seconds
        kbd_data.cmd_set.tm_rate = 100;         // Set Default Typematic rate of 10.9cps
        kbd_data.cmd_set.tm_key = 0x00;         // Blank the Typematic key  

        ps2ack();
        
        #if DEBUG > 0
        printf("AT Command Set: Det Default\n");
        fflush(stdout);
        #endif 

        break;

    case CMD_DISABLE: // Disable
        //== Keyboard stops scanning, loads default values (see "Set Default" command), and waits further instructions

        kbd_data.cmd_set.kbd_enabled = false;   // Disable the keyboard
        kbd_data.cmd_set.key_mkbktm = 0x07;     // Set Default All Keys state
        kbd_data.cmd_set.tm_delay = 500;        // Set Default Typematic delay of 0.5 seconds
        kbd_data.cmd_set.tm_rate = 100;         // Set Default Typematic rate of 10.9cps
        kbd_data.cmd_set.tm_key = 0x00;         // Blank the Typematic key  

        ps2ack();

        #if DEBUG > 0
        printf("AT Command Set: Disable & Reset\n");
        fflush(stdout);
        #endif 

        break;

    case CMD_ENBALE: // Enable
        //== Re-enables keyboard after disabled using previous command.

        kbd_data.cmd_set.kbd_enabled = true;    // Enable the keyboard
        ps2ack();

        #if DEBUG > 0
        printf("AT Command Set: Enable\n");
        fflush(stdout);
        #endif 

        break;

    case SET_TYPEMATIC_RATE_DELAY: //set typematic rate
        //== Host follows this command with one argument byte that defines the typematic rate and delay

        ps2ack();

        if ( at_read(&val) == 0 ) {
            kbd_data.cmd_set.tm_delay = LKP_TM_DELAY[ (val & 0x60) >> 5 ];  // Store the Typematic Delay 
            kbd_data.cmd_set.tm_rate = LKP_TM_RRATE[ val & 0x1F ];          // Store Default Typematic rate of 10.9cps
            ps2ack();
        } 
        
        #if DEBUG > 0
        printf("AT Command Set: Set Typematic Rate Delay: Typematic rate %d | typematic delay %d\n", kbd_data.cmd_set.tm_rate, kbd_data.cmd_set.tm_delay );
        fflush(stdout);
        #endif

        break;

    case CMD_READ_ID: //##== PS/2 Only
        //== The keyboard responds by sending a two-byte device ID of 0xAB, 0x83

        ps2ack();
        at_write(0xAB);
        at_write(0x83);
        
        #if DEBUG > 0
        printf("AT Command Set: Read ID");
        fflush(stdout);
        #endif

        break;

    case SET_SCAN_CODE_SET:  //##== PS/2 Only
        //== Host follows this command with one argument byte, that specifies which scan code set the keyboard should use. 
        //== This argument byte may be 0x01, 0x02, or 0x03 to select scan code set 1, 2, or 3, respectively. 
        //== You can get the current scan code set from the keyboard by sending this command with 0x00 as the argument byte.

        ps2ack();

        if ( at_read(&val) == 0 ) {
            
            switch (val) {
            case 0x00:  
                // Host is looking for the scan code set we are set to
                // TODO: this but better

                at_write(0x02);
                break;
            
            case 0x01:
                // Set scan code set to set 1
                break;

            case 0x02:
                // Set scan code set to set 2
                break;

            case 0x03:
                // Set scan code set to set 3
                break;
            }
               
            ps2ack();
        }

        #if DEBUG > 0
        printf("AT command Set: Set Scan Code Set");
        fflush(stdout);
        #endif 

        break;

    case CMD_EHCO: //echo
        //== The keyboard responds with "Echo" (0xEE).
        at_write(0xEE);

        #if DEBUG > 0
        printf("AT Command Set: Echo\n");
        fflush(stdout);
        #endif

        break;

    case SET_RESET_LEDS: //set/reset LEDs
        //== The host follows this command with one argument byte, that specifies
        //== the state of the keyboard's Num Lock, Caps Lock, and Scroll Lock LEDs

        ps2ack();
        if(!at_read(leds)) ps2ack(); //do nothing with the rate
        
        if ( kbd_data.kbd_count > 0 ) {
            set_locks_from_din(*leds);
        }
        
        #ifdef DEBUG
        printf("AT Command Set: Set/Reset LEDS: %x\n", *leds);
        fflush(stdout);
        #endif

        return 1;
        break;
    }
    return 0;
}

int8_t keyboard_handle(unsigned char *leds) {
    
    // If the din port is potentially dead
    if ( din_dead() ) { return DIN_RET_DISCONN; }

    // If the DIN port is currently busy or something.
    if ( !din_available() )  { return DIN_RET_UNAVAIL; }

    unsigned char c;                        // Char stores data recieved from computer for KBD
    int8_t readresult = at_read(&c);        // Store the return of the read command.

    if ( readresult == DIN_RET_SUCC ) { 
        return keyboard_reply(c, leds); }

    return readresult;                      // return result from the rad function.
}


//--------------------------------------------------------------------+
//          Functions for communicating with AT/PS2 keyboard controllers
//--------------------------------------------------------------------+


// Keyboard button is pressed, takes usbhid keycode and coverts it ps2 keycode for writing to ps2 out
uint8_t keyboard_make( unsigned char usbhidcode ) {

    // ========== Unknown Code ==========
    // Incase the usbhid code sent is unknown, just ignore it
    if ( ( usbhidcode > 0xA4 && usbhidcode < 0xE0 ) || ( usbhidcode > 0xE7 ) ) {
        return 0; //TMP
    }

    // Loop through out look up array
    for(uint8_t i = 0 ; i < 2 ; i++) {   
        
        // ========== Modifers ==========
        // Send modifer key from different translation table
        if ( usbhidcode >= 0xE0 ) { 

            // ===== Write the keycode to PS2 =====
            // Send keycode if it is not zero
            if ( USB2PS2SMake[usbhidcode - 0xE0][KB_TYPE][i] != 0x00 ) {
                at_write( USB2PS2SMake[usbhidcode - 0xE0][KB_TYPE][i] );
            }

            // Next itter
            continue;
        }
        
        // ========== Pause/Break ==========
        if ( usbhidcode == 0x48 ) {
            // xt keyboard
            if ( KB_TYPE == 0 ) {
                xt_pause_press();
            } else {
                at_pause_press();
            }

            // Next itter
            continue;
        }

        // ========== Regular Keycodes ==========
        // Zend keycode if the keycode is not zero
        if ( USB2PS2Make[usbhidcode][KB_TYPE][i] != 0x00 ) {
            at_write( USB2PS2Make[usbhidcode][KB_TYPE][i] );
        }
    }

    return 0;
}

// Keyboard butons is depressed, takes usbhid keycode and convers it to ps2 keycode for writing to ps2 out
uint8_t keyboard_break( unsigned char usbhidcode ) {

    // ========== Unknown Code ==========
    // Incase the usbhid code sent is unknown, just ignore it
    if ( ( usbhidcode > 0xA4 && usbhidcode < 0xE0 ) || ( usbhidcode > 0xE7 ) ) {
        return 0; //TMP
    }

    // Loop through out look up array || kEYBOARD TYPE used as an XT/AT switch
    for(uint8_t i = 0 ; i < (KB_TYPE + 2) ; i++) {   

        // ========== Modifers ==========
        // Send modifer key from different translation table
        if ( usbhidcode >= 0xE0 ) {

            printf("special break");
            // ===== Blanks =====
            // Zend keycode if the keycode is not zero
            if ( USB2PS2SBreak[usbhidcode - 0xE0][KB_TYPE][i] != 0x00 ) {
                printf("%#8X\n", (USB2PS2SBreak[usbhidcode - 0xE0][KB_TYPE][i]));   // gives 0x000007

                at_write( USB2PS2SBreak[usbhidcode - 0xE0][KB_TYPE][i] );
            }

            // Next Itter 
            continue;
        }
        
        // ========== Regular Keycodes ==========
        // If the keycode is not zero
        if ( USB2PS2Break[usbhidcode][KB_TYPE][i] != 0x00 ) {
            // Send the keycode
            at_write( USB2PS2Break[usbhidcode][KB_TYPE][i] );
        }
    }

    return 0;
}



