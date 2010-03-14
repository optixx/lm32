
module divide_by_two (
    input clk, 
    output reg dclk
);

reg [1:0] CounterZ;

always @(posedge clk)
begin
    CounterZ <= CounterZ + 1;
    dclk <= CounterZ[0];
end
endmodule


module divide_by_N( reset, clk, enable, n, clk_out );

input clk;
input reset;
input enable;
input [7:0] n;
output clk_out;

wire [7:0] m;
wire dbn_en;
reg [7:0] count;
reg out1;
reg out2;
wire out;
wire clk_out;

assign dbn_en = n[7] | n[6] | n[5] | n[4] | n[3] | n[2] | n[1];

always @(negedge clk or posedge reset) begin
    if (reset==1) begin
        out1<=1'b0;
        count<=8'h00;
    end
    else if (dbn_en==1 && enable==1) begin
        if (n[0]==0) begin // even count
            if (count==m-1)begin
                count<=8'h00;
                out1<=~out1;
            end
        else
            count<=count+1;
        end
        else if (count==n-1)begin // odd count
            count<=8'h00;
            out1<=~out1;
        end
    else
        count<=count+1;
    end
end

assign m=n>>1;

always @(posedge clk or posedge reset) begin
    if (reset==1) begin
        out2<=1'b0;
    end
    else if (count==m && enable==1)
        out2<=out1;
end

assign out=(enable==1)?((dbn_en==0)? clk : (n[0]==1)? out1^out2 : out1):1'b0;

assign clk_out = out;

endmodule
