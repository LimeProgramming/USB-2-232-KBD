#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "utils.h"
#include "ctypes.h"
#include "serial.h"
#include <default_config.h>

// ==================================================================================================== //
//                                             Readability                                              //
// ==================================================================================================== //
/* Improve the readability of the the fifo code */

void wait_readable_fifo() {
  // Wait for fifo to have something to say
  while ( !multicore_fifo_rvalid() ) { tight_loop_contents(); } 

  return;
}

uint8_t read_fifo() {
  return  multicore_fifo_pop_blocking();
}

void write_fifo(uint8_t core_flag) {

  // Wait to be able to writeable
  while ( !multicore_fifo_wready() ) { tight_loop_contents(); }

  // Write Flag to fifo
  multicore_fifo_push_blocking( core_flag );

  return;
}

void busy_core() {
  // Do Nothing Forever more
  while (true) { tight_loop_contents(); }  
}


// ==================================================================================================== //
//                                             Core 1 Jobs                                              //
// ==================================================================================================== //

void core1_serial_mouse() {

  /* ----- VARIABLES ----- */
  // ==================================================
  static absolute_time_t serial_mouse_target;
  uint flag;
  bool term_save;

  // Set a time for the next serial mouse trigger
  serial_mouse_target = delayed_by_us( get_absolute_time(), mouse_data.persistent.mousetype == TWOBTN ? mouse_data.serialdelay_3B : mouse_data.serialdelay_4B);
  
  /*----------   TIMERS   ----------*/
  startTerminalTimer();               // Start Our Terminal Timer

  while(1) { 

    /* ----- If Core 0 has something to say ----- */
    // ==================================================
    if ( multicore_fifo_rvalid() ) {

      switch ( multicore_fifo_pop_blocking() ) {
    
      case cf_stop: // Core 0 is telling Core 1 to stop processing

        // Must stop timers to prevent a panic
        stopPWRBlinkerTimer();          // Stop Blinking the PWR LED
        stopTerminalTimer();            // Stop Terminal Timer

        write_fifo(cf_resume);          // Tell Core 0 that Core 1 is done
        busy_core();                    // Do Nothing Forever more
      break;

      case cf_post:     // If there is a mouse update
        postSerialMouse();
      break;

      case cf_nopost:   // If there isn't a mouse update
      break;

      } // End Switch

    } // End fifo If


    /*---------- Post Serial mouse packet ----------*/
    // ==================================================
    if ( time_reached(serial_mouse_target) )
    {  
      /* ----- Set a time for the next serial mouse trigger ----- */
      // ==================================================
      serial_mouse_target = delayed_by_us( get_absolute_time(), mouse_data.persistent.mousetype == TWOBTN ? mouse_data.serialdelay_3B : mouse_data.serialdelay_4B );

      // IF there is no USB mouse connected OR adpter is not to act as a serial mouse
      // ==================================================
      if ( (mouse_data.mouse_count == 0 && DEBUG == 0) || (mouse_data.serial_state != 0) ) { continue; };

      /* ----- Talk to Core 0 ----- */
      // ==================================================
      // Core 0 generates the next mouse packet because there was a noticable lag if Core1 generated it

      // Tell core0 to generate updated mouse packet
      write_fifo(cf_update); 

    } // Timer reached switch


    /*----------   Serial Terminal   ----------*/
    // ==================================================
    // Serial State is used to say if the pico should be a serial terminal or a mouse. 
    if ( mouse_data.serial_state == 1  || mouse_data.serial_state == 2 )
    { 
      stopTerminalTimer();            // Stop Terminal Timer
      startPWRBlinkerTimer();         // Blink the PWR led 

      // If the UART we got data from is the one we're using for the mouse
      if ( ( mouse_data.serial_state == 1 && UART_ID == uart0 ) || ( mouse_data.serial_state == 2 && UART_ID == uart1 ) ) {
        term_save = serial_terminal(UART_ID,  (10000 / (int) ( mouse_data.realbaudrate / 1200 )) );  

      } else {  // Else the user connected a TTL to the unused uart
        term_save = serial_terminal(mouse_data.serial_state == 1 ? uart0 : uart1, 1500);  // 10000 / (115200 / 1200 ) ~= 105 
      }

      stopPWRBlinkerTimer();          // Stop Blinking the PWR LED
      gpio_put(LED_PWR, 1);           // Turn on PWR LED

      switch ( term_save ) {
        case true:   

          // Tell Core 0 to save settings 
          write_fifo(cf_save_settings);

          // Do nothing forever more
          busy_core();
        break;

        case false:

          // Set serial State to mouse mode
          mouse_data.serial_state = 0; 

          // Resume Terminal Timer
          startTerminalTimer();           
        break;
      }

    }


    /*----------   Main Mouse Part of the mouse  ----------*/
    // ==================================================
    bool cts_pin = gpio_get(UART_CTS_PIN);
    
    // CTS pin pulled low by PC ( reversed by the max(3)232 ), meaning it's trying to talk to us.
    if(cts_pin) { 

      if ( mouse_data.pc_state != CTS_TOGGLED ) { mouse_data.pc_state = CTS_LOW_INIT; } 
      
      /* ----- LOGITECH DRIVERS COMBATIBILITY -----
      Sometimes the PC sporadically pulls the CTS pin low when loads games with Logitech drivers.
      So to avoid spamming the serial port, we'll wait a bit to see whats going on; if the serial mouse was already connected.
      */
      else {
        if (mouse_data.persistent.mousetype == THREEBTN ) {
          busy_wait_us_32(mouse_data.serialdelay_3B);
        }

        cts_pin = gpio_get(UART_CTS_PIN);
        
        if ( cts_pin ) { mouse_data.pc_state = CTS_LOW_INIT; }
      }
    }

    /* ----- Mouse initializing request detected ----- */
    // ==================================================
    if( !cts_pin && mouse_data.pc_state == CTS_LOW_INIT ) {
      serialMouseNego();
      mouse_data.pc_state = CTS_TOGGLED;
    }


    /* ----- CTMOUSE DRIVERS COMBATIBILITY ----- */
    // ==================================================
    if ( !cts_pin && mouse_data.pc_state == CTS_UNINIT) {
      /*
      Unlike the Logitech drivers, the CTMouse driver doesn't seem to pull CTS low when it looses connection with the mouse. 
      If the USB mouse is replaced or lost connection for a moment:
          - CTMouse driver the loop should fall here to resume mouse functionality
          - Logitech driver should fall in the top if to resume mouse functionality. 
      */

      mouse_data.pc_state = CTS_LOW_INIT;
    }

    // we fall in here for the thing (yank out serial cable )
    if( (mouse_data.pc_state != CTS_TOGGLED && mouse_data.mouse_count > 0) ) {   
      #if DEBUG > 0
      //printf("Serial Connection not found");
      #endif

      // Call ALRT Code flasher for Serial Connection Lost. 
      // TODO
    }  
  }
}


