//-----------------------------------------------------------------
// Wishbone BlockRAM
//-----------------------------------------------------------------

module wb_bram #(
	parameter mem_file_name = "none",
	parameter adr_width = 11
) (
	input             clk_i, 
	input             rst_i,
	//
	input             wb_stb_i,
	input             wb_cyc_i,
	input             wb_we_i,
	output            wb_ack_o,
	input      [31:0] wb_adr_i,
	output reg [31:0] wb_dat_o,
	input      [31:0] wb_dat_i,
	input      [ 3:0] wb_sel_i
);

//-----------------------------------------------------------------
// Storage depth in 32 bit words
//-----------------------------------------------------------------
parameter word_width = adr_width - 2;
parameter word_depth = (1 << word_width);

//-----------------------------------------------------------------
// 
//-----------------------------------------------------------------
reg            [31:0] ram [0:word_depth-1];    // actual RAM
reg                   ack;
wire [word_width-1:0] adr;


assign adr        = wb_adr_i[adr_width-1:2];      // 
assign wb_ack_o   = wb_stb_i & ack;

always @(posedge clk_i)
begin
	if (wb_stb_i && wb_cyc_i) 
	begin
		if (wb_we_i) 
			ram[ adr ] <= wb_dat_i;

		wb_dat_o <= ram[ adr ];
		ack <= ~ack;
	end else
		ack <= 0;
    
end

initial 
begin
	if (mem_file_name != "none")
	begin
		$readmemh(mem_file_name, ram);
	end
end

endmodule


module wb_bram_milk #(
	parameter adr_width = 11
) (
	input             clk_i, 
	input             rst_i,
	//
	input             wb_stb_i,
	input             wb_cyc_i,
	input             wb_we_i,
	output reg        wb_ack_o,
	input      [31:0] wb_adr_i,
	output     [31:0] wb_dat_o,
	input      [31:0] wb_dat_i,
	input      [ 3:0] wb_sel_i
);

//-----------------------------------------------------------------
// Storage depth in 32 bit words
//-----------------------------------------------------------------
parameter word_width = adr_width - 2;
parameter word_depth = (1 << word_width);


//-----------------------------------------------------------------
// Actual RAM
//-----------------------------------------------------------------
reg [7:0] ram0 [0:word_depth-1];
reg [7:0] ram1 [0:word_depth-1];
reg [7:0] ram2 [0:word_depth-1];
reg [7:0] ram3 [0:word_depth-1];
wire [word_width-1:0] adr;

wire [7:0] ram0di;
wire ram0we;
wire [7:0] ram1di;
wire ram1we;
wire [7:0] ram2di;
wire ram2we;
wire [7:0] ram3di;
wire ram3we;

reg [7:0] ram0do;
reg [7:0] ram1do;
reg [7:0] ram2do;
reg [7:0] ram3do;

always @(posedge clk_i) begin
	if(ram0we)
		ram0[adr] <= ram0di;
	ram0do <= ram0[adr];
end

always @(posedge clk_i) begin
	if(ram1we)
		ram1[adr] <= ram1di;
	ram1do <= ram1[adr];
end

always @(posedge clk_i) begin
	if(ram2we)
		ram2[adr] <= ram2di;
	ram2do <= ram2[adr];
end

always @(posedge clk_i) begin
	if(ram3we)
		ram3[adr] <= ram3di;
	ram3do <= ram3[adr];
end

assign ram0we = wb_cyc_i & wb_stb_i & wb_we_i & wb_sel_i[0];
assign ram1we = wb_cyc_i & wb_stb_i & wb_we_i & wb_sel_i[1];
assign ram2we = wb_cyc_i & wb_stb_i & wb_we_i & wb_sel_i[2];
assign ram3we = wb_cyc_i & wb_stb_i & wb_we_i & wb_sel_i[3];

assign ram0di = wb_dat_i[7:0];
assign ram1di = wb_dat_i[15:8];
assign ram2di = wb_dat_i[23:16];
assign ram3di = wb_dat_i[31:24];

assign wb_dat_o = {ram3do, ram2do, ram1do, ram0do};

assign adr = wb_adr_i[adr_width-1:2];

always @(posedge clk_i) begin
	if(rst_i)
		wb_ack_o <= 1'b0;
	else begin
		if(wb_cyc_i & wb_stb_i)
			wb_ack_o <= ~wb_ack_o;
		else
			wb_ack_o <= 1'b0;
	end
end

endmodule

