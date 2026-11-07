# mXT144U Object Configuration and Bit Mappings

This document details the configuration register state for all 34 objects decoded from the active memory read of the Microchip mXT144U touchscreen controller.

---

### Object 0: T37 Diagnostic Debug (`0x00D6`)
* **Size**: 130 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (16 bytes)**: `00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `MODE` | `0x00` | 7:0 | 0 | Default diagnostic mode (Page 0 navigation). |
| 1 | `PAGE` | `0x00` | 7:0 | 0 | Active page index is 0. |
| 2–15 | `DATA[0..13]` | `0x00` | 7:0 | 0 | Diagnostic data buffer holds zero/inactive deltas. |

---

### Object 1: T44 Message Count (`0x0158`)
* **Size**: 1 byte | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (1 byte)**: `00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `MSGCNT` | `0x00` | 7:0 | 0 | No pending messages in the FIFO queue. |

---

### Object 2: T5 Message Processor (`0x0159`)
* **Size**: 11 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (11 bytes)**: `ff 00 00 00 00 00 00 00 00 00 ff`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `REPORTID` | `0xFF` | 7:0 | 255 | Invalid / empty message marker (no unread reports). |
| 1–9 | `MESSAGE[0..8]` | `0x00` | 7:0 | 0 | Message payload area is empty. |
| 10 | `CHECKSUM` | `0xFF` | 7:0 | 255 | Checksum byte for communications verification. |

---

### Object 3: T34 Sensor Config (`0x0164`)
* **Size**: 7 bytes | **Instances**: 53 | **Report IDs**: 0
* **Raw Read (7 bytes)**: `00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0–6 | Configuration | `0x00` | 7:0 | 0 | Custom vendor sensor parameters are unassigned. |

---

### Object 4: T6 Command Processor (`0x02D7`)
* **Size**: 7 bytes | **Instances**: 1 | **Report IDs**: 1
* **Raw Read (7 bytes)**: `00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `RESET` | `0x00` | 7:0 | 0 | No reset command pending. |
| 1 | `BACKUPNV` | `0x00` | 7:0 | 0 | No backup command pending (`0x55` initiates backup). |
| 2 | `CALIBRATE` | `0x00` | 7:0 | 0 | No calibration command pending. |
| 3 | `REPORTALL` | `0x00` | 7:0 | 0 | Status reporting across objects is inactive. |
| 4 | `DEBUGCTRL` | `0x00` | 0 (`CHIP`)<br>1 (`TSCRN`)<br>2 (`DELTAS8`)<br>3 (`DELTAS`)<br>4 (`REFS`)<br>5 (`SIGNAL`) | 0<br>0<br>0<br>0<br>0<br>0 | Debug general status disabled.<br>Touchscreen debug stream disabled.<br>8-bit delta debug disabled.<br>16-bit delta debug disabled.<br>Reference stream disabled.<br>Signal stream disabled. |
| 5 | `DIAGNOSTIC` | `0x00` | 7:0 | 0 | No diagnostic command active. |
| 6 | `DEBUGCTRL2` | `0x00` | 7 (`DBGOBJMODEEN`) | 0 | Object-specific debug stream disabled. |

---

