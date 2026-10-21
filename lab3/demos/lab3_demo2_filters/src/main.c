/*
 * BKIT ARM4 - Lab 3 - Demo 2: Bộ lọc CAN (ID/mask, ID mở rộng, RTR, callback)
 *
 * Chế độ loopback. Gửi một loạt frame có ID khác nhau và quan sát
 * frame nào lọt qua bộ lọc nào:
 *   Bộ lọc A: ID chuẩn 0x100, mask 0x7F0 -> nhận 0x100..0x10F (callback/ISR)
 *   Bộ lọc B: ID mở rộng 0x18FF0001, so khớp đủ 29 bit   -> message queue
 */
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const struct gpio_dt_spec y0 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec y1 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

CAN_MSGQ_DEFINE(ext_msgq, 8);       /* frame của bộ lọc B */

/* Sự kiện ghi log do callback (ISR) gửi sang thread */
struct rx_evt {
	uint32_t id;
	uint8_t flags;
};
K_MSGQ_DEFINE(evt_q, sizeof(struct rx_evt), 16, 4);

static atomic_t cnt_a, cnt_b;

/* Callback của bộ lọc A: chạy trong ngữ cảnh ngắt -> chỉ ghi nhận rồi thoát */
static void filter_a_cb(const struct device *dev, struct can_frame *frame, void *user_data)
{
	struct rx_evt e = { .id = frame->id, .flags = frame->flags };

	atomic_inc(&cnt_a);
	k_msgq_put(&evt_q, &e, K_NO_WAIT);
}

static void thread_log_a(void *p1, void *p2, void *p3)
{
	struct rx_evt e;

	while (1) {
		k_msgq_get(&evt_q, &e, K_FOREVER);
		printf("    -> loc A (callback): ID=0x%03x %s\n", e.id,
		       (e.flags & CAN_FRAME_RTR) ? "[RTR]" : "");
		gpio_pin_toggle_dt(&y0);
	}
}
K_THREAD_DEFINE(log_a_tid, 1024, thread_log_a, NULL, NULL, NULL, 5, 0, 0);

static void thread_rx_b(void *p1, void *p2, void *p3)
{
	struct can_frame f;

	while (1) {
		k_msgq_get(&ext_msgq, &f, K_FOREVER);
		atomic_inc(&cnt_b);
		printf("    -> loc B (msgq):     ID=0x%08x (mo rong) DATA[0]=%02x\n", f.id, f.data[0]);
		gpio_pin_toggle_dt(&y1);
	}
}
K_THREAD_DEFINE(rx_b_tid, 1024, thread_rx_b, NULL, NULL, NULL, 5, 0, 0);

/* Danh sách frame thử: ID, cờ (IDE = mở rộng, RTR = yêu cầu dữ liệu) */
static const struct {
	uint32_t id;
	uint8_t flags;
} tests[] = {
	{0x100, 0}, {0x105, 0}, {0x10F, 0}, {0x110, 0}, {0x200, 0},
	{0x107, CAN_FRAME_RTR},
	{0x18FF0001, CAN_FRAME_IDE}, {0x18FF0002, CAN_FRAME_IDE},
	{0x100, CAN_FRAME_IDE},      /* cùng số 0x100 nhưng là ID mở rộng */
};

int main(void)
{
	const struct can_filter filter_a = {
		.id = 0x100, .mask = 0x7F0, .flags = 0,
	};
	const struct can_filter filter_b = {
		.id = 0x18FF0001, .mask = CAN_EXT_ID_MASK, .flags = CAN_FILTER_IDE,
	};

	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&y1, GPIO_OUTPUT_INACTIVE);
	printf("Lab 3 - Demo 2: CAN filters\n");

	if (!device_is_ready(can_dev) || can_set_mode(can_dev, CAN_MODE_LOOPBACK) != 0) {
		printf("Loi khoi tao CAN\n");
		return 0;
	}
	can_add_rx_filter(can_dev, filter_a_cb, NULL, &filter_a);
	can_add_rx_filter_msgq(can_dev, &ext_msgq, &filter_b);
	can_start(can_dev);

	for (int round = 1; ; round++) {
		printf("===== Vong %d =====\n", round);
		for (int i = 0; i < ARRAY_SIZE(tests); i++) {
			struct can_frame f = {
				.id = tests[i].id,
				.flags = tests[i].flags,
				.dlc = (tests[i].flags & CAN_FRAME_RTR) ? 0 : 1,
			};

			f.data[0] = i;
			if (tests[i].flags & CAN_FRAME_IDE) {
				printf("TX ID=0x%08x (mo rong)\n", f.id);
			} else {
				printf("TX ID=0x%03x %s\n", f.id, (f.flags & CAN_FRAME_RTR) ? "[RTR]" : "");
			}
			can_send(can_dev, &f, K_MSEC(100), NULL, NULL);
			k_msleep(300);
		}
		printf("Tong: loc A = %ld, loc B = %ld, bi loai = %ld\n",
		       atomic_get(&cnt_a), atomic_get(&cnt_b),
		       (long)(round * ARRAY_SIZE(tests)) - atomic_get(&cnt_a) - atomic_get(&cnt_b));
		k_msleep(5000);
	}
	return 0;
}
