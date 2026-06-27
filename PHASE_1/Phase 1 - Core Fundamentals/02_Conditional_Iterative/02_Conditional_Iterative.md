# Conditional and iterative constructs in C++

## Introduction

**Control flow** determines which instructions are executed and how many times they are
repeated. Without it, a program would just be a linear sequence of instructions. In C++,
control flow falls into two large families:

- **conditional constructs** (or selection), which execute different blocks depending on a
  logical condition: `if`/`else`, the ternary operator `?:` and `switch`;
- **iterative constructs** (or loops), which repeat a block while a condition stays true:
  `for`, range-based `for`, `while`, `do-while`, together with the `break` and `continue`
  statements.

Unlike the original C, C++ has a **native boolean type** `bool`, with the values `true`
and `false`. Conditions may still use numeric expressions or pointers, which are implicitly
converted to `bool`: the value `0` (or `nullptr`) is equivalent to `false`, any other value
to `true`.

## 1. Operators used in conditions

Conditions are built with **relational** and **logical** operators:

| Category   | Operators                                     |
| ---------- | --------------------------------------------- |
| relational | `==`  `!=`  `<`  `>`  `<=`  `>=`               |
| logical    | `&&` (AND), `||` (OR), `!` (NOT)              |

A classic mistake is confusing the assignment `=` with the comparison `==`. Writing
`if (x = 5)` assigns 5 to `x` and is always true: modern compilers warn about this, which
is why it is best to always compile with `-Wall`.

The `&&` and `||` operators use **short-circuit evaluation**: in `a && b`, if `a` is false
`b` is not even evaluated; in `a || b`, if `a` is true `b` is skipped. This allows safe
idioms such as `if (p != nullptr && p->value > 0)`.

## 2. `if`, `else if`, `else`

The fundamental selection construct:

```cpp
if (condition) {
    // executed if condition is true
} else if (other_condition) {
    // executed if the first is false and this is true
} else {
    // executed if all the previous ones are false
}
```

Conditions are evaluated **in order**: as soon as one is true, its block is executed and
the other branches are skipped. Since C++17 there is also the form with an initializer,
`if (auto it = ...; condition)`, which limits the scope of a variable to the `if` only.

## 3. The ternary operator `?:`

The ternary operator is a compact form of `if/else` that returns a **value**:

```cpp
int maximum = (a > b) ? a : b;
```

it is equivalent to "if `a > b` then `a` else `b`". It is handy for short conditional
assignments, but should be used sparingly so as not to harm readability.

## 4. `switch`

When you have to choose among many discrete values of the same **integer** (or enumerated)
expression, `switch` is clearer than a long chain of `else if`:

```cpp
switch (expression) {
    case 1:
        // ...
        break;        // leaves the switch
    case 2:
        // ...
        break;
    default:
        // executed if no case matches
        break;
}
```

A crucial point: without the `break` statement, execution **continues into the next case**
(behaviour called *fall-through*). Sometimes this is intentional (several labels sharing
the same code), but more often a missing `break` is a bug; in C++17 an intentional
fall-through can be marked with the `[[fallthrough]];` attribute.

## 5. `for` loops

The `for` loop is the most used form when the number of iterations is known or tied to a
counter. It has three parts separated by semicolons:

```cpp
for (int i = 0; i < 10; ++i) {
    std::cout << i << " ";
}
```

The initialization is executed only once; the condition is checked **before** each
iteration; the update is executed **after** each iteration. In HPC the `for` loop over
indices is the "workhorse" of numerical computing (iterating over vectors and matrices):
its predictable structure allows the compiler optimizations such as *unrolling* and
vectorization.

### Range-based `for` (C++11)

To iterate over a whole container without manually managing indices, C++ offers the
range-based `for`:

```cpp
std::vector<int> v = {10, 20, 30};
for (int x : v) {           // a copy of each element
    std::cout << x << " ";
}
for (int& x : v) {          // reference: allows modifying the elements
    x *= 2;
}
```

It is more concise and less prone to index errors than the classic `for`.

## 6. `while` and `do-while` loops

The `while` loop checks the condition **before** each iteration: if it is already false at
the start, the body is not executed even once.

```cpp
while (condition) {
    // body
}
```

The `do-while` loop checks the condition **after** the body, thus guaranteeing **at least
one** execution:

```cpp
do {
    // body executed at least once
} while (condition);
```

`do-while` is useful, for example, to repeat an input request until it is valid, or in
iterative methods that continue until a required precision is reached.

## 7. `break`, `continue`

Inside loops:

- `break` immediately interrupts the loop (it exits the innermost loop);
- `continue` skips the rest of the current iteration and moves to the next one.

There is also `goto`, inherited from C, but in structured programming its use is
discouraged: in C++ it can almost always be replaced with clearer constructs.

## 8. `switch` with enumerations

`switch` pairs naturally with **enumerated types**, which give names to a small set of
discrete values instead of using "magic numbers". The modern form is the strongly typed
`enum class` (C++11):

```cpp
enum class State { Idle, Running, Paused, Stopped };

switch (s) {
    case State::Idle:    /* ... */ break;
    case State::Running: /* ... */ break;
    // ...
}
```

This makes the code more readable and lets the compiler warn if a case is forgotten. The
example `enums_switch.cpp` uses this to implement a small state machine.

## 9. Loops and iterative numerical methods

Loops are the backbone of **iterative numerical methods**, where a computation is repeated
until the result is accurate enough. A typical pattern is a `while` loop that continues as
long as the error is above a tolerance, with a safety cap on the number of iterations to
avoid an infinite loop:

```cpp
while (std::fabs(x * x - a) > tol) {
    x = 0.5 * (x + a / x);     // Newton step for sqrt(a)
    ++iters;
    if (iters >= max_iters) break;
}
```

This is shown in `newton_iteration.cpp`, which computes a square root with the
Newton-Raphson method. The same idea — iterate until convergence — underlies many HPC
algorithms (linear solvers, optimization, simulations).

## 10. Example files in this folder

| File                  | What it shows                                                  |
| --------------------- | ------------------------------------------------------------- |
| `conditionals.cpp`    | `if`/`else if`/`else`, the ternary operator, `switch`         |
| `loops.cpp`           | `for`, range-based `for`, `while`, `do-while` loops           |
| `break_continue.cpp`  | use of `break` and `continue`, parameter-sweep example        |
| `enums_switch.cpp`    | `switch` with an `enum class` (a small state machine)         |
| `newton_iteration.cpp`| iterative method with convergence (Newton-Raphson square root)|

## 11. Summary

Control flow in C++ is based on conditional constructs (`if`/`else`, `?:`, `switch`) and
iterative ones (`for`, range-based `for`, `while`, `do-while`). C++ has a native `bool`
type, but conditions also accept numeric expressions and pointers that are implicitly
converted. The logical operators `&&` and `||` are short-circuit evaluated. `switch` is
suited to selection over discrete integer/enum values, but requires care with `break`.
The `for` loops over indices are central to numerical computing and are the ones the
compiler optimizes most effectively; the range-based `for` makes container traversal
safer; `do-while` guarantees at least one iteration.
