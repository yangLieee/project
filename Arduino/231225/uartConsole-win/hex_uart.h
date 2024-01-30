#ifndef __HEXCONSOLE_H
#define __HEXCONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hardware/uart.h"

#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_ID uart0
#define BAUD_RATE 115200

  typedef struct uart_data_pack {
    uint8_t startFlag;
    uint8_t moduleAddress;
    uint8_t cmdNumber;
    // 对于设备端是命令请求类型，对于用户端是应答类型
    uint8_t cmdType;
    uint8_t cmdLen;
    // DATA
    uint16_t crc;
    uint8_t stopFlag;
    uint8_t cmdData[0];
  } DATAPACK;

  typedef enum command_ack {
    // 成功
    ACK_SUCCESSFUL = 0x00,
    // 命令号不支持
    ACK_CMD_UNSUPPORT = 0x01,
    // 命令数据错
    ACK_CMDDATA_ERR = 0x02,
    // CRC 校验错
    ACK_CRC_ERR = 0x03,
    // More
  } ACK_CMD;


  typedef enum user_command {
    // 查询
    CMD_INQUIRE = 0x00,
    // 设置
    CMD_SET = 0x01,
    // More
  } USR_CMD;

/*
 * 串口功能初始化
 * @param uartId : 选择使用的串口号
 * @param tx : 串口的发送引脚号
 * @param rx : 串口的接收引脚号
 * @param baud_rate : 串口的波特率，默认115200
 */   
  void hex_uart_init(uart_inst_t* uartId, uint32_t tx, uint32_t rx, uint32_t baud_rate);

/*
 * 串口功能失能
 */
  void hex_uart_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __HEXCONSOLE_H */
