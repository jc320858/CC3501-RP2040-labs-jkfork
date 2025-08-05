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

#define TM1637_DIO_PIN2 11
#define TM1637_CLK_PIN2 12

#define BBIF_PIN 4

void tm1637_init_IR2();
void tm1637_start_IR2();
void tm1637_stop_IR2();
void tm1637_write_byte_IR2(uint8_t b);
void tm1637_display_digits_IR2(int d0, int d1, int d2, int d3, bool colon);
void tm1637_set_brightness_IR2(uint8_t brightness);



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
    
    // Variables for last lap time - make sure these persist
    static int last_lap_minutes = -1;  // Use -1 to indicate no lap yet
    static int last_lap_seconds = -1;
    static bool has_lap_time = false;
    static int total_laps = 0;  // Track total laps for debugging

    bool beam_present = gpio_get(BBIF_PIN);

    // Beam break detection with debouncing
    static absolute_time_t last_beam_change = {0};
    static const uint32_t DEBOUNCE_TIME_US = 50000; // 50ms debounce
    
    if (last_beam != beam_present) {
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last_beam_change, now) > DEBOUNCE_TIME_US) {
            last_beam_change = now;
            
            if (last_beam && !beam_present) { // Beam just broken
                if (!timing) {
                    // Start timer
                    timing = true;
                    minutes = 0;
                    seconds = 0;
                    last_tick = get_absolute_time();
                    printf("Car passed! Timer started.\n");
                } else {
                    // Lap completed
                    total_laps++;
                    printf("=== LAP %d COMPLETED ===\n", total_laps);
                    printf("Lap time: %02d:%02d\n", minutes, seconds);
                    
                    // Save the lap time - these should persist!
                    last_lap_minutes = minutes;
                    last_lap_seconds = seconds;
                    has_lap_time = true;
                    
                    printf("Stored lap time: %02d:%02d (has_lap_time=%s)\n", 
                           last_lap_minutes, last_lap_seconds, 
                           has_lap_time ? "TRUE" : "FALSE");
                    
                    // Reset timer for next lap
                    minutes = 0;
                    seconds = 0;
                    last_tick = get_absolute_time();
                    printf("Timer reset for next lap.\n");
                    printf("========================\n");
                }
            }
        }
        last_beam = beam_present;
    }

    // Timer increment
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
    
    // Display on second screen - CRITICAL: This should maintain lap time
    if (has_lap_time && last_lap_minutes >= 0 && last_lap_seconds >= 0) {
        // Show the stored lap time
        int ld0 = last_lap_minutes / 10;
        int ld1 = last_lap_minutes % 10;
        int ld2 = last_lap_seconds / 10;
        int ld3 = last_lap_seconds % 10;
        tm1637_display_digits_IR2(ld0, ld1, ld2, ld3, true);
        
        // Debug output every 5 seconds to verify persistence
        static absolute_time_t last_debug = {0};
        if (absolute_time_diff_us(last_debug, get_absolute_time()) >= 5000000) {
            printf("Second display: %02d:%02d (stored: %02d:%02d, total_laps=%d)\n", 
                   ld0*10+ld1, ld2*10+ld3, last_lap_minutes, last_lap_seconds, total_laps);
            last_debug = get_absolute_time();
        }
    } else {
        // Show dashes or 00:00 until first lap
        tm1637_display_digits_IR2(0, 0, 0, 0, false);
    }
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
    // Set auto-increment mode
    tm1637_start_IR2();
    tm1637_write_byte_IR2(0x40);
    tm1637_stop_IR2();

    // Write data to display
    tm1637_start_IR2();
    tm1637_write_byte_IR2(0xc0); // Start address 0
    tm1637_write_byte_IR2(digitToSegment[d0]);
    uint8_t seg1 = digitToSegment[d1];
    if (colon) seg1 |= 0x80; // Add colon if needed
    tm1637_write_byte_IR2(seg1);
    tm1637_write_byte_IR2(digitToSegment[d2]);
    tm1637_write_byte_IR2(digitToSegment[d3]);
    tm1637_stop_IR2();
    
    // Set display control (brightness + on)
    tm1637_start_IR2();
    tm1637_write_byte_IR2(0x8F); // Display ON + max brightness
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

