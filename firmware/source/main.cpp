#include "nrf.h"
#define GREYSCALE_IMG_SIZE (320*240/8)
#define PIXEL_THRESHOLD 0x35

extern void spi_send_arr(volatile uint32_t arr_ptr, uint32_t size, bool is_to_nano);
extern void get_img(volatile uint8_t * output, uint8_t threshold);

void busy_sleep(uint32_t delay_us){
    NRF_TIMER1->TASKS_STOP = 1; // stop other timers
    NRF_TIMER1->BITMODE = 3; // 32 bit
    NRF_TIMER1->CC[0] = delay_us; // set cmp to time
    NRF_TIMER1->PRESCALER = 4;

    // clear flags
    NRF_TIMER1->EVENTS_COMPARE[0] = 0;
    NRF_TIMER1->TASKS_CLEAR = 1;

    // wait for delay to be met
    NRF_TIMER1->TASKS_START = 1;
    while(NRF_TIMER1->EVENTS_COMPARE[0] == 0);
    
    // clear flags
    NRF_TIMER1->TASKS_STOP = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;
    NRF_TIMER1->EVENTS_COMPARE[0] = 0;
}


void inference_loop(){

}

int main()
{
    // A 28x28 matrix pattern with varied row boundaries and a diagonal step
    volatile uint32_t test_image[28] = {
        0x0AFFFFFF,
        0x0FFFFF0A,
        0x05555555,
        0x0AAAAAAA,
        0x0F000000,
        0x03C00000,
        0x00F00000,
        0x003C0000,
        0x000F0000,
        0x0003C000,
        0x0000F000,
        0x00003C00,
        0x00000F00,
        0x000003C0,
        0x000000F0,
        0x0000003C,
        0x0000000F,
        0x08000000,
        0x00000001,
        0x08000001,
        0x04000002,
        0x02000004,
        0x01000008,
        0x00000000,
        0x00000000,
        0x05555555,
        0x0AAAAAAA,
        0x0FFFFFFF
    };    

    for(int i = 0; i < 28; i++){
        spi_send_arr((uint32_t) &test_image[i], 4, true);
        busy_sleep(20);
    }
    while(true){    
        inference_loop();
        static volatile uint8_t img_buffer[GREYSCALE_IMG_SIZE];
        get_img(img_buffer, PIXEL_THRESHOLD);
    }
}
