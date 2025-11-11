#ifndef BOOST_LEAF_CONFIG_TLS_WIN32_HPP_INCLUDED
#define BOOST_LEAF_CONFIG_TLS_WIN32_HPP_INCLUDED

// Copyright 2018-2024 Emil Dotchevski and Reverge Studios, Inc.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <windows.h>
#include <unordered_map>
#include <typeinfo>
#include <cstdint>
#include <atomic>
#include <stdexcept>
#include <cstring>
#include <string_view>
#ifdef min
#   undef min
#endif
#ifdef max
#   undef max
#endif

namespace boost { namespace leaf {

class win32_tls_error:
    public std::runtime_error
{
public:
    explicit win32_tls_error(char const * what) noexcept:
        std::runtime_error(what)
    {
    }
};

namespace detail
{
    using atomic_unsigned_int = std::atomic<unsigned int>;

    class slot_map
    {
        slot_map(slot_map const &) = delete;
        slot_map & operator=(slot_map const &) = delete;

        class tls_slot_index
        {
            tls_slot_index(tls_slot_index const &) = delete;
            tls_slot_index & operator=(tls_slot_index const &) = delete;
            tls_slot_index & operator=(tls_slot_index &&) = delete;

            DWORD idx_;

        public:

            tls_slot_index():
                idx_(TlsAlloc())    
            {
                if (idx_ == TLS_OUT_OF_INDEXES)
                    throw_exception_(win32_tls_error("TLS_OUT_OF_INDEXES"));
            }   

            ~tls_slot_index() noexcept
            {
                if (idx_ == TLS_OUT_OF_INDEXES)
                    return;
                BOOL r = TlsFree(idx_);
                BOOST_LEAF_ASSERT(r), (void) r;
            }

            tls_slot_index(tls_slot_index && other) noexcept:
                idx_(other.idx_)
            {
                other.idx_ = TLS_OUT_OF_INDEXES;
            }

            DWORD get() const noexcept
            {
                BOOST_LEAF_ASSERT(idx_ != TLS_OUT_OF_INDEXES);
                return idx_;
            }
        };

        struct cstring_hash
        {
            std::size_t operator()(char const * s) const noexcept
            {
                return std::hash<std::string_view>{}(s);
            }
        };
        
        struct cstring_equal
        {
            bool operator()(char const * a, char const * b) const noexcept
            {
                return a == b || std::strcmp(a, b) == 0;
            }
        };

        tls_slot_index const error_id_slot_;
        mutable CRITICAL_SECTION cs_;
        std::unordered_map<char const *, tls_slot_index, cstring_hash, cstring_equal> map_;

    public:

        slot_map()
        {
            InitializeCriticalSection(&cs_);
        }

        ~slot_map() noexcept
        {
            DeleteCriticalSection(&cs_);
        }

        DWORD check(char const * type_name) const noexcept
        {
            EnterCriticalSection(&cs_);
            auto it = map_.find(type_name);
            DWORD idx = (it != map_.end()) ? it->second.get() : TLS_OUT_OF_INDEXES;
            LeaveCriticalSection(&cs_);
            return idx;
        }

        DWORD get(char const * type_name)
        {
            EnterCriticalSection(&cs_);
            DWORD idx = map_[type_name].get();
            LeaveCriticalSection(&cs_);
            BOOST_LEAF_ASSERT(idx != TLS_OUT_OF_INDEXES);
            return idx;
        }

        DWORD error_id_slot() const noexcept
        {
            return error_id_slot_.get();
        }
    };

    template<int = 0>
    struct global_slot_map
    {
        static slot_map * ptr;
    };

    template<int N>
    slot_map * global_slot_map<N>::ptr = nullptr;

    inline void NTAPI tls_callback(PVOID, DWORD dwReason, PVOID) noexcept
    {
        static HANDLE s_mapping = INVALID_HANDLE_VALUE;
        if (dwReason == DLL_PROCESS_ATTACH)
        {
            char name[64];
            int num_written = std::snprintf(name, sizeof(name), "Local\\boost_leaf_tls_%lu", GetCurrentProcessId());
            BOOST_LEAF_ASSERT(num_written >= 0 && num_written < sizeof(name)), (void) num_written;
            HANDLE mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(slot_map *), name);
            if (!mapping)
                return;
            bool is_main_module = (GetLastError() != ERROR_ALREADY_EXISTS);
            if (is_main_module)
            {
                slot_map * * mapped_ptr = static_cast<slot_map * *>(MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, sizeof(slot_map *)));
                if (!mapped_ptr)
                {
                    BOOL r = CloseHandle(mapping);
                    BOOST_LEAF_ASSERT(r), (void) r;
                    return;
                }
                try
                {
                    global_slot_map<>::ptr = *mapped_ptr = new slot_map;
                }
                catch(...)
                {
                    EXCEPTION_RECORD rec = {};
                    rec.ExceptionCode = STATUS_NO_MEMORY;
                    rec.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                    RaiseFailFastException(&rec, nullptr, 0);
                }
                s_mapping = mapping;
                UnmapViewOfFile(mapped_ptr);
            }
            else
            {
                slot_map * const * mapped_ptr = static_cast<slot_map * const *>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof(slot_map *)));
                if (!mapped_ptr)
                {
                    BOOL r = CloseHandle(mapping);
                    BOOST_LEAF_ASSERT(r), (void) r;
                    return;
                }
                global_slot_map<>::ptr = *mapped_ptr;
                UnmapViewOfFile(mapped_ptr);
                BOOL r = CloseHandle(mapping);
                BOOST_LEAF_ASSERT(r), (void) r;
            }
        }
        else if (dwReason == DLL_PROCESS_DETACH)
        {
            if (s_mapping != INVALID_HANDLE_VALUE)
            {
                delete global_slot_map<>::ptr;
                BOOL r = CloseHandle(s_mapping);
                BOOST_LEAF_ASSERT(r), (void) r;
                global_slot_map<>::ptr = nullptr;
                s_mapping = INVALID_HANDLE_VALUE;
            }
        }
    }
