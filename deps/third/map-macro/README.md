# The Map Macro

This repository implements a `MAP` macro, which can be used as follows:

```c
#define PRINT(a) printf(#a": %d", a); /* An example macro */
MAP(PRINT, a, b, c) /* Apply PRINT to a, b, and c */
```

This macro came about in answer to a [Stack Overflow question.](http://stackoverflow.com/questions/6707148/foreach-macro-on-macros-arguments/13459454#13459454).
The original answer can be found in the [`stackoverflow` branch](https://github.com/swansontec/map-macro/tree/stackoverflow).
The `master` branch contains a few extra enhancements.
Pull requests are welcome.

## How it Works

The goal is to create a macro which performs some operation each element of
a list. Doing that requires recursion, though, which the C preprocessor
doesn't allow. Fortunately, there is a workaround.

### Basic Recursion

First, we need a technique for emitting something that looks like a macro
call, but isn't yet:

    #define MAP_OUT

Imagine we have the following macros:

    #define A(x) x B MAP_OUT (x)
    #define B(x) x A MAP_OUT (x)

Evaluating the macro `A (blah)` produces the output text:

    blah B (blah)

The preprocessor doesn't see any recursion, since the `B (blah)` call is
just plain text at this point, and `B` isn't even the name of the current
macro. Feeding this text back into the preprocessor expands the call,
producing the output:

    blah blah A (blah)

Evaluating the output a third time expands the `A (blah)` macro, carrying
the recursion full-circle. The recursion continues as long as the caller
continues to feed the output text back into the preprocessor.

To perform these repeated evaluations, the following `EVAL` macro passes
its arguments down a tree of macro calls:

    #define EVAL0(...) __VA_ARGS__
    #define EVAL1(...) EVAL0 (EVAL0 (EVAL0 (__VA_ARGS__)))
    #define EVAL2(...) EVAL1 (EVAL1 (EVAL1 (__VA_ARGS__)))
    #define EVAL3(...) EVAL2 (EVAL2 (EVAL2 (__VA_ARGS__)))
    #define EVAL4(...) EVAL3 (EVAL3 (EVAL3 (__VA_ARGS__)))
    #define EVAL(...)  EVAL4 (EVAL4 (EVAL4 (__VA_ARGS__)))

Each level multiplies the effort of the level before, evaluating the input
365 times in total. In other words, calling `EVAL (A (blah))` would
produce 365 copies of the word `blah`, followed by a final un-evaluated `B
(blah)`. This provides the basic framework for recursion, at least within a
certain stack depth.

### End Detection

The next challenge is to stop the recursion when it reaches the end of the
list.

The basic idea is to emit the following macro name instead of the normal
recursive macro when the time comes to quit:

    #define MAP_END(...)

Evaluating this macro does nothing, which ends the recursion.

To actually select between the two macros, the following `MAP_NEXT`
macro compares a single list item against the special end-of-list marker
`()`. The macro returns `MAP_END` if the test item matches, or the `next`
parameter if the item is anything else:

    #define MAP_GET_END() 0, MAP_END
    #define MAP_NEXT0(test, next, ...) next MAP_OUT
    #define MAP_NEXT1(test, next) MAP_NEXT0 (test, next, 0)
    #define MAP_NEXT(test, next)  MAP_NEXT1 (MAP_GET_END test, next)

This macro works by placing the test item next to the `MAP_GET_END` macro.
If doing that forms a macro call, everything moves over by a slot in the
`MAP_NEXT0` parameter list, changing the output. The `MAP_OUT` trick
prevents the preprocessor from evaluating the final result.

### Putting it All Together

With these pieces in place, it is now possible to implement useful versions
of the `A` and `B` macros from the example above:

    #define MAP0(f, x, peek, ...) f(x) MAP_NEXT (peek, MAP1) (f, peek, __VA_ARGS__)
    #define MAP1(f, x, peek, ...) f(x) MAP_NEXT (peek, MAP0) (f, peek, __VA_ARGS__)

These macros apply the operation `f` to the current list item `x`. They then
examine the next list item, `peek`, to see if they should continue or not.

The final step is to tie everything together in a top-level `MAP` macro:

    #define MAP(f, ...) EVAL (MAP1 (f, __VA_ARGS__, (), 0))

This macro places a `()` marker on the end of the list, as well as an extra
`0` for ANSI compliance (otherwise, the last iteration would have an illegal
0-length list). It then passes the whole thing through `EVAL` and
returns the result.

### Evaluation Depth

Each level of the `EVAL` macro multiplies the effort of the previous
level by 3, but also adds one evaluation of its own. Invoking the macro as a
whole adds one more level, taking the total to:

    1 + (3 * (3 * (3 * (3 * (3 * (1) + 1) + 1) + 1) + 1) + 1) = 365

Other interesting combinations include:

    calls/level levels  total depth
    2           3       16
    2           4       32
    2           5       64
    3           3       41
    3           4       122
    3           5       365
    4           3       86
    4           4       342
    4           5       1366
    5           2       32
    5           3       157
    5           4       782

Special thanks to [pfultz2](https://github.com/pfultz2/Cloak/wiki/Is-the-C-preprocessor-Turing-complete%3F) for inventing the EVAL idea.

## See Also

C++ users may be intersted in the ['visit_struct' library](https://github.com/cbeck88/visit_struct),
which uses a version of this macro to implement structure visitors in C++11.
