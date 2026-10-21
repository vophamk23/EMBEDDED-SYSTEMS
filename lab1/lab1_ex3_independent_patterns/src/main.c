/*
 * ============================================================================
 * TRƯỜNG ĐẠI HỌC BÁCH KHOA - ĐHQG-HCM
 * KHOA KHOA HỌC VÀ KỸ THUẬT MÁY TÍNH
 * MÔN HỌC: HỆ THỐNG NHÚNG (EMBEDDED SYSTEMS)
 *
 * LAB 1 - BÀI 3: CÁC MẪU NHÁY ĐỘC LẬP (INDEPENDENT BLINK PATTERNS)
 * Nền tảng : Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 168 MHz)
 * Framework: Zephyr RTOS v4.4.0 + VS Code
 * ============================================================================
 *
 * 1. ĐỀ BÀI (YÊU CẦU BÀI TẬP):
 * - Xây dựng 2 thread chạy song song, điều khiển các mẫu nháy hoàn toàn bất đồng bộ:
 *   + Thread 1 "thread_led_blinky":
 *     * Điều khiển LED3 (PE4, alias led0).
 *     * Chu kỳ: SÁNG 1.5s (1500 ms) --> TẮT 0.5s (500 ms).
 *     * Tổng chu kỳ lặp: 2.0 giây (2000 ms).
 *   + Thread 2 "thread_output_sequence":
 *     * Điều khiển cặp ngõ ra Output Y0 (PE5) và Y1 (PE6) tuần tự qua 3 pha:
 *       - Pha 1: Chỉ Y0 sáng (Y0=1, Y1=0)  -> Duy trì 1 giây (1000 ms).
 *       - Pha 2: Chỉ Y1 sáng (Y0=0, Y1=1)  -> Duy trì 2 giây (2000 ms).
 *       - Pha 3: Cả Y0 và Y1 cùng sáng     -> Duy trì 2 giây (2000 ms).
 *     * Tổng chu kỳ lặp: 1s + 2s + 2s = 5.0 giây (5000 ms).
 * - YÊU CẦU BẮT BUỘC:
 *   + Hai thread chạy hoàn toàn độc lập, KHÔNG dùng biến dùng chung.
 *   + Chứng minh cơ chế đa nhiệm ưu tiên (Preemptive Multitasking): Dù chu kỳ của
 *     2 thread là 2.0s và 5.0s (bội chung nhỏ nhất 10s), các pha bật tắt vẫn diễn
 *     ra cực kỳ chính xác mà không hề bị trễ nải hoặc chặn (block) lẫn nhau.
 *
 * 2. MỤC TIÊU VÀ NGUYÊN LÝ HOẠT ĐỘNG:
 * - Hiểu sâu sắc cách hệ điều hành thời gian thực Zephyr quản lý nhiều luồng độc lập.
 * - Khi Thread 2 đang ngủ dài (1s hoặc 2s), Timer của Thread 1 hết hạn sẽ đánh thức
 *   Thread 1 dậy thực thi đúng thời điểm, sau đó lại chuyển giao CPU.
 *
 * 3. ÁNH XẠ PHẦN CỨNG (KIT BKIT ARM4):
 * - LED3      : Chân PE4, alias led0 -> gpios = <&gpioe 4 GPIO_ACTIVE_HIGH>
 * - Output Y0 : Chân PE5, alias led1 -> gpios = <&gpioe 5 GPIO_ACTIVE_HIGH>
 * - Output Y1 : Chân PE6, alias led2 -> gpios = <&gpioe 6 GPIO_ACTIVE_HIGH>
 * ============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define STACK_SIZE 1024         /* Stack 1024 bytes cho mỗi thread */
#define PRIORITY   7            /* Cùng độ ưu tiên 7 trong Scheduler */

#define LED3_NODE DT_ALIAS(led0) /* PE4 */
#define Y0_NODE   DT_ALIAS(led1) /* PE5 */
#define Y1_NODE   DT_ALIAS(led2) /* PE6 */

