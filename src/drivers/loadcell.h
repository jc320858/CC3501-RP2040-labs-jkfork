#pragma once

void hx711_init();

uint32_t hx711_read();

float hx711_get_weight_kg(uint32_t raw_value, uint32_t zero_offset, float scale_factor);

static uint32_t tare_offset = 0;
static float calibration_factor = 1.0f;

void lc_calibrate_tare();

void lc_calibrate_scale(float known_weight_kg);

float lc_get_weight_kg();

void lc_calibrate();

void tm1637_init();

void tm1637_start();

void tm1637_stop();

void tm1637_write_byte(uint8_t data);

void display_weight(float weight_kg);

void lc_calibrate_send();