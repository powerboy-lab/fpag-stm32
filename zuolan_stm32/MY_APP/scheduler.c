/*******************************************************************************
 * @file      scheduler.c
 * @author    左岚
 * @version   V1.0
 * @date      2025-07-18
 * @brief     一个简单的前后台任务调度器实现
 * @note      此调度器实现了一个基础的非抢占式、时间片轮转调度策略。它适用于
 * 裸机（无操作系统）环境。通过在主循环中不断调用 `scheduler_run()`
 * 函数，可以周期性地检查并执行任务列表中的任务。所有任务均在
 * 同一个上下文中执行（通常是主循环的上下文）。
 *******************************************************************************/

#include "scheduler.h"
#include "key_app.h"
#include "AD9959.h"

// 全局变量，用于存储在任务列表中定义的任务数量。
uint8_t task_num;

/**
 * @brief 任务控制块结构体定义
 * @note  每个任务都由一个该结构体来描述。
 */
typedef struct
{
    void (*task_func)(void); // 任务函数的指针，指向需要被调度的具体函数。
    uint32_t rate_ms;        // 任务执行的周期，单位：毫秒(ms)。
    uint32_t last_run;       // 上一次任务执行的时间戳 (由HAL_GetTick()提供)。
} task_t;

// 一个未使用的全局变量，建议移除以保持代码整洁。
u32 i = 0;

/**
 * @brief  串口处理任务示例
 * @details 此函数作为一个示例任务，通过串口1打印当前的电压反馈值和PID输出值，
 * 常用于系统调试和状态监控。
 * @param  None
 * @retval None
 */
void uart_proc(void)
{
    // 调用自定义的printf函数，将浮点数和整数格式化后通过UART1发送。
    // `vol_amp2/2*10` 是反馈值，`output` 是控制器输出值。
    my_printf(&huart1, "%f,%d\r\n", vol_amp2 / 2 * 10, output);
}

/**
 * @brief  静态任务列表
 * @note   这是调度器的核心任务清单。所有需要被调度的任务都在此数组中定义。
 * 每一行代表一个任务，格式为：{ 任务函数名, 执行周期(ms), 初始上次运行时间 }
 * 通过注释或取消注释某一行，可以方便地启用或禁用对应的任务。
 */
static task_t scheduler_task[] =
    {
        //  { function,   rate_ms, last_run }
//				{freq_proc,1000,0},
//        {ad_proc, 1, 0},   // ADC采样处理任务，每1ms执行一次
        {key_proc, 10, 0} // 按键扫描处理任务，每10ms执行一次        

				//{AD9959_Modulation_State_Update, 5, 0}, // 调制状态机更新任务，每5ms执行一次
};

/**
 * @brief  调度器初始化函数
 * @details 此函数计算在任务列表中定义的任务总数。必须在系统启动时调用一次，
 * 先于 `scheduler_run()` 的首次调用。
 * @param  None
 * @retval None
 */
void scheduler_init(void)
{
    // 通过 `sizeof` 运算符计算出数组中的元素个数，并存入全局变量 `task_num`。
    // `sizeof(scheduler_task)` 是整个数组的总字节数。
    // `sizeof(task_t)` 是单个结构体元素的字节数。
    task_num = sizeof(scheduler_task) / sizeof(task_t);
}

/**
 * @brief  调度器主运行函数
 * @details 这是调度器的核心执行函数，需要被放置在`main`函数的`while(1)`主循环中
 * 不断地被调用。它会遍历整个任务列表，检查每个任务是否到达了执行时间。
 * @param  None
 * @retval None
 */
void scheduler_run(void)
{
    // 遍历任务列表中的每一个任务。
    for (uint8_t i = 0; i < task_num; i++)
    {
        // 获取当前系统的时间戳（从系统启动开始的毫秒数）。
        uint32_t now_time = HAL_GetTick();

        // 判断当前任务是否到达执行时间。
        // 判定条件为：当前时间 >= 上次运行时间 + 任务周期。
        // 这个判断方式可以处理 HAL_GetTick() 计时器回绕（溢出）的情况。
        if (now_time - scheduler_task[i].last_run >= scheduler_task[i].rate_ms)
        {
            // 更新任务的上次运行时间为当前时间，为下一次调度做准备。
            scheduler_task[i].last_run = now_time;

            // 通过函数指针调用对应的任务函数。
            scheduler_task[i].task_func();
        }
    }
}
