//Copyright (C)2014-2026 Gowin Semiconductor Corporation.
//All rights reserved.
//File Title: Template file for instantiation
//Tool Version: V1.9.12.02_SP2
//IP Version: 1.2
//Part Number: GW1NZ-LV1QN48C6/I5
//Device: GW1NZ-1
//Device Version: D
//Created Time: Mon Jul 13 18:17:57 2026

//Change the instance name and port connections to the signal names
//--------Copy here to design--------

    Gowin_User_Flash your_instance_name(
        .dout(dout), //output [31:0] dout
        .xe(xe), //input xe
        .ye(ye), //input ye
        .se(se), //input se
        .prog(prog), //input prog
        .erase(erase), //input erase
        .nvstr(nvstr), //input nvstr
        .xadr(xadr), //input [4:0] xadr
        .yadr(yadr), //input [5:0] yadr
        .din(din) //input [31:0] din
    );

//--------Copy end-------------------
