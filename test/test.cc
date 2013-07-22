/*  libbittwiddle
    Copyright (C) 2013  Povilas Kanapickas tir5c3@yahoo.co.uk

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#define LIBBITTWIDDLE_TESTING
#include "libbittwiddle/libbittwiddle.h"

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE bittwiddle
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <iomanip>

BOOST_AUTO_TEST_SUITE(bittwiddle)

// NOTE: we assume CHAR_BIT == 8

/*  Since the library does not use shifts, the testing for (e.g)
    check_has_byte_eq can be effectively exhaustive by testing 4*256*4 * 8 (all
    values of a byte, plus the values of four neighbouring bits, plus the 8
    possible positions within a 64-bit block.
*/
std::uint64_t build_test_block(unsigned ch, unsigned ms_bits, unsigned ls_bits)
{
    return (ls_bits << 6) | (ch << 8) | (ms_bits << 16);
}

std::uint64_t combine_test_bg(std::uint64_t t, std::uint64_t bg, int pos)
{
    std::uint64_t mask = 0x03ffc0;

    pos--;
    if (pos < 0) {
        t >>= -pos*8;
        mask >>= -pos*8;
    } else {
        t <<= pos*8;
        mask <<= pos*8;
    }

    return (t & mask) | (bg & ~mask);
}

void print_test(std::ostream& out, std::string op, unsigned ch, unsigned ch2,
                std::uint64_t x)
{
    out << op << " " << std::hex << std::setw(2) << std::setfill('0') << ch
        << "/" << std::hex << std::setw(2) << std::setfill('0') << ch2
        << " [ ";
    for (unsigned i = 0; i < 8; i++) {
        unsigned y = x >> (8*(7-i));
        y &= 0xff;
        out << std::hex << std::setw(2) << std::setfill('0') << y << " ";
    }
    out << "]\n";
}

template<unsigned Ch>
void test_exhaustive()
{
    for (unsigned ch = 0; ch < 256; ch++) {
        for (int pos = 0; pos < sizeof(std::uint64_t); pos++) {
            for (unsigned ms_bits = 0; ms_bits < 4; ms_bits++) {
                for (unsigned ls_bits = 0; ls_bits < 4; ls_bits++) {

                    std::uint64_t t, bg;

                    { // test has_eq

                        // make sure we don't have any other bytes equal to Ch
                        t = build_test_block(ch, ms_bits, ls_bits);
                        bg = 0;

                        if (popcnt(Ch) <= 4) {
                            bg = ~bg;
                        }

                        t = combine_test_bg(t, bg, pos);

                        bool ok = (check_has_byte_eq<Ch>(t) == (Ch == ch));
                        BOOST_CHECK(ok);
                        if (!ok) {
                            print_test(std::cout, "has_eq", Ch, ch, t);
                        }
                    }

                    { // test has_lt

                        // make sure neighbor bits don't 'create' any characters
                        // that will be less than Ch
                        unsigned ms_bits2, ls_bits2;
                        if (Ch >= 0xc0) {
                            ls_bits2 = 3;
                        } else if (Ch >= 0x80) {
                            ls_bits2 = std::max<unsigned>(2, ls_bits);
                        } else if (Ch >= 0x40) {
                            ls_bits2 = std::max<unsigned>(1, ls_bits);
                        } else {
                            ls_bits2 = ls_bits;
                        }

                        switch (Ch) {
                        case 0xff: ms_bits2 = 3; break;
                        case 0xfe: ms_bits2 = std::max<unsigned>(2, ms_bits); break;
                        case 0xfd: ms_bits2 = std::max<unsigned>(1, ms_bits); break;
                        default: ms_bits2 = ms_bits; break;
                        }

                        bg = 0;
                        bg = ~bg;

                        t = build_test_block(ch, ms_bits2, ls_bits2);
                        t = combine_test_bg(t, bg, pos);

                        bool ok = (check_has_byte_lt<Ch>(t) == (ch < Ch));
                        BOOST_CHECK(ok);
                        if (!ok) {
                            print_test(std::cout, "has_lt", Ch, ch, t);
                        }
                    }

                    { // test has_gt

                        unsigned ms_bits2, ls_bits2;
                        if (Ch < 0x40) {
                            ls_bits2 = 0;
                        } else if (Ch < 0x80) {
                            ls_bits2 = std::min<unsigned>(1, ls_bits);
                        } else if (Ch < 0xc0) {
                            ls_bits2 = std::min<unsigned>(2, ls_bits);
                        } else {
                            ls_bits2 = ls_bits;
                        }

                        switch (Ch) {
                        case 0x0: ms_bits2 = 0; break;
                        case 0x1: ms_bits2 = std::min<unsigned>(1, ms_bits); break;
                        case 0x2: ms_bits2 = std::min<unsigned>(2, ms_bits); break;
                        default: ms_bits2 = ms_bits; break;
                        }

                        bg = 0;

                        t = build_test_block(ch, ms_bits2, ls_bits2);
                        t = combine_test_bg(t, bg, pos);

                        bool ok = (check_has_byte_gt<Ch>(t) == (ch > Ch));
                        BOOST_CHECK(ok);
                        if (!ok) {
                            print_test(std::cout, "has_gt", Ch, ch, t);
                        }
                    }
                }
            }
        }
    }
}

