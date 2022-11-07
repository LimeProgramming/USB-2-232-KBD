#ifndef HID_APP_H_
#define HID_APP_H_

#include "bsp/board.h"
#include "tusb.h"

#define MAX_HID_REPORT  4

static struct
{
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_HID_REPORT];
}hid_info[CFG_TUH_HID];

//void tuh_cdc_xfer_isr(uint8_t dev_addr, xfer_result_t event, cdc_pipeid_t pipe_id, uint32_t xferred_bytes);


/*---------------------------------------*/
//        Keyboard handling funcs        //
/*---------------------------------------*/

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len);

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance);

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

void process_sony_ds4(uint8_t const* report, uint16_t len);
void process_sony_psc(uint8_t const* report, uint16_t len);


#endif // HID_APP_H_