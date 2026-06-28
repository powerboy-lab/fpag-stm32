module ADC_ZERO_CROSS #(
    parameter [11:0] MID_CODE = 12'd2048,
    parameter [11:0] HYSTERESIS = 12'd64
) (
    input             AD_CLK,
    input      [11:0] AD_DATA,
    output reg        SIG_CLK
);

  wire [11:0] ad_code;

  assign ad_code[0]  = AD_DATA[11];
  assign ad_code[1]  = AD_DATA[10];
  assign ad_code[2]  = AD_DATA[9];
  assign ad_code[3]  = AD_DATA[8];
  assign ad_code[4]  = AD_DATA[7];
  assign ad_code[5]  = AD_DATA[6];
  assign ad_code[6]  = AD_DATA[5];
  assign ad_code[7]  = AD_DATA[4];
  assign ad_code[8]  = AD_DATA[3];
  assign ad_code[9]  = AD_DATA[2];
  assign ad_code[10] = AD_DATA[1];
  assign ad_code[11] = AD_DATA[0];

  always @(posedge AD_CLK) begin
    if (ad_code > (MID_CODE + HYSTERESIS))
      SIG_CLK <= 1'b1;
    else if (ad_code < (MID_CODE - HYSTERESIS))
      SIG_CLK <= 1'b0;
  end

endmodule
