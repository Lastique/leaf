#ifndef SO_DLL_TEST_LIB1_HPP_INCLUDED
#define SO_DLL_TEST_LIB1_HPP_INCLUDED

// Copyright 2018-2024 Emil Dotchevski and Reverge Studios, Inc.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "so_dll_test.hpp"

#ifdef BOOST_LEAF_SO_DLL_TEST_BUILDING_EXE
#   if defined(_WIN32)
#       define BOOST_LEAF_SO_DLL_TEST_LIB1_API __declspec(dllimport)
#   else
#       define BOOST_LEAF_SO_DLL_TEST_LIB1_API
#   endif
#else
#   if defined(_WIN32)
#       define BOOST_LEAF_SO_DLL_TEST_LIB1_API __declspec(dllexport)
#   elif defined(__GNUC__)
#       define BOOST_LEAF_SO_DLL_TEST_LIB1_API __attribute__((visibility("default")))
#   else
#       define BOOST_LEAF_SO_DLL_TEST_LIB1_API
#   endif
#endif

namespace boost { namespace leaf {
    template <class T> class result;
} }

boost::leaf::result<void> BOOST_LEAF_SO_DLL_TEST_LIB1_API hidden_result1();

#ifndef BOOST_LEAF_NO_EXCEPTIONS
void BOOST_LEAF_SO_DLL_TEST_LIB1_API hidden_throw1();
#endif

#endif

