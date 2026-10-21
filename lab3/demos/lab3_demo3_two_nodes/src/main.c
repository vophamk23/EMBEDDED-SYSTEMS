/*
 * BKIT ARM4 - Lab 3 - Demo 3: Hai node trên bus CAN thật + giám sát trạng thái bus
 *
 * Nạp chương trình này cho HAI kit, đổi NODE_ID (1 cho kit A, 2 cho kit B).
 * Mỗi node gửi heartbeat ID 0x700 + NODE_ID mỗi giây và nhận heartbeat của node kia.
 * SELF_TEST = 1: chạy loopback trên một kit (tự nhận heartbeat của chính mình).
 */
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>

#define NODE_ID   1          /* kit A = 1, kit B = 2 */
#define SELF_TEST 0          /* 1 = loopback một kit, 0 = bus thật hai kit */
#define HB_BASE   0x700

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec y0 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

CAN_MSGQ_DEFINE(hb_msgq, 8);

/* Sự kiện đổi trạng thái bus: callback (ISR) -> hàng đợi -> thread in log */
struct state_evt {
	enum can_state state;
	struct can_bus_err_cnt err;
};
K_MSGQ_DEFINE(state_q, sizeof(struct state_evt), 8, 4);

static const char *state_name(enum can_state s)
{
	switch (s) {
	case CAN_STATE_ERROR_ACTIVE:  return "ERROR-ACTIVE";
	case CAN_STATE_ERROR_WARNING: return "ERROR-WARNING";
	case CAN_STATE_ERROR_PASSIVE: return "ERROR-PASSIVE";
	case CAN_STATE_BUS_OFF:       return "BUS-OFF";
	default:                      return "STOPPED";
	}
}

static void state_change_cb(const struct device *dev, enum can_state state,
			    struct can_bus_err_cnt err_cnt, void *user_data)
{
	struct state_evt e = { .state = state, .err = err_cnt };

	k_msgq_put(&state_q, &e, K_NO_WAIT);
}

/* Callback khi gửi xong (ISR): chỉ lưu mã lỗi gần nhất */
static atomic_t last_tx_err;
static void tx_done_cb(const struct device *dev, int error, void *user_data)
{
	atomic_set(&last_tx_err, error);
}

static void thread_heartbeat_tx(void *p1, void *p2, void *p3)
{
	struct can_frame f = { .id = HB_BASE + NODE_ID, .dlc = 2 };
	uint8_t seq = 0;

	while (1) {
		f.data[0] = NODE_ID;
		f.data[1] = seq++;
		/* Không chặn: nếu 3 hộp thư TX đều đầy (không ai ACK) -> -EAGAIN sau 100 ms */
		int ret = can_send(can_dev, &f, K_MSEC(100), tx_done_cb, NULL);

		if (ret != 0) {
			printf("[%6u ms] TX heartbeat that bai: %d\n", k_uptime_get_32(), ret);
		}
		k_msleep(1000);
	}
}
K_THREAD_DEFINE(hb_tx_tid, 1024, thread_heartbeat_tx, NULL, NULL, NULL, 6, 0, 500);

static void thread_heartbeat_rx(void *p1, void *p2, void *p3)
{
	struct can_frame f;

	while (1) {
		/* Chờ heartbeat tối đa 3 s; quá hạn -> coi như mất node kia */
		if (k_msgq_get(&hb_msgq, &f, K_SECONDS(3)) == 0) {
			printf("[%6u ms] RX heartbeat tu node %u, seq=%u\n",
			       k_uptime_get_32(), f.data[0], f.data[1]);
			gpio_pin_toggle_dt(&y0);
		} else {
			printf("[%6u ms] !!! Khong nhan duoc heartbeat trong 3 s\n", k_uptime_get_32());
		}
	}
}
K_THREAD_DEFINE(hb_rx_tid, 1024, thread_heartbeat_rx, NULL, NULL, NULL, 5, 0, 500);

static void thread_state_log(void *p1, void *p2, void *p3)
{
	struct state_evt e;

	while (1) {
		k_msgq_get(&state_q, &e, K_FOREVER);
		printf("[%6u ms] Trang thai bus: %s (TEC=%u, REC=%u)\n", k_uptime_get_32(),
		       state_name(e.state), e.err.tx_err_cnt, e.err.rx_err_cnt);
		/* LED3 sáng khi bus không còn ở trạng thái bình thường */
		gpio_pin_set_dt(&led3, e.state != CAN_STATE_ERROR_ACTIVE);
	}
}
K_THREAD_DEFINE(state_tid, 1024, thread_state_log, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	/* Nhận mọi heartbeat 0x700..0x70F */
	const struct can_filter hb_filter = { .id = HB_BASE, .mask = 0x7F0, .flags = 0 };

	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);
	printf("Lab 3 - Demo 3: NODE_ID=%d, %s\n", NODE_ID, SELF_TEST ? "loopback" : "bus that");

	if (!device_is_ready(can_dev)) {
		printf("Loi: CAN chua san sang\n");
		return 0;
	}
	can_set_mode(can_dev, SELF_TEST ? CAN_MODE_LOOPBACK : CAN_MODE_NORMAL);
	can_set_state_change_callback(can_dev, state_change_cb, NULL);
	can_add_rx_filter_msgq(can_dev, &hb_msgq, &hb_filter);
	can_start(can_dev);
	return 0;
}