#ifdef _MSC_VER
#pragma data_seg(".CRT$XLB")
    PIMAGE_TLS_CALLBACK p_tls_callback = tls_callback;
#pragma data_seg()
#elif defined(__GNUC__)
    PIMAGE_TLS_CALLBACK p_tls_callback __attribute__((section(".CRT$XLB"))) = tls_callback;
#endif

    inline DWORD check_tls_slot_for_type_name(char const * type_name) noexcept
    {
        BOOST_LEAF_ASSERT(type_name && *type_name);
        slot_map const * sm = global_slot_map<>::ptr;
        BOOST_LEAF_ASSERT(sm);
        DWORD idx = sm->check(type_name);
        return idx;
    }

    inline DWORD get_existing_tls_slot_for_type_name(char const * type_name) noexcept
    {
        BOOST_LEAF_ASSERT(type_name && *type_name);
        slot_map const * sm = global_slot_map<>::ptr;
        BOOST_LEAF_ASSERT(sm);
        DWORD idx = sm->check(type_name);
        BOOST_LEAF_ASSERT(idx != TLS_OUT_OF_INDEXES);
        return idx;
    }

    inline DWORD get_tls_slot_for_type_name(char const * type_name)
    {
        BOOST_LEAF_ASSERT(type_name && *type_name);
        slot_map * sm = global_slot_map<>::ptr;
        BOOST_LEAF_ASSERT(sm);
        DWORD idx = sm->get(type_name);
        BOOST_LEAF_ASSERT(idx != TLS_OUT_OF_INDEXES);
        return idx;
    }

    template <class>
    struct t_
    {
    };

    template<class T>
    DWORD check_tls_index() noexcept
    {
        thread_local DWORD cached_idx = TLS_OUT_OF_INDEXES;
        if (cached_idx == TLS_OUT_OF_INDEXES)
            cached_idx = check_tls_slot_for_type_name(typeid(t_<T>).name());
        return cached_idx;
    }

    template<class T>
    DWORD get_existing_tls_index() noexcept
    {
        thread_local DWORD const cached_idx = get_existing_tls_slot_for_type_name(typeid(t_<T>).name());
        BOOST_LEAF_ASSERT(cached_idx != TLS_OUT_OF_INDEXES);
        return cached_idx;
    }

    template<class T>
    DWORD get_tls_index()
    {
        thread_local DWORD const cached_idx = get_tls_slot_for_type_name(typeid(t_<T>).name());
        BOOST_LEAF_ASSERT(cached_idx != TLS_OUT_OF_INDEXES);
        return cached_idx;
    }
}

namespace tls
{
    template <class T>
    T * read_ptr() noexcept
    {
        DWORD slot = detail::check_tls_index<T>();
        if (slot == TLS_OUT_OF_INDEXES)
            return nullptr;
        LPVOID value = TlsGetValue(slot);
        BOOST_LEAF_ASSERT(GetLastError() == ERROR_SUCCESS);
        return static_cast<T *>(value);
    }

    template <class T>
    void alloc_write_ptr(T * p)
    {
        DWORD slot = detail::get_tls_index<T>();
        BOOST_LEAF_ASSERT(slot != TLS_OUT_OF_INDEXES);
        BOOL r = TlsSetValue(slot, p);
        BOOST_LEAF_ASSERT(r), (void) r;
    }

    template <class T>
    void write_ptr(T * p) noexcept
    {
        DWORD slot = detail::get_existing_tls_index<T>();
        BOOST_LEAF_ASSERT(slot != TLS_OUT_OF_INDEXES);
        BOOL r = TlsSetValue(slot, p);
        BOOST_LEAF_ASSERT(r), (void) r;
    }

    inline unsigned read_current_error_id() noexcept
    {
        detail::slot_map const * sm = detail::global_slot_map<>::ptr;
        BOOST_LEAF_ASSERT(sm);
        DWORD slot = sm->error_id_slot();
        LPVOID value = TlsGetValue(slot);
        BOOST_LEAF_ASSERT(GetLastError() == ERROR_SUCCESS);
        return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(value));
    }

    inline void write_current_error_id(unsigned x) noexcept
    {
        detail::slot_map const * sm = detail::global_slot_map<>::ptr;
        BOOST_LEAF_ASSERT(sm);
        DWORD slot = sm->error_id_slot();
        BOOL r = TlsSetValue(slot, reinterpret_cast<void *>(static_cast<std::uintptr_t>(x)));
        BOOST_LEAF_ASSERT(r), (void) r;
    }
}

} }

#endif // BOOST_LEAF_CONFIG_TLS_WIN32_HPP_INCLUDED