#ifndef __HEXCONSOLE_H
#define __HEXCONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif



    typedef unsigned char uint8_t; 
    typedef unsigned short uint16_t;
    // User Config
#define UART_BUFFER_SIZE 1024
#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define BAUD_RATE 115200  // 波特率参数
#define DATA_BITS 8       // 数据位长度
#define STOP_BITS 1       // 停止位长度
#define PARITY UART_PARITY_NONE


#define PACK_START_FLAG 0x7E
#define PACK_STOP_FLAG 0X7F

    typedef struct uart_data_pack {
        uint8_t startFlag;
        uint8_t moduleAddress;
        uint8_t cmdNumber;
        // For Machine->cmdType; For User->ackType
        uint8_t cmdType;
        uint8_t cmdLen;
        // DATA
        uint16_t crc;
        uint8_t stopFlag;
        // Zero long data must be put in the end
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
        // 起始/结束位错误
        ACK_MAGIC_ERR = 0x04,
        // More
    } ACK_CMD;


    typedef enum user_command {
        // 查询
        CMD_INQUIRE = 0x00,
        // 设置
        CMD_SET = 0x01,
        // More
    } USR_CMD;



#ifdef __cplusplus
}
#endif

#endif /* __HEXCONSOLE_H */