### Object 5: T68 Serial Data Command (`0x02DE`)
* **Size**: 73 bytes | **Instances**: 1 | **Report IDs**: 1
* **Raw Read (16 bytes)**: `00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>1 (`RPTEN`) | 0<br>0 | Serial data interface disabled.<br>Message reporting disabled. |
| 1–15 | Header/Payload | `0x00` | 7:0 | 0 | No data payloads queued. |

---

### Object 6: T38 User Data Storage (`0x0327`)
* **Size**: 64 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (16 bytes)**: `00 06 00 00 00 00 00 00 00 00 00 00 38 3f 5e 72`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `DATA[0]` | `0x00` | 7:0 | 0 | User-defined storage byte. |
| 1 | `DATA[1]` | `0x06` | 7:0 | 6 | User-defined storage byte. |
| 2–11 | `DATA[2..11]` | `0x00` | 7:0 | 0 | Unused user storage bytes. |
| 12 | `DATA[12]` | `0x38` | 7:0 | 56 | ASCII `'8'`. |
| 13 | `DATA[13]` | `0x3F` | 7:0 | 63 | ASCII `'?'`. |
| 14 | `DATA[14]` | `0x5E` | 7:0 | 94 | ASCII `'^'`. |
| 15 | `DATA[15]` | `0x72` | 7:0 | 114 | ASCII `'r'`. |

---

### Object 7: T71 Dynamic Configuration Container (`0x0367`)
* **Size**: 200 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (16 bytes)**: `00 00 00 00 00 ff 00 00 00 00 02 00 00 00 00 05`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0–4 | Container Data | `0x00` | 7:0 | 0 | Dynamic configuration payload offset 0–4. |
| 5 | Container Data | `0xFF` | 7:0 | 255 | Dynamic configuration payload offset 5. |
| 6–9 | Container Data | `0x00` | 7:0 | 0 | Dynamic configuration payload offset 6–9. |
| 10 | Container Data | `0x02` | 7:0 | 2 | Dynamic configuration payload offset 10. |
| 11–14 | Container Data | `0x00` | 7:0 | 0 | Dynamic configuration payload offset 11–14. |
| 15 | Container Data | `0x05` | 7:0 | 5 | Dynamic configuration payload offset 15. |

---

### Object 8: T110 Self Capacitance Tuning Parameters (`0x042F`)
* **Size**: 24 bytes | **Instances**: 4 | **Report IDs**: 0
* **Raw Read (16 bytes)**: `c9 02 41 05 51 04 91 05 c9 04 f9 04 91 04 31 05`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0–1 | `PARAMS[0]` | `0x02C9` | 15:0 | 713 | Tuning parameter 0 (LSB first: `c9 02`). |
| 2–3 | `PARAMS[1]` | `0x0541` | 15:0 | 1345 | Tuning parameter 1 (LSB first: `41 05`). |
| 4–5 | `PARAMS[2]` | `0x0451` | 15:0 | 1105 | Tuning parameter 2 (LSB first: `51 04`). |
| 6–7 | `PARAMS[3]` | `0x0591` | 15:0 | 1425 | Tuning parameter 3 (LSB first: `91 05`). |
| 8–9 | `PARAMS[4]` | `0x04C9` | 15:0 | 1225 | Tuning parameter 4 (LSB first: `c9 04`). |
| 10–11 | `PARAMS[5]` | `0x04F9` | 15:0 | 1273 | Tuning parameter 5 (LSB first: `f9 04`). |
| 12–13 | `PARAMS[6]` | `0x0491` | 15:0 | 1169 | Tuning parameter 6 (LSB first: `91 04`). |
| 14–15 | `PARAMS[7]` | `0x0531` | 15:0 | 1329 | Tuning parameter 7 (LSB first: `31 05`). |

---

### Object 9: T7 Power Configuration (`0x048F`)
* **Size**: 5 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (5 bytes)**: `05 05 14 40 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `IDLEACQINT` | `0x05` | 7:0 | 5 | Idle acquisition interval = $5\text{ ms}$. |
| 1 | `ACTVACQINT` | `0x05` | 7:0 | 5 | Active acquisition interval = $5\text{ ms}$. |
| 2 | `ACTV2IDLETO` | `0x14` | 7:0 | 20 | Timeout before entering idle = $20 \times 200\text{ ms} = 4000\text{ ms}$. |
| 3 | `CFG` | `0x40` | 0 (`IDLEPIPEEN`)<br>1 (`ACTVPIPEEN`)<br>5:2 (`ACTV2IDLETOMSB`)<br>6 (`OVFRPTSUP`)<br>7 (`INITACTV`) | 0<br>0<br>0<br>**1**<br>0 | Idle pipelining disabled.<br>Active pipelining disabled.<br>Upper timeout bits = 0.<br>**Overflow messages suppressed.**<br>Chip boots in idle mode. |
| 4 | `CFG2` | `0x00` | 0 (`IGNSTATICTCH`) | 0 | Static touches prevent transition to idle. |

---

