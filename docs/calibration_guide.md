# Calibration Guide

## Overview

The instrument uses a **fractional piecewise-linear calibration** system. You can record at any decimal jaw position — c0, c2.5, c17.3, etc. All 80 points are sorted automatically before use.

## Quick Steps

1. Set `CALIBRATION_DONE = false` in firmware → Upload
2. Open Serial Monitor at **115200 baud**
3. Set jaw to exact position using digital caliper
4. Type `c<position>` → Enter (e.g. `c0`, `c2.5`, `c35`)
5. Repeat for all positions
6. Type `p` → copy the 3 printed lines into firmware
7. Set `CALIBRATION_DONE = true` → Re-upload

## Commands

| Command | Action |
|---------|--------|
| `c0` | Record at 0mm |
| `c2.5` | Record at 2.5mm |
| `c17.3` | Record at 17.3mm |
| `p` | Print sorted arrays — copy directly into firmware |
| `r` | Clear all points |
| `t` | Show raw value only |

## Tips for Best Accuracy

1. **Hand position** — keep hand away from the coil tube when recording. Hand interference shifts readings by ~20,000 counts at 17mm.
2. **Consistent grip** — hold the instrument body the same way during all measurements.
3. **More points = better accuracy** — especially in the 20–35mm zone where sensitivity is lower.
4. **Fix violations** — check for monotonicity (raw values should strictly decrease as mm increases). Re-measure any point that shows a violation.

## Current Calibration (35 points)

```cpp
int cal_count = 35;
float CAL_POS[MAX_CAL_POINTS] = {
  0.00,1.00,2.00,3.00,4.00,5.00,6.00,7.00,8.00,9.00,
  10.00,11.00,12.00,13.00,14.00,15.00,16.00,17.00,18.00,19.00,
  20.00,21.00,22.00,23.00,24.00,25.00,26.00,27.00,28.00,29.00,
  30.00,31.00,32.00,33.00,34.00
};
uint32_t CAL_RAW[MAX_CAL_POINTS] = {
  22766723,22722796,22669910,22590279,22546193,22504932,22443128,
  22397782,22357397,22324447,22277149,22230950,22202319,22174191,
  22147629,22126156,22098540,22077995,22058101,22040191,22025032,
  22005980,21987921,21970523,21958033,21942469,21930907,21920389,
  21910436,21903332,21896228,21884697,21880335,21876291,21874731
};
```

## Sensitivity by Zone

| Zone | Sensitivity | Effective Resolution |
|------|------------|---------------------|
| 0–10mm | ~50,000 counts/mm | ~0.004mm |
| 10–20mm | ~24,000 counts/mm | ~0.008mm |
| 20–28mm | ~12,000 counts/mm | ~0.017mm |
| 28–34mm | ~6,000 counts/mm | ~0.033mm |
