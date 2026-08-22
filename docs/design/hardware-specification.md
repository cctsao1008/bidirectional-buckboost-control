# CBB02405D / CBB024D V1.2 Hardware Specification

## Purpose

This document is the canonical project-owned hardware specification for the CBB024D V1.2 board used by `bidirectional-buckboost-control`.

Primary implementation source:

- `CBB024D-1-V1.2.pdf`, dated 2022-03-10

Cross-reference sources:

- vendor bidirectional BUCK-BOOST hardware design report, Rev1.0, 2021-04-10;
- vendor user manual, Rev1.0, 2021-04-10;
- vendor software design report, Rev1.0, 2021-04-10;
- vendor example firmware.

Where older conceptual material conflicts with the V1.2 schematic/net mapping, the V1.2 schematic controls the physical implementation decision.

## 1. Converter Topology

The board is a non-isolated synchronous four-switch bidirectional buck-boost converter using two half bridges and one main inductor.

```text
Port A / VIN side                    Port B / VOUT side

      Q1 high                              Q2 high
         |                                    |
         +----------- L1 = 22 uH -------------+
         |                                    |
      Q4 low                               Q3 low
         |                                    |
        GND----------------------------------GND
```

### 1.1 Canonical V1.2 bridge mapping

| Half bridge | High-side MOSFET | Low-side MOSFET | PWM signals |
| --- | --- | --- | --- |
| Left / Port A | Q1 | Q4 | `PWM1H`, `PWM1L` |
| Right / Port B | Q2 | Q3 | `PWM2H`, `PWM2L` |

This mapping is mandatory for firmware and documentation.

### 1.2 Vendor-document labeling caveat

Some older vendor conceptual diagrams group switch labels differently. Those diagrams are useful for topology explanation but must not override the V1.2 schematic/net routing.

## 2. Board-Level Ratings

| Item | Vendor/reference specification |
| --- | --- |
| Topology | four-switch synchronous bidirectional buck-boost |
| Input voltage | 12–48 VDC |
| Output voltage | 5–48 VDC |
| Rated output | 24 V / 5 A |
| Suggested maximum output power | 200 W |
| Output current range | 0–5 A |
| Published current limit | 5 A |
| Published output ripple | <= 1% p-p |
| Switching frequency | 200 kHz |
| Vendor control frequency | 200 kHz |
| Cooling | natural convection |
| Operating temperature | -30 to +40 C |
| Storage temperature | -40 to +80 C |
| Board size | about 100 x 90 x 20 mm |
| Communication | reserved UART |
| Programming/debug | SWD |

These are vendor/reference ratings, not automatically the validated operating envelope of every new controller.

## 3. Vendor Protection Evidence and Documentation Conflict

Vendor material indicates support for short-circuit, overcurrent, input under/overvoltage, and output overvoltage handling. However, the vendor documents are not fully consistent about recovery semantics: user/product-facing material describes automatic recovery for several protection cases, while the software reference distinguishes more severe fault behavior for at least some overvoltage conditions.

Therefore this repository does **not** collapse the vendor evidence into one invented recovery rule.

Project rule:

> Vendor thresholds and recovery behavior are reference evidence only. Independent firmware protection states, thresholds, retry, and latch policy are defined by `protection-and-state-machine.md` and validated as implementation deltas.

## 4. Main Power Components

### 4.1 MOSFETs

| Reference | Part |
| --- | --- |
| Q1 | BSC070N10NS3G |
| Q2 | BSC070N10NS3G |
| Q3 | BSC070N10NS3G |
| Q4 | BSC070N10NS3G |

Reference device data used for engineering estimates include approximately 100 V `VDS`, 6.3 mΩ typical `RDS(on)`, and 42 nC typical gate charge under datasheet conditions. Actual switching behavior depends on board conditions.

Each gate has a gate-source pull resistor; external gate-drive resistance is 10 Ω.

### 4.2 Main inductor

| Item | Value |
| --- | --- |
| Reference | L1 |
| Nominal inductance | 22 µH |
| Tolerance | ±20% |
| DCR reference value | about 20.5 mΩ typ |

The estimator/controller must tolerate parameter uncertainty; nominal L is not treated as identified truth.

### 4.3 Port capacitance

Input side includes two 220 µF / 63 V bulk capacitors plus 10 µF local capacitors. Output side uses the same nominal arrangement.

Useful starting values:

```text
bulk per port       ≈ 440 uF
including 2 x 10 uF ≈ 460 uF nominal
```

Effective capacitance, ESR, and ceramic DC-bias derating may differ materially from these nominal sums.

## 5. Gate-Drive Architecture

| Item | Hardware |
| --- | --- |
| Gate driver | Si8233BD-D-IS |
| Quantity | 2 |
| External gate resistor | 10 Ω |
| DT-programming resistor | 3.3 kΩ to 5 V reference network |
| Bootstrap diode | SS210 |
| Bootstrap capacitor | 100 nF |
| Driver logic supply | 5 V |
| Driver `DISABLE` | tied low / not MCU-controlled on V1.2 |

### 5.1 PWM-to-device mapping

