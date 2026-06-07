# IntelliOBD Simulator (STM32 ECU Emulator & Scanner)

Bu proje, otomotiv endüstrisinde kullanılan **OBD-II (On-Board Diagnostics) haberleşme protokolünü** donanım düzeyinde simüle eden çift düğümlü bir gömülü sistem uygulamasıdır. 

Sistem iki adet STM32 mikrokontrolcüden oluşmaktadır:
1. **ECU Emulator:** Bir aracın motor kontrol ünitesini (ECU) simüle eder. Sensör verilerini (RPM, Hız, Sıcaklık) üretir ve ISO-TP (ISO 15765-2) standartlarında gelen tanılama isteklerine yanıt verir. Ayrıca bir potansiyometre aracılığıyla donanımsal gaz pedalı simülasyonunu destekler ve makine öğrenmesi/analiz için UART üzerinden telemetri logları (CSV) sunar.
2. **OBD Scanner:** Gerçek bir arıza tespit cihazı gibi çalışarak ECU'dan sensör verilerini, arıza kodlarını (DTC) ve araç kimlik numarasını (VIN) çeker; verileri OLED ekran üzerinde gösterir.

## Donanım Gereksinimleri

Proje tamamen gömülü donanım üzerinde çalışmak üzere tasarlanmıştır. Bu projeyi çalıştırmak için aşağıdaki bileşenlere ihtiyacınız vardır:

* **2 x STM32F411RE Nucleo-64** Geliştirme Kartı
* **2 x MCP2515 CAN Bus Modülü** (SPI arabirimli)
* **1 x SSD1306/SSD1315 OLED Ekran** (I2C arabirimli)
* **1 x 10kΩ Potansiyometre** (ECU Gaz pedalı simülasyonu için)
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
| **INT** | PB0 (EXTI0 - Falling Edge) |

| Potansiyometre | STM32F411RE (ECU Emulator) |
| :--- | :--- |
| **VCC (Sol Bacak)** | 3.3V *(Kesinlikle 5V bağlamayın!)* |
| **GND (Sağ Bacak)** | GND |
| **Wiper (Orta Bacak)**| PA0 (ADC1_IN0) |

| OLED Ekran | STM32F411RE (Scanner) |
| :--- | :--- |
| **SDA** | PB9 (I2C1_SDA) |
| **SCL** | PB8 (I2C1_SCL) |

| Butonlar | STM32F411RE (Scanner) |
| :--- | :--- |
| **K1 (Yukarı/Live Data)** | PC0 (Pull-up) |
| **K2 (Aşağı/Read DTC)** | PC1 (Pull-up) |
| **K3 (Seç/Clear DTC)** | PC2 (Pull-up) |
| **K4 (Geri/VIN)** | PC3 (Pull-up) |

> **Not:** İki MCP2515 modülünün CAN_H ve CAN_L pinleri birbirine bağlanmalıdır. İletişim hattının her iki ucunda 120 ohm sonlandırma direnci bulunmalıdır.

## Sistem Mimarisi ve Yazılım

* **RTOS:** FreeRTOS (Task yönetimi, Queue ve Semaphore yapısı)
* **Haberleşme Katmanı:** CAN 2.0B / ISO-TP (ISO 15765-2) Transport Layer
* **OBD Protokolü:** ISO 15031 / SAE J1979
* **Sürücü Mimarisi:** CMSIS / STM32 HAL / Özel yazılmış MCP2515 sürücüsü

Yazılım, `Common`, `ECU_Emulator` ve `OBD_Scanner` olmak üzere üç ana modüle ayrılmıştır:
- **`Common/`**: Her iki projede de ortak kullanılan CAN Interface, ISO-TP motoru ve donanım sürücüleri.
- **`ECU_Emulator/`**: Sensör modelleri (ADC destekli RPM/Hız hesabı) ve OBD servis yanıt motoru.
- **`OBD_Scanner/`**: İstek motoru, OLED UI yönetimi ve buton kesmeleri.

## Kurulum ve Derleme (Nasıl Kullanılır?)

Bu repo, standart bir STM32CubeIDE Workspace formatındadır.

