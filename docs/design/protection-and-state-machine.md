# Protection and State Machine

## Purpose

This document defines the supervisory behavior that surrounds all control algorithms.

The converter must not transition directly from reset into closed-loop regulation. Startup, PWM qualification, soft-start, regulation, shutdown, and fault recovery are explicit states with deterministic entry and exit conditions.

## Supervisory Principle

The control law determines the requested actuation only while the supervisor allows power-stage operation.

Conceptually:

```text
Measurements / faults
        ↓
Supervisor
        ↓
Controller enable + limits
        ↓
Controller
        ↓
Modulation
        ↓
PWM / gate driver
```

A controller cannot override protection or state-machine decisions.

## Proposed State Model

The initial state machine should contain at least:

```text
RESET
  ↓
IDLE
  ↓
QUALIFY
  ↓
SOFT_START
  ↓
REGULATION
  ↓
SHUTDOWN
```

with fault transitions available from every state in which the power stage can become energized:

```text
FAULT_LATCHED
FAULT_RETRY_WAIT
```

The exact set of states may evolve after the reference behavior is fully characterized.

## RESET

Responsibilities:

- initialize clocks and peripherals;
- force PWM outputs into a known inactive state;
- initialize ADC scaling and calibration data;
- clear software controller state;
- initialize fault inputs and status reporting.

No power-stage switching is allowed until the required hardware and measurement paths are known to be initialized.

## IDLE

The converter is initialized but not switching.

The supervisor may continue to monitor:

- input voltage;
- output voltage;
- measured current;
- requested setpoint;
- fault inputs;
- user enable request.

## QUALIFY

Before energizing the bridge, the supervisor verifies that startup conditions are acceptable.

Candidate qualification checks include:

- supply voltage inside the allowed operating range;
- no active overcurrent / overvoltage condition;
- valid ADC measurements;
- valid requested operating direction;
- valid voltage target;
- PWM subsystem ready;
- power-stage state consistent with the requested direction.

Thresholds that are not yet independently verified should remain explicit configuration or characterization items rather than copied blindly from a reference implementation.

## SOFT_START

Soft-start is an explicit controlled transition, not simply a delay before regulation.

Possible responsibilities include:

- ramping voltage or current reference;
- limiting duty slew;
- limiting current;
- initializing PI/PID integrators;
- initializing observer or estimator states;
- selecting the initial operating region;
- detecting abnormal startup behavior.

The reference system exhibits controlled startup behavior without large output overshoot; the independent implementation should reproduce and quantify this behavior before more advanced controllers are introduced.

## REGULATION

Closed-loop control is active only in this state.

The supervisor still owns:

- operating-region selection;
- power-flow direction;
- duty and current limits;
- controller enable state;
- protection response;
- transition to shutdown or fault states.

## SHUTDOWN

Shutdown should be deterministic and should account for energy stored in the inductor and capacitors.

A shutdown strategy may include:

- controlled reference ramp-down where safe;
- controller disable;
- deterministic modulation stop;
- PWM output disable;
- verification that current has decayed appropriately.

Emergency fault shutdown is distinct from normal shutdown and may bypass the controlled ramp.

## Fault Categories

The architecture should distinguish fault *detection* from fault *policy*.

Potential fault classes include:

- overcurrent;
- output overvoltage;
- input undervoltage;
- input overvoltage;
- short circuit;
- invalid sensing;
- impossible operating request;
- timing / PWM configuration fault;
- hardware fault input.

Some faults may be recoverable while others should require manual reset.

## Fault Response

A fault response can contain several layers:

```text
hardware event
    ↓
immediate PWM suppression
    ↓
software fault capture
    ↓
state transition
    ↓
recovery policy
```

Where hardware can suppress PWM independently of the control loop, that mechanism should be preferred for faults requiring the fastest response.

## Retry Behavior

Reference measurements show repeated shutdown/restart behavior under persistent output short-circuit conditions, consistent with an automatic retry policy. This observation does not by itself prove the internal vendor algorithm.

The independent implementation should therefore define its own explicit retry policy, including:

- retry delay;
- maximum number of retries if applicable;
- re-qualification before restart;
- controller-state reset;
- event logging;
- conditions that escalate to a latched fault.

## State Transfer Between Controllers

The architecture is intended to support multiple controllers. Changing controller or operating region must not introduce hidden state discontinuities.

Relevant state may include:

- PI/PID integrator state;
- filtered measurements;
- observer state;
- Kalman state and covariance;
- previous duty commands;
- reference-ramp state.

Controller changes must either transfer compatible state explicitly or reinitialize it through a defined transition.

## Protection Hierarchy

A practical hierarchy is:

```text
Hardware-immediate protection
        ↓
PWM / modulation limits
        ↓
Supervisor limits and state machine
        ↓
Control-law constraints
```

Protection must not depend on the stability or correctness of a newly developed controller.

## Logging Requirements

Every fault or abnormal transition should eventually record enough context to reproduce the event:

- state before fault;
- fault source;
- `Vin`, `Vout`, `Iin`, `Iout`;
- operating region;
- power-flow direction;
- `D1`, `D2`;
- reference values;
- timestamp or sample index;
- retry count where applicable.

## Design Rules

1. PWM starts only through an explicit state transition.
2. Protection is independent of the selected control algorithm.
3. Fault detection and recovery policy are separate concerns.
4. Soft-start initializes controller state deliberately.
5. Mode or controller changes define state-transfer behavior.
6. Emergency shutdown and normal shutdown are distinct paths.
7. Fault thresholds and retry behavior are measurable configuration, not hidden magic constants.

## Validation Targets

The supervisor should eventually be tested for:

- power-on with valid and invalid input conditions;
- normal enable / disable cycles;
- repeated soft-start;
- load applied before startup;
- mode transition during regulation;
- overcurrent event;
- output short circuit;
- input undervoltage / overvoltage;
- output overvoltage;
- fault retry and latched-fault behavior;
- controller reset and state initialization after faults.

Each test should have explicit pass/fail criteria and associated measurement evidence.