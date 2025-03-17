#pragma once

#include <memory>
#include <unordered_map>
#include <mutex>
#include <cappuccino/lock.hpp>

namespace jre
{
    template <typename Key, typename Value, typename ValueCreator, cappuccino::thread_safe ThreadSafe = cappuccino::thread_safe::no>
    class Resources
    {
    public:
        Resources(const ValueCreator &value_loader = ValueCreator()) : m_value_loader(value_loader) {}

        bool contains(const Key &key)
        {
            std::lock_guard lock(m_mutex);
            return m_resources.contains(key);
        }

        Value &get_or_create(const Key &key)
        {
            std::lock_guard lock(m_mutex);
            if (!m_resources.contains(key))
            {
                return m_resources.emplace(key, m_value_loader(key));
            }
            return m_resources[key];
        }

        void insert(const Key &key, Value &&value)
        {
            std::lock_guard lock(m_mutex);
            m_resources.emplace(key, value);
        }

        // delete all no other referenced resource (use count == 1)
        void purge()
        {
            std::lock_guard lock(m_mutex);
            m_resources.erase(std::remove_if(m_resources.begin(), m_resources.end(), [](const auto &pair)
                                             { return pair.second.use_count() == 1; }));
        }

    private:
        ValueCreator m_value_loader;
        cappuccino::mutex<ThreadSafe> m_mutex;
        std::unordered_map<Key, Value> m_resources;
    };

}