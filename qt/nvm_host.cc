#include "nvm_host.hh"

#include <fstream>
#include <iostream>
#include <sstream>

NvmHost::NvmHost(std::string_view file_name) :
    m_file_name(file_name)
{
    // Read the file (key : value pairs) and populate m_storage_cache
    std::ifstream file(m_file_name);
    if (file.is_open())
    {
        std::string line;

        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string key;
            uint32_t value;

            if (std::getline(iss, key, ':') && iss >> value)
            {
                m_storage_cache[key] = value;
            }
        }

        file.close();
    }
}

void
NvmHost::Commit()
{
    // Store the cache to file
    std::ofstream file(m_file_name);
    if (file.is_open())
    {
        for (const auto& [key, value] : m_storage_cache)
        {
            file << key << ':' << value << '\n';
        }

        file.close();
    }
}

void
NvmHost::EraseAll()
{
    m_storage_cache.clear();
}

void
NvmHost::EraseKey(const char* key)
{
    m_storage_cache.erase(key);
}


std::optional<uint32_t>
NvmHost::GetUint32_t(const char* key)
{
    if (auto it = m_storage_cache.find(key); it != m_storage_cache.end())
    {
        return it->second;
    }

    return std::nullopt;
}

void
NvmHost::SetUint32_t(const char* key, uint32_t value)
{
    m_storage_cache[key] = value;
}
