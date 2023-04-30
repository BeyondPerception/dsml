#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace dsml
{
    class State
    {
    public:
        State(std::string config, std::string program_name);

        ~State();

        int register_owner(std::string variable_owner, std::string owner_ip, int owner_port);

        int register_owner(std::string variable_owner, int socket);

        template <typename T>
        T get(std::string var)
        {
            check_var<T>(var);

            return *static_cast<T *>(vars[var].data);
        }

        // template <typename T>
        // void get(std::string var, T &value_ret);

        template <typename T>
        void set(std::string var, T value)
        {
            check_var<T>(var);

            *static_cast<T *>(vars[var].data) = value;
        }

    private:
        std::string self;

        enum Type
        {
            INT8,
            INT16,
            INT32,
            INT64,
            UINT8,
            UINT16,
            UINT32,
            UINT64,
            STRING
        };

        // Map from string Type to Type enum.
        std::unordered_map<std::string, Type> type_map = {{"INT8", INT8},
                                                          {"INT16", INT16},
                                                          {"INT32", INT32},
                                                          {"INT64", INT64},
                                                          {"UINT8", UINT8},
                                                          {"UINT16", UINT16},
                                                          {"UINT32", UINT32},
                                                          {"UINT64", UINT64},
                                                          {"STRING", STRING}};

        struct Variable
        {
            Type type;
            bool is_array;
            int size; // if isArray, then the number of elements in the array.
            std::string owner;
            int owner_socket;
            void *data;
        };

        std::unordered_map<std::string, Variable> vars;

        void create_var(std::string var, Type type, std::string owner, bool is_array);

        template <typename T>
        void check_var(std::string var)
        {
            if (vars.find(var) == vars.end())
            {
                throw std::runtime_error("Variable " + var + " does not exist.");
            }

            if (vars[var].owner_socket < 0 && vars[var].owner != self)
            {
                throw std::runtime_error("Variable " + var + " has no owner registered.");
            }

            Variable v = vars[var];

            switch (v.type)
            {
            case INT8:
                if (v.is_array)
                {
                    if (!std::is_same_v<T, std::vector<int8_t>>)
                        throw std::runtime_error("Variable '" + var + "' is of type std::vector<int8_t>.");
                }
                else
                {
                    if (!std::is_same_v<T, int8_t>)
                        throw std::runtime_error("Variable '" + var + "' is of type int8_t.");
                }
                break;
            case INT16:
                if (v.is_array)
                {
                    if (!std::is_same_v<T, std::vector<int16_t>>)
                        throw std::runtime_error("Variable '" + var + "' is of type std::vector<int16_t>.");
                }
                else
                {
                    if (!std::is_same_v<T, int16_t>)
                        throw std::runtime_error("Variable '" + var + "' is of type int16_t.");
                }
                break;
            case INT32:
                if (v.is_array)
                {
                    if (!std::is_same_v<T, std::vector<int32_t>>)
                        throw std::runtime_error("Variable '" + var + "' is of type std::vector<int32_t>.");
                }
                else
                {
                    if (!std::is_same_v<T, int32_t>)
                        throw std::runtime_error("Variable '" + var + "' is of type int32_t.");
                }
                break;
            case INT64:
                if (v.is_array)
                {
                    if (!std::is_same_v<T, std::vector<int64_t>>)
                        throw std::runtime_error("Variable '" + var + "' is of type std::vector<int64_t>.");
                }
                else
                {
                    if (!std::is_same_v<T, int64_t>)
                        throw std::runtime_error("Variable '" + var + "' is of type int64_t.");
                }
                break;
            case UINT8:
                if (v.is_array)
                {
                    if (!std::is_same_v<T, std::vector<uint8_t>>)
                        throw std::runtime_error("Variable '" + var + "' is of type std::vector<uint8_t>.");
                }
                else
                {
                    if (!std::is_same_v<T, uint8_t>)
                        throw std::runtime_error("Variable '" + var + "' is of type uint8_t.");
                }
                break;
            case UINT16:
                if (v.is_array)
                {
                    if (!std::is_same_v<T, std::vector<uint16_t>>)
                        throw std::runtime_error("Variable '" + var + "' is of type std::vector<uint16_t>.");
                }
                else
                {
                    if (!std::is_same_v<T, uint16_t>)
                        throw std::runtime_error("Variable '" + var + "' is of type uint16_t.");
                }
                break;
            case UINT32:
                if (v.is_array)
                {
                    if (!std::is_same_v<T, std::vector<uint32_t>>)
                        throw std::runtime_error("Variable '" + var + "' is of type std::vector<uint32_t>.");
                }
                else
                {
                    if (!std::is_same_v<T, uint32_t>)
                        throw std::runtime_error("Variable '" + var + "' is of type uint32_t.");
                }
                break;
            case UINT64:
                if (v.is_array)
                {
                    if (!std::is_same_v<T, std::vector<uint64_t>>)
                        throw std::runtime_error("Variable '" + var + "' is of type std::vector<uint64_t>.");
                }
                else
                {
                    if (!std::is_same_v<T, uint64_t>)
                        throw std::runtime_error("Variable '" + var + "' is of type uint64_t.");
                }
                break;
            case STRING:
                if (!std::is_same_v<T, uint64_t>)
                    throw std::runtime_error("Variable '" + var + "' is of type std::string.");
                break;
            default:
                break;
            }
        }
    };
}
