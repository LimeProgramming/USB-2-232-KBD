#include "tusb.h"
#include <stdio.h>
#include "bsp/board.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "pico/multicore.h"

#include "include/utils.h"
#include "include/ps2dev.h"
#include "include/core_1.h"
#include "include/ctypes.h"
#include "include/serial.h"
#include "include/version.h"
#include "include/hid_app.h"
#include "include/hid_con.h"
#include "default_config.h"

/* ---------------------------------------------------------- */
// Mouse Tracking Variables

MOUSE_DATA mouse_data;

KEYBOARD_DATA kbd_data;

GAMEPAD_DATA gpd_data;

// Aggregate movements before sending
CFG_TUSB_MEM_SECTION static hid_mouse_report_t usb_mouse_report_prev;

static absolute_time_t usb_polling_target;

//uint8_t val = 0;
//static absolute_time_t din_polling_target;
//static absolute_time_t ps2_kbd_conn_target;

//unsigned char leds;

/*---------------------------------------*/
//                  Main                 //
/*---------------------------------------*/
int main(){

    // Mild under clock
    set_sys_clock_khz(120000, true);

    stdio_init_all();           // pico SDK
    board_init();               // init board from TinyUSB
    tusb_init();                // init TinyUSB

    /*---------------------------------------*/
    //                 LEDS                  //
    /*---------------------------------------*/

    init_led(LED_PWR);          // Init Power LED
    gpio_put(LED_PWR, 1);       // Turn on Power LED
    init_led(LED_MOUSE);        // Init Mouse LED
    init_led(LED_KBD);          // Init KBD LED

    /*---------------------------------------*/
    //            Reset the flash            //
    /*---------------------------------------*/

    // Init the reset pin
    init_pinheader(RESET_FLASH);  
    sleep_us(500);         // wait a bit to avoid false positive

    // If the reset pin is closed on startup
    if ( !gpio_get(RESET_FLASH) ) {   

      board_led_write(1);     // Turn on Built in LED for feedback
      sleep_ms(3000);         // Wait for 3 seconds.
      
      // IF reset pin is still held
      if ( !gpio_get(RESET_FLASH) ) {   
          
        loadPersistentDefaults();       // Load the default Settings
        savePersistentSet();            // Save the default settings

        // Blink built-in LED forever waiting for user to reboot device
        while(1) {   
          board_led_write(0);   
          sleep_ms(800);
          board_led_write(1); 
          sleep_ms(800);
        }

      } else {
        board_led_write(0);     // Turn off Built in LED
      }
    }
    
    /*---------------------------------------*/
    //              Mouse Options            //
    /*---------------------------------------*/
    
    // Setup Dip Switch Pins
    uint8_t dipswpins[6] = { DIPSW_THREEBTN, DIPSW_WHEEL, DIPSW_75XYSPEED, DIPSW_50XYSPEED, DIPSW_7N2, DIPSW_19200 };
    for ( uint8_t i = 0; i < 6; i++ ) { 
        
      init_pinheader(dipswpins[i]);           // init Dip Switch Pins
      
      sleep_us(20000);                        // Wait a bit to avoid false positive

      // Set up IRQ callbacks on Dip Switch Pins
      gpio_set_irq_enabled_with_callback(dipswpins[i], GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &dipswGPIOCallback);
      }

    initPersistentSet();        // try to load persistent settings.
  

    /*---------------------------------------*/
    //                KBD PINS               //
    /*---------------------------------------*/
    
    // No point in configuring GPIO for the keyboard if we're not using it. 
    #if KB_ENABLE
      // keyboard data setup
      kbd_data.kbd_count = 0;

      for ( uint8_t i ; i < KB_MAX_KEYBOARDS ; i++ ) {
        kbd_data.kbd_tusb_addr[i][0] = 0;
        kbd_data.kbd_tusb_addr[i][1] = 0;
      }
      

      // Clock In
      gpio_init(PS2_CLOCK_IN); 
      gpio_set_dir(PS2_CLOCK_IN, GPIO_IN);
      gpio_pull_down(PS2_CLOCK_IN);

      // Data In 
      gpio_init(PS2_DATA_IN); 
      gpio_set_dir(PS2_DATA_IN, GPIO_IN);
      gpio_pull_down(PS2_DATA_IN);
      
      // Clock Out
      init_led(PS2_CLOCK_OUT);

      // Data Out
      init_led(PS2_DATA_OUT);

      // Blank out the TinyUSB keyboard reports
      for (uint8_t i = 0 ; i < KB_MAX_KEYBOARDS ; i++ ) {
        kbd_data.kbd_tusb_prev_report[i] = (hid_keyboard_report_t) { 0, 0, {0} };
      };

      kbd_data.cmd_set.led_state = AT_KB_LED_UNCHANGED;
    
      // Check Keyboard clock and data lines looking for a possible connection
      // This can be duped by connecteding KB PWR header on the KBD PCB, This is mostly for XT machines. 
      // This will be true when the device is powered via the din port
      if ( gpio_get(PS2_DATA_IN) && gpio_get(PS2_CLOCK_IN) ) { 
        
        kbd_data.din_present = true;            // Flag computer as connected
        kbd_data.din_initalised = false;        // Din keyboard is no longer active
        kbd_data.din_conn_fail = 0;             // Reset connection failure counter

        load_cmd_set_settings();                // Load the CMD set settings

        // If we're trying to be an XT keyboard and an IBM XT, Pull data low as soon as possible.
        // IBM XT start with data line low
        if ( kbd_data.persistent.kbd_type == 0 && kbd_data.persistent.kbd_xtclone == 0 ){            
          gpio_put( PS2_DATA_OUT, 1);
        }

      // Else set up an IRQ to flag Din as present when pc is powered on. 
      } else {
        gpio_set_irq_enabled_with_callback(PS2_CLOCK_IN, GPIO_IRQ_EDGE_RISE , true, &dinPresentingCallback);
      }

    #endif

    /*---------------------------------------*/
    //              UART STUFF               //
    /*---------------------------------------*/

    /* ----- UART SETUP ----- */
    // ==================================================

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART); // Set the TX pins
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART); // Set the RX pins

    gpio_init(UART_CTS_PIN);                        // CTS Pin
    gpio_set_dir(UART_CTS_PIN, GPIO_IN);            // CTS Pin direction

    init_serial_uart(7);         // Init our uart
    calcSerialDelay();

    start_core1(0);

    /*---------------------------------------*/
    //               Main Loop               //
    /*---------------------------------------*/
    while(1)
    {  
        /*----------   Blink Built in LED to show the main loop is running   ----------*/
        // Blink the built in LED when in DEBUG Mode
        #if DEBUG > 0
        blink_led_task();
        tuh_task(); // TinyUSB usb host task
        #endif
    
        /*---------- Core 1 talking to Core 0 ----------*/
        if ( multicore_fifo_rvalid() )
        {
          switch ( multicore_fifo_pop_blocking() )
          {
            case cf_update:
            
              // Update the mouse packet from Core0 so Core1 can pull the correct data
              update_mousepacket();

              // Wait for fifo to be writable
              while ( !multicore_fifo_wready() ) { tight_loop_contents(); } 

              // Reply to Core1
              multicore_fifo_push_blocking( mouse_data.mpkt.update ? cf_post : cf_nopost );

            break;

            case cf_save_settings:

              // Stop Core 0
              force_stop_core1();

              // Save Persistent       
              savePersistentSet();

              // If the UART we got data from is the one we're using for the mouse
              if ( ( mouse_data.serial_state == 1 && UART_ID == uart0 ) || ( mouse_data.serial_state == 2 && UART_ID == uart1 ) ) {
                post_settings_saved(UART_ID,  (10000 / (int) ( mouse_data.realbaudrate / 1200 )) );  

              } else {  // Else the user connected a TTL to the unused uart
                post_settings_saved(mouse_data.serial_state == 1 ? uart0 : uart1, 1500);  // 10000 / (115200 / 1200 ) ~= 105 
              }

              // Set serial State to mouse mode        
              mouse_data.serial_state = 0;  

              // Resume Core 1  
              start_core1(0);

            break;

          }
        }


        // USB Mouse Polling
        // -----------------------------
        // TinyUSB support USB polling rates up to 1000hz
        // However
        // AVG movement @1000hz feels very very wrong. 
        // Adding these delays for the AVG movement is acting as a sort of handicap on the USB polling rate 

        switch ( mouse_data.persistent.mouse_movt_type ) {
          case MO_MVT_ADDITIVE: case MO_MVT_COAST:
            tuh_task(); // TinyUSB usb host task
          break;
  
          case MO_MVT_AVERAGE:  
            if ( time_reached(usb_polling_target) ) {
              switch ( mouse_data.persistent.baudrate ) {
                case 19200:
                  usb_polling_target = delayed_by_us( get_absolute_time(), 1000);
                break;
                
                case 9600:
                  usb_polling_target = delayed_by_us( get_absolute_time(), 2000);
                break;

                case 4800:  case 2400:  case 1200:  default:
                  usb_polling_target = delayed_by_us( get_absolute_time(), 4000);
                break;
              }

              tuh_task(); // TinyUSB usb host task
            }
          break;
        }


        // DIN Keyboard Polling
        // -----------------------------

        // ----- Is KB_ENABLE flagged as true or false
        // If keyboard is not enabled, skip over keyboard handle functions
        #if KB_ENABLE


        // ----- Keyboard Leds -----
        // I've attempted this several different ways, some smarter and some dumber. However I kept having problems with TinyUSB being terrible
        // This seems to be a more stable way of setting the led state at least for just the one keyboard. 
        // Changing the LED state of multiple keyboards require strange timing and some luck
        if ( kbd_data.cmd_set.led_state != AT_KB_LED_UNCHANGED ) {
          
          update_kbd_locks();                               // Update the LEDs on the fist most connected keyboard
          tuh_task();                                       // TinyUSB host task, calling this immediately seems to help reduce errors 
          kbd_data.cmd_set.led_state = AT_KB_LED_UNCHANGED; // Re-set LED state value
        };


        // ----- Typematic -----
        // Once we hit the tm_timestamp threshold we will repeatedly send the typematic key
        // at a rate set by either PS2 command set commands or default values. 
        if ( kbd_data.cmd_set.tm_key != 0x00 && time_reached(kbd_data.cmd_set.tm_timestamp) ) {
          keyboard_make(kbd_data.cmd_set.tm_key);
          kbd_data.cmd_set.tm_timestamp = delayed_by_ms(get_absolute_time(), kbd_data.cmd_set.tm_rate);
        };

        // ----- Is something flagged as connected to the DIN port
        // If we think there isn't a computer connected to our din port, then continue the main loop
        if ( !kbd_data.din_present ) {
          continue;
          } 


        // ----- Are we initialized to the host pc as a keyboard?
        // If DIN Keyboard is not flagged as connected
        if ( !kbd_data.din_initalised ) { 
          
          // If it's time to try connecting and the din port is idle/ready
          if ( time_reached(kbd_data.din_polling_target) && din_idle() ) {

              kbd_data.din_initalised = din_init();

              // AT keyboard controllers can handle 100ms between connection attempts but ps2 controllers can take only 10ms
              kbd_data.din_polling_target = delayed_by_us( get_absolute_time(), (kbd_data.din_initalised) ? 10000 : 5000 );
            }


        // If PS2 keyboard is flagged as connected
        } else {
          // Poke the PC every 10 ms for any updates.
          if ( time_reached(kbd_data.din_polling_target) ) {

            uint8_t kbdret = pool_din_kbd();

            switch (kbdret) {
            // Timeout
            case DIN_RET_TOUT:
              break;

            // Disconnection error
            case DIN_RET_DISCONN:
              if ( kbd_data.din_conn_fail++ >= 10 ) {
                
                // Reset the keyboard variables
                reset_kbd_defaults();

                // Set up a timer for idle locks if it somehow isn't running 
                if ( kbd_data.idle_lock_timer_id == 0 ) {
                  kbd_data.idle_lock_timer_id = alarm_pool_add_alarm_at(alarm_pool_get_default(), delayed_by_ms(get_absolute_time(), 800), idle_kbd_locks, (void*)(13), true); 
                }

                // Enable IRQ listening for Din present
                gpio_set_irq_enabled_with_callback(PS2_CLOCK_IN, GPIO_IRQ_EDGE_RISE , true, &dinPresentingCallback);
              }

              break;
            
            // Successful connection
            case DIN_RET_SUCC:
              kbd_data.din_conn_fail = 0;
              break;

            default:
              break;
            }

            kbd_data.din_polling_target = delayed_by_us( get_absolute_time(), 10000);
           }
        }
      }


    #endif

}
