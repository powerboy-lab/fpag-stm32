//&----------------------------------------------------------------------------------------
//& 模块名: DA_WAVEFORM
//& 文件名: DA_WAVEFORM.v
//& 作  者: 左岚
//& 日  期: 2025-07-20
//&
//& 功  能: 通用型单通道波形发生器模块。
//&         根据输入的波形选择码，从对应的ROM中读取数据，并通过两级流水线输出。
//&
//& 设计说明:
//& 1. 本模块为全同步设计，可被多次实例化以支持多通道。
//& 2. 使用参数定义波形选择码，提高了代码的可读性和可维护性。
//& 3. 修正了流水线逻辑，确保两级流水线能有效改善时序。
//&----------------------------------------------------------------------------------------
module DA_WAVEFORM #(
    // --- 参数定义: 使用参数替代魔术数字 ---
    parameter SINE_WAVE     = 8'd0,
    parameter SQUARE_WAVE   = 8'd1,
    parameter TRIANGLE_WAVE = 8'd2,
    parameter SAWTOOTH_WAVE = 8'd3
) (
    // --- 端口定义 (清晰的单通道接口) ---
    input         CLK,         // 系统主时钟
    input  [ 7:0] WAVE_SEL,    // 8位波形选择码输入
    input  [ 7:0] PHASE_ADDR,  // 8位的ROM相位地址输入
    output [13:0] WAVE_OUT     // 14位波形数据输出
);

  // --- 内部信号定义 ---
  wire [13:0] sin_data;  // 来自正弦波ROM的数据
  wire [13:0] tri_data;  // 来自三角波ROM的数据
  wire [13:0] squ_data;  // 来自方波ROM的数据
  wire [13:0] swt_data;  // 来自锯齿波ROM的数据

  // --- 修正流水线逻辑 ---
  // 第0级: 组合逻辑多路选择器 (Mux)
  reg  [13:0] selected_wave_data;
  // 第1级和第2级: 时序逻辑流水线寄存器
  reg  [13:0] wave_data_pipe1;
  reg  [13:0] wave_data_pipe2;  // 最终输出寄存器

  // --- 波形ROM实例化 ---
  // 每个ROM根据相同的相位地址输入，输出对应波形的采样点值
  sin_rom sin_rom_inst (
      .address(PHASE_ADDR),
      .clock  (CLK),
      .q      (sin_data)
  );

  sqaure_rom sqaure_rom_inst (
      .address(PHASE_ADDR),
      .clock  (CLK),
      .q      (squ_data)
  );

  triangle_rom triangle_rom_inst (
      .address(PHASE_ADDR),
      .clock  (CLK),
      .q      (tri_data)
  );

  sawtooth_rom sawtooth_rom_inst (
      .address(PHASE_ADDR),
      .clock  (CLK),
      .q      (swt_data)
  );

  // --- 组合逻辑: 波形选择MUX ---
  // 根据选择码，在ROM数据就绪后，立即选出当前周期的波形数据
  always @(*) begin
    case (WAVE_SEL)
      SQUARE_WAVE:   selected_wave_data = squ_data;
      TRIANGLE_WAVE: selected_wave_data = tri_data;
      SAWTOOTH_WAVE: selected_wave_data = swt_data;
      SINE_WAVE:     selected_wave_data = sin_data;  // 默认正弦波
      default:       selected_wave_data = sin_data;  // 默认正弦波
    endcase
  end

  // --- 时序逻辑: 两级流水线寄存 ---
  // 在时钟上升沿，将数据逐级送入流水线
  always @(posedge CLK) begin
    wave_data_pipe1 <= selected_wave_data;
    wave_data_pipe2 <= wave_data_pipe1;
  end

  // --- 最终输出赋值 ---
  assign WAVE_OUT = wave_data_pipe2;

endmodule
