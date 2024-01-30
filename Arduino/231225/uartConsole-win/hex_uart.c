#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hex_uart.h"

#define UART_BUFFER_SIZE 1024
#define DATA_BITS 8  // 数据位长度
#define STOP_BITS 1  // 停止位长度
#define PARITY UART_PARITY_NONE

#define PACK_START_FLAG (0x7E)
#define PACK_STOP_FLAG (0x7F)

static volatile int recv_size = 0;
static uart_inst_t* uart_index;
static volatile bool uart_recv_flag = false;
static uint8_t recv_buffer[UART_BUFFER_SIZE];

extern int user_do_inquire(DATAPACK* rcv_pack, DATAPACK* DATAPACK);
extern void user_do_setSomething(DATAPACK* rcv_pack, DATAPACK* ack_pack);

static void dump_packet(DATAPACK* unit) {
  uart_putc(uart_index, unit->startFlag);
  uart_putc(uart_index, unit->moduleAddress);
  uart_putc(uart_index, unit->cmdNumber);
  uart_putc(uart_index, unit->cmdType);
  uart_putc(uart_index, unit->cmdLen);
  for (int i = 0; i < unit->cmdLen; i++) {
    uart_putc(uart_index, unit->cmdData[i]);
  }
  uart_write_blocking(uart_index, (uint8_t*)&unit->crc, sizeof(uint16_t));
  uart_putc(uart_index, unit->stopFlag);
}

static uint16_t crc_ccitt(uint8_t* data, uint16_t size) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < size; ++i) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint16_t j = 0; j < 8; ++j) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

static DATAPACK* get_uart_recv_packet(uint8_t* data) {
  // Parser Packet
  uint16_t crc;
  uint8_t dataLen = data[4];
  memcpy(&crc, &data[5 + dataLen], sizeof(crc));

  // Alloca protocol packet for receive data
  DATAPACK* uint = (DATAPACK*)malloc(sizeof(DATAPACK) + dataLen);
  if (uint == NULL) {
    // printf("malloc a uart protocol unit Failed!\n");
    return NULL;
  }

  // Parser Packet other params
  uint->startFlag = data[0];
  uint->moduleAddress = data[1];
  uint->cmdNumber = data[2];
  uint->cmdType = data[3];
  uint->cmdLen = dataLen;
  memcpy(uint->cmdData, data + 5, dataLen);
  uint->crc = crc;
  uint->stopFlag = data[5 + dataLen + 2];
  return uint;
}

static DATAPACK* check_uart_recv_packet(DATAPACK* rcv_pack) {
  // Alloca protocol packet for ack
  DATAPACK* ack_pack = malloc(sizeof(DATAPACK) + UART_BUFFER_SIZE);
  if (ack_pack == NULL) {
    // printf("malloc ack unit Failed!\n");
    return NULL;
  }
  memset(ack_pack, 0, sizeof(DATAPACK) + UART_BUFFER_SIZE);

  // Check Command and Command Data
  switch (rcv_pack->cmdType) {
    case CMD_INQUIRE:
      user_do_inquire(rcv_pack, ack_pack);
      break;
    case CMD_SET:
      user_do_setSomething(rcv_pack, ack_pack);
      break;
    default:
      ack_pack->cmdType = ACK_CMD_UNSUPPORT;
      return ack_pack;
  }

  // Check CRC
  int checkLen = 4 + rcv_pack->cmdLen;
  uint8_t buffer[checkLen];
  memcpy(buffer, &rcv_pack->moduleAddress, 4);
  memcpy(buffer + 4, rcv_pack->cmdData, checkLen);
  uint16_t crc = crc_ccitt(buffer, checkLen);
  if (rcv_pack->crc != crc) {
    // printf("CRC Check Failed! [%02x - %02x]\n", rcv_pack->crc, crc);
    ack_pack->cmdLen = 0;
    ack_pack->cmdType = ACK_CRC_ERR;
  }
  return ack_pack;
}

