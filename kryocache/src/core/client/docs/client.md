# `snprintf(3)` 

### 🚨 Critical Security Points

#### 1. **`sprintf()` and `vsprintf()` are Deadly Dangerous**
*   **The Problem:** These functions **do not check** the length of the `str` buffer. They write data until a terminating null character (`'\0'`) is placed in the formatted string. If the result does not fit, a **buffer overflow** occurs.
*   **Quote from "Bugs":** *"Note that the length of the strings produced is locale-dependent and difficult to predict. Use `snprintf()` and `vsnprintf()` instead."*
*   **Recommendation:** **NEVER** use `sprintf()` or `vsprintf()`. Always replace them with `snprintf()`/`vsnprintf()`.

#### 2. **`snprintf()` with Early libc (libc4) — False Security**
*   **The Problem:** In old library versions (Linux libc4.[45]), `snprintf()` **ignored** the `size` parameter, effectively turning into the dangerous `sprintf()`.
*   **Quote from "Bugs":** *"...provides a libbsd that contains an `snprintf()` equivalent to `sprintf()`, that is, one that ignores the size argument. Thus, the use of `snprintf()` with early libc4 leads to serious security problems."*
*   **Recommendation:** For modern code, this is a historical issue, but it's important to remember when porting or working with embedded systems.

#### 3. **Overlapping Buffers — Undefined Behavior (UB)**
*   **The Problem:** If the destination buffer (`str`) and one of the source arguments refer to the same memory region, the behavior of `sprintf()`, `snprintf()`, `vsprintf()`, and `vsnprintf()` is **undefined by the standard**.
*   **Quote from "Description" and "Notes":** *"the results are undefined if a call to... would cause copying to take place between objects that overlap."* An example of dangerous code is given: `sprintf(buf, "%s some further text", buf);`
*   **Recommendation:** Never allow the destination buffer and arguments to overlap. Code attempting to "append to the same string" in this way is **incorrect**.

#### 4. **Format String (`format`) from Untrusted Source — Format String Vulnerability**
*   **The Problem:** If a user can control the format string (`format`), they can use specifiers (`%x`, `%s`) to read memory or **`%n` to write to memory**.
*   **Quote from "Bugs":** *"Code such as `printf(foo);` often indicates a bug, since foo may contain a % character. If foo comes from untrusted user input, it may contain `%n`, causing the `printf()` call to write to memory and creating a security hole."*
*   **Recommendation:** **Never pass** an unchecked or user-provided string as the `format` argument. If you simply need to print a string, use `fputs()` or `%s` with a fixed format string: `printf("%s", user_string);`.

#### 5. **Misinterpreting the `snprintf()` Return Value**
*   **The Problem:** The `snprintf()` function returns **the number of characters that *would have been* written** given a sufficiently large buffer (excluding the terminating null). If this value is **greater than or equal to `size`**, then **truncation** occurred. It is an error to assume truncation returns `-1` (that only happened in very old glibc).
*   **Quote from "Return value":** *"a return value of `size` or more means that the output was truncated."*
*   **Quote from "Notes":** *"Until glibc 2.0.6 they would return -1 when the output was truncated."*
*   **Recommendation:** Always check the return value for `>= size`. If truncation is unacceptable, increase the buffer and retry. A correct pattern is provided in the "Example" section of the man page.

### ✅ Final Security Rules

1.  **Use only `snprintf()` and `vsnprintf()`** for writing to character buffers.
2.  **Always check the return value** of these functions for truncation.
3.  **Allocate a buffer of sufficient size** with margin, remembering locale dependencies (thousands separators, comma vs. point for decimals).
4.  **Never allow the format string to be user-controlled.** Use `"%s"` for outputting untrusted data.
5.  **Ensure the destination buffer and arguments do not overlap.**
6.  For dynamic allocation of correctly-sized strings, consider using `asprintf()` (non-standard but convenient), following the safe pattern from the man page's "Example".

