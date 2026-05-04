# IntelliOBD Simulator (STM32 ECU Emulator & Scanner)

Bu proje, otomotiv endüstrisinde kullanılan **OBD-II (On-Board Diagnostics) haberleşme protokolünü** donanım düzeyinde simüle eden çift düğümlü bir gömülü sistem uygulamasıdır. 

Sistem iki adet STM32 mikrokontrolcüden oluşmaktadır:
1. **ECU Emulator:** Bir aracın motor kontrol ünitesini (ECU) simüle eder. Sensör verilerini (RPM, Hız, Sıcaklık) üretir ve ISO-TP (ISO 15765-2) standartlarında gelen diagnoz isteklerine yanıt verir.
2. **OBD Scanner:** Gerçek bir arıza tespit cihazı gibi çalışarak ECU'dan sensör verilerini, arıza kodlarını (DTC) ve araç kimlik numarasını (VIN) çeker; verileri OLED ekran üzerinde gösterir.

## 🛠 Donanım Gereksinimleri

Proje tamamen gömülü donanım üzerinde çalışmak üzere tasarlanmıştır. Bu projeyi çalıştırmak için aşağıdaki bileşenlere ihtiyacınız vardır:

* **2 x STM32F411RE Nucleo-64** Geliştirme Kartı
* **2 x MCP2515 CAN Bus Modülü** (SPI arabirimli)
* **1 x SSD1306/SSD1315 OLED Ekran** (I2C arabirimli)
* **4 x Push Buton** (Menü kontrolü için)
* Jumper kablolar ve breadboard

### Pin Bağlantıları

| MCP2515 Pin | STM32F411RE (ECU & Scanner) |
| :--- | :--- |
| **VCC** | 5V |
| **GND** | GND |
| **CS** | PB6 |
| **SO (MISO)** | PA6 (SPI1_MISO) |
| **SI (MOSI)** | PA7 (SPI1_MOSI) |
| **SCK** | PA5 (SPI1_SCK) |
| **INT** | PA0 (EXTI0) |

| OLED Ekran | STM32F411RE (Sadece Scanner) |
| :--- | :--- |
| **SDA** | PB9 (I2C1_SDA) |
| **SCL** | PB8 (I2C1_SCL) |

| Butonlar | STM32F411RE (Sadece Scanner) |
| :--- | :--- |
| **K1 (Yukarı/Live Data)** | PC0 (Pull-up) |
| **K2 (Aşağı/Read DTC)** | PC1 (Pull-up) |
| **K3 (Seç/Clear DTC)** | PC2 (Pull-up) |
| **K4 (Geri/VIN)** | PC3 (Pull-up) |

> **Not:** İki MCP2515 modülünün CAN_H ve CAN_L pinleri birbirine bağlanmalıdır. İletişim hattının her iki ucunda 120 ohm sonlandırma direnci bulunduğundan emin olun.

## 🏗 Sistem Mimarisi ve Yazılım

* **RTOS:** FreeRTOS (Task yönetimi, Queue ve Semaphore yapısı)
* **Haberleşme Katmanı:** CAN 2.0B / ISO-TP (ISO 15765-2) Transport Layer
* **OBD Protokolü:** ISO 15031 / SAE J1979
* **Sürücü Mimarisi:** CMSIS / STM32 HAL / Özel yazılmış MCP2515 sürücüsü

Yazılım, `Common`, `ECU_Emulator` ve `OBD_Scanner` olmak üzere üç ana modüle ayrılmıştır:
- **`Common/`**: Her iki projede de ortak kullanılan CAN Interface, ISO-TP motoru ve donanım sürücüleri.
- **`ECU_Emulator/`**: Sensör modelleri (RPM artışı/azalışı vb.) ve OBD servis yanıt motoru.
- **`OBD_Scanner/`**: İstek motoru, OLED UI yönetimi ve buton kesmeleri.

## 🚀 Kurulum ve Derleme (Nasıl Kullanılır?)

Bu repo, standart bir STM32CubeIDE Workspace formatındadır.

1. **Repoyu Klonlayın:**
   ```bash
   git clone <repo_url>
   ```
2. **STM32CubeIDE ile Açın:**
   * STM32CubeIDE'yi açın.
   * `File -> Import -> General -> Existing Projects into Workspace` seçeneğine tıklayın.
   * Klonladığınız dizini seçip `ECU_Emulator` ve `OBD_Scanner` projelerini içe aktarın.
3. **Derleme (Build):**
   * Her iki projeyi de sırayla seçip **Project -> Build Project** menüsü ile derleyin.
4. **Yükleme (Flash):**
   * Birinci STM32'yi bilgisayarınıza bağlayın ve `ECU_Emulator` projesini yükleyin.
   * İkinci STM32'yi bağlayın ve `OBD_Scanner` projesini yükleyin.

## 🎮 Kullanım

Her iki cihaz güce bağlandığında:
1. **ECU Emulator** sessizce çalışmaya başlar, CAN hattından gelecek istekleri dinler.
2. **OBD Scanner** ekranında ana menü belirir.
3. Butonları kullanarak "Live Data", "Read DTC" (Arıza Kodu Oku), "Clear DTC" veya "Vehicle Info" menülerine girebilirsiniz.
4. Scanner, ECU'ya ISO-TP paketleri atar, ECU bunları işler ve sensör/hata verilerini döndürür. Sonuç anlık olarak ekranda gösterilir.
