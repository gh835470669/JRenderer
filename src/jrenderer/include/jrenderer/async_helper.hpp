#pragma once

#include <future>

namespace jre
{
    template <typename R>
    bool is_ready(std::future<R> const &f)
    {
        return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }
}