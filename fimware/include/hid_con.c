#include "tusb.h"
#include "bsp/board.h"
#include "pico/stdlib.h"

#include "utils.h"
#include "hid_con.h"
#include <default_config.h>



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
//          Controller Util Functions
//--------------------------------------------------------------------+

// Is gamepad analog stick difference in motion larger than a tolerance defined as tol
// Also to handle the deadzone, if a value is no longer zero, then return is true
bool gmp_thumb_diff_than(uint16_t new, uint16_t old, uint8_t tol) {
   return   (   (new - old > tol) || 
                (old - new > tol) ||
                (new == 0 && old != 0) ||
                (new != 0 && old == 0)
            );
};

// Checks if there is enough of a difference to qualify as an actual update.
// This is aimed to just remove the slight jitter that exists in analog input devices. 
bool gmp_diff_report(gamepad_report_t * new, gamepad_report_t * old, uint8_t atol) {
    return(
        gmp_thumb_diff_than(     (uint16_t) new->pad_thumb_raw[0], (uint16_t) old->pad_thumb_raw[0], atol )           ||
        gmp_thumb_diff_than(     (uint16_t) new->pad_thumb_raw[1], (uint16_t) old->pad_thumb_raw[1], atol )           ||
        gmp_thumb_diff_than(     (uint16_t) new->pad_thumb_raw[2], (uint16_t) old->pad_thumb_raw[2], atol )           ||
        gmp_thumb_diff_than(     (uint16_t) new->pad_thumb_raw[3], (uint16_t) old->pad_thumb_raw[3], atol )           ||
        new->pad_btns != old->pad_btns
    );
};

// Zero out a custom gamepad report variable
void gmp_blank_report(gamepad_report_t* pad_report) {

    // ===== Zero-ing out our report =====
    // This is just to make sure that our custom report type is actually zerod out vs random numbers

    // ----- Thumbsticks -----
    for ( uint8_t i = 0 ; i < 4 ; i++ ) {
        pad_report->pad_thumb[i] = 0;
        pad_report->pad_thumb_raw[i] = 0;
    }

    // ----- Buttons -----
    pad_report->pad_btns = 0;

    return;
}

// Return thumbstick axis value corrected for deadzone. 
// Axis value defined as val, max unsigned travel of axis: max_limit, deadzone percentage: deadzone_pc
// If val is inside the deadsone_area then return is 0
int16_t gmp_thumb_deadzoned(int16_t val, uint16_t max_limit, uint8_t deadzone_pc ) {

    uint16_t deadzone_area = max_limit * ( (float) deadzone_pc/100 );

    if ( (val >= 0 ? val : val * (-1)) < deadzone_area ) { 
        return 0; 
    }    

    // if val is outside of deadzone_area then return val as is.
    return val;
};



//--------------------------------------------------------------------+
//          Controller Report Processors
//--------------------------------------------------------------------+

// Process the Sony PlayStation DualShock 4 controller report into our custom report type
void process_sony_ds4(uint8_t const* report, uint16_t len) {

  uint8_t const report_id = report[0];
  report++;

  // all buttons state is stored in ID 1
  if (report_id == 1) {

    // ===== Our Custom gamepad report type
    gamepad_report_t pad_report;    // Make the variable
    gmp_blank_report(&pad_report);      // Blank the variable

    // Load the weird report from TinyUSB into a custom data type
    sony_ds4_report_t ds4_report;
    memcpy(&ds4_report, report, sizeof(ds4_report));
    
    // counter is +1, assign to make it easier to compare 2 report
    //prev_report.counter = ds4_report.counter;

    // ========== Buttons ==========
    // This code sucks so much, my god is it terrible
    if ( ds4_report.square      ) pad_report.pad_btns |= GPAD_BTN_1;
    if ( ds4_report.circle      ) pad_report.pad_btns |= GPAD_BTN_2;
    if ( ds4_report.cross       ) pad_report.pad_btns |= GPAD_BTN_3;
    if ( ds4_report.triangle    ) pad_report.pad_btns |= GPAD_BTN_4;
    if ( ds4_report.l1          ) pad_report.pad_btns |= GPAD_LSHLDR;
    if ( ds4_report.r1          ) pad_report.pad_btns |= GPAD_RSHLDR;
    if ( ds4_report.l2          ) pad_report.pad_btns |= GPAD_LSHLDR_2;
    if ( ds4_report.r2          ) pad_report.pad_btns |= GPAD_RSHLDR_2;
    if ( ds4_report.l3          ) pad_report.pad_btns |= GPAD_L_STICK;
    if ( ds4_report.r3          ) pad_report.pad_btns |= GPAD_R_STICK;
    if ( ds4_report.option      ) pad_report.pad_btns |= GPAD_ST;
    if ( ds4_report.share       ) pad_report.pad_btns |= GPAD_SEL;

    // D-pad buttons
    switch (ds4_report.dpad)
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
    // PS4 used unsigned 8 bit int for it's analog stick axis's. Seems nice on paper but it's a bit of a pain to make it work nicely with the mouse and xbox controller code
    // So I convert it to a signed int and subtract 128 on the fly so the zero point on the stick should be 0,0.

    // Left X Axis
    pad_report.pad_thumb_raw[0] = gmp_thumb_deadzoned( ( ((short) ds4_report.lx) - 128), 256, 10 );
    pad_report.pad_thumb[0]     = constraini(pad_report.pad_thumb_raw[0], -127, 127);

    // Left Y Axis
    pad_report.pad_thumb_raw[1] = gmp_thumb_deadzoned( ( ((short) ds4_report.ly) - 128), 256, 10 );
    pad_report.pad_thumb[1]     = constraini(pad_report.pad_thumb_raw[1], -127, 127);

    // Right X Axis
    pad_report.pad_thumb_raw[2] = gmp_thumb_deadzoned( ( ((short) ds4_report.rx) - 128), 256, 10 );
    pad_report.pad_thumb[2]     = constraini(pad_report.pad_thumb_raw[2], -127, 127);

    // Right Y Axis
    pad_report.pad_thumb_raw[3] = gmp_thumb_deadzoned( ( ((short) ds4_report.ry) - 128), 256, 10 );
    pad_report.pad_thumb[3]     = constraini(pad_report.pad_thumb_raw[3], -127, 127);


    // ========== Update ==========
    // Every time tinyusb host func is called, a report is generated for the PS4 DualShock4 regardless if there is a difference or not.
    // If there is no difference, we don't care about the repeated data
    if ( gmp_diff_report(&pad_report, &gpd_data.prev_report, 2) ) {
        print_binary(pad_report.pad_btns);
        printf("        %d", pad_report.pad_btns );
        printf("\r\n");
        fflush(stdout);

        gpd_data.prev_report = pad_report;
    };


  } // END if (report_id == 1)
  
}

