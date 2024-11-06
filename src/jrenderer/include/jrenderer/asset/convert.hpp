#pragma once

namespace jre
{
    template <typename To, typename From>
    To convert_to(const From &from) { return static_cast<To>(from); }
}