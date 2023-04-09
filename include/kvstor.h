// kvstor.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <atomic>
#include <cassert>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>


namespace kvstor
{

    template <class key_type, class value_type>
    struct traits_t
    {
        using hash_t = std::hash<key_type>;
        using kequal_t = std::equal_to<key_type>;

        template <class item_type>
        using alloc_t = std::allocator<item_type>;
    };

    template
    <
        class key_type,
        class value_type,
        class traits_type = traits_t<key_type, value_type>
    >
    class storage_t final
    {
    public:
        using key_t = key_type;
        using value_t = value_type;
        using hash_t = typename traits_type::hash_t;
        using kequal_t = typename traits_type::kequal_t;

        explicit storage_t(size_t max_size) noexcept;
        storage_t(const std::vector<std::pair<key_t, value_t>> & dump_data, size_t max_size);
        storage_t(const storage_t &) = delete;
        storage_t(storage_t &&) = delete;
        ~storage_t() noexcept;

        storage_t operator=(const storage_t &) = delete;
        storage_t operator=(storage_t &&) = delete;

        void push(const key_t & key, value_t && value);
        void push(const key_t & key, const value_t & value);

        bool compare_exchange(const key_t & key, value_t && desired, std::optional<value_t> & expected);
        bool compare_exchange(const key_t & key, const value_t & desired, std::optional<value_t> & expected);

        std::optional<value_t> find(const key_t & key) const;
        std::optional<value_t> first() const;
        std::optional<value_t> last() const;

        void map(std::function<void (const key_t & key, value_t & value)> func);
        void map(std::function<void (const key_t & key, const value_t & value)> func) const;

        size_t size() const noexcept;
        bool empty() const noexcept;
        size_t max_size() const noexcept;

        void erase(const key_t& key);
        void clear() noexcept;

        std::vector<std::pair<key_t, value_t>> dump() const;

    private:
        struct item_t
        {
            item_t(const value_type & value_, const key_t& key_)
                : value(value_)
                , key(key_)
            {
            }

            item_t(value_type && value_, const key_t & key_)
            :   value(std::move(value_))
            ,   key(key_)
            {
            }

            value_t value;
            key_t   key;
        };

        using list_alloc_t = typename traits_t<key_t, value_t>::template alloc_t<item_t>;
        using list_t = std::list<item_t, list_alloc_t>;
        using index_item_t = typename list_t::iterator;
        using index_pair_t = std::pair<const key_t, index_item_t>;
        using index_alloc_t = typename traits_t<key_t, value_t>::template alloc_t<index_pair_t>;
        using index_t = std::unordered_map<key_t, index_item_t, hash_t, kequal_t, index_alloc_t>;

        void apply_new(const key_t & key, value_t && value, typename index_t::iterator found);
        void fix_size();
        void build_from_dump(const std::vector<std::pair<key_t, value_t>> & dump_data);
        bool compare_with(typename index_t::iterator found, std::optional<value_t> & expected);

        list_t  m_data;
        index_t m_index;

        mutable std::mutex  m_lock;

        std::atomic<size_t> m_size;
        const size_t        m_max_size;
    };


    template <class key_type, class value_type, class traits_type>
    inline storage_t<key_type, value_type, traits_type>::storage_t(size_t max_size) noexcept
    :   m_data()
    ,   m_index()
    ,   m_lock()
    ,   m_size(0)
    ,   m_max_size(max_size)
    {
    }


    template <class key_type, class value_type, class traits_type>
    inline storage_t<key_type, value_type, traits_type>::storage_t
    (
        const std::vector<std::pair<key_t, value_t>>  & dump_data,
        size_t                                          max_size
    )
    :   storage_t(max_size)
    {
        build_from_dump(dump_data);
    }


    template <class key_type, class value_type, class traits_type>
    inline storage_t<key_type, value_type, traits_type>::~storage_t() noexcept
    {
        clear();
    }


    template <class key_type, class value_type, class traits_type>
    void storage_t<key_type, value_type, traits_type>::push(const key_t & key, value_t && value)
    {
        const std::lock_guard guard{ m_lock };

        auto found = m_index.find(key);
        apply_new(key, std::move(value), found);
        fix_size();
    }


    template <class key_type, class value_type, class traits_type>
    inline void storage_t<key_type, value_type, traits_type>::push(const key_t & key, const value_t & value)
    {
        push(key, std::move(value_t(value)));
    }


    template <class key_type, class value_type, class traits_type>
    bool storage_t<key_type, value_type, traits_type>::compare_exchange
    (
        const key_t              & key,
        value_t                 && desired,
        std::optional<value_t>   & expected
    )
    {
        const std::lock_guard guard{ m_lock };

        auto found = m_index.find(key);
        if (!compare_with(found, expected))
            return false;

        apply_new(key, std::move(desired), found);
        fix_size();

        return true;
    }


    template <class key_type, class value_type, class traits_type>
    inline bool storage_t<key_type, value_type, traits_type>::compare_exchange
    (
        const key_t              & key,
        const value_t            & desired,
        std::optional<value_t>   & expected
    )
    {
        return compare_exchange(key, std::move(value_t(desired)), expected);
    }