// Sony PS1 Classic controller
void process_sony_psc(uint8_t const* report, uint16_t len) {
    /* Process the Sony PlayStation one classic controller report into our custom report type */

    // ===== Our Custom gamepad report type
    gamepad_report_t pad_report;    // Make the variable
    gmp_blank_report(&pad_report);      // Blank the variable

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

    // ========== Update ==========
    // Every time tinyusb host func is called, a report is generated for the PS1 Classic controller regardless if there is a difference or not.
    // If there is no difference, we don't care about the repeated data
    if ( gmp_diff_report(&pad_report, &gpd_data.prev_report, 2) ) {
        print_binary(pad_report.pad_btns);
        printf("        %d", pad_report.pad_btns );
        printf("\r\n");
        fflush(stdout);

        gpd_data.prev_report = pad_report;
    }
    
    return;
}



//--------------------------------------------------------------------+
//          X-Input Callbacks
//--------------------------------------------------------------------+
// X-input Lib Source: https://github.com/Ryzee119/tusb_xinput

#if CFG_TUH_XINPUT

// Report received from GamePad
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {

    xinputh_interface_t *xid_itf = (xinputh_interface_t *)report;

    // If pad is connected and it has some new data for us.
    if (xid_itf->connected && xid_itf->new_pad_data) {
        
        // ========== Data ==========
        // ===== Our Custom gamepad report type
        gamepad_report_t pad_report;        // Make the variable
        gmp_blank_report(&pad_report);      // Blank the variable

        // ===== Dump the report data to a non pointer value
        xinput_gamepad_t xreport;
        memcpy(&xreport, &xid_itf->pad, sizeof(xinput_gamepad_t));
        

        // ========== Thumbsticks ==========
        // Process data for deadzone and limit travel to a 8 bit int
        pad_report.pad_thumb_raw[0] = gmp_thumb_deadzoned( xreport.sThumbLX, 65535, 10 );
        pad_report.pad_thumb[0]     = constraini( (pad_report.pad_thumb_raw[0] / 256), -127, 127);

        pad_report.pad_thumb_raw[1] = gmp_thumb_deadzoned( xreport.sThumbLY, 65535, 10 );
        pad_report.pad_thumb[1]     = constraini( (pad_report.pad_thumb_raw[1] / 256) * -1, -127, 127);

        pad_report.pad_thumb_raw[2] = gmp_thumb_deadzoned( xreport.sThumbRX, 65535, 10 );
        pad_report.pad_thumb[2]     = constraini( (pad_report.pad_thumb_raw[2] / 256), -127, 127);

        pad_report.pad_thumb_raw[3] = gmp_thumb_deadzoned( xreport.sThumbRY, 65535, 10 );
        pad_report.pad_thumb[3]     = constraini( (pad_report.pad_thumb_raw[3] / 256), -127, 127);


        // ========== Buttons ==========
        // Dump xreport buttons into our pad report buttons var. The bitwise mask are the same for everything except left and right shoulder 2 buttons
        pad_report.pad_btns = xreport.wButtons;

        // Left Shoulder 2
        if ( (xreport.bLeftTrigger > 200)) { pad_report.pad_btns |= GPAD_LSHLDR_2; };

        //Right Shoulder 2
        if ( (xreport.bRightTrigger > 200)) { pad_report.pad_btns |= GPAD_RSHLDR_2; };


        // ========== Update ==========
        // If there's enough of a difference in the report for us to care about prcessing the new data.
        if ( gmp_diff_report(&pad_report, &gpd_data.prev_report, 2) ) {

            process_gpd_report(dev_addr, instance, &pad_report);

            gpd_data.prev_report = pad_report;    
        };
    
    } // end if (xid_itf->connected && xid_itf->new_pad_data) 

    tuh_xinput_receive_report(dev_addr, instance);
}

