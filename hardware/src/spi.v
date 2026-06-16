module spi_recv(
    input i_clk,
    input i_mosi,
    input i_cs,
    output reg [1:0] o_debugrow
);

reg [31:0] mem_br;
reg [27:0] img_mem [0:27];
reg [4:0] row = 5'd0;

always @(posedge i_clk) begin
    if (!i_cs) begin
        mem_br <= {mem_br[30:0], i_mosi};
    end
end

always @(posedge i_cs) begin
    img_mem[row] <= mem_br[27:0];
    o_debugrow <= mem_br[1:0];
    row <= row + 5'd1;
end

endmodule