# Smart Load Cell Indicator

Smart Load Cell Indicator berbasis ESP32 untuk aplikasi penimbangan dan sortir produk. Sistem menggunakan beberapa load cell yang dibaca melalui HX711, menampilkan berat secara real-time pada LCD, serta menyediakan fitur alarm dengan berbagai mode trigger dan output untuk kebutuhan otomasi industri.

## Features

* Multi Load Cell Support

  * Mendukung hingga 4 load cell.
  * Pembacaan berat individual dan total.

* Calibration System

  * Tare calibration.
  * Known weight calibration.
  * Penyimpanan parameter kalibrasi ke NVS.

* Alarm System

  * 3 channel alarm independen.
  * Mode alarm:

    * Above
    * Below
    * Inside Range
    * Outside Range

* Advanced Trigger

  * Level Trigger
  * Stable High Trigger

* Advanced Output

  * Direct Output
  * ON Delay
  * OFF Delay
  * Timed Stop Sequence

* Persistent Storage

  * Semua konfigurasi tersimpan pada ESP32 NVS.
  * Tetap tersimpan setelah power off.

* FreeRTOS Based

  * Task terpisah untuk:

    * HX711 acquisition
    * LCD update
    * Button handling
    * Alarm processing

---

## Hardware

### Main Controller

* ESP32

### Weight Measurement

* HX711 Load Cell Amplifier
* 1–4 Load Cell

### User Interface

* 16x2 LCD
* 3 Push Buttons

### Output

* 3 Relay Outputs

---

## Software Architecture

### Tasks

#### HX711 Task

Bertugas membaca data load cell secara periodik dan memperbarui nilai raw ADC.

#### LCD Task

Menampilkan:

* Total weight
* Individual load cell weight
* Calibration menu
* Alarm configuration
* Advanced alarm configuration

#### Button Task

Mendeteksi:

* Short press
* Long press
* Very long press

dan mengubahnya menjadi event aplikasi.

#### Alarm Logic

Proses alarm terdiri dari tiga tahap:

```text
Condition
    ↓
Trigger Logic
    ↓
Output Logic
    ↓
Relay Output
```

---

## Trigger Modes

### Level

Output mengikuti kondisi alarm secara langsung.

### Stable High

Kondisi alarm harus aktif selama waktu tertentu sebelum dianggap valid.

---

## Output Modes

### Direct

Relay mengikuti trigger secara langsung.

### ON Delay

Relay aktif setelah trigger valid selama waktu tertentu.

### OFF Delay

Relay tetap aktif beberapa saat setelah trigger hilang.

### Timed Stop

Urutan kerja:

```text
Trigger Detected
      ↓
Relay ON (T1)
      ↓
Relay OFF (T2)
      ↓
Ready For Next Product
```

Mode ini cocok untuk aplikasi product sorting menggunakan actuator atau pneumatic gate.

---

## Project Structure

```text
main/
├── app/
│   ├── app_logic.c
│   ├── app_logic.h
│   ├── app_context.c
│   └── app_context.h
│
├── core/
│   ├── drivers.c
│   ├── drivers.h
│   ├── hx711_driver.c
│   ├── hx711_driver.h
│   └── lcd_driver.c
│
├── ui/
│   ├── editor.c
│   ├── selector.c
│   └── render.c
│
├── storage/
│   └── nvs_helper.c
│
└── types.h
```

---

## Build

ESP-IDF Required

```bash
idf.py build
```

Flash Firmware

```bash
idf.py flash
```

Monitor Serial

```bash
idf.py monitor
```

---

## Applications

* Checkweigher
* Weight Sorting Machine
* Conveyor Sorting System
* Packaging Line
* Industrial Weight Monitoring

---

## Author

Developed using ESP32, FreeRTOS, and ESP-IDF.
