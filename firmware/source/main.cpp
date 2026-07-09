#include "nrf.h"
#define IMG_H 240
#define IMG_W 320
#define GREYSCALE_IMG_SIZE (IMG_W*IMG_H/8)
#define INIT_PIXEL_THRESHOLD 0x35
#define BLANK_THRESHOLD 5

extern void spi_send_arr(volatile uint32_t arr_ptr, uint32_t size, bool is_to_nano);
extern void get_img(uint8_t * output, uint8_t * threshold);

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

void inference_loop(uint8_t * threshold){
    static uint8_t raw_img_buffer[GREYSCALE_IMG_SIZE];
    for(uint16_t i = 0; i<GREYSCALE_IMG_SIZE; i++) raw_img_buffer[i] = 0xFF;
    get_img(raw_img_buffer, threshold);
    uint16_t min_x = IMG_W;
    uint16_t max_x = 0;
    uint16_t min_y = IMG_H;
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
            min_y = (y < min_y ? y : min_y);
            max_y = (y > max_y ? y : max_y);
        }
    }

    // needs another step before these boundaries are accepted, for now just early return if data is obvious garbage
    if ((max_x - min_x) > (IMG_W*3/4) || (max_y - min_y) > (IMG_H*3/4)) return;
    if ((max_x - min_x) > 255 || (max_y - min_y) > 255) return; // so i can use 8 bit
    if ((max_x - min_x) < 20 && (max_y - min_y) < 20) return;
    
    // lets use nearest neighbour first
    volatile uint32_t grid[28];
    for(uint8_t i = 0; i < 28; i++)
        grid[i] = 0x0fffffff;
    uint8_t raw_w = max_x - min_x;
    uint8_t raw_h = max_y - min_y;
    // maintain aspect ratio by setting biggest dimension to 20px
    uint8_t new_w = raw_w < raw_h ? (20*raw_w) / raw_h : 20;
    uint8_t new_h = raw_w > raw_h ? (20*raw_h) / raw_w : 20;

    // centre of mass calculations
    int32_t col_acc = 0;
    int32_t row_acc = 0;
    uint16_t pxn_acc = 0;
    uint16_t mid_col = (max_x-min_x) / 2;
    uint16_t mid_row = (max_y-min_y) / 2;

    for(uint16_t i = min_y; i <= max_y; i++){
        for(uint16_t j = min_x; j <= max_x; j++){
            if(!is_pixel_blank(raw_img_buffer, i*IMG_W + j)){
                col_acc += j;
                row_acc += i;
                pxn_acc++;
            }
        }
    }

    if(pxn_acc == 0) return; // no pixels?? 

    int16_t raw_com_x_offset = (col_acc / pxn_acc) - (min_x + mid_col);
    int16_t raw_com_y_offset = (row_acc / pxn_acc) - (min_y + mid_row);
    

    for(uint8_t i = 0; i < new_h; i++){
        for(uint8_t j = 0; j < new_w; j++){
            uint16_t raw_x_px = (((uint32_t) j * raw_w) / new_w) + min_x + raw_com_x_offset;
            uint16_t raw_y_px = (((uint32_t) i * raw_h) / new_h) + min_y + raw_com_y_offset;
            
            uint16_t grid_x = j + (28-new_w)/2;
            uint16_t grid_y = i + (28-new_h)/2;
            
            uint8_t grid_bit_index = 27 - grid_x;
            uint32_t pixel_num = raw_y_px * IMG_W + raw_x_px;

            
            uint8_t mass_acc = 0;
            if(!is_pixel_blank(raw_img_buffer, pixel_num))
                mass_acc++;
            if(raw_x_px > 0 && !is_pixel_blank(raw_img_buffer, pixel_num - 1))
                mass_acc++;
            if(raw_x_px < (IMG_W-1) && !is_pixel_blank(raw_img_buffer, pixel_num + 1))
                mass_acc++;
            if(raw_y_px > 0 && !is_pixel_blank(raw_img_buffer, pixel_num - IMG_W))
                mass_acc++;
            if(raw_y_px < (IMG_H-1) && !is_pixel_blank(raw_img_buffer, pixel_num + IMG_W))
                mass_acc++;
            if(mass_acc >= 2)
                grid[grid_y] &= ~(1 << grid_bit_index);
            
        }
    }

    for(uint8_t i = 0; i < 28; i++)
        spi_send_arr((uint32_t) &grid[i], 4, true);
}

int main()
{
    uint8_t threshold = INIT_PIXEL_THRESHOLD;
    while(true)
        inference_loop(&threshold);
}
