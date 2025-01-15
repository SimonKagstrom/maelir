#pragma once

#include <cstdint>
#include <optional>

namespace hal
{

class INvm
{
public:
    virtual ~INvm() = default;

    template <typename T>
    std::optional<T> Get(const char* key)
    {
        static_assert(sizeof(T) <= sizeof(uint32_t), "T must fit in a uint32_t");

        if (auto raw = GetUint32_t(key); raw)
        {
            T value = {};

            memcpy(&value, &*raw, sizeof(T));

            return value;
        }

        return std::nullopt;
    }

    template <typename T>
    void Set(const char* key, T value)
    {
        static_assert(sizeof(T) <= sizeof(uint32_t), "T must fit in a uint32_t");

        uint32_t raw = 0;

        memcpy(&raw, &value, sizeof(T));

        SetUint32_t(key, raw);
    }

    virtual void Commit() = 0;

    virtual void EraseAll() = 0;

    virtual void EraseKey(const char* key) = 0;

protected:
    virtual std::optional<uint32_t> GetUint32_t(const char* key) = 0;
    virtual void SetUint32_t(const char* key, uint32_t value) = 0;
};

} // namespace hal
