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

    return  (  (vid == 0x054c && (pid == 0x09cc || pid == 0x05c4))  // Sony DualShock4 
            || (vid == 0x0f0d && pid == 0x005e)                     // Hori FC4 
            || (vid == 0x0f0d && pid == 0x00ee)                     // Hori PS4 Mini (PS4-099U) 
            || (vid == 0x1f4f && pid == 0x1002)                     // ASW GG xrd controller
            );
}

// check if device is Sony PlayStation Classic controller
bool is_sony_psc(uint8_t dev_addr)
{
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    return ( (vid == 0x054c && pid == 0x0CDA)); // Sony PlayStation Classic controller 
}

// Check if device is a whitelisted controller
bool is_whitelisted_con(uint8_t dev_addr) 
{
    return  (   ( is_sony_ds4(dev_addr) )   // Is PS4 controller
            ||  ( is_sony_psc(dev_addr) )   // Is Playstation Classic Controller
    );
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

        /*
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
         
        */
    
    // Our custom generic gamepad report
    gamepad_report_t pad_report;

    // Load the weird report from TinyUSB into a custom data type
    sony_psc_report_t psc_report;
    memcpy(&psc_report, report, sizeof(psc_report));

    // ========== Buttons ==========
    // This code sucks so much, my god is it terrible
    if ( psc_report.square      ) pad_report.pad_btns |= GPAD_BTN_1;
    if ( psc_report.circle      ) pad_report.pad_btns |= GPAD_BTN_2;
    if ( psc_report.cross       ) pad_report.pad_btns |= GPAD_BTN_3;
    if ( psc_report.triangle    ) pad_report.pad_btns |= GPAD_BTN_4;
    if ( psc_report.l1          ) pad_report.pad_btns |= GPAD_LSHLDR;
    if ( psc_report.r1          ) pad_report.pad_btns |= GPAD_RSHLDR;
    if ( psc_report.l2          ) pad_report.pad_btns |= GPAD_LSHLDR_2;
    if ( psc_report.r2          ) pad_report.pad_btns |= GPAD_RSHLDR_2;
    if ( psc_report.start       ) pad_report.pad_btns |= GPAD_ST;
    if ( psc_report.select      ) pad_report.pad_btns |= GPAD_SEL;

    switch (psc_report.dpad)
    {
        case 0:     pad_report.pad_btns |= GPAD_DPAD_UP; pad_report.pad_btns |= GPAD_DPAD_LEFT;     break;
        case 1:     pad_report.pad_btns |= GPAD_DPAD_UP;                                            break;
        case 2:     pad_report.pad_btns |= GPAD_DPAD_UP; pad_report.pad_btns |= GPAD_DPAD_RIGHT;    break;
        case 4:     pad_report.pad_btns |= GPAD_DPAD_LEFT;                                          break;
        case 6:     pad_report.pad_btns |= GPAD_DPAD_RIGHT;                                         break;
        case 8:     pad_report.pad_btns |= GPAD_DPAD_DOWN; pad_report.pad_btns |= GPAD_DPAD_LEFT;   break;
        case 9:     pad_report.pad_btns |= GPAD_DPAD_DOWN;                                          break;
        case 10:    pad_report.pad_btns |= GPAD_DPAD_DOWN; pad_report.pad_btns |= GPAD_DPAD_RIGHT;  break;
    };

    // ========== Thumbsticks ==========
    // PS1 Classic controller doesn't have thumbsticks, so we'll just zero out the value to avoid errors
    // yeah, I know I could use memset instead

    for ( uint8_t i = 0 ; i < 4 ; i++ ) {
        pad_report.pad_thumb[i] = 0;
        pad_report.pad_thumb_raw[i] = 0;
    }

    // ========== Update ==========
    // Every time tinyusb host func is called, a report is generated for the PS1 Classic controller regardless if there is a difference or not.
    // If there is no difference, we don't care about the repeated data
    if ( pad_report.pad_btns != gpd_data.prev_report.pad_btns ) {
        print_binary(pad_report.pad_btns);
        printf("\r\n");
        fflush(stdout);

        gpd_data.prev_report = pad_report;
    }
    
    return;
}


//--------------------------------------------------------------------+
//          X-Input Callbacks
//--------------------------------------------------------------------+

// With xinput disabled, several data types are missing and the prog refuses to compile.
// this is just a lazy way to prevent errors
#if CFG_TUH_XINPUT

static xinput_gamepad_t pre_xbox_report_1;
static xinput_gamepad_t pre_xbox_report_2;


// ----------------------------------------
//          X-Input Tools
// ----------------------------------------

// is Xinput analog stick motion larger than a tolerance defined as tol
bool xinput_ana_bigger_than(int16_t new, int16_t old, uint8_t tol) {
   uint16_t unew = (uint16_t) new;
   uint16_t uold = (uint16_t) old;
   
   return ( (unew - uold > tol) || (uold - unew > tol) );
};

// is Xinput trigger motion larger than a tolerance defined as tol
bool xinput_trig_bigger_than(uint8_t new, uint8_t old, uint8_t tol) {
    return ( (new - old > tol) || (old - new > tol) );
};

