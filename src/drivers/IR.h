#pragma once

void tm1637_init_IR();
void tm1637_start_IR();
void tm1637_stop_IR();
void tm1637_write_byte_IR(uint8_t b);
void tm1637_set_brightness_IR(uint8_t brightness);
void tm1637_display_digits_IR(int d0, int d1, int d2, int d3, bool colon);
void run_IR();

