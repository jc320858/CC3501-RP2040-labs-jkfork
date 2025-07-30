#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "WS2812.pio.h" 
#include "drivers/logging/logging.h"
#include "hardware/adc.h"

#define TM1637_DIO_PIN 18
#define TM1637_CLK_PIN 19

#define BBIF_PIN 4

const uint8_t digitToSegment[] = {
    0x3f, // 0
    0x06, // 1
    0x5b, // 2
    0x4f, // 3
    0x66, // 4
    0x6d, // 5
    0x7d, // 6
    0x07, // 7
    0x7f, // 8
    0x6f  // 9
};

void tm1637_start_IR() {
    gpio_put(TM1637_CLK_PIN, 1);
    gpio_put(TM1637_DIO_PIN, 1);
    sleep_us(2);
    gpio_put(TM1637_DIO_PIN, 0);
    sleep_us(2);
    gpio_put(TM1637_CLK_PIN, 0);
}

void tm1637_stop_IR() {
    gpio_put(TM1637_CLK_PIN, 0);
    sleep_us(2);
    gpio_put(TM1637_DIO_PIN, 0);
    sleep_us(2);
    gpio_put(TM1637_CLK_PIN, 1);
    sleep_us(2);
    gpio_put(TM1637_DIO_PIN, 1);
}

void tm1637_write_byte_IR(uint8_t b) {
    for (int i = 0; i < 8; i++) {
        gpio_put(TM1637_CLK_PIN, 0);
        gpio_put(TM1637_DIO_PIN, (b >> i) & 1);
        sleep_us(3);
        gpio_put(TM1637_CLK_PIN, 1);
        sleep_us(3);
    }
    // Wait for ACK
    gpio_put(TM1637_CLK_PIN, 0);
    gpio_set_dir(TM1637_DIO_PIN, GPIO_IN);
    sleep_us(5);
    gpio_put(TM1637_CLK_PIN, 1);
    sleep_us(5);
    gpio_set_dir(TM1637_DIO_PIN, GPIO_OUT);
    gpio_put(TM1637_CLK_PIN, 0);
}

void tm1637_set_brightness_IR(uint8_t brightness) {
    tm1637_start_IR();
    tm1637_write_byte_IR(0x88 | (brightness & 0x07));
    tm1637_stop_IR();
}

void tm1637_display_digits_IR(int d0, int d1, int d2, int d3, bool colon) {
    tm1637_start_IR();
    tm1637_write_byte_IR(0x40); // Set auto-increment mode
    tm1637_stop_IR();

    tm1637_start_IR();
    tm1637_write_byte_IR(0xc0); // Start address 0
    tm1637_write_byte_IR(digitToSegment[d0]);
    uint8_t seg1 = digitToSegment[d1];
    if (colon) seg1 |= 0x80; // Add colon if needed
    tm1637_write_byte_IR(seg1);
    tm1637_write_byte_IR(digitToSegment[d2]);
    tm1637_write_byte_IR(digitToSegment[d3]);
    tm1637_stop_IR();
}

void tm1637_init_IR() {
    gpio_init(TM1637_DIO_PIN);
    gpio_init(TM1637_CLK_PIN);
    gpio_set_dir(TM1637_DIO_PIN, GPIO_OUT);
    gpio_set_dir(TM1637_CLK_PIN, GPIO_OUT);
}

void init_IR() {
    gpio_init(BBIF_PIN);
    gpio_set_dir(BBIF_PIN, GPIO_IN);
    gpio_pull_up(BBIF_PIN);
}

void run_IR() {
    stdio_init_all();
    tm1637_init_IR();
    init_IR();

    tm1637_set_brightness_IR(7); // Max brightness

    int minutes = 0;
    int seconds = 0;

    printf("Waiting for car...\n");
    while (gpio_get(BBIF_PIN) != 0) {
        tm1637_display_digits_IR(0, 0, 0, 0, true);
        sleep_ms(200);
    }
    printf("Car passed! Timer started.\n");
    
    while (true) {
        int d0 = minutes / 10;
        int d1 = minutes % 10;
        int d2 = seconds / 10;
        int d3 = seconds % 10;
        tm1637_display_digits_IR(d0, d1, d2, d3, true);

        sleep_ms(1000); // 1 second

        seconds++;
        if (seconds > 59) {
            seconds = 0;
            minutes++;
            if (minutes > 99) { // Roll over after 99:59
                minutes = 0;
            }
        }

        // Restart
        if (gpio_get(BBIF_PIN) == 0) {
            printf("Lap completed, time displayed.\n");
            minutes = 0;
            seconds = 0;
            sleep_ms(200); 
        }
    }
}