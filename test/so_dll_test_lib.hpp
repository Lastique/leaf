#ifndef VISIBILITY_TEST_LIB_HPP_INCLUDED
#define VISIBILITY_TEST_LIB_HPP_INCLUDED

// Copyright 2018-2024 Emil Dotchevski and Reverge Studios, Inc.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iosfwd>

#if defined(_WIN32) && BOOST_LEAF_CFG_WIN32 < 2
#   error This test requires BOOST_LEAF_CFG_WIN32 >= 2
#endif

#ifdef BOOST_LEAF_SO_DLL_TEST_BUILDING_LIB
#   if defined(_WIN32)
#       define BOOST_LEAF_SO_DLL_TEST_API __declspec(dllexport)
#   elif defined(__GNUC__)
#       define BOOST_LEAF_SO_DLL_TEST_API __attribute__((visibility("default")))
#   else
#       define BOOST_LEAF_SO_DLL_TEST_API
#   endif
#else
#   if defined(_WIN32)
#       define BOOST_LEAF_SO_DLL_TEST_API __declspec(dllimport)
#   else
#       define BOOST_LEAF_SO_DLL_TEST_API
#   endif
#endif

template <int Tag>
struct BOOST_LEAF_SO_DLL_TEST_API my_info
{
    int value;

    template <class CharT, class Traits>
    friend std::ostream & operator<<( std::basic_ostream<CharT, Traits> & os, my_info const & x )
    {
        return os << "Test my_info<" << Tag << ">::value = " << x.value;
    }
};

#endif
