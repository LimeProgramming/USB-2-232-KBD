#include "tusb.h"
#include <stdio.h>
#include <stdlib.h>
#include "bsp/board.h"
#include "pico/stdlib.h"

#include "utils.h"
#include "ctypes.h"
#include "hid_con.h"
#include <default_config.h>


//--------------------------------------------------------------------+
//          Controller Stuff
//--------------------------------------------------------------------+


//--------------------------------------------------------------------+
//          Controller Detecting Funcs
//--------------------------------------------------------------------+

// check if device is Sony DualShock 4
bool is_sony_ds4(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return ( (vid == 0x054c && (pid == 0x09cc || pid == 0x05c4)) // Sony DualShock4 
           || (vid == 0x0f0d && pid == 0x005e)                 // Hori FC4 
           || (vid == 0x0f0d && pid == 0x00ee)                 // Hori PS4 Mini (PS4-099U) 
           || (vid == 0x1f4f && pid == 0x1002)                 // ASW GG xrd controller
         );

}

// check if device is Sony PlayStation Classic controller
bool is_sony_psc(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return ( (vid == 0x054c && pid == 0x0CDA)); // Sony PlayStation Classic controller 
}


//--------------------------------------------------------------------+
//          PS4 Controller Example Code
//--------------------------------------------------------------------+

// check if different than 2
bool diff_than_2(uint8_t x, uint8_t y)
{
  return (x - y > 2) || (y - x > 2);
}

// check if 2 reports are different enough
bool diff_report(sony_ds4_report_t const* rpt1, sony_ds4_report_t const* rpt2)
{
  bool result;

  // x, y, z, rz must different than 2 to be counted
  result = diff_than_2(rpt1->x, rpt2->x) || diff_than_2(rpt1->y , rpt2->y ) ||
           diff_than_2(rpt1->z, rpt2->z) || diff_than_2(rpt1->rz, rpt2->rz);

  // check the reset with mem compare
  result |= memcmp(&rpt1->rz + 1, &rpt2->rz + 1, sizeof(sony_ds4_report_t)-4);

  return result;
}


//--------------------------------------------------------------------+
//          Controller Report Processors
//--------------------------------------------------------------------+

// Sony DualShock4
void process_sony_ds4(uint8_t const* report, uint16_t len)
{
  const char* dpad_str[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW", "none" };

  // previous report used to compare for changes
  static sony_ds4_report_t prev_report = { 0 };

  uint8_t const report_id = report[0];
  report++;
  len--;

  // all buttons state is stored in ID 1
  if (report_id == 1)
  {
    sony_ds4_report_t ds4_report;
    memcpy(&ds4_report, report, sizeof(ds4_report));

    // counter is +1, assign to make it easier to compare 2 report
    prev_report.counter = ds4_report.counter;

    // only print if changes since it is polled ~ 5ms
    // Since count+1 after each report and  x, y, z, rz fluctuate within 1 or 2
    // We need more than memcmp to check if report is different enough
    if ( diff_report(&prev_report, &ds4_report) )
    {
      printf("(x, y, z, rz) = (%u, %u, %u, %u)\r\n", ds4_report.x, ds4_report.y, ds4_report.z, ds4_report.rz);
      printf("DPad = %s ", dpad_str[ds4_report.dpad]);

      if (ds4_report.square   ) printf("Square ");
      if (ds4_report.cross    ) printf("Cross ");
      if (ds4_report.circle   ) printf("Circle ");
      if (ds4_report.triangle ) printf("Triangle ");

      if (ds4_report.l1       ) printf("L1 ");
      if (ds4_report.r1       ) printf("R1 ");
      if (ds4_report.l2       ) printf("L2 ");
      if (ds4_report.r2       ) printf("R2 ");

      if (ds4_report.share    ) printf("Share ");
      if (ds4_report.option   ) printf("Option ");
      if (ds4_report.l3       ) printf("L3 ");
      if (ds4_report.r3       ) printf("R3 ");

      if (ds4_report.ps       ) printf("PS ");
      if (ds4_report.tpad     ) printf("TPad ");

      printf("\r\n");
    }

    prev_report = ds4_report;
  }
}

// Sony PS1 Classic controller
void process_sony_psc(uint8_t const* report, uint16_t len) {


    // previous report used to compare for changes
    static sony_psc_report_t prev_report = { 0 };

    sony_psc_report_t psc_report;


    memcpy(&psc_report, report, sizeof(psc_report));


    if (psc_report.square   ) printf("Square ");
    if (psc_report.cross    ) printf("Cross ");
    if (psc_report.circle   ) printf("Circle ");
    if (psc_report.triangle ) printf("Triangle ");

    if (psc_report.l1       ) printf("L1 ");
    if (psc_report.r1       ) printf("R1 ");
    if (psc_report.l2       ) printf("L2 ");
    if (psc_report.r2       ) printf("R2 ");


    if (psc_report.start    ) printf("ST ");
    if (psc_report.select   ) printf("SEL ");

    // This suck so much
    switch (psc_report.dpadss)
    {
        case 0:
            printf("NW ");
            break;

        case 1:
            printf("N ");
            break;

        case 2:
            printf("NE ");
            break;

        case 4:
            printf("W ");
            break;
        
        
        case 5:
            //nothing;
            break;
    
        case 6:
            printf("E ");
            break;  
        
        case 8:
            printf("SW ");
            break;
        
        case 9:
            printf("S ");
            break;

        case 10:
            printf("SE ");
            break;
    };

    printf("\r\n");
}


//--------------------------------------------------------------------+
//          X-Input Callbacks
//--------------------------------------------------------------------+

// With xinput disabled, several data types are missing and the prog refuses to compile.
// this is just a lazy way to prevent errors
#if CFG_TUH_XINPUT

void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
    xinputh_interface_t *xid_itf = (xinputh_interface_t *)report;
    xinput_gamepad_t *p = &xid_itf->pad;
    const char* type_str;
    switch (xid_itf->type)
    {
        case 1: type_str = "Xbox One";          break;
        case 2: type_str = "Xbox 360 Wireless"; break;
        case 3: type_str = "Xbox 360 Wired";    break;
        case 4: type_str = "Xbox OG";           break;
        default: type_str = "Unknown";
    }

    if (xid_itf->connected && xid_itf->new_pad_data)
    {
        TU_LOG1("[%02x, %02x], Type: %s, Buttons %04x, LT: %02x RT: %02x, LX: %d, LY: %d, RX: %d, RY: %d\n",
             dev_addr, instance, type_str, p->wButtons, p->bLeftTrigger, p->bRightTrigger, p->sThumbLX, p->sThumbLY, p->sThumbRX, p->sThumbRY);

        //How to check specific buttons
        if (p->wButtons & XINPUT_GAMEPAD_A) TU_LOG1("You are pressing A\n");
    }
    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf)
{
    TU_LOG1("XINPUT MOUNTED %02x %d\n", dev_addr, instance);
    // If this is a Xbox 360 Wireless controller we need to wait for a connection packet
    // on the in pipe before setting LEDs etc. So just start getting data until a controller is connected.
    if (xinput_itf->type == XBOX360_WIRELESS && xinput_itf->connected == false)
    {
        tuh_xinput_receive_report(dev_addr, instance);
        return;
    }
    tuh_xinput_set_led(dev_addr, instance, 0, true);
    tuh_xinput_set_led(dev_addr, instance, 2, true);
    tuh_xinput_set_rumble(dev_addr, instance, 0, 0, true);
    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    TU_LOG1("XINPUT UNMOUNTED %02x %d\n", dev_addr, instance);
}

#endif