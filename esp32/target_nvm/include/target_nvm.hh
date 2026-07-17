#pragma once

#include "hal/i_nvm.hh"

#include <nvs.h>

class NvmTarget : public hal::INvm
{
public:
    NvmTarget();

    void Commit() final;
    void EraseAll() final;
    void EraseKey(const char* key) final;

private:
    std::optional<uint32_t> GetUint32_t(const char* key) final;
    void SetUint32_t(const char* key, uint32_t value) final;
    esp_err_t Open();

    nvs_handle_t m_handle;
};
