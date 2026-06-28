/************************************************************************************
 * @file    : FMC_CONTROL.v
 * @author  : 左岚
 * @date    : 2025-07-18
 * @version : V1.0
 * @brief   : FMC（Flexible Memory Controller）接口控制模块。
 *             该模块实现了FPGA与STM32之间通过FMC总线的双向数据通信。
 *             STM32作为主机，FPGA作为从机。
 *             模块功能：
 *             1. 锁存STM32发送的地址。
 *             2. 根据地址，将STM32写入的数据存入FPGA内部对应的寄存器中。
 *             3. 根据地址，将FPGA内部的数据发送到数据总线上，供STM32读取。
 ************************************************************************************/

module FMC_CONTROL (
    // ---------------- 系统信号 ----------------
    input clk,  // 系统时钟
    input rst,  // 复位信号，低电平有效

    // ---------------- FMC 接口信号 (来自STM32) ----------------
    input           fpga_nl_nadv,   // 地址锁存使能/地址有效信号 (Address Valid)，低电平有效。用于区分地址和数据阶段。
    input fpga_cs_ne1,  // 片选信号 (Chip Select)，低电平有效。
    input fpga_wr_nwe,  // 写使能信号 (Write Enable)，低电平有效。
    input fpga_rd_noe,  // 读使能/输出使能信号 (Output Enable)，低电平有效。
    inout [15:0] fpga_db,  // 16位双向地址/数据总线

    // ---------------- 数据通道：FPGA -> STM32 ----------------
    // 这些是FPGA内部逻辑准备好，希望通过FMC发送给STM32的数据。
    input [15:0] write_data_0_,   // 通道0，FPGA发送给STM32的数据
    input [15:0] write_data_1_,   // 通道1，FPGA发送给STM32的数据
    input [15:0] write_data_2_,   // ...
    input [15:0] write_data_3_,
    input [15:0] write_data_4_,
    input [15:0] write_data_5_,
    input [15:0] write_data_6_,
    input [15:0] write_data_7_,
    input [15:0] write_data_8_,
    input [15:0] write_data_9_,
    input [15:0] write_data_10_,
    input [15:0] write_data_11_,
    input [15:0] write_data_12_,
    input [15:0] write_data_13_,
    input [15:0] write_data_14_,
    input [15:0] write_data_15_,

    // ---------------- 数据通道：STM32 -> FPGA ----------------
    // 这些是从STM32接收到，并锁存在FPGA内部，供其他逻辑使用的数据。
    output [15:0] read_data_0_,   // 通道0，FPGA从STM32接收的数据
    output [15:0] read_data_1_,   // 通道1，FPGA从STM32接收的数据
    output [15:0] read_data_2_,   // ...
    output [15:0] read_data_3_,
    output [15:0] read_data_4_,
    output [15:0] read_data_5_,
    output [15:0] read_data_6_,
    output [15:0] read_data_7_,
    output [15:0] read_data_8_,
    output [15:0] read_data_9_,
    output [15:0] read_data_10_,
    output [15:0] read_data_11_,
    output [15:0] read_data_12_,
    output [15:0] read_data_13_,
    output [15:0] read_data_14_,
    output [15:0] read_data_15_,

    // ---------------- 调试/状态信号 ----------------
    output [15:0] addr,       // 锁存后的地址值
    output        fmc_wr_en,  // 内部生成的写使能标志
    output        fmc_rd_en   // 内部生成的读使能标志
);

  //==================================================================================
  // 1. 内部寄存器定义
  //==================================================================================

  // 定义16个16位的寄存器，用于存储从STM32写入FPGA的数据。
  // 每个寄存器对应一个地址通道。
  reg [15:0] read_data_0__reg;
  reg [15:0] read_data_1__reg;
  reg [15:0] read_data_2__reg;
  reg [15:0] read_data_3__reg;
  reg [15:0] read_data_4__reg;
  reg [15:0] read_data_5__reg;
  reg [15:0] read_data_6__reg;
  reg [15:0] read_data_7__reg;
  reg [15:0] read_data_8__reg;
  reg [15:0] read_data_9__reg;
  reg [15:0] read_data_10__reg;
  reg [15:0] read_data_11__reg;
  reg [15:0] read_data_12__reg;
  reg [15:0] read_data_13__reg;
  reg [15:0] read_data_14__reg;
  reg [15:0] read_data_15__reg;

  // 定义一个16位的寄存器，用于缓存即将通过fpga_db总线发送给STM32的数据。
  reg [15:0] rd_data_reg;

  //==================================================================================
  // 2. 信号连接与组合逻辑
  //==================================================================================

  // 将内部寄存器连接到模块的输出端口。
  assign read_data_0_ = read_data_0__reg;
  assign read_data_1_ = read_data_1__reg;
  assign read_data_2_ = read_data_2__reg;
  assign read_data_3_ = read_data_3__reg;
  assign read_data_4_ = read_data_4__reg;
  assign read_data_5_ = read_data_5__reg;
  assign read_data_6_ = read_data_6__reg;
  assign read_data_7_ = read_data_7__reg;
  assign read_data_8_ = read_data_8__reg;
  assign read_data_9_ = read_data_9__reg;
  assign read_data_10_ = read_data_10__reg;
  assign read_data_11_ = read_data_11__reg;
  assign read_data_12_ = read_data_12__reg;
  assign read_data_13_ = read_data_13__reg;
  assign read_data_14_ = read_data_14__reg;
  assign read_data_15_ = read_data_15__reg;

  // 生成内部写使能信号 fmc_wr_en。
  // 当片选有效(低)、写使能有效(低)并且地址锁存无效(高，表示当前是数据阶段)时，
  // 判定为一次有效的写操作。
  assign fmc_wr_en = ((!fpga_cs_ne1) & (!fpga_wr_nwe) & fpga_nl_nadv);

  // 生成内部读使能信号 fmc_rd_en。
  // 当片选有效(低)、读使能有效(低)并且地址锁存无效(高，表示当前是数据阶段)时，
  // 判定为一次有效的读操作。
  assign fmc_rd_en = ((!fpga_cs_ne1) & (!fpga_rd_noe) & fpga_nl_nadv);

  // 控制双向数据总线 fpga_db。
  // 当读使能有效时 (fmc_rd_en == 1)，FPGA驱动总线，将rd_data_reg中的数据发送出去。
  // 否则，FPGA释放总线，将其置为高阻态(16'hzzzz)，以便STM32可以驱动总线。
  assign fpga_db = fmc_rd_en ? rd_data_reg : 16'hzzzz;

  // 地址锁存逻辑。
  // 当地址有效信号fpga_nl_nadv为低电平且片选有效时，总线fpga_db上的值为地址，锁存到addr信号。
  // 否则，addr保持其值不变。
  // 注意：这会综合成一个锁存器(Latch)，在设计中应谨慎使用。
  // 确保外部时序满足建立和保持时间要求。
  assign addr = ((fpga_nl_nadv == 1'b0) & (fpga_cs_ne1 == 1'b0)) ? fpga_db : addr;

  //==================================================================================
  // 3. 时序逻辑 (核心处理)
  //==================================================================================

  // 主时序逻辑块，由时钟上升沿触发，并带有异步复位。
  always @(posedge clk or negedge rst) begin
    // ---- 复位逻辑 ----
    if (!rst) begin  // 当复位信号rst为低电平时
      // 将所有从STM32接收数据的寄存器清零。
      read_data_0__reg  <= 16'd0;
      read_data_1__reg  <= 16'd0;
      read_data_2__reg  <= 16'd0;
      read_data_3__reg  <= 16'd0;
      read_data_4__reg  <= 16'd0;
      read_data_5__reg  <= 16'd0;
      read_data_6__reg  <= 16'd0;
      read_data_7__reg  <= 16'd0;
      read_data_8__reg  <= 16'd0;
      read_data_9__reg  <= 16'd0;
      read_data_10__reg <= 16'd0;
      read_data_11__reg <= 16'd0;
      read_data_12__reg <= 16'd0;
      read_data_13__reg <= 16'd0;
      read_data_14__reg <= 16'd0;
      read_data_15__reg <= 16'd0;
    end  // ---- 写操作：STM32 -> FPGA ----
    else if (fmc_wr_en) begin
      // 当检测到有效的写操作时，根据锁存的地址addr，将总线fpga_db上的数据写入对应的寄存器。
      case (addr)
        16'h0000: read_data_0__reg <= fpga_db;  // 地址为0x0000, 写入通道0
        16'h0001: read_data_1__reg <= fpga_db;  // 地址为0x0001, 写入通道1
        16'h0002: read_data_2__reg <= fpga_db;  // ...
        16'h0003: read_data_3__reg <= fpga_db;
        16'h0004: read_data_4__reg <= fpga_db;
        16'h0005: read_data_5__reg <= fpga_db;
        16'h0006: read_data_6__reg <= fpga_db;
        16'h0007: read_data_7__reg <= fpga_db;
        16'h0008: read_data_8__reg <= fpga_db;
        16'h0009: read_data_9__reg <= fpga_db;
        16'h000A: read_data_10__reg <= fpga_db;
        16'h000B: read_data_11__reg <= fpga_db;
        16'h000C: read_data_12__reg <= fpga_db;
        16'h000D: read_data_13__reg <= fpga_db;
        16'h000E: read_data_14__reg <= fpga_db;
        16'h000F: read_data_15__reg <= fpga_db;  // 地址为0x000F, 写入通道15
        default:  ;  // 如果地址不匹配任何通道，则不执行任何操作。
      endcase
    end  // ---- 读操作：FPGA -> STM32 ----
    else if (fmc_rd_en) begin
      // 当检测到有效的读操作时，根据锁存的地址addr，将FPGA内部对应通道的数据加载到发送缓存rd_data_reg中。
      // rd_data_reg中的数据将在下一个周期通过组合逻辑驱动到fpga_db总线上。
      case (addr)
        16'h0000: rd_data_reg <= write_data_0_;  // 地址为0x0000, 准备发送通道0的数据
        16'h0001: rd_data_reg <= write_data_1_;  // 地址为0x0001, 准备发送通道1的数据
        16'h0002: rd_data_reg <= write_data_2_;  // ...
        16'h0003: rd_data_reg <= write_data_3_;
        16'h0004: rd_data_reg <= write_data_4_;
        16'h0005: rd_data_reg <= write_data_5_;
        16'h0006: rd_data_reg <= write_data_6_;
        16'h0007: rd_data_reg <= write_data_7_;
        16'h0008: rd_data_reg <= write_data_8_;
        16'h0009: rd_data_reg <= write_data_9_;
        16'h000A: rd_data_reg <= write_data_10_;
        16'h000B: rd_data_reg <= write_data_11_;
        16'h000C: rd_data_reg <= write_data_12_;
        16'h000D: rd_data_reg <= write_data_13_;
        16'h000E: rd_data_reg <= write_data_14_;
        16'h000F: rd_data_reg <= write_data_15_;  // 地址为0x000F, 准备发送通道15的数据
      endcase
    end
  end

endmodule
