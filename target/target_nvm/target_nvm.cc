#include "target_nvm.hh"

#include <nvs_flash.h>


NvmTarget::NvmTarget()
{
    auto err = Open();
    if (err == ESP_ERR_NVS_NOT_INITIALIZED)
    {
        nvs_flash_init();

        err = Open();
    }
    assert(err == ESP_OK);
}

esp_err_t
NvmTarget::Open()
{
    return nvs_open("storage", NVS_READWRITE, &m_handle);
}

void
NvmTarget::Commit()
{
    nvs_commit(m_handle);
}

void
NvmTarget::EraseAll()
{
    nvs_erase_all(m_handle);
}

void
NvmTarget::EraseKey(const char* key)
{
    nvs_erase_key(m_handle, key);
}

std::optional<uint32_t>
NvmTarget::GetUint32_t(const char* key)
{
    uint32_t value;
    auto err = nvs_get_u32(m_handle, key, &value);
    if (err == ESP_OK)
    {
        return value;
    }

    return std::nullopt;
}

void
NvmTarget::SetUint32_t(const char* key, uint32_t value)
{
    // Hope for the best and ignore the error
    nvs_set_u32(m_handle, key, value);
}
