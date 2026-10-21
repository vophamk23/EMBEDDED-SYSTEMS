/*
 * ============================================================================
 * TRƯỜNG ĐẠI HỌC BÁCH KHOA - ĐHQG-HCM
 * KHOA KHOA HỌC VÀ KỸ THUẬT MÁY TÍNH
 * MÔN HỌC: HỆ THỐNG NHÚNG (EMBEDDED SYSTEMS)
 *
 * LAB 1 - BÀI 5: MÔ PHỎNG HỆ THỐNG ĐÈN GIAO THÔNG (TRAFFIC LIGHT SIMULATION)
 * Nền tảng : Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 168 MHz)
 * Framework: Zephyr RTOS v4.4.0 + VS Code
 * ============================================================================
 *
 * 1. ĐỀ BÀI (YÊU CẦU BÀI TẬP):
 * - Xây dựng hệ thống nhúng điều khiển đèn giao thông phối hợp đa luồng:
 *   + Thread 1 "thread_led_blinky":
 *     * Điều khiển LED3 (PE4, alias led0).
 *     * Nháy đều đặn chu kỳ 1s sáng / 1s tắt đóng vai trò là "Heartbeat" báo hiệu
 *       hệ thống và vi điều khiển đang sống và hoạt động bình thường.
 *   + Thread 2 "thread_traffic_light":
 *     * Điều khiển cặp ngõ ra Output Y0 (PE5) và Y1 (PE6) mô phỏng 3 pha đèn giao thông:
 *       - Pha 1: ĐÈN ĐỎ   -> Chỉ Y0 sáng (Y0=1, Y1=0), duy trì 7 giây (7000 ms).
 *       - Pha 2: ĐÈN XANH  -> Chỉ Y1 sáng (Y0=0, Y1=1), duy trì 5 giây (5000 ms).
 *       - Pha 3: ĐÈN VÀNG  -> Cả Y0 và Y1 cùng sáng   , duy trì 2 giây (2000 ms).
 *       - Chu trình tuần hoàn vô hạn: ĐỎ (7s) -> XANH (5s) -> VÀNG (2s) -> ĐỎ...
 * - YÊU CẦU KỸ THUẬT:
 *   + Lưu danh sách các pha vào mảng hằng số struct phase traffic_phases[].
 *   + Dùng vòng lặp for duyệt qua mảng để chuyển pha, giúp mã nguồn cô đọng và dễ bảo trì.
 *   + Mỗi lần chuyển pha, in thông báo ra console: printf("[DEN] %s - %d s\n", name, s).
 *
 * 2. MỤC TIÊU VÀ NGUYÊN LÝ HOẠT ĐỘNG:
 * - Ứng dụng cấu trúc dữ liệu bảng (Table-driven State Machine) trong hệ điều hành thời gian thực.
 * - Thực hành cơ chế đa nhiệm: Luồng đèn giao thông có các khoảng delay rất dài (7s, 5s)
 *   nhưng luồng Heartbeat (1s) vẫn nháy liên tục chuẩn xác, không bị ảnh hưởng.
 *
 * 3. ÁNH XẠ PHẦN CỨNG (KIT BKIT ARM4):
 * - LED3 (Heartbeat): Chân PE4, alias led0 -> gpios = <&gpioe 4 GPIO_ACTIVE_HIGH>
 * - Output Y0 (Đỏ)  : Chân PE5, alias led1 -> gpios = <&gpioe 5 GPIO_ACTIVE_HIGH>
 * - Output Y1 (Xanh): Chân PE6, alias led2 -> gpios = <&gpioe 6 GPIO_ACTIVE_HIGH>
 *   (Khi pha Vàng: Bật đồng thời cả Y0 và Y1).
 * ============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define STACK_SIZE 1024         /* Stack 1024 bytes cho mỗi luồng */
#define PRIORITY   7            /* Cùng chạy ở độ ưu tiên 7 */

#define LED3_NODE DT_ALIAS(led0) /* PE4 - Heartbeat LED */
#define Y0_NODE   DT_ALIAS(led1) /* PE5 - Output Y0 */
#define Y1_NODE   DT_ALIAS(led2) /* PE6 - Output Y1 */

static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);
static const struct gpio_dt_spec y0   = GPIO_DT_SPEC_GET(Y0_NODE, gpios);
static const struct gpio_dt_spec y1   = GPIO_DT_SPEC_GET(Y1_NODE, gpios);