### Object 10: T8 Acquisition Configuration (`0x0494`)
* **Size**: 15 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (15 bytes)**: `0c 00 05 05 00 00 05 00 0a 19 01 01 01 01 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CHRGTIME` | `0x0C` | 7:0 | 12 | Charge dwell time = $12 \times 41.67\text{ ns} = 500\text{ ns}$. |
| 1 | Reserved | `0x00` | 7:0 | 0 | Reserved. |
| 2 | `TCHDRIFT` | `0x05` | 7:0 | 5 | Touch drift interval = $5 \times 200\text{ ms} = 1000\text{ ms}$. |
| 3 | `DRIFTST` | `0x05` | 7:0 | 5 | Drift suspend time after release = $5 \times 200\text{ ms} = 1000\text{ ms}$. |
| 4 | `TCHAUTOCAL` | `0x00` | 7:0 | 0 | Automatic recalibration on stuck touch is disabled. |
| 5 | Reserved | `0x00` | 7:0 | 0 | Reserved. |
| 6 | `ATCHCALST` | `0x05` | 7:0 | 5 | Antitouch calibration suspend time = $5 \times 200\text{ ms} = 1000\text{ ms}$. |
| 7 | `ATCHCALSTHR` | `0x00` | 7:0 | 0 | Calibration suspend threshold disabled. |
| 8 | `ATCHFRCCALTHR`| `0x0A` | 7:0 | 10 | Palm recovery channel threshold = 10 channels. |
| 9 | `ATCHFRCCALRATIO`| `0x19`| 7:0 | 25 | Palm recovery ratio = 25 (approx. 60% antitouch ratio). |
| 10 | `MEASALLOW` | `0x01` | 0 (`MUTUALTCH`)<br>1 (`SELFTCH`)<br>3 (`SELFPROX`) | **1**<br>0<br>0 | **Mutual capacitance allowed.**<br>Self capacitance touches disallowed.<br>Self capacitance single-ended disallowed. |
| 11 | `MEASIDLEDEF` | `0x01` | 0 (`MUTUALTCH`)<br>1 (`SELFTCH`)<br>3 (`SELFPROX`) | **1**<br>0<br>0 | **Default idle mode is Mutual Capacitance.**<br>Self cap idle disallowed.<br>Low power self cap idle disallowed. |
| 12 | `MEASACTVDEF` | `0x01` | 0 (`MUTUALTCH`)<br>1 (`SELFTCH`) | **1**<br>0 | **Default active mode is Mutual Capacitance.**<br>Self cap active disallowed. |
| 13 | `REFMODE` | `0x01` | 7:0 | **1** | **Dual reference sets enabled (Dual X compatible).** |
| 14 | `CFG` | `0x00` | 0 (`DISCALRPT`)<br>1 (`SEPKEYATCHEN`)<br>2 (`SEPMCTATCHEN`)<br>7 (`CALRECTUN`) | 0<br>0<br>0<br>0 | Autocalibration reporting enabled.<br>Individual key antitouch recovery disabled.<br>Separate mutual touch calibration disabled.<br>Calibration recovery tuning output disabled. |

---

### Object 11: T15 Key Array (`0x04A3`)
* **Size**: 11 bytes | **Instances**: 1 | **Report IDs**: 1
* **Raw Read (11 bytes)**: `00 00 00 00 00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>1 (`RPTEN`)<br>4 (`TSCONTSUP`)<br>7 (`INTAKSEN`) | 0<br>0<br>0<br>0 | Key Array disabled.<br>Reporting disabled.<br>Touchscreen contact suppression disabled.<br>Internal Adjacent Key Suppression disabled. |
| 1–10 | Geometry/Gain | `0x00` | 7:0 | 0 | Geometry origin, thresholds, and hysteresis all set to 0. |

---

### Object 12: T18 Communication Config (`0x04AE`)
* **Size**: 2 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (2 bytes)**: `01 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x01` | 0 (`MODE`)<br>2 (`HISPEEDSPI`)<br>6 (`RETRIGEN`) | **1**<br>0<br>0 | **CHG Mode 1 selected (level-triggered interrupt).**<br>Standard speed SPI debug output.<br>Host retriggering disabled. |
| 1 | `COMMAND` | `0x00` | 7:0 | 0 | Default state (no manual pin-level override active). |

---

### Object 13: T24 One-Touch Gestures (`0x04B0`)
* **Size**: 19 bytes | **Instances**: 1 | **Report IDs**: 4
* **Raw Read (16 bytes)**: `00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>1 (`RPTEN`) | 0<br>0 | Single-touch gesture processing disabled.<br>Reporting disabled. |
| 1–15 | Gestures/Timeouts | `0x00` | 7:0 | 0 | Tap, flick, drag, and press parameters inactive. |

---

### Object 14: T25 Self Test (`0x04C3`)
* **Size**: 16 bytes | **Instances**: 1 | **Report IDs**: 1
* **Raw Read (16 bytes)**: `03 00 94 2f 0c 12 00 00 00 00 64 88 13 00 00 64`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x03` | 0 (`ENABLE`)<br>1 (`RPTEN`) | **1**<br>**1** | **Self-test module enabled.**<br>**Self-test error reporting enabled.** |
| 1 | `CMD` | `0x00` | 7:0 | 0 | No active execution command. |
| 2–3 | `UPSIGLIM[0]` | `0x2F94` | 15:0 | 12180 | Touch object 0 upper signal limit. |
| 4–5 | `LOSIGLIM[0]` | `0x120C` | 15:0 | 4620 | Touch object 0 lower signal limit. |
| 6–9 | Signal Limits [1] | `0x0000` | 15:0 | 0 | Reserved / object 1 limits unassigned. |
| 10 | `PINDWELLUS` | `0x64` | 7:0 | 100 | Pin dwell time = $100\ \mu\text{s}$. |
| 11–12 | `SIGRANGELIM[0]` | `0x1388` | 15:0 | 5000 | Touch object 0 signal range limit. |
| 13–14 | `SIGRANGELIM[1]` | `0x0000` | 15:0 | 0 | Reserved / object 1 limit unassigned. |
| 15 | `PINTHR` | `0x64` | 7:0 | 100 | Pin fault detection threshold. |

---

### Object 15: T27 Two-Touch Gestures (`0x04D3`)
* **Size**: 7 bytes | **Instances**: 1 | **Report IDs**: 1
* **Raw Read (7 bytes)**: `00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>1 (`RPTEN`) | 0<br>0 | Two-touch gesture engine disabled.<br>Reporting disabled. |
| 1–6 | Angles/Zoom | `0x00` | 7:0 | 0 | Rotate and pinch parameters unconfigured. |

