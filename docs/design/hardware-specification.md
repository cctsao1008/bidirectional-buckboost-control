# CBB02405D / CBB024D V1.2 Hardware Specification

## Purpose

This document consolidates the hardware specification of the CBB02405D / CBB024D bidirectional four-switch buck-boost board for use as the hardware baseline of the `bidirectional-buckboost-control` project.

Primary source:
- `CBB024D-1-V1.2.pdf`, dated 2022-03-10

Cross-checked against:
- `CBB02405D Bidirectional BUCK-BOOST Hardware Design Report.pdf`, Rev1.0, dated 2021-04-10
- `CBB024D User Manual.pdf`, Rev1.0, dated 2021-04-10
- `CBB02405D Bidirectional BUCK-BOOST Software Design Report.pdf`, Rev1.0, dated 2021-04-10

Where the 2021 conceptual documentation conflicts with the later V1.2 schematic, the V1.2 schematic and MCU net mapping are treated as authoritative for implementation.

---

## 1. Converter Topology

The board is a non-isolated, synchronous, four-switch bidirectional buck-boost converter using two half bridges and one main inductor.

### 1.1 Physical switch arrangement

```text
                         L1 = 22 uH
                  HS-1 ───────── HS-2
                    │               │
                Q1 high          Q2 high
VIN+ ───────────────┤               ├────────────── VOUT+
                    │               │
                Q4 low           Q3 low
                    │               │
                   GND─────────────GND
```

For the V1.2 hardware:

| Half bridge | High-side MOSFET | Low-side MOSFET | PWM signals |
|---|---|---|---|
| Left bridge | Q1 | Q4 | `PWM1H`, `PWM1L` |
| Right bridge | Q2 | Q3 | `PWM2H`, `PWM2L` |

This mapping is the one to use in firmware and documentation.

### 1.2 Documentation inconsistency

The 2021 hardware design report uses a conceptual labeling that groups Q1/Q2 and Q3/Q4 as the synchronous converter pairs. The 2022 V1.2 schematic and MCU signal routing establish the actual physical half-bridge pairs as:

```text
Q1 / Q4 = left half bridge
Q2 / Q3 = right half bridge
```

Firmware must follow the V1.2 schematic/net mapping.

---

## 2. Board-Level Electrical Ratings

| Item | Specification |
|---|---|
| Topology | Four-switch synchronous bidirectional buck-boost |
| Input voltage | 12–48 VDC |
| Output voltage | 5–48 VDC |
| Rated output | 24 V / 5 A |
| Suggested maximum output power | 200 W |
| Output current range | 0–5 A |
| Output current limit | 5 A |
| Output-voltage ripple | ≤ 1% of output voltage, peak-to-peak |
| Switching frequency | 200 kHz |
| Control frequency | 200 kHz |
| Cooling | Natural convection |
| Operating temperature | −30 to +40 °C |
| Storage temperature | −40 to +80 °C |
| Board size | 100 × 90 × 20 mm |
| Communication | Reserved UART interface |
| Programming/debug | SWD |

### 2.1 Vendor protection functions

| Protection | Vendor specification |
|---|---|
| Short-circuit protection | Supported, automatic recovery |
| Output over-voltage protection | Supported, >48 V, automatic recovery |
| Input under-voltage protection | Supported, <12 V, automatic recovery |
| Input over-voltage protection | Supported, >48 V, automatic recovery |
| Output over-current protection | Supported, automatic recovery |

These are vendor-system behavior specifications. The independent firmware must define and validate its own protection behavior.

---

## 3. Main Power Stage

### 3.1 MOSFETs

| Reference | Part |
|---|---|
| Q1 | BSC070N10NS3G |
| Q2 | BSC070N10NS3G |
| Q3 | BSC070N10NS3G |
| Q4 | BSC070N10NS3G |

Each MOSFET gate has a 10 kΩ gate-source pull resistor.

### 3.2 Main inductor

| Item | Value |
|---|---|
| Reference | L1 |
| Inductance | 22 µH |
| Tolerance | ±20% |

### 3.3 Port capacitance

#### Input port

| Component | Value |
|---|---|
| C1 | 220 µF / 63 V |
| C2 | 220 µF / 63 V |
| C5 | 10 µF / 50 V |
| C6 | 10 µF / 50 V |
| R3 | 10 kΩ |
| D1 | SS210 |

Nominal bulk capacitance:

```text
CIN,bulk = 440 µF
```

#### Output port