/* Cấu trúc dữ liệu đại diện cho một pha của đèn giao thông */
struct phase {
    const char *name; /* Tên hiển thị của pha ("DO", "XANH", "VANG") */
    int y0;           /* Trạng thái ngõ ra Y0 (1: Bật, 0: Tắt) */
    int y1;           /* Trạng thái ngõ ra Y1 (1: Bật, 0: Tắt) */
    int ms;           /* Thời gian duy trì pha (mili-giây) */
};

/* Bảng dữ liệu cấu hình 3 pha đèn giao thông (Lưu cố định trong bộ nhớ Flash) */
static const struct phase traffic_phases[] = {
    {
        .name = "DO",   /* Pha ĐỎ: Chỉ Y0 sáng */
        .y0   = 1,
        .y1   = 0,
        .ms   = 7000    /* Duy trì 7 giây */
    },
    {
        .name = "XANH", /* Pha XANH: Chỉ Y1 sáng */
        .y0   = 0,
        .y1   = 1,
        .ms   = 5000    /* Duy trì 5 giây */
    },
    {
        .name = "VANG", /* Pha VÀNG: Cả Y0 và Y1 cùng sáng */
        .y0   = 1,
        .y1   = 1,
        .ms   = 2000    /* Duy trì 2 giây */
    }
};

#define NUM_PHASES (sizeof(traffic_phases) / sizeof(traffic_phases[0]))

/*
 * ============================================================================
 * HÀM THỰC THI CỦA THREAD 1: thread_led_blinky_entry (LUỒNG HEARTBEAT HỆ THỐNG)
 * ----------------------------------------------------------------------------
 * Nháy LED3 đều đặn 1s sáng / 1s tắt liên tục.
 * Mục đích kỹ thuật: Trong các hệ thống nhúng chuyên nghiệp, luồng Heartbeat giúp
 * kỹ sư quan sát bằng mắt thường để biết chắc chắn CPU không bị treo/Watchdog reset.
 * ============================================================================
 */
void thread_led_blinky_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    if (!gpio_is_ready_dt(&led3)) return;
    gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

    while (1) {
        /* Bật LED3 trong 1 giây */
        gpio_pin_set_dt(&led3, 1);
        k_msleep(1000);

        /* Tắt LED3 trong 1 giây */
        gpio_pin_set_dt(&led3, 0);
        k_msleep(1000);
    }
}

/*
 * ============================================================================
 * HÀM THỰC THI CỦA THREAD 2: thread_traffic_light_entry (ĐÈN GIAO THÔNG FSM)
 * ----------------------------------------------------------------------------
 * - Duyệt tuần tự qua từng phần tử trong bảng traffic_phases[].
 * - Xuất trạng thái logic tương ứng ra ngõ ra Y0 và Y1.
 * - In thông báo ra màn hình console và gọi k_msleep(p->ms) để giữ nguyên trạng thái.
 * ============================================================================
 */
void thread_traffic_light_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    if (!gpio_is_ready_dt(&y0) || !gpio_is_ready_dt(&y1)) return;
    gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&y1, GPIO_OUTPUT_INACTIVE);

    while (1) {
        /* Duyệt tuần tự qua tất cả các pha được định nghĩa trong mảng */
        for (int i = 0; i < NUM_PHASES; i++) {
            const struct phase *p = &traffic_phases[i];

            /* Xuất trạng thái đèn ra cổng Output Y0 và Y1 */
            gpio_pin_set_dt(&y0, p->y0);
            gpio_pin_set_dt(&y1, p->y1);
            printf("[%6u ms] [DEN] %s - %d s\n", k_uptime_get_32(), p->name, p->ms / 1000);

            /* Duy trì trạng thái pha tương ứng thời gian cài đặt */
            k_msleep(p->ms);
        }
    }
}

/*
 * ============================================================================
 * ĐĂNG KÝ 2 THREAD TĨNH (K_THREAD_DEFINE):
 * ----------------------------------------------------------------------------
 * Thread Heartbeat và Thread Đèn giao thông cùng khởi chạy đồng thời.
 * Zephyr RTOS điều phối CPU nhịp nhàng dù luồng đèn giao thông ngủ đến 7 giây.
 * ============================================================================
 */
K_THREAD_DEFINE(thread_led_blinky, STACK_SIZE,
                thread_led_blinky_entry, NULL, NULL, NULL,
                PRIORITY, 0, 0);

K_THREAD_DEFINE(thread_traffic_light, STACK_SIZE,
                thread_traffic_light_entry, NULL, NULL, NULL,
                PRIORITY, 0, 0);

int main(void)
{
    printf("[Main] Bài 5 hoàn tất khởi động. Hệ thống đèn giao thông đang hoạt động.\n");
    return 0;
}
