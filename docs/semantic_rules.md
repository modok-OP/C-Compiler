# Semantic Rules & Diagnostic Catalogue

This document defines the semantic type rules, conversion constraints, scope rules, function rules, array rules, formatted I/O rules, and the complete diagnostic catalogue (`SEM-01` through `SEM-20`) for the C Compiler front end, as specified in the frozen **C Compiler Front-End Language & Semantic Specification (Version 1.0)**.

---

## 1. Type Conversion Rules

### 1.1 Implicit Conversion Matrix
The front end strictly enforces that only **widening numeric conversion from `int` to `float`** is implicitly allowed across different value types.

| Source Type | Target Type | Implicitly Allowed? | Reason / Constraint |
| :--- | :--- | :---: | :--- |
| `int` | `int` | **Yes** | Identical type |
| `int` | `float` | **Yes** | Safe widening numeric promotion |
| `int` | `char` | **No** | Cross-category conversion requires explicit cast (`SEM-04`) |
| `float` | `int` | **No** | Potential narrowing conversion requires explicit cast (`SEM-04`) |
| `float` | `float` | **Yes** | Identical type |
| `float` | `char` | **No** | Cross-category conversion requires explicit cast (`SEM-04`) |
| `char` | `int` | **No** | `char` is kept distinct in front-end rules without implicit promotion |
| `char` | `float` | **No** | `char` is kept distinct without implicit conversion |
| `char` | `char` | **Yes** | Identical type |
| `void` | Any | **No** | `void` does not produce a value |

### 1.2 Explicit Cast Matrix
Programmers may explicitly request conversions using C-style cast syntax `(type)operand` between numeric and character types:

| Source Type | Target Type | Cast Legal? | Result Type |
| :--- | :--- | :---: | :--- |
| `int`, `float`, `char` | `int` | **Yes** | `int` |
| `int`, `float`, `char` | `float` | **Yes** | `float` |
| `int`, `float`, `char` | `char` | **Yes** | `char` |
| `void` | Any | **No** | `SEM-19`: Unsupported cast form |
| Any | `void` | **No** | `SEM-19`: Unsupported cast form |

---

## 2. Expression Type Rules

### 2.1 Arithmetic Operators (`+`, `-`, `*`, `/`)
- Operands may be `int` or `float`.
- If either operand is `float`, the result is `float`.
- If both operands are `int`, the result is `int`.
- `char` operands are **not** implicitly promoted to `int` for arithmetic; using `char` in arithmetic without an explicit cast is invalid (`SEM-05`).

### 2.2 Modulus Operator (`%`)
- Both operands must have type `int`.
- Non-integer operands (e.g. `10.5 % 2` or `x % 2.0`) trigger `SEM-06`.

### 2.3 Increment and Decrement (`++`, `--`)
- Both prefix (`++x`, `--x`) and postfix (`x++`, `x--`) forms are supported.
- Operand must be an lvalue (modifiable variable or array element) of type `int` or `float`.
- Incrementing/decrementing `char` or non-lvalues triggers `SEM-05`.

### 2.4 Relational and Equality Operators (`<`, `<=`, `>`, `>=`, `==`, `!=`)
- Permitted operand pairs: `int`-`int`, `int`-`float`, `float`-`int`, `float`-`float`, and `char`-`char`.
- Comparison produces an internal `boolean` type (`SEM_TYPE_BOOL`).
- Cross-type comparisons between `char` and numeric types are rejected.

### 2.5 Logical Operators (`&&`, `||`, `!`)
- Operands to `&&`, `||`, and `!` **must** be boolean expressions produced by relational, equality, or logical operations.
- General C-style numeric "truthiness" (e.g. `if (x)`) is rejected with `SEM-07` or `SEM-08`.

### 2.6 Assignment Operators (`=`, `+=`, `-=`, `*=`, `/=`, `%=`)
- Left-hand side must be an lvalue (`AST_IDENTIFIER` or `AST_ARRAY_ACCESS`).
- Right-hand side type must be implicitly convertible to left-hand side type.
- Compound assignments evaluate the binary operation first and verify compatibility with the left-hand type.

---

## 3. Scope and Symbol-Table Rules

- **Declaration Before Use**: An identifier must be declared in the current or an enclosing ancestor scope before it can be referenced.
- **No Duplicate in Same Scope**: Declaring the same identifier name more than once within the same lexical scope triggers `SEM-02`.
- **Lexical Shadowing**: An inner block or function scope may declare an identifier with the same name as an outer-scope identifier; references inside the inner block resolve to the innermost declaration.
- **Out-of-Scope Detection**: If an identifier was declared in an inner block that has already terminated, referencing it produces `SEM-03` rather than generic `SEM-01`.
- **Predefined Symbols**: `printf` and `scanf` are pre-inserted into the global scope.

---

## 4. Function & Program Structure Rules

- **Entry Point**: The program must contain exactly one entry point declared as `int main()` without parameters (`SEM-20`).
- **Signature Matching**:
  - Function calls must provide exactly the number of arguments declared in the function's parameter list (`SEM-09`).
  - Argument types must be implicitly convertible to the corresponding declared parameter types (`SEM-10`).
