# OBD Scanner Modülü Kaynak Dosyaları

Bu modül, araçlara takılan Arıza Tespit Cihazlarının (Diagnostic Scanner) davranışlarını taklit eder, ECU'ya istek atar ve gelen verileri işleyip ekrana yansıtır.

*   **`app_tasks_scanner.c`**: Scanner sisteminin ana görev mimarıdır. CAN haberleşme, buton dinleme, veri iletişim ve OLED ekran güncelleme işlemlerini paralel olarak yürütecek FreeRTOS görevlerini (task) ayağa kaldırır.
*   **`obd_pid_decode.c`**: ECU'dan alınan ham hexadecimal bayt formundaki sensör yanıtlarını işleyen çözücüdür. Gelen verileri OBD-II mühendislik formüllerinden geçirerek insanların okuyabileceği (örn: km/h, RPM) fiziksel değerlere çevirir.
*   **`scanner_buttons.c`**: Scanner cihazındaki K1, K2, K3 ve K4 menü butonlarının basım durumlarını okuyan, sinyal gürültülerini filtreleyen ve UI/İletişim tasklarına gönderilecek menü komutlarını üreten yöneticidir.
*   **`scanner_comm.c`**: OBD-II istek/yanıt döngüsünün tam merkezidir. Kullanıcının arayüzden seçtiği işleme göre (Canlı Veri okuma, Arıza Kodu silme vb.) doğru ISO-TP paketini oluşturup ECU'ya yollar ve senkronize şekilde cevabı bekler.
*   **`scanner_ui.c`**: Cihazın kullanıcı ile iletişim kurduğu ana görsel katmandır. Sensör değerlerini, hata kodlarını (DTC) ve menü ekranlarını dinamik olarak yönetip, SSD1306 kütüphanesi yardımıyla OLED ekrana çizer.