| PWM signal | MOSFET |
| --- | --- |
| `PWM1H` | Q1 |
| `PWM1L` | Q4 |
| `PWM2H` | Q2 |
| `PWM2L` | Q3 |

### 5.2 Safety implication

Because driver `DISABLE` is not routed to the MCU, project firmware must establish safe-off behavior through GPIO/HRTIM output forcing and usable STM32F334 hardware fault paths. A software assumption that a separate gate-driver-disable line exists is invalid.

## 6. Auxiliary Power

The auxiliary supply can draw from either power port through diode isolation:

```text
VIN+  -- diode --+
                 +--> auxiliary regulator input
VOUT+ -- diode --+
```

Reference rail chain:

| Stage | Device | Nominal function |
| --- | --- | --- |
| first | XL7005A | approximately 10 V auxiliary/gate-drive rail |
| second | AMS1117-5 | 5 V |
| third | AMS1117-3.3 | 3.3 V |
| analog filtering | L3/L4 + capacitors | filtered analog supply/return |

### 6.1 `P12V` naming caveat

The schematic net is named `P12V`, but the XL7005A divider values (`33 kΩ`, `4.7 kΩ`, `VFB ≈ 1.25 V`) imply approximately:

```text
1.25 * (1 + 33 / 4.7) ≈ 10.0 V
```

The hardware design report also describes about 10 V. Treat `P12V` as a legacy net name, not proof of a regulated 12 V rail.

### 6.2 Startup implication

Because logic/gate-drive power can be sourced from either power port, firmware startup must consider:

```text
Port A energized / Port B discharged
Port A energized / Port B pre-biased
Port A absent / Port B energized
both ports energized
reverse-power startup
```

## 7. Voltage Sensing

Both terminal voltages use GS8552-SR signal-conditioning stages.

Nominal scale:

```text
Kv = 3.3 kΩ / 68 kΩ
   ≈ 0.04853 V/V
```

Thus:

```text
Vadc  ≈ Kv * Vport
Vport ≈ Vadc / Kv
```

Examples:

```text
48 V port -> about 2.33 V ADC
3.3 V ADC -> about 68.0 V equivalent sense full scale
```

The approximately 68 V figure is only a sensing full-scale estimate, not a board operating rating.

Reference output RC uses approximately 100 Ω / 330 pF, giving an ideal pole near 4.82 MHz before op-amp/ADC effects.

## 8. Bidirectional Current Sensing

### 8.1 Shunts

| Signal | Shunt | Value |
| --- | --- | --- |
| `Iin` | R7 | 1 mΩ, 1%, 2512 |
| `Iout` | R8 | 1 mΩ, 1%, 2512 |

### 8.2 Amplification and bias

The current paths use GS8552-SR differential amplification with nominal resistor ratio:

```text
15 kΩ / 100 Ω = 150 V/V
```

With a 1 mΩ shunt:

```text
sensitivity ≈ 0.150 V/A
```

The ADC current signal is centered on nominal 1.65 V:

```text
Vadc,I = 1.65 + 0.150 * I
```

Nominal examples:

```text
-5 A -> 0.90 V
 0 A -> 1.65 V
+5 A -> 2.40 V
```

Measured zero offset and polarity must be encoded in calibration. Negative current is valid and is not clipped by the project sensing layer.

### 8.3 1.65 V reference

The bias is derived from a 3.3 kΩ / 3.3 kΩ divider, buffered by the GS8552-SR family device and filtered. Firmware must not assume exact 1.650 V when measured offset is available.

### 8.4 No direct `iL` ADC

MCU ADC inputs are:

```text
Vin
Iin
Vout
Iout
VADJ
```

There is no dedicated main-inductor-current ADC channel.

The `CNT1/CNT2` location is an inductor-series measurement access point intended for external development measurement and is not an MCU current channel.

Final project constraint:

> No added inductor-current sensor. Controllers that require `iL` use `iL_hat` reconstructed from existing signals and converter state.

## 9. Analog Reference Potentiometer

Reference hardware uses a 50 kΩ potentiometer with a 10 kΩ series feed from 3.3 V and an ADC filter network. Approximate unloaded maximum is:

```text
3.3 * 50 / (50 + 10) ≈ 2.75 V
```

The project may use `VADJ` for local/reference experiments, but host-controlled operation does not depend on it.

## 10. STM32F334 and Pin Mapping

### 10.1 MCU/peripheral reference configuration

| Item | Vendor/reference value |
| --- | --- |
| MCU | STM32F334C8T6 |
| HCLK | 64 MHz |
| HRTIM clock | 128 MHz |
| ADC clock | 64 MHz |
| ADC resolution | 12 bit |
| switching/control rate | 200 kHz |

### 10.2 PWM

| MCU pin | Signal | Device |
| --- | --- | --- |
| PA8 | `PWM1H` | Q1 left high-side |
| PA9 | `PWM1L` | Q4 left low-side |
| PA10 | `PWM2H` | Q2 right high-side |
| PA11 | `PWM2L` | Q3 right low-side |

### 10.3 ADC

