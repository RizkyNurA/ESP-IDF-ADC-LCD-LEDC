# Smart Load Cell Indicator

Firmware Smart Load Cell Indicator berbasis ESP32 menggunakan HX711 dan LCD 16x2.

## Features

* Support hingga 4 load cell
* HX711 ADC interface
* Kalibrasi langsung dari LCD
* Penyimpanan parameter menggunakan NVS
* Monitoring berat realtime
* 3 channel alarm output
* Konfigurasi alarm melalui tombol dan LCD

### Trigger Mode

* Level
* Stable High

### Output Mode

* Direct
* ON Delay
* OFF Delay
* Timed Stop

---

## Folder Structure

```text
main/
├── app/
│   ├── app_logic.c
│   ├── app_logic.h
│   ├── editor.c
│   ├── editor.h
│   ├── selector.c
│   └── selector.h
│
├── common/
│   ├── types.h
│   ├── utils.c
│   └── utils.h
│
├── config/
│   └── config.h
│
├── core/
│   ├── drivers.c
│   └── drivers.h
│
├── drivers/
│   ├── adc/
│   ├── button/
│   ├── hx711/
│   ├── lcd/
│   └── led/
│
├── system/
│   ├── app_context.c
│   └── app_context.h
│
├── main.c
└── CMakeLists.txt
```

---

## Alarm Configuration

Setiap alarm memiliki parameter:

* Alarm Mode
* Threshold Low
* Threshold High
* Trigger Mode
* Output Mode
* Trigger Delay
* Output Delay
* Output Delay 2

### Alarm Mode

* Atas
* Bawah
* Dalam Range
* Luar Range

### Trigger Mode

#### Level

Output mengikuti kondisi alarm secara langsung.

#### Stable High

Kondisi harus aktif selama waktu tertentu sebelum dianggap valid.

### Output Mode

#### Direct

Output langsung mengikuti trigger.

#### ON Delay

Output aktif setelah delay tertentu.

#### OFF Delay

Output tetap aktif selama delay tertentu setelah trigger hilang.

#### Timed Stop

Output ON selama waktu T1 kemudian OFF selama waktu T2.

---

## Build

```bash
idf.py build
```

## Flash

```bash
idf.py flash monitor
```

---

## Hardware

* ESP32
* HX711
* Load Cell
* LCD 16x2
* Push Button
* Relay Output

---
