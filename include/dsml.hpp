#pragma once

#include <atomic>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace dsml
{
    class State
    {
    public:
        State(std::string config, std::string program_name, int port = 0);

        ~State();

        int register_owner(std::string variable_owner, std::string owner_ip, int owner_port);

        int register_owner(std::string variable_owner, int socket);

        template <typename T>
        T get(std::string var)
        {
            T v;
            get(var, v);

            return v;
        }

        template <typename T>
        void get(std::string var, T &ret_value)
        {
            check_var_type<T>(var);

            // Tell the owner that we are interested in this variable.
            if (interested_vars.find(var) == interested_vars.end())
            {
                send_interest(vars[var].owner_socket, var);
                interested_vars.insert(var);
            }

            ret_value = *static_cast<T *>(vars[var].data);
        }

        template <typename T>
        void get(std::string var, std::vector<T> &ret_value)
        {
            check_var_type<std::vector<T>>(var);

            // Tell the owner that we are interested in this variable.
            if (interested_vars.find(var) == interested_vars.end())
            {
                send_interest(vars[var].owner_socket, var);
                interested_vars.insert(var);
            }

            ret_value = std::vector<T>(static_cast<T *>(vars[var].data),
                                       static_cast<T *>(vars[var].data) + vars[var].size);
        }

        void get(std::string var, std::string &ret_value)
        {
            std::vector<char> v;
            get(var, v);
            ret_value = std::string(v.begin(), v.end());
        }

        template <typename T>
        void set(std::string var, T value)
        {
            check_var_type<T>(var);

            // Check if this program owns the variable.
            if (self != vars[var].owner)
            {
                throw std::runtime_error("Variable " + var + " is not owned by this program.");
            }

            *static_cast<T *>(vars[var].data) = value;

            // Send the message to all subscribers.
            for (auto &socket : subscriber_list[var])
            {
                send_message(socket, var);
            }
        }

        template <typename T>
        void set(std::string var, std::vector<T> value)
        {
            check_var_type<std::vector<T>>(var);

            // Check if this program owns the variable.
            if (self != vars[var].owner)
            {
                throw std::runtime_error("Variable " + var + " is not owned by this program.");
            }

            free(vars[var].data);
            vars[var].data = malloc(value.size() * sizeof(T));
            vars[var].size = value.size();
            memcpy(vars[var].data, value.data(), value.size() * sizeof(T));

            // Send the message to all subscribers.
            for (auto &socket : subscriber_list[var])
            {
                send_message(socket, var);
            }
        }

        void set(std::string var, std::string value)
        {
            set(var, std::vector<char>(value.begin(), value.end()));
        }

    private:
        std::string self;

        std::atomic<bool> recv_thread_running;

        std::atomic<bool> accept_thread_running;

        std::atomic<bool> identification_thread_running;

        std::thread recv_thread;

        std::thread accept_thread;

        std::thread identification_thread;

        std::unordered_set<std::string> interested_vars;

        int server_socket;

        int recv_wakeup_fd;
        int identification_wakeup_fd;

        std::mutex socket_list_m;
        std::vector<int> recv_socket_list;

        std::mutex client_socket_list_m;
        std::vector<int> client_socket_list;

        std::unordered_map<std::string, std::vector<int>> subscriber_list;

        enum Type : uint8_t
        {
            INT8,
            INT16,
            INT32,
            INT64,
            UINT8,
            UINT16,
            UINT32,
            UINT64,
            STRING,
        };

        size_t type_size(Type type);

        // Maps a string to a `Type` enum.
        std::unordered_map<std::string, Type> type_map = {
            {"INT8", INT8},
            {"INT16", INT16},
            {"INT32", INT32},
            {"INT64", INT64},
            {"UINT8", UINT8},
            {"UINT16", UINT16},
            {"UINT32", UINT32},
            {"UINT64", UINT64},
            {"STRING", STRING},
        };

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

        int recv_message(int socket);

        int send_message(int socket, std::string var);

        int recv_interest(int socket);

        int send_interest(int socket, std::string var);

        int accept_connection();

        template <typename T>
        void check_var_type(std::string var)
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
                if (!std::is_same_v<T, std::vector<char>>)
                    throw std::runtime_error("Variable '" + var + "' is of type std::string.");
                break;
            default:
                break;
            }
        }
    };
}
