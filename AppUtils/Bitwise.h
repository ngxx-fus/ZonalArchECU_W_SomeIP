#ifndef __BITWISE_H__
#define __BITWISE_H__


/// @brief Perform bitwise OR on up to 10 arguments (Compile-time)
/// @param ... List of values to OR
/// @return Result of ((a) | (b) | (c) | ...)
#define OR_ALL(...) _OR_GET_MACRO(__VA_ARGS__, _OR_10, _OR_9, _OR_8, _OR_7, _OR_6, _OR_5, _OR_4, _OR_3, _OR_2, _OR_1)(__VA_ARGS__)

/* Text */
/* Internal helper macros for OR expansion with safe parentheses */
#define _OR_1(a)                          ((a))
#define _OR_2(a, b)                       ((a) | (b))
#define _OR_3(a, b, c)                    ((a) | (b) | (c))
#define _OR_4(a, b, c, d)                 ((a) | (b) | (c) | (d))
#define _OR_5(a, b, c, d, e)              ((a) | (b) | (c) | (d) | (e))
#define _OR_6(a, b, c, d, e, f)           ((a) | (b) | (c) | (d) | (e) | (f))
#define _OR_7(a, b, c, d, e, f, g)        ((a) | (b) | (c) | (d) | (e) | (f) | (g))
#define _OR_8(a, b, c, d, e, f, g, h)     ((a) | (b) | (c) | (d) | (e) | (f) | (g) | (h))
#define _OR_9(a, b, c, d, e, f, g, h, i)  ((a) | (b) | (c) | (d) | (e) | (f) | (g) | (h) | (i))
#define _OR_10(a, b, c, d, e, f, g, h, i, j) ((a) | (b) | (c) | (d) | (e) | (f) | (g) | (h) | (i) | (j))

/* Text */
/* Dispatcher macro for OR */
#define _OR_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME


/// @brief Perform bitwise AND on up to 10 arguments (Compile-time)
/// @param ... List of values to AND
/// @return Result of ((a) & (b) & (c) & ...)
#define AND_ALL(...) _AND_GET_MACRO(__VA_ARGS__, _AND_10, _AND_9, _AND_8, _AND_7, _AND_6, _AND_5, _AND_4, _AND_3, _AND_2, _AND_1)(__VA_ARGS__)

/* Text */
/* Internal helper macros for AND expansion with safe parentheses */
#define _AND_1(a)                         ((a))
#define _AND_2(a, b)                      ((a) & (b))
#define _AND_3(a, b, c)                   ((a) & (b) & (c))
#define _AND_4(a, b, c, d)                ((a) & (b) & (c) & (d))
#define _AND_5(a, b, c, d, e)             ((a) & (b) & (c) & (d) & (e))
#define _AND_6(a, b, c, d, e, f)          ((a) & (b) & (c) & (d) & (e) & (f))
#define _AND_7(a, b, c, d, e, f, g)       ((a) & (b) & (c) & (d) & (e) & (f) & (g))
#define _AND_8(a, b, c, d, e, f, g, h)    ((a) & (b) & (c) & (d) & (e) & (f) & (g) & (h))
#define _AND_9(a, b, c, d, e, f, g, h, i) ((a) & (b) & (c) & (d) & (e) & (f) & (g) & (h) & (i))
#define _AND_10(a, b, c, d, e, f, g, h, i, j) ((a) & (b) & (c) & (d) & (e) & (f) & (g) & (h) & (i) & (j))

/* Text */
/* Dispatcher macro for AND */
#define _AND_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME


#endif /// __BITWISE_H__