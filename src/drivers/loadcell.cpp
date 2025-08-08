#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include <string.h>
#include "pico/binary_info.h"
#include "hardware/adc.h"

// Load cell and display configuration
#define HX711_DOUT_PIN 2
#define HX711_SCK_PIN 3
#define DISPLAY_DIO_PIN 18
#define DISPLAY_CLK_PIN 19
#define BBIF_PIN 4

// UART configuration
#define UART_ID uart0
#define BAUD_RATE 115200
#define TX_PIN 0
#define RX_PIN 1

// Add function declarations here
void tm1637_init();
void tm1637_start();
void tm1637_stop();
void tm1637_write_byte(uint8_t data);
void display_weight(float weight_kg);

// Loadcell initialisation
void hx711_init() {
    gpio_init(HX711_DOUT_PIN);
    gpio_init(HX711_SCK_PIN);
    gpio_set_dir(HX711_DOUT_PIN, GPIO_IN);
    gpio_set_dir(HX711_SCK_PIN, GPIO_OUT);
    gpio_put(HX711_SCK_PIN, 0);
}

// Function to read data from HX711 load cell, returns raw value
uint32_t hx711_read() {
    uint32_t data = 0;
    
    // Wait for HX711 to be ready
    while (gpio_get(HX711_DOUT_PIN)) {
        sleep_us(1);
    }
    
    // Read 24 bits
    for (int i = 0; i < 24; i++) {
        gpio_put(HX711_SCK_PIN, 1);
        sleep_us(1);
        data = (data << 1) | gpio_get(HX711_DOUT_PIN);
        gpio_put(HX711_SCK_PIN, 0);
        sleep_us(1);
    }
    
    // Set gain (1 extra pulse = 128 gain)
    gpio_put(HX711_SCK_PIN, 1);
    sleep_us(1);
    gpio_put(HX711_SCK_PIN, 0);
    
    // Convert to signed if needed
    if (data & 0x800000) {
        data |= 0xFF000000;
    }
    
    return data;
}

// Function to convert raw HX711 value to weight in kg
float hx711_get_weight_kg(uint32_t raw_value, uint32_t zero_offset, float scale_factor) {
    return (float)(raw_value - zero_offset) / scale_factor;
}

// Add these global variables after your defines
static uint32_t tare_offset = 0;
static float calibration_factor = 1.0f;

// Calibration function - call this to zero/tare the scale
void lc_calibrate_tare() {
    printf("Calibrating tare (zero point)...\n");
    printf("Remove all weight from the scale and press any key...\n");
    getchar(); // Wait for user input
    
    // Take multiple readings and average them
    uint32_t sum = 0;
    int samples = 10;
    
    // Take multiple readings to get a stable tare offset
    for (int i = 0; i < samples; i++) {
        sum += hx711_read();
        sleep_ms(100);
    }
    
    tare_offset = sum / samples;
    printf("Tare complete. Offset: %lu\n", tare_offset);
}

// Calibration function - call this to set the scale factor
void lc_calibrate_scale(float known_weight_kg) {
    printf("Calibrating scale factor...\n");
    printf("Place %.2f kg on the scale and press any key...\n", known_weight_kg);
    getchar(); // Wait for user input
    
    // Take multiple readings and average them
    uint32_t sum = 0;
    int samples = 10;
    
    //
    for (int i = 0; i < samples; i++) {
        sum += hx711_read();
        sleep_ms(100);
    }
    
    // Calculate calibration factor
    uint32_t loaded_reading = sum / samples;
    calibration_factor = (float)(loaded_reading - tare_offset) / known_weight_kg;
    
    printf("Scale calibration complete. Factor: %.2f\n", calibration_factor);
}

// Updated weight reading function
float lc_get_weight_kg() {
    uint32_t raw_reading = hx711_read();
    return (float)(raw_reading - tare_offset) / calibration_factor;
}

// Complete calibration function
void lc_calibrate() {
    printf("Load Cell Test Program\n");
    printf("Press 'c' to calibrate, or any other key to start reading...\n");
    
    gpio_init(DISPLAY_CLK_PIN);
    gpio_init(DISPLAY_DIO_PIN);
    gpio_set_dir(DISPLAY_CLK_PIN, GPIO_OUT);
    gpio_set_dir(DISPLAY_DIO_PIN, GPIO_OUT);
    gpio_put(DISPLAY_CLK_PIN, 1);
    gpio_put(DISPLAY_DIO_PIN, 1);
     
    char input = getchar();
    if (input == 'c' || input == 'C') {
        printf("Starting load cell calibration...\n");
        
        // Step 1: Tare (zero point)
        lc_calibrate_tare();
        
        // Step 2: Scale factor with known weight
        printf("Enter the weight of your calibration object in kg: ");
        float known_weight;
        scanf("%f", &known_weight);
        
        lc_calibrate_scale(known_weight);
        
        printf("Calibration complete!\n");
    }

    sleep_ms(2000);

    while (1) {
        float weight_kg = lc_get_weight_kg();
        printf("Weight: %.3f kg\n", weight_kg);
        
        // Display weight on 7-segment display
        display_weight(weight_kg);
        
        sleep_ms(500);
    }
}

// TM1637 7-segment display functions
// Initialize the TM1637 display
void tm1637_init() {
    gpio_init(DISPLAY_CLK_PIN);
    gpio_init(DISPLAY_DIO_PIN);
    gpio_set_dir(DISPLAY_CLK_PIN, GPIO_OUT);
    gpio_set_dir(DISPLAY_DIO_PIN, GPIO_OUT);
    gpio_put(DISPLAY_CLK_PIN, 1);
    gpio_put(DISPLAY_DIO_PIN, 1);
}

