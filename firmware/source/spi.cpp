#include "nrf.h"
#define CAM_CSN 9
#define NANO_CSN 4


void cnf_spi_pins(){
    static volatile bool cfgd = false;
    if(cfgd) return;
    cfgd = true;

    NRF_SPIM3->PSEL.MOSI = 2; // send bits over external pin 0
    NRF_SPIM3->PSEL.SCK  = 3; // send clock signal over external pin 1
    NRF_SPIM3->PSEL.MISO = 34;  // set MISO to external pin 16
    NRF_SPIM3->PSEL.CSN = 0xFFFFFFFF; // blank to manually set CS pins now
    NRF_SPIM3->FREQUENCY = 0x80000000; // frequency to 8MHz
    NRF_SPIM3->CONFIG = 0b000;
    NRF_SPIM3->RXD.MAXCNT = 0;

    NRF_SPIM3->ENABLE = 0b111; // enable SPI peripheral

    // configure ext p0 and p1 as outputs
    NRF_P0->PIN_CNF[2] = 1;
    NRF_P0->PIN_CNF[3] = 1;

    // set chip selects high
    NRF_P0->PIN_CNF[NANO_CSN] = 1;
    NRF_P0->PIN_CNF[CAM_CSN] = 1;
    NRF_P0->OUTSET = (1 << NANO_CSN) | (1 << CAM_CSN);
}

void spi_send_arr(volatile uint32_t *arr, uint32_t size, bool big_endian){
    cnf_spi_pins();
    // clear event registers
    NRF_SPIM3->EVENTS_END = 0;
    NRF_SPIM3->EVENTS_STARTED = 0;

    if(big_endian) *arr = __REV(*arr);
    // configure TX buffer
    NRF_SPIM3->TXD.PTR = (uint32_t)arr; 
    NRF_SPIM3->TXD.MAXCNT = (size) * sizeof(uint32_t); 
    
    NRF_SPIM3->TASKS_START = 1; // start sending array

    while(NRF_SPIM3->EVENTS_STARTED==0); // busy wait for event to start

    NRF_P0->OUTCLR = (1 << NANO_CSN); // set CS low

    while(NRF_SPIM3->EVENTS_END==0); // busy wait for event to end

    NRF_P0->OUTSET = (1 << NANO_CSN); // set CS high

    NRF_SPIM3->TASKS_STOP = 1; // stop sending when buffer is sent
}

void cnf_camera(){
    static volatile bool cfgd = false;
    if(cfgd) return;
    cfgd = true;

    NRF_TWIM0->PSEL.SCL = 8; // set i2c scl to external pin 20
    NRF_TWIM0->PSEL.SDA = 16; // set i2c sda to external pin 19
}

void spi_get_img(){
    cnf_spi_pins();
    cnf_camera();
}