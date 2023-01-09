

/*---------------------------------------*/
//                 DEBUG                 //
/*---------------------------------------*/

// All this really does is give printf's out via UART0
#define DEBUG true          // Debug Flag for things


/*---------------------------------------*/
//                GPIO PINS              //
/*---------------------------------------*/

// DIP Switch Pins
#define DIPSW_THREEBTN 10   // Dip Switch 1 | LOGITECH
#define DIPSW_WHEEL 11      // Dip Switch 2 | MS Wheel
#define DIPSW_75XYSPEED 12  // Dip Switch 3 | 75% speed | Dip 3 + 4 depressed will set mouse speed to 25%
#define DIPSW_50XYSPEED 13  // Dip Switch 4 | 50% speed | Dip 3 + 4 depressed will set mouse speed to 25%
#define DIPSW_7N2 14        // Dip Switch 5 | 7N2 format
#define DIPSW_19200 15      // Dip Switch 6 | High Buad Rate

// LEDS 
#define LED_PWR 2           // Power LED
#define LED_MOUSE 3         // Mouse LED
#define LED_KBD 4           // Keybaord LED

//UART stuff
#define UART_ID uart1       // Use UART1, keep UART0 for DEBUG printing
#define UART_TX_PIN 8       // UART1 TX --> MAX232 pin 11
#define UART_RX_PIN 9       // UART1 RX --> MAX232 pin 12
#define UART_CTS_PIN 6      // CTS      --> MAX232 pin 9
#define UART_RTS_PIN 7      // RTS      --> MAX232 pin 10
#define AUX_RTS_PIN 5       // RTS 2

// PS2 GPIO
#define PS2_CLOCK_IN 19     // Clock signal line in from ps2 port
#define PS2_CLOCK_OUT 21    // Clock signal line out, trigger NPN to ground clock line
#define PS2_DATA_IN 20      // Data signal line in from ps2 port
#define PS2_DATA_OUT 18     // Clock signal line out, trigger NPN to ground clock line

// RESET
#define RESET_FLASH 17      // Button for resetting the stored settings back to default, defined below.


/*---------------------------------------*/
//            Keyboard Settings          //
/*---------------------------------------*/

// Keyboard
#define KB_ENABLE true      // Enable Keyboard

// Max number of usb keyboards supported
#define KB_MAX_KEYBOARDS  4

// Set your keyboard type (XT type currently not supported)
//#define KB_TYPE  1          // 0 -> XT | 1 -> AT/PS2

// Due to timings, mimicking a real ibmxt might not be possible but we'll try anyway
// Does not apply to AT/ps2 keyboards
//#define KB_XTCLONE 1        // 0 -> IBM XT | 1 --> XT Clone


/*---------------------------------------*/
//            Settings Defaults          //
/*---------------------------------------*/
// These are used until they are changed via software for dipswitches

// global limit | Range 1 -> 200
static uint8_t default_xytravel_percentage = 100;

// seperate x and y limits. | Range 1 -> 200
static uint8_t default_xtravel_percentage = 100;
static uint8_t default_ytravel_percentage = 100;

// TWOBTN = 0 | THREEBTN = 1 | WHEELBTN = 2 
static int8_t default_mousetype = 0;

// Double Stop Bit
// 7N1 = false | 7N2 = true
static bool default_doublestopbit = false;

// Baud Rate
/* ---------- Acceptible baud rates ----------
        1200 | 2400 | 4800 | 9600 | 19200
            Recommended value is 1200 

Any values outside of the ones above, are rejected
and the default value of 1200 is used.

Note:
As far as I can see from docs, serial mice ran at
1200 Baud, so you will need modified drivers for higher baud rates
   ------------------------------------------ */

static uint16_t default_baudrate = 1200;

// Swap left and right mouse buttons
// Could be handy for left handed people.
static bool default_swap_left_right = false;

