#pragma once

#include <string>
#include <unordered_map>

namespace dsml {

class State
{
public:
    State(std::string config);

    int register_owner(std::string variable_owner, std::string location);

    int register_owner(std::string variable_owner, int socket);

    template <typename T>
    T get(std::string var);

    template <typename T>
    void get(std::string var, T& valueRet);

    template <typename T>
    void set(std::string var, T value);

private:
    std::unordered_map<std::string, void*> vars;

    template <typename T>
    void create(std::string var);
};

class Variable {

    

private:
    void* data;


};

}
