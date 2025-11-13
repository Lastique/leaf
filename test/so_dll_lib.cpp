// Copyright 2018-2024 Emil Dotchevski and Reverge Studios, Inc.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef _WIN32
#   ifdef BOOST_LEAF_CFG_WIN32
#       undef BOOST_LEAF_CFG_WIN32
#   endif
#   define BOOST_LEAF_CFG_WIN32 2
#endif

#define BOOST_LEAF_SO_DLL_TEST_BUILDING_LIB
#include "so_dll_test_lib.hpp"

#ifdef BOOST_LEAF_TEST_SINGLE_HEADER
#   include "leaf.hpp"
#else
#   include <boost/leaf/exception.hpp>
#   include <boost/leaf/result.hpp>
#   include <boost/leaf/on_error.hpp>
#endif

namespace leaf = boost::leaf;

leaf::result<void> BOOST_LEAF_SO_DLL_TEST_API hidden_result()
{
    auto load = leaf::on_error( my_info<1>{1}, my_info<3>{3} );
    return leaf::new_error( my_info<2>{2} );
}

#ifndef BOOST_LEAF_NO_EXCEPTIONS

void BOOST_LEAF_SO_DLL_TEST_API hidden_throw()
{
    auto load = leaf::on_error( my_info<1>{1}, my_info<3>{3} );
    leaf::throw_exception( my_info<2>{2} );
}

#endif
