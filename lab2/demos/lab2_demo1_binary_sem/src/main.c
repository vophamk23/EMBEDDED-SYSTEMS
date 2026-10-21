/*
 * BKIT ARM4 - Lab 2 - Demo 1: Binary semaphore (ngắt -> thread)
 *
 * Timer phần cứng của kernel (k_timer) tạo "ngắt" mỗi 2 giây.
 * Hàm ngắt chỉ GIVE semaphore; thread xử lý TAKE semaphore rồi
 * nháy LED3 ba lần. Kỹ thuật này gọi là "deferred interrupt processing".
 */
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* Binary semaphore: giá trị ban đầu 0, giá trị tối đa 1 */
K_SEM_DEFINE(evt_sem, 0, 1);

/* Callback của timer: chạy trong ngữ cảnh ngắt (ISR) -> phải thật ngắn */
static void timer_isr(struct k_timer *timer)
{
	k_sem_give(&evt_sem);
}
K_TIMER_DEFINE(evt_timer, timer_isr, NULL);

static void thread_handler(void *p1, void *p2, void *p3)
{
	unsigned int n = 0;

	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

	while (1) {
		/* Ngủ (không tốn CPU) cho tới khi ISR give semaphore */
		k_sem_take(&evt_sem, K_FOREVER);
		n++;
		printf("[%6u ms] Su kien #%u -> thread xu ly\n", k_uptime_get_32(), n);

		for (int i = 0; i < 3; i++) {
			gpio_pin_set_dt(&led3, 1);
			k_msleep(100);
			gpio_pin_set_dt(&led3, 0);
			k_msleep(100);
		}
	}
}
K_THREAD_DEFINE(handler_tid, 1024, thread_handler, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	printf("Lab 2 - Demo 1: Binary semaphore (ISR -> thread)\n");
	/* Hết hạn lần đầu sau 2 s, sau đó lặp lại mỗi 2 s */
	k_timer_start(&evt_timer, K_SECONDS(2), K_SECONDS(2));
	return 0;
}
