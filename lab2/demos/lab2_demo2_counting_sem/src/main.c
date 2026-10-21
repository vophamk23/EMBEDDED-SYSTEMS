/*
 * BKIT ARM4 - Lab 2 - Demo 2: Counting semaphore (đếm sự kiện)
 *
 * Mỗi đợt (burst), timer tạo 15 sự kiện cách nhau 300 ms -> nhanh hơn
 * tốc độ xử lý của thread (1000 ms/sự kiện). Counting semaphore "ghi nhớ"
 * các sự kiện chưa xử lý, tối đa EVT_LIMIT sự kiện; vượt quá thì bị mất.
 */
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define EVT_LIMIT 5

static const struct gpio_dt_spec y0 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* Counting semaphore: ban đầu 0, đếm tối đa EVT_LIMIT */
K_SEM_DEFINE(evt_count, 0, EVT_LIMIT);

static volatile unsigned int produced;
static volatile unsigned int lost;

static void timer_isr(struct k_timer *timer)
{
	produced++;
	if (k_sem_count_get(&evt_count) >= EVT_LIMIT) {
		lost++;                 /* semaphore đã đầy: give sẽ không có tác dụng */
	}
	k_sem_give(&evt_count);
}
K_TIMER_DEFINE(evt_timer, timer_isr, NULL);

static void thread_consumer(void *p1, void *p2, void *p3)
{
	unsigned int done = 0;

	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);
	while (1) {
		k_sem_take(&evt_count, K_FOREVER);
		done++;
		printf("  xu ly #%2u | con cho: %u | da tao: %u | bi mat: %u\n",
		       done, k_sem_count_get(&evt_count), produced, lost);
		gpio_pin_set_dt(&y0, 1);    /* Y0 sáng = đang xử lý 1 sự kiện */
		k_msleep(700);
		gpio_pin_set_dt(&y0, 0);
		k_msleep(300);
	}
}
K_THREAD_DEFINE(consumer_tid, 1024, thread_consumer, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	printf("Lab 2 - Demo 2: Counting semaphore (limit = %d)\n", EVT_LIMIT);

	while (1) {
		printf("--- Burst: 15 su kien, moi 300 ms ---\n");
		gpio_pin_set_dt(&led3, 1);  /* LED3 sáng trong lúc phát sự kiện */
		k_timer_start(&evt_timer, K_MSEC(300), K_MSEC(300));
		k_msleep(4500 + 100);       /* 15 lần hết hạn */
		k_timer_stop(&evt_timer);
		gpio_pin_set_dt(&led3, 0);
		k_msleep(10000);            /* nghỉ để consumer xử lý hết */
	}
	return 0;
}
