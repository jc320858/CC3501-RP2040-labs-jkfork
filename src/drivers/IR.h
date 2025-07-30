#pragma once

void tm1637_init();
void tm1637_start();
void tm1637_stop();
void tm1637_write_byte(uint8_t b);
void tm1637_set_brightness(uint8_t brightness);
void tm1637_display_digits(int d0, int d1, int d2, int d3, bool colon);
void run_IR();

