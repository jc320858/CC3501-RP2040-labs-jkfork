#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "drivers/loadcell.h"
#include "drivers/IR.h"
#include "drivers/ultrasonic.h"

#include "WS2812.pio.h" // This header file gets produced during compilation from the WS2812.pio file
#include "drivers/logging/logging.h"

#define BUTTON_PIN 15  // Change to your button's GPIO pin

#define UART_ID uart0
#define BAUD_RATE 115200
#define TX_PIN 0
#define RX_PIN 1

volatile bool button_pressed = false;

// Interrupt handler function
void button_irq_handler(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN && (events & GPIO_IRQ_EDGE_RISE)) {
        button_pressed = true;
    }
}

int main() {
    stdio_init_all();
    hx711_init();  
    ultra_init();
    tm1637_init();

    // Initialize UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);


    // Initialize button GPIO
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);

    // Set up interrupt on rising edge (button press)
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, &button_irq_handler);

    int mode = 0; // State variable for toggling functions
    const int NUM_MODES = 2; // Change this to the number of functions you want to toggle

     while (true) {
        if (button_pressed) {
            button_pressed = false;
            mode = (mode + 1) % NUM_MODES; // Cycle through modes
            printf("Button pressed! Switched to mode %d\n", mode);
            
            // Print mode name once when switching
            if (mode == 0) {
                printf("Entering Weighing mode\n");
            } else {
                printf("Entering Race mode\n");
            }
        }

        // Call different functions based on mode (without printing mode repeatedly)
        switch (mode) {
            case 0:
                lc_calibrate_send();
                break;
            case 1:
                run_ultrasonic();
                run_IR();
                break;
        }
        sleep_ms(10); // Debounce delay
    }
}

      

    
