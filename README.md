# 🔬 Pipe Bend Ovality Measurement Instrument

> **Developed for IGCAR Kalpakkam (Indira Gandhi Centre for Atomic Research)**  
> A compact, handheld, battery-powered inductance-based instrument for measuring the ovality of pipe bends in nuclear plant pipelines.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--C3-green.svg)
![Sensor](https://img.shields.io/badge/sensor-LDC1612-orange.svg)
![Status](https://img.shields.io/badge/status-Validated%20at%20IGCAR-brightgreen.svg)

---

## 📸 Instrument Overview

> *Add photos of your final instrument here — OLED showing a measurement, the enclosure, jaws on a pipe*

| View | Description |
|------|-------------|
| ![Front]() | Front view — OLED display + buttons |
| ![Jaws]() | Jaw mechanism measuring a pipe |
| ![Result]() | OLED result screen showing ovality % |

---

## 📐 What is Ovality?

When a pipe bend is subjected to bending loads, its cross-section deforms from a circle to an oval shape. **Ovality** quantifies this deformation:

$$W(\%) = \frac{D_{max} - D_{min}}{D_0} \times 100$$

Where:
- **D₀** = nominal outer diameter (straight section)
- **Dmax** = maximum outer diameter at the bend
- **Dmin** = minimum outer diameter at the bend

Ovality measurement is a critical quality criterion for pipe bend validation in nuclear installations.

---

## ✨ Features

- 📏 **0–35mm pipe OD measurement range** — covers all IGCAR pipe bend sizes
- 🔋 **~17 hour battery life** — LiPo 1500mAh, full shift coverage
- ⚡ **USB-C charging** — via TP4056 module, charge while off or on
- 📺 **SH1106 1.3" OLED display** — animated UI, step-by-step guided workflow
- 🎯 **35-point fractional calibration** — any decimal position (c0, c2.5, c17.3...)
- 🔧 **Vernier caliper-style friction slider** — zero force on pipe during measurement
- 🛡️ **I2C latch-up protection** — 100Ω series resistors on SDA/SCL
- 📡 **BLE 5.0 capable** — ESP32-C3, ready for future Flutter companion app
- 🖨️ **Fully 3D-printed enclosure** — PETG body, 212×44×28mm

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    OVALITY METER SYSTEM                      │
├──────────────┬──────────────┬──────────────┬────────────────┤
│    POWER     │   SENSING    │  PROCESSING  │   USER I/F     │
│              │              │              │                │
│ LiPo 1500mAh │ Solenoid coil│ ESP32-C3     │ 1.3" SH1106   │
│ Slide switch │ 28AWG ~90T  │ SuperMini    │ OLED 128×64   │
│ TP4056 USB-C │ Ferrite rod │ 160MHz RISC-V│ BTN MEASURE   │
│ 1N5819 diode │ LDC1612     │ BLE 5.0      │ BTN RESET     │
│ → 3.4V rail  │ 28-bit I2C  │ 4MB Flash    │ Slide switch  │
└──────────────┴──────────────┴──────────────┴────────────────┘
         Shared I2C bus: GPIO4=SDA, GPIO5=SCL
         LDC1612 @ 0x2B  |  OLED @ 0x3C
```

---

## 🔌 Hardware Components

| Component | Specification | Notes |
|-----------|--------------|-------|
| Microcontroller | ESP32-C3 SuperMini | RISC-V 160MHz, BLE 5.0 |
| Inductive sensor | Seeed Grove LDC1612 | 28-bit, I2C 0x2B |
| OLED display | 1.3" SH1106 128×64 | I2C 0x3C |
| Battery | LiPo 3.7V 1500mAh | 3-wire (RED/BLACK/WHITE-NTC) |
| Charger | TP4056 USB-C module | B+/B− only, OUT pins unused |
| Power diode | 1N5819 Schottky | ~0.3V drop, protects ESP32 |
| Coil | 28AWG enamelled wire | ~90 turns, 58mm tube, 19mm OD |
| Ferrite rod | 85×10mm | 15mm screw hold, 70mm working |
| Coil tube | 3D printed PETG | 2mm grooves, 18 sections |
| Enclosure | 3D printed PETG | 212×44×28mm body |
| Buttons | 6×6mm tactile | MEASURE (GPIO2), RESET (GPIO3) |

---

## ⚡ Circuit Connections

### Power Chain
```
LiPo RED (+) ──┬──── TP4056 B+          ← charging path (always connected)
               └──── Slide switch leg 1
                          │
                     Slide switch leg 2
                          │
                     1N5819 ANODE
                          │
                     1N5819 CATHODE ──┬── ESP32-C3 3V3
                                      ├── LDC1612 VCC
                                      └── OLED VCC

LiPo BLACK (−) ──┬── TP4056 B−
                 └── GND rail

TP4056 OUT+, OUT− → leave unconnected
LiPo WHITE (NTC) → tape the tip, leave unconnected
```

### Complete Pin Table

| FROM | TO | WIRE |
|------|----|------|
| LiPo RED | TP4056 B+ | Red |
| LiPo RED | Slide switch leg 1 | Red |
| Slide switch leg 2 | 1N5819 ANODE | Red |
| 1N5819 CATHODE | ESP32-C3 3V3 | Orange |
| 1N5819 CATHODE | LDC1612 VCC | Orange |
| 1N5819 CATHODE | OLED VCC | Orange |
| LiPo BLACK | TP4056 B− | Black |
| LiPo BLACK | GND rail | Black |
| LDC1612 GND | GND rail | Black |
| LDC1612 SDA | ESP32-C3 GPIO4 | Yellow |
| LDC1612 SCL | ESP32-C3 GPIO5 | Yellow |
| LDC1612 CH0 IN0A | Coil end A | Bare |
| LDC1612 CH0 IN0B | Coil end B | Bare |
| OLED GND | GND rail | Black |
| OLED SDA | ESP32-C3 GPIO4 | Yellow |
| OLED SCL | ESP32-C3 GPIO5 | Yellow |
| BTN MEASURE leg A | ESP32-C3 GPIO2 | Grey |
| BTN MEASURE leg B | GND rail | Black |
| BTN RESET leg A | ESP32-C3 GPIO3 | Grey |
| BTN RESET leg B | GND rail | Black |
| ESP32-C3 GND | GND rail | Black |

---

## 💻 Software Setup

### Requirements

Install these libraries in Arduino IDE (Manage Libraries):
- **Adafruit SH110X** by Adafruit
- **Adafruit GFX Library** by Adafruit

### Board Settings

```
Board:              ESP32C3 Dev Module
USB CDC On Boot:    Enabled  ← REQUIRED for Serial Monitor
Port:               Your COM port
```

### Flash Firmware

1. Open `firmware/ovality_meter_v1_3.ino` in Arduino IDE
2. Verify board settings above
3. Click Upload

---

## 📊 Calibration

The instrument uses a **fractional calibration system** — you can record at any decimal jaw position.

### Calibration Procedure

```
1. Set CALIBRATION_DONE = false → Upload
2. Open Serial Monitor at 115200 baud
3. Use digital caliper to set exact jaw position
4. Type command and press Enter:
   c0       → record at 0mm
   c2.5     → record at 2.5mm
   c17.3    → record at 17.3mm
   (any decimal value, any order)
5. Type p → prints sorted arrays ready to paste
6. Copy the 3 printed lines into firmware
7. Set CALIBRATION_DONE = true → Re-upload
```

### Calibration Commands

| Command | Action |
|---------|--------|
| `c<mm>` | Record current raw value at jaw position (e.g. `c0`, `c2.5`) |
| `p` | Print all recorded points sorted, formatted for paste |
| `r` | Clear all points and start over |
| `t` | Show current raw value without recording |

### Current Calibration Data

35 points covering 0–34mm at 1mm steps. Zero monotonicity violations.

---

## 📱 Measurement Procedure

```
STEP 1 → Press MEASURE          STEP 2 → Press MEASURE
         Open jaw                        Rotate 90°
         Place at widest point           Place at narrowest point
         Press MEASURE to lock  →        Press MEASURE to lock
         Dmax locked ✓                   Dmin locked ✓

STEP 3 → Press MEASURE          STEP 4 → Press MEASURE
         Move to straight section         View result:
         Measure nominal diameter         OVALITY = X.XX%
         Press MEASURE to lock  →        GOOD / MODERATE / HIGH / CRITICAL
         D0 locked ✓
```

**RESET button** clears all values and returns to start at any time.

### Result Classification

| Ovality % | Classification |
|-----------|---------------|
| < 3% | 🟢 GOOD |
| 3–8% | 🟡 MODERATE |
| 8–15% | 🟠 HIGH |
| ≥ 15% | 🔴 CRITICAL |

---

## 🔧 Coil Winding

- **Tube:** 58mm length, 19mm OD, 11mm ID (3D printed PETG)
- **Grooves:** 18 grooves × 2mm wide × 1mm deep, 1mm land between
- **Wire:** 28AWG enamelled copper
- **Method:** Wind 5 turns per groove, seal with cyanoacrylate
- **Total turns:** ~90

The **deep marker groove at 5mm** from entry end is the assembly zero reference. Align the ferrite rod tip here with jaws closed, then tighten the set screw.

---

## 📁 Repository Structure

```
igcar-ovality-meter/
├── firmware/
│   └── ovality_meter_v1_3.ino     ← Complete Arduino firmware
├── docs/
│   ├── circuit_connections.md     ← Full wiring guide
│   ├── calibration_guide.md       ← Step-by-step calibration
│   ├── pcb_design_spec.md         ← PCB layout specification
│   └── technical_report.md        ← Full technical report
├── cad/
│   └── README.md                  ← CAD files description
├── images/
│   └── (add your photos here)
└── README.md
```

---

## 🆚 Novel Contributions vs Reference Paper

Based on: *Ramesh et al., "Deployment of Inductance-Based Pulsating Sensor Toward Development of Measurement Technique for Ovality in Pipe," IEEE TIM, 2016.*

| Feature | Reference Paper | This Instrument |
|---------|----------------|-----------------|
| Sensing circuit | LGO (74HC14 + discrete) | TI LDC1612 28-bit IC |
| Microcontroller | 8051 | ESP32-C3 160MHz RISC-V |
| Resolution | ~16-bit (frequency counting) | 28-bit direct inductance |
| Calibration | Polynomial curve fit | 35-point fractional LUT |
| Measurement range | 0–25mm | 0–35mm |
| Display | LCD | 1.3" OLED animated UI |
| Battery | 9V | LiPo 3.7V, USB-C, ~17hr |
| Wireless | None | BLE 5.0 (ESP32-C3) |
| Jaw mechanism | Spring return | Friction slider (zero force) |
| Enclosure | Not specified | 3D-printed PETG, 212mm |

---

## 📜 Reference

> S. Ramesh, M. P. Rajiniganth, and P. Sahoo, *"Deployment of Inductance-Based Pulsating Sensor Toward Development of Measurement Technique for Ovality in Pipe,"* IEEE Transactions on Instrumentation and Measurement, 2016.

---

## 🏛️ Acknowledgements

Developed under the guidance of faculty at **IGCAR Kalpakkam** (Indira Gandhi Centre for Atomic Research), a unit of the Department of Atomic Energy, Government of India.

---

## 📄 License

MIT License — see [LICENSE](LICENSE) for details.
