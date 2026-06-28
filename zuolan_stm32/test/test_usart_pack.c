#include <stdio.h>
#include "my_usart_pack.h"

int main()
{
    /*** 接收相关变量 ***/
    int recvInt1 = 0, recvInt2 = 0, recvInt3 = 0;
    float recvFloat1 = 0.0;
    uint8_t recvByte1 = 0;
    int recvInts[5] = {0};

    // 定义接收数据的模板数组和变量数组
    DataType recvTemplateArray[] = {
        TYPE_INT,   // recvInt1
        TYPE_FLOAT, // recvFloat1
        TYPE_BYTE,  // recvByte1
        TYPE_INT,   // recvInt2
        TYPE_INT,   // recvInt3
        TYPE_INT,   // recvInts[0]
        TYPE_INT,   // recvInts[1]
        TYPE_INT,   // recvInts[2]
        TYPE_INT,   // recvInts[3]
        TYPE_INT    // recvInts[4]
    };

    void *recvVariableArray[] = {
        &recvInt1,    // recvInt1
        &recvFloat1,  // recvFloat1
        &recvByte1,   // recvByte1
        &recvInt2,    // recvInt2
        &recvInt3,    // recvInt3
        &recvInts[0], // recvInts[0]
        &recvInts[1], // recvInts[1]
        &recvInts[2], // recvInts[2]
        &recvInts[3], // recvInts[3]
        &recvInts[4]  // recvInts[4]
    };

    /*** 发送相关变量 ***/
    int sendInt1 = 500, sendInt2 = 1000, sendInt3 = 1500;
    float sendFloat1 = 25.7;
    uint8_t sendByte1 = 250;
    int sendInts[5] = {10, 20, 30, 40, 50};

    // 定义发送数据的模板数组和变量数组
    DataType sendTemplateArray[] = {
        TYPE_INT,   // sendInt1
        TYPE_FLOAT, // sendFloat1
        TYPE_BYTE,  // sendByte1
        TYPE_INT,   // sendInt2
        TYPE_INT,   // sendInt3
        TYPE_INT,   // sendInts[0]
        TYPE_INT,   // sendInts[1]
        TYPE_INT,   // sendInts[2]
        TYPE_INT,   // sendInts[3]
        TYPE_INT    // sendInts[4]
    };

    void *sendVariableArray[] = {
        &sendInt1,    // sendInt1
        &sendFloat1,  // sendFloat1
        &sendByte1,   // sendByte1
        &sendInt2,    // sendInt2
        &sendInt3,    // sendInt3
        &sendInts[0], // sendInts[0]
        &sendInts[1], // sendInts[1]
        &sendInts[2], // sendInts[2]
        &sendInts[3], // sendInts[3]
        &sendInts[4]  // sendInts[4]
    };

    /*** 模拟接收到的数据帧 ***/
    uint8_t receivedFrame[] = {
        FRAME_HEADER,           // 帧头
        0x00, 0x00, 0x00, 0x64, // recvInt1 = 100
        0x42, 0x48, 0x96, 0x49, // recvFloat1 = 50.3 (假设 IEEE754)
        0x7F,                   // recvByte1 = 127
        0x00, 0x00, 0x00, 0xC8, // recvInt2 = 200
        0x00, 0x00, 0x00, 0x2A, // recvInt3 = 42
        0x00, 0x00, 0x00, 0x01, // recvInts[0] = 1
        0x00, 0x00, 0x00, 0x02, // recvInts[1] = 2
        0x00, 0x00, 0x00, 0x03, // recvInts[2] = 3
        0x00, 0x00, 0x00, 0x04, // recvInts[3] = 4
        0x00, 0x00, 0x00, 0x05, // recvInts[4] = 5
        0xD8,                   // 校验和
        FRAME_TAIL              // 帧尾
    };

    uint16_t frameLength = sizeof(receivedFrame);

    /*** 解析接收到的数据帧 ***/
    // 设置接收模板
    SetParseTemplate(recvTemplateArray, recvVariableArray, 10);

    // 解析接收到的数据帧
    ParseFrame(receivedFrame, frameLength);

    // 打印解析后的变量值
    printf("Parsed Received Data:\n");
    printf("recvInt1: %d\n", recvInt1);
    printf("recvFloat1: %f\n", recvFloat1);
    printf("recvByte1: %d\n", recvByte1);
    printf("recvInt2: %d\n", recvInt2);
    printf("recvInt3: %d\n", recvInt3);
    for (int i = 0; i < 5; i++)
    {
        printf("recvInts[%d]: %d\n", i, recvInts[i]);
    }

    /*** 准备发送序列化后的数据帧 ***/
    // 设置发送模板
    SetParseTemplate(sendTemplateArray, sendVariableArray, 10);

    // 准备发送数据帧
    uint8_t sendBuffer[256];
    uint16_t sendFrameLength = PrepareFrame(sendBuffer, sizeof(sendBuffer));

    if (sendFrameLength > 0)
    {
        // 发送数据帧
        printf("\nSerialized and Sending Data:\n");
        SendFrame(sendBuffer, sendFrameLength);
    }
    else
    {
        printf("Failed to prepare frame for sending!\n");
    }

    return 0;
}
