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


