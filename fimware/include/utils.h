#include "tusb.h"
#include "pico.h"

#ifndef UTILS_H_ 
#define UTILS_H_

/*---------------------------------------*/
//                Utilites               //
/*---------------------------------------*/

int constraini(int16_t value, int16_t min, int16_t max);

void init_pinheader(uint pin);

void init_led(uint pin);

void machine_reboot();

void blink_aled(uint gpio, uint8_t code);

/*---------------------------------------*/
//            Repeating Timers           //
/*---------------------------------------*/

void startTerminalTimer();

void stopTerminalTimer();

void startPWRBlinkerTimer();

void stopPWRBlinkerTimer();

/*---------------------------------------*/
//          Mouse Settings Stuff         //
/*---------------------------------------*/

void calcSerialDelay();

void setDipSerialBaud();

void setDipMouseType();

void setDipMouseSpeed();

/*---------------------------------------*/
//             Dip Switch IRQ            //
/*---------------------------------------*/

void dipswGPIOCallback(uint gpio, uint32_t events);


/*---------------------------------------*/
//              DIN port IRQ             //
/*---------------------------------------*/

void dinPresentingCallback(uint gpio, uint32_t events);


/*---------------------------------------*/
//           Persistent Settings         //
/*---------------------------------------*/

void initPersistentSet();

void savePersistentSet();

void loadPersistentSet();

void loadPersistentMOUSEDefaults();

void loadPersistentKBDDefaults();

void loadPersistentDefaults();

void updateStoredDipswitchs();

/*---------------------------------------*/
//              DEBUG Tools              //
/*---------------------------------------*/

void blink_led_task(void);

/*---------------------------------------*/
//             Travel Limits             //
/*---------------------------------------*/

int16_t travel_limit(int16_t val, uint8_t percentage, uint16_t constainval);

int16_t travel_limit_d(int16_t val, double percentage, uint16_t constainval);

/*---------------------------------------*/
//            Mouse Processing           //
/*---------------------------------------*/

void set_mouseclick(uint8_t spot, bool value);

void process_mouse_report(hid_mouse_report_t const * report);

void reset_cycle();

void update_mousepacket();


/*---------------------------------------*/
//           Useful KBD functions        //
/*---------------------------------------*/

bool is_kb_connected(uint8_t kbd_addr, uint8_t kbd_inst);

void reset_kbd_defaults();

void set_not_present_locks(uint8_t addr, uint8_t inst);


/*---------------------------------------*/
//             Keyboard Timers           //
/*---------------------------------------*/

//int64_t start_idle_leds(alarm_id_t id, void *user_data);
int64_t idle_usb_leds(alarm_id_t id, void *user_data);


/*---------------------------------------*/
//             KBD Processing            //
/*---------------------------------------*/

void set_locks_from_din(uint8_t data);

void process_kbd_report(hid_keyboard_report_t const * report);

#endif