| Component | Value |
|---|---|
| C3 | 220 µF / 63 V |
| C4 | 220 µF / 63 V |
| C7 | 10 µF / 50 V |
| C8 | 10 µF / 50 V |
| R4 | 10 kΩ |
| D2 | SS210 |

Nominal bulk capacitance:

```text
COUT,bulk = 440 µF
```

---

## 4. Gate-Drive Architecture

| Item | Specification |
|---|---|
| Driver | Si8233BD-D-IS |
| Quantity | 2 |
| Gate resistors | 10 Ω |
| DT programming resistor | 3.3 kΩ to 5 V |
| Bootstrap diode | SS210 |
| Bootstrap capacitor | 100 nF |
| Driver input logic supply | 5 V |
| Driver `DISABLE` | Permanently tied low in schematic |

### 4.1 Driver mapping

| PWM signal | MOSFET |
|---|---|
| `PWM1H` | Q1 |
| `PWM1L` | Q4 |
| `PWM2H` | Q2 |
| `PWM2L` | Q3 |

### 4.2 Safety implication

The gate-driver `DISABLE` input is not routed to an MCU-controlled shutdown signal. Safe shutdown must therefore be implemented through the STM32F334/HRTIM output-control path and any usable hardware fault mechanisms available in the MCU/peripheral routing.

This must be validated before closed-loop power testing.

---

## 5. Auxiliary Power

The auxiliary power supply can draw power from either power port:

```text
VIN+  ── D4 ──┐
               ├── auxiliary supply input
VOUT+ ── D3 ──┘
```

### 5.1 Power rails

| Stage | Device | Nominal function |
|---|---|---|
| First stage | XL7005A | ~10 V auxiliary/gate-driver rail |
| Second stage | AMS1117-5 | 5 V |
| Third stage | AMS1117-3.3 | 3.3 V |
| Analog rail filtering | L3/L4 + capacitors | Filtered analog supply/return |

### 5.2 `P12V` naming caveat

The V1.2 schematic labels the first auxiliary rail as `P12V`, but the XL7005A feedback network is:

```text
R17 = 33 kΩ
R19 = 4.7 kΩ
VFB  = 1.25 V
```

Derived nominal rail:

```text
VOUT ≈ 1.25 × (1 + 33 / 4.7)
     ≈ 10.0 V
```

The hardware design report also describes this rail as 10 V.

Therefore:

> `P12V` is a legacy schematic net name; the documented and resistor-derived nominal rail is approximately 10 V.

---

## 6. Voltage Sensing

The board measures both input and output port voltages using GS8552-SR signal-conditioning stages.

Nominal resistor ratio:

```text
RIN = 68 kΩ
RF  = 3.3 kΩ
KV  = 3.3 / 68
    ≈ 0.04853 V/V
```

Nominal relationship:

```text
VADC ≈ KV × VPORT
```

Examples:

```text
VPORT = 48 V  →  VADC ≈ 2.33 V
VADC  = 3.3 V →  VPORT ≈ 68.0 V
```

The ~68 V value is a theoretical signal-conditioning full-scale value, not an operating-voltage rating.

### 6.1 ADC output filter

```text
R = 100 Ω
C = 330 pF
fc ≈ 1 / (2πRC) ≈ 4.82 MHz
```

The complete analog path must be characterized as part of the measurement plant.

---

## 7. Current Sensing

The board already includes bidirectional input- and output-port current sensing.

### 7.1 Shunts

| Signal | Shunt | Value |
|---|---|---|
| `Iin` | R7 | 1 mΩ, ±1%, 2512 |
| `Iout` | R8 | 1 mΩ, ±1%, 2512 |

### 7.2 Current amplifier

Nominal resistor values:

```text
RIN = 100 Ω
RF  = 15 kΩ
```

Amplifier gain:

```text
GI = 15 kΩ / 100 Ω
   = 150 V/V
```

With a 1 mΩ shunt:

```text
Current sensitivity = 150 × 0.001
                    = 0.150 V/A
```

The circuit uses a nominal 1.65 V offset for bidirectional current measurement:

```text
VADC,I = 1.65 V + 0.150 × I
I      = (VADC,I - 1.65) / 0.150
```

Examples:

```text
I = +5 A → VADC ≈ 2.40 V
I =  0 A → VADC ≈ 1.65 V
I = -5 A → VADC ≈ 0.90 V
```

Actual firmware polarity must be verified against the physical current direction and calibration data.

### 7.3 Current-sense output filter

```text
R = 100 Ω
C = 330 pF
fc ≈ 4.82 MHz
```

