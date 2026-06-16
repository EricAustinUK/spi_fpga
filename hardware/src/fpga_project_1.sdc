//Copyright (C)2014-2026 GOWIN Semiconductor Corporation.
//All rights reserved.
//File Title: Timing Constraints file
//Tool Version: V1.9.12.02_SP2 
//Created Time: 2026-06-15 02:00:12
create_clock -name nrf_SPIM_CLK -period 125 -waveform {0 62.5} [get_ports {i_clk}]