---

### Object 16: T40 Grip Suppression (`0x04DA`)
* **Size**: 5 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (5 bytes)**: `00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>3 (`GRIPMODE`)<br>4 (`EVLEACHCYCLE`) | 0<br>0<br>0 | Grip suppression disabled.<br>Locking grip mode default.<br>Per-touchdown evaluation default. |
| 1–4 | Boundaries | `0x00` | 7:0 | 0 | Low/high X and Y edge exclusion zones set to 0. |

---

### Object 17: T42 Touch Suppression (`0x04DF`)
* **Size**: 14 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (14 bytes)**: `00 00 00 00 00 00 00 00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>2 (`SHAPEEN`)<br>3 (`DISLOBJ`)<br>4 (`DISTLOCK`)<br>5 (`SUPDISTEN`)<br>6 (`EDGESUP`) | 0<br>0<br>0<br>0<br>0<br>0 | Palm/face large object suppression disabled.<br>Shape classification disabled.<br>Large object suppression enabled if active.<br>Distance locking disabled.<br>Distance touch suppression disabled.<br>Edge suppression disabled. |
| 1–13 | Thresholds | `0x00` | 7:0 | 0 | Touch suppression limits and timeouts are 0. |

---

### Object 18: T46 CTE Configuration (`0x04ED`)
* **Size**: 12 bytes | **Instances**: 1 | **Report IDs**: 1
* **Raw Read (12 bytes)**: `00 00 10 10 00 00 01 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 5:4 (`INTMODE`) | 0 | Normal integration mode selected. |
| 1 | `MODE` | `0x00` | 7:0 | 0 | Matrix layout mode is 12 X by 12 Y. |
| 2 | `IDLESYNCSPERX`| `0x10` | 7:0 | 16 | Idle analog conversions per X line = 16 ADCs. |
| 3 | `ACTVSYNCSPERX`| `0x10` | 7:0 | 16 | Active analog conversions per X line = 16 ADCs. |
| 4–5 | Reserved | `0x00` | 7:0 | 0 | Reserved. |
| 6 | `PULSESPERADC`| `0x01` | 1:0 | 1 | 2 pulses per ADC conversion (encoded as $N-1$). |
| 7–11 | `INRUSHCFG` | `0x00` | 2:0 (`YRESISTOR`) | 0 | Inrush series resistor = $50\ \Omega$. |

---

### Object 19: T51 Custom OEM???? / Message Processor Gate (`0x04F9`)
* **Size**: 13 bytes (`0x04F9`–`0x0505`) | **Instances**: 1 | **Report IDs**: 1 (Report ID `4`)
* **Raw Read (13 bytes, Factory NVM)**: `00 00 00 00 00 00 00 00 08 00 00 00 00`

| Byte | Address | Field | Factory Value | Bit(s) | State | Mapping / Description |
| :--- | :---: | :--- | :---: | :---: | :---: | :--- |
| 0 | `0x04F9` | `CTRL` | `0x00` | 0 (`ENABLE`) | 0 | Control byte (note: Offset 8 remains active even when `CTRL = 0x00`). |
| 1–7 | `0x04FA`–`0x0500` | `CFG[1..7]` | `0x00` | 7:0 | 0 | Unused / reserved parameters (`0x00`). |
| 8 | `0x0501` | `MSG_GATE` | `0x08` | 3<br>7 (`T5_FIFO_EN`) | **1**<br>**0** | **Factory NVM default (`0x08`) masks `T44` (`0x0158`) and `T5` (`0x0159`) FIFO readout (`00 ff`).**<br>**Setting Bit 7 (`0x80`, e.g. `0x88` or `0xFF`) unlocks standard `T44`/`T5` FIFO message reporting.** |
| 9–12 | `0x0502`–`0x0505` | `CFG[9..12]` | `0x00` | 7:0 | 0 | Unused / reserved parameters (`0x00`). |

#### Empirical Behavior & Hardware Verification (`mXT144U` Family `0xA6`, Variant `0x0A`, Version `0x11`, Build `0xAA`)
1. **Factory NVM State (`0x0501 = 0x08`)**:
   * Out of reset (`T6 RESET`), all 13 bytes of `T51` (`0x04F9`–`0x0505`) are `0x00` except **Byte 8 (`0x0501`)**, which is programmed in factory NVM to `0x08` (Bit 3 = 1, Bit 7 = 0).
   * While `0x0501` has Bit 7 cleared (`0x00`–`0x7F`), the controller still asserts $\overline{\text{CHG}}$ (`P1.00`) active-low (`0`) when `T6` status events or `T100` touch events occur, and updates the internal `T34` message state table (`0x0164`–`0x02D6`) and `T37` diagnostic memory (`0x00D6`), **but reading `T44` (`0x0158`) and `T5` (`0x0159`–`0x0163`) over I2C always returns `00 ff 00 00 00 00 00 00 00 00 00`** (`COUNT = 0`, `REPORTID = 0xFF`).
2. **Bit-by-Bit Sweep on `0x0501` (`T51` Offset 8)**:
   * Testing each individual bit (`0x01`, `0x02`, `0x04`, `0x08`, `0x10`, `0x20`, `0x40`, `0x80`) at `0x0501` followed by `T6 CALIBRATE` (`0x02D9 = 0x01`) proved that **Bit 7 (`0x80`)** is the exact gate controlling `T44`/`T5` output:
     * `0x0501` = `0x01`, `0x02`, `0x04`, `0x08`, `0x10`, `0x20`, or `0x40` $\rightarrow$ `T44` + `T5` (`0x0158`) returns `00 ff 00 00 00 00 00 00 00 00 00`.
     * `0x0501` = `0x80` (or `0x88` / `0xFF`) $\rightarrow$ `T44` + `T5` (`0x0158`) immediately shifts out queued messages (`02 01 90 9c aa 4a 00 00 00 00 00` for `T6` status + config checksum `0x4AAA9C`, `0x2C` for `T100` screen status, and `0x2E`–`0x32` for `T100` finger touch coordinates).
3. **Driver Initialization Requirement**:
   * In [`input_maxtouch.c`](file:///c:/Users/Eric/Documents/GitHub/zmk-driver-maxtouch/drivers/input/input_maxtouch.c), [`mxt_load_object_table`](file:///c:/Users/Eric/Documents/GitHub/zmk-driver-maxtouch/drivers/input/input_maxtouch.c#L400-L402) records the base address of `T51` (`data->t51_custom_address`), and [`mxt_load_config`](file:///c:/Users/Eric/Documents/GitHub/zmk-driver-maxtouch/drivers/input/input_maxtouch.c#L558-L573) writes `0xFF` to `data->t51_custom_address + 8` (`0x0501`) before issuing `T6 CALIBRATE` (`0x02D9 = 0x01`).

---

### Object 20: T56 Shieldless Integration (`0x0506`)
* **Size**: 16 bytes | **Instances**: 1 | **Report IDs**: 1
* **Raw Read (16 bytes)**: `00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>1 (`RPTEN`) | 0<br>0 | Shieldless tracking disabled.<br>Reporting disabled. |
| 1 | Reserved | `0x00` | 7:0 | 0 | Reserved. |
| 2 | `OPTINT` | `0x00` | 0 (`OPTINTEN`) | 0 | Optimal integration window disabled. |
| 3–15 | `INTDELAY[]` | `0x00` | 7:0 | 0 | Line delay compensations unconfigured. |

