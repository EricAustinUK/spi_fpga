#include "nrf.h"

void cnf_spi_pins(){
    static bool cfgd = false;
    if(cfgd) return;
    cfgd = true;
    
    NRF_SPIM0->PSEL.SCK  = 13; 
    NRF_SPIM0->PSEL.MISO = 0xFFFFFFFF; 
    NRF_SPIM0->PSEL.MOSI = 15;
}

void send_arr(uint32_t *arr, uint32_t size){

}
