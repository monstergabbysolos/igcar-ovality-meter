# Complete Circuit Connections

## Power Chain

```
LiPo RED (+) ──┬──── TP4056 B+          (charging path, always connected)
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

TP4056 OUT+/OUT− → leave unconnected
LiPo WHITE (NTC) → tape tip, unused
```

## I2C Bus (GPIO4=SDA, GPIO5=SCL)

Both LDC1612 (0x2B) and OLED (0x3C) share the same two wires.

```
ESP32-C3 GPIO4 → 100Ω → SDA bus → LDC1612 SDA + OLED SDA
ESP32-C3 GPIO5 → 100Ω → SCL bus → LDC1612 SCL + OLED SCL
```

## GND Rail (all connect here)

- TP4056 B−
- ESP32-C3 GND
- LDC1612 GND
- OLED GND
- BTN MEASURE leg B
- BTN RESET leg B

## Full Pin Table

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

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| OLED blank | Wrong address | Try 0x3D in code |
| OLED garbled | Wrong driver | Use Adafruit SH110X library |
| LDC not found | Wrong address | Try 0x2A in code |
| Reading stuck | Coil not soldered | Solder to CH0 pads |
| Nothing in Serial | USB CDC off | Enable USB CDC On Boot |
| Brownout resets | Power rail collapsing | Check 1N5819 wiring |
