#include <dsml.hpp>

dsml::State::State(std::string config)
{
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
