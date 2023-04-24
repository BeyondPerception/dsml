#include <dsml.hpp>

DSML::DSML(std::string config)
{
}

int DSML::register_owner(std::string program_name, std::string program_ip)
{
    return 0;
}

template <typename T>
inline T DSML::get(std::string var)
{
    return *static_cast<T *>(vars[var]);
}

template <typename T>
void DSML::set(std::string var, T value)
{
}

template <typename T>
void DSML::create(std::string var)
{
}

template <typename T>
void DSML::create(std::string var, T value)
{
}
