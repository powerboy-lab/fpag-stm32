module CNT32 (
    input         CLR,
    input         CLK,
    input         CLKBASE,
    input         CLKEN,
    input         CLKBASEEN,
    output [31:0] Q,
    output [31:0] QBASE
);

  reg [31:0] pulse_count;
  reg [31:0] base_count;
  reg [2:0]  clk_sync;
  reg [1:0]  clken_sync;
  reg [1:0]  clkbaseen_sync;

  wire clk_rising;

  assign clk_rising = (clk_sync[2:1] == 2'b01);
  assign Q = pulse_count;
  assign QBASE = base_count;

  always @(posedge CLKBASE or negedge CLR) begin
    if (!CLR) begin
      pulse_count   <= 32'd0;
      base_count    <= 32'd0;
      clk_sync      <= 3'd0;
      clken_sync    <= 2'd0;
      clkbaseen_sync <= 2'd0;
    end else begin
      clk_sync       <= {clk_sync[1:0], CLK};
      clken_sync     <= {clken_sync[0], CLKEN};
      clkbaseen_sync <= {clkbaseen_sync[0], CLKBASEEN};

      if (clken_sync[1] && clk_rising)
        pulse_count <= pulse_count + 1'b1;

      if (clkbaseen_sync[1])
        base_count <= base_count + 1'b1;
    end
  end

endmodule
