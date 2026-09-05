# CBB02405D / CBB024D V1.2 Hardware Specification

## Purpose

This document defines the physical hardware facts used by `bidirectional-buckboost-control`.

The CBB024D V1.2 schematic/net mapping is authoritative for switch identity, signal routing, and MCU pin mapping.

## Converter topology

The board is a non-isolated synchronous four-switch bidirectional buck-boost converter:

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

| Half bridge | High-side MOSFET | Low-side MOSFET | PWM signals |
| --- | --- | --- | --- |
| Left / Port A | Q1 | Q4 | `PWM1H`, `PWM1L` |
| Right / Port B | Q2 | Q3 | `PWM2H`, `PWM2L` |

## Published board ratings

| Item | Value |
| --- | --- |
| Input voltage | 12–48 VDC |
| Output voltage | 5–48 VDC |
| Rated output | 24 V / 5 A |
| Suggested maximum output power | 200 W |
| Output current range | 0–5 A |
| Published current limit | 5 A |
| Published output ripple | ≤ 1% p-p |
| Switching frequency | 200 kHz |
| Control frequency | 200 kHz |
| Cooling | natural convection |
| Operating temperature | -30 to +40 °C |
| Storage temperature | -40 to +80 °C |
| Board size | approximately 100 × 90 × 20 mm |
| Communication | USART1 header |
| Programming/debug | SWD |

These ratings describe the board hardware; controller-specific validated envelopes are defined by test evidence and configuration.

## Main power components

### MOSFETs

| Reference | Part |
| --- | --- |
| Q1 | BSC070N10NS3G |
| Q2 | BSC070N10NS3G |
| Q3 | BSC070N10NS3G |
| Q4 | BSC070N10NS3G |

Each MOSFET gate has a gate-source pull resistor. External gate-drive resistance is 10 Ω.

### Main inductor

| Item | Value |
| --- | --- |
| Reference | L1 |
| Nominal inductance | 22 µH |
| Tolerance | ±20% |
| DCR reference | approximately 20.5 mΩ typical |

### Port capacitance

Each power port contains two 220 µF / 63 V bulk capacitors and two 10 µF local capacitors.

```text
bulk capacitance per port        ≈ 440 uF
nominal sum including 10 uF caps ≈ 460 uF
```

Effective capacitance and ESR are operating-condition-dependent model parameters.

## Gate-drive hardware

| Item | Hardware |
| --- | --- |
| Gate driver | Si8233BD-D-IS |
| Quantity | 2 |
| External gate resistor | 10 Ω |
| DT network | 3.3 kΩ to 5 V |
| Bootstrap diode | SS210 |
| Bootstrap capacitor | 100 nF |
| Logic supply | 5 V |
| Driver `DISABLE` | not MCU-controlled on V1.2 |

PWM mapping:

| PWM signal | MOSFET |
| --- | --- |
| `PWM1H` | Q1 |
| `PWM1L` | Q4 |
| `PWM2H` | Q2 |
| `PWM2L` | Q3 |

Safe-off authority is therefore implemented through GPIO/HRTIM output state rather than a dedicated MCU gate-driver-disable signal.

## Auxiliary power

Auxiliary power can be sourced from either converter port through diode isolation:

```text
VIN+  -- diode --+
                 +--> auxiliary regulator input
VOUT+ -- diode --+
```

Rail chain:

| Stage | Device | Nominal rail |
| --- | --- | --- |
| first | XL7005A | approximately 10 V |
| second | AMS1117-5 | 5 V |
| third | AMS1117-3.3 | 3.3 V |
| analog filtering | L3/L4 + capacitors | filtered analog 3.3 V/return |

The schematic net named `P12V` is approximately 10 V from the XL7005A divider and is treated as a legacy net name.

## Voltage sensing

Both terminal-voltage channels use GS8552-SR conditioning.

Nominal scale:

```text
Kv = 3.3 kΩ / 68 kΩ
   ≈ 0.04853 V/V

Vadc  ≈ Kv * Vport
Vport ≈ Vadc / Kv
```

A 3.3 V ADC input corresponds to approximately 68 V at the sense input. This is a sensing full-scale estimate, not an operating-voltage rating.