// Checks if there is enough of a difference to qualify as an actual update.
// This is aimed to just remove the slight jitter that exists is in analog input devices. 
bool xinput_diff_report(xinput_gamepad_t * new, xinput_gamepad_t * old) {
    return(
        xinput_ana_bigger_than(     new->sThumbLX, old->sThumbLX, 10 )          ||
        xinput_ana_bigger_than(     new->sThumbLY, old->sThumbLY, 10 )          ||
        xinput_ana_bigger_than(     new->sThumbRX, old->sThumbRX, 10 )          ||
        xinput_ana_bigger_than(     new->sThumbRY, old->sThumbRY, 10 )          ||
        (new->wButtons > old->wButtons)                                         ||
        (old->wButtons > new->wButtons)
    );
};

// Work out if analog stick axis value falls outside of the deadzone area
int16_t xinput_ana_deadzone(int16_t val, uint16_t deadzone_area) {
    // If val is inside the deadsone_area then return 0
    if ( (val >= 0 ? val : val * (-1)) < deadzone_area ) { 
        return 0; 
    }    

    // if val is outside of deadzone_area then return val as is.
    return val;
};


// ----------------------------------------
//          X-Input Report
// ----------------------------------------

void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    xinputh_interface_t *xid_itf = (xinputh_interface_t *)report;
    //xinput_gamepad_t *p = &xid_itf->pad;

    //const char* type_str;
    //switch (xid_itf->type)
   // {
    //    case 1: type_str = "Xbox One";          break;
    //    case 2: type_str = "Xbox 360 Wireless"; break;
    //    case 3: type_str = "Xbox 360 Wired";    break;
    //    case 4: type_str = "Xbox OG";           break;
    //    default: type_str = "Unknown";
    //}
    
    
    // If pas is connected and it has some new data for us.
    if (xid_itf->connected && xid_itf->new_pad_data) {
        
        // Dump the report data to a non pointer value
        xinput_gamepad_t xreport;
        memcpy(&xreport, &xid_itf->pad, sizeof(xinput_gamepad_t));

        // Calculate the deadzone of the analog stick
        uint8_t storeddeadzone = 20;
        uint16_t deadzoneval = 32767 * ( (float) storeddeadzone/100 );  // var
        xreport.sThumbLX = xinput_ana_deadzone(xreport.sThumbLX, deadzoneval);
        xreport.sThumbLY = xinput_ana_deadzone(xreport.sThumbLY, deadzoneval);
        xreport.sThumbRX = xinput_ana_deadzone(xreport.sThumbRX, deadzoneval);
        xreport.sThumbRY = xinput_ana_deadzone(xreport.sThumbRY, deadzoneval);

        // Convert the trigger value from analog to digital based on a trigger value.
        // ===== Left Trigger
        if ( xinput_trig_bigger_than, xreport.bLeftTrigger, pre_xbox_report_1.bLeftTrigger, 10) {
            if ( xreport.bLeftTrigger > 200 ) { xreport.wButtons |= GPAD_LSHLDR_2; };     
        };

        // ===== Right Trigger
        if ( xinput_trig_bigger_than, xreport.bRightTrigger, pre_xbox_report_1.bRightTrigger, 10) {
            if ( xreport.bRightTrigger > 200 ) { xreport.wButtons |= GPAD_RSHLDR_2; }; 
        };  


        // If there's enough of a difference in the report for us to care about prcessing the new data.
        if ( xinput_diff_report(&xreport, &pre_xbox_report_1) ) {   

            //xreport.sThumbLX = (xreport.sThumbLX  / 256);    // === Constrain Left X-Axis 
            //xreport.sThumbLY = (xreport.sThumbLY  / 256);    // === Constrain Left Y-Axis
            //xreport.sThumbRX = (xreport.sThumbRX  / 256);    // === Constrain Right X-Axis
            //xreport.sThumbRY = (xreport.sThumbRY  / 256);    // === Constrain Right Y-Axis
            
            printf("Buttons %04x, LX: %d, LY: %d, RX: %d, RY: %d\n",
                xreport.wButtons, xreport.sThumbLX, xreport.sThumbLY, xreport.sThumbRX, xreport.sThumbRY);
            fflush(stdout);            


            // Is mouse movement inverted
            //if ( mouse_data.persistent.invert_x ) { xreport.sThumbLX = -( xreport.sThumbLX ); }
            //if ( mouse_data.persistent.invert_y ) { xreport.sThumbLY = -( xreport.sThumbLY ); }

           // mouse_data.rmpkt.x = constraini( ( mouse_data.rmpkt.x + xreport.sThumbLX ), -30000, 30000);
            //mouse_data.rmpkt.y = constraini( ( mouse_data.rmpkt.y + (xreport.sThumbLY * (-1)) ), -30000, 30000);


            // Increment mouse movement ticker for AVG movement style.
            //mouse_data.mouse_movt_ticker++;

            //How to check specific buttons
            //if (p->wButtons & XINPUT_GAMEPAD_A) TU_LOG1("You are pressing A\n");
            pre_xbox_report_1 = xreport;
        }
    
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