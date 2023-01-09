// These two lines prevents the file from being included multiple
// times in the same source file
#ifndef CTYPES_H_
#define CTYPES_H_1

#include "pico.h"


/* -------------------- Some Basic emums -------------------- */

// Mouse tracking speed modifier
enum LED_STATE {
  LED_OFF = 0,  
  LED_ON = 1,   
  LED_UNKNOWN_DEVICE = 2,
  LED_UNKNOWN_DEVICE_1 = 3,
  LED_SERIAL_CONNECTED = 4,
  LED_SERIAL_CONNECTED_1 = 5,
  LED_SERIAL_DISCONNECTED = 6,
  LED_SERIAL_DISCONNECTED_1 = 7,
};

// Mouse Type enum
enum MO_TYPE {
  TWOBTN = 0,   // MS Two Button
  THREEBTN = 1,   // Logitech
  WHEELBTN = 2  // MS Wheel
};

// States of mouse init request from PC
enum PC_INIT_STATES {
  CTS_UNINIT   = 0, // Initial state
  CTS_LOW_INIT = 1, // CTS pin has been set low, wait for high.
  CTS_TOGGLED  = 2  // CTS was low, now high -> do ident.
};

// Mouse Movement Type
enum MO_MVT_TYPE {
  MO_MVT_ADDITIVE  = 0, // Cumulative and constrain the X and Y values.
  MO_MVT_AVERAGE = 1, // Average X and Y values.
  MO_MVT_COAST  = 2  // Coast X and Y values.
};

// Cosine Smoothed
enum MO_MVT_COS {
  MO_MVT_COS_DISABLED = 0,
  MO_MVT_COS_LOW = 1,
  MO_MVT_COS_MEDIUM = 2,
  MO_MVT_COS_HIGH = 3,
  MO_MVT_COS_VERYHIGH = 4
};

// Mouse Type enum
enum CORE_FLAGS {
  cf_nothing = 0,
  cf_stop = 1, 
  cf_resume = 2,
  cf_update = 3,
  cf_post = 4,
  cf_nopost = 5,
  cf_save_settings = 6
};

// LED values from the AT keyboard controller
enum AT_KB_LED {
  AT_KB_LED_UNCHANGED = -1,
  AT_KB_LED_NONE      = 0,
  AT_KB_LED_S         = 1,
  AT_KB_LED_N         = 2,
  AT_KB_LED_NS        = 3,
  AT_KB_LED_C         = 4,
  AT_KB_LED_CS        = 5,
  AT_KB_LED_NC        = 6,
  AT_KB_LED_NCS       = 7
};


enum DIN_CONN_RETS {
  DIN_RET_DISCONN= 7, // Disconnected
  DIN_RET_UNAVAIL= 6, // Unavailable
  DIN_RET_PRFAIL = 5, // ?
  DIN_RET_TOUT   = 4, // Time out
  DIN_RET_DCFAIL = 3, // Data and clock fail
  DIN_RET_DFAIL  = 2, // DAta Fail
  DIN_RET_CFAIL  = 1, // Clock Fail
  DIN_RET_SUCC   = 0
};


/* -------------------- Mouse Packet Structure -------------------- */
/*  This is the hold the mouse state for the previous cycle. 
    Each cycle is defined by the Baud rate. */

typedef struct {    
    // State of left, middle and right buttons                        
    bool left, middle, right;

    // mouse location delta
    int16_t  x, y, wheel;

    // Is update required this cycle?
    bool update;

} MOUSE_PKT;

/* -------------------- Raw Mouse Packet Structure -------------------- */
/*  This is the hold the mouse state for the previous cycle. 
    Each cycle is defined by the Baud rate. */
typedef struct {    

  // The Button Flip Flop       | Left Click + Middle Click + Right Click
  bool btnFlipFlop[3];  

  // FlipFlop Cycle Update Flag | Left Click + Middle Click + Right Click       
  bool btnUpdated[3];   

  // FlipFlop toggle flag       | Left Click + Middle Click + Right Click                
  bool btnToggle[3];                     

  // Mouse location delta       | X + Y + Wheel
  int16_t  x, y, wheel;

} MOUSE_RPKT;

/* -------------------- Persistent Data Structure -------------------- */
/*  This is to hold the mouse data that is to survive hard reboots */

