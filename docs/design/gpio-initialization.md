# GPIO Initialization and Safe Startup States

## Purpose

This document defines the deterministic startup state of MCU GPIOs on the CBB024D V1.2 board before any converter-control action is permitted. The rules are permanent platform requirements, not gate-specific status notes.

The V1.2 schematic is the physical source of truth.

## Startup Rule

Power-stage gate-driver inputs are initialized first and held inactive. No peripheral initialization may implicitly authorize switching.

Canonical startup order:

```text
Reset
  ↓
preload PA8..PA11 LOW
  ↓
configure PA8..PA11 as GPIO outputs, still LOW
  ↓
initialize system clock
  ↓
initialize non-power board GPIOs
  ↓
initialize timebase
  ↓
initialize UART / other passive services
  ↓
initialize ADC/HRTIM as required, outputs still inactive
  ↓
Power Manager remains OFF until qualification
```

## Verified GPIO Mapping and Startup State

| MCU pin | Board signal | Startup configuration | Startup state | Owner |
| --- | --- | --- | --- | --- |
| PA0 | `ADC_Vin` | Analog, no pull | Passive | sensing |
| PA1 | `ADC_Iin` | Analog, no pull | Passive | sensing |
| PA2 | `ADC_Vout` | Analog, no pull | Passive | sensing |
| PA3 | `ADC_Iout` | Analog, no pull | Passive | sensing |
| PA4 | `ADC_VADJ` | Analog, no pull | Passive | sensing |
| PA8 | `PWM1H` | GPIO output, pull-down | LOW | power stage |
| PA9 | `PWM1L` | GPIO output, pull-down | LOW | power stage |
| PA10 | `PWM2H` | GPIO output, pull-down | LOW | power stage |
| PA11 | `PWM2L` | GPIO output, pull-down | LOW | power stage |
| PA13 | `SWDAT` | not reconfigured | SWD | debug |
| PA14 | `SWCLK` | not reconfigured | SWD | debug |
| PB0 | `LED_G` | push-pull output | LOW / off | board I/O |
| PB1 | `LED_Y` | push-pull output | LOW / off | board I/O |
| PB2 | `LED_R` | push-pull output | LOW / off | board I/O |
| PB3 | `KEY1` | input, no internal pull | external pull-up / active-low | board I/O |
| PB4 | `KEY2` | input, no internal pull | external pull-up / active-low | board I/O |
| PB6 | `USART1_TX` | input + pull-up before UART AF | high/passive | UART |
| PB7 | `USART1_RX` | input + pull-up before UART AF | high/passive | UART |
| PB8 | `I2C1_SCL` | input, no internal pull | high-impedance | OLED/I2C |
| PB9 | `I2C1_SDA` | input, no internal pull | high-impedance | OLED/I2C |

LEDs are active-high, so LOW is the inactive state. KEY1/KEY2 have external 10 kΩ pull-ups and RC filtering; pressed = LOW.

## Verified Unused Pins

The following pins have no connected board signal on the V1.2 schematic and are placed in analog/no-pull mode to avoid floating digital inputs:

```text
PA5 PA6 PA7 PA12 PA15
PB5 PB10 PB11 PB12 PB13 PB14 PB15
PC13 PC14 PC15
PF0 PF1
```

This policy is board-revision-specific and must be reviewed if a different revision is supported.

## Deliberately Excluded Pins

`PA8..PA11` are owned by the power-stage platform code, not generic board I/O.

`PA13/PA14` remain reserved for SWD.

`NRST` and `BOOT0` are hardware startup/debug signals and are not managed as application GPIOs.

## HRTIM Ownership Handoff

Before HRTIM takes ownership of PA8–PA11, the pins remain GPIO safe-low. The permanent handoff rule is:

```text
GPIO safe-low
    ↓
configure HRTIM timebase / waveform / polarity
    ↓
configure dead time and fault behavior
    ↓
force HRTIM outputs inactive
    ↓
switch PA8..PA11 to HRTIM alternate function
    ↓
verify outputs remain inactive
    ↓
Power Manager qualification
    ↓
explicit PWM enable
```

If HRTIM is reset, reconfigured, or loses valid configuration, the platform must return to an explicitly inactive state before switching authority can be restored.

## Rules

1. Reset/startup never produces a gate pulse.
2. Generic board-I/O code never manipulates PA8–PA11 after ownership is assigned to the power-stage module.
3. HRTIM configuration and HRTIM enable are separate operations.
4. Keys must not directly bypass Power Manager or protection policy.
5. LED behavior is diagnostic only and has no safety authority.
6. SWD pins remain usable during unpowered/low-risk bring-up, subject to the board vendor’s warning against online debugging while the converter is energized.
