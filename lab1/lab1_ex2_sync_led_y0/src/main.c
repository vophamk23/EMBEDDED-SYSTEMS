/*
 * ============================================================================
 * TRƯỜNG ĐẠI HỌC BÁCH KHOA - ĐHQG-HCM
 * KHOA KHOA HỌC VÀ KỸ THUẬT MÁY TÍNH
 * MÔN HỌC: HỆ THỐNG NHÚNG (EMBEDDED SYSTEMS)
 *
 * LAB 1 - BÀI 2: ĐỒNG BỘ LED3 VÀ OUTPUT Y0 (SYNCHRONIZATION VIA SHARED VARIABLE)
 * Nền tảng : Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 168 MHz)
 * Framework: Zephyr RTOS v4.4.0 + VS Code
 * ============================================================================
 *
 * 1. ĐỀ BÀI (YÊU CẦU BÀI TẬP):
 * - Tạo 2 thread hoạt động song song và phối hợp đồng bộ qua một biến dùng chung:
 *   + Thread 1 "thread_led_blinky" (Luồng Master):
 *     * Điều khiển LED3 (PE4, alias led0) nháy với chu kỳ Sáng 2s (2000 ms), Tắt 2s (2000 ms).
 *     * Cập nhật trạng thái đèn vào biến dùng chung toàn cục g_led_state (true: sáng, false: tắt).
 *   + Thread 2 "thread_y0_follow" (Luồng Follower / Slave):
 *     * Điều khiển ngõ ra Output Y0 (PE5, alias led1 - quan sát qua đèn LED trạng thái Y0 trên kit).
 *     * KHÔNG được tự đếm thời gian 2 giây.
 *     * Định kỳ thăm dò (Polling mỗi 10 ms qua k_msleep(10)) biến dùng chung g_led_state.
 *     * Khi phát hiện trạng thái thay đổi, lập tức cập nhật Y0 đồng bộ theo LED3.
 * - Khởi tạo 2 thread tĩnh bằng macro K_THREAD_DEFINE với cùng độ ưu tiên.
 * - Hàm main() in thông báo khởi động và kết thúc.
 *
 * 2. MỤC TIÊU VÀ NGUYÊN LÝ HOẠT ĐỘNG:
 * - Hiểu cơ chế giao tiếp và đồng bộ cơ bản giữa các tiến trình qua biến toàn cục (Shared Variable).
 * - Nắm vững ý nghĩa của từ khóa 'volatile': bắt buộc trình biên dịch không tối ưu ghi đè
 *   thanh ghi CPU mà luôn đọc/ghi trực tiếp vào bộ nhớ RAM, đảm bảo thread khác nhìn thấy giá trị mới ngay lập tức.
 * - Phân biệt giữa luồng chủ động định thời (Master) và luồng thụ động theo dõi sự kiện (Follower).
 *
 * 3. ÁNH XẠ PHẦN CỨNG (KIT BKIT ARM4):
 * - LED3      : Chân PE4, alias led0 -> gpios = <&gpioe 4 GPIO_ACTIVE_HIGH>
 * - Output Y0 : Chân PE5, alias led1 -> gpios = <&gpioe 5 GPIO_ACTIVE_HIGH>
 *   (Ngõ ra Y0 có sẵn LED báo trạng thái trực tiếp trên kit BKIT ARM4, không cần gắn tải ngoài).
 * ============================================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define STACK_SIZE 1024         /* Kích thước stack cho mỗi thread: 1 KB */
#define PRIORITY   7            /* Cả 2 thread cùng chạy ở độ ưu tiên 7 */

#define LED3_NODE DT_ALIAS(led0) /* LED3 (PE4) */
#define Y0_NODE   DT_ALIAS(led1) /* Output Y0 (PE5) */

static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);
static const struct gpio_dt_spec y0   = GPIO_DT_SPEC_GET(Y0_NODE, gpios);

/*
 * BIẾN DÙNG CHUNG ĐỂ ĐỒNG BỘ HAI THREAD (SHARED VARIABLE):
 * - Kiểu dữ liệu: volatile bool
 * - Ý nghĩa từ khóa 'volatile': Rất quan trọng trong lập trình RTOS/Đa luồng!
 *   Báo cho trình biên dịch (Compiler) biết giá trị của biến này có thể bị thay đổi
 *   bất ngờ bởi một thread khác hoặc ngắt phần cứng. Trình biên dịch sẽ bị cấm
 *   tối ưu lưu giá trị trong thanh ghi nội bộ của CPU, mà bắt buộc mỗi lần đọc/ghi
 *   đều phải truy xuất trực tiếp ô nhớ RAM thật sự.
 */