// Start communication with the TM1637 display
void tm1637_start() {
    gpio_put(DISPLAY_DIO_PIN, 0);
    sleep_us(50);
}

// Stop communication with the TM1637 display
void tm1637_stop() {
    gpio_put(DISPLAY_CLK_PIN, 0);
    sleep_us(50);
    gpio_put(DISPLAY_DIO_PIN, 0);
    sleep_us(50);
    gpio_put(DISPLAY_CLK_PIN, 1);
    sleep_us(50);
    gpio_put(DISPLAY_DIO_PIN, 1);
}

// Write a byte to the TM1637 display
void tm1637_write_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        gpio_put(DISPLAY_CLK_PIN, 0);
        if (data & 0x01) {
            gpio_put(DISPLAY_DIO_PIN, 1);
        } else {
            gpio_put(DISPLAY_DIO_PIN, 0);
        }
        sleep_us(50);
        data >>= 1;
        gpio_put(DISPLAY_CLK_PIN, 1);
        sleep_us(50);
    }
    
    // ACK
    gpio_put(DISPLAY_CLK_PIN, 0);
    gpio_put(DISPLAY_DIO_PIN, 1);
    sleep_us(50);
    gpio_put(DISPLAY_CLK_PIN, 1);
    sleep_us(50);
    gpio_put(DISPLAY_CLK_PIN, 0);
}

// Display weight on the TM1637 7-segment display
void display_weight(float weight_kg) {
    // Convert weight to display format (e.g., 1.234 kg -> show "1234")
    int weight_display = (int)(fabs(weight_kg) * 1000); // Show as grams (multiply by 1000)
    
    // Handle negative weights
    bool negative = weight_kg < 0;
    
    // Digit encoding for 7-segment (0-9)
    uint8_t digits[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
    uint8_t minus_sign = 0x40; // Minus sign encoding
    
    tm1637_start();
    tm1637_write_byte(0x40); // Data command
    tm1637_stop();
    
    tm1637_start();
    tm1637_write_byte(0xC0); // Address command
    
    // Display format: XXXX (no decimal points, showing weight in grams)
    if (negative && weight_display < 10000) {
        tm1637_write_byte(minus_sign);  // Show minus sign
        tm1637_write_byte(digits[(weight_display / 100) % 10]); // Hundreds
        tm1637_write_byte(digits[(weight_display / 10) % 10]);  // Tens
        tm1637_write_byte(digits[weight_display % 10]);         // Units
    } else {
        tm1637_write_byte(digits[(weight_display / 1000) % 10]); // Thousands
        tm1637_write_byte(digits[(weight_display / 100) % 10]);  // Hundreds
        tm1637_write_byte(digits[(weight_display / 10) % 10]);   // Tens
        tm1637_write_byte(digits[weight_display % 10]);          // Units
    }
    
    tm1637_stop();
    
    tm1637_start();
    tm1637_write_byte(0x8F); // Display control (brightness)
    tm1637_stop();
}

// Function to send load cell data periodically
// This function will be called in the main loop to send data every 15 seconds
static bool lc_send_initialized = false;
static absolute_time_t last_send_time = {0};
static const uint32_t SEND_INTERVAL_MS = 15000; // 15 seconds

// Function to send load cell data over UART
void lc_calibrate_send() {
    // One-time initialization
    if (!lc_send_initialized) {
        printf("Load Cell Test Program\n");
        printf("Press 'c' to calibrate, or any other key to start reading...\n");
        
        // Initialize display
        gpio_init(DISPLAY_CLK_PIN);
        gpio_init(DISPLAY_DIO_PIN);
        gpio_set_dir(DISPLAY_CLK_PIN, GPIO_OUT);
        gpio_set_dir(DISPLAY_DIO_PIN, GPIO_OUT);
        gpio_put(DISPLAY_CLK_PIN, 1);
        gpio_put(DISPLAY_DIO_PIN, 1);
        
        // Initialize UART
        char input = getchar();
        if (input == 'c' || input == 'C') {
            printf("Starting load cell calibration...\n");
            
            // Step 1: Tare (zero point)
            lc_calibrate_tare();
            
            // Step 2: Scale factor with known weight
            printf("Enter the weight of your calibration object in kg: ");
            float known_weight;
            scanf("%f", &known_weight);
            
            lc_calibrate_scale(known_weight);
            
            printf("Calibration complete!\n");
        }

        printf("Starting weight measurements and UART transmission...\n");
        printf("Sending readings every 15 seconds...\n");
        
        // Initialize last send time
        last_send_time = get_absolute_time();
        lc_send_initialized = true;
        return; // Exit to allow button checking
    }
    
    // Check if it's time for next measurement and transmission
    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(last_send_time, now) >= SEND_INTERVAL_MS * 1000) {
        float weight_kg = lc_get_weight_kg();
        
        // Display locally
        printf("Weight: %.3f kg\n", weight_kg);
        
        // Send JSON data to Raspberry Pi
        char json_buffer[128];
        snprintf(json_buffer, sizeof(json_buffer), 
                "{\"weight\":%.3f,\"unit\":\"kg\",\"timestamp\":%lu}\n", 
                weight_kg, to_ms_since_boot(get_absolute_time()));
        uart_puts(UART_ID, json_buffer);
        
        printf("Sent to Pi: %s", json_buffer);
        
        // Display weight on 7-segment display
        display_weight(weight_kg);
        
        printf("Next reading in 15 seconds...\n");
        last_send_time = now;
    }
}