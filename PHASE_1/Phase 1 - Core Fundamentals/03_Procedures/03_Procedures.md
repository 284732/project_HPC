# Procedures (functions) in C++

## Introduction

A **procedure** is a named block of code that performs a well-defined task and can be
called several times from different points in the program. Splitting a program into
procedures improves readability, avoids code duplication and lets you reason about a
problem one piece at a time.

In C++ all procedures are called **functions**: a function that does not need to return
anything has the return type `void`. The program's entry point itself, `main`, is a
function. Compared with C, C++ adds important tools: **references**, **overloading**
(several functions with the same name but different signatures) and **default arguments**.

## 1. Defining a function

The general form of a function is:

```cpp
return_type function_name(parameter_list)
{
    // function body
    return value;   // consistent with return_type
}
```

Example:

```cpp
double circle_area(double radius)
{
    const double PI = 3.14159265358979;
    return PI * radius * radius;
}
```

The `return` statement terminates the function and returns its value. A `void` function can
use `return;` with no value, or terminate by reaching the end of the body.

## 2. Declaration (prototype) and headers

In C++ a function must be **declared before it is used**. The declaration, called a
**prototype**, gives the name, return type and parameter types, without a body:

```cpp
double circle_area(double radius);   // prototype: note the trailing ';'
```

The prototype lets the compiler check that calls use the correct number and type of
arguments. In real projects prototypes are collected in **header files** (`.h`/`.hpp`),
while definitions go in source files (`.cpp`): this is the standard way to separate
interface from implementation.

## 3. Passing parameters: by value

As in C, by default parameters are **passed by value**: the function receives a *copy* of
the argument. Modifying the parameter inside the function has **no** effect on the caller's
variable.

```cpp
void test(int x) { x = 99; }   // modifies only the local copy
```

## 4. Passing by reference

C++ introduces **references** (`&`), which are the idiomatic way to let a function modify a
caller's variable, without the pointer syntax:

```cpp
void doubleValue(int& r)   // r is a reference (an alias) to the variable passed
{
    r = r * 2;
}
...
int a = 21;
doubleValue(a);            // no '&' at the call site: a becomes 42
```

A reference is an alias of the original variable: safer than a pointer (it cannot be null
or "moved"). To return multiple values you can use several output parameters by reference.
To avoid copying large objects **without** modifying them, a **constant reference**
`const T&` is used: efficient and safe, a fundamental idiom in HPC when passing bulky
containers.

The **pointer** form (`int*`) inherited from C is still available, useful when the argument
can be optional (`nullptr`) or when working with raw memory.

## 5. `std::vector` as a parameter

In C++ `std::vector` is preferred over C's raw arrays: it knows its own size and manages
memory automatically. Passing it by reference avoids the copy:

```cpp
double average(const std::vector<double>& v)   // const& : no copy, read-only
{
    double sum = 0.0;
    for (double x : v) sum += x;
    return sum / v.size();
}
```

If the function must **modify** the vector, it is passed by non-constant reference
(`std::vector<double>& v`). The fact that containers are passed by reference, without a
copy, is efficient and is one of the reasons C++ is well suited to numerical computing.

## 6. Overloading and default arguments

Two tools typical of C++, absent in C:

- **Overloading**: several functions with the same name but different parameters; the
  compiler picks the right one based on the arguments.

  ```cpp
  int    square(int x)    { return x * x; }
  double square(double x) { return x * x; }
  ```

- **Default arguments**: a parameter can have a default value, used if the caller omits it.

  ```cpp
  double power(double base, int exp = 2);   // exp is 2 if not specified
  ```

## 7. Recursion

A function can call itself: this is called **recursion**. Each recursive call must move
closer to a **base case** that stops the recursion, otherwise the program does not
terminate (and the stack space is exhausted).

```cpp
long factorial(int n)
{
    if (n <= 1) return 1;                  // base case
    return n * factorial(n - 1);           // recursive step
}
```

Recursion is elegant for recursively defined problems, but in high-performance contexts the
iterative version is often preferred, as it avoids the overhead of function calls and the
risk of stack overflow.

## 8. Scope and storage classes

**Scope** is the region of the code in which a name is visible:

- variables declared inside a function are **local**: they exist only while the function is
  running;
- variables declared outside any function are **global**: visible to the whole file (and to
  other files unless `static`).

The `static` keyword has two relevant uses:

- on a **local** variable, it makes its lifetime equal to the whole program: the value is
  preserved between calls (useful, for example, for a call counter);
- on a variable or function at file level, it limits its visibility to the current file.

There is also `const` (and `constexpr`, computed at compile time) to define constants
safely.

## 9. Example files in this folder

| File               | What it shows                                                     |
| ------------------ | ---------------------------------------------------------------- |
| `functions.cpp`    | definition and prototypes, overloading, default arguments, `void`|
| `pass_by_ref.cpp`  | passing by value, by reference (`&`), `const&` and `std::vector` |
| `recursion.cpp`    | recursion (factorial, Fibonacci) and comparison with iteration   |
| `scope_static.cpp` | local/global scope and `static` variables                        |

## 10. Summary

In C++ every procedure is a function, possibly `void`. Functions must be declared
(prototype) before use, typically through headers that separate interface and
implementation. Parameters are passed by value by default; C++ adds references (`&` and
`const&`) as a safe and efficient way to modify arguments or avoid copying them, in
addition to the pointer passing inherited from C. `std::vector` is preferred over raw
arrays. Overloading and default arguments make interfaces more flexible. Recursion is
possible but more expensive than iteration. Finally, scope and storage classes (`static`,
`const`/`constexpr`, local/global variables) govern the visibility and lifetime of
variables.
