module nn_pass(
    input i_clk,
    input i_data_ready,
    input [27:0] i_row, // row pixel input
    output reg o_data_ready,
    output reg [4:0] o_class
);

wire [63:0] _l1_pass_result;
wire [255:0] _l2_pass_result;
wire [127:0] _l3_pass_result;

wire _l1_data_ready, _l2_data_ready, _l3_data_ready; 

nn_pass_l1 l1(
    .i_clk(i_clk),
    .i_data_ready(i_data_ready),
    .i_row(i_row),
    .o_pass_result(_l1_pass_result),
    .o_data_ready(_l1_data_ready)
);

nn_pass_l2 l2(
    .i_clk(i_clk),
    .i_data_ready(_l1_data_ready),
    .i_l1(_l1_pass_result),
    .o_pass_result(_l2_pass_result),
    .o_data_ready(_l2_data_ready)
);

/*

nn_pass_l3 l3(
    .i_clk(i_clk),
    .i_data_ready(_l2_data_ready),
    .i_l1(_l2_pass_result),
    .o_pass_result(_l3_pass_result),
    .o_data_ready(_l3_data_ready)
);

nn_pass_l4 l4(
    .i_clk(i_clk),
    .i_data_ready(_l3_data_ready),
    .i_l1(_l3_pass_result),
    .o_pass_result(o_class),
    .o_data_ready(o_data_ready)
);

*/

endmodule

module nn_pass_l1(
    input wire i_clk,
    input i_data_ready,
    input [27:0] i_row, // row pixel input
    output reg [63:0] o_pass_result, // result of forward pass
    output reg o_data_ready // is result ready
);

reg [27:0] mem [0:1791]; // this will be the 784 -> 64 layer
reg [4:0] BNN_THRESHOLD = 5'd14; 


reg [10:0] mem_addr = 11'd0;
reg [27:0] current_weights;

reg [9:0] l1_pass_acc [63:0];

initial begin
    $readmemb("weights/l1_weights.txt", mem); // lets put layer 1 weights in BRAM
end

reg inf_started = 0; // is nn in wider "inference" state
reg processing = 0; // is nn in processing state
reg [4:0] row_no = 5'd0; // row number
reg [5:0] neuron_no = 6'd0; // neuron number

// pipelining means having each of these a cycle behind where my memory reads are
reg [4:0] prev_row_no = 5'd0; // row number
reg [5:0] prev_neuron_no = 6'd0; // neuron number
reg prev_proc = 0;

reg [4:0] counted = 5'd0;
reg [9:0] acc_val = 10'd0;

always @(posedge i_clk) begin
    current_weights <= mem[mem_addr]; // retrieve l1 weights for this row + node

    // pipelining - wait till the next cycle to process because the memory read takes a cycle
    prev_neuron_no <= neuron_no;
    prev_row_no <= row_no;
    prev_proc <= processing;

    // state machine: inf_started debounces setting neuron back to 0
    if(!inf_started && i_data_ready) begin
        inf_started <= 1;
        neuron_no <= 0;
        mem_addr <= row_no;
        processing <= 1;
        o_data_ready <= 0;
    end else if(!i_data_ready) begin // if SPI starts reading again, reset state
        inf_started <= 0;
        // possibly set row no to row + 1? I think forward pass always beats SPI (27MHz vs 8MHz) recv so probably not
    end

    // loop state calculator
    if (processing) begin
        if (neuron_no == 63) begin      // stop processing if final neuron reached
            processing <= 0;
            if (row_no == 27) begin // reset row if complete
                row_no <= 0;
            end else begin // else increment row
                row_no <= row_no + 1;
            end
        end else begin                  // otherwise increment neuron and look at next row
            neuron_no <= neuron_no + 1;
            mem_addr <= mem_addr + 28;
        end
    end

    // delayed pipelining, behind by a cycle
    if (prev_proc) begin
        counted = popcount(~(i_row ^ current_weights)); // popcount of row XNOR weights
        // if new row, overwrite accumulator with count, otherwise increment
        acc_val = (prev_row_no == 0) ? counted : (l1_pass_acc[prev_neuron_no] + counted);

        // add to forward pass accumulator array
        l1_pass_acc[prev_neuron_no] <= acc_val;
        
        // if row complete, compare against threshold (may be able to optimise with some bit shifts here)
        if (prev_row_no == 27) begin
            o_pass_result[prev_neuron_no] <= (acc_val > BNN_THRESHOLD);
            if(prev_neuron_no == 63) begin
                o_data_ready <= 1;
            end
        end
    end
end


endmodule

module nn_pass_l2(
    input wire i_clk,
    input i_data_ready,
    input [63:0] i_l1, // input from layer 1
    output reg [255:0] o_pass_result, // layer 2 result
    output reg o_data_ready // result's ready flag
);

reg [63:0] neuron_weights; // register cache for each input's weights



always @(posedge i_clk) begin
    
end

endmodule

// popcount function: count how many pos bits are in the 28 bit word
function [4:0] popcount;
    input [27:0] i_bits;
    integer i;
    begin
        popcount = 0;
        for (i = 0; i < 28; i = i + 1) begin
            popcount = popcount + i_bits[i];
        end
    end
endfunction