/*
 * BKIT ARM4 - Lab 2 - Demo 3: Mutex và race condition
 *
 * Hai thread cùng tăng một biến đếm dùng chung 1000 lần.
 * Kết quả đúng phải là 2000. Đặt USE_MUTEX = 0 để thấy lỗi race condition.
 */
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define USE_MUTEX 1
#define LOOPS     1000

static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec y0 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static unsigned int counter;      /* tài nguyên dùng chung */
K_MUTEX_DEFINE(counter_mutex);
K_SEM_DEFINE(done_sem, 0, 2);     /* mỗi worker give 1 lần khi xong */

static void thread_worker(void *p1, void *p2, void *p3)
{
	const char *name = p1;

	for (int i = 0; i < LOOPS; i++) {
#if USE_MUTEX
		k_mutex_lock(&counter_mutex, K_FOREVER);
#endif
		unsigned int tmp = counter;   /* 1. đọc   */
		k_yield();                    /*    bị chen ngang giữa chừng */
		counter = tmp + 1;            /* 2. ghi   */
#if USE_MUTEX
		k_mutex_unlock(&counter_mutex);
#endif
	}
	printf("%s xong\n", name);
	k_sem_give(&done_sem);
}
K_THREAD_DEFINE(worker_a, 1024, thread_worker, (void *)"Worker A", NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(worker_b, 1024, thread_worker, (void *)"Worker B", NULL, NULL, 7, 0, 0);

int main(void)
{
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);
	printf("Lab 2 - Demo 3: USE_MUTEX = %d\n", USE_MUTEX);

	/* Đợi cả hai worker hoàn tất */
	k_sem_take(&done_sem, K_FOREVER);
	k_sem_take(&done_sem, K_FOREVER);

	printf("counter = %u (mong doi %d) -> %s\n", counter, 2 * LOOPS,
	       counter == 2 * LOOPS ? "DUNG" : "SAI (race condition!)");

	/* LED3 sáng = kết quả đúng, Y0 sáng = kết quả sai */
	gpio_pin_set_dt(counter == 2 * LOOPS ? &led3 : &y0, 1);
	return 0;
}
