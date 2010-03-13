module gpio_sevenseg 
(
	input           clk_i, 
	output reg      [7:0] seg,
	output          [3:0] an,
    input           [15:0] gpio
);

reg [16:0] counter;
reg [3:0] led_counter;
reg [3:0] gpio_source;
reg [3:0] led_select;

initial begin
    counter = 16'h0;
    led_counter = 4'h0;
end

always @(posedge clk_i)
begin
  if (counter==16'hffff) begin
      counter <= 16'h0;
      led_counter <= led_counter + 1;
      if (led_counter == 3) begin
          led_counter <= 0;
      end
  end
  else begin
    counter <= counter + 1;
  end
end


always @(posedge clk_i)
begin
case (led_counter)
  0:
    begin 
      gpio_source <= gpio[3:0];
      led_select <= 4'b1110;
  end
  1:
    begin
      gpio_source <= gpio[7:4];
      led_select <= 4'b1101;
    end
  2:
    begin
      gpio_source <= gpio[11:8];
      led_select <= 4'b1011;
    end
  default:
    begin
      gpio_source <= gpio[15:12];
      led_select <= 4'b0111;
    end
endcase
end

assign an = led_select;

always @(posedge clk_i)
begin
case (gpio_source[3:0])
    4'h0: seg = 7'b1000000;
    4'h1: seg = 7'b1111001;
    4'h2: seg = 7'b0100100; 
    4'h3: seg = 7'b0110000; 
    4'h4: seg = 7'b0011001; 
    4'h5: seg = 7'b0010010; 
    4'h6: seg = 7'b0000010; 
    4'h7: seg = 7'b1111000; 
    4'h8: seg = 7'b0000000; 
    4'h9: seg = 7'b0010000; 
    4'ha: seg = 7'b0001000; 
    4'hb: seg = 7'b0000011; 
    4'hc: seg = 7'b1000110;
    4'hd: seg = 7'b0100001; 
    4'he: seg = 7'b0000110; 
    default: seg = 7'b0001110;
endcase     
end
endmodule

