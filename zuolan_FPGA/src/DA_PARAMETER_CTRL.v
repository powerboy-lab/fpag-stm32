//&----------------------------------------------------------------------------------------
//& 模块名: DA_PARAMETER_CTRL
//& 文件名: DA_PARAMETER_CTRL.v
//& 作 者: 左岚
//& 日 期: 2025-07-18
//&
//& 功 能: DA参数控制模块。此模块是一个复杂的双通道信号发生器。它包含两个部分：
//&     1. 一个双通道数字控制振荡器(NCO)，用于生成两个可编程频率的方波时钟。
//&     2. 一个相位控制计数器级，它使用NCO生成的时钟，并根据外部设定的相位
//&      值进行计数和相位同步调整。
//&
//& 设计警告:
//& 1. **严重警告 - 门控时钟 (Gated Clock)**:
//&  本模块中的计数器(CNT_A, CNT_B)使用了NCO的输出(FREQ_OUT_A_FINAL, FREQ_OUT_B_FINAL)
//&  作为时钟信号 (e.g., `always @(posedge FREQ_OUT_A_FINAL)`).
//&  这是一种非常危险的设计实践，称为门控时钟。它会导致时钟偏斜、毛刺和严重的
//&  时序收敛问题，应极力避免。
//&  **推荐的修改方法**: 使用系统的主时钟(CLK_BASE)作为所有always块的时钟，
//&  并将NCO的输出作为时钟使能信号(Clock Enable)来控制计数器。
//&
//& 2. **逻辑复杂性**:
//&  模块的相位同步逻辑非常复杂，特别是当`flag_reg`为高时，输出会根据一个
//&  组合逻辑的结果进行切换，需要仔细进行时序分析以确保其稳定性。
//&
//& 修改说明 (2025-07-20):
//& - 增加了当 EN=0 时，所有内部状态和输出清零的功能。
//& - 对NCO部分使用同步清零，对门控时钟驱动的计数器部分使用异步清零。
//&
//& 修改说明 (2025-07-28 - Gemini):
//& - 增加了 PHASE_WIDTH 参数，用于自定义相位和计数器的位宽。
//& - 修正了 COUT_A_FINAL 的赋值为非阻塞赋值 (<=)。
//&----------------------------------------------------------------------------------------

