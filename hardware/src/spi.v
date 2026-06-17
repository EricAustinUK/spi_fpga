module spi_recv(
    input i_clk,
    input i_sck,
    input i_mosi,
    input i_cs,
    output reg [27:0] o_row_bits,
    output reg o_data_ready
);

reg [31:0] mem_br;

always @(posedge i_sck) begin
    if (!i_cs) begin
        mem_br <= {mem_br[30:0], i_mosi};
    end
end

reg prev_cs = 1;

always @(posedge i_clk) begin
    if (prev_cs == 0 && i_cs == 1) begin 
        o_row_bits <= mem_br[27:0];
        o_data_ready <= 1;
    end else if(prev_cs == 1 && i_cs == 0) begin
        o_data_ready <= 0;
    end
    prev_cs <= i_cs;
end

endmodule