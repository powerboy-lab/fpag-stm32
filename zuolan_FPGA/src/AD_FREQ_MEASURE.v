//&----------------------------------------------------------------------------------------
//& 模块名: AD_FREQ_MEASURE
//& 文件名: AD_FREQ_MEASURE.v
//& 作  者: 左岚
//& 日  期: 2025-07-18
//&
//& 功  能: AD频率测量数据显示模块。该模块将4个32位的频率测量值（2个ADC通道，2个基准通道）
//&         映射到不同的地址上。当外部总线进行读操作时，它根据地址将相应的32位数据
//&         拆分成高16位和低16位，分两次提供给16位的数据总线。
//&----------------------------------------------------------------------------------------

module AD_FREQ_MEASURE #(
    // --- 参数定义 (地址映射) ---
    parameter ADDR2 = 16'h0002,  // 基准频率1 高16位数据地址
    parameter ADDR3 = 16'h0003,  // 基准频率1 低16位数据地址
    parameter ADDR4 = 16'h0004,  // 基准频率2 高16位数据地址
    parameter ADDR5 = 16'h0005,  // 基准频率2 低16位数据地址

    parameter ADDR10 = 16'h000A,  // AD1测量频率 高16位数据地址
    parameter ADDR11 = 16'h000B,  // AD1测量频率 低16位数据地址
    parameter ADDR12 = 16'h000C,  // AD2测量频率 高16位数据地址
    parameter ADDR13 = 16'h000D   // AD2测量频率 低16位数据地址
) (
    // --- 端口定义 ---
    // -- 总线控制信号
    input             CS,                 // 片选信号，低电平有效
    input             RD,                 // 读使能信号，高电平有效
    // -- 32位频率数据输入
    input      [31:0] AD1_FREQ_DATA,      // AD1通道测得的32位频率数据
    input      [31:0] AD2_FREQ_DATA,      // AD2通道测得的32位频率数据
    input      [31:0] BASE1_FREQ_DATA,    // 基准通道1的32位频率数据
    input      [31:0] BASE2_FREQ_DATA,    // 基准通道2的32位频率数据
    // -- 总线地址输入
    input      [15:0] ADDR,               // 16位地址总线
    // -- 16位数据总线输出 (将32位数据拆分)
    output reg [15:0] AD1_FREQ_DATA_H,    // 输出到总线的AD1频率数据高16位
    output reg [15:0] AD1_FREQ_DATA_L,    // 输出到总线的AD1频率数据低16位
    output reg [15:0] AD2_FREQ_DATA_H,    // 输出到总线的AD2频率数据高16位
    output reg [15:0] AD2_FREQ_DATA_L,    // 输出到总线的AD2频率数据低16位
    output reg [15:0] BASE1_FREQ_DATA_H,  // 输出到总线的基准1频率数据高16位
    output reg [15:0] BASE1_FREQ_DATA_L,  // 输出到总线的基准1频率数据低16位
    output reg [15:0] BASE2_FREQ_DATA_H,  // 输出到总线的基准2频率数据高16位
    output reg [15:0] BASE2_FREQ_DATA_L   // 输出到总线的基准2频率数据低16位
);
  // --- 逻辑实现 ---
  // 组合逻辑块，实现地址译码和数据选择
  always @(*) begin
    // 当片选有效(CS为低)且读使能有效(RD为高)时，执行总线读操作
    if (!CS && RD) begin
      // 根据地址ADDR的值，选择性地将32位输入数据的高16位或低16位赋给对应的16位输出端口
      case (ADDR)
        // --- 基准频率1数据读取 ---
        ADDR2:  BASE1_FREQ_DATA_H = BASE1_FREQ_DATA[31:16];  // 读取高16位
        ADDR3:  BASE1_FREQ_DATA_L = BASE1_FREQ_DATA[15:0];  // 读取低16位
        // --- 基准频率2数据读取 ---
        ADDR4:  BASE2_FREQ_DATA_H = BASE2_FREQ_DATA[31:16];  // 读取高16位
        ADDR5:  BASE2_FREQ_DATA_L = BASE2_FREQ_DATA[15:0];  // 读取低16位
        // --- AD1测量频率数据读取 ---
        ADDR10: AD1_FREQ_DATA_H = AD1_FREQ_DATA[31:16];  // 读取高16位
        ADDR11: AD1_FREQ_DATA_L = AD1_FREQ_DATA[15:0];  // 读取低16位
        // --- AD2测量频率数据读取 ---
        ADDR12: AD2_FREQ_DATA_H = AD2_FREQ_DATA[31:16];  // 读取高16位
        ADDR13: AD2_FREQ_DATA_L = AD2_FREQ_DATA[15:0];  // 读取低16位
        // default: ; // 在实际设计中，可以为未定义的地址添加默认行为
      endcase
    end
  end
endmodule
