# ECU Emulator Modülü Kaynak Dosyaları

Bu modül, bir aracın Motor Kontrol Ünitesini (ECU) simüle eden ve dışarıdan gelen OBD diagnoz cihazlarının isteklerine cevap veren projenin kaynak dosyalarını içerir.

*   **`app_tasks_ecu.c`**: ECU simülatörünün ana sistem başlatıcısı (orchestrator) ve görev dağıtıcısıdır. Donanımları başlatır ve SPI iletişimi, CAN dinleme mekanizması ve sensör modelleme gibi FreeRTOS task'larını oluşturur.
*   **`button_handler.c`**: ECU kartı üzerindeki fiziksel butonların durumlarını okuyan, sinyaldeki elektriksel titreşimleri filtreleyen (debounce) ve buton hareketlerini bir olaya dönüştürüp sistem kuyruğuna aktaran yöneticidir.
*   **`fault_manager.c`**: Sanal aracın Hata Teşhis Kodlarını (DTC - Diagnostic Trouble Codes) hafızasında tutan, yenilerini ekleyen ve dışarıdan (Scanner'dan) gelen DTC silme isteklerini gerçekleştiren modüldür.
*   **`obd_service_ecu.c`**: OBD-II protokolünün sunucu (server) tarafıdır. ISO-TP üzerinden gönderilen Service 01 (Sensör Verisi), Service 03 (Arıza Oku) ve Service 09 (VIN Oku) gibi standart OBD komutlarını çözer ve uygun yanıt paketleri hazırlar.
*   **`sensor_model.c`**: Fiziksel bir aracın motor ve hareket dinamiklerini taklit eden matematiksel modeldir. Zaman ilerledikçe motor devrini (RPM), araç hızını (Speed) ve motor suyu sıcaklığını mantıklı oranlarda artırır ve azaltır.