1. **Repoyu Klonlayın:**
   ```bash
   git clone https://github.com/Ejlonn/stm32-obd2-simulator.git
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

## Kullanım & Veri Loglama

Her iki cihaz güce bağlandığında:
1. **ECU Emulator** sessizce çalışmaya başlar, CAN hattından gelecek istekleri dinler.
2. **OBD Scanner** ekranında ana menü belirir.
3. Butonları kullanarak "Live Data", "Read DTC" (Arıza Kodu Oku), "Clear DTC" veya "Vehicle Info" menülerine girebilirsiniz.
4. Scanner, ECU'ya ISO-TP paketleri atar, ECU bunları işler ve sensör/hata verilerini döndürür. Sonuç anlık olarak ekranda gösterilir.
5. **Gaz Pedalı:** ECU'ya bağlı potansiyometreyi çevirerek RPM (motor devri) değerini anlık olarak değiştirebilirsiniz. Hız ve vites oranları bu RPM değerine göre dinamik olarak hesaplanıp simüle edilir.
6. **UART CSV Loglama (Makine Öğrenmesi İçin):** ECU'nun USB (Virtual COM Port) çıkışından saniyede 10 kez telemetri verisi akar. Verileri `tools/uart_csv_logger.py` scripti ile yakalayıp `.csv` formatında kaydedebilir ve model eğitmek için kullanabilirsiniz. (Bağlantı ayarları: 115200 Baud, 8N1).

---

# IntelliOBD Simulator (STM32 ECU Emulator & Scanner) - English

This project is a dual-node embedded system application that simulates the **OBD-II (On-Board Diagnostics) communication protocol** used in the automotive industry at the hardware level.

The system consists of two STM32 microcontrollers:
1. **ECU Emulator:** Simulates a vehicle's Engine Control Unit (ECU). It generates sensor data (RPM, Speed, Temperature) and responds to diagnostic requests coming in ISO-TP (ISO 15765-2) standards. It also supports hardware-based throttle simulation via a potentiometer and outputs telemetry logs (CSV) over UART for machine learning/analysis.
2. **OBD Scanner:** Acts like a real diagnostic scanner tool; retrieves sensor data, diagnostic trouble codes (DTC), and vehicle identification number (VIN) from the ECU, displaying the data on an OLED screen.

## Hardware Requirements

The project is designed to run entirely on embedded hardware. To run this project, you need the following components:

* **2 x STM32F411RE Nucleo-64** Development Boards
* **2 x MCP2515 CAN Bus Modules** (SPI interface)
* **1 x SSD1306/SSD1315 OLED Display** (I2C interface)
* **1 x 10kΩ Potentiometer** (For ECU Throttle simulation)
* **4 x Push Buttons** (For menu control)
* Jumper wires and breadboard

### Pin Connections

| MCP2515 Pin | STM32F411RE (ECU & Scanner) |
| :--- | :--- |
| **VCC** | 5V |
| **GND** | GND |
| **CS** | PB6 |
| **SO (MISO)** | PA6 (SPI1_MISO) |
| **SI (MOSI)** | PA7 (SPI1_MOSI) |
| **SCK** | PA5 (SPI1_SCK) |
| **INT** | PB0 (EXTI0-Falling Edge) |

| Potentiometer | STM32F411RE (ECU Emulator) |
| :--- | :--- |
| **VCC (Left Pin)** | 3.3V *(DO NOT USE 5V!)* |
| **GND (Right Pin)** | GND |
| **Wiper (Middle Pin)**| PA0 (ADC1_IN0) |

| OLED Display | STM32F411RE (Scanner) |
| :--- | :--- |
| **SDA** | PB9 (I2C1_SDA) |
| **SCL** | PB8 (I2C1_SCL) |

| Buttons | STM32F411RE (Scanner) |
| :--- | :--- |
| **K1 (Up/Live Data)** | PC0 (Pull-up) |
| **K2 (Down/Read DTC)** | PC1 (Pull-up) |
| **K3 (Select/Clear DTC)** | PC2 (Pull-up) |
| **K4 (Back/VIN)** | PC3 (Pull-up) |

> **Note:** The CAN_H and CAN_L pins of the two MCP2515 modules must be connected to each other. Ensure there is a 120-ohm termination resistor at both ends of the communication line.

## System Architecture and Software

* **RTOS:** FreeRTOS (Task management, Queue, and Semaphore structures)
* **Communication Layer:** CAN 2.0B / ISO-TP (ISO 15765-2) Transport Layer
* **OBD Protocol:** ISO 15031 / SAE J1979
* **Driver Architecture:** CMSIS / STM32 HAL / Custom-written MCP2515 driver

The software is divided into three main modules: `Common`, `ECU_Emulator`, and `OBD_Scanner`:
- **`Common/`**: CAN Interface, ISO-TP engine, and hardware drivers shared across both projects.
- **`ECU_Emulator/`**: Sensor models (ADC-supported RPM/Speed calculation) and OBD service response engine.
- **`OBD_Scanner/`**: Request engine, OLED UI management, and button interrupts.

## Setup and Build (How to Use?)

This repo is structured as a standard STM32CubeIDE Workspace.

1. **Clone the Repo:**
   ```bash
   git clone https://github.com/Ejlonn/stm32-obd2-simulator.git
   ```
2. **Open with STM32CubeIDE:**
   * Open STM32CubeIDE.
   * Click on `File -> Import -> General -> Existing Projects into Workspace`.
   * Select the cloned directory and import both the `ECU_Emulator` and `OBD_Scanner` projects.
3. **Build:**
   * Select each project sequentially and compile using the **Project -> Build Project** menu.
4. **Flash:**
   * Connect the first STM32 to your computer and flash the `ECU_Emulator` project.
   * Connect the second STM32 and flash the `OBD_Scanner` project.

## Usage & Data Logging

When both devices are powered on:
1. The **ECU Emulator** silently starts running and listens for incoming requests on the CAN line.
2. The main menu appears on the **OBD Scanner** display.
3. Use the buttons to navigate into "Live Data", "Read DTC", "Clear DTC", or "Vehicle Info" menus.
4. The Scanner sends ISO-TP packets to the ECU, the ECU processes them and returns sensor/fault data. The result is instantly displayed on the screen.
5. **Throttle Pedal:** By turning the potentiometer connected to the ECU, you can dynamically change the RPM. Speed and gear ratios are calculated and simulated based on this RPM value.
6. **UART CSV Logging (For Machine Learning):** Telemetry data flows from the ECU's USB (Virtual COM Port) at 10Hz. You can capture this data using the `tools/uart_csv_logger.py` script and save it in `.csv` format for training models. (Connection settings: 115200 Baud, 8N1).
