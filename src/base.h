#ifndef ORCA_BASE_H
#define ORCA_BASE_H

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// clang or gcc or other
#if defined(__clang__)
    #define ORCA_OPT_MINSIZE __attribute__((minsize))
#elif defined(__GNUC__)
    #define ORCA_OPT_MINSIZE __attribute__((optimize("Os")))
#else
    #define ORCA_OPT_MINSIZE
#endif

// (gcc / clang) or msvc or other
#if defined(__GNUC__) || defined(__clang__)
    #define ORCA_FORCEINLINE __attribute__((always_inline)) inline
    #define ORCA_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
    #define ORCA_FORCEINLINE __forceinline
    #define ORCA_NOINLINE __declspec(noinline)
#else
    #define ORCA_FORCEINLINE inline
    #define ORCA_NOINLINE
#endif

// (gcc / clang) or other
#if defined(__GNUC__) || defined(__clang__)
    #define ORCA_ASSUME_ALIGNED(_ptr, _alignment) __builtin_assume_aligned(_ptr, _alignment)
    #define ORCA_PURE __attribute__((pure))
    #define ORCA_LIKELY(_x) __builtin_expect(_x, 1)
    #define ORCA_UNLIKELY(_x) __builtin_expect(_x, 0)
    #define ORCA_OK_IF_UNUSED __attribute__((unused))
    #define ORCA_UNREACHABLE __builtin_unreachable()
#else
    #define ORCA_ASSUME_ALIGNED(_ptr, _alignment) (_ptr)
    #define ORCA_PURE
    #define ORCA_LIKELY(_x) (_x)
    #define ORCA_UNLIKELY(_x) (_x)
    #define ORCA_OK_IF_UNUSED
    #define ORCA_UNREACHABLE assert(false)
#endif

// array count, safer on gcc/clang
#if defined(__GNUC__) || defined(__clang__)
    #define ORCA_ASSERT_IS_ARRAY(_array)                                                                \
        (sizeof(char[1 - 2 * __builtin_types_compatible_p(__typeof(_array), __typeof(&(_array)[0]))]) - \
         1)
    #define ORCA_ARRAY_COUNTOF(_array)                                                             \
        (sizeof(_array) / sizeof((_array)[0]) + ORCA_ASSERT_IS_ARRAY(_array))
#else
    // pray
    #define ORCA_ARRAY_COUNTOF(_array) (sizeof(_array) / sizeof(_array[0]))
#endif

#define staticni ORCA_NOINLINE static

#ifdef __cplusplus
extern "C" {
#endif


    //----------------------------------------------------------------------------------------
    // Types
    //----------------------------------------------------------------------------------------
    typedef uint8_t U8;
    typedef int8_t I8;
    typedef uint16_t U16;
    typedef int16_t I16;
    typedef uint32_t U32;
    typedef int32_t I32;
    typedef uint64_t U64;
    typedef int64_t I64;
    typedef size_t Usz;
    typedef ssize_t Isz;

    typedef char Glyph;
    typedef U8 Mark;

//----------------------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------------------
#define ORCA_Y_MAX UINT16_MAX
#define ORCA_X_MAX UINT16_MAX

    enum
    {
        Hud_height = 2
    };


    //----------------------------------------------------------------------------------------
    // Utils
    //----------------------------------------------------------------------------------------

    ORCA_FORCEINLINE static Usz orca_round_up_power2(Usz x)
    {
        assert(x <= SIZE_MAX / 2 + 1);
        x -= 1;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
#if SIZE_MAX > UINT32_MAX
        x |= x >> 32;
#endif
        return x + 1;
    }

    ORCA_OK_IF_UNUSED
    static bool orca_is_valid_glyph(Glyph c)
    {
        if (c >= '0' && c <= '9')
            return true;
        if (c >= 'A' && c <= 'Z')
            return true;
        if (c >= 'a' && c <= 'z')
            return true;
        switch (c) {
            case '!':
            case '#':
            case '%':
            case '*':
            case '.':
            case ':':
            case ';':
            case '=':
            case '?':
                return true;
        }
        return false;
    }

    static ORCA_FORCEINLINE double ms_to_sec(double ms)
    {
        return ms / 1000.0;
    }


    staticni bool str_to_int(char const *str, int *out)
    {
        int a;
        int res = sscanf(str, "%d", &a);
        if (res != 1)
            return false;
        *out = a;
        return true;
    }

    // Reads something like '5x3' or '5'. Writes the same value to both outputs if
    // only one is specified. Returns false on error.
    staticni bool read_nxn_or_n(char const *str, int *out_a, int *out_b)
    {
        int a, b;
        int res = sscanf(str, "%dx%d", &a, &b);
        if (res == EOF)
            return false;
        if (res == 1) {
            *out_a = a;
            *out_b = a;
            return true;
        }
        if (res == 2) {
            *out_a = a;
            *out_b = b;
            return true;
        }
        return false;
    }

#ifdef __cplusplus
};
#endif


#endif //ORCA_BASE_H
