/*******************************************************************************
 * @file      my_usart_pack.c
 * @author    左岚
 * @version   V2.0
 * @date      2025-07-18
 * @brief     一个通用的、基于模板的串口数据帧打包与解包模块
 * @note      该模块实现了一套灵活的数据通信协议。用户可通过`SetParseTemplate`
 * 函数在运行时定义数据帧中包含的变量类型、顺序和地址，模块会
 * 自动根据此模板进行数据的序列化（打包）和反序列化（解包）。
 *
 * 协议帧结构为：[帧头] [数据区] [校验和] [帧尾]，保证了基本的通信可靠性。
 *
 * V2.0更新: 将 `SendFrame` 函数修改为接收 `UART_HandleTypeDef` 句柄
 * 作为参数，移除了对全局宏的依赖，使发送功能更加灵活。
 *******************************************************************************/

#include "my_usart_pack.h"
#include <string.h> // 用于内存操作

// ======================== 模块内部静态全局变量 ========================
// 这些变量用于存储用户定义的通信协议模板。

// 数据类型模板数组：存储每个变量的数据类型（如BYTE, INT, FLOAT）。
static DataType parseTemplate[MAX_VARIABLES];
// 变量地址映射数组：使用void指针存储每个变量在内存中的实际地址。
// 这是实现通用性的关键，允许模块直接读写任意类型的外部变量。
static void *variableMapping[MAX_VARIABLES];
// 模板中已定义的变量个数
static uint16_t variableCount = 0;

/**
 * @brief  设置数据解析/打包的模板
 * @details 此函数是使用本模块的第一步，用于定义通信数据帧的内容。
 * @param  templateArray 指向一个包含变量数据类型的枚举数组
 * @param  variableArray 指向一个包含变量实际地址的void指针数组
 * @param  count         模板中变量的总个数
 * @retval None
 */
void SetParseTemplate(DataType *templateArray, void **variableArray, uint16_t count)
{
    // 防止配置的变量数超出模块支持的最大限制
    if (count > MAX_VARIABLES)
        return;

    // 循环拷贝用户定义的模板到模块内部的静态变量中
    for (uint16_t i = 0; i < count; i++)
    {
        parseTemplate[i] = templateArray[i];   // 设置数据类型
        variableMapping[i] = variableArray[i]; // 关联变量地址
    }
    variableCount = count; // 更新当前模板的变量总数
}

/**
 * @brief  [内部函数] 按模板将数据区字节流解析到变量中
 * @param  my_data 指向数据帧中纯数据区的指针
 * @param  length  纯数据区的长度
 * @retval None
 */
static void ParseDataToVariables(uint8_t *my_data, uint16_t length)
{
    uint16_t index = 0; // 用于追踪在数据区(my_data)中解析的位置

    // 按照模板中定义的顺序，逐个解析变量
    for (uint16_t i = 0; i < variableCount; i++)
    {
        // 如果数据区已经解析完毕，但模板还未完成，则提前退出
        if (index >= length)
            return;

        // 根据当前变量的类型，选择不同的解析方式
        switch (parseTemplate[i])
        {
        case TYPE_BYTE: // 单字节类型 (uint8_t)
        {
            // 将void*指针强制转换为目标类型的指针
            uint8_t *byteVar = (uint8_t *)variableMapping[i];
            // 解引用指针，将解析出的值赋给模板关联的变量
            *byteVar = my_data[index];
            index++; // 单字节占用1个字节
            break;
        }
        case TYPE_SHORT: // 短整型 (uint16_t)，大端模式 (Big-Endian)
        {
            if (index + 1 >= length) return; // 检查剩余长度是否足够
            uint16_t *shortVar = (uint16_t *)variableMapping[i];
            // 将接收到的高字节和低字节拼接成一个16位数
            *shortVar = (my_data[index] << 8) | my_data[index + 1];
            index += 2; // 短整型占用2个字节
            break;
        }
        case TYPE_INT: // 整型 (int / uint32_t)，大端模式 (Big-Endian)
        {
            if (index + 3 >= length) return; // 检查剩余长度
            int *intVar = (int *)variableMapping[i];
            // 拼接4个字节
            *intVar = (my_data[index] << 24) | (my_data[index + 1] << 16) |
                    (my_data[index + 2] << 8) | my_data[index + 3];
            index += 4; // 整型占用4个字节
            break;
        }
        case TYPE_FLOAT: // 浮点型 (float)
        {
            if (index + 3 >= length) return; // 检查剩余长度
            float *floatVar = (float *)variableMapping[i];
            // 这是一个技巧，通过指针类型转换直接将4个字节的内存内容解释为float。
            // 这要求发送方和接收方遵循相同的浮点数表示法（通常是IEEE 754）和相同的字节序(Endianness)。
            *floatVar = *((float *)(my_data + index));
            index += 4; // 浮点型占用4个字节
            break;
        }
        default:
            // 如果遇到不支持的类型，则停止解析
            return;
        }
    }
}

/**
 * @brief  数据帧解析入口函数
 * @details 负责校验整个数据帧的合法性（帧头、帧尾、校验和），
 * 如果合法，则调用内部函数对数据区进行解析。
 * @param  buffer 指向接收到的完整数据帧的指针
 * @param  length 数据帧的总长度
 * @retval None
 */
