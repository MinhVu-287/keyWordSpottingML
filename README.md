## 🛠 Tên Dự Án 

`Xây dựng hệ thống nhận diện giọng nói dựa trên TinyML`

Người làm chính: Thái Minh Vũ-B21DCDT250

Đồ án này tập trung vào việc phát triển một hệ thống nhận diện giọng nói đơn giản, có thể nhận diện 4 từ khóa: "bật đèn", "tắt đèn", "mở cửa" và "đóng cửa". Mục đích của hệ thống là giúp người dùng điều khiển các thiết bị như đèn và cửa chỉ bằng giọng nói mà không cần sử dụng các thiết bị điều khiển phức tạp.

## 🧰 Tổng quan về công nghệ và thư viện được dùng

•	ESP32-S3: ESP32-S3 là vi điều khiển mạnh mẽ với khả năng xử lý tín hiệu và hỗ trợ AI trực tiếp trên phần cứng. Với bộ vi xử lý này, hệ thống có thể thực hiện các tác vụ nhận diện giọng nói ngay trên thiết bị mà không cần kết nối đến máy chủ, giúp tiết kiệm băng thông và giảm độ trễ.

•	Edge Impulse: Edge Impulse là một nền tảng cho phép phát triển và huấn luyện các mô hình TinyML. Nền tảng này hỗ trợ thu thập dữ liệu, xử lý tín hiệu, huấn luyện mô hình và triển khai mô hình lên các thiết bị nhúng một cách dễ dàng và hiệu quả.

•	MFCC (Mel Frequency Cepstral Coefficients): MFCC là phương pháp phổ biến trong xử lý âm thanh, đặc biệt là trong nhận diện giọng nói. MFCC giúp chuyển đổi tín hiệu âm thanh từ dạng sóng (waveform) sang dạng đặc trưng có thể sử dụng cho học máy. Các đặc trưng này sẽ được dùng làm đầu vào cho các mô hình học máy để nhận diện lệnh giọng nói.

•	Arduino trên VS Code với PlatformIO: Môi trường phát triển này cung cấp một nền tảng lập trình quen thuộc và mạnh mẽ cho việc phát triển ứng dụng trên ESP32-S3. Nó hỗ trợ nhiều thư viện và công cụ cần thiết để lập trình và tương tác với phần cứng một cách dễ dàng.



## Hiệu suất 

Training performance 

![Image](https://github.com/user-attachments/assets/ece05b3b-d7cd-4de8-b17b-b7a7772fc064)


## 💻 Lập Trình

Ngôn ngữ: PlatformIO

Cách build:
```bash
platformio run
```
Cách upload: 
```bash 
platformio run --target upload
```

cách bật monitor:
```bash 
platformio device monitor
```


## Tài liệu chi tiết

[Đồ án hệ thống nhúng](https://docs.google.com/document/d/1MevTHBgarqiUmr2diGxpP9_f-btOGW0k/edit?usp=sharing&ouid=106490323673097930968&rtpof=true&sd=true)


## Demo

https://github.com/user-attachments/assets/b4940727-265e-4c2b-946e-b80dc0e683cf

<!-- Uploading "main.cpp - keyWord_spotting - Copy - Visual Studio Code 2025-03-30 21-13-38.mp4"... -->