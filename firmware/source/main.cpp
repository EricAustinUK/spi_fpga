#include "nrf.h"
#define IMG_H 240
#define IMG_W 320
#define GREYSCALE_IMG_SIZE (IMG_W*IMG_H/8)
#define PIXEL_THRESHOLD 0x35 // MAY NEED TO MAKE ADAPTIVE
#define BLANK_THRESHOLD 5

extern void spi_send_arr(volatile uint32_t arr_ptr, uint32_t size, bool is_to_nano);
extern void get_img(uint8_t * output, uint8_t threshold);

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

void get_adjacent_nums(uint32_t pixel_num, uint32_t * adj_buf){
    uint16_t x = pixel_num % IMG_W;
    uint16_t y = pixel_num / IMG_W;
    uint8_t fine = 0x0F; // left right up down (lsb)
    
    // don't need AND NOT (CLR) because we know its full of 1s
    if(x == 0)          fine ^= (1 << 3);
    if(x == IMG_W-1)    fine ^= (1 << 2);
    if(y == 0)          fine ^= (1 << 1);
    if(y == IMG_H - 1)  fine ^= 1;
    
    adj_buf[0] = fine & 0x8 ? (pixel_num - 1) : pixel_num;
    adj_buf[1] = fine & 0x4 ? (pixel_num + 1) : pixel_num;
    adj_buf[2] = fine & 0x2 ? (pixel_num - IMG_W) : pixel_num;
    adj_buf[3] = fine & 0x1 ? (pixel_num + IMG_W) : pixel_num;
    adj_buf[4] = fine & 0xA ? (pixel_num - IMG_W - 1) : pixel_num;
    adj_buf[5] = fine & 0x6 ? (pixel_num - IMG_W + 1) : pixel_num;
    adj_buf[6] = fine & 0x9 ? (pixel_num + IMG_W - 1) : pixel_num;
    adj_buf[7] = fine & 0x5 ? (pixel_num + IMG_W + 1) : pixel_num;
}

bool is_pixel_blank(uint8_t * img_buffer, uint32_t pixel_num){
    uint8_t * byte_ptr = (img_buffer + (pixel_num / 8));
    uint8_t byte_n = pixel_num & 0x7;
    return *byte_ptr & (1 << byte_n);
}

void set_pixel_blank(uint8_t * img_buffer, uint32_t pixel_num){
    uint8_t * byte_ptr = (img_buffer + (pixel_num / 8));
    uint8_t byte_n = pixel_num & 0x7;
    *byte_ptr |= (1 << byte_n);
}

bool is_noise(uint8_t * img_buffer, uint32_t pixel_num){
    uint32_t adj_buf[8];
    get_adjacent_nums(pixel_num, adj_buf);
    uint8_t blanks = 0;
    for(uint8_t i = 0; i < 8; i++){
        if(is_pixel_blank(img_buffer, adj_buf[i])) blanks++;
        if (blanks == BLANK_THRESHOLD) return true;
    }
    return false;
}

void inference_loop(){
    static uint8_t raw_img_buffer[GREYSCALE_IMG_SIZE];
    get_img(raw_img_buffer, PIXEL_THRESHOLD);
    uint16_t min_x = IMG_H;
    uint16_t max_x = 0;
    uint16_t min_y = IMG_W;
    uint16_t max_y = 0;
    
    for(uint16_t i = 0; i < GREYSCALE_IMG_SIZE; i++){
        for(uint8_t j = 0; j < 8; j++){
            uint32_t pixel_num = i * 8 + j;
            if(is_pixel_blank(raw_img_buffer, pixel_num)) continue;
            if(is_noise(raw_img_buffer, pixel_num)) { set_pixel_blank(raw_img_buffer, pixel_num); continue; };
            uint16_t x = pixel_num % IMG_W;
            uint16_t y = pixel_num / IMG_W;
            min_x = (x < min_x ? x : min_x);
            max_x = (x > max_x ? x : max_x);
            min_y = (y < min_y ? x : min_y);
            max_y = (y > max_y ? x : max_y);
        }
    }
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
    }
}
