/*  libbittwiddle
    Copyright (C) 2013  Povilas Kanapickas tir5c3@yahoo.co.uk

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef LIBBITTWIDDLE_LIBBITWIDDLE_H
#define LIBBITTWIDDLE_LIBBITWIDDLE_H

#include <cstdint>

namespace bittwiddle {

namespace internal {

// Broadscasts the byte to all bytes of the value
template<class T>
constexpr T broadcast(std::uint8_t x)
{
    // equivalent to 0x0101..01 * C
    return ((~T(0))/0xff) * x;
}

#if defined(__GNUC__) && !defined(LIBBITTWIDDLE_TESTING)
inline unsigned popcnt_impl(std::uint32_t x)
{
    return __builtin_popcount(x);
}

inline unsigned popcnt_impl(std::uint64_t x)
{
    return __builtin_popcountll(x);
}
#else
template<class T>
unsigned popcnt_impl(T x)
{
    T b_0x01 = broadcast<T>(0x01);
    T b_0x55 = broadcast<T>(0x55);
    T b_0x33 = broadcast<T>(0x33);
    T b_0x0f = broadcast<T>(0x0f);

    // sum adjacent bits
    x -= (x >> 1) & b_0x55;
    // sum adjacent pairs of bits
    x = (x & b_0x33) + ((x >> 2) & b_0x33);
    // sum adjacent quartets of bits
    x = (x + (x >> 4)) & b_0x0f;

    // sum all bytes
    return (x * b_0x01) >> (sizeof(T)-1)*8;
}
#endif

#if defined(__GNUC__) && !defined(LIBBITTWIDDLE_TESTING)
inline unsigned clz_impl(std::uint32_t x)
{
    return __builtin_clz(x);
}

inline unsigned clz_impl(std::uint64_t x)
{
    return __builtin_clzll(x);
}
#else
// http://aggregate.org/MAGIC/#Leading%20Zero%20Count
inline unsigned clz_impl(std::uint32_t x)
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return(32 - popcnt_impl(x));
}

inline unsigned clz_impl(std::uint64_t x)
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    x |= (x >> 32);
    return(64 - popcnt_impl(x));
}
#endif

#if defined(__GNUC__) && !defined(LIBBITTWIDDLE_TESTING)
inline unsigned ctz_impl(std::uint32_t x)
{
    return __builtin_ctz(x);
}

inline unsigned ctz_impl(std::uint64_t x)
{
    return __builtin_ctzll(x);
}
#else
// http://aggregate.org/MAGIC/#Trailing%20Zero%20Count
template<class T>
unsigned ctz_impl(T x)
{
    return popcnt_impl((x & -x) - 1);
}
#endif

template<class T>
bool check_is_power_2(T x)
{
    return x && !(x & (x - 1));
}

template<class T>
bool check_has_zero_impl(T x)
{
    T b_1 = broadcast<T>(1);
    T b_0x80 = broadcast<T>(0x80);

    return (x - b_1) & ~x & b_0x80;
}

template<class T>
bool check_has_byte_eq_impl(T x, std::uint8_t a)
{
    T b_a = broadcast<T>(a);
    return check_has_zero_impl(x ^ b_a);
}

#ifdef LIBBITTWIDDLE_BRANCHLESS_CHECK_HAS_LTGT
/* This implementation is useful if @a a is effectively random -- i.e. branch
    predictor can't predict whether @a a is less or greater than 127 or 128. If
    this is not the case, use the implementation with branches. For values
    known at compile-time, both implementations are equivalent
*/

template<class T>
bool check_has_byte_lt_impl(T x, std::uint8_t a)
{
    bool gt127 = (a >= 128);
    T mask = gt127 ? ~T(0) : 0;

    x = x ^ mask;
    a = (255-a & mask) | (a & ~mask);

    T b_0x80 = broadcast<T>(0x80);
    T b_127 = broadcast<T>(127);
    T b_a = broadcast<T>(a);

    b_127 &= mask;
    T x_or = x & mask;
    T x_and = ~x | mask;

    return (((x + b_127 - b_a) & x_and) | x_or) & b_0x80;
}

template<class T>
bool check_has_byte_gt_impl(T x, std::uint8_t a)
{
    bool lt128 = (a >= 127);
    T mask = lt128 ? ~T(0) : 0;

    x = x ^ mask;
    a = (255-a & mask) | (a & ~mask);

    T b_0x80 = broadcast<T>(0x80);
    T b_127 = broadcast<T>(127);
    T b_a = broadcast<T>(a);

    b_127 &= ~mask;
    T x_or = x & ~mask;
    T x_and = ~x | ~mask;

    return (((x + b_127 - b_a) & x_and) | x_or) & b_0x80;
}
#else

