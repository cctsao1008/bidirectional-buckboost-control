# Protection and State Machine

## Purpose

This document is the canonical definition of Power Manager states, switching authority, fault ownership, shutdown, and recovery policy. Other documents should reference these states rather than invent local variants.

The independent firmware does not copy vendor retry or OVP policy blindly. Vendor behavior is reference evidence; project policy is explicit and testable.

## Supervisory Principle

A controller may request actuation only while the Power Manager authorizes it:

```text
measurements / faults / host-local requests
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

Protection always overrides controller and host requests.

## Canonical Externally Visible Power States

These values are also reserved by the host protocol:

| Value | State | Meaning |
| ---: | --- | --- |
| 0 | `OFF` | initialized or inactive; PWM authority absent |
| 1 | `QUALIFY` | startup conditions being checked |
| 2 | `SOFT_START` | controlled energization/reference ramp |
| 3 | `REGULATION` | normal closed-loop operation |
| 4 | `SHUTDOWN` | controlled stop in progress |
| 5 | `FAULT` | faulted; PWM suppressed according to policy |
| 6 | `RETRY_WAIT` | recoverable-fault delay before requalification |

Reset/boot is an internal initialization condition and is not a stable wire-level power state.

## Canonical State Flow

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

Fault paths:

```text
any energized/qualifying state
        ↓
      FAULT
       /  \
 latched    recoverable
   |           ↓
 remain     RETRY_WAIT
 FAULT         ↓
         re-QUALIFY
```

A fault-clear request never skips qualification.

## OFF

Required properties:

- no switching authority;
- HRTIM outputs inactive or platform held in an equivalent proven safe state;
- controller output ignored;
- host telemetry allowed;
- passive measurement/calibration allowed only when its assumptions are valid;
- enable request stored only as a request for `QUALIFY`.

## QUALIFY

Before energizing, check at least the prerequisites relevant to the requested direction and operating envelope:

- measurement validity/plausibility;
- voltage range and pre-bias state;
- no active blocking fault;
- valid target/reference;
- valid power-flow direction;
- HRTIM/modulation ready and inactive;
- estimator quality if the selected startup/control path requires it;
- configuration/calibration validity.

A failed qualification returns to `OFF` or enters `FAULT` depending on severity; it never falls through to switching.

## SOFT_START

Soft start is a controlled transition, not a timer delay. It may:

- ramp voltage/current/power reference;
- bound `iL_ref` and duty slew;
- initialize controller integrators;
- initialize estimator state/confidence;
- establish the initial modulation region;
- detect startup abnormality;
- handle pre-biased or reverse-powered terminals.

The objective is safe, bounded energization according to project limits. It does not need to reproduce the exact vendor transient waveform.

## REGULATION

Closed-loop control is active only here. The Power Manager retains authority over:

- controller enable;
- reference limits;
- allowed direction;
- shutdown request;
- fault transitions;
- controller-change qualification;
- behavior when estimator confidence degrades.

Operating-region selection itself belongs to modulation, not the Power Manager, unless a region becomes forbidden by safety policy.

## SHUTDOWN

Normal shutdown is deterministic and distinct from emergency fault suppression. Depending on direction and stored energy it may include:

- controlled reference ramp-down;
- bounded current reduction;
- transition toward zero energy flow;
- controller disable;
- deterministic PWM stop;
- confirmation of a safe inactive state.

A fault requiring immediate suppression may bypass the controlled ramp.

## FAULT

Fault detection and recovery policy are separate concerns. On fault:

```text
fast source if available
      ↓
immediate PWM/HRTIM suppression
      ↓
software fault capture
      ↓
FAULT state
      ↓
policy: latch or retry
```

Fault metadata should retain enough information for post-event analysis.

## RETRY_WAIT

Only explicitly recoverable faults may enter retry. Retry policy should define:

- delay;
- optional retry count/backoff;
- conditions that escalate to latched `FAULT`;
- controller/estimator state reset;
- requirement to return through `QUALIFY`.

No vendor auto-retry behavior is copied unless it is deliberately adopted and documented.

## Fault Classes

Potential classes include:

```text
overcurrent / short circuit
Port A or Port B overvoltage
undervoltage / invalid supply condition
ADC saturation or sensing implausibility
HRTIM/timing/configuration fault
estimator invalid when required by active controller
unrealizable modulation request
host/session policy violation
internal deadline/health fault
```

Thresholds and response class belong to versioned configuration and test evidence, not undocumented magic constants.

## Protection Hierarchy

```text
1. hardware-immediate / HRTIM fault suppression
2. modulation hard bounds and illegal-state prevention
3. Power Manager qualification/state limits
4. controller constraints and anti-windup
```

Higher-numbered layers may not weaken lower-numbered protection.

## Host Ownership

Host commands are requests:

```text
OUTPUT_ENABLE  -> request OFF -> QUALIFY
OUTPUT_DISABLE -> request controlled SHUTDOWN
CLEAR_FAULT    -> request policy evaluation
```

No normal protocol command directly asserts PWM enable, writes gate states, disables mandatory protection, or jumps to `REGULATION`.

## Controller / Estimator State Transfer

Changing controller or recovering from fault may involve:

- PI/LQI integrator state;
- observer state/covariance;
- filter state;
- previous `vL*`, `d1`, `d2`;
- reference-ramp state.

Every change must either transfer compatible state explicitly or pass through a defined reinitialization transition. Hidden state discontinuities are not acceptable.

## Fault Logging

Record at least when available:

```text
timestamp/sample index
power state
fault bits and source
Vin / Iin / Vout / Iout
iL_hat + confidence
power-flow direction
operating region
vL*
d1 / d2
references
controller type
retry count
```

## Minimum Prerequisite Before Energized Closed Loop

Before Phase 3 energized testing, the implementation must already provide:

- stable `OFF` state;
- qualified enable path;
- safe HRTIM inactive/disable behavior;
- hard duty/pulse constraints;
- at least the required immediate fault suppression path;
- controlled shutdown;
- telemetry indicating state/fault reason.

Advanced controllers are never used as a substitute for these prerequisites.

## Validation Scope

Test the independent implementation delta:

- reset to OFF remains inactive;
- invalid enable stays out of switching;
- valid enable follows QUALIFY -> SOFT_START -> REGULATION;
- disable follows SHUTDOWN -> OFF;
- fault overrides controller/host authority;
- retry returns through QUALIFY;
- latched fault remains latched;
- controller/estimator state is reinitialized deterministically;
- reverse/pre-biased cases cannot bypass qualification.

Do not re-run the vendor’s complete protection qualification merely to prove the board can protect itself under vendor firmware.