void ParseFrame(uint8_t *buffer, uint16_t length)
{
    // 1. 检查帧的最小长度是否满足
    if (length < MIN_FRAME_LENGTH)
        return;

    // 2. 检查帧头和帧尾是否正确
    if (buffer[0] != FRAME_HEADER || buffer[length - 1] != FRAME_TAIL)
        return;

    // 3. 校验和验证
    uint16_t dataLength = length - 3; // 纯数据区的长度 = 总长 - 帧头(1) - 校验和(1) - 帧尾(1)
    uint8_t checksum = 0;
    // 计算从帧头后到校验和前的所有字节的累加和
    for (uint16_t i = 1; i <= dataLength; i++)
    {
        checksum += buffer[i];
    }
    // 将计算出的校验和与接收到的校验和进行比较
    if ((checksum & 0xFF) != buffer[length - 2])
        return;

    // 4. 所有校验通过，调用核心解析函数
    ParseDataToVariables(buffer + 1, dataLength); // buffer+1 跳过帧头，指向数据区开始
}

/**
 * @brief  数据帧序列化函数（打包数据帧以备发送）
 * @param  buffer      一个足够大的缓冲区指针，用于存储打包后的数据帧
 * @param  maxLength   缓冲区的最大长度，防止溢出
 * @return (uint16_t)  成功则返回打包好的数据帧的总长度，失败则返回0
 */
uint16_t PrepareFrame(uint8_t *buffer, uint16_t maxLength)
{
    if (maxLength < MIN_FRAME_LENGTH) // 检查缓冲区容量
        return 0;

    uint16_t index = 0; // 用于在buffer中构建数据帧的索引

    // 1. 添加帧头
    buffer[index++] = FRAME_HEADER;

    // 2. 根据模板序列化数据区
    for (uint16_t i = 0; i < variableCount; i++)
    {
        if (index >= maxLength) return 0; // 检查缓冲区是否将要溢出

        switch (parseTemplate[i])
        {
        case TYPE_BYTE:
        {
            if (index + 1 > maxLength) return 0;
            uint8_t *byteVar = (uint8_t *)variableMapping[i];
            buffer[index++] = *byteVar;
            break;
        }
        case TYPE_SHORT: // 按大端模式(Big-Endian)拆分，高位在前
        {
            if (index + 2 > maxLength) return 0;
            uint16_t *shortVar = (uint16_t *)variableMapping[i];
            buffer[index++] = (*shortVar >> 8) & 0xFF; // 高字节
            buffer[index++] = *shortVar & 0xFF;        // 低字节
            break;
        }
        case TYPE_INT: // 按大端模式(Big-Endian)拆分
        {
            if (index + 4 > maxLength) return 0;
            int *intVar = (int *)variableMapping[i];
            buffer[index++] = (*intVar >> 24) & 0xFF;
            buffer[index++] = (*intVar >> 16) & 0xFF;
            buffer[index++] = (*intVar >> 8) & 0xFF;
            buffer[index++] = *intVar & 0xFF;
            break;
        }
        case TYPE_FLOAT:
        {
            if (index + 4 > maxLength) return 0;
            float *floatVar = (float *)variableMapping[i];
            // 使用指针转换，将float变量的内存内容按字节拷贝到缓冲区。
            uint8_t *floatBytes = (uint8_t *)floatVar;
            // **注意**: 这里的字节序完全取决于MCU的大小端(endianness)模式。
            // 这与上面整型(int, short)的显式大端打包方式不一致。
            // 为保证跨平台兼容性，最好对浮点数也进行显式的大小端处理。
            buffer[index++] = floatBytes[0];
            buffer[index++] = floatBytes[1];
            buffer[index++] = floatBytes[2];
            buffer[index++] = floatBytes[3];
            break;
        }
        default:
            return 0; // 不支持的类型
        }
    }

    // 3. 计算并添加校验和
    if (index + 1 > maxLength) return 0;
    uint8_t checksum = 0;
    // 对数据区（从buffer[1]到当前index之前）计算累加和
    for (uint16_t i = 1; i < index; i++)
    {
        checksum += buffer[i];
    }
    buffer[index++] = checksum & 0xFF;

    // 4. 添加帧尾
    if (index < maxLength)
    {
        buffer[index++] = FRAME_TAIL;
    }
    else
    {
        return 0; // 缓冲区已满，无法添加帧尾
    }

    // 返回构建好的完整数据帧的长度
    return index;
}

/**
 * @brief  通过指定的UART发送数据帧
 * @param  huart_blue  用于通信的UART句柄 (例如 &huart2)
 * @param  frame       指向已打包好的数据帧的指针
 * @param  length      数据帧的长度 (由PrepareFrame函数返回)
 * @retval None
 */
void SendFrame(UART_HandleTypeDef huart_blue, uint8_t *frame, uint16_t length)
{
    // 调用HAL库函数，通过指定的UART句柄发送数据
    HAL_UART_Transmit(&huart_blue, frame, length, HAL_MAX_DELAY);
}
