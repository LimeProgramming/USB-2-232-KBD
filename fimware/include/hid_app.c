#include "tusb.h"
#include "bsp/board.h"
#include "pico/stdlib.h"

#include "utils.h"
#include "hid_app.h"
#include <default_config.h>

// Controllers are not supported if keyboard is not enabled
#if KB_ENABLE
    #include "hid_con.h"
#endif

//--------------------------------------------------------------------+
//          inyUSB Stuff
//--------------------------------------------------------------------+

CFG_TUSB_MEM_SECTION static char serial_in_buffer[64] = { 0 };


//--------------------------------------------------------------------+
//          TinyUSB Callbacks
//--------------------------------------------------------------------+

// ==================================================
// ---------- This is executed when a new device is mounted
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {   

    switch ( tuh_hid_interface_protocol(dev_addr, instance) ) {   

    // ========== Handle Mouse Mount ==========
    case HID_ITF_PROTOCOL_MOUSE:
        
        // If this is our first mouse
        if ( mouse_data.mouse_count == 0 ) {    gpio_put(LED_MOUSE, 1); };  // Turn on Alert LED

        ++mouse_data.mouse_count;                                           // Increment our mouse counter

        // ========== We want to talk to this USB device ==========
        // Manually tell TinyUSB that we do actually want data from the connected USB device
        // I guess only weirdos want their connected USB device to do something ¯\_(ツ)_/¯
        tuh_hid_receive_report(dev_addr, instance);
        
    break;

    // ========== Handle Keyboard Mount ==========
    case HID_ITF_PROTOCOL_KEYBOARD: 

        #if KB_ENABLE

        // ========== Keyboard Filtering ==========
        if ( kbd_data.kbd_count >= 4 ) { return; };                         // If 4 keyboards already connected, ignore the new keyboard

        kbd_data.kbd_tusb_addr[kbd_data.kbd_count][0] = dev_addr;           // Note the new keyboard's device address
        kbd_data.kbd_tusb_addr[kbd_data.kbd_count][1] = instance;           // Note the new keyboard's instance count

        // If this is our first keyboard
        if ( kbd_data.kbd_count == 0) { gpio_put(LED_KBD, 1); };            // Turn on Alert LED

        kbd_data.kbd_count++;                                               // Increment Keyboard counter


        // ========== Idle Locks ==========
        // If a host computer is connected, set LED state to the host values
        // If a host is not connected, strobe the leds to look all cool and hip with the kids daddy-oh
        if ( !kbd_data.din_present && !kbd_data.idle_lock_timer_id ) {
            kbd_data.idle_lock_timer_id = alarm_pool_add_alarm_at(alarm_pool_get_default(), delayed_by_ms(get_absolute_time(), 500), idle_kbd_locks, (void*)(13), true);  
        };


        // ========== We want to talk to this USB device ==========
        // Manually tell TinyUSB that we do actually want data from the connected USB device
        // I guess only weirdos want their connected USB device to do something ¯\_(ツ)_/¯
        tuh_hid_receive_report(dev_addr, instance);

        #endif
            
    break;

    // ========== Handle a None/Controller Mount ==========
    case HID_ITF_PROTOCOL_NONE:

        // If the device is falling here, it could be a weird mouse/keyboard that gives a generic report but there isn't much we can do (usually) with them since the pico is in boot protocol mode.
        // We can also fall here for controllers. Supported controllers are whitelisted because different controllers give different report.

        #if KB_ENABLE

        // Check if device is a whitelisted controller
        if ( is_whitelisted_con(dev_addr) ) {
        
            // ========== Controller filtering ==========
            // Currently we are supporting only one controller connected at a time.
            if ( gpd_data.gpd_con ) { return; };

            gpd_data.gpd_con = true;

            gpd_data.gpd_tusb_addr[0] = dev_addr;
            gpd_data.gpd_tusb_addr[1] = instance;
            

            // ========== Update the adapters LEDS ==========
            // Set our ALRT LEDS
            if ( !gpio_get(LED_KBD) ) { gpio_put(LED_KBD, 1); };                // Turn on the Keyboard LED if it wasn't already
            if ( !gpio_get(LED_MOUSE) ) { gpio_put(LED_MOUSE, 1); };            // Turn on the Mouse LED is it wasn't already


            // ========== We want to talk to this USB device ==========
            // Manually tell TinyUSB that we do actually want data from the connected USB device
            // I guess only weirdos want their connected USB device to do something ¯\_(ツ)_/¯
            tuh_hid_receive_report(dev_addr, instance);
        } 

        #endif

    break;
    
    // Process Generic Report
    default:    break;
    }
    
    // ===== Dev DEBUG Printout =====
    #if DEBUG > 0

    const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
    const uint8_t itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    printf("HID device with address %d, instance %d, protocol %d, is a %s, has mounted.\r\n", dev_addr, instance, itf_protocol, protocol_str[itf_protocol]); 
    fflush(stdout);

    #endif

    return;
}

