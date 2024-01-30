#include <stdio.h>
#include <string.h>
#include "hex_uart.h"

static uint8_t ack_data[] = {
  0x6C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x02, 0x23, 0x11, 0x27, 0x17, 0x00
};

// User TODO
int __attribute__((weak)) user_do_inquire(DATAPACK* rcv_pack, DATAPACK* DATAPACK) {
  return 0;
}

// User TODO
void __attribute__((weak)) user_do_setSomething(DATAPACK* rcv_pack, DATAPACK* ack_pack) {
  int ack_dataLen = 0;
  uint8_t firstByte = 0x6C;

  if (!strncmp(rcv_pack->cmdData, &firstByte, sizeof(firstByte))) {
    ack_pack->cmdType = ACK_SUCCESSFUL;
    ack_pack->cmdLen = sizeof(ack_data);
    memcpy(ack_pack->cmdData, ack_data, sizeof(ack_data));
  } else {
    ack_pack->cmdType = ACK_CMDDATA_ERR;
  }
}