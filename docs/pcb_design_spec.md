# PCB Design Specification

## Board: 80mm × 34mm, 2-layer, 1.6mm FR4

### Top Layer — Component Side (faces OLED)

| Component | Size | Center (X, Y) | Notes |
|-----------|------|---------------|-------|
| LDC1612 Grove module | 50×20mm | (27, 17) | CH0 pads face LEFT (toward coil) |
| ESP32-C3 SuperMini | 23×18mm | (66.5, 17) | USB-C faces RIGHT edge |
| Battery JST-PH | 2-pin | (8, 28) | Wire down to battery |
| OLED 4-pin header | 2.54mm | (60, 28) | Wires up to OLED |

### Bottom Layer — Faces Battery

| Component | Size | Center (X, Y) | Notes |
|-----------|------|---------------|-------|
| TP4056 module | 26×17mm | (15, 16.5) | USB-C faces LEFT edge |
| Buttons | 6×6mm | (49, 17), (59, 17) | MEASURE + RESET |
| Mounting holes | M2 | (4,4), (76,4), (4,30), (76,30) | 4mm from each corner |

### Height Stack

```
OLED module         5mm
── standoff ────────3mm
Top components      8mm (LDC1612 tallest)
── PCB ─────────────1.6mm
Bottom components   4mm (TP4056 tallest)
── standoff ────────2mm
LiPo battery        7mm
────────────────────────
Total:              30.6mm  (within 40mm budget)
```

### Net List (20 connections)

```
LiPo RED    → Slide switch → 1N5819 → ESP32 3V3 + LDC VCC + OLED VCC
LiPo BLACK  → TP4056 B− + GND plane
TP4056 B+   → LiPo RED (charging path)
LDC SDA     → ESP32 GPIO4 (100Ω series)
LDC SCL     → ESP32 GPIO5 (100Ω series)
OLED SDA    → ESP32 GPIO4 (shared)
OLED SCL    → ESP32 GPIO5 (shared)
BTN MEASURE → ESP32 GPIO2
BTN RESET   → ESP32 GPIO3
```

### Fabrication

Send Gerber files to JLCPCB or PCBWay.
- 2-layer, FR4, 1.6mm, HASL finish
- Estimated cost: ₹400–700 for 5 boards
