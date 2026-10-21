/*
 * BKIT ARM4 - Lab 3 - Demo 1: Gửi và nhận CAN ở chế độ loopback
 *
 * Ở chế độ LOOPBACK, bộ điều khiển bxCAN nhận lại chính frame nó gửi,
 * nên chỉ cần MỘT kit, không cần đấu dây. Mỗi giây gửi một frame ID 0x123;
 * frame nhận được đi qua bộ lọc phần cứng vào message queue rx_msgq.
 */
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>

#define TX_ID 0x123

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* Message queue chứa tối đa 8 frame CAN (struct can_frame) */
CAN_MSGQ_DEFINE(rx_msgq, 8);

static void print_frame(const char *tag, const struct can_frame *f)
{
	printf("[%6u ms] %s ID=0x%03x DLC=%u DATA=", k_uptime_get_32(), tag, f->id, f->dlc);
	for (int i = 0; i < can_dlc_to_bytes(f->dlc); i++) {
		printf("%02x ", f->data[i]);
	}
	printf("\n");
}

/* Thread nhận: chờ frame trong rx_msgq, in ra và đảo LED3 */
static void thread_rx(void *p1, void *p2, void *p3)
{
	struct can_frame frame;

	while (1) {
		k_msgq_get(&rx_msgq, &frame, K_FOREVER);
		print_frame("RX", &frame);
		gpio_pin_toggle_dt(&led3);
	}
}
K_THREAD_DEFINE(rx_tid, 1024, thread_rx, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	/* Bộ lọc: chỉ nhận đúng ID chuẩn 0x123 (mask = so khớp cả 11 bit) */
	const struct can_filter filter = {
		.id = TX_ID,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	struct can_frame frame = {
		.id = TX_ID,
		.dlc = 8,
		.flags = 0,     /* frame dữ liệu, ID chuẩn 11 bit */
	};
	uint8_t counter = 0;
	int ret;

	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	printf("Lab 3 - Demo 1: CAN loopback\n");

	if (!device_is_ready(can_dev)) {
		printf("Loi: CAN chua san sang\n");
		return 0;
	}

	/* 1. Chọn chế độ (phải làm khi bộ điều khiển đang dừng) */
	ret = can_set_mode(can_dev, CAN_MODE_LOOPBACK);
	if (ret != 0) {
		printf("Loi can_set_mode: %d\n", ret);
		return 0;
	}

	/* 2. Gắn bộ lọc nhận vào message queue */
	ret = can_add_rx_filter_msgq(can_dev, &rx_msgq, &filter);
	if (ret < 0) {
		printf("Loi can_add_rx_filter_msgq: %d\n", ret);
		return 0;
	}
	printf("Filter #%d: ID=0x%03x MASK=0x%03x\n", ret, filter.id, filter.mask);

	/* 3. Khởi động bộ điều khiển CAN */
	ret = can_start(can_dev);
	if (ret != 0) {
		printf("Loi can_start: %d\n", ret);
		return 0;
	}

	while (1) {
		frame.data[0] = counter++;
		frame.data[1] = 0xCA;
		frame.data[2] = 0xFE;
		for (int i = 3; i < 8; i++) {
			frame.data[i] = i;
		}

		/* callback = NULL -> chặn cho tới khi frame được gửi xong */
		ret = can_send(can_dev, &frame, K_MSEC(100), NULL, NULL);
		if (ret != 0) {
			printf("Loi can_send: %d\n", ret);
		} else {
			print_frame("TX", &frame);
		}
		k_msleep(1000);
	}
	return 0;
}
