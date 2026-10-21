/*
 * ============================================================================
 * TRƯỜNG ĐẠI HỌC BÁCH KHOA - ĐHQG-HCM
 * KHOA KHOA HỌC VÀ KỸ THUẬT MÁY TÍNH
 * MÔN HỌC: HỆ THỐNG NHÚNG (EMBEDDED SYSTEMS)
 *
 * LAB 1 - BÀI 1: NHÁY LED BẤT ĐỐI XỨNG (ASYMMETRICAL BLINKY)
 * Nền tảng : Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 168 MHz)
 * Framework: Zephyr RTOS v4.4.0 + VS Code
 * ============================================================================
 *
 * 1. ĐỀ BÀI (YÊU CẦU BÀI TẬP):
 * - Tạo một luồng thực thi (Thread) độc lập có tên "thread_led_blinky".
 * - Điều khiển LED3 (PE4, alias devicetree: led0) nháy với chu kỳ bất đối xứng:
 *   + Thời gian SÁNG (ON) : 1000 ms (1 giây).
 *   + Thời gian TẮT (OFF): 2000 ms (2 giây).
 *   + Chu kỳ tổng cộng    : 3000 ms (3 giây), lặp lại vô hạn.
 * - Khởi tạo thread tĩnh ở phạm vi toàn cục bằng macro K_THREAD_DEFINE.
 * - Hàm main() không can thiệp logic nháy LED, chỉ in log khởi động rồi kết thúc.
 * - Sử dụng hàm gpio_pin_set_dt() với giá trị logic tường minh: mức 1 (sáng) và mức 0 (tắt).
 *
 * 2. MỤC TIÊU VÀ NGUYÊN LÝ HOẠT ĐỘNG:
 * - Hiểu cấu trúc một Thread cơ bản trong hệ điều hành thời gian thực Zephyr RTOS.
 * - Nắm vững cơ chế định thời không chiếm dụng CPU (Non-blocking delay) qua k_msleep().
 * - Khi thread gọi k_msleep(), hệ điều hành chuyển thread từ trạng thái RUNNING
 *   sang WAITING/SUSPENDED, giải phóng CPU cho Idle thread hoặc các tác vụ khác.
 *
 * 3. ÁNH XẠ PHẦN CỨNG (KIT BKIT ARM4):
 * - Ngoại vi : LED3 (LED người dùng / Debug LED)
 * - Chân MCU: PE4 (Port E, Pin 4)
 * - Devicetree: DT_ALIAS(led0) -> gpios = <&gpioe 4 GPIO_ACTIVE_HIGH>
 * - Mức logic: Mức 1 (High, 3.3V) -> SÁNG | Mức 0 (Low, 0V) -> TẮT
 * ============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define TIME_LED_ON_MS   1000   /* Thời gian đèn sáng: 1000 ms = 1 giây */
#define TIME_LED_OFF_MS  2000   /* Thời gian đèn tắt: 2000 ms = 2 giây */

#define BLINKY_STACK_SIZE 1024  /* Kích thước bộ nhớ stack: 1024 bytes (1 KB) */
#define BLINKY_PRIORITY   7     /* Độ ưu tiên của thread (số càng nhỏ ưu tiên càng cao) */

/* Lấy thông tin cấu hình LED3 từ Devicetree alias "led0" */
#define LED3_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

/*
 * ============================================================================
 * HÀM THỰC THI CỦA THREAD (Thread Entry Function):
 * ----------------------------------------------------------------------------
 * 1. NGUYÊN MẪU HÀM (Function Signature):
 *    - Zephyr kernel quy định mọi thread entry function phải có định dạng chuẩn:
 *        void (*k_thread_entry_t)(void *p1, void *p2, void *p3);
 *    - Bắt buộc phải có đủ 3 con trỏ tham số (p1, p2, p3) dù thread có dùng hay không.
 *    - Macro ARG_UNUSED(p) dùng để báo cho trình biên dịch (GCC/Clang) bỏ qua
 *      cảnh báo biến không sử dụng (-Wunused-parameter).
 *
 * 2. CẤU TRÚC CHUẨN CỦA MỘT THREAD TRONG RTOS:
 *    - Khởi tạo (Chạy 1 lần duy nhất khi luồng vừa bắt đầu):
 *      + gpio_is_ready_dt(): Kiểm tra xem driver phần cứng GPIO đã sẵn sàng chưa.
 *      + gpio_pin_configure_dt(): Cấu hình chiều I/O và mức logic ban đầu (Output Low).
 *    - Vòng đời (Life Cycle - Chạy vĩnh viễn):
 *      + Vòng lặp `while (1)` giữ cho thread luôn tồn tại để phục vụ tác vụ chu kỳ.
 *      + Điều khiển bật/tắt thiết bị qua API gpio_pin_set_dt().
 *      + Gọi hàm k_msleep() để nhường CPU: Zephyr Scheduler sẽ tạm dừng thread này,
 *        đưa vào hàng đợi ngủ và nhường CPU cho các tác vụ khác. Tuyệt đối KHÔNG
 *        dùng vòng lặp for/while rỗng (busy-wait) vì sẽ gây nghẽn CPU 100%.
 * ============================================================================
 */