| MCU pin | Signal |
| --- | --- |
| PA0 | `ADC_Vin` |
| PA1 | `ADC_Iin` |
| PA2 | `ADC_Vout` |
| PA3 | `ADC_Iout` |
| PA4 | `ADC_VADJ` |

Vendor firmware uses ADC1 scan/DMA for the four terminal measurements and ADC2 for `VADJ`.

### 10.4 Local keys

| MCU pin | Signal | Hardware behavior |
| --- | --- | --- |
| PB3 | `KEY1` | external 10 kΩ pull-up, active-low, RC filtered |
| PB4 | `KEY2` | external 10 kΩ pull-up, active-low, RC filtered |

Keys are inputs to application policy only; they must not directly bypass Power Manager authority.

### 10.5 LEDs

| MCU pin | Signal |
| --- | --- |
| PB0 | `LED_G` |
| PB1 | `LED_Y` |
| PB2 | `LED_R` |

Reference documentation indicates active-high LEDs.

### 10.6 UART

| MCU pin | Signal |
| --- | --- |
| PB6 | `USART1_TX` |
| PB7 | `USART1_RX` |

### 10.7 OLED / I2C

| MCU pin | Signal |
| --- | --- |
| PB8 | `I2C1_SCL` |
| PB9 | `I2C1_SDA` |

### 10.8 SWD

| MCU pin | Signal |
| --- | --- |
| PA13 | `SWDAT` |
| PA14 | `SWCLK` |

### 10.9 Verified unused pins on V1.2

```text
PA5 PA6 PA7 PA12 PA15
PB5 PB10 PB11 PB12 PB13 PB14 PB15
PC13 PC14 PC15
PF0 PF1
```

Board-specific startup treatment is defined in `gpio-initialization.md`.

## 11. Vendor ADC Timing Evidence

Reference firmware configures:

- ADC1 channels 1–4 for `Vin`, `Iin`, `Vout`, `Iout`;
- scan mode and circular DMA;
- HRTIM Timer A Compare3 as ADC trigger;
- short ADC sample time at the vendor clock configuration.

This is relevant because terminal-current/voltage samples are switching-phase dependent. The independent implementation deliberately defines trigger phase, channel sequence, conversion latency, and control consume point rather than assuming vendor timing is automatically correct for the new estimator.

## 12. Hardware Constraints That Firmware Must Respect

1. No dedicated `iL` ADC and no added final-architecture sensor.
2. No MCU-controlled gate-driver `DISABLE`.
3. Bootstrap high-side drive imposes off-time/refresh constraints.
4. Physical half bridges are Q1/Q4 left and Q2/Q3 right.
5. Port-current sensing is bidirectional around a nominal 1.65 V offset.
6. Auxiliary logic power can come from either converter port.
7. HRTIM/ADC timing is part of control design.
8. SWD debugging must follow the vendor’s powered-converter safety warning.
9. High-side/switch-node probing requires differential or otherwise isolated instrumentation.

## 13. Nominal Measurement Constants

| Quantity | Nominal value | Basis |
| --- | ---: | --- |
| voltage gain | 0.04853 V/V | 3.3 kΩ / 68 kΩ |
| equivalent port at 3.3 V ADC | ~68.0 V | `3.3 / Kv` |
| current amplifier gain | 150 V/V | 15 kΩ / 100 Ω |
| current sensitivity | 0.150 V/A | gain × 1 mΩ |
| zero-current voltage | 1.65 V nominal | midpoint bias |
| +5 A voltage | 2.40 V nominal | ideal |
| -5 A voltage | 0.90 V nominal | ideal |
| sense RC pole | ~4.82 MHz ideal | 100 Ω / 330 pF |
| first auxiliary rail | ~10.0 V | XL7005A divider |
| `VADJ` max | ~2.75 V unloaded | 3.3 V, 10 kΩ + 50 kΩ |

Use calibrated values in control firmware.

## 14. Canonical Hardware Abstraction

```text
measurements
  Vin / Iin / Vout / Iout / VADJ

power-stage mapping
  left_high  -> Q1
  left_low   -> Q4
  right_high -> Q2
  right_low  -> Q3

host UART
  TX -> PB6
  RX -> PB7
```

Dependency direction:

```text
Control / Estimation
        ↓
Modulation
        ↓
Platform API
        ↓
libopencm3 / STM32F334
```

## 15. Source Hierarchy

For physical implementation decisions:

1. **2022 V1.2 schematic/net mapping** — physical source of truth.
2. **Controlled physical board measurement** — empirical source when method/conditions are recorded.
3. **2021 hardware design report** — design intent/derivation.
4. **2021 software report/examples** — vendor implementation reference.
5. **2021 user/product material** — published operating guidance/specification.

When sources disagree, record the discrepancy rather than silently reconciling it.

## 16. Architecture Consequence

```text
existing Vin / Iin / Vout / Iout sensing
        ↓
PWM-synchronized calibration/acquisition
        ↓
iL estimator
        ↓
controller produces vL*
        ↓
unified d1 / d2 allocation
        ↓
HRTIM
        ↓
Q1/Q4 left + Q2/Q3 right
```

Real-time control/protection remains local to STM32F334; host communication is supervisory only.
