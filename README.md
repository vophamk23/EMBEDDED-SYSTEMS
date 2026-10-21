# LAB 3: CAN BUS NETWORK AND DISTRIBUTED SYSTEMS
**Môn học:** Hệ thống nhúng (Embedded Systems) - ĐH Bách Khoa ĐHQG-HCM  
**Kit thực hành:** Kit BKIT ARM4 (STM32F407ZGT6, CAN1 PB8/PB9)  
**RTOS:** Zephyr RTOS v4.4.0

---

## Danh sách nội dung Lab 3:
- **demos/**: 3 bài demo CAN Loopback, CAN Filters, và Giao tiếp 2 Node mạng thực tế.
- **lab3_ex1_can_sensor_loopback**: Đóng gói và truyền nhận dữ liệu cảm biến qua mạng CAN Loopback.
- **lab3_ex2_can_command_rtr**: Cơ chế hỏi/đáp và khung yêu cầu từ xa (Remote Transmission Request - RTR).
- **lab3_ex3_can_filter_priority**: Bộ lọc ID phần cứng và giải quyết xung đột đường truyền CAN theo độ ưu tiên.
- **lab3_ex4_can_heartbeat_ping**: Luồng giám sát sống còn (Heartbeat Ping-Pong) giữa các nút mạng CAN.
- **lab3_ex5_distributed_traffic_light**: Mạng đèn giao thông phân tán giao tiếp và đồng bộ trạng thái qua CAN Bus.