---

### Object 21: T61 Timer Configuration (`0x0516`)
* **Size**: 5 bytes | **Instances**: 6 | **Report IDs**: 1
* **Raw Read (5 bytes)**: `00 00 00 64 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>1 (`RPTEN`) | 0<br>0 | Timer instance 0 disabled.<br>Reporting disabled. |
| 1 | `CMD` | `0x00` | 7:0 | 0 | No active timer command. |
| 2 | `MODE` | `0x00` | 0 (`MODE`) | 0 | Single-shot trigger mode selected. |
| 3–4 | `PERIOD` | `0x0064` | 15:0 | 100 | Timer expiration period = $100\text{ ms}$. |

---

### Object 22: T65 Lens Bending Correction (`0x0534`)
* **Size**: 23 bytes | **Instances**: 3 | **Report IDs**: 1
* **Raw Read (16 bytes)**: `00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>1 (`RPTEN`)<br>3 (`DISPRESS`)<br>4 (`DISRELEASE`)<br>7 (`DISHIST`) | 0<br>0<br>0<br>0<br>0 | Lens bending compensation disabled.<br>Force reporting disabled.<br>Press events enabled.<br>Release events enabled.<br>Inter-line history enabled. |
| 1–15 | Thresholds | `0x00` | 7:0 | 0 | Gradient, filter, and force limits unconfigured. |

---