// Xinput GamePad connected
void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf) {
    
    // ========== Controller filtering ==========
    // Currently we are supporting only one controller connected at a time.
    if ( gpd_data.gpd_con ) { return; }
    else {

        gpd_data.gpd_con = true;

        gpd_data.gpd_tusb_addr[0] = dev_addr;
        gpd_data.gpd_tusb_addr[1] = instance;
    };


    // ========== Configure the controller ==========
    // If this is a Xbox 360 Wireless controller we need to wait for a connection packet
    // on the in pipe before setting LEDs etc. So just start getting data until a controller is connected.    
    if ( !(xinput_itf->type == XBOX360_WIRELESS && xinput_itf->connected == false) ) {

        // Talk to the end user using the connected controller 
        tuh_xinput_set_rumble(dev_addr, instance, 200, 200, true);          // Rumble our controller
        tuh_xinput_set_led(dev_addr, instance, 0, true);                    // Turn off the ring of LEDS
        tuh_xinput_set_led(dev_addr, instance, 1, true);                    // Turn on LED Quad 1
        busy_wait_ms(500);                                                  // Wait and allow the end user to enjoy the soothing vibrations of a rumbling controllering in their hands
        tuh_xinput_set_rumble(dev_addr, instance, 0, 0, true);              // Thats enough rumbling
    };
    

    // ========== Update the adapters LEDS ==========
    // Set our ALRT LEDS
    if ( !gpio_get(LED_KBD) ) { gpio_put(LED_KBD, 1); };                // Turn on the Keyboard LED if it wasn't already
    if ( !gpio_get(LED_MOUSE) ) { gpio_put(LED_MOUSE, 1); };            // Turn on the Mouse LED is it wasn't already
    

    // ========== TinyUSB Stuff ==========
    // Manually tell TinyUSB that we do actually want data from the connected USB device
    // I guess only weirdos want their connected USB device to do something ¯\_(ツ)_/¯
    bool claim_endpoint = tuh_xinput_receive_report(dev_addr, instance);


    // ========== Debug printouts for Development ==========
    #if DEBUG > 0

    const char* type_str;
    switch (xinput_itf->type)   {
        case 1: type_str = "Xbox One";          break;
        case 2: type_str = "Xbox 360 Wireless"; break;
        case 3: type_str = "Xbox 360 Wired";    break;
        case 4: type_str = "Xbox OG";           break;
        default: type_str = "Unknown";
    }

    printf("XINPUT device type %s, with address %d, instance %d was mounted.\r\n", type_str, dev_addr, instance);
    fflush(stdout);

    // ---------- Print out for bad USB device.
    if ( !claim_endpoint ) {
        printf("Error: cannot request to receive report\r\n");
        fflush(stdout);
    }

    #endif

}

// Xinput Gamepad Disconnected
void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance) {

    // ========== Controller filtering ==========
    // Are we flagged as having a controller connected?
    if ( gpd_data.gpd_con ) {
        
        // Is the disconnected controller the controller that we have flagged as connected?
        if ( gpd_data.gpd_tusb_addr[0] == dev_addr && gpd_data.gpd_tusb_addr[1] == instance ) {
            gpd_data.gpd_con = false;
            gpd_data.gpd_tusb_addr[0] = 0;
            gpd_data.gpd_tusb_addr[1] = 0;

            // if our keyboard LED is turned on and no USB keyboard is connected, turn off the keyboard LED
            if ( gpio_get(LED_KBD) && kbd_data.kbd_count == 0 ) { gpio_put(LED_KBD, 0); };

            // If our mouse LED is turned on and no USB mouse is connected, turn off the mouse LED
            if ( gpio_get(LED_MOUSE) && mouse_data.mouse_count == 0) {  gpio_put(LED_MOUSE, 0); };            

        };
    };

    // ========== Debug printouts for Development ==========
    #if DEBUG > 0

    printf("XINPUT device with address %d, instance %d was unmounted.\r\n", dev_addr, instance);
    fflush(stdout);

    #endif
}

#endif // #if CFG_TUH_XINPUT