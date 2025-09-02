#include <stdio.h>
#include "pico/stdlib.h"
#include "zipsrc/ztypes.h"

int main()
{
    const uint LED_PIN = 25;

    stdio_init_all();

    // bi_decl(bi_program_description("First Blink"));
    // bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true)
    {
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
        gpio_put(LED_PIN, 1);

        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
