#pragma

#include <functional>

namespace jre
{
    template <typename ValueType, typename TriggerFunc = std::function<void(ValueType, ValueType)>, bool ImmediateTrigger = true>
    class DiffTrigger
    {
    public:
        DiffTrigger(ValueType value, TriggerFunc trigger_func) : m_value(value), m_trigger_func(std::move(trigger_func)) {}

        void set_value(ValueType value)
        {
            if (m_value != value)
            {
                m_trigger_func(std::exchange(m_value, value), value);
            }
        }

        const ValueType &value() const { return m_value; }

    private:
        ValueType m_value;
        TriggerFunc m_trigger_func;
    };

    template <typename ValueType, typename TriggerFunc>
    class DiffTrigger<ValueType, TriggerFunc, false>
    {
    public:
        DiffTrigger(ValueType value, TriggerFunc trigger_func) : m_value(value), m_trigger_func(std::move(trigger_func)) {}

        void set_value(ValueType value)
        {
            if (m_value != value)
            {
                std::exchange(m_value, value);
                m_is_dirty = true;
            }
        }

        const ValueType &value() const { return m_value; }
        bool is_dirty() const { return m_is_dirty; }
        void reset_dirty() { m_is_dirty = false; }

        void trigger()
        {
            if (m_is_dirty)
            {
                m_trigger_func(m_value);
                m_is_dirty = false;
            }
        }

    private:
        ValueType m_value;
        TriggerFunc m_trigger_func;
        bool m_is_dirty = false;
    };
}