// ==================================================
// ---------- This is executed when a device is unmounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {  

    switch ( tuh_hid_interface_protocol(dev_addr, instance) ) {
    
    // ==========  Handle Mouse Unmount ==========
    case HID_ITF_PROTOCOL_MOUSE:

        --mouse_data.mouse_count;       // Decrement the mouse counter

        if ( mouse_data.mouse_count <= 0 )
        {   
            mouse_data.mouse_count = 0; // Make sure mouse count is actually zero
            gpio_put(LED_MOUSE, 0);     // Turn off mouse connected LED
        }

    break;
    
    // ========== Handle Keyboard Unmount ==========
    case HID_ITF_PROTOCOL_KEYBOARD: 
        
        #if KB_ENABLE

        // ========== Is our keyboard ==========
        // Check is keybaord is one we care about
        if ( !is_kb_connected(dev_addr, instance) ) { break; }

        // ========== Typematic ==========
        // Break typematic when a usb keyboard is disconnected because the typematic key could be from the disconnected keyboard/
        kbd_data.cmd_set.tm_key = 0x00;        // USB hid key 0x00 is all zeros in lookup tables
        
        // ========== Rebuild stored data arrays ==========
        // We need to rebuild the arrays of stored data to account for the missing keyboard. 
        uint8_t newkbd_tusb_addr[4][2]; uint8_t j = 0;
        hid_keyboard_report_t newkbd_tusb_prev_report[4];

        for ( uint8_t i = 0 ; i < kbd_data.kbd_count ; i++ ) {
            
            // Make sure all our new array init with all zeros
            newkbd_tusb_prev_report[i] = (hid_keyboard_report_t) { 0, 0, {0} };
            newkbd_tusb_addr[i][0] = 0;
            newkbd_tusb_addr[i][1] = 0;

            // Start rebuilding out arrays, removing which keyboard has been removed
            if ( kbd_data.kbd_tusb_addr[i][0] != dev_addr || newkbd_tusb_addr[i][1] != instance ) {
                newkbd_tusb_addr[j][0] = kbd_data.kbd_tusb_addr[i][0];          // keyboard address
                newkbd_tusb_addr[j][1] = kbd_data.kbd_tusb_addr[i][1];          // keyboard instance
                newkbd_tusb_prev_report[j] = kbd_data.kbd_tusb_prev_report[i];  // keyboard reports
                j++;
            } else if ( kbd_data.kbd_tusb_addr[i][0] == dev_addr && newkbd_tusb_addr[i][1] == instance ) {
                // Depress any keys pressed by the disconnected keyboard
                delete_kbd_report(kbd_data.kbd_tusb_prev_report[i]);
            };
        }
        
        memcpy(kbd_data.kbd_tusb_addr, newkbd_tusb_addr, 4);
        memcpy(kbd_data.kbd_tusb_prev_report, newkbd_tusb_prev_report, 4);

        // ========== Keyboard Counts ==========
        // Update out keyboard counter for the sake of the ALRT LEDS
        kbd_data.kbd_count = j;

        // Turn off KBD LED if no USB Keyboard or Gamepad is connected
        if ( kbd_data.kbd_count == 0 && gpd_data.gpd_con == false ) { gpio_put(LED_KBD, 0); };

        #endif
    
    break;

    // ========== Handle Controller Unmount ==========
    case HID_ITF_PROTOCOL_NONE: 

        #if KB_ENABLE

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

        #endif

    break;
    }

    // ===== Dev DEBUG Printout =====
    #if DEBUG > 0

    printf("HID device with address %d, instance %d was unmounted.\r\n", dev_addr, instance);
    fflush(stdout);

    #endif
}