### 7.4 No direct inductor-current ADC

The MCU directly samples:

```text
Vin
Iin
Vout
Iout
VADJ
```

There is no dedicated ADC channel for the main inductor current `iL`.

The `CNT1/CNT2` path is an inductor-current measurement access point described by the vendor as a normally shorted series point for external test measurement.

Project constraint:

> No additional current sensor will be added.

If a controller requires inductor current, it must be reconstructed from existing signals and converter state:

```text
Vin
Iin
Vout
Iout
D1
D2
switching/operating state
        ↓
iL estimator
        ↓
iL_hat
```

---

## 8. 1.65 V Current-Sense Reference

| Item | Value |
|---|---|
| Divider | 3.3 kΩ / 3.3 kΩ |
| Nominal reference | 1.65 V |
| Buffer | GS8552-SR |
| Filter capacitor | 470 nF |
| Net name | `P1V65` |

Zero-current calibration must use measured offset rather than assuming exactly 1.650 V.

---

## 9. Analog Reference Potentiometer

| Item | Value |
|---|---|
| Potentiometer | 50 kΩ (`3296W-1-503LF`) |
| Series resistor from 3.3 V | 10 kΩ |
| ADC filter resistor | 3.3 kΩ |
| ADC filter capacitor | 470 nF |
| ADC net | `ADC_VADJ` |

Approximate unloaded maximum:

```text
VADJ,max ≈ 3.3 × 50 / (50 + 10)
         ≈ 2.75 V
```

---

## 10. MCU and Peripheral Mapping

### 10.1 MCU

| Item | Specification |
|---|---|
| MCU | STM32F334C8T6 |
| Vendor HCLK configuration | 64 MHz |
| Vendor HRTIM clock | 128 MHz |
| Vendor ADC clock | 64 MHz |
| Vendor ADC resolution | 12 bit |
| Vendor switching/control rate | 200 kHz |

### 10.2 PWM pins

| MCU pin | Signal | Physical MOSFET |
|---|---|---|
| PA8 | `PWM1H` | Q1, left high-side |
| PA9 | `PWM1L` | Q4, left low-side |
| PA10 | `PWM2H` | Q2, right high-side |
| PA11 | `PWM2L` | Q3, right low-side |

### 10.3 ADC pins

| MCU pin | Signal | Measurement |
|---|---|---|
| PA0 | `ADC_Vin` | Input voltage |
| PA1 | `ADC_Iin` | Input current |
| PA2 | `ADC_Vout` | Output voltage |
| PA3 | `ADC_Iout` | Output current |
| PA4 | `ADC_VADJ` | Local reference potentiometer |

Vendor firmware configures ADC1 channels 1–4 for Vin/Iin/Vout/Iout with DMA and ADC2 for the potentiometer.

### 10.4 UART

| MCU pin | Signal |
|---|---|
| PB6 | `USART1_TX` |
| PB7 | `USART1_RX` |

Planned host path:

```text
Web Browser
    ↓
Web Serial API
    ↓
USB-to-UART
    ↓
STM32F334 USART1
```

### 10.5 SWD

| MCU pin | Signal |
|---|---|
| PA13 | `SWDAT` |
| PA14 | `SWCLK` |

### 10.6 OLED

| MCU pin | Signal |
|---|---|
| PB8 | `I2C1_SCL` |
| PB9 | `I2C1_SDA` |

### 10.7 LEDs

| MCU pin | Signal |
|---|---|
| PB0 | `LED_G` |
| PB1 | `LED_Y` |
| PB2 | `LED_R` |

---

## 11. Vendor ADC Timing

The vendor software report configures:

- ADC1 channels 1–4 for Vin, Iin, Vout, and Iout
- 12-bit sampling
- ADC1 scan mode
- DMA circular transfer
- HRTIM Timer A Compare3 as ADC trigger
- 4.5 ADC-clock-cycle sample time

This matters because `Iin`/`Iout` reconstruction quality depends on the switching state at the ADC sample instant.

The independent firmware must deliberately characterize and select ADC sampling phase.

---

## 12. Hardware Constraints Relevant to Firmware

### 12.1 No added current sensor

Only existing sensing channels are used:

```text
Vin
Iin
Vout
Iout
VADJ
```

### 12.2 No MCU-controlled gate-driver disable

The Si8233 `DISABLE` pins are tied low, so protection architecture must rely on HRTIM-safe output forcing and applicable STM32F334 fault paths.

