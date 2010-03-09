//---------------------------------------------------------------------------
// LatticeMico32 System On A Chip
//
// Top Level Design for the Digilent Nexsy2 Spartan 3e Board
//---------------------------------------------------------------------------


module PushButton_Debouncer (clk, pushbutton, pushbutton_state, pushbutton_up, pushbutton_down);
input clk;  
input pushbutton;  
output pushbutton_state;
output pushbutton_down;
output pushbutton_up;

// First use two flipflops to synchronize the pushbutton signal the "clk"
// clock domain
reg pushbutton_sync_0;
always @(posedge clk)
    pushbutton_sync_0 <= ~pushbutton; 

// invert pushbutton to make pushbutton_sync_0 active high
reg pushbutton_sync_1;
always @(posedge clk)
    pushbutton_sync_1 <= pushbutton_sync_0;


// Next declare a 16-bits counter
reg [15:0] pushbutton_cnt;

// When the push-button is pushed or released, we increment the counter
// The counter has to be maxed out before we decide that the push-button state has changed
reg pushbutton_state;  // state of the push-button (0 when up, 1 when down)
wire pushbutton_idle = (pushbutton_state==pushbutton_sync_1);
wire pushbutton_cnt_max = &pushbutton_cnt;  // true when all bits of pushbutton_cnt are 1's

always @(posedge clk)
if(pushbutton_idle)
    pushbutton_cnt <= 0;  // nothing's going on
else
begin
    pushbutton_cnt <= pushbutton_cnt + 1;  // something's going on, increment the counter
    if(pushbutton_cnt_max)
        pushbutton_state <= ~pushbutton_state;  // if thecounter is maxed out, pushbutton changed!
end

wire pushbutton_down = ~pushbutton_state & ~pushbutton_idle & pushbutton_cnt_max;  // true for one clock cycle when we detect that pushbutton went down
wire pushbutton_up   =  pushbutton_state & ~pushbutton_idle & pushbutton_cnt_max;  // true for one clock cycle when we detect that PB went up

endmodule


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


module system
#(
	parameter   bootram_file     = "../../firmware/boot0-serial/image.ram",
	parameter   clk_freq         = 50000000,
	parameter   uart_baud_rate   = 115200
) (
	input                   clk, 
	// Debug 
	output            [7:0] led,
	input             [3:0] btn,
	input             [7:0] sw,
	output            [6:0] seg,
	output            [3:0] an,
	// UART
	input                   uart_rxd, 
	output                  uart_txd
	// SRAM
	//output           [22:0] sram_adr,
	//inout            [15:0] sram_dat,
	//output            [1:0] sram_be_n,    // Byte   Enable
	//output                  sram_ce_n,    // Chip   Enable
	//output                  sram_oe_n,    // Output Enable
	//output                  sram_we_n,    // Write  Enable
	//output                  sram_ub,      // Upper byte Enable
	//output                  sram_lb,      // Lower byte Enable
	//output                  sram_clk,     // Clock
	//input                   sram_wait,    // Wait
	//output                  sram_cre,     // 
	//output                  sram_adv,
	//output                  flash_cs,     // Flash chip select 
	//output                  flash_rp      // Flash chip select 

);
	
wire         rst;

//------------------------------------------------------------------
// Whishbone Wires
//------------------------------------------------------------------
wire         gnd   =  1'b0;
wire   [3:0] gnd4  =  4'h0;
wire  [31:0] gnd32 = 32'h00000000;

 
wire [31:0]  lm32i_adr,
             lm32d_adr,
             uart0_adr,
             timer0_adr,
             gpio0_adr,
             bram0_adr,
             bram1_adr;


wire [31:0]  lm32i_dat_r,
             lm32i_dat_w,
             lm32d_dat_r,
             lm32d_dat_w,
             uart0_dat_r,
             uart0_dat_w,
             timer0_dat_r,
             timer0_dat_w,
             gpio0_dat_r,
             gpio0_dat_w,
             bram0_dat_r,
             bram0_dat_w,
             bram1_dat_w,
             bram1_dat_r;

wire [3:0]   lm32i_sel,
             lm32d_sel,
             uart0_sel,
             timer0_sel,
             gpio0_sel,
             bram0_sel,
             bram1_sel;

wire         lm32i_we,
             lm32d_we,
             uart0_we,
             timer0_we,
             gpio0_we,
             bram0_we,
             bram1_we;

wire         lm32i_cyc,
             lm32d_cyc,
             uart0_cyc,
             timer0_cyc,
             gpio0_cyc,
             bram0_cyc,
             bram1_cyc;

