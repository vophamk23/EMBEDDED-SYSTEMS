/*
 * BKIT ARM4 - Lab 3 - Ex 5: Distributed CAN Traffic Light System
 * Yeu cau: Den giao thong phan tan qua CAN (Kit A dieu khien, Kit B hien thi)
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
    printf("Lab 3 - Ex 5: Distributed CAN Traffic Light System san sang!\n");
    return 0;
}
