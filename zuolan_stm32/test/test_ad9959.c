#include "AD9959.h"
void main()
{
    AD9959_Init();
    // 这里进行幅度的信息输入，下面函数参数为
    // 通道，频率，初始相位，幅度
    AD9959_Single_Output(0, 1000, 0, 500);
    AD9959_Single_Output(1, 1000, 0, 500);
    AD9959_Single_Output(2, 1000, 0, 500);
    AD9959_Single_Output(3, 1000, 0, 500);
    AD9959_IO_UpDate(); // 这段代码在你每次进行赋值修改后都要打过去
}