static volatile bool g_led_state = false;

/*
 * ============================================================================
 * HÀM THỰC THI CỦA THREAD 1: thread_led_blinky_entry (LUỒNG MASTER)
 * ----------------------------------------------------------------------------
 * - Đóng vai trò làm nguồn xung nhịp / máy phát tín hiệu đồng bộ.
 * - Tự chịu trách nhiệm đếm thời gian chính xác: Sáng 2000 ms, Tắt 2000 ms.
 * - Mỗi lần thay đổi trạng thái LED3, đồng thời ghi trạng thái mới vào g_led_state.
 * ============================================================================
 */
void thread_led_blinky_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    /* Kiểm tra và cấu hình GPIO LED3 */
    if (!gpio_is_ready_dt(&led3)) return;
    gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

    while (1) {
        /* Bật LED3 và ghi nhận biến trạng thái = true (SÁNG) */
        gpio_pin_set_dt(&led3, 1);
        g_led_state = true;
        printf("[%6u ms] [Master LED3] SÁNG (2s)\n", k_uptime_get_32());
        k_msleep(2000); /* Ngủ 2 giây nhường CPU */

        /* Tắt LED3 và ghi nhận biến trạng thái = false (TẮT) */
        gpio_pin_set_dt(&led3, 0);
        g_led_state = false;
        printf("[%6u ms] [Master LED3] TẮT  (2s)\n", k_uptime_get_32());
        k_msleep(2000); /* Ngủ 2 giây nhường CPU */
    }
}

/*
 * ============================================================================
 * HÀM THỰC THI CỦA THREAD 2: thread_y0_follow_entry (LUỒNG FOLLOWER / SLAVE)
 * ----------------------------------------------------------------------------
 * - KHÔNG có bất kỳ hàm delay dài 2 giây nào.
 * - Cơ chế hoạt động: Thăm dò chu kỳ ngắn (Periodic Polling) mỗi 10 ms.
 * - Sử dụng kỹ thuật phát hiện sườn (Edge Detection) bằng biến cục bộ `last_state`:
 *   Chỉ khi giá trị g_led_state khác với last_state thì mới kích hoạt xuất chân GPIO,
 *   giúp tránh ghi đè liên tục vào phần cứng gây lãng phí năng lượng.
 * ============================================================================
 */
void thread_y0_follow_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    /* Kiểm tra và cấu hình GPIO Output Y0 */
    if (!gpio_is_ready_dt(&y0)) return;
    gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);

    bool last_state = false; /* Lưu lại trạng thái của lần thăm dò liền trước */

    while (1) {
        bool current_state = g_led_state; /* Đọc trực tiếp từ RAM nhờ volatile */

        /* Nếu phát hiện trạng thái có sự thay đổi -> Đồng bộ Output Y0 theo */
        if (current_state != last_state) {
            gpio_pin_set_dt(&y0, current_state ? 1 : 0);
            printf("[%6u ms] >>> [Follower Y0] Cập nhật theo LED3: %s\n",
                   k_uptime_get_32(), current_state ? "SÁNG" : "TẮT");
            last_state = current_state;
        }

        /* Ngủ 10 ms để nhường CPU cho Master thread và Scheduler */
        k_msleep(10);
    }
}

/*
 * ============================================================================
 * ĐĂNG KÝ 2 THREAD TĨNH VỚI ZEPHYR (K_THREAD_DEFINE):
 * ----------------------------------------------------------------------------
 * - Cả 2 thread được kernel cấp phát vùng nhớ riêng (1024 bytes stack mỗi thread).
 * - Cùng độ ưu tiên PRIORITY (7): Zephyr sẽ luân phiên điều phối khi một luồng
 *   chuyển sang trạng thái ngủ k_msleep().
 * - Khởi chạy đồng thời ngay sau khi boot hệ thống (delay = 0).
 * ============================================================================
 */
K_THREAD_DEFINE(thread_led_blinky, STACK_SIZE,
                thread_led_blinky_entry, NULL, NULL, NULL,
                PRIORITY, 0, 0);

K_THREAD_DEFINE(thread_y0_follow, STACK_SIZE,
                thread_y0_follow_entry, NULL, NULL, NULL,
                PRIORITY, 0, 0);

int main(void)
{
    printf("[Main] Bài 2 hoàn tất khởi động. 2 Thread đang đồng bộ qua biến chung.\n");
    return 0;
}
