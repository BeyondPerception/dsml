#include <fstream>
#include <sstream>

#include <dsml.hpp>

void dsml::State::process_var(std::string var, std::string type, std::string owner)
{
    if (type == "INT8")
    {
        create<int8_t>(var);
    }
    else if (type == "INT16")
    {
        create<int16_t>(var);
    }
    else if (type == "INT32")
    {
        create<int32_t>(var);
    }
    else if (type == "INT64")
    {
        create<int64_t>(var);
    }
    else if (type == "UINT8")
    {
        create<uint8_t>(var);
    }
    else if (type == "UINT16")
    {
        create<uint16_t>(var);
    }
    else if (type == "UINT32")
    {
        create<uint32_t>(var);
    }
    else if (type == "UINT64")
    {
        create<uint64_t>(var);
    }
    else
    {
        throw std::runtime_error("Invalid type in configuration file.");
    }
}

dsml::State::State(std::string config)
{
    std::ifstream config_file(config);
    std::string line;
    while (std::getline(config_file, line))
    {
        std::istringstream iss(line);
        std::string var, type, owner;

        if (!(iss >> var >> type >> owner))
        {
            throw std::runtime_error("Invalid line in configuration file.");
            break;
        }

        process_var(var, type, owner);
    }
}

int dsml::State::register_owner(std::string program_name, std::string program_ip)
{
    return 0;
}

template <typename T>
inline T dsml::State::get(std::string var)
{
    return *static_cast<T *>(vars[var]);
}

template <typename T>
void dsml::State::set(std::string var, T value)
{
}

template <typename T>
void dsml::State::create(std::string var)
{
}