    template <class key_type, class value_type, class traits_type>
    inline std::optional<value_type> storage_t<key_type, value_type, traits_type>::find(const key_t & key) const
    {
        const std::lock_guard guard{ m_lock };
        auto found = m_index.find(key);

        if (found == m_index.end())
            return std::optional<value_t>{};

        assert(found->second->key == key);

        return std::optional<value_t>{ found->second->value };
    }


    template <class key_type, class value_type, class traits_type>
    inline std::optional<value_type> storage_t<key_type, value_type, traits_type>::first() const
    {
        const std::lock_guard guard{ m_lock };

        if (m_size == 0)
        {
            assert(m_index.empty());
            assert(m_data.empty());
            return std::optional<value_t>{};
        }

        return std::optional<value_t>{ m_data.front().value };
    }


    template <class key_type, class value_type, class traits_type>
    inline std::optional<value_type> storage_t<key_type, value_type, traits_type>::last() const
    {
        const std::lock_guard guard{ m_lock };

        if (m_size == 0)
        {
            assert(m_index.empty());
            assert(m_data.empty());
            return std::optional<value_t>{};
        }

        return std::optional<value_t>{ m_data.back().value };
    }


    template <class key_type, class value_type, class traits_type>
    void storage_t<key_type, value_type, traits_type>::map(std::function<void (const key_t & key, value_t & value)> func)
    {
        const std::lock_guard guard{ m_lock };

        for (item_t & item : m_data)
            func(item.key, item.value);
    }


    template <class key_type, class value_type, class traits_type>
    void storage_t<key_type, value_type, traits_type>::map(std::function<void (const key_t & key, const value_t & value)> func) const
    {
        const std::lock_guard guard{ m_lock };

        for (const item_t & item : m_data)
            func(item.key, item.value);
    }


    template <class key_type, class value_type, class traits_type>
    inline size_t storage_t<key_type, value_type, traits_type>::size() const noexcept
    {
        return m_size;
    }


    template <class key_type, class value_type, class traits_type>
    inline bool storage_t<key_type, value_type, traits_type>::empty() const noexcept
    {
        return m_size == 0;
    }


    template <class key_type, class value_type, class traits_type>
    inline size_t storage_t<key_type, value_type, traits_type>::max_size() const noexcept
    {
        return m_max_size;
    }


    template <class key_type, class value_type, class traits_type>
    void storage_t<key_type, value_type, traits_type>::erase(const key_t & key)
    {
        const std::lock_guard guard{ m_lock };
        auto found = m_index.find(key);

        if (found != m_index.end())
        {
            assert(found->second->key == key);
            m_data.erase(found->second);
            m_index.erase(found);

            m_size = m_data.size();
            assert(m_size == m_index.size());
        }
    }


    template <class key_type, class value_type, class traits_type>
    void storage_t<key_type, value_type, traits_type>::clear() noexcept
    {
        try
        {
            const std::lock_guard guard{ m_lock };
            m_index.clear();
            m_data.clear();
            m_size = 0;
        }
        catch (...)
        {
            // ignore unexpected exception in release
            assert(false);
        }
    }


    template <class key_type, class value_type, class traits_type>
    std::vector<std::pair<key_type, value_type>> storage_t<key_type, value_type, traits_type>::dump() const
    {
        std::vector<std::pair<key_t, value_t>> dump_data;
        dump_data.reserve(size());

        auto do_dump = [&dump_data](key_t key, const value_t& value)
        {
            dump_data.emplace_back(key, value);
        };

        map(do_dump);

        return dump_data;
    }


    template <class key_type, class value_type, class traits_type>
    void storage_t<key_type, value_type, traits_type>::apply_new
    (
        const key_t                  & key,
        value_t                     && value,
        typename index_t::iterator     found
    )
    {
        m_data.emplace_front(std::move(value), key);

        if (found == m_index.end())
        {
            m_index.emplace(key, m_data.begin());
        }
        else
        {
            assert(found->second->key == key);
            m_data.erase(found->second);
            found->second = m_data.begin();
        }
    }


    template <class key_type, class value_type, class traits_type>
    void storage_t<key_type, value_type, traits_type>::fix_size()
    {
        if (m_data.size() > m_max_size)
        {
            m_index.erase(m_data.back().key);
            m_data.pop_back();
        }

        m_size = m_data.size();
        assert(m_size == m_index.size());
    }


    template <class key_type, class value_type, class traits_type>
    void storage_t<key_type, value_type, traits_type>::build_from_dump
    (
        const std::vector<std::pair<key_t, value_t>> & dump_data
    )
    {
        const size_t dump_size = dump_data.size();
        const size_t offset = dump_size < m_max_size ? 0 : dump_size - m_max_size;

        for (auto it = dump_data.rbegin() + offset; it < dump_data.rend(); ++it)
        {
            const key_type & key = it->first;
            value_t value = it->second;
            const auto found = m_index.find(key);

            apply_new(key, std::move(value), found);
            fix_size();
        }
    }


    template <class key_type, class value_type, class traits_type>
    bool storage_t<key_type, value_type, traits_type>::compare_with
    (
        typename index_t::iterator    found,
        std::optional<value_t>      & expected
    )
    {
        if (found == m_index.end() || !expected)
        {
            if (found == m_index.end() && !expected)
                return true;

            if (expected)
            {
                expected.reset();
                return false;
            }

            expected = found->second->value;
            return false;
        }

        if (*expected == found->second->value)
            return true;

        expected = found->second->value;
        return false;
    }

}   // namespace kvstor
