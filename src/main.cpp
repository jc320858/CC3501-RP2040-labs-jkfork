#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "drivers/loadcell.h"
#include "drivers/IR.h"
#include "drivers/ultrasonic.h"

#include "WS2812.pio.h" // This header file gets produced during compilation from the WS2812.pio file
#include "drivers/logging/logging.h"

#define LED_PIN 14

int main()
{
    stdio_init_all();

    hx711_init();    

    lc_calibrate();

    return 0;
}
