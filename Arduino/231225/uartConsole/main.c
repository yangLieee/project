#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdbool.h"
#include "uartConsole.h"


#define MAX_DATAPACK_DATASIZE 1024
static uint8_t recv[UART_BUFFER_SIZE] = { 0 };
static uint8_t send[UART_BUFFER_SIZE] = { 0 };
static volatile bool uart_recv_flag = false;
static volatile uint16_t chars_rxed = 0;


static uint16_t chars_txed = 0;
static uint16_t recv_flag = 0;  // 可继续写入buffer的标志位，在识别到0x7E时置为1表示可写
static uint8_t have_done_chars = 0;

static uint8_t test_data[] = { 0x7E, 0x00, 0xA0, 0x01, 0x1C,
    0x6C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Data Start
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,  // Data End
    0xF9, 0x3D, 0x7F };

static uint8_t ack_data[] = {
    0x6C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x23, 0x11, 0x27, 0x17, 0x00};



// Maybe Export
int user_do_inquire(DATAPACK* rcv_pack, DATAPACK* DATAPACK) {
    printf("%s Enter \n", __func__);
    // User to do
    return 0;
}

// Maybe Export
void user_do_setSomething(DATAPACK* rcv_pack, DATAPACK* ack_pack) {
    // User to do. This is demo
    
    int ack_dataLen = 0;
    uint8_t firstByte = 0x6C;

    if(!strncmp(rcv_pack->cmdData, &firstByte, sizeof(firstByte))){
        ack_pack->cmdType = ACK_SUCCESSFUL;
        ack_pack->cmdLen = sizeof(ack_data);
        memcpy(ack_pack->cmdData, ack_data, sizeof(ack_data));
    }
    else {
        ack_pack->cmdType = ACK_CMDDATA_ERR;
    }
}

static void dump_packet(DATAPACK* unit)
{
    printf("================ Dump Unit =============\n");
    printf("StartFlag:      %x\n", unit->startFlag);
    printf("moduleAddress:  %x\n", unit->moduleAddress);
    printf("cmdNumber:      %x\n", unit->cmdNumber);
    printf("cmdType:        %x\n", unit->cmdType);
    printf("cmdLen:         %x\n", unit->cmdLen);
    printf("crc:            %x\n", unit->crc);
    printf("stopFlag:       %x\n", unit->stopFlag);
    printf(" Data :");
    for(int i=0; i<unit->cmdLen; i++)
//        printf(" Data[%d]  %x \n",i, (unit->cmdData[i]));
        printf(" [%x] ",(unit->cmdData[i]));
    printf("\n");
    printf("================ Dump Unit =============\n");
}

/* CCITT */
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

static DATAPACK* get_uart_recv_unit(uint8_t* data) {
    DATAPACK* uint = NULL;
    // Parser params
    uint16_t crc;
    uint8_t dataLen = data[4];
    memcpy(&crc, &data[5 + dataLen], sizeof(crc));

    // Alloca struct uint
    uint = (DATAPACK*)malloc(sizeof(DATAPACK) + dataLen);
    if (uint == NULL) {
        // printf("malloc a uart protocol unit Failed!\n");
        return NULL;
    }
    // Init
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

static DATAPACK* check_uart_packet(DATAPACK* rcv_pack) {
    int ret = -1;
    DATAPACK* ack_pack;
    ack_pack = malloc(sizeof(DATAPACK) + MAX_DATAPACK_DATASIZE);
    if(ack_pack == NULL) {
        printf("malloc ack unit Failed!\n");
        return NULL;
    }
    ack_pack->cmdLen = 0;

    // Check Magic Flag
    if ((rcv_pack->startFlag != PACK_START_FLAG) || (rcv_pack->stopFlag != PACK_STOP_FLAG)) {
        printf("Start or Stop Flag Check Failed!\n");
        ack_pack->cmdType = ACK_MAGIC_ERR;
    }

    // Check Crc
    int checkLen = 1 + 1 + 1 + 1 + rcv_pack->cmdLen;
    uint8_t buffer[checkLen];
    memcpy(buffer,   &rcv_pack->moduleAddress, 4);
    memcpy(buffer+4, rcv_pack->cmdData, checkLen);
    uint16_t crc = crc_ccitt(buffer, checkLen);
    if (rcv_pack->crc != crc) {
        printf("CRC Check Failed! [%02x - %02x]\n", rcv_pack->crc, crc);
        ack_pack->cmdType = ACK_CRC_ERR;
    }

    // Check CMD
    switch (rcv_pack->cmdType) {
        case CMD_INQUIRE:
            user_do_inquire(rcv_pack, ack_pack);
            break;
        case CMD_SET:
            user_do_setSomething(rcv_pack, ack_pack);
            break;
        default:
            ack_pack->cmdType = ACK_CMD_UNSUPPORT;
            break;
    }

    // Check successful
    return ack_pack;
}


void uart_packet_ack(DATAPACK* rcv_pack, DATAPACK* ack_pack) 
{
    int checkLen = 0;
    uint16_t new_crc = 0;
    ack_pack->startFlag = PACK_START_FLAG;
    ack_pack->moduleAddress = rcv_pack->moduleAddress;
    ack_pack->cmdNumber = rcv_pack->cmdNumber;
    /* ack_pack->cmdType has been given */
    if(ack_pack->cmdLen == 0) {
        ack_pack->cmdLen = rcv_pack->cmdLen;
        memcpy(ack_pack->cmdData, rcv_pack->cmdData, rcv_pack->cmdLen);
        ack_pack->crc = rcv_pack->crc;
    }
    else {
        checkLen = 1 + 1 + 1 + 1 + ack_pack->cmdLen;
        uint8_t buffer[checkLen];
        memcpy(buffer,   &ack_pack->moduleAddress, 4);
        memcpy(buffer+4, ack_pack->cmdData, checkLen);
        new_crc = crc_ccitt(buffer, checkLen);
        ack_pack->crc = new_crc;
    }
    ack_pack->stopFlag = PACK_STOP_FLAG;
}



int main(int argc, char* argv[])
{
    memcpy(recv, test_data, sizeof(test_data));
#ifdef DEBUG
    for(int i=0; i<sizeof(test_data);i++) {
        printf(" %x ", recv[i]);
    }
    printf("\n");
#endif
    DATAPACK*   rcv_pack;
    DATAPACK*   ack_pack;

    rcv_pack = get_uart_recv_unit(recv);
    dump_packet(rcv_pack);
    ack_pack = check_uart_packet(rcv_pack);
    uart_packet_ack(rcv_pack, ack_pack);
    dump_packet(ack_pack);

    free(rcv_pack);
    free(ack_pack);



    return 0;
}
