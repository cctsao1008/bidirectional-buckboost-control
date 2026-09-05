# Protection and State Machine

## Purpose

This document defines Power Manager states, switching authority, fault ownership, shutdown, and recovery semantics.

Protection is independent of the selected controller and always has higher authority than host or controller requests.

## Authority chain

```text
measurements / faults / requests
        ↓
Power Manager
        ↓
controller enable + reference limits
        ↓
controller
        ↓
modulation
        ↓
HRTIM / gate driver
```

A controller produces actuation only while the Power Manager authorizes it.

## Externally visible power states

| Value | State | Meaning |
| ---: | --- | --- |
| 0 | `OFF` | initialized/inactive; no switching authority |
| 1 | `QUALIFY` | startup conditions are being evaluated |
| 2 | `SOFT_START` | controlled energization/reference ramp |
| 3 | `REGULATION` | normal closed-loop operation |
| 4 | `SHUTDOWN` | controlled stop |
| 5 | `FAULT` | faulted; PWM suppressed by policy |
| 6 | `RETRY_WAIT` | recoverable-fault delay before requalification |

Reset/boot is an internal initialization condition, not a stable wire-level state.

## State flow

```text
BOOT/RESET
    ↓
   OFF
    ↓ enable request
 QUALIFY
    ↓ qualified
SOFT_START
    ↓ complete
REGULATION
    ↓ disable request
 SHUTDOWN
    ↓
   OFF
```

Fault flow:

```text
qualifying or energized state
          ↓
        FAULT
       /     \
 latched    recoverable
   |            ↓
 remain      RETRY_WAIT
 FAULT          ↓
             QUALIFY
```

Fault clear never skips `QUALIFY`.

## `OFF`

- no switching authority;
- HRTIM outputs inactive;
- controller output ignored;
- host telemetry allowed;
- passive sensing/calibration allowed only when its assumptions are valid;
- enable request maps to `QUALIFY` only.

## `QUALIFY`

Qualification evaluates the conditions required by the requested operating envelope:

- measurement validity and plausibility;
- terminal-voltage range and pre-bias state;
- no blocking fault;
- valid reference and direction;
- HRTIM/modulation ready while inactive;
- estimator validity when required by the selected controller;
- valid configuration/calibration.

Failed qualification does not enable switching.

## `SOFT_START`

Soft start is a controlled state transition. It owns:

- bounded reference ramp;
- bounded `iL_ref` and actuation slew;
- controller-state initialization;
- estimator-state/validity initialization;
- startup fault detection;
- pre-biased and reverse-powered terminal handling.

## `REGULATION`

Closed-loop regulation is active only in this state. The Power Manager retains authority over:

- controller enable;
- reference limits;
- allowed power-flow direction;
- shutdown requests;
- fault transitions;
- estimator-validity response.

Operating-region labels belong to modulation and do not change Power Manager ownership.

## `SHUTDOWN`

Normal shutdown is deterministic and distinct from emergency suppression. It includes the subset required by the active energy-flow condition:

- reference ramp-down;
- bounded current reduction;
- transition toward zero power flow;
- controller disable;
- deterministic PWM stop;
- confirmation of inactive outputs.

A fault requiring immediate suppression bypasses the controlled ramp.

## `FAULT`

```text
fault source
    ↓
PWM/HRTIM suppression
    ↓
fault capture
    ↓
FAULT
    ↓
latch or retry policy
```

Fault state retains the context needed to identify the triggering condition.

## `RETRY_WAIT`

Only faults configured as recoverable enter retry. Retry semantics define:

- delay;
- retry count/backoff where configured;
- escalation to latched `FAULT`;
- controller/estimator reinitialization;
- mandatory return through `QUALIFY`.

## Fault classes

The protection namespace covers:

```text
overcurrent / short circuit
Port A over/undervoltage
Port B over/undervoltage
ADC saturation / sensing implausibility
HRTIM / timing / configuration fault
estimator invalid while required
unrealizable modulation request
internal deadline / health fault
host/session policy fault
```

Thresholds, debounce times, retry policy, and latch behavior are explicit configuration values.

## Protection hierarchy

```text
1. hardware-immediate / HRTIM output suppression
2. modulation hard bounds and illegal-state prevention
3. Power Manager qualification and state limits
4. controller constraints and anti-windup
```

A higher-numbered layer cannot weaken a lower-numbered layer.

## Host ownership

```text
OUTPUT_ENABLE  -> request OFF -> QUALIFY
OUTPUT_DISABLE -> request SHUTDOWN
CLEAR_FAULT    -> request fault-policy evaluation
```

No normal protocol command directly enables HRTIM, writes gate states, disables mandatory protection, or jumps to `REGULATION`.

## Controller and estimator state transfer

A controller change, fault recovery, or restart explicitly handles all state that affects actuation:

```text
PI/LQI integrator state
observer/filter state
previous vL*
previous e1/e2 and d1/d2
reference-ramp state
```

State is either transferred under defined compatible semantics or reinitialized before actuation resumes.

## Fault record fields

A fault record contains the available subset of:

```text
timestamp / sample index
power state
fault bits and source
Vin / Iin / Vout / Iout
iL_hat + confidence
power-flow direction
operating-region label
vL*
e1 / e2
d1 / d2
references
controller type
retry count
```

## Invariants

1. Reset converges to `OFF` with inactive outputs.
2. Invalid enable requests never create switching authority.
3. Fault authority overrides controller and host authority.
4. Recoverable retry always returns through `QUALIFY`.
5. Latched faults remain latched until policy permits clear.
6. Reverse or pre-biased operation cannot bypass qualification.
7. Controller choice cannot remove mandatory protection.