// ==================================================================================================== //
//                                          Core 1 Management                                           //
// ==================================================================================================== //

void start_core1(uint process) {

  // Which process do we start on core 1
  switch (process) {
    case 0:
      multicore_launch_core1(core1_serial_mouse);
    break;
  }
  

  #if DEBUG > 0
  printf("Core 1 Called\n");
  #endif
}

void stop_core1(){
  // Stop Core 1, this func is called from core 0 so we will need to fifo data to talk to core 1
  // Sometimes just killing core1 messed up the uart. So telling Core 1 to stop prevents it from using the uart until core 1 is restarted

  // ===== Tell and wait for Core 1 to stop processing ===== 
  // Wait for fifo to be writable
  while ( !multicore_fifo_wready() ) { tight_loop_contents(); }

  // Tell Core 1 to stop processing
  multicore_fifo_push_blocking( cf_stop );

  // Wait for fifo to have a reply
  while ( !multicore_fifo_rvalid() ) { tight_loop_contents(); } 


  // ===== Reset Core 1 =====
  multicore_reset_core1();


  // ===== Debug printout =====
  #if DEBUG > 0
  printf("Core 1 Stopped\n");
  #endif
}

void force_stop_core1() {
  // Stops core 1 without grace.

  // ===== Reset Core 1 =====
  multicore_reset_core1();

  // ===== Debug printout =====
  #if DEBUG > 0
  printf("Core 1 Force Stopped\n");
  #endif
}

