//&----------------------------------------------------------------------------------------
//& 模块名: AD_DATA_DEAL
//& 文件名: AD_DATA_DEAL.v
//& 作  者: 左岚
//& 日  期: 2025-07-18
//&
//& 功  能: AD数据处理模块。此模块作为一个总线外设，根据传入的地址（ADDR），
//&         当片选（CS）和读使能（RD_EN）有效时，将两个ADC FIFO的数据和状态标志位
//&         输出到16位数据总线上。其中，ADC数据会进行位序反转处理。
//&----------------------------------------------------------------------------------------

module AD_DATA_DEAL #(
    // --- 参数定义 ---
    parameter ADDR6 = 16'h0006,  // 读取AD1 FIFO数据的地址
    parameter ADDR7 = 16'h0007,  // 读取AD1 FIFO标志位的地址
    parameter ADDR8 = 16'h0008,  // 读取AD2 FIFO数据的地址
    parameter ADDR9 = 16'h0009   // 读取AD2 FIFO标志位的地址
) (
    // --- 端口定义 ---
    // -- 控制信号
    input             CS,                 // 片选信号，低电平有效
    input             RD_EN,              // 读使能信号，高电平有效
    // -- AD通道1输入
    input             AD1_FLAG,           // AD1 FIFO非空标志位 (1: 非空, 0: 空)
    input      [11:0] AD1_FIFO_DATA_IN,   // 从AD1 FIFO读出的12位数据
    // -- AD通道2输入
    input             AD2_FLAG,           // AD2 FIFO非空标志位 (1: 非空, 0: 空)
    input      [11:0] AD2_FIFO_DATA_IN,   // 从AD2 FIFO读出的12位数据
    // -- 总线接口
    input      [15:0] ADDR,               // 16位地址总线
    // -- 总线输出
    output reg [15:0] AD1_FLAG_SHOW,      // 输出到总线的AD1标志位状态
    output reg [15:0] AD2_FLAG_SHOW,      // 输出到总线的AD2标志位状态
    output reg [15:0] AD1_FIFO_DATA_OUT,  // 输出到总线的AD1数据
    output reg [15:0] AD2_FIFO_DATA_OUT   // 输出到总线的AD2数据
);

  // --- 内部信号定义 ---
  integer i;  // for循环中使用的循环变量
  reg [11:0] ad1_fifo_recv, ad2_fifo_recv;  // 用于暂存位序反转后的ADC数据

  // --- 逻辑实现 ---
  // 组合逻辑块，当任意输入信号变化时执行
  always @(*) begin
    // 当片选有效(CS为低)且读使能有效(RD_EN为高)时，执行总线读操作
    if (!CS && RD_EN) begin
      // 根据地址ADDR的值，选择不同的数据源输出到总线
      case (ADDR)
        // 地址为ADDR6时，读取AD1的数据
        ADDR6: begin
          // 使用for循环将输入的12位数据AD1_FIFO_DATA_IN进行位序反转
          // 例如: AD1_FIFO_DATA_IN[11] -> ad1_fifo_recv[0]
          //       AD1_FIFO_DATA_IN[10] -> ad1_fifo_recv[1]
          //       ...
          //       AD1_FIFO_DATA_IN[0]  -> ad1_fifo_recv[11]
          for (i = 0; i < 12; i = i + 1) begin
            ad1_fifo_recv[i] = AD1_FIFO_DATA_IN[11-i];
          end
          // 将反转后的12位数据ad1_fifo_recv与4位'b0000拼接，形成16位数据输出
          // 高4位补零，低12位为有效数据
          AD1_FIFO_DATA_OUT = {4'b0000, ad1_fifo_recv};
        end

        // 地址为ADDR7时，读取AD1的标志位
        ADDR7:
        AD1_FLAG_SHOW = (AD1_FLAG) ? 16'h0001 : 16'h0000; // 如果AD1_FLAG为真(1)，则输出1，否则输出0

        // 地址为ADDR8时，读取AD2的数据
        ADDR8: begin
          // 同样地，对AD2_FIFO_DATA_IN进行位序反转
          for (i = 0; i < 12; i = i + 1) begin
            ad2_fifo_recv[i] = AD2_FIFO_DATA_IN[11-i];
          end
          // 将反转后的12位数据与4位'b0000拼接，形成16位数据输出
          AD2_FIFO_DATA_OUT = {4'b0000, ad2_fifo_recv};
        end

        // 地址为ADDR9时，读取AD2的标志位
        ADDR9:
        AD2_FLAG_SHOW = (AD2_FLAG) ? 16'h0001 : 16'h0000; // 如果AD2_FLAG为真(1)，则输出1，否则输出0

        // default: ; // 在实际设计中，可以考虑为不匹配的地址提供一个默认输出
      endcase
    end
    // else begin
    //    // 在实际设计中，当读操作无效时，通常会将输出置为高阻态或保持上一个值，
    //    // 但由于这里是组合逻辑，不满足条件时输出会保持不变。
    // end
  end

endmodule
