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

#define TM1637_DIO_PIN2 8
#define TM1637_CLK_PIN2 9

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
    static bool initialised = false;
    if (!initialised) {
        tm1637_init_IR();
        init_IR();
        tm1637_set_brightness_IR(7); // Max brightness
        
        // Initialize second display
        tm1637_init_IR2();
        tm1637_set_brightness_IR2(7); // Max brightness
        
        initialised = true;
        printf("IR system initialized.\n");
    }

    static int minutes = 0;
    static int seconds = 0;
    static bool timing = false;
    static absolute_time_t last_tick;
    static bool last_beam = true;
    
    // Variables for last lap time
    static int last_lap_minutes = 0;
    static int last_lap_seconds = 0;

    bool beam_present = gpio_get(BBIF_PIN);

    if (last_beam && !beam_present) {
        if (!timing) {
            // Start timer
            timing = true;
            minutes = 0;
            seconds = 0;
            last_tick = get_absolute_time();
            printf("Car passed! Timer started.\n");
        } else {
            // Lap completed, save lap time and restart for next lap
            printf("Lap completed, time displayed: %02d:%02d\n", minutes, seconds);
            
            // Save the lap time for second display
            last_lap_minutes = minutes;
            last_lap_seconds = seconds;
            
            // Reset timer for next lap (restart automatically)
            minutes = 0;
            seconds = 0;
            last_tick = get_absolute_time();
            printf("Timer restarted for next lap.\n");
            // Note: timing remains true, so timer continues running
        }
    }
    last_beam = beam_present;

    if (timing) {
        if (absolute_time_diff_us(last_tick, get_absolute_time()) >= 1000000) {
            last_tick = delayed_by_us(last_tick, 1000000);
            seconds++;
            if (seconds > 59) {
                seconds = 0;
                minutes++;
                if (minutes > 99) minutes = 0;
            }
        }
    }

    // Display current timer on first display
    int d0 = minutes / 10;
    int d1 = minutes % 10;
    int d2 = seconds / 10;
    int d3 = seconds % 10;
    tm1637_display_digits_IR(d0, d1, d2, d3, true);
    
    // Display last lap time on second display
    int ld0 = last_lap_minutes / 10;
    int ld1 = last_lap_minutes % 10;
    int ld2 = last_lap_seconds / 10;
    int ld3 = last_lap_seconds % 10;
    tm1637_display_digits_IR2(ld0, ld1, ld2, ld3, true);
}

void tm1637_start_IR2() {
    gpio_put(TM1637_CLK_PIN2, 1);
    gpio_put(TM1637_DIO_PIN2, 1);
    sleep_us(2);
    gpio_put(TM1637_DIO_PIN2, 0);
    sleep_us(2);
    gpio_put(TM1637_CLK_PIN2, 0);
}

void tm1637_stop_IR2() {
    gpio_put(TM1637_CLK_PIN2, 0);
    sleep_us(2);
    gpio_put(TM1637_DIO_PIN2, 0);
    sleep_us(2);
    gpio_put(TM1637_CLK_PIN2, 1);
    sleep_us(2);
    gpio_put(TM1637_DIO_PIN2, 1);
}

void tm1637_write_byte_IR2(uint8_t b) {
    for (int i = 0; i < 8; i++) {
        gpio_put(TM1637_CLK_PIN2, 0);
        gpio_put(TM1637_DIO_PIN2, (b >> i) & 1);
        sleep_us(3);
        gpio_put(TM1637_CLK_PIN2, 1);
        sleep_us(3);
    }
    // Wait for ACK
    gpio_put(TM1637_CLK_PIN2, 0);
    gpio_set_dir(TM1637_DIO_PIN2, GPIO_IN);
    sleep_us(5);
    gpio_put(TM1637_CLK_PIN2, 1);
    sleep_us(5);
    gpio_set_dir(TM1637_DIO_PIN2, GPIO_OUT);
    gpio_put(TM1637_CLK_PIN2, 0);
}

void tm1637_display_digits_IR2(int d0, int d1, int d2, int d3, bool colon) {
    tm1637_start_IR2();
    tm1637_write_byte_IR2(0x40); // Set auto-increment mode
    tm1637_stop_IR2();

    tm1637_start_IR2();
    tm1637_write_byte_IR2(0xc0); // Start address 0
    tm1637_write_byte_IR2(digitToSegment[d0]);
    uint8_t seg1 = digitToSegment[d1];
    if (colon) seg1 |= 0x80; // Add colon if needed
    tm1637_write_byte_IR2(seg1);
    tm1637_write_byte_IR2(digitToSegment[d2]);
    tm1637_write_byte_IR2(digitToSegment[d3]);
    tm1637_stop_IR2();
}

void tm1637_set_brightness_IR2(uint8_t brightness) {
    tm1637_start_IR2();
    tm1637_write_byte_IR2(0x88 | (brightness & 0x07));
    tm1637_stop_IR2();
}

void tm1637_init_IR2() {
    gpio_init(TM1637_DIO_PIN2);
    gpio_init(TM1637_CLK_PIN2);
    gpio_set_dir(TM1637_DIO_PIN2, GPIO_OUT);
    gpio_set_dir(TM1637_CLK_PIN2, GPIO_OUT);

    gpio_put(TM1637_DIO_PIN2, 1);  // Start high
    gpio_put(TM1637_CLK_PIN2, 1);  // Start high
    sleep_ms(10);  // Give time for initialization
}