/*
 * BKIT ARM4 - Lab 1: Blinky với Zephyr RTOS
 * Nháy LED3 (LED DEBUG, chân PE4): sáng 1 giây, tắt 1 giây.
 */
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Thời gian giữa hai lần đảo trạng thái LED (ms) */
#define SLEEP_TIME_MS 1000

/* Lấy node LED từ bí danh "led0" trong devicetree (app.overlay) */
#define LED0_NODE DT_ALIAS(led0)

/* gpio_dt_spec gom đủ thông tin: port (GPIOE), chân (4), cờ (ACTIVE_HIGH) */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	int ret;
	bool led_state = true;

	/* Kiểm tra driver GPIO của port đã sẵn sàng chưa */
	if (!gpio_is_ready_dt(&led)) {
		printf("Loi: GPIO cua LED chua san sang\n");
		return 0;
	}

	/* Cấu hình chân là output, trạng thái ban đầu: LED sáng */
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printf("Loi: khong cau hinh duoc chan LED (%d)\n", ret);
		return 0;
	}

	printf("BKIT ARM4 - Zephyr Blinky bat dau!\n");

	while (1) {
		printf("LED3: %s\n", led_state ? "ON" : "OFF");

		k_msleep(SLEEP_TIME_MS);      /* Nhường CPU trong 1000 ms */

		ret = gpio_pin_toggle_dt(&led); /* Đảo trạng thái LED */
		if (ret < 0) {
			return 0;
		}
		led_state = !led_state;
	}
	return 0;
}