static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);
static const struct gpio_dt_spec y0   = GPIO_DT_SPEC_GET(Y0_NODE, gpios);
static const struct gpio_dt_spec y1   = GPIO_DT_SPEC_GET(Y1_NODE, gpios);

/*
 * ============================================================================
 * HÀM THỰC THI CỦA THREAD 1: thread_led_blinky_entry (LUỒNG LED3)
 * ----------------------------------------------------------------------------
 * Chu kỳ tuần hoàn độc lập: Sáng 1500 ms + Tắt 500 ms = 2000 ms.
 * Hoạt động độc lập không phụ thuộc vào trạng thái hay thời gian của Y0 và Y1.
 * ============================================================================
 */
void thread_led_blinky_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    if (!gpio_is_ready_dt(&led3)) return;
    gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

    while (1) {
        /* BẬT LED3 trong 1.5 giây */
        gpio_pin_set_dt(&led3, 1);
        printf("[%6u ms] [LED3] SÁNG (1.5s)\n", k_uptime_get_32());
        k_msleep(1500);

        /* TẮT LED3 trong 0.5 giây */
        gpio_pin_set_dt(&led3, 0);
        printf("[%6u ms] [LED3] TẮT  (0.5s)\n", k_uptime_get_32());
        k_msleep(500);
    }
}

/*
 * ============================================================================
 * HÀM THỰC THI CỦA THREAD 2: thread_output_sequence_entry (LUỒNG TUẦN TỰ Y0 & Y1)
 * ----------------------------------------------------------------------------
 * Chu kỳ tuần hoàn độc lập: 1s + 2s + 2s = 5000 ms qua 3 pha tuần tự.
 * Điều khiển 2 ngõ ra phần cứng Y0 và Y1.
 * ============================================================================
 */
void thread_output_sequence_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    if (!gpio_is_ready_dt(&y0) || !gpio_is_ready_dt(&y1)) return;
    gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&y1, GPIO_OUTPUT_INACTIVE);

    while (1) {
        /* PHA 1: Chỉ Y0 sáng trong 1 giây (Y0=1, Y1=0) */
        gpio_pin_set_dt(&y0, 1);
        gpio_pin_set_dt(&y1, 0);
        printf("[%6u ms] [Y0/Y1] Pha 1: Chỉ Y0 SÁNG (1s)\n", k_uptime_get_32());
        k_msleep(1000);

        /* PHA 2: Chỉ Y1 sáng trong 2 giây (Y0=0, Y1=1) */
        gpio_pin_set_dt(&y0, 0);
        gpio_pin_set_dt(&y1, 1);
        printf("[%6u ms] [Y0/Y1] Pha 2: Chỉ Y1 SÁNG (2s)\n", k_uptime_get_32());
        k_msleep(2000);

        /* PHA 3: Cả Y0 và Y1 cùng sáng trong 2 giây (Y0=1, Y1=1) */
        gpio_pin_set_dt(&y0, 1);
        gpio_pin_set_dt(&y1, 1);
        printf("[%6u ms] [Y0/Y1] Pha 3: Cả Y0 và Y1 SÁNG (2s)\n", k_uptime_get_32());
        k_msleep(2000);
    }
}

/*
 * ============================================================================
 * ĐĂNG KÝ 2 THREAD TĨNH (K_THREAD_DEFINE):
 * ----------------------------------------------------------------------------
 * Hai thread chạy song song, hoàn toàn bất đồng bộ về chu kỳ (2s so với 5s).
 * Hệ điều hành Zephyr tự động chia sẻ thời gian CPU và chuyển đổi ngữ cảnh (Context Switch).
 * ============================================================================
 */
K_THREAD_DEFINE(thread_led_blinky, STACK_SIZE,
                thread_led_blinky_entry, NULL, NULL, NULL,
                PRIORITY, 0, 0);

K_THREAD_DEFINE(thread_output_sequence, STACK_SIZE,
                thread_output_sequence_entry, NULL, NULL, NULL,
                PRIORITY, 0, 0);

int main(void)
{
    printf("[Main] Bài 3 hoàn tất khởi động. 2 Thread chạy chu kỳ độc lập.\n");
    return 0;
}