wire         lm32i_stb,
             lm32d_stb,
             uart0_stb,
             timer0_stb,
             gpio0_stb,
             bram0_stb,
             bram1_stb;

wire         lm32i_ack,
             lm32d_ack,
             uart0_ack,
             timer0_ack,
             gpio0_ack,
             bram0_ack,
             bram1_ack;

wire         lm32i_rty,
             lm32d_rty;

wire         lm32i_err,
             lm32d_err;

wire         lm32i_lock,
             lm32d_lock;

wire [2:0]   lm32i_cti,
             lm32d_cti;

wire [1:0]   lm32i_bte,
             lm32d_bte;


wire [31:0]  intr_n;
wire         uart0_intr = 0;
wire   [1:0] timer0_intr;
wire         gpio0_intr;

assign intr_n = { 28'hFFFFFFF, ~timer0_intr[1], ~gpio0_intr, ~timer0_intr[0], ~uart0_intr };

//---------------------------------------------------------------------------
// Wishbone Interconnect
//---------------------------------------------------------------------------
wb_conbus_top #(
	.s0_addr_w ( 3 ),
	.s0_addr   ( 3'h4 ),        // sram0
	.s1_addr_w ( 3 ),
	.s1_addr   ( 3'h5 ),        
	.s27_addr_w( 15 ),
	.s2_addr   ( 15'h0000 ),    // bram0 
	.s3_addr   ( 15'h7000 ),    // uart0
	.s4_addr   ( 15'h7001 ),    // timer0
	.s5_addr   ( 15'h7002 ),    // gpio0
	.s6_addr   ( 15'h7003 ),
	.s7_addr   ( 15'h7004 )
) conmax0 (
	.clk_i( clk ),
	.rst_i( rst ),
	// Master0
	.m0_dat_i(  lm32i_dat_w  ),
	.m0_dat_o(  lm32i_dat_r  ),
	.m0_adr_i(  lm32i_adr    ),
	.m0_we_i (  lm32i_we     ),
	.m0_sel_i(  lm32i_sel    ),
	.m0_cyc_i(  lm32i_cyc    ),
	.m0_stb_i(  lm32i_stb    ),
	.m0_ack_o(  lm32i_ack    ),
	.m0_rty_o(  lm32i_rty    ),
	.m0_err_o(  lm32i_err    ),
	// Master1
	.m1_dat_i(  lm32d_dat_w  ),
	.m1_dat_o(  lm32d_dat_r  ),
	.m1_adr_i(  lm32d_adr    ),
	.m1_we_i (  lm32d_we     ),
	.m1_sel_i(  lm32d_sel    ),
	.m1_cyc_i(  lm32d_cyc    ),
	.m1_stb_i(  lm32d_stb    ),
	.m1_ack_o(  lm32d_ack    ),
	.m1_rty_o(  lm32d_rty    ),
	.m1_err_o(  lm32d_err    ),
	// Master2
	.m2_dat_i(  gnd32  ),
	.m2_adr_i(  gnd32  ),
	.m2_sel_i(  gnd4   ),
	.m2_cyc_i(  gnd    ),
	.m2_stb_i(  gnd    ),
	// Master3
	.m3_dat_i(  gnd32  ),
	.m3_adr_i(  gnd32  ),
	.m3_sel_i(  gnd4   ),
	.m3_cyc_i(  gnd    ),
	.m3_stb_i(  gnd    ),
	// Master4
	.m4_dat_i(  gnd32  ),
	.m4_adr_i(  gnd32  ),
	.m4_sel_i(  gnd4   ),
	.m4_cyc_i(  gnd    ),
	.m4_stb_i(  gnd    ),
	// Master5
	.m5_dat_i(  gnd32  ),
	.m5_adr_i(  gnd32  ),
	.m5_sel_i(  gnd4   ),
	.m5_cyc_i(  gnd    ),
	.m5_stb_i(  gnd    ),
	// Master6
	.m6_dat_i(  gnd32  ),
	.m6_adr_i(  gnd32  ),
	.m6_sel_i(  gnd4   ),
	.m6_cyc_i(  gnd    ),
	.m6_stb_i(  gnd    ),
	// Master7
	.m7_dat_i(  gnd32  ),
	.m7_adr_i(  gnd32  ),
	.m7_sel_i(  gnd4   ),
	.m7_cyc_i(  gnd    ),
	.m7_stb_i(  gnd    ),

	// Slave0
	.s0_dat_i(  bram1_dat_r   ),
	.s0_dat_o(  bram1_dat_w   ),
	.s0_adr_o(  bram1_adr     ),
	.s0_sel_o(  bram1_sel     ),
	.s0_we_o(   bram1_we      ),
	.s0_cyc_o(  bram1_cyc     ),
	.s0_stb_o(  bram1_stb     ),
	.s0_ack_i(  bram1_ack     ),
	.s0_err_i(  gnd    ),
	.s0_rty_i(  gnd    ),
	// Slave1
	.s1_dat_i(  gnd32  ),
	.s1_ack_i(  gnd    ),
	.s1_err_i(  gnd    ),
	.s1_rty_i(  gnd    ),
	// Slave2
	.s2_dat_i(  bram0_dat_r ),
	.s2_dat_o(  bram0_dat_w ),
	.s2_adr_o(  bram0_adr   ),
	.s2_sel_o(  bram0_sel   ),
	.s2_we_o(   bram0_we    ),
	.s2_cyc_o(  bram0_cyc   ),
	.s2_stb_o(  bram0_stb   ),
	.s2_ack_i(  bram0_ack   ),
	.s2_err_i(  gnd         ),
	.s2_rty_i(  gnd         ),
	// Slave3
	.s3_dat_i(  uart0_dat_r ),
	.s3_dat_o(  uart0_dat_w ),
	.s3_adr_o(  uart0_adr   ),
	.s3_sel_o(  uart0_sel   ),
	.s3_we_o(   uart0_we    ),
	.s3_cyc_o(  uart0_cyc   ),
	.s3_stb_o(  uart0_stb   ),
	.s3_ack_i(  uart0_ack   ),
	.s3_err_i(  gnd         ),
	.s3_rty_i(  gnd         ),
	// Slave4
	.s4_dat_i(  timer0_dat_r ),
	.s4_dat_o(  timer0_dat_w ),
	.s4_adr_o(  timer0_adr   ),
	.s4_sel_o(  timer0_sel   ),
	.s4_we_o(   timer0_we    ),
	.s4_cyc_o(  timer0_cyc   ),
	.s4_stb_o(  timer0_stb   ),
	.s4_ack_i(  timer0_ack   ),
	.s4_err_i(  gnd          ),
	.s4_rty_i(  gnd          ),
	// Slave5
	.s5_dat_i(  gpio0_dat_r  ),
	.s5_dat_o(  gpio0_dat_w  ),
	.s5_adr_o(  gpio0_adr    ),
	.s5_sel_o(  gpio0_sel    ),
	.s5_we_o(   gpio0_we     ),
	.s5_cyc_o(  gpio0_cyc    ),
	.s5_stb_o(  gpio0_stb    ),
	.s5_ack_i(  gpio0_ack    ),
	.s5_err_i(  gnd          ),
	.s5_rty_i(  gnd          ),
	// Slave6
	.s6_dat_i(  gnd32  ),
	.s6_ack_i(  gnd    ),
	.s6_err_i(  gnd    ),
	.s6_rty_i(  gnd    ),
	// Slave7
	.s7_dat_i(  gnd32  ),
	.s7_ack_i(  gnd    ),
	.s7_err_i(  gnd    ),
	.s7_rty_i(  gnd    )
);


//---------------------------------------------------------------------------
// LM32 CPU 
//---------------------------------------------------------------------------
lm32_cpu lm0 (
	.clk_i(  clk  ),
	.rst_i(  rst  ),
	.interrupt_n(  intr_n  ),
	//
	.I_ADR_O(  lm32i_adr    ),
	.I_DAT_I(  lm32i_dat_r  ),
	.I_DAT_O(  lm32i_dat_w  ),
	.I_SEL_O(  lm32i_sel    ),
	.I_CYC_O(  lm32i_cyc    ),
	.I_STB_O(  lm32i_stb    ),
	.I_ACK_I(  lm32i_ack    ),
	.I_WE_O (  lm32i_we     ),
	.I_CTI_O(  lm32i_cti    ),
	.I_LOCK_O( lm32i_lock   ),
	.I_BTE_O(  lm32i_bte    ),
	.I_ERR_I(  lm32i_err    ),
	.I_RTY_I(  lm32i_rty    ),
	//
	.D_ADR_O(  lm32d_adr    ),
	.D_DAT_I(  lm32d_dat_r  ),
	.D_DAT_O(  lm32d_dat_w  ),
	.D_SEL_O(  lm32d_sel    ),
	.D_CYC_O(  lm32d_cyc    ),
	.D_STB_O(  lm32d_stb    ),
	.D_ACK_I(  lm32d_ack    ),
	.D_WE_O (  lm32d_we     ),
	.D_CTI_O(  lm32d_cti    ),
	.D_LOCK_O( lm32d_lock   ),
	.D_BTE_O(  lm32d_bte    ),
	.D_ERR_I(  lm32d_err    ),
	.D_RTY_I(  lm32d_rty    )
);
	
//---------------------------------------------------------------------------
// Block RAM
//---------------------------------------------------------------------------
wb_bram #(
	.adr_width( 12 ),
	.mem_file_name( bootram_file )
) bram0 (
	.clk_i(  clk  ),
	.rst_i(  rst  ),
	//
	.wb_adr_i(  bram0_adr    ),
	.wb_dat_o(  bram0_dat_r  ),
	.wb_dat_i(  bram0_dat_w  ),
	.wb_sel_i(  bram0_sel    ),
	.wb_stb_i(  bram0_stb    ),
	.wb_cyc_i(  bram0_cyc    ),
	.wb_ack_o(  bram0_ack    ),
	.wb_we_i(   bram0_we     )
);


//---------------------------------------------------------------------------
// Block RAM1 for testing, untisl i get this fucking PSRAM working
//---------------------------------------------------------------------------
wb_bram_milk #(
	.adr_width( 15 )
) bram1 (
	.clk_i(  clk  ),
	.rst_i(  rst  ),
	//
	.wb_adr_i(  bram1_adr    ),
	.wb_dat_o(  bram1_dat_r  ),
	.wb_dat_i(  bram1_dat_w  ),
	.wb_sel_i(  bram1_sel    ),
	.wb_stb_i(  bram1_stb    ),
	.wb_cyc_i(  bram1_cyc    ),
	.wb_ack_o(  bram1_ack    ),
	.wb_we_i(   bram1_we     )
);



//---------------------------------------------------------------------------
// sram0
//---------------------------------------------------------------------------
/*
wb_sram16 #(
	.adr_width(  18  ),
	.latency(    0   )
) sram0 (
	.clk(         clk           ),
	.reset(       rst           ),
	// Wishbone
	.wb_cyc_i(    sram0_cyc     ),
	.wb_stb_i(    sram0_stb     ),
	.wb_we_i(     sram0_we      ),
	.wb_adr_i(    sram0_adr     ),
	.wb_dat_o(    sram0_dat_r   ),
	.wb_dat_i(    sram0_dat_w   ),
	.wb_sel_i(    sram0_sel     ),
	.wb_ack_o(    sram0_ack     ),
	// SRAM
	.sram_adr(    sram_adr      ),
	.sram_dat(    sram_dat      ),
	.sram_be_n(   sram_be_n     ),
	.sram_ce_n(   sram_ce_n     ),
	.sram_oe_n(   sram_oe_n     ),
	.sram_we_n(   sram_we_n     )
);

*/

//---------------------------------------------------------------------------
// uart0
//---------------------------------------------------------------------------
wire uart0_rxd;
wire uart0_txd;

wb_uart #(
	.clk_freq( clk_freq        ),
	.baud(     uart_baud_rate  )
) uart0 (
	.clk( clk ),
	.reset( rst ),
	//
	.wb_adr_i( uart0_adr ),
	.wb_dat_i( uart0_dat_w ),
	.wb_dat_o( uart0_dat_r ),
	.wb_stb_i( uart0_stb ),
	.wb_cyc_i( uart0_cyc ),
	.wb_we_i(  uart0_we ),
	.wb_sel_i( uart0_sel ),
	.wb_ack_o( uart0_ack ), 
//	.intr(       uart0_intr ),
	.uart_rxd( uart0_rxd ),
	.uart_txd( uart0_txd )
);

//---------------------------------------------------------------------------
// timer0
//---------------------------------------------------------------------------
wb_timer #(
	.clk_freq(   clk_freq  )
) timer0 (
	.clk(      clk          ),
	.reset(    rst          ),
	//
	.wb_adr_i( timer0_adr   ),
	.wb_dat_i( timer0_dat_w ),
	.wb_dat_o( timer0_dat_r ),
	.wb_stb_i( timer0_stb   ),
	.wb_cyc_i( timer0_cyc   ),
	.wb_we_i(  timer0_we    ),
	.wb_sel_i( timer0_sel   ),
	.wb_ack_o( timer0_ack   ), 
	.intr(     timer0_intr  )
);

//---------------------------------------------------------------------------
// General Purpose IO
//---------------------------------------------------------------------------
wire [31:0] gpio0_in;
wire [31:0] gpio0_out;
wire [31:0] gpio0_oe;

wb_gpio gpio0 (
	.clk(      clk          ),
	.reset(    rst          ),
	//
	.wb_adr_i( gpio0_adr    ),
	.wb_dat_i( gpio0_dat_w  ),
	.wb_dat_o( gpio0_dat_r  ),
	.wb_stb_i( gpio0_stb    ),
	.wb_cyc_i( gpio0_cyc    ),
	.wb_we_i(  gpio0_we     ),
	.wb_sel_i( gpio0_sel    ),
	.wb_ack_o( gpio0_ack    ), 
	.intr(     gpio0_intr   ),
	// GPIO
	.gpio_in(  gpio0_in     ),
	.gpio_out( gpio0_out    ),
	.gpio_oe(  gpio0_oe     )
);

//----------------------------------------------------------------------------
// Enable PSRAM
//----------------------------------------------------------------------------

//assign sram_ub = 0;
//assign sram_lb = 0;
//assign sram_clk = 0;
//assign sram_cre = 0;
//assign sram_adv = 0;
//assign flash_cs = 1;
//assign flash_rp = 0;


//---------------------------------------------------------------------------
// LogicAnalyzerComponent
//---------------------------------------------------------------------------
wire        lac_rxd;
wire        lac_txd;
wire        lac_cts;
wire        lac_rts;
assign      lac_rts = 1;
wire [7:0]  select;
wire [7:0]  probe;

/*
lac #(
	.uart_freq_hz(     clk_freq ),
	.uart_baud(  uart_baud_rate ),
	.adr_width(              11 ),
	.width(                   8 )
) lac0 (
	.reset(       rst      ),
	.uart_clk(    clk      ),
	.uart_rxd(    lac_rxd  ),
	.uart_cts(    lac_cts  ),
	.uart_txd(    lac_txd  ),
	.uart_rts(    lac_rts  ),
	//
	.probe_clk(  clk       ),
	.probe(      probe     ),
	.select(     select    )
);

// MUX probe input
assign probe = (select[3:0] == 'h0) ? { rst, lm32i_stb, lm32i_cyc, lm32i_ack, lm32d_stb, lm32d_cyc, lm32d_we, lm32d_ack } :
               (select[3:0] == 'h1) ? lm32i_adr[31:24] :
               (select[3:0] == 'h2) ? lm32i_adr[23:16] :
               (select[3:0] == 'h3) ? lm32i_adr[15: 8] :
               (select[3:0] == 'h4) ? lm32i_adr[ 7: 0] :
               (select[3:0] == 'h5) ? lm32i_dat_r[31:24] :
               (select[3:0] == 'h6) ? lm32i_dat_r[23:16] :
               (select[3:0] == 'h7) ? lm32i_dat_r[15: 8] :
               (select[3:0] == 'h8) ? lm32i_dat_r[ 7: 0] :
               (select[3:0] == 'h9) ? lm32d_adr[31:24] :
               (select[3:0] == 'ha) ? lm32d_adr[23:16] :
               (select[3:0] == 'hb) ? lm32d_adr[15: 8] :
                                      lm32d_adr[ 7: 0] ;

*/

//---------------------------------------------------------------------------
// Sevensegment
//---------------------------------------------------------------------------
gpio_sevenseg gpio_sevenseg0 (
	.clk_i( clk ),
	.seg( seg ),
	.an( an ),
	.gpio( gpio0_out )
);


//----------------------------------------------------------------------------
// Mux UART wires according to sw[0]
//----------------------------------------------------------------------------
assign uart_txd  = (sw[0]) ? uart0_txd : lac_txd;
assign lac_rxd   = (sw[0]) ?         1 : uart_rxd;
assign uart0_rxd = (sw[0]) ? uart_rxd  : 1;

//----------------------------------------------------------------------------
// Mux LEDs and Push Buttons according to sw[1]
//----------------------------------------------------------------------------
wire [7:0] debug_leds = { clk, rst, ~uart_rxd, ~uart_txd, lm32i_stb, lm32i_ack, lm32d_stb, lm32d_ack };
wire [7:0] gpio_leds  = gpio0_out[7:0];

assign led             = (sw[1]) ? gpio_leds : debug_leds;
assign rst             = (sw[1]) ?      1'b0 : btn[0];

assign gpio0_in[11: 8] = (sw[1]) ?       btn : 4'b0;
assign gpio0_in[31:12] = 20'b0;
assign gpio0_in[ 7: 0] =  8'b0;
endmodule    
    

