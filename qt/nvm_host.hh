#include "hal/i_nvm.hh"

#include <map>
#include <string>

class NvmHost : public hal::INvm
{
public:
    NvmHost(std::string_view file_name);

    void Commit() final;
    void EraseAll() final;
    void EraseKey(const char* key) final;

private:
    std::optional<uint32_t> GetUint32_t(const char* key) final;
    void SetUint32_t(const char* key, uint32_t value) final;


    const std::string m_file_name;
    std::map<std::string, uint32_t> m_storage_cache;
};