### Object 23: T70 Dynamic Configuration Controller (`0x0579`)
* **Size**: 10 bytes | **Instances**: 20 | **Report IDs**: 1
* **Raw Read (10 bytes)**: `01 0d 00 07 00 00 00 00 00 01`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x01` | 0 (`ENABLE`)<br>1 (`RPTEN`)<br>2 (`ALTSRC`)<br>4:3 (`FRCCMD`) | **1**<br>0<br>0<br>0 | **Controller instance 0 enabled.**<br>Reporting disabled.<br>Source configuration read from T71 container.<br>No forced command edge. |
| 1–2 | `EVENT` | `0x000D` | 11:0 (`ENUMID`)<br>15 (`HOSTDEF`) | 13<br>0 | **Triggers on Event 13 (Timer 0 Elapsed).**<br>Device-defined enumeration mapping. |
| 3 | `OBJTYPE` | `0x07` | 7:0 | 7 | Target object type: **T7 (Power Configuration)**. |
| 4 | Reserved | `0x00` | 7:0 | 0 | Reserved. |
| 5 | `OBJINST` | `0x00` | 7:0 | 0 | Target object instance: 0. |
| 6 | `DSTOFFSET` | `0x00` | 7:0 | 0 | Target register offset: Byte 0 (`IDLEACQINT`). |
| 7–8 | `SRCOFFSET` | `0x0000` | 15:0 | 0 | Offset within T71 container: Byte 0. |
| 9 | `LENGTH` | `0x01` | 7:0 | 1 | Transfers 2 bytes ($1 + 1$). |

---

### Object 24: T72 Noise Suppression (`0x0641`)
* **Size**: 89 bytes | **Instances**: 1 | **Report IDs**: 1
* **Raw Read (16 bytes)**: `01 00 00 01 00 ff 01 05 00 10 0f 0f 05 04 21 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x01` | 0 (`ENABLE`)<br>1 (`RPTEN`)<br>2 (`RPTACQ`)<br>3 (`RPTSTATE`)<br>4 (`RPTNOISELVL`)<br>5 (`RPTNLTHR`)<br>6 (`RPTDUALX`) | **1**<br>0<br>0<br>0<br>0<br>0<br>0 | **Noise suppression algorithm enabled.**<br>Reporting disabled.<br>Acquisition change reporting disabled.<br>State transition reporting disabled.<br>Noise level change reporting disabled.<br>Noise threshold reporting disabled.<br>Dual X change reporting disabled. |
| 1 | `CALCFG1` | `0x00` | 0 (`NOISEON`)<br>1 (`VNOISEON`) | 0<br>0 | Calibration software triggers inactive. |
| 2 | `CFG1` | `0x00` | 0 (`NOISEON`)<br>1 (`VNOISEON`) | 0<br>0 | Runtime noise flags inactive. |
| 3 | `CFG2` | `0x01` | 0 (`NOBLKTCH`)<br>7 (`NOTCHNOISEON`) | **1**<br>0 | **Do not block touches during frequency hops.**<br>Noise demotion during continuous touch disabled. |
| 4 | Reserved | `0x00` | 7:0 | 0 | Reserved. |
| 5 | `HOPCNT` | `0xFF` | 7:0 | 255 | Hop count limit = 255 hops (continuous hopping). |
| 6 | `HOPCNTPER` | `0x01` | 7:0 | 1 | Hop period = $1 \times 100\text{ cycles} = 100\text{ cycles}$. |
| 7 | `HOPEVALTO` | `0x05` | 7:0 | 5 | Hop evaluation timeout = $5 \times 10 = 50\text{ cycles}$. |
| 8 | `HOPST` | `0x00` | 7:0 | 0 | Hop suspend time disabled (immediate hop allowed). |
| 9 | `NLGAINDUALX`| `0x10` | 7:0 | 16 | Noise line measurement gain for Dual X mode. |
| 10 | `MINNLTHR` | `0x0F` | 7:0 | 15 | Minimum noise lines threshold = 15 deltas. |
| 11 | `INCNLTHR` | `0x0F` | 7:0 | 15 | Noise threshold increment step = 15 deltas. |
| 12 | `FALLNLTHR` | `0x05` | 7:0 | 5 | Falling noise lines threshold = 5 deltas. |
| 13 | `NLTHRMARGIN`| `0x04` | 7:0 | 4 | Noise threshold margin = 4 deltas. |
| 14 | `MINTHRADJ` | `0x21` | 3:0 (`MINTHRFRAC`)<br>7:4 (`MINTHRFACTOR`) | 1<br>2 | Minimum touch threshold fractional part = $1/16$.<br>Minimum touch threshold factor = 2. |
| 15 | `NLTHRLIMIT` | `0x00` | 7:0 | 0 | Minimum touch threshold unconstrained. |

---

### Object 25: T78 Glove Detection (`0x069A`)
* **Size**: 12 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (12 bytes)**: `00 00 00 00 00 00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>2 (`EDGESUP`)<br>3 (`LOCKEDGESUP`)<br>5 (`DBGDISCRIMEN`)<br>7 (`GLOVERPTEN`) | 0<br>0<br>0<br>0<br>0 | Glove detection disabled.<br>Edge suppression disabled.<br>Locked edge suppression disabled.<br>Debug discriminant disabled.<br>Glove touches report as finger events. |
| 1–11 | Thresholds | `0x00` | 7:0 | 0 | Glove contact area and discriminant thresholds are 0. |

