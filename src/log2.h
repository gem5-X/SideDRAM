/* 
 * 
 * Preprocessor to obtain the rounded-up log2 of an integer.
 *
 */

#ifndef LOG2_H
#define LOG2_H

#define LOG2(n) \
(\
    n > 1024 ? 11 : \
    n >  512 ? 10 : \
    n >  256 ?  9 : \
    n >  128 ?  8 : \
    n >   64 ?  7 : \
    n >   32 ?  6 : \
    n >   16 ?  5 : \
    n >    8 ?  4 : \
    n >    4 ?  3 : \
    n >    2 ?  2 : 1 \
)

#define MAX(a,b) (a > b ? a : b)

#endif
