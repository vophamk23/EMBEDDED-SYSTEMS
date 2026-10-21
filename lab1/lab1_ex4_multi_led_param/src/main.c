/*
 * ============================================================================
 * TRƯỜNG ĐẠI HỌC BÁCH KHOA - ĐHQG-HCM
 * KHOA KHOA HỌC VÀ KỸ THUẬT MÁY TÍNH
 * MÔN HỌC: HỆ THỐNG NHÚNG (EMBEDDED SYSTEMS)
 *
 * LAB 1 - BÀI 4: MỘT HÀM THREAD — NHIỀU ĐÈN (TRUYỀN THAM SỐ CHO THREAD)
 * Nền tảng : Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 168 MHz)
 * Framework: Zephyr RTOS v4.4.0 + VS Code
 * ============================================================================
 *
 * 1. ĐỀ BÀI (YÊU CẦU BÀI TẬP):
 * - Định nghĩa struct blink_cfg chứa thông số cấu hình:
 *   + struct gpio_dt_spec led: Cấu hình chân GPIO điều khiển.
 *   + int on_ms              : Thời gian đèn sáng (ms).
 *   + int off_ms             : Thời gian đèn tắt (ms).
 *   + const char *name       : Tên đại diện in ra màn hình console.
 * - Viết DUY NHẤT một hàm thực thi thread:
 *     void thread_blink(void *p1, void *p2, void *p3);
 *   Hàm này nhận con trỏ struct blink_cfg thông qua tham số p1 và điều khiển
 * đèn tương ứng.
 * - Gọi macro K_THREAD_DEFINE đúng 3 lần với 3 cấu hình đèn khác nhau:
 *   + Thread 1 (LED3): Sáng 2s (2000 ms), Tắt 2s (2000 ms).
 *   + Thread 2 (Y0)  : Sáng 0.5s (500 ms), Tắt 0.5s (500 ms).
 *   + Thread 3 (Y1)  : Sáng 3s (3000 ms), Tắt 1s (1000 ms).
 *
 * 2. TRẢ LỜI CÂU HỎI BÁO CÁO: "VÌ SAO KHÔNG CẦN BA HÀM THREAD RIÊNG?"
 * --> TRẢ LỜI:
 * - Bản chất logic của cả 3 đèn là hoàn toàn đồng nhất: Đều lặp vô hạn chu
 * trình [Bật đèn -> k_msleep(on_ms) -> Tắt đèn -> k_msleep(off_ms)].
 * - Điểm khác biệt duy nhất chỉ là THAM SỐ DỮ LIỆU (chân GPIO nào và thời gian
 * bao lâu).
 * - Nếu viết 3 hàm riêng, mã máy sinh ra bị trùng lặp 3 lần, gây lãng phí bộ
 * nhớ Flash của chip.
 * - Bằng cách gom dữ liệu vào struct và truyền con trỏ qua tham số p1 của
 * K_THREAD_DEFINE, cả 3 thread dùng chung một khối mã lệnh (Code Segment) nhưng
 * mỗi thread có vùng Stack RAM và con trỏ ngữ cảnh riêng. Kỹ thuật này giúp mã
 * nguồn cô đọng, tối ưu tài nguyên và dễ mở rộng.
 *
 * 3. ÁNH XẠ PHẦN CỨNG (KIT BKIT ARM4):
 * - LED3      : Chân PE4, alias led0 -> gpios = <&gpioe 4 GPIO_ACTIVE_HIGH>
 * - Output Y0 : Chân PE5, alias led1 -> gpios = <&gpioe 5 GPIO_ACTIVE_HIGH>
 * - Output Y1 : Chân PE6, alias led2 -> gpios = <&gpioe 6 GPIO_ACTIVE_HIGH>
 * ============================================================================
 */

#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define STACK_SIZE 1024 /* Stack riêng 1024 bytes cho mỗi thread */
#define PRIORITY 7      /* Cả 3 thread cùng độ ưu tiên 7 */

#define LED3_NODE DT_ALIAS(led0) /* PE4 */
#define Y0_NODE DT_ALIAS(led1)   /* PE5 */
#define Y1_NODE DT_ALIAS(led2)   /* PE6 */