module DA_PARAMETER_CTRL #(
    parameter ADDR10 = 16'h000A,
    parameter ADDR11 = 16'h000B,
    parameter PHASE_WIDTH = 8  // New parameter for customizable phase/counter width
) (
    // --- 端口定义 ---
    input                         CLK_BASE,          // 系统主时钟 (用于NCO)
    input                         EN,                // NCO 全局使能
    // -- 频率控制字输入
    input       [           15:0] FREQAH_W,          // 通道A 频率字高16位
    input       [           15:0] FREQAL_W,          // 通道A 频率字低16位
    input       [           15:0] FREQBH_W,          // 通道B 频率字高16位
    input       [           15:0] FREQBL_W,          // 通道B 频率字低16位
    // -- 相位控制输入 (Parameterized Width)
    input       [           15:0] PHASEA_IN,         // 通道A 目标相位值
    input       [           15:0] PHASEB_IN,         // 通道B 目标相位值
    // -- 总线写控制
    input                         CS,                // 片选信号，低有效
    input                         WR_EN,             // 写使能，高有效
    input       [           15:0] ADDR,              // 地址总线
    // -- 最终输出 (Parameterized Width)
    output reg  [PHASE_WIDTH-1:0] COUT_A_FINAL,      // 通道A 最终的相位计数器输出
    output reg  [PHASE_WIDTH-1:0] COUT_B_FINAL,      // 通道B 最终的相位计数器输出
    // -- NCO生成的时钟输出 (内部用作门控时钟)
    output wire                   FREQ_OUT_A_FINAL,
    output wire                   FREQ_OUT_B_FINAL
);

  //==================================================================================
  //== 第一部分: 双通道NCO频率合成器
  //==================================================================================

  // --- 通道 A NCO ---
  reg [31:0] FREQ_WORD_A;  // 通道A 32位频率控制字
  reg [31:0] ACC_A = 32'd0;  // 通道A 32位相位累加器
  reg        FREQ_OUT_A;  // 通道A 原始方波输出 (累加器最高位)

  // 通道A 频率字寄存器
  always @(posedge CLK_BASE) begin
    FREQ_WORD_A <= {FREQAH_W, FREQAL_W};
  end

  // 通道A 相位累加器
  always @(posedge CLK_BASE) begin
    if (EN) begin
      ACC_A <= ACC_A + FREQ_WORD_A;
      FREQ_OUT_A <= ACC_A[31];
    end else begin  // 修改点: 当 EN 为 0 时, 同步清零NCO_A的状态
      ACC_A <= 32'd0;
      FREQ_OUT_A <= 1'b0;
    end
  end

  // --- 通道 B NCO 与 相位接近检测 ---
  reg [31:0] FREQ_WORD_B;  // 通道B 32位频率控制字
  reg [31:0] ACC_B = 32'd0;  // 通道B 32位相位累加器
  reg        FREQ_OUT_B;  // 通道B 原始方波输出

  reg        flag;  // 瞬时的相位接近标志
  reg        flag_reg;  // 寄存后的相位接近标志，用于同步

  // 通道B 频率字寄存器
  always @(posedge CLK_BASE) begin
    FREQ_WORD_B <= {FREQBH_W, FREQBL_W};
  end

  // 通道B 相位累加器 和 相位接近检测逻辑
  always @(posedge CLK_BASE) begin
    if (EN) begin
      ACC_B <= ACC_B + FREQ_WORD_B;

      // 相位接近检测: 检查两个累加器的差的绝对值是否小于等于3
      if( ((ACC_B-ACC_A>=0)&&(ACC_B-ACC_A<=3))||((ACC_A-ACC_B>=0)&&(ACC_A-ACC_B<=3)))
        flag <= 1'b1;  // 如果两个NCO的相位非常接近，则置位标志
      else flag <= 1'b0;

      // 将标志寄存一拍，用于下游逻辑同步
      flag_reg   <= flag;
      FREQ_OUT_B <= ACC_B[31];
    end else begin  // 修改点: 当 EN 为 0 时, 同步清零NCO_B的状态和标志位
      ACC_B <= 32'd0;
      flag <= 1'b0;
      flag_reg <= 1'b0;
      FREQ_OUT_B <= 1'b0;
    end
  end

  // NCO最终输出信号
  assign FREQ_OUT_A_FINAL = FREQ_OUT_A;
  assign FREQ_OUT_B_FINAL = (ACC_B == ACC_A) ? FREQ_OUT_A : FREQ_OUT_B;

  //==================================================================================
  //== 第二部分: 门控时钟驱动的计数器和相位控制
  //== **警告**: 下面的 always 块使用了门控时钟，这是非常危险的设计实践。
  //==================================================================================
  reg [PHASE_WIDTH-1:0] CNT_A;  // 通道A的自由运行计数器 (Parameterized)
  reg [PHASE_WIDTH-1:0] CNT_B;  // 通道B的自由运行计数器 (Parameterized)

  // --- 通道 A 计数器 ---
  // 在NCO A输出的上升沿触发
  // 修改点: 增加了 negedge EN 作为异步复位信号
  always @(posedge FREQ_OUT_A_FINAL or negedge EN) begin
    if (!EN) begin  // 修改点: 当 EN 为 0 时, 异步清零计数器和输出
      CNT_A <= 'd0;
      COUT_A_FINAL <= 'd0;
    end else begin  // 修改点: EN 为 1 时执行原逻辑
      if (!CS && WR_EN) begin  // 如果总线正在进行写操作
        case (ADDR)
          ADDR10:
          CNT_A <= PHASEA_IN;  // 在地址ADDR10写入时，将计数器A加载为目标相位值
          ADDR11: CNT_A <= CNT_A;  // 在地址ADDR11写入时，保持不变
          default: CNT_A <= CNT_A + 1;  // 其他地址写入时，计数器加1
        endcase
      end else begin  // 如果没有总线写操作
        CNT_A <= CNT_A + 1;  // 计数器自由加1
      end
      // 最终输出逻辑: 当检测到相位接近时，切换到交叉耦合的调整值COUT_A，否则使用本地计数值CNT_A
      // Gemini: Corrected to non-blocking assignment for proper sequential logic.
      COUT_A_FINAL <= (flag_reg == 1'b1) ? COUT_A : CNT_A;
    end
  end

  // --- 通道 B 计数器 ---
  // 在NCO B输出的上升沿触发
  // 修改点: 增加了 negedge EN 作为异步复位信号
  always @(posedge FREQ_OUT_B_FINAL or negedge EN) begin
    if (!EN) begin  // 修改点: 当 EN 为 0 时, 异步清零计数器和输出
      CNT_B <= 'd0;
      COUT_B_FINAL <= 'd0;
    end else begin  // 修改点: EN 为 1 时执行原逻辑
      if (!CS && WR_EN) begin  // 如果总线正在进行写操作
        case (ADDR)
          ADDR11:
          CNT_B <= PHASEB_IN;  // 在地址ADDR11写入时，将计数器B加载为目标相位值
          ADDR10: CNT_B <= CNT_B;  // 在地址ADDR10写入时，保持不变
          default: CNT_B <= CNT_B + 1;  // 其他地址写入时，计数器加1
        endcase
      end else begin  // 如果没有总线写操作
        CNT_B <= CNT_B + 1;  // 计数器自由加1
      end
      // 最终输出逻辑: 当检测到相位接近时，切换到交叉耦合的调整值COUT_B，否则使用本地计数值CNT_B
      COUT_B_FINAL <= (flag_reg == 1'b1) ? COUT_B : CNT_B;
    end
  end

  //==================================================================================
  //== 第三部分: 最终输出的交叉耦合组合逻辑
  //== 功能: 根据外部设定的相位差，计算出用于同步的调整值
  //==================================================================================
  wire [PHASE_WIDTH-1:0] COUT_A;
  wire [PHASE_WIDTH-1:0] COUT_B;

  // 如果PHASEA > PHASEB，则COUT_A在CNT_A基础上增加相位差，COUT_B跟随CNT_A
  // 如果PHASEA <= PHASEB，则COUT_A跟随CNT_B
  assign COUT_A = ((PHASEA_IN > PHASEB_IN) ? (CNT_A + (PHASEA_IN - PHASEB_IN)) : (CNT_B));

  // 如果PHASEA > PHASEB，则COUT_B跟随CNT_A
  // 如果PHASEA <= PHASEB，则COUT_B在CNT_B基础上增加相位差
  assign COUT_B = ((PHASEA_IN > PHASEB_IN) ? CNT_A : (CNT_B + (PHASEB_IN - PHASEA_IN)));

endmodule
