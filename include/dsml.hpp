#pragma once

#include <string>
#include <unordered_map>

class DSML
{
public:
    DSML(std::string config);

    int register_owner(std::string variable_owner, std::string owner_ip);

    int register_owner(std::string variable_owner, int socket);

    template <typename T>
    T get(std::string var);

    template <typename T>
    void get(std::string var, T &value_ret);

    template <typename T>
    void set(std::string var, T value);

private:
    std::unordered_map<std::string, void *> vars;

    void process_var(std::string var, std::string type, std::string owner);

    template <typename T>
    void create(std::string var);
};
