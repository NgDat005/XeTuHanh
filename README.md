📌 Giới thiệu
Dự án Xe tự hành sử dụng ESP32 được xây dựng nhằm mô phỏng một hệ thống di động thông minh có khả năng:
  - Điều khiển chuyển động qua giao diện web
  - Tự động tránh vật cản bằng cảm biến siêu âm
  - Quét môi trường bằng servo
  - Hoạt động ổn định trên nền tảng Arduino (ESP32)


⚙️ Chức năng chính:
  - Điều khiển xe thông qua Web Server ESP32
  - Điều khiển động cơ DC: tiến, lùi, rẽ trái, rẽ phải, dừng
  - Phát hiện và đo khoảng cách vật cản bằng HC-SR04
  - Servo quay cảm biến để quét không gian phía trước
  - Tự động dừng hoặc đổi hướng khi phát hiện vật cản
  - LED báo trạng thái hệ thống

🧰 Phần cứng sử dụng
  ESP32 Dev Module
  Driver động cơ (L298N / L293D)
  Động cơ DC + bánh xe
  Cảm biến siêu âm HC-SR04
  Servo SG90
  Cảm biến dò line 5 mắt
  LED báo hiệu
  Nguồn pin / adapter

💻 Công nghệ sử dụng:
  Ngôn ngữ lập trình: C / C++ (Arduino Framework)
  ESP32 WebServer
  HTML + CSS + JavaScript (Joystick điều khiển)
  Giao tiếp: GPIO, PWM
  Lập trình nhúng & IoT
