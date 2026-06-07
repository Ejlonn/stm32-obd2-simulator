"""
IntelliOBD ECU — UART CSV Logger
Nucleo'dan gelen telemetri verilerini .csv dosyasına kaydeder.

Kullanım:
    python uart_csv_logger.py              # Otomatik COM port bulur
    python uart_csv_logger.py --port COM5  # Manuel port belirt
    python uart_csv_logger.py --list       # Mevcut portları listele

Gereksinim: pip install pyserial
"""

import serial
import serial.tools.list_ports
import argparse
import datetime
import sys
import os


def find_nucleo_port():
    """ST-Link Virtual COM Port'u otomatik bul."""
    ports = serial.tools.list_ports.comports()
    for p in ports:
        desc = (p.description or "").lower()
        mfg = (p.manufacturer or "").lower()
        if "stlink" in desc or "stlink" in mfg or "st-link" in desc:
            return p.device
    # Bulamazsa tek port varsa onu dön
    if len(ports) == 1:
        return ports[0].device
    return None


def list_ports():
    """Mevcut COM portlarını listele."""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("Hiç COM port bulunamadı.")
        return
    print(f"{'Port':<10} {'Açıklama':<40} {'Üretici'}")
    print("-" * 70)
    for p in ports:
        print(f"{p.device:<10} {p.description:<40} {p.manufacturer or '-'}")


def main():
    parser = argparse.ArgumentParser(description="IntelliOBD UART CSV Logger")
    parser.add_argument("--port", type=str, default=None, help="COM port (örn: COM3)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (varsayılan: 115200)")
    parser.add_argument("--output", type=str, default=None, help="Çıktı dosya adı (.csv)")
    parser.add_argument("--list", action="store_true", help="Mevcut portları listele")
    args = parser.parse_args()

    if args.list:
        list_ports()
        return

    port = args.port or find_nucleo_port()
    if port is None:
        print("HATA: COM port bulunamadı. --port COM3 şeklinde belirtin veya --list ile kontrol edin.")
        sys.exit(1)

    os.makedirs("logs", exist_ok=True)
    filename = args.output or f"logs/ecu_log_{datetime.datetime.now():%Y%m%d_%H%M%S}.csv"

    print(f"Port     : {port}")
    print(f"Baud     : {args.baud}")
    print(f"Dosya    : {filename}")
    print(f"Durdurmak için Ctrl+C\n")

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"HATA: Port açılamadı — {e}")
        sys.exit(1)

    line_count = 0
    with open(filename, "w", newline="") as f:
        try:
            while True:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="ignore").strip()
                if not line:
                    continue

                # Boot mesajlarını ve debug loglarını filtrele, sadece CSV satırlarını kaydet
                if line.startswith("[") or line.startswith("===") or line.startswith("Task") or line.startswith("ECU"):
                    print(f"  [debug] {line}")
                    continue

                f.write(line + "\n")
                f.flush()
                line_count += 1

                if line_count == 1:
                    print(f"  Header : {line}")
                elif line_count % 10 == 0:
                    print(f"  [{line_count} satır] {line}")

        except KeyboardInterrupt:
            print(f"\n\nKaydedildi: {filename} ({line_count} satır)")
        finally:
            ser.close()


if __name__ == "__main__":
    main()