typedef struct  {
    // 255 = first time execution | false = ran before.
    uint8_t firstrun;

    // global limit | Range 1 -> 200
    uint8_t xytravel_percentage;

    // separate x and y limits. | Range 1 -> 200
    uint8_t xtravel_percentage;
    uint8_t ytravel_percentage;

    // TWOBTN = 0 | THREEBTN = 1 | WHEELBTN = 2 
    uint8_t mousetype;

    // Double Stop Bit
    // 7N1 = 0 | 7N2 = 1
    bool doublestopbit;

    // Baud rate 
    // 1200 | 2400 | 4800 | 9600 | 19200
    uint baudrate;

    // Swap the left and right buttons 
    bool swap_left_right;

    // use forward and backward as ALT left and right buttons
    bool use_forward_backward;

    // Swap forward and backwards
    bool swap_forward_backward;

    // Invert X and Y movement
    bool invert_x;
    bool invert_y;

    // Mouse Movement Type enum MO_MVT_TYPE
    uint8_t mouse_movt_type;

    // USe cosine smoothing
    uint8_t use_cosine_smoothing;

    // Firmware Versioning
    uint8_t FW_V_MAJOR;
    uint8_t FW_V_MINOR;
    uint8_t FW_V_REVISION;

    // Previous DipSwitch Button State
    bool ST_DIPSW_THREEBTN;         // DIP 1
    bool ST_DIPSW_WHEEL;            // DIP 2
    bool ST_DIPSW_75XYSPEED;        // DIP 3
    bool ST_DIPSW_50XYSPEED;        // DIP 4
    bool ST_DIPSW_7N2;              // DIP 5
    bool ST_DIPSW_19200;            // DIP 6

    uint8_t language;

} PERSISTENT_MOUSE_DATA; 

extern PERSISTENT_MOUSE_DATA pmData;

/* -------------------- Mouse Settings -------------------- */
// Mouse Settings and information

typedef struct {                            

  // CTS state tracker | taken from Aviancer's code since it was more straightforward than what I had already
  uint8_t pc_state;

  // yeah that should be a bitwise thing but eh
  // serial state tracker. 0 = Mouse mode | 1 = terminal mode uart 0 | 2 = terminal mode uart 1 | 3 = paused
  uint8_t serial_state;

  // Current Processed Mouse Packet.                    
  MOUSE_PKT mpkt;                       

  // Raw Mouse data.                    
  MOUSE_RPKT rmpkt;    

  // Persistent Mouse data, survives reboots.
  PERSISTENT_MOUSE_DATA persistent;               

  // Is mouse connected flag
  uint8_t mouse_count;
  
  // count the number of mouse updates between serial cycles. USed for AVG mouse movement
  uint8_t mouse_movt_ticker;

  // The real Baudrate returned by pico
  uint realbaudrate;

  // One Bit Delay Time, calculated on startup.
  float serialdelay_1b;    

  // One Byte Delay Time, calculated on startup.
  uint serialdelay_1B;        

  // Three Byte Delay Time, calculated on startup.         
  uint serialdelay_3B;

  // Four Byte Delay Time, calculated on startup.
  uint serialdelay_4B;


} MOUSE_DATA;

// Extern value, declared again in usb-2-232.c, can be used everywhere ctypes is included.
extern MOUSE_DATA mouse_data;


/* -------------------- Keyboard Data -------------------- */

typedef struct  {

  // 255 = first time execution | false = ran before.
  uint8_t firstrun;

  // Set your keyboard type (XT type currently not supported)
  // 0 -> for XT | 1 -> AT
  uint8_t kbd_type;

  // Due to timings, mimicking a real ibmxt might not be possible but we'll try anyway
  // 0 -> IBM XT | 1 -> Clone
  // Does not apply to AT/ps2 keyboards
  bool kbd_xtclone;

  // Which scan code set should we use while the adapter is an AT keyboard
  // This can be overwritten but not saved by the PS/2 command set functions.
  // XT keyboard mode will use set 1 regardless of this setting.
  //==-- 1 -> Set 1 | 2 -> set2 (default) | 3 -> set 3 (rarely used)
  //uint8_t kbd_ps2_codeset;

} PERSISTENT_KBD_DATA;


typedef struct {
  // Should the keyboard be scanning?
  // Set command values of F5h for disable and load defaults and F4 for enable
  bool kbd_enabled;

  // Key make, break or typematic. Tells the keyboard if the keys should send make, break codes or be typematic.
  // Set command values of FAh, F9h, F8h, F7h 
  // Bitwise ops. 0000-0mkbktm | 1 == enabled 0 == disabled
  uint8_t key_mkbktm;

  // Store the USB hid keycode of the most recently pressed key. 
  uint8_t tm_key;

  // The timestamp of when the most recently pressed key was pressed.
  absolute_time_t tm_timestamp;

  // Stores the number of milliseconds between presses of the typematic key
  uint16_t tm_rate;

  // Store the number of milliseconds after the key is pressed before it becomes typematic
  uint16_t tm_delay;

  // Are we using PS2 set 1, 2 or 3
  // 0x00 -> set 1 | 0x01 -> set 2 | 0x02 -> set 3
  uint8_t scancode_set;

  // LEDS
  int8_t led_state;

} KBD_CMD_SET;


typedef struct {

    // How many Keyboards are connected
  uint8_t kbd_count;

  // 4 keyboards [device addr, device instance]
  uint8_t kbd_tusb_addr[4][2];

  // Previous report of keyboard
  hid_keyboard_report_t kbd_tusb_prev_report[4];

  // Set to true if we think the din port is connected to a computer.
  bool din_present;

  // Set to true if we know that we have initialized with the host computer
  bool din_initalised;

  // Connection failure tracker
  uint8_t din_conn_fail;

  // Alarm ID for the idle lock timer
  alarm_id_t idle_lock_timer_id;

  // Time for when to poll the host computer
  absolute_time_t din_polling_target;
  
  // command set data
  KBD_CMD_SET cmd_set;

  // persistent keyboard data, survives reboots.
  PERSISTENT_KBD_DATA persistent;

} KEYBOARD_DATA;