---

### Object 26: T80 Retransmission Compensation (`0x06A6`)
* **Size**: 14 bytes | **Instances**: 1 | **Report IDs**: 1
* **Raw Read (14 bytes)**: `00 00 00 00 00 00 00 00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>1 (`RPTEN`)<br>2 (`DISGAINLIMIT`)<br>3 (`COMPENHEN`)<br>4 (`DISINVTCHREM`)<br>5 (`DISATCHCHECK`) | 0<br>0<br>0<br>0<br>0<br>0 | Retransmission compensation disabled.<br>Reporting disabled.<br>Gain limit based on target delta enabled.<br>Compensation enhancement disabled.<br>Invalid moisture node removal active.<br>Antitouch checking active. |
| 1–13 | Thresholds | `0x00` | 7:0 | 0 | Target deltas, moisture thresholds, and filters are 0. |

---

### Object 27: T100 Multiple Touch Touchscreen (`0x06B4`)
* **Size**: 62 bytes | **Instances**: 1 | **Report IDs**: 8
* **Raw Read (16 bytes)**: `8f f4 00 01 07 00 05 88 00 0a 2b 1e 1e 7f 07 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x8F` | 0 (`ENABLE`)<br>1 (`RPTEN`)<br>2 (`DISSCRMSG0`)<br>3 (Reserved)<br>4:6 (Reserved)<br>7 (`SCANEN`) | **1**<br>**1**<br>**1**<br>**1**<br>0<br>**1** | **Touchscreen controller enabled.**<br>**Message reporting enabled.**<br>**Screen status message 0 disabled.**<br>Reserved bit set to 1.<br>Reserved bits cleared to 0.<br>**Close scanning enabled (runs every $200\text{ ms}$).** |
| 1 | `CFG1` | `0xF4` | 0 (`RPTEACHCYCLE`)<br>2 (Reserved)<br>4 (`DISLOCK`)<br>5 (`SWITCHXY`)<br>6 (`INVERTX`)<br>7 (`INVERTY`) | 0<br>**1**<br>**1**<br>**1**<br>**1**<br>**1** | Report on event transitions only.<br>Reserved bit set to 1.<br>**Coordinate locking inside clipping boundary disabled.**<br>**X and Y axes swapped.**<br>**X coordinates inverted.**<br>**Y coordinates inverted.** |
| 2 | `SCRAUX` | `0x00` | 0 (`NUMRPTTCH`)<br>1 (`TCHAREA`)<br>2 (`ATCHAREA`)<br>3 (`INTTHRAREA`) | 0<br>0<br>0<br>0 | Auxiliary screen reports disabled. |
| 3 | `TCHAUX` | `0x01` | 0 (`VECT`)<br>1 (`AMPL`)<br>2 (`AREA`)<br>3 (`HW`)<br>4 (`PEAK`)<br>5 (`AREAHW`) | **1**<br>0<br>0<br>0<br>0<br>0 | **Touch vector reported in auxiliary bytes.**<br>Amplitude reporting disabled.<br>Node area reporting disabled.<br>Height/width reporting disabled.<br>Peak delta reporting disabled.<br>Combined area/geometry reporting disabled. |
| 4 | `TCHEVENTCFG`| `0x07` | 0 (Reserved)<br>1 (`DISDOWN`)<br>2 (`DISSUP`)<br>3 (`DISUNSUP`)<br>4 (`DISUP`)<br>5 (`DISMOVE`) | **1**<br>**1**<br>**1**<br>0<br>0<br>0 | Reserved bit set to 1.<br>**Touch DOWN event message disabled.**<br>**Touch SUP (suppressed) message disabled.**<br>Touch UNSUP message enabled.<br>Touch UP (release) message enabled.<br>Touch MOVE message enabled. |
| 5 | `AKSCFG` | `0x00` | 7:0 | 0 | Not assigned to any Adjacent Key Suppression group. |
| 6 | `NUMTCH` | `0x05` | 7:0 | 5 | Tracks up to 5 concurrent touches. |
| 7 | `XYCFG` | `0x88` | 3 (`XSPAN`)<br>7 (`YSPAN`) | **1**<br>**1** | **Extra X electrode enabled (edge compensation).**<br>**Extra Y electrode enabled (edge compensation).** |
| 8 | `XORIGIN` | `0x00` | 7:0 | 0 | Touch matrix origin begins at electrode X0. |
| 9 | `XSIZE` | `0x0A` | 7:0 | 10 | Touch matrix spans 10 X electrodes. |
| 10 | `XPITCH` | `0x2B` | 7:0 | 43 | Electrode pitch = $43 \times 0.1\text{ mm} = 4.3\text{ mm}$. |
| 11 | `XLOCLIP` | `0x1E` | 7:0 | 30 | Low X clipping boundary = 30 counts. |
| 12 | `XHICLIP` | `0x1E` | 7:0 | 30 | High X clipping boundary = 30 counts. |
| 13–14 | `XRANGE` | `0x077F`| 15:0 | 1919 | Full output resolution along X axis is 1920 pixels (0 to 1919). |
| 15 | `XEDGECFG` | `0x00` | 5:0 (`GRADIENT`) | 0 | Edge correction gradient disabled. |

---

### Object 28: T104 Auxiliary Touch Screen (`0x06F2`)
* **Size**: 11 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (11 bytes)**: `00 00 00 00 00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`) | 0 | Auxiliary self-capacitance touch sensing disabled. |
| 1–10 | Gain/Thresholds| `0x00` | 7:0 | 0 | Gains, touch thresholds, and hysteresis are 0. |

---

### Object 29: T108 Self Capacitance Noise Suppression (`0x06FD`)
* **Size**: 75 bytes | **Instances**: 1 | **Report IDs**: 1
* **Raw Read (16 bytes)**: `00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>1 (`RPTEN`) | 0<br>0 | Self capacitance noise suppression disabled.<br>Reporting disabled. |
| 1–15 | Frequency Presets | `0x00` | 7:0 | 0 | Frequency presets and filter thresholds are 0. |