### 12.3 Bootstrap constraints

The high-side drive uses bootstrap components. Modulation must account for:

```text
maximum duty
minimum off-time
minimum pulse width
bootstrap refresh
dead time
```

### 12.4 Bidirectional auxiliary supply

The logic/gate-drive supply can be powered from either converter port. Startup logic must therefore handle:

```text
Vin energized, Vout discharged
Vin energized, Vout pre-biased
Vin absent, Vout energized
both ports energized
reverse-power startup
```

### 12.5 Measurement/debug safety

Vendor documentation explicitly warns:

- do not perform online debugging while converter input power is connected;
- incorrect PWM can cause MOSFET shoot-through;
- do not simultaneously use ordinary earth-referenced probes on floating high-side and low-side nodes;
- use appropriate isolated/differential measurement methods for high-side measurements.

---

## 13. Derived Nominal Measurement Constants

These are schematic-derived nominal starting values, not final calibration constants.

| Quantity | Nominal value | Basis |
|---|---:|---|
| Voltage gain `KV` | 0.04853 V/V | 3.3 kΩ / 68 kΩ |
| Port voltage at 3.3 V ADC | ~68.0 V | `3.3 / KV` |
| Current amplifier gain | 150 V/V | 15 kΩ / 100 Ω |
| Current sensitivity | 0.150 V/A | 150 × 1 mΩ |
| Zero-current ADC voltage | 1.65 V nominal | Midscale bias |
| +5 A ADC voltage | 2.40 V nominal | `1.65 + 5 × 0.15` |
| −5 A ADC voltage | 0.90 V nominal | `1.65 − 5 × 0.15` |
| ADC output RC pole | ~4.82 MHz | 100 Ω / 330 pF |
| First auxiliary rail | ~10.0 V | XL7005A divider |
| Potentiometer max | ~2.75 V | 3.3 V, 10 kΩ + 50 kΩ |

Use calibrated values in control firmware.

---

## 14. Recommended Firmware Hardware Abstraction

```text
board_measurements_t
    vin
    iin
    vout
    iout
    vadj

board_pwm_t
    left_high   -> Q1
    left_low    -> Q4
    right_high  -> Q2
    right_low   -> Q3

board_comms_t
    USART1 TX   -> PB6
    USART1 RX   -> PB7
```

Recommended dependency direction:

```text
Control / Estimation
        ↓
Modulation
        ↓
Board / Platform API
        ↓
libopencm3
        ↓
STM32F334
```

---

## 15. Key Hardware Facts to Freeze

1. Physical half bridges are `Q1/Q4` on the left and `Q2/Q3` on the right.
2. Main inductor is nominally 22 µH.
3. Nominal switching/control rate is 200 kHz.
4. Port-current sensing already exists for both `Iin` and `Iout`.
5. Current shunts are 1 mΩ.
6. Current sensing is bidirectional using a nominal 1.65 V ADC bias.
7. There is no dedicated inductor-current ADC channel.
8. No additional current sensor will be added.
9. Inductor current must be reconstructed if required by the controller.
10. Existing host interface is USART1 on PB6/PB7.
11. Gate-driver `DISABLE` is not MCU-controlled.
12. Schematic net `P12V` is nominally about 10 V according to the documented regulator design.
13. Auxiliary power can be sourced from either converter port.
14. ADC sample timing relative to PWM is a control-design variable.
15. Safe PWM shutdown must be verified before closed-loop power experiments.

---

## 16. Source Hierarchy

For implementation decisions:

1. **2022 V1.2 schematic** — physical net/component truth
2. **Physical board measurement** — final empirical truth
3. **2021 hardware design report** — design intent and derivation
4. **2021 software design report** — vendor firmware reference
5. **2021 user manual** — vendor operating specifications and safety guidance

If a conceptual diagram conflicts with the V1.2 schematic, record the discrepancy and follow the actual schematic/netlist for firmware implementation.

---

## 17. Immediate Project Architecture

```text
Existing sensors
Vin / Iin / Vout / Iout
        ↓
Calibration / synchronized sampling
        ↓
State reconstruction / iL estimator
        ↓
Controller
        ↓
Unified modulation
        ↓
HRTIM
        ↓
Q1 / Q4 + Q2 / Q3
```

Host supervision:

```text
Web App
   ↓
Web Serial
   ↓
USB-to-UART
   ↓
USART1
   ↓
COBS + CRC protocol
   ↓
Power Manager / Telemetry
```

Real-time control and protection remain local to the STM32F334.