/* Cấu trúc dữ liệu chứa tham số cấu hình riêng cho từng đèn */
struct blink_cfg {
  struct gpio_dt_spec led; /* Thông tin phần cứng GPIO */
  int on_ms;               /* Thời gian sáng (ms) */
  int off_ms;              /* Thời gian tắt (ms) */
  const char *name;        /* Tên đại diện in log */
};

/* Khởi tạo tĩnh 3 bảng cấu hình riêng biệt trong bộ nhớ Flash (const) */
static const struct blink_cfg cfg_led3 = {
    .led = GPIO_DT_SPEC_GET(LED3_NODE, gpios),
    .on_ms = 2000,
    .off_ms = 2000,
    .name = "LED3"};

static const struct blink_cfg cfg_y0 = {.led = GPIO_DT_SPEC_GET(Y0_NODE, gpios),
                                        .on_ms = 500,
                                        .off_ms = 500,
                                        .name = "OUT_Y0"};

static const struct blink_cfg cfg_y1 = {.led = GPIO_DT_SPEC_GET(Y1_NODE, gpios),
                                        .on_ms = 3000,
                                        .off_ms = 1000,
                                        .name = "OUT_Y1"};

/*
 * ============================================================================
 * HÀM THỰC THI CHUNG DUY NHẤT: thread_blink (GENERIC THREAD ENTRY FUNCTION)
 * ----------------------------------------------------------------------------
 * - Tham số p1: Nhận con trỏ (void *) trỏ tới struct blink_cfg của thread đó.
 * - Tham số p2, p3: Không sử dụng, dùng ARG_UNUSED() để tránh warning.
 * - Bên trong hàm: Ép kiểu (type cast) con trỏ void *p1 thành (const struct
 * blink_cfg *), từ đó truy xuất chân GPIO và thời gian on_ms / off_ms tương
 * ứng.
 * ============================================================================
 */
void thread_blink(void *p1, void *p2, void *p3) {
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  /*
   * ÉP KIỂU CON TRỎ (TYPE CASTING):
   * Tham số p1 truyền vào là con trỏ vạn năng "void *" (không rõ kiểu dữ liệu),
   * nên trình biên dịch không cho phép gọi trực tiếp p1->on_ms.
   * Ta cần ép kiểu p1 về con trỏ "const struct blink_cfg *" để trình biên dịch
   * hiểu ô nhớ này chứa struct cấu hình, từ đó mới truy xuất được cfg->led,
   * cfg->on_ms,...
   */
  const struct blink_cfg *cfg = (const struct blink_cfg *)p1;

  /* Kiểm tra và cấu hình GPIO của đèn này */
  if (!gpio_is_ready_dt(&cfg->led))
    return;
  gpio_pin_configure_dt(&cfg->led, GPIO_OUTPUT_INACTIVE);

  while (1) {
    /* Bật đèn theo thời gian on_ms */
    gpio_pin_set_dt(&cfg->led, 1);
    printf("[%6u ms] [%-6s] SÁNG (%d ms)\n", k_uptime_get_32(), cfg->name,
           cfg->on_ms);
    k_msleep(cfg->on_ms);

    /* Tắt đèn theo thời gian off_ms */
    gpio_pin_set_dt(&cfg->led, 0);
    printf("[%6u ms] [%-6s] TẮT  (%d ms)\n", k_uptime_get_32(), cfg->name,
           cfg->off_ms);
    k_msleep(cfg->off_ms);
  }
}

/*
 * ============================================================================
 * ĐĂNG KÝ 3 THREAD TĨNH CÙNG TRỎ VÀO MỘT HÀM thread_blink:
 * ----------------------------------------------------------------------------
 * - Tham số p1 nhận lần lượt: (void *)&cfg_led3, (void *)&cfg_y0,
 * (void*)&cfg_y1.
 * - Mỗi thread có stack riêng biệt (1024 bytes) nhưng tái sử dụng hoàn toàn mã
 * hàm.
 * ============================================================================
 */
K_THREAD_DEFINE(t_blink_led3, STACK_SIZE, thread_blink, (void *)&cfg_led3, NULL,
                NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(t_blink_y0, STACK_SIZE, thread_blink, (void *)&cfg_y0, NULL,
                NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(t_blink_y1, STACK_SIZE, thread_blink, (void *)&cfg_y1, NULL,
                NULL, PRIORITY, 0, 0);

int main(void) {
  printf(
      "[Main] Bài 4 hoàn tất khởi động. 3 Thread dùng chung 1 hàm thực thi.\n");
  return 0;
}
