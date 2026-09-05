# Modulation and Operating Regions

## Purpose

This document defines the control-allocation boundary between controller output `vL*` and physical four-switch duty commands.

Buck-like, Mixed-like, and Boost-like labels describe operating points and switching allocation. They are not separate controller identities.

## Physical mapping and duties

```text
left bridge  : Q1 high / Q4 low
right bridge : Q2 high / Q3 low

d1 = Q1 left high-side average duty
d2 = Q3 right low-side average duty
```

The ideal averaged inductor voltage is:

```text
vL = d1 Vin - (1 - d2) Vout
```

## Effective-duty coordinates

```text
e1 = d1
e2 = 1 - d2
```

so:

```text
vL = Vin e1 - Vout e2
```

Vector form:

```text
a = [ Vin, -Vout ]^T
e = [ e1,  e2   ]^T

a^T e = vL
```

The controller requests:

```text
vL*
```

and does not select a Buck/Mixed/Boost controller mode or raw HRTIM compare value.

## Feasible set

Hardware constraints define a box:

```text
e1_min <= e1 <= e1_max
e2_min <= e2 <= e2_max
```

For positive `Vin` and `Vout`, the realizable average-inductor-voltage interval is:

```text
vL_min = Vin * e1_min - Vout * e2_max
vL_max = Vin * e1_max - Vout * e2_min
```

The requested actuation is clamped to this interval before duty allocation.

## Continuous constrained allocator

For a realizable `vL*`, valid duty pairs lie on:

```text
Vin e1 - Vout e2 = vL*
```

The allocator selects the point on the feasible line segment closest to the previous command:

```text
minimize
    (e1 - e1_prev)^2 + (e2 - e2_prev)^2

subject to
    Vin e1 - Vout e2 = vL*
    e1_min <= e1 <= e1_max
    e2_min <= e2 <= e2_max
```

The unconstrained orthogonal projection is:

```text
e_proj = e_prev
       + ((vL* - a^T e_prev) / (a^T a)) * a
```

If `e_proj` lies outside the feasible line segment, the allocator selects the nearest segment endpoint.

Output conversion is:

```text
d1 = e1
d2 = 1 - e2
```

This produces a continuous constant-time control allocation without explicit Buck/Mixed/Boost branching in the core control path.

## Null-space interpretation

A direction that does not change ideal average inductor voltage is:

```text
u = [ Vout, Vin ]^T
```

because:

```text
[ Vin, -Vout ] · [ Vout, Vin ] = 0
```

Movement along this direction redistributes switching effort while preserving the requested `vL*`. The defined allocator uses this redundancy to minimize command movement while satisfying hard constraints.

## Realized actuation

The estimator and anti-windup logic use:

```text
vL_realized = Vin e1 - Vout e2
```

not the unconstrained controller request. Saturation metadata accompanies the allocator output.

## Hardware constraints

The modulation layer owns:

```text
duty bounds
minimum pulse width
minimum off-time
commanded/effective dead-time limits
bootstrap refresh
complementary-output legality
fault-forced inactive state
```

A mathematically valid duty pair is invalid if it violates any physical timing constraint.

## Operating-region interpretation

```text
Buck-like  : allocation lies near right-leg pass-through boundary
Mixed-like : both effective duties materially participate
Boost-like : allocation lies near left-leg pass-through boundary
```

These labels are telemetry/analysis descriptions derived from the allocation result. They do not change controller identity, physical port identity, ADC mapping, or timer ownership.

## Bidirectional operation

The same equations and allocator apply for both energy-flow directions. Direction is represented by signed states and references; hardware channels are never remapped.

## Control envelope

The canonical allocator/control model is CCM-oriented. DCM/light-load behavior and zero-current operation are outside the accepted CCM model unless explicitly covered by a controller-specific envelope.

## HRTIM boundary

```text
vL*
 ↓
feasibility clamp
 ↓
continuous e1/e2 allocation
 ↓
d1 / d2 + saturation metadata
 ↓
platform HRTIM API
```

Only the platform layer converts logical duties into timer compare values, complementary outputs, and dead-time configuration.