- **Return Requirements**:
  - Functions declared with a non-void return type (`int`, `float`, `char`) must contain at least one compatible return statement (`SEM-12`).
  - Returning a value incompatible with the function's declared return type triggers `SEM-11`.
  - Functions declared `void` cannot return an expression value (`SEM-11`).

---

## 5. Array Rules

- **Dimension**: Only one-dimensional arrays are supported. Array dimension in a declaration must be a positive integer literal.
- **Index Type**: Array index expressions must evaluate to `int`. Non-integer indices (e.g. `arr[2.5]`) trigger `SEM-15`.
- **Element Assignment**: Assigning a value incompatible with the array element type triggers `SEM-16`.

---

## 6. Formatted I/O Rules (`printf` and `scanf`)

### 6.1 `printf`
- First argument must be a string literal.
- Supported format specifiers:
  - `%d`: requires `int` argument.
  - `%f`: requires `float` argument.
  - `%c`: requires `char` argument.
  - `%s`: requires string literal argument.
  - `%%`: escaped literal `%`, takes no arguments.
- The number of arguments following the format string must match the number of format specifiers (`SEM-17`).
- Implicit conversions are **not** performed in `printf` arguments; passing a `float` to `%d` or an `int` to `%f` triggers `SEM-17`.
- Unsupported format specifiers (e.g. `%x`, `%u`) trigger `SEM-17`.

### 6.2 `scanf`
- First argument must be a string literal.
- Supported format specifiers: `%d` (`int`), `%f` (`float`), `%c` (`char`). `%s` is **strictly unsupported** in `scanf` (`SEM-18`).
- Destinations must be provided using the explicit address-of syntax: `&identifier`.
- Target identifier must resolve to a modifiable variable of matching type (`%d` $\rightarrow$ `int`, `%f` $\rightarrow$ `float`, `%c` $\rightarrow$ `char`).
- Passing non-lvalues, expressions (e.g. `&(x + 1)`), array elements, or missing `&` triggers `SEM-18`.

### 6.3 Ampersand (`&`) Restriction
- The address-of operator `&` is recognized **exclusively** as destination syntax inside `scanf` calls.
- Using `&` in any other expression (e.g. `x = &y;`, `foo(&x);`) triggers `SEM-18`.

---

## 7. Semantic Diagnostic Catalogue (`SEM-01` ... `SEM-20`)

| Code | Category | Frozen Specification Example | Description / Constraint |
| :--- | :--- | :--- | :--- |
| **`SEM-01`** | Undeclared Identifier | `x = 10;` | Identifier is not declared in any accessible scope |
| **`SEM-02`** | Duplicate Declaration | `int x; int x;` | Identifier already declared in the same scope |
| **`SEM-03`** | Out of Scope | `use y after inner block ends` | Identifier was declared in an inner block that has terminated |
| **`SEM-04`** | Assignment Mismatch | `int x = 3.14;` | Right-hand type cannot implicitly convert to left-hand variable type |
| **`SEM-05`** | Invalid Arithmetic Operands | `char + int` | Operands incompatible with arithmetic operation (e.g. `char` without cast) |
| **`SEM-06`** | Invalid Modulus | `10.5 % 2` | Modulus operator requires integer operands |
| **`SEM-07`** | Invalid Condition | `if (x + 1)` | Loop or if condition must evaluate to a boolean comparison result |
| **`SEM-08`** | Invalid Logical Operand | `x && y` | Logical operators (`&&`, `\|\|`, `!`) require boolean operands |
| **`SEM-09`** | Wrong Function Arity | `add(1)` | Call argument count does not match function parameter count |
| **`SEM-10`** | Wrong Argument Type | `int func called with float` | Argument cannot implicitly convert to parameter type |
| **`SEM-11`** | Invalid Return Type | `int f() { return 3.14; }` | Return expression is incompatible with function return type |
| **`SEM-12`** | Missing Return | `non-void f() without return` | Non-void function missing a valid return statement |
| **`SEM-13`** | Break Outside Loop | `break; in main body` | `break` statement located outside any loop body |
| **`SEM-14`** | Continue Outside Loop | `continue; outside loop` | `continue` statement located outside any loop body |
| **`SEM-15`** | Invalid Array Index | `a[2.5]` | Array index expression must evaluate to integer |
| **`SEM-16`** | Array Assignment Mismatch | `int a[5]; a[0] = 3.14;` | Assigned value incompatible with array element type |
| **`SEM-17`** | Printf Format Mismatch | `printf("%d", 3.14);` | Format specifier count, type, or unsupported specifier error in `printf` |
| **`SEM-18`** | Scanf Format Mismatch | `scanf("%d", &f);` | Destination syntax, count, type mismatch, or invalid `&` usage |
| **`SEM-19`** | Invalid Cast | `(int)void_expr` | Explicit cast between unsupported type combinations |
| **`SEM-20`** | Invalid Main | `void main()` | Missing `main` or main signature is not `int main()` |
