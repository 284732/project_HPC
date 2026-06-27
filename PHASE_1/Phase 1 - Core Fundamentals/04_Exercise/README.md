# 04 - Putting It Together

This folder contains a single integrative example that combines the three Phase 1 topics
in one small, realistic program.

`integrate_config.cpp` estimates the integral of `f(x) = x^2` over an interval using the
**trapezoidal rule**, and in doing so it exercises:

- **I/O** — it reads the parameters (`a`, `b`, `n`) from a configuration file
  (`integration_config.txt`), creating the file with default values if it does not exist,
  and writes a short report to `integration_result.txt`.
- **Conditional and iterative constructs** — the accumulation of the trapezoidal sum is a
  `for` loop, with a validity check on `n`.
- **Procedures** — both the integrand and the integrator are functions, and the
  integrator receives the function to integrate as a parameter (a function pointer).

This mirrors, on a small serial scale, the structure of the project's case study: read
configuration, run a numerical computation in a loop, report the result together with its
error.

## Build and run

```bash
g++ -Wall -Wextra -std=c++17 integrate_config.cpp -o integrate_config
./integrate_config
```

On the first run the program creates `integration_config.txt`; you can then edit the
values of `a`, `b` and `n` and run it again to see how the result and the error change.
