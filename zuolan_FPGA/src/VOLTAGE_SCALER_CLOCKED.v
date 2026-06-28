//&----------------------------------------------------------------------------------------
//& 模块名: VOLTAGE_SCALER_CLOCKED
//& 文件名: VOLTAGE_SCALER_CLOCKED.v
//& 作  者: 左岚
//& 日  期: 2025年7月18日
//&
//& 功  能: 同步电压幅度缩放模块。
//&         本模块的功能是接收一个标准的14位波形数据(通常来自ROM)，并根据一个
//&         可编程的目标峰值电压，对其幅度进行线性缩放。它假设输入波形数据是以
//&         一个直流偏置(HALF_ROM_MAX)为中心的。
//&----------------------------------------------------------------------------------------

module  VOLTAGE_SCALER_CLOCKED(
    // --- 端口定义 ---
    input wire clk,  // 时钟输入
    input wire [13:0] rom_data,  // 14位原始波形数据输入 (范围: 0 - 16383)
    input wire [11:0] voltage_mv, // 12位目标峰值电压（单位:毫伏），例如1550表示1.55V，上限3000mV

    output reg [13:0] scaled_data  // 14位幅度缩放后的波形数据输出
);

  // --- 参数定义 ---
  // ROM数据的最大值 (2^14 - 1)
  parameter ROM_MAX = 16383;
  // ROM数据对应的参考峰值电压 (单位:毫伏)。这是计算缩放比例时的分母。
  // 例如，当ROM数据达到最大幅度时，对应的电压被定义为3080mV (3.08V)。
  parameter DEFAULT_PEAK_MV = 3080;
  // ROM数据的中心值或直流偏置值 (16383 / 2 = 8191)。
  // 波形的正半轴在 [8191, 16383] 区间，负半轴在 [0, 8191] 区间。
  parameter HALF_ROM_MAX = ROM_MAX / 2;

  // --- 幅度缩放逻辑 ---
  // 在每个时钟的上升沿，对输入的ROM数据进行缩放并寄存输出
  always @(posedge clk) begin
    // 判断数据点在波形的正半轴还是负半轴
    if (rom_data >= HALF_ROM_MAX) begin
      // --- 处理正半轴 ---
      // 1. (rom_data - HALF_ROM_MAX): 计算出原始波形距离中心点的正向幅度。
      // 2. (... * voltage_mv / DEFAULT_PEAK_MV): 根据目标电压与参考电压的比例，对幅度进行缩放。
      // 3. HALF_ROM_MAX + (...): 将缩放后的幅度重新加到中心值上，得到最终输出值。
      scaled_data <= HALF_ROM_MAX + (rom_data - HALF_ROM_MAX) * voltage_mv / DEFAULT_PEAK_MV;
    end else begin
      // --- 处理负半轴 ---
      // 1. (HALF_ROM_MAX - rom_data): 计算出原始波形距离中心点的负向幅度。
      // 2. (... * voltage_mv / DEFAULT_PEAK_MV): 对该幅度进行缩放。
      // 3. HALF_ROM_MAX - (...): 从中心值减去缩放后的幅度，得到最终输出值。
      scaled_data <= HALF_ROM_MAX - (HALF_ROM_MAX - rom_data) * voltage_mv / DEFAULT_PEAK_MV;
    end
  end

endmodule