static void build_uart_ack_packet(DATAPACK* rcv_pack, DATAPACK* ack_pack) {
  // Build ack pack
  ack_pack->startFlag = PACK_START_FLAG;
  ack_pack->moduleAddress = rcv_pack->moduleAddress;
  ack_pack->cmdNumber = rcv_pack->cmdNumber;

  if (ack_pack->cmdLen == 0) {
    // Err Occur
    ack_pack->cmdLen = rcv_pack->cmdLen;
    memcpy(ack_pack->cmdData, rcv_pack->cmdData, rcv_pack->cmdLen);
  }
  int checkLen = 4 + ack_pack->cmdLen;
  uint8_t buffer[checkLen];
  memcpy(buffer, &ack_pack->moduleAddress, 4);
  memcpy(buffer + 4, ack_pack->cmdData, checkLen);
  uint16_t new_crc = crc_ccitt(buffer, checkLen);
  ack_pack->crc = new_crc;
  ack_pack->stopFlag = PACK_STOP_FLAG;
}

static int do_process_data(uint8_t* recv) {
  int ret = 0;
  DATAPACK* rcv_pack;
  DATAPACK* ack_pack;
  // Get Recv Packet
  rcv_pack = get_uart_recv_packet(recv);
  // Parser Recv Packet
  ack_pack = check_uart_recv_packet(rcv_pack);
  // Build Ack Packet
  build_uart_ack_packet(rcv_pack, ack_pack);
  // Send Ack
  dump_packet(ack_pack);
  // Error State
  if (rcv_pack == NULL || ack_pack == NULL) {
    ret = -1;
  }
  // Free Buffer
  if (rcv_pack != NULL) {
    free(rcv_pack);
    rcv_pack = NULL;
  }
  if (ack_pack != NULL) {
    free(ack_pack);
    ack_pack = NULL;
  }
  return ret;
}

static void uart_rx_handler() {
  while (uart_is_readable(uart_index)) {
    uint8_t ch = uart_getc(uart_index);
    // Only Recv Useful chars
    if (ch == PACK_START_FLAG) {
      uart_recv_flag = true;
    }
    // Store chars
    if (uart_recv_flag == true) {
      recv_buffer[recv_size] = ch;
      recv_size++;
    }
    // Process Data
    if (ch == PACK_STOP_FLAG) {
      uart_recv_flag = false;
      if (!do_process_data(recv_buffer)) {
        memset(recv_buffer, 0, recv_size);
        recv_size = 0;
      }
    }
  }
}

void hex_uart_init(uart_inst_t* uartId, uint32_t tx, uint32_t rx, uint32_t baud_rate) {
  // Init UART
  uart_index = uartId;
  uart_init(uart_index, baud_rate);

  // Set Uart Function
  gpio_set_function(tx, GPIO_FUNC_UART);
  gpio_set_function(rx, GPIO_FUNC_UART);

  // Set Baudrate
  uart_set_baudrate(uart_index, baud_rate);

  // Close hardware Flow Control. If We need FlowControl must to set PIN_CTS PIN_RTS
  uart_set_hw_flow(uart_index, false, false);

  // Set Data Format(8N1)
  uart_set_format(uart_index, DATA_BITS, STOP_BITS, PARITY);

  // Get Correct IRQ For RX
  int UART_IRQ = uart_index == uart0 ? UART0_IRQ : UART1_IRQ;

  // Set up and enable the interrupt handlers
  irq_set_exclusive_handler(UART_IRQ, uart_rx_handler);
  irq_set_enabled(UART_IRQ, true);

  // Enable IRQ
  uart_set_irq_enables(uart_index, true, false);

  // Enable FIFO
  uart_set_fifo_enabled(uart_index, true);

  // Clear FIFO
  uart_getc(uart_index);

  memset(recv_buffer, 0, UART_BUFFER_SIZE);
  // Hex Uart Init Sucessful
}

void hex_uart_deinit(void) {
  // Disable FIFO
  uart_set_fifo_enabled(uart_index, false);
  // Disable IRQ
  int UART_IRQ = uart_index == uart0 ? UART0_IRQ : UART1_IRQ;
  irq_set_enabled(UART_IRQ, false);
  // deinit uart
  uart_deinit(uart_index);
}