void thread_led_blinky_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    /* 1. Kiểm tra driver GPIO của Port E đã khởi tạo và sẵn sàng chưa */
    if (!gpio_is_ready_dt(&led3)) {
        printf("[Lỗi] GPIO LED3 chưa sẵn sàng!\n");
        return;
    }

    /* 2. Cấu hình chân PE4 làm ngõ ra (Output), trạng thái ban đầu là TẮT */
    gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

    printf("[Lab 1 - Bài 1] Bắt đầu: SÁNG %d ms --> TẮT %d ms\n", TIME_LED_ON_MS, TIME_LED_OFF_MS);

    /* 3. Vòng đời lặp vô hạn của thread */
    while (1) {
        /* BẬT LED3: Xuất mức 1 (3.3V) -> LED SÁNG */
        gpio_pin_set_dt(&led3, 1);
        printf("[%6u ms] [LED3] SÁNG (1s)\n", k_uptime_get_32());
        k_msleep(TIME_LED_ON_MS); /* Ngủ 1s nhường CPU */

        /* TẮT LED3: Xuất mức 0 (0V) -> LED TẮT */
        gpio_pin_set_dt(&led3, 0);
        printf("[%6u ms] [LED3] TẮT  (2s)\n", k_uptime_get_32());
        k_msleep(TIME_LED_OFF_MS); /* Ngủ 2s nhường CPU */
    }
}

/*
 * ============================================================================
 * ĐĂNG KÝ VÀ KHỞI TẠO THREAD TĨNH (K_THREAD_DEFINE):
 * ----------------------------------------------------------------------------
 * 1. BẢN CHẤT CỦA STATIC THREAD TRONG ZEPHYR:
 *    - Khác với k_thread_create() tạo thread động khi chương trình đang chạy,
 *      macro K_THREAD_DEFINE định nghĩa thread TĨNH ngay tại thời điểm BIÊN DỊCH.
 *    - Toàn bộ bộ nhớ Stack và cấu trúc quản lý kernel struct k_thread được cấp
 *      phát tĩnh trong phân vùng .noinit/.bss của vi điều khiển.
 *    - Ưu điểm: Tốc độ khởi động tức thì (Zero runtime overhead), không tốn RAM heap,
 *      hoàn toàn không lo rò rỉ bộ nhớ (Memory Leak) hay phân mảnh bộ nhớ.
 *
 * 2. GIẢI THÍCH CHI TIẾT 9 THAM SỐ CỦA MACRO:
 *    (1) name       : thread_led_blinky        -> Tên định danh kernel object của thread
 *    (2) stack_size : BLINKY_STACK_SIZE (1024) -> Kích thước vùng nhớ Stack (bytes)
 *    (3) entry_fn   : thread_led_blinky_entry  -> Con trỏ tới hàm thực thi của thread
 *    (4) p1         : NULL                     -> Tham số thứ 1 truyền cho thread (không dùng)
 *    (5) p2         : NULL                     -> Tham số thứ 2 truyền cho thread (không dùng)
 *    (6) p3         : NULL                     -> Tham số thứ 3 truyền cho thread (không dùng)
 *    (7) prio       : BLINKY_PRIORITY (7)      -> Độ ưu tiên trong Scheduler (số nhỏ = ưu tiên cao)
 *    (8) options    : 0                        -> Cờ cấu hình đặc biệt của kernel (0 = luồng thường)
 *    (9) delay      : 0                        -> Thời gian hoãn khởi động (0 = chạy ngay sau khi boot)
 * ============================================================================
 */
K_THREAD_DEFINE(thread_led_blinky, BLINKY_STACK_SIZE,
                thread_led_blinky_entry, NULL, NULL, NULL,
                BLINKY_PRIORITY, 0, 0);

/*
 * Hàm main() chuẩn của hệ điều hành Zephyr:
 * Vì thread_led_blinky đã được kernel tự động khởi chạy ngầm, hàm main() chỉ
 * in thông báo xác nhận hoàn tất khởi động rồi kết thúc (return 0).
 */
int main(void)
{
    printf("[Main] Bài 1 hoàn tất khởi động. Thread chạy ngầm.\n");
    return 0;
}
