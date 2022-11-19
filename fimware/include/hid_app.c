#include "tusb.h"
#include <stdio.h>
#include <stdlib.h>
#include "bsp/board.h"
#include "pico/stdlib.h"

#include "utils.h"
#include "ctypes.h"
#include "hid_app.h"
#include "hid_con.h"
#include <default_config.h>

//--------------------------------------------------------------------+
//          inyUSB Stuff
//--------------------------------------------------------------------+

CFG_TUSB_MEM_SECTION static char serial_in_buffer[64] = { 0 };


//--------------------------------------------------------------------+
//          TinyUSB Callbacks
//--------------------------------------------------------------------+

// ==================================================
// ---------- This is executed when a new device is mounted
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{   
    // Manually tell TinyUSB that we do actually want data from the connected USB device
    // I guess only weirdos want their conneced USB device to do something ¯\_(ツ)_/¯
    //bool claim_endpoint = 0;

    static uint8_t value = 0;

    switch ( tuh_hid_interface_protocol(dev_addr, instance) ) 
    {
        case HID_ITF_PROTOCOL_MOUSE:

            gpio_put(LED_MOUSE, 1);     // Turn on Alert LED
            ++mouse_data.mouse_count;   // Increment our mouse counter
            
        break;

        case HID_ITF_PROTOCOL_KEYBOARD: 

            if ( kbd_data.kbd_count >= 4 ) { return; };                          // If 4 keyboards already connected, ignore the new keyboard

            kbd_data.kbd_tusb_addr[kbd_data.kbd_count][0] = dev_addr;           // Note the new keyboard's device address
            kbd_data.kbd_tusb_addr[kbd_data.kbd_count][1] = instance;           // Note the new keyboard's instance count

            // If this is our first keyboard
            if ( kbd_data.kbd_count == 0) { 
                gpio_put(LED_KBD, 1);                                           // Turn on Alert LED
            }

            kbd_data.kbd_count++;                                               // Increment Keyboard counter

            // If a host computer is connected, set LED state to the host values
            // If a host is not connected, stobe the leds to look all cool and hip with the kids daddy-oh
            if ( !kbd_data.din_present ) {  
                add_alarm_in_ms(500, idle_kbd_locks, (void*)(13), true);    
            };
                
        break;

        // Process HID Report and hope it's a mouse
        case HID_ITF_PROTOCOL_NONE:    
            //tuh_hid_set_report(dev_addr, instance, 0, HID_REPORT_TYPE_OUTPUT, (void*)value, 1);
            // By default host stack will use activate boot protocol on supported interface.
            hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_HID_REPORT, desc_report, desc_len);
            printf("HID has %u reports \r\n", hid_info[instance].report_count);
            fflush(stdout);

        break;
        
        // Process Generic Report
        default:    break;
    }
    
    // Manually tell TinyUSB that we do actually want data from the connected USB device
    // I guess only weirdos want their conneced USB device to do something ¯\_(ツ)_/¯
    //const bool claim_endpoint = tuh_hid_receive_report(dev_addr, instance);

    tuh_hid_receive_report(dev_addr, instance);


    #if DEBUG > 0

    // ---------- Print out the type of device connected
    const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
    const uint8_t itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    printf("HID device with address %d, instance %d, protocol %d, is a %s, has mounted.\r\n", dev_addr, instance, itf_protocol, protocol_str[itf_protocol]); 
    fflush(stdout);

    // ---------- Print out for bad USB device.
    //if ( !claim_endpoint ) { printf("Error: cannot request to receive report\r\n"); }

    #endif

    return;
}

// ==================================================
// ---------- This is executed when a device is unmounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{   
    switch ( tuh_hid_interface_protocol(dev_addr, instance) )
    {
        case HID_ITF_PROTOCOL_MOUSE:

            --mouse_data.mouse_count;       // Deincrement the mouse counter

            if ( mouse_data.mouse_count <= 0 )
            {   
                mouse_data.mouse_count = 0; // Make sure mouse count is actually zero
                gpio_put(LED_MOUSE, 0);     // Turn off mouse connected LED
            }

        break;

        case HID_ITF_PROTOCOL_KEYBOARD: 
            
            // ========== Is our keyboard ==========
            // Check is keybaord is one we care about
            if ( !is_kb_connected(dev_addr, instance) ) { break; }

            // ========== Typematic ==========
            // Break typematic when a usb keyboard is disconnected because the typematic key could be from the disconnected keyboard/
            kbd_data.cmd_set.tm_key = 0x00;        // USB hid key 0x00 is all zeros in lookup tables
            
            // ========== Rebuid stored data arrays ==========
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
                } 
            }
            
            memcpy(kbd_data.kbd_tusb_addr, newkbd_tusb_addr, 4);
            memcpy(kbd_data.kbd_tusb_prev_report, newkbd_tusb_prev_report, 4);

            // ========== Keyboard Counts ==========
            // Update out keyboard counter for the sake of the ALRT LEDS
            kbd_data.kbd_count = j;

            // Turn off Keyboard led if no USB keyboard is flagged as connected
            if ( j == 0 ) {
                gpio_put(LED_KBD, 0);     // Turn on Alert LED
            }
        
        break;
    }

    #if DEBUG
    printf("HID device with address %d, instance %d was unmounted.\r\n", dev_addr, instance);
    fflush(stdout);
    #endif
}

// ==================================================
// ---------- This is executed when data is received from the mouse
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{  
    switch (tuh_hid_interface_protocol(dev_addr, instance)) 
    {   
         // Process Mouse Report
        case HID_ITF_PROTOCOL_MOUSE:   
            if ( mouse_data.serial_state == 0 ) {
                process_mouse_report((hid_mouse_report_t const*) report );
            }
            break;

        // Process Keyboard Reports
        case HID_ITF_PROTOCOL_KEYBOARD: 

            // Check is keybaord is valid
            if ( !is_kb_connected(dev_addr, instance) ) { break; }

            process_kbd_report( dev_addr, instance, (hid_keyboard_report_t const*) report );
            break;

        // Process Generic Report
        case HID_ITF_PROTOCOL_NONE: 
        default:  
            if ( is_sony_ds4(dev_addr) ) {
                process_sony_ds4(report, len);
            } else if ( is_sony_psc(dev_addr) ){
                process_sony_psc(report, len);
            } else {
                process_generic_report(dev_addr, instance, report, len);
            }

            break;
    }

    // Manually tell TinyUSB that we do actually want data from the connected USB device
    // I guess only weirdos want their conneced USB device to do something ¯\_(ツ)_/¯
    bool claim_endpoint = tuh_hid_receive_report(dev_addr, instance);

    #if DEBUG > 0

    // ---------- Print out for bad USB device.
    if ( !claim_endpoint ) {
        printf("Error: cannot request to receive report\r\n");
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