// Extern value, declared again in usb-2-232.c, can be used everywhere ctypes is included.
extern KEYBOARD_DATA kbd_data;


/* -------------------- Controller Data -------------------- */
// ============================================================

/* Modified ASCI graphic originally from https://www.kernel.org/doc/html/latest/input/gamepad.html

         ________              ________          __
        / [_L2_] \            / [_R2_] \           |
       / [_ L1 _] \          / [_ R1 _] \          | Front Triggers
     _/------------\________/------------\_      __|
    /    UP         __    __         (4)   \       |
   /     ||        |SE|  |ST|     _       _ \      | Main Pad
  {  L ==||== R  ___        ___  (3) -|- (2) }     |
   \     ||     /   \      /   \      _     /    __|   
    \    DN    { LS  }    {  RS }    (1)   /       |
     \_________ \___/ ____ \___/ _________/        | Control Sticks
               \_____/    \_____/                __| 
 
      |________|______|   |______|___________|
        D-Pad    Left       Right   Action Pad
                Stick       Stick
*/

#define GPAD_DPAD_UP    0x0001
#define GPAD_DPAD_DOWN  0x0002
#define GPAD_DPAD_LEFT  0x0004
#define GPAD_DPAD_RIGHT 0x0008
#define GPAD_ST         0x0010
#define GPAD_SEL        0x0020
#define GPAD_L_STICK    0x0040
#define GPAD_R_STICK    0x0080
#define GPAD_LSHLDR     0x0100
#define GPAD_RSHLDR     0x0200
#define GPAD_LSHLDR_2   0x0400
#define GPAD_RSHLDR_2   0x0800
#define GPAD_BTN_1      0x1000
#define GPAD_BTN_2      0x2000
#define GPAD_BTN_3      0x4000
#define GPAD_BTN_4      0x8000

typedef struct {

  /* ===== Buttons ===== */
  // Bitwise mask defined above
  //  0000      0000      0000        0000 
  //  ||||      ||||      ||||        ||||
  //  |||BTN1   |||L1     |||START    |||UP
  //  ||BTN2    ||R1      ||SELECT    ||DOWN
  //  |BTN3     |L2       |L-STICK    |LEFT
  //  BTN4      R2        R-STICK     RIGHT

  uint16_t pad_btns;

  // [LX LY RX RY]
  // These values are stored unprocessed for the sake of detecting change in any new values in from the controller
  int16_t pad_thumb_raw[4];

  // [LX LY RX RY]
  // Processed and limited values of our controllers thumbsticks
  int8_t pad_thumb[4];

} gamepad_report_t;


typedef struct {

  // Gamepad connected tracker
  bool gpd_con;

  // 4 keyboards [device addr, device instance]
  uint8_t gpd_tusb_addr[2];

  // Previous gamepad report
  gamepad_report_t prev_report;

} GAMEPAD_DATA;


// Extern value, declared again in usb-2-232.c, can be used everywhere ctypes is included.
extern GAMEPAD_DATA gpd_data;

/* -------------------- Make the file work stuff -------------------- */

#if CTYPES_C_ 

enum MO_SPEED MO_SPEED;
enum MO_TYPE MO_TYPE;
enum PC_INIT_STATES PC_INIT_STATES;
enum MO_MVT_TYPE MO_MVT_TYPE;
enum MO_MVT_COS MO_MVT_COS;
enum CORE_FLAGS CORE_FLAGS;
enum AT_KB_LED AT_KB_LED;
enum PS2_WRITE_RETS PS2_WRITE_RETS;
//PersistentData persistentData;

#else

extern enum MO_SPEED MO_SPEED;
extern enum MO_TYPE MO_TYPE;
extern enum PC_INIT_STATES PC_INIT_STATES;
extern enum MO_MVT_TYPE MO_MVT_TYPE;
extern enum MO_MVT_COS MO_MVT_COS;
extern enum CORE_FLAGS CORE_FLAGS;
extern enum AT_KB_LED AT_KB_LED;
extern enum PS2_WRITE_RETS PS2_WRITE_RETS;
//extern PersistentData persistentData;


static uint8_t TUSB_KB_LED_NONE  = 0;
static uint8_t TUSB_KB_LED_N     = 1;
static uint8_t TUSB_KB_LED_C     = 2;
static uint8_t TUSB_KB_LED_NC    = 3;
static uint8_t TUSB_KB_LED_S     = 4;
static uint8_t TUSB_KB_LED_NS    = 5;
static uint8_t TUSB_KB_LED_CS    = 6;
static uint8_t TUSB_KB_LED_NCS   = 7;

#endif

#endif // CTYPES_H_