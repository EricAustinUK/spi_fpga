//Copyright (C)2014-2026 Gowin Semiconductor Corporation.
//All rights reserved.
//File Title: IP file
//Tool Version: V1.9.12.02_SP2
//IP Version: 1.2
//Part Number: GW1NZ-LV1QN48C6/I5
//Device: GW1NZ-1
//Device Version: D
//Created Time: Mon Jul 13 18:17:57 2026

module Gowin_User_Flash (dout, xe, ye, se, prog, erase, nvstr, xadr, yadr, din);

output [31:0] dout;
input xe;
input ye;
input se;
input prog;
input erase;
input nvstr;
input [4:0] xadr;
input [5:0] yadr;
input [31:0] din;

FLASH64KZ flash_inst (
    .DOUT(dout),
    .XE(xe),
    .YE(ye),
    .SE(se),
    .PROG(prog),
    .ERASE(erase),
    .NVSTR(nvstr),
    .XADR(xadr),
    .YADR(yadr),
    .DIN(din)
);

endmodule //Gowin_User_Flash