The output RC network is approximately 100 Ω / 330 pF, with an ideal pole near 4.82 MHz.

## Bidirectional current sensing

| Signal | Shunt | Value |
| --- | --- | --- |
| `Iin` | R7 | 1 mΩ, 1%, 2512 |
| `Iout` | R8 | 1 mΩ, 1%, 2512 |

Nominal amplifier ratio:

```text
15 kΩ / 100 Ω = 150 V/V
```

With the 1 mΩ shunt:

```text
sensitivity ≈ 0.150 V/A
Vadc,I      ≈ 1.65 + 0.150 * I
```

| Current | Nominal ADC voltage |
| ---: | ---: |
| -5 A | 0.90 V |
| 0 A | 1.65 V |
| +5 A | 2.40 V |

Current-channel calibration owns measured gain, offset, and polarity. Negative current is valid.

## Main-inductor current access

The MCU has no dedicated ADC channel for `iL`.

`CNT1/CNT2` is an inductor-series measurement access point for external instrumentation. Runtime control reconstructs `iL` as `iL_hat` from the existing board signals and converter state.

## Local reference input

`VADJ` uses a 50 kΩ potentiometer with a 10 kΩ feed from 3.3 V.

Approximate unloaded maximum:

```text
3.3 * 50 / (50 + 10) ≈ 2.75 V
```

## MCU and signal mapping

| Item | Value |
| --- | --- |
| MCU | STM32F334C8T6 |
| HCLK | 64 MHz |
| HRTIM clock | 128 MHz |
| ADC clock | 64 MHz reference configuration |
| ADC resolution | 12 bit |

### PWM

| MCU pin | Signal | Device |
| --- | --- | --- |
| PA8 | `PWM1H` | Q1 |
| PA9 | `PWM1L` | Q4 |
| PA10 | `PWM2H` | Q2 |
| PA11 | `PWM2L` | Q3 |

### ADC

| MCU pin | Signal |
| --- | --- |
| PA0 | `ADC_Vin` |
| PA1 | `ADC_Iin` |
| PA2 | `ADC_Vout` |
| PA3 | `ADC_Iout` |
| PA4 | `ADC_VADJ` |

The four terminal measurements are acquired through ADC1 scan/DMA; `VADJ` uses ADC2.

### Local I/O

| MCU pin | Signal | Behavior |
| --- | --- | --- |
| PB3 | `KEY1` | external pull-up, active-low |
| PB4 | `KEY2` | external pull-up, active-low |
| PB0 | `LED_G` | active-high |
| PB1 | `LED_Y` | active-high |
| PB2 | `LED_R` | active-high |
| PB6 | `USART1_TX` | host UART TX |
| PB7 | `USART1_RX` | host UART RX |
| PB8 | `I2C1_SCL` | OLED/I2C |
| PB9 | `I2C1_SDA` | OLED/I2C |
| PA13 | `SWDAT` | SWD data |
| PA14 | `SWCLK` | SWD clock |

## Hardware constraints visible to firmware

1. No dedicated `iL` ADC channel.
2. No MCU-controlled Si8233 `DISABLE` signal.
3. Bootstrap high-side drive imposes refresh/off-time constraints.
4. Physical half bridges are Q1/Q4 left and Q2/Q3 right.
5. Port-current sensing is bidirectional around a nominal 1.65 V offset.
6. Auxiliary logic power can be sourced from either converter port.
7. HRTIM and ADC timing are part of the control-system model.
8. High-side/switch-node probing requires differential or isolated instrumentation.
9. Energized power-stage operation is not compatible with arbitrary MCU halt through online debugging.

## Nominal measurement constants

| Quantity | Nominal value |
| --- | ---: |
| voltage gain | 0.04853 V/V |
| equivalent port at 3.3 V ADC | ~68.0 V |
| current amplifier gain | 150 V/V |
| current sensitivity | 0.150 V/A |
| zero-current voltage | 1.65 V |
| +5 A voltage | 2.40 V |
| -5 A voltage | 0.90 V |
| sense RC pole | ~4.82 MHz ideal |
| first auxiliary rail | ~10.0 V |
| `VADJ` max | ~2.75 V unloaded |

Control firmware uses calibrated values where calibration exists.