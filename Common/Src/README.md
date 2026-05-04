# Common Modülü Kaynak Dosyaları

Bu modül, hem **ECU Emulator** hem de **OBD Scanner** projeleri tarafından ortak olarak kullanılan temel kütüphaneleri, sürücüleri ve haberleşme protokollerini barındırır.

*   **`can_interface.c`**: MCP2515 donanım sürücüsü ile sistemin geri kalanı arasında köprü görevi görür. CAN mesajlarını alma/gönderme işlerini FreeRTOS kuyruklarıyla (Queue) ve `RxTask` mekanizmasıyla yöneten donanım soyutlama katmanıdır.
*   **`isotp.c`**: Klasik 8 baytlık CAN mesajlarına sığmayan büyük veri paketlerinin parçalanarak (segmentation) gönderilmesi ve alınması standartlarını belirleyen **ISO 15765-2 (ISO-TP)** taşıma katmanının (Transport Layer) implementasyonudur.
*   **`mcp2515.c`**: Harici CAN denetleyicisi olan MCP2515 entegresiyle mikrodenetleyici arasındaki SPI haberleşmesini sağlayan düşük seviyeli donanım sürücüsüdür. Filtre (mask) tanımlamaları ve hız ayarları burada yapılır.
*   **`obd_pid_encode.c`**: Sanal araç tarafından üretilen gerçek ve okunaklı sensör değerlerini (devir, sıcaklık vs.), OBD-II standartlarına uygun ham hexadecimal bayt dizilimlerine formüller kullanarak dönüştüren (kodlayan) modüldür.
*   **`ssd1306.c`**: I2C haberleşme protokolü ile çalışan SSD1306/SSD1315 OLED ekranlar için geliştirilmiş ekran sürücüsüdür. Nokta çizme, şekil çizme, frame buffer (ekran belleği) yönetimi ve metin yazdırma fonksiyonlarını barındırır.
