#pragma once

void tm1637_init_IR();
void tm1637_start_IR();
void tm1637_stop_IR();
void tm1637_write_byte_IR(uint8_t b);
void tm1637_set_brightness_IR(uint8_t brightness);
void tm1637_display_digits_IR(int d0, int d1, int d2, int d3, bool colon);
void run_IR();

void tm1637_init_IR2();
void tm1637_start_IR2();
void tm1637_stop_IR2();
void tm1637_write_byte_IR2(uint8_t b);
void tm1637_display_digits_IR2(int d0, int d1, int d2, int d3, bool colon);
void tm1637_set_brightness_IR2(uint8_t brightness);

