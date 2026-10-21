/*
 * BKIT ARM4 - Lab 2 - Demo 4: Message queue (producer -> consumer)
 *
 * Thread "producer" gửi các lệnh điều khiển LED vào hàng đợi.
 * Thread "consumer" (driver LED) lấy lệnh ra và thực thi theo thứ tự FIFO.
 */
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Một phần tử của hàng đợi: bật đèn 'led' trong 'ms' mili-giây */
struct led_cmd {
	uint8_t led;      /* 0 = LED3, 1 = Y0, 2 = Y1 */
	uint16_t ms;
};

/* Hàng đợi chứa tối đa 4 lệnh, mỗi lệnh sizeof(struct led_cmd) byte */
K_MSGQ_DEFINE(cmd_q, sizeof(struct led_cmd), 4, 4);

static const struct gpio_dt_spec leds[3] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};
static const char *const names[3] = { "LED3", "Y0", "Y1" };

static void thread_producer(void *p1, void *p2, void *p3)
{
	/* Mẫu "chạy đuổi": LED3 -> Y0 -> Y1 -> Y0 -> LED3 ... */
	static const struct led_cmd pattern[] = {
		{0, 300}, {1, 300}, {2, 300}, {1, 300}, {0, 300}, {2, 1000},
	};

	while (1) {
		for (int i = 0; i < ARRAY_SIZE(pattern); i++) {
			/* Hàng đợi đầy -> producer bị chặn tới khi có chỗ trống */
			k_msgq_put(&cmd_q, &pattern[i], K_FOREVER);
			printf("[P] gui lenh %d (%s) | trong hang doi: %u/4\n",
			       i, names[pattern[i].led], k_msgq_num_used_get(&cmd_q));
		}
		k_msleep(2000);
	}
}

static void thread_consumer(void *p1, void *p2, void *p3)
{
	struct led_cmd cmd;

	for (int i = 0; i < 3; i++) {
		gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
	}

	while (1) {
		k_msgq_get(&cmd_q, &cmd, K_FOREVER);   /* rỗng -> chờ */
		printf("    [C] thuc thi: %s sang %u ms\n", names[cmd.led], cmd.ms);
		gpio_pin_set_dt(&leds[cmd.led], 1);
		k_msleep(cmd.ms);
		gpio_pin_set_dt(&leds[cmd.led], 0);
	}
}

K_THREAD_DEFINE(producer_tid, 1024, thread_producer, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(consumer_tid, 1024, thread_consumer, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	printf("Lab 2 - Demo 4: Message queue\n");
	return 0;
}
