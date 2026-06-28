/*******************************************************************************
 * @file      my_hmi.c
 * @author    左岚
 * @version   V2.0
 * @date      2025-07-18
 * @brief     模块化的串口HMI触摸屏通信驱动
 * @note      此文件实现了一系列函数，用于通过串口向HMI屏幕发送指令。
 * 与上一版本的主要区别在于，本版将通信串口的句柄`UART_HandleTypeDef`
 * 作为参数直接传入每个函数，移除了对全局宏的依赖。这使得驱动更加
 * 灵活，理论上可以同时操作连接在不同串口上的多个HMI屏幕。
 *
 * 协议本身保持不变，仍基于ASCII指令集，并以`\xff\xff\xff`作为结束符。
 *******************************************************************************/

#include "my_hmi.h"
#include "math.h" // 包含数学库，用于HMI_Send_Float中的pow()函数

/**
 * @brief  向HMI控件发送字符串数据
 * @param  huart_hmi   用于通信的UART句柄 (例如 &huart1)。函数参数名 `huartr_hmi` 存在拼写错误，此处按实际使用 `huart_hmi` 注释。
 * @param  obj_name    控件的名称 (例如 "p0.t0")
 * @param  show_data   要显示的字符串内容
 * @retval None
 */
void HMI_Send_String(UART_HandleTypeDef huartr_hmi, char *obj_name, char *show_data)
{
    // 将参数 `huartr_hmi` 的地址传递给 my_printf 函数
    UART_HandleTypeDef huart_hmi = huartr_hmi; // 创建一个副本以匹配原有代码逻辑
    // 指令格式: 控件名.txt="字符串" + 结束符
    my_printf(&huart_hmi, "%s.txt=\"%s\"\xff\xff\xff", obj_name, show_data);
}

/**
 * @brief  向HMI控件发送整数数据
 * @param  huart_hmi   用于通信的UART句柄
 * @param  obj_name    控件的名称 (例如 "p0.n0")
 * @param  show_data   要显示的整数值
 * @retval None
 */
void HMI_Send_Int(UART_HandleTypeDef huartr_hmi, char *obj_name, int show_data)
{
    UART_HandleTypeDef huart_hmi = huartr_hmi;
    // 指令格式: 控件名.val=整数值 + 结束符
    my_printf(&huart_hmi, "%s.val=%d\xff\xff\xff", obj_name, show_data);
}

/**
 * @brief  向HMI控件发送浮点数数据
 * @details 通过将浮点数放大为整数来间接实现浮点数的显示。
 * @param  huart_hmi   用于通信的UART句柄
 * @param  obj_name    控件的名称 (例如 "p0.n0")
 * @param  show_data   要显示的浮点数值
 * @param  point_index 小数点后保留的位数
 * @retval None
 */
void HMI_Send_Float(UART_HandleTypeDef huartr_hmi, char *obj_name, float show_data, int point_index)
{
    UART_HandleTypeDef huart_hmi = huartr_hmi;
    // 例如: 发送12.34, point_index=2, 则temp = 12.34 * 10^2 = 1234。
    // HMI端需要相应配置小数点位置才能正确显示。
    int temp = show_data * pow(10, point_index);
    my_printf(&huart_hmi, "%s.val=%d\xff\xff\xff", obj_name, temp);
}

/**
 * @brief  清除指定波形控件上的某条通道数据
 * @param  huart_hmi   用于通信的UART句柄
 * @param  obj_name    波形控件的名称 (例如 "s0")
 * @param  ch          要清除的波形通道编号 (通常从0开始)
 * @retval None
 */
void HMI_Wave_Clear(UART_HandleTypeDef huartr_hmi, char *obj_name, int ch)
{
    UART_HandleTypeDef huart_hmi = huartr_hmi;
    // 指令格式: cle 控件名,通道号 + 结束符
    my_printf(&huart_hmi, "cle %s,%d\xff\xff\xff", obj_name, ch);
}

/**
 * @brief  向指定波形控件添加单个数据点 (低速)
 * @param  huart_hmi   用于通信的UART句柄
 * @param  obj_name    波形控件的名称 (例如 "s0")
 * @param  ch          波形通道编号
 * @param  val         数据值 (范围0-255)
 * @retval None
 */
void HMI_Write_Wave_Low(UART_HandleTypeDef huartr_hmi, char *obj_name, int ch, int val)
{
    UART_HandleTypeDef huart_hmi = huartr_hmi;
    // 指令格式: add 控件ID,通道号,数值 + 结束符
    my_printf(&huart_hmi, "add %s.id,%d,%d\xff\xff\xff", obj_name, ch, val);
}

/**
 * @brief  使用透明传输模式向波形控件快速发送一批数据 (高速)
 * @param  huart_hmi   用于通信的UART句柄
 * @param  obj_name    波形控件的名称 (例如 "s0")
 * @param  ch          波形通道编号
 * @param  len         要发送的数据点数量
 * @param  val         存储数据点的数组指针 (每个值范围0-255)
 * @retval None
 */
void HMI_Write_Wave_Fast(UART_HandleTypeDef huartr_hmi, char *obj_name, int ch, int len, int *val)
{
    UART_HandleTypeDef huart_hmi = huartr_hmi;
    // 步骤1: 发送 "addt" 指令，使HMI进入“透明传输”模式
    my_printf(&huart_hmi, "addt %s.id,%d,%d\xff\xff\xff", obj_name, ch, len);

    // 步骤2: 短暂延时，确保HMI准备好接收原始数据流
    HAL_Delay(100);

    // 步骤3: 以原始字节形式循环发送波形数据
    for (int i = 0; i < len; i++)
    {
        my_printf(&huart_hmi, "%c", val[i]);
    }

    // 步骤4: 发送一个无效指令来强制HMI退出透传模式
    my_printf(&huart_hmi, "\x01\xff\xff\xff");
}
