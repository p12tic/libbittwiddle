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
template<std::uint8_t C, class T>
constexpr T broadcast()
{
    // equivalent to 0x0101..01 * C
    return ((~T(0))/0xff) * C;
}

#if defined(__GNUC__) && !defined(LIBBITTWIDDLE_TESTING)
unsigned popcnt_impl(std::uint32_t x)
{
    return __builtin_popcount(x);
}

unsigned popcnt_impl(std::uint64_t x)
{
    return __builtin_popcountll(x);
}
#else
template<class T>
unsigned popcnt_impl(T x)
{
    T b_0x01 = broadcast<0x01,T>();
    T b_0x55 = broadcast<0x55,T>();
    T b_0x33 = broadcast<0x33,T>();
    T b_0x0f = broadcast<0x0f,T>();

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
unsigned clz_impl(std::uint32_t x)
{
    return __builtin_clz(x);
}

unsigned clz_impl(std::uint64_t x)
{
    return __builtin_clzll(x);
}
#else
// http://aggregate.org/MAGIC/#Leading%20Zero%20Count
unsigned clz_impl(std::uint32_t x)
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return(32 - popcnt_impl(x));
}

unsigned clz_impl(std::uint64_t x)
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
unsigned ctz_impl(std::uint32_t x)
{
    return __builtin_ctz(x);
}

unsigned ctz_impl(std::uint64_t x)
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
    T b_1 = broadcast<1,T>();
    T b_0x80 = broadcast<0x80,T>();

    return (x - b_1) & ~x & b_0x80;
}

template<std::uint8_t A, class T>
bool check_has_byte_eq_impl(T x)
{
    T b_a = broadcast<A,T>();
    return check_has_zero_impl(x ^ b_a);
}

template<std::uint8_t A, class T>
bool check_has_byte_lt_impl128(T x)
{
    //static_assert(A <= 128, "A out of range");
    T b_a = broadcast<A,T>();
    T b_128 = broadcast<128,T>();

    return (x - b_a) & ~x & b_128;
}

template<std::uint8_t A, class T>
bool check_has_byte_gt_impl127(T x)
{
    //static_assert(A <= 127, "A out of range");
    T b_127ma = broadcast<127-A,T>();
    T b_128 = broadcast<128,T>();

    return ((x + b_127ma) | x ) & b_128;
}

template<std::uint8_t A, class T>
bool check_has_byte_lt_impl(T x)
{
    //static_assert(A <= 128, "A out of range");
    if (A >= 128) {
        x = x ^ ~T(0);
        return check_has_byte_gt_impl127<255-A>(x);
    } else {
        return check_has_byte_lt_impl128<A>(x);
    }
}

template<std::uint8_t A, class T>
bool check_has_byte_gt_impl(T x)
{
    //static_assert(A <= 128, "A out of range");
    if (A >= 127) {
        x = x ^ ~T(0);
        return check_has_byte_lt_impl128<255-A>(x);
    } else {
        return check_has_byte_gt_impl127<A>(x);
    }
}

template<std::uint8_t A, std::uint8_t B, class T>
bool check_has_byte_between_impl(T x)
{
    static_assert(0 <= A && A <= 127, "A out of range");
    static_assert(0 <= B && B <= 128, "B out of range");
    static_assert(A < B, "A must be less than B");
    T b_127pa = broadcast<127+A,T>();
    T b_127mb = broadcast<127-B,T>();
    T b_127 = broadcast<127,T>();
    T b_128 = broadcast<128,T>();

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
/** Returns @c true if word has a byte equal to A.
*/
template<std::uint8_t A>
bool check_has_byte_eq(std::uint32_t word)
{
    return internal::check_has_byte_eq_impl<A>(word);
}
template<std::uint8_t A>
bool check_has_byte_eq(std::uint64_t word)
{
    return internal::check_has_byte_eq_impl<A>(word);
}
/// @}

/// @{
/** Returns @c true if word has a byte in the range [0, A).
*/
template<std::uint8_t A>
bool check_has_byte_lt(std::uint32_t word)
{
    return internal::check_has_byte_lt_impl<A>(word);
}
template<std::uint8_t A>
bool check_has_byte_lt(std::uint64_t word)
{
    return internal::check_has_byte_lt_impl<A>(word);
}
/// @}

/// @{
/** Returns @c true if word has a byte in the range (A, 255].
*/
template<std::uint8_t A>
bool check_has_byte_gt(std::uint32_t word)
{
    return internal::check_has_byte_gt_impl<A>(word);
}
template<std::uint8_t A>
bool check_has_byte_gt(std::uint64_t word)
{
    return internal::check_has_byte_gt_impl<A>(word);
}
/// @}

/// @{
/** Returns @c true if word has a byte in the range [M, N).
    The following must hold true:

    0 <= A <= 127
    0 <= B <= 128
    A < B

    FIXME: not tested
*/
template<std::uint8_t A, std::uint8_t B>
bool check_has_byte_between(std::uint32_t word)
{
    return internal::check_has_byte_between_impl<A,B>(word);
}
template<std::uint8_t A, std::uint8_t B>
bool check_has_byte_between(std::uint64_t word)
{
    return internal::check_has_byte_between_impl<A,B>(word);
}
/// @}

} // namespace bittwiddle

#endif
