#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include <string.h>
#include "pico/binary_info.h"
#include "hardware/adc.h"
#include <math.h>

// Ultrasonic sensor I2C configuration
#define I2C_PORT i2c0
#define I2C_ADDR 0x35
#define SDA_PIN 16
#define SCL_PIN 17

// Function to read distance from the ultrasonic sensor
uint16_t read_distance_mm() {
    uint8_t reg = 0x05;
    uint8_t buf[2] = {0};

    i2c_write_blocking(I2C_PORT, I2C_ADDR, &reg, 1, true); 
    sleep_ms(100);
    i2c_read_blocking(I2C_PORT, I2C_ADDR, buf, 2, false);

    return (buf[0] << 8) | buf[1]; 
}

// Function to initialize the ultrasonic sensor
void ultra_init(){
    i2c_init(I2C_PORT, 100 * 1000); // Initialize I2C at 100kHz
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
}

// Function to run ultrasonic sensor speed measurement
void run_ultrasonic() {
    // Only initialize once
    static bool initialized = false;
    if (!initialized) {
        ultra_init();
        printf("Starting ultrasonic speed measurement...\n");
        sleep_ms(500); // Let sensor stabilize
        initialized = true;
    }

    // Static variables to keep state between calls
    static uint16_t prev_dist = 0;
    static absolute_time_t prev_time;
    static bool first_run = true;
    static float speed_sum = 0.0f;
    static uint32_t speed_count = 0;
    static float top_speed = 0.0f;
    static absolute_time_t last_measurement = {0};

    // Only measure every 400ms
    absolute_time_t now = get_absolute_time();
    if (!first_run && absolute_time_diff_us(last_measurement, now) < 400000) {
        return;
    }
    last_measurement = now;

    // Read current distance and time
    uint16_t curr_dist = read_distance_mm();
    absolute_time_t curr_time = get_absolute_time();

    // If this is the first run, initialize previous values
    if (first_run) {
        prev_dist = curr_dist;
        prev_time = curr_time;
        first_run = false;
        return;
    }

    // Calculate speed
    float delta_dist = (curr_dist - prev_dist) / 1000.0f; // mm to m
    float delta_time = absolute_time_diff_us(prev_time, curr_time) / 1e6f; // us to s

    // Calculate speed only if delta_time is non-zero
    float speed = 0.0f;
    if (delta_time > 0) {
        speed = delta_dist / delta_time; // m/s
    }

    // Convert speed to cm/s for output
    if (speed > 0.5f) {
        printf("Distance: %.1f cm, Speed: %.3f cm/s\n", curr_dist / 10.0f, speed);
        speed_sum += speed;
        speed_count++;
        if (speed > top_speed) {
            top_speed = speed;
        }
    } else {
        if (speed_count > 0) {
            float avg_speed = speed_sum / speed_count;
            if (avg_speed > 0.5f) {
                printf("Average Speed: %.3f cm/s\n", avg_speed);
                printf("Top speed: %.3f cm/s\n", top_speed);
            }
            speed_sum = 0.0f;
            speed_count = 0;
            // top_speed is NOT reset, so it maintains the highest speed seen
        }
    }
    // Update previous values for next iteration
    prev_dist = curr_dist;
    prev_time = curr_time;
}