// Uses the forward and back buttons of a mouse as alturnative left and right mouse buttons. 
// The forward and back butons are on the side of the mouse and usually act as the page forward and backwards in file and web browsers. 
// Forward button -> left click | Back Button -> right click
static bool default_use_forward_backward = false;

// Changes above to 
// Forward button -> right click | Back Button -> left click
static bool default_swap_forward_backward = false;

// Invert X axis (Left and Right movement)
static bool default_invert_x = false;

// Invert Y axis (Up and Down movement)
static bool default_invert_y = false;

// Added version 1.1.X
/*
/==-- USB mice communicate with the pico faster than the pico communicates with the serial port. 
/==-- Movement type determines what is done wit the extra packets of data. 
/==-- Note: Applies only to X and Y mouse movement
​/==--
/==-- ADDITIVE:
/==--       Adds up the mouse movement and sends it off in the next serial packet. 
/==--       Results in a very sensitive feeling mouse, sometimes too sensitive  and that's where Averaging comes in.
/==--
/==-- AVERAGE:
/==--       Finds the average change in mouse movement and sends that off in the next serial packet.
/==--       Results in a fairly insensitive  feeling mouse, could be useful for a higher DPI mouse or personal preference.
/==--
/==-- COAST:
/==--       Unlike Additive and Average; Coast does not throw away extra movement when the mouse is moved quickly, instead it incrementally sends out the movement. 
/==--       Results in a cursor feeling like it's skidding along on ice at high speeds. I don't know where it would be useful but it's fun to mess with!
/==-- 0 -> Additive axis movement | 1 -> Average axis movement | 2-> Coast
*/
static uint8_t default_mouse_movt_type = 0;

/*
/==-- COSINE SMOOTHING 
/==-- Makes the cursor proportionally less sensitive  at high speeds, leaving the movement mostly one to one at low speeds. 
/==-- This was added as an attempt to mitigate the problem of the much more sensitive  optical mouse overshooting icons on a low res screen. 
/==-- It could be useful for point and click DOS games but is not recommended for games where mouse movement controls a camera.
/==-- 0 -> Disabled | 1 -> Low | 2 -> Medium | 3 -> High | 4 - > Very High
*/
static uint8_t default_use_cosine_smoothing = 0;


// Added version 1.2.X
/*
/==-- LANGUAGE
/==-- 0 -> English | 1 -> German 
*/
static uint8_t default_language = 0;


// Added in Version 1 of KBD

// Set your keyboard type (XT type currently not supported)
static uint8_t default_keyboard_type = 1;       // 0 -> XT | 1 -> AT/PS2

// Due to timings, mimicking a real ibmxt might not be possible but we'll try anyway
// Does not apply to AT/ps2 keyboards
static uint8_t default_xtclone = 1 ;            // 0 -> IBM XT | 1 --> XT Clone

// Which scan code set should we use while the adapter is an AT keyboard
// This can be overwritten but not saved by the PS/2 command set functions.
// XT keyboard mode will use set 1 regardless of this setting.
// Auto will pick the scan code set based on keyboard type above and the ps2 command set
//==-- 0 -> AUTO (DEFAULT) | 1 -> Set 1 | 2 -> set2 | 3 -> set 3 (rarely used)
//static uint8_t default_ps2codeset = 0;


/*---------------------------------------*/
//            Advanced Settings          //
/*---------------------------------------*/


/* ---------------------------------------------------------- */
/*  Numbers for Serial Speed and Delay times


    // Works for bits per microsecond
    1 bit =  ( 1000000 / (real_baudrate) );

    // Work out the time for 1, 3, 4 mouse packet(s)
    1 Packet = (1 bit * (doublestopbit ? 9 : 8) ) + TXWIGGLEROOM;
    3 Packets = 1 Packet * 3;
    4 Packets = 1 Packet * 4;

    SerialDelay is the time between when serial packets are sent, based on the math above */
/* ---------------------------------------------------------- */

// This is for adding a bit of padding to the serial delays, shouldn't be needed but some controllers are awkward.
static uint32_t TXWIGGLEROOM    = 0; 
