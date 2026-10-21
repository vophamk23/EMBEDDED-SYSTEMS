# LAB 1: MULTITHREADING IN ZEPHYR RTOS
**Môn học:** Hệ thống nhúng (Embedded Systems) - ĐH Bách Khoa ĐHQG-HCM  
**Kit thực hành:** Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 168 MHz)  
**RTOS:** Zephyr RTOS v4.4.0

---

## Danh sách bài tập Lab 1:
1. **bkit_blinky**: Bài mẫu kiểm tra phần cứng LED3 (`PE4`).
2. **lab1_ex1_asym_blinky**: Nháy LED3 chu kỳ bất đối xứng (Sáng 1s / Tắt 2s) dùng `K_THREAD_DEFINE`.
3. **lab1_ex2_sync_led_y0**: Đồng bộ LED3 và Output Y0 (`PE5`) qua biến dùng chung `volatile bool g_led_state`.
4. **lab1_ex3_independent_patterns**: 2 luồng độc lập: LED3 chu kỳ 2s và Output Y0/Y1 (`PE6`) chu kỳ tuần tự 5s qua 3 pha.
5. **lab1_ex4_multi_led_param**: Tái sử dụng 1 hàm `thread_blink` duy nhất điều khiển 3 đèn qua tham số `struct blink_cfg`.
6. **lab1_ex5_traffic_light**: Mô phỏng hệ thống đèn giao thông 3 pha (Đỏ 7s, Xanh 5s, Vàng 2s) và nháy Heartbeat 1s/1s.
