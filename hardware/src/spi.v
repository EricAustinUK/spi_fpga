module spi_recv(
    input i_clk,
    input i_mosi,
    input i_ci
);

reg [27:0] mem_br;
reg [27:0] img_mem [0:27];
reg [4:0] row = 5'd0;
reg [4:0] col = 5'd0;

always @(posedge i_clk) begin
    if (i_ci) begin
        mem_br[col] <= i_mosi;
        if(col == 5'd27) begin
            img_mem[row] <= {mem_br[27:1], i_mosi};
            row <= row + 5'd1;
            col <= 5'd0;
        end else begin
            col <= col + 5'd1;
        end
    end
end

endmodule