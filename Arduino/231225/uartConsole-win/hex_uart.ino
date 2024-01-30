#include "hex_uart.h"

void setup() {
  hex_uart_init(UART_ID, UART_TX_PIN, UART_RX_PIN, BAUD_RATE);
}

void loop() {
}
