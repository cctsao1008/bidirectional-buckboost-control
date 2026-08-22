# GPIO Initialization and Safe Startup States

## Purpose

This document defines the deterministic startup state of MCU GPIOs on the CBB024D V1.2 board before any converter control, ADC acquisition, display activity, or host command is allowed to affect the power stage.

The CBB024D V1.2 schematic is the source of truth for the pin mapping in this document.

## Startup rule

The power-stage gate-driver inputs are always initialized first and held inactive. No peripheral initialization is allowed to implicitly enable PWM generation.

Startup order:

```text
Reset
  |
  v
Force PA8..PA11 LOW
  |
  v
Configure PA8..PA11 as GPIO outputs, still LOW
  |
  v
Initialize system clock
  |
  v
Initialize verified non-power GPIOs
  |
  v
Initialize SysTick
  |
  v
Initialize USART1
  |
  v
Start host protocol service
```

HRTIM is intentionally not initialized during Gate 1.

## Verified GPIO mapping and startup state

| MCU pin | Board signal | Startup configuration | Startup state | Ownership |
|---|---|---|---|---|
| PA0 | `ADC_Vin` | Analog, no pull | Passive | ADC/sensing |
| PA1 | `ADC_Iin` | Analog, no pull | Passive | ADC/sensing |
| PA2 | `ADC_Vout` | Analog, no pull | Passive | ADC/sensing |
| PA3 | `ADC_Iout` | Analog, no pull | Passive | ADC/sensing |
| PA4 | `ADC_VADJ` | Analog, no pull | Passive | ADC/sensing |
| PA8 | `PWM1H` | GPIO output, pull-down | LOW | Power stage |
| PA9 | `PWM1L` | GPIO output, pull-down | LOW | Power stage |
| PA10 | `PWM2H` | GPIO output, pull-down | LOW | Power stage |
| PA11 | `PWM2L` | GPIO output, pull-down | LOW | Power stage |
| PA13 | `SWDAT` | Not reconfigured | SWD | Debug |
| PA14 | `SWCLK` | Not reconfigured | SWD | Debug |
| PB0 | `LED_G` | Push-pull output | LOW / off | Board I/O |
| PB1 | `LED_Y` | Push-pull output | LOW / off | Board I/O |
| PB2 | `LED_R` | Push-pull output | LOW / off | Board I/O |
| PB3 | `KEY1` | Input, no internal pull | External pull-up / active-low | Board I/O |
| PB4 | `KEY2` | Input, no internal pull | External pull-up / active-low | Board I/O |
| PB6 | `USART1_TX` | Input with pull-up until UART init | High / passive | UART |
| PB7 | `USART1_RX` | Input with pull-up until UART init | High / passive | UART |
| PB8 | `I2C1_SCL` | Input, no internal pull | High-impedance | OLED/I2C |
| PB9 | `I2C1_SDA` | Input, no internal pull | High-impedance | OLED/I2C |

The three status LEDs are active-high according to the vendor hardware documentation, so LOW is the inactive startup state.

KEY1 and KEY2 have external 10 kOhm pull-ups to 3.3 V and RC filtering on the V1.2 board. A pressed key pulls the corresponding MCU input low.

## Verified unused pins

The following MCU pins have no connected board signal in the CBB024D V1.2 schematic and are placed in analog mode during startup to avoid floating digital inputs:

```text
PA5 PA6 PA7 PA12 PA15
PB5 PB10 PB11 PB12 PB13 PB14 PB15
PC13 PC14 PC15
PF0 PF1
```

This policy is specific to the CBB024D V1.2 board. It must be reviewed before supporting another board revision.

## Pins deliberately not managed by board_io.c

`PA8..PA11` are owned exclusively by `power_stage.c`. They must remain LOW until a future HRTIM bring-up sequence explicitly transfers ownership to the HRTIM alternate-function outputs.

`PA13/PA14` are reserved for SWD and must remain available during bring-up.

`NRST` and `BOOT0` are hardware startup/debug signals and are not treated as application GPIOs.

## Future HRTIM handoff

When PWM support is introduced, the safe handoff must be:

```text
PA8..PA11 held LOW as GPIO
        |
        v
Configure HRTIM timebase
        |
        v
Configure dead time, output polarity, fault handling, and idle states
        |
        v
Force all HRTIM outputs inactive
        |
        v
Switch PA8..PA11 to HRTIM alternate function
        |
        v
Verify outputs remain inactive
        |
        v
Power Manager qualification
        |
        v
Explicit PWM enable
```

Peripheral initialization alone must never authorize switching.
