module DA_PARAMETER_DEAL #(
  // --- 参数定义 (地址映射) ---
  parameter DA1_FREQ_H_ADDR2 = 16'h0002,
  parameter DA1_FREQ_L_ADDR3 = 16'h0003,
  parameter DA2_FREQ_H_ADDR4 = 16'h0004,
  parameter DA2_FREQ_L_ADDR5 = 16'h0005,
  parameter WAVE_SEL_ADDR12 = 16'h000C,
  parameter DA_STEP_ADDR13 = 16'h000D,
  parameter DA1_AMP_ADDR14 = 16'h000E,
  parameter DA2_AMP_ADDR15 = 16'h000F
) (
  // --- 端口定义 ---
  // -- 总线控制信号
  input      CS,
  input      WR_EN,
  // -- 数据输入
  input   [15:0] DA1_FREQ_H,
  input   [15:0] DA1_FREQ_L,
  input   [15:0] DA2_FREQ_H,
  input   [15:0] DA2_FREQ_L,
  input   [15:0] WAVE_SEL_IN,
  input   [15:0] DA_STEP_IN,
  input   [15:0] DA1_AMP_IN,
  input   [15:0] DA2_AMP_IN,
  // -- 总线地址输入
  input   [15:0] ADDR,
  // -- 寄存器输出
  output reg [15:0] DA1_OUTH,
  output reg [15:0] DA1_OUTL,
  output reg [15:0] DA2_OUTH,
  output reg [15:0] DA2_OUTL,
  output reg [11:0] DA1_AMP_OUT,
  output reg [11:0] DA2_AMP_OUT,
  output reg [ 7:0] DA1_STEP_OUT,
  output reg [ 7:0] DA2_STEP_OUT,
  output reg [ 7:0] DA1_WAVE_OUT,
  output reg [ 7:0] DA2_WAVE_OUT
);

 // --- 逻辑实现 (锁存器) ---
 // Register DA parameters on the FMC write pulse.
 always @(posedge WR_EN) begin
  if (!CS) begin
   case (ADDR)
    DA1_FREQ_H_ADDR2: DA1_OUTH <= DA1_FREQ_H;
    DA1_FREQ_L_ADDR3: DA1_OUTL <= DA1_FREQ_L;
    DA2_FREQ_H_ADDR4: DA2_OUTH <= DA2_FREQ_H;
    DA2_FREQ_L_ADDR5: DA2_OUTL <= DA2_FREQ_L;
    DA1_AMP_ADDR14: DA1_AMP_OUT <= DA1_AMP_IN[11:0];
    DA2_AMP_ADDR15: DA2_AMP_OUT <= DA2_AMP_IN[11:0];
    DA_STEP_ADDR13: begin
     DA1_STEP_OUT <= DA_STEP_IN[15:8];
     DA2_STEP_OUT <= DA_STEP_IN[7:0];
    end
    WAVE_SEL_ADDR12: begin
     DA1_WAVE_OUT <= WAVE_SEL_IN[15:8];
     DA2_WAVE_OUT <= WAVE_SEL_IN[7:0];
    end
   endcase
  end
 end

endmodule
