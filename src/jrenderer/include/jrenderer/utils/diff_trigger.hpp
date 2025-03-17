#pragma once

#include <functional>

namespace jre
{
    template <typename ValueType>
    class DiffTrigger
    {
    public:
        DiffTrigger() : m_value() {}
        DiffTrigger(ValueType value) : m_value(value) {}

        bool update(ValueType value)
        {
            return std::exchange(m_value, value) != value;
        }

        const ValueType &value() const { return m_value; }

    private:
        ValueType m_value;
    };
}