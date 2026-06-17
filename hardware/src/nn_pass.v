module nn_pass_l1(
    input wire i_clk,
    input i_data_ready,
    input [27:0] i_row, // row pixel input
    output reg [63:0] o_pass_result // result of forward pass
);

reg [27:0] rom [0:1791]; // this will be the 784 -> 64 layer

reg [4:0] row_no = 5'd0; // row number
reg [5:0] neuron_no = 6'd0; // neuron number

wire [10:0] rom_addr;
assign rom_addr = (neuron_no * 28) + row_no; // possibly optimise mult later
wire [27:0] current_weights;
assign current_weights = rom[rom_addr];

reg [9:0] l1_pass_acc [63:0];

initial begin
    $readmemb("l1_weights.txt", rom); 
end

reg inf_started = 0;
reg processing = 0;
reg weights_ready = 0;

always @(posedge i_clk) begin
    if(!inf_started && i_data_ready) begin
        inf_started <= 1;
        neuron_no <= 0;
        processing <= 1;
    end else if(!i_data_ready) begin
        inf_started <= 0;
    end

    if (processing) begin
        
        if(!weights_ready) begin
            weights_ready <= 1;
        end else begin 
            
            l1_pass_acc[neuron_no] <= l1_pass_acc[neuron_no] + popcount(~(i_row ^ current_weights));
            
            if (neuron_no == 63) begin
                processing <= 0;
                weights_ready <= 0;
                row_no <= row_no + 1;
                if (row_no == 27) begin
                    row_no <= 0;
                end
            end else begin
                neuron_no <= neuron_no + 1;
            end
        end
    end
end

endmodule

function [4:0] popcount;
    input [27:0] i_bits;
    integer i;
    integer i;
    begin
        popcount = 0;
        for (i = 0; i < 28; i = i + 1) begin
            popcount = popcount + i_bits[i];
        end
    end
endfunction