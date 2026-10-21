# LAB 2: SYNCHRONIZATION AND IPC IN ZEPHYR RTOS
**Môn học:** Hệ thống nhúng (Embedded Systems) - ĐH Bách Khoa ĐHQG-HCM  
**Kit thực hành:** Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 168 MHz)  
**RTOS:** Zephyr RTOS v4.4.0

---

## Danh sách nội dung Lab 2:
- **demos/**: 4 bài demo mẫu về Binary Semaphore, Counting Semaphore, Mutex, và Message Queue.
- **lab2_ex1_relay_binary_sem**: Điều khiển Relay bảo vệ quá nhiệt bằng Binary Semaphore.
- **lab2_ex2_timeout_monitor**: Giám sát lỗi Timeout cảm biến bằng Semaphore định thời.
- **lab2_ex3_counting_sem_workers**: Quản lý hồ chứa tài nguyên (Worker Threads Pool) bằng Counting Semaphore.
- **lab2_ex4_isr_message_queue**: Giao tiếp ISR ngắt phần cứng tới Thread qua Message Queue.
- **lab2_ex5_pedestrian_traffic_light**: Đèn giao thông ưu tiên người đi bộ bấm nút khẩn cấp.
