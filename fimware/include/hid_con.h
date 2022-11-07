#include "pico.h"

#ifndef HID_CON_H_
#define HID_APP_H_

#include "bsp/board.h"
#include "tusb.h"


//--------------------------------------------------------------------+
//          Controller Detecting Funcs
//--------------------------------------------------------------------+

// check if device is Sony DualShock 4
bool is_sony_ds4(uint8_t dev_addr);

// check if device is Sony PlayStation Classic controller
bool is_sony_psc(uint8_t dev_addr);

//--------------------------------------------------------------------+
//          Sony DualShock 4 Report
//--------------------------------------------------------------------+

// Gamepad info From https://www.kernel.org/doc/html/latest/input/gamepad.html
// Sony DS4 report layout detail https://www.psdevwiki.com/ps4/DS4-USB
typedef struct TU_ATTR_PACKED
{
  uint8_t x, y, z, rz; // joystick

  struct {
    uint8_t dpad     : 4; // (hat format, 0x08 is released, 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW)
    uint8_t square   : 1; // west
    uint8_t cross    : 1; // south
    uint8_t circle   : 1; // east
    uint8_t triangle : 1; // north
  };

  struct {
    uint8_t l1     : 1;
    uint8_t r1     : 1;
    uint8_t l2     : 1;
    uint8_t r2     : 1;
    uint8_t share  : 1;
    uint8_t option : 1;
    uint8_t l3     : 1;
    uint8_t r3     : 1;
  };

  struct {
    uint8_t ps      : 1; // playstation button
    uint8_t tpad    : 1; // track pad click
    uint8_t counter : 6; // +1 each report
  };

  // comment out since not used by this example
  // uint8_t l2_trigger; // 0 released, 0xff fully pressed
  // uint8_t r2_trigger; // as above

  //  uint16_t timestamp;
  //  uint8_t  battery;
  //
  //  int16_t gyro[3];  // x, y, z;
  //  int16_t accel[3]; // x, y, z

  // there is still lots more info

} sony_ds4_report_t;

//--------------------------------------------------------------------+
//          Sony Classic Controller Report
//--------------------------------------------------------------------+
typedef struct TU_ATTR_PACKED
{
  struct {

    uint8_t triangle    : 1;
    uint8_t circle      : 1;
    uint8_t cross       : 1;
    uint8_t square      : 1; 
    uint8_t l2          : 1;
    uint8_t r2          : 1;
    uint8_t l1          : 1;
    uint8_t r1          : 1;
    
  };

  struct {
    uint8_t select      : 1;
    uint8_t start       : 1;
    uint8_t dpadss     : 4; //(hat format, 0x08 is released, 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW)
  };

} sony_psc_report_t;

//--------------------------------------------------------------------+
//          Controller Report Processors
//--------------------------------------------------------------------+

void process_sony_ds4(uint8_t const* report, uint16_t len);

void process_sony_psc(uint8_t const* report, uint16_t len);


//--------------------------------------------------------------------+
//          X-Input callbacks
//--------------------------------------------------------------------+

#if CFG_TUH_XINPUT
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len);
void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf);
void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance);
#endif

#endif // HID_CON_H_