template<unsigned Ch>
struct TestAllChExhaustive {
    static void test()
    {
        test_exhaustive<Ch>();
        TestAllChExhaustive<Ch-1>::test();
    }
};

template<>
struct TestAllChExhaustive<0> {
    static void test()
    {
        test_exhaustive<0>();
    }
};

BOOST_AUTO_TEST_CASE(popcnt_test)
{
    using namespace bittwiddle;

    BOOST_CHECK(popcnt(0) == 0);
    BOOST_CHECK(popcnt(0xffffffffffffffff) == 64);
    BOOST_CHECK(popcnt(0x00000000ffffffff) == 32);
    BOOST_CHECK(popcnt(0xffffffff00000000) == 32);
    BOOST_CHECK(popcnt(0xffff0000ffff0000) == 32);
    BOOST_CHECK(popcnt(0x0000ffff0000ffff) == 32);

    for (unsigned i = 0; i < 256; i++) {
        std::uint64_t z = i * 0x0101010101010101;
        BOOST_CHECK(popcnt(z) == popcnt(i)*8);
    }

    for (unsigned i = 0; i < 64; i++) {
        std::uint64_t z = 1;
        z <<= i;
        BOOST_CHECK(popcnt(z) == 1);
    }

    for (unsigned i = 0; i < 63; i++) {
        std::uint64_t z = 0x03;
        z <<= i;
        BOOST_CHECK(popcnt(z) == 2);
    }

    for (unsigned i = 0; i < 56; i++) {
        std::uint64_t z;
        z = 0x55;
        z <<= i;
        BOOST_CHECK(popcnt(z) == 4);
        z = 0xaa;
        z <<= i;
        BOOST_CHECK(popcnt(z) == 4);
        z = 0xff;
        z <<= i;
        BOOST_CHECK(popcnt(z) == 8);
    }
}

BOOST_AUTO_TEST_CASE(clz_test)
{
    using namespace bittwiddle;

    for (unsigned i = 0; i < 64; i++) {
        for (unsigned i2 = i; i2 < 64; i2++) {
            std::uint64_t z = 1;
            z <<= 63;
            z = (z >> i) | (z >> i2);
            BOOST_CHECK(clz(z) == i);
        }
    }

    for (unsigned i = 0; i < 32; i++) {
        for (unsigned i2 = i; i2 < 32; i2++) {
            std::uint32_t z = 1;
            z <<= 31;
            z = (z >> i) | (z >> i2);
            BOOST_CHECK(clz(z) == i);
        }
    }
}

BOOST_AUTO_TEST_CASE(ctz_test)
{
    using namespace bittwiddle;

    for (unsigned i = 0; i < 64; i++) {
        for (unsigned i2 = i; i2 < 64; i2++) {
            std::uint64_t z = 1;
            z = (z << i) | (z << i2);
            BOOST_CHECK(ctz(z) == i);
        }
    }

    for (unsigned i = 0; i < 32; i++) {
        for (unsigned i2 = i; i2 < 32; i2++) {
            std::uint32_t z = 1;
            z = (z << i) | (z << i2);
            BOOST_CHECK(ctz(z) == i);
        }
    }
}

BOOST_AUTO_TEST_CASE(all_tests)
{
    TestAllChExhaustive<255>::test();
}

BOOST_AUTO_TEST_SUITE_END()
