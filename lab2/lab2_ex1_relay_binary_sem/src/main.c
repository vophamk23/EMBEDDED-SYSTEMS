/*
 * BKIT ARM4 - Lab 2 - Ex 1: Relay LEDs with Binary Semaphores
 * Yeu cau: Chay tiep suc ba den bang binary semaphore (LED3 -> Y0 -> Y1, moi den 500 ms)
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Khai bao cac den tu Devicetree (app.overlay) */
#define LED0_NODE DT_ALIAS(led0) /* LED3   - PE4 */
#define LED1_NODE DT_ALIAS(led1) /* Out Y0 - PE5 */
#define LED2_NODE DT_ALIAS(led2) /* Out Y1 - PE6 */

static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec y0   = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec y1   = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

int main(void)
{
    printf("Lab 2 - Ex 1: Relay LEDs with Binary Semaphores san sang!\n");
    return 0;
}
