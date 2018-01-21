# albion
A small dynamic programming language

## Build instructions

```
make release
```

## Examples

### Hello world

```
> "Hello, world!".print;
Hello, world!
```

Functions are called with the call operator '.'

### Types

Basic types are:
* nil (`nil`)
* double (`42`)
* bool (`true`, `false`)
* string (`"string"`)
* tuple (`1, "str", true`)
* function (`fun input { body; }`)

### Functions

Functions are first class objects.

```
var f = fun x {
    if (x < 0) {
        return "negative";
    } else if (x == 0) {
        return "zero";
    } else {
        return "positive";
    }
};
```

Functions can be called with either the high-precedence `.` operator or the low-precedence `->` operator:

```
> (1, 2, 3).print;
(1.000000, 2.000000, 3.000000)
> 1, 2, 3 -> print;
(1.000000, 2.000000, 3.000000)
```

### Control flow

```
if (condition) { body; }
while (condition) { body; }
for (var i; i < 10; i = i + 1) { body; }
```

### Operators

High to low precedence:
* unary `.`
* `.`
* unary `!`, unary `-`
* `*`, `/`
* `+`, `-`
* `and`
* `or`
* `,`
* `->`