---

### Object 30: T109 Self Capacitance Global Configuration (`0x0748`)
* **Size**: 9 bytes | **Instances**: 1 | **Report IDs**: 1
* **Raw Read (9 bytes)**: `00 00 05 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 1 (`RPTEN`)<br>2 (`SNGLENDEN`)<br>6 (`DISMEASX`)<br>7 (`DISMEASY`) | 0<br>0<br>0<br>0 | Reporting disabled.<br>Single-ended measurement disabled.<br>X self-cap enabled.<br>Y self-cap enabled. |
| 1 | Reserved | `0x00` | 7:0 | 0 | Reserved. |
| 2 | `CMDONRESET` | `0x05` | 7:0 | 5 | **Apply tuning parameters from configuration RAM on reset.** |
| 3 | `CMD` | `0x00` | 7:0 | 0 | No active runtime command pending. |
| 4–8 | Tuning / Config | `0x00` | 7:0 | 0 | Extra tuning iterations disabled. |

---

### Object 31: T111 Self Capacitance Configuration (`0x0751`)
* **Size**: 30 bytes | **Instances**: 2 | **Report IDs**: 0
* **Raw Read (16 bytes)**: `00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 3 (`LFCOMPEN`)<br>7 (`DISDCCORR`) | 0<br>0 | Low frequency compensation disabled.<br>DC correction enabled. |
| 1–15 | Timing/Drift | `0x00` | 7:0 | 0 | Integration timings, drift intervals, and filters are 0. |

---

### Object 32: T113 Self Capacitance Measurement Configuration (`0x078D`)
* **Size**: 3 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (3 bytes)**: `00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`)<br>4 (`ALTAXISEN`) | 0<br>0 | Object disabled.<br>Primary axis selected for measurement. |
| 1 | `GAINX` | `0x00` | 7:0 | 0 | X axis measurement gain is 0. |
| 2 | `GAINY` | `0x00` | 7:0 | 0 | Y axis measurement gain is 0. |

---

### Object 33: T121 Sensor Correction (`0x0790`)
* **Size**: 3 bytes | **Instances**: 1 | **Report IDs**: 0
* **Raw Read (3 bytes)**: `00 00 00`

| Byte | Field | Value | Bit(s) | State | Mapping / Description |
| :--- | :--- | :---: | :---: | :---: | :--- |
| 0 | `CTRL` | `0x00` | 0 (`ENABLE`) | 0 | Sensor correction disabled. |
| 1 | `CFG1` | `0x00` | 0 (`STAGXYEDGEEN`)<br>1 (`STAGXODDLOS`) | 0<br>0 | Staggered X edge correction disabled.<br>Offset pattern even X line default. |
| 2 | `YEDGESF` | `0x00` | 3:0 (`LO`)<br>7:4 (`HI`) | 0<br>0 | Y low / high correction scaling factors default. |