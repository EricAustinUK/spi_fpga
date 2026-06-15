module adder (
    input wire a,
    input wire b,
    input wire cin,
    output wire sum,
    output wire cout
);

    wire _sum, _cout1, _cout2;

    half_adder ha_1(
        .a(a),
        .b(b),
        .sum(_sum),
        .cout(_cout1)
    );

    half_adder ha_2(
        .a(_sum),
        .b(cin),
        .sum(sum),
        .cout(_cout2)
    );

    assign cout = _cout1 | _cout2;

endmodule