// ==================================================
// ---------- This is executed when data is received from the mouse
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {

    switch ( tuh_hid_interface_protocol(dev_addr, instance) ) {   

    // ==========  Handle Mouse Reports ==========
    case HID_ITF_PROTOCOL_MOUSE:   
        // If the serial terminal is not open
        if ( mouse_data.serial_state > 0 ) {  break; };

        process_mouse_report( (hid_mouse_report_t const*) report );

    break;

    // ==========  Handle Keyboard Reports ==========
    case HID_ITF_PROTOCOL_KEYBOARD: 

        #if KB_ENABLE

        // Check is keyboard is valid
        if ( !is_kb_connected(dev_addr, instance) ) { break; };

        process_kbd_report( dev_addr, instance, (hid_keyboard_report_t const*) report );

        #endif

    break;

    // ==========  Handle None/Controller Reports ==========
    case HID_ITF_PROTOCOL_NONE: default:  

        #if KB_ENABLE 
        
        if ( is_sony_ds4(dev_addr) ) {
            process_sony_ds4(report, len);
        } else if ( is_sony_psc(dev_addr) ){
            process_sony_psc(report, len);
        } else {
            process_generic_report(dev_addr, instance, report, len);
        }

        #else 

            process_generic_report(dev_addr, instance, report, len);

        #endif

    break;
    }

    // Manually tell TinyUSB that we do actually want data from the connected USB device
    // I guess only weirdos want their connected USB device to do something ¯\_(ツ)_/¯
    bool claim_endpoint = tuh_hid_receive_report(dev_addr, instance);


    // ===== Dev DEBUG Printout =====
    #if DEBUG > 0

    if ( !claim_endpoint ) {    // ----- Print out for bad USB device.
        printf("Error: cannot request to receive report\r\n");
        fflush(stdout);
    }

    #endif

}


//--------------------------------------------------------------------+
//          HID Processors 
//--------------------------------------------------------------------+

// ==================================================
// ---------- Handle generic USB Report
void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) 
{   
    (void) dev_addr;

    uint8_t const rpt_count = hid_info[instance].report_count;
    tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
    tuh_hid_report_info_t* rpt_info = NULL;

    if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0) {    // Simple report without report ID as 1st byte
        rpt_info = &rpt_info_arr[0];        
    }
    
    else {
        uint8_t const rpt_id = report[0];                       // Composite report, 1st byte is report ID, data starts from 2nd byte

        for ( uint8_t i=0; i<rpt_count; i++ ) {
            if ( rpt_id == rpt_info_arr[i].report_id ) {
                rpt_info = &rpt_info_arr[i];
                break;
            }}
            
        report++;   
        len--;
    }

    // If the Hid usage isn't something we care about
    if ( rpt_info->usage_page != HID_USAGE_PAGE_DESKTOP ) { return; }

    
    // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. && Assume mouse follow boot report layout
    switch ( rpt_info->usage ) 
    {
        case HID_USAGE_DESKTOP_MOUSE:

            // If a mouse is giving reports but was somehow never flagged as connected
            if ( mouse_data.mouse_count < 1 ){
                ++mouse_data.mouse_count;   // Increment the mouse counter
                gpio_put(LED_MOUSE, 1);     // Turn on Mouse LED    
            }

            // Process the hopefully mouse report
            process_mouse_report((hid_mouse_report_t const*) report); 
        break;

        case HID_USAGE_DESKTOP_KEYBOARD:
        break;
    }

}






//--------------------------------------------------------------------+
// Keyboard Testing
//--------------------------------------------------------------------+