template<class T>
bool check_has_byte_lt_impl128(T x, std::uint8_t a)
{
    T b_a = broadcast<T>(a);
    T b_0x80 = broadcast<T>(0x80);

    return (x - b_a) & ~x & b_0x80;
}

template<class T>
bool check_has_byte_gt_impl127(T x, std::uint8_t a)
{
    T b_127ma = broadcast<T>(127-a);
    T b_0x80 = broadcast<T>(0x80);

    return ((x + b_127ma) | x ) & b_0x80;
}

template<class T>
bool check_has_byte_lt_impl(T x, std::uint8_t a)
{
    if (a >= 128) {
        x = ~x;
        return check_has_byte_gt_impl127(x, 255-a);
    } else {
        return check_has_byte_lt_impl128(x, a);
    }
}

template<class T>
bool check_has_byte_gt_impl(T x, std::uint8_t a)
{
    if (a >= 127) {
        x = ~x;
        return check_has_byte_lt_impl128(x, 255-a);
    } else {
        return check_has_byte_gt_impl127(x, a);
    }
}
#endif

template<class T>
bool check_has_byte_between_impl(T x, std::uint8_t a, std::uint8_t b)
{
    T b_127pa = broadcast<T>(127+a);
    T b_127mb = broadcast<T>(127-b);
    T b_127 = broadcast<T>(127);
    T b_128 = broadcast<T>(128);

    T res;
    res = b_127pa - b_127;
    res &= ~x;
    res &= ((x & b_127) + b_127mb);
    res &= b_128;
    return res;
}

} // namespace internal

/// @{
/// Counts the number of set bits
inline unsigned popcnt(std::uint32_t x)
{
    return internal::popcnt_impl(x);
}
inline unsigned popcnt(std::int32_t x)
{
    return internal::popcnt_impl(static_cast<std::uint32_t>(x));
}
inline unsigned popcnt(std::uint64_t x)
{
    return internal::popcnt_impl(x);
}
inline unsigned popcnt(std::int64_t x)
{
    return internal::popcnt_impl(static_cast<std::uint64_t>(x));
}
/// @}

/// @{
/// Counts the number of leading zero bits
inline unsigned clz(std::uint32_t x)
{
    return internal::clz_impl(x);
}
inline unsigned clz(std::int32_t x)
{
    return internal::clz_impl(static_cast<std::uint32_t>(x));
}
inline unsigned clz(std::uint64_t x)
{
    return internal::clz_impl(x);
}
inline unsigned clz(std::int64_t x)
{
    return internal::clz_impl(static_cast<std::uint64_t>(x));
}
/// @}

/// @{
/// Counts the number of trailing zero bits
inline unsigned ctz(std::uint32_t x)
{
    return internal::ctz_impl(x);
}
inline unsigned ctz(std::int32_t x)
{
    return internal::ctz_impl(static_cast<std::uint32_t>(x));
}
inline unsigned ctz(std::uint64_t x)
{
    return internal::ctz_impl(x);
}
inline unsigned ctz(std::int64_t x)
{
    return internal::ctz_impl(static_cast<std::uint64_t>(x));
}
/// @}

/// @{
/** Returns @c true if word has a byte equal to a.
*/
inline bool check_has_byte_eq(std::uint32_t word, std::uint8_t a)
{
    return internal::check_has_byte_eq_impl(word, a);
}
inline bool check_has_byte_eq(std::uint64_t word, std::uint8_t a)
{
    return internal::check_has_byte_eq_impl(word, a);
}
/// @}

/// @{
/** Returns @c true if word has a byte in the range [0, a).
*/
inline bool check_has_byte_lt(std::uint32_t word, std::uint8_t a)
{
    return internal::check_has_byte_lt_impl(word, a);
}
inline bool check_has_byte_lt(std::uint64_t word, std::uint8_t a)
{
    return internal::check_has_byte_lt_impl(word, a);
}
/// @}

/// @{
/** Returns @c true if word has a byte in the range (a, 255].
*/
inline bool check_has_byte_gt(std::uint32_t word, std::uint8_t a)
{
    return internal::check_has_byte_gt_impl(word, a);
}
inline bool check_has_byte_gt(std::uint64_t word, std::uint8_t a)
{
    return internal::check_has_byte_gt_impl(word, a);
}
/// @}

/// @{
/** Returns @c true if word has a byte in the range [a, b).
    The following must hold true:

    0 <= a <= 127
    0 <= b <= 128
    a < b

    FIXME: not tested
*/
inline bool check_has_byte_between(std::uint32_t word, std::uint8_t a, std::uint8_t b)
{
    return internal::check_has_byte_between_impl(word, a, b);
}

inline bool check_has_byte_between(std::uint64_t word, std::uint8_t a, std::uint8_t b)
{
    return internal::check_has_byte_between_impl(word, a, b);
}
/// @}

} // namespace bittwiddle

#endif
