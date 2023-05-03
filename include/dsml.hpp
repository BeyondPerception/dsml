#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <unistd.h>

namespace dsml
{
    class State
    {
    public:
        /**
         * Construct a new `State` object.
         *
         * @param config Path to the configuration file.
         * @param program_name Name of the program.
         * @param port Port on which to listen.
         */
        State(std::string config, std::string program_name, int port = 0);

        /**
         * Destroy the `State` object.
         */
        ~State();

        /**
         * Register the owner of a variable.
         *
         * @param variable_owner Name of the owner program.
         * @param owner_ip IP address of the owner program.
         * @param owner_port Port of the owner program.
         * @return 0 on success, -1 on failure.
         */
        int register_owner(std::string variable_owner, std::string owner_ip, int owner_port);

        /**
         * Register the owner of a variable.
         *
         * @param variable_owner Name of the owner program.
         * @param socket Socket of the owner program.
         * @return 0 on success, -1 on failure.
         */
        int register_owner(std::string variable_owner, int socket);

        /**
         * Get the variable stored in the state.
         *
         * @tparam T Type of the variable.
         * @param var Name of the variable.
         * @return The variable.
         */
        template <typename T>
        T get(std::string var)
        {
            T v;
            get(var, v);
            return v;
        }

        /**
         * Get the variable stored in the state.
         *
         * @tparam T Type of the variable.
         * @param var Name of the variable.
         * @param ret_value Where to store the variable.
         */
        template <typename T>
        void get(std::string var, T &ret_value)
        {
            std::unique_lock lk(var_locks[var]);

            check_var_type<T>(var);

            // Tell the owner that we are interested in this variable.
            if (vars[var].owner != self && interested_vars.find(var) == interested_vars.end())
            {
                if (send_interest(vars[var].owner_socket, var) < 0)
                {
                    // Do some error reporting.
                }
                interested_vars.insert(var);
                var_cvs[var].wait(lk);
            }

            ret_value = *static_cast<T *>(vars[var].data);
        }

        /**
         * Get the variable stored in the state.
         *
         * @tparam T Type of the variable.
         * @param var Name of the variable.
         * @param ret_value Where to store the variable.
         */
        template <typename T>
        void get(std::string var, std::vector<T> &ret_value)
        {
            std::unique_lock lk(var_locks[var]);

            check_var_type<std::vector<T>>(var);

            // Tell the owner that we are interested in this variable.
            if (vars[var].owner != self && interested_vars.find(var) == interested_vars.end())
            {
                if (send_interest(vars[var].owner_socket, var) < 0)
                {
                    // Do some error reporting.
                }
                interested_vars.insert(var);
                var_cvs[var].wait(lk);
            }

            ret_value = std::vector<T>(static_cast<T *>(vars[var].data),
                                       static_cast<T *>(vars[var].data) + vars[var].size);
        }

        /**
         * Get the variable stored in the state.
         *
         * @param var Name of the variable.
         * @param ret_value Where to store the variable.
         */
        void get(std::string var, std::string &ret_value)
        {
            std::vector<char> v;
            get(var, v);
            ret_value = std::string(v.begin(), v.end());
        }

        /**
         * Set the variable stored in the state.
         *
         * @tparam T Type of the variable.
         * @param var Name of the variable.
         * @param value New value of the variable.
         */
        template <typename T>
        void set(std::string var, T value)
        {
            std::unique_lock lk(var_locks[var]);

            check_var_type<T>(var);

            // Check if this program owns the variable.
            if (self != vars[var].owner)
            {
                request_update(vars[var].owner_socket, var, &value, sizeof(value));
                return;
            }

            *static_cast<T *>(vars[var].data) = value;
            var_cvs[var].notify_all();

            lk.unlock();
            notify_subscribers(var);
        }

        /**
         * Set the variable stored in the state.
         *
         * @tparam T Type of the variable.
         * @param var Name of the variable.
         * @param value New value of the variable.
         */
        template <typename T>
        void set(std::string var, std::vector<T> value)
        {
            std::unique_lock lk(var_locks[var]);

            check_var_type<std::vector<T>>(var);

            // Check if this program owns the variable.
            if (self != vars[var].owner)
            {
                request_update(vars[var].owner_socket, var, value.data(), value.size() * sizeof(T));
                return;
            }

            free(vars[var].data);
            vars[var].data = malloc(value.size() * sizeof(T));
            vars[var].size = value.size();
            memcpy(vars[var].data, value.data(), value.size() * sizeof(T));
            var_cvs[var].notify_all();

            lk.unlock();
            notify_subscribers(var);
        }

        /**
         * Set the variable stored in the state.
         *
         * @param var Name of the variable.
         * @param value New value of the variable.
         */
        void set(std::string var, std::string value)
        {
            set(var, std::vector<char>(value.begin(), value.end()));
        }

        /**
         * Waits indefinitely until `var` is changed.
         *
         * @param var Name of the variable.
         */
        void wait(std::string var)
        {
            std::unique_lock lk(var_locks[var]);
            var_cvs[var].wait(lk);
        }

        /**
         * Waits for `rel_time` or until `var` is changed.
         *
         * @tparam Rep Number of ticks.
         * @tparam Period Tick period.
         * @param var Name of the variable.
         * @param rel_time Maximum duration to wait.
         * @return Whether the variable was changed.
         */
        template <class Rep, class Period>
        bool wait_for(std::string var, const std::chrono::duration<Rep, Period> &rel_time)
        {
            std::unique_lock lk(var_locks[var]);
            return var_cvs[var].wait_for(lk, rel_time) == std::cv_status::no_timeout;
        }

    private:
        /**
         * Self program name.
         */
        std::string self;

        /**
         * Whether the `recv_thread` is running.
         */
        std::atomic<bool> recv_thread_running;

        /**
         * Whether the `accept_thread` is running.
         */
        std::atomic<bool> accept_thread_running;

        /**
         * Whether the `identification_thread` is running.
         */
        std::atomic<bool> identification_thread_running;

        /**
         * Thread for receiving data from other programs.
         */
        std::thread recv_thread;

        /**
         * Thread for accepting new connections.
         */
        std::thread accept_thread;

        /**
         * Thread for identifying other programs.
         */
        std::thread identification_thread;

        /**
         * Set of variables that this program is interested in.
         */
        std::unordered_set<std::string> interested_vars;

        /**
         * Socket for the server.
         */
        int server_socket;

        /**
         * File descriptors for waking up the threads.
         */
        int recv_wakeup_fd;
        int identification_wakeup_fd;

        /**
         * Mutex for the `recv_socket_list` list.
         */
        std::mutex socket_list_m;
        std::vector<int> recv_socket_list;

        /**
         * Mutex for the `client_socket_list` list.
         */
        std::mutex client_socket_list_m;
        std::vector<int> client_socket_list;

        /**
         * Mutex for the `subscriber_list` list.
         */
        std::mutex subscriber_list_m;
        std::unordered_map<std::string, std::vector<int>> subscriber_list;

        /**
         * Enumerates the types of variables.
         */
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
            FLOAT,
            DOUBLE,
            STRING,
        };

        /**
         * Get the size of a type.
         *
         * @param type Type of variable.
         * @return Size of the type.
         */
        size_t type_size(Type type);

        /**
         * Maps a string to a `Type` enum.
         */
        std::unordered_map<std::string, Type> type_map = {
            {"INT8", INT8},
            {"INT16", INT16},
            {"INT32", INT32},
            {"INT64", INT64},
            {"UINT8", UINT8},
            {"UINT16", UINT16},
            {"UINT32", UINT32},
            {"UINT64", UINT64},
            {"FLOAT", FLOAT},
            {"DOUBLE", DOUBLE},
            {"STRING", STRING},
        };

        /**
         * Structure for storing a variable.
         */
        struct Variable
        {
            Type type;
            bool is_array;
            int size; // if isArray, then the number of elements in the array.
            std::string owner;
            int owner_socket;
            void *data;
        };

        /**
         * Variable condition variables and locks.
         */
        std::unordered_map<std::string, std::condition_variable> var_cvs;
        std::unordered_map<std::string, std::mutex> var_locks;

        /**
         * Map of variables.
         */
        std::unordered_map<std::string, Variable> vars;

        /**
         * Create a variable.
         *
         * @param var Name of the variable.
         * @param type Type of the variable.
         * @param owner Name of the program that owns the variable.
         * @param is_array Whether the variable is an array.
         */
        void create_var(std::string var, Type type, std::string owner, bool is_array);

        /**
         * Receive a message from a socket.
         *
         * @param socket Socket to receive from.
         * @return 0 on success, -1 on failure.
         */
        int recv_message(int socket);

        /**
         * Send a message to a socket.
         *
         * @param socket Socket to send to.
         * @param var Name of the variable.
         * @return 0 on success, -1 on failure.
         */
        int send_message(int socket, std::string var);

        /**
         * Receive an interest message from a socket.
         *
         * @param socket Socket to receive from.
         * @return 0 on success, -1 on failure.
         */
        int recv_interest(int socket);

        /**
         * Send an interest message to a socket.
         *
         * @param socket Socket to send to.
         * @param var Name of the variable.
         * @return 0 on success, -1 on failure.
         */
        int send_interest(int socket, std::string var);

        /**
         * Send a request to update a variable.
         *
         * @param socket Socket to send to.
         * @param var Name of the variable.
         * @param data New data.
         * @param data_size Size of the new data.
         * @return 0 on success, -1 on failure.
         */
        int request_update(int socket, std::string var, void *data, int data_size);

        /**
         * Accept a connection.
         *
         * @return 0 on success, -1 on failure.
         */
        int accept_connection();

        /**
         * Wake up a thread.
         *
         * @param m Mutex to lock.
         * @param fd File descriptor to write to.
         * @param action Action to perform after writing to the file descriptor.
         */
        void wakeup_thread(std::mutex &m, int fd, std::function<void()> action);

        /**
         * Receive loop for the server.
         */
        void recv_loop();

        /**
         * Accept loop for the server.
         */
        void accept_loop();

        /**
         * Identification loop for the server.
         */
        void identification_loop();

        /**
         * Send a message to all subscribers of a variable.
         *
         * @param var Name of the variable.
         */
        void notify_subscribers(std::string var);

        /**
         * Check if a variable exists and is of the correct type.
         *
         * @tparam T Type to check for.
         * @param var Name of the variable.
         */
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
                    if (!std::is_same_v<T, int16_t> && !std::is_same_v<T, int8_t>)
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
                    if (!std::is_same_v<T, int32_t> && !std::is_same_v<T, int16_t> && !std::is_same_v<T, int8_t>)
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
                    if (!std::is_same_v<T, int64_t> && !std::is_same_v<T, int32_t> && !std::is_same_v<T, int16_t> && !std::is_same_v<T, int8_t>)
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
                    if (!std::is_same_v<T, uint16_t> && !std::is_same_v<T, uint8_t>)
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
                    if (!std::is_same_v<T, uint32_t> && !std::is_same_v<T, uint16_t> && !std::is_same_v<T, uint8_t>)
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
                    if (!std::is_same_v<T, uint64_t> && !std::is_same_v<T, uint32_t> && !std::is_same_v<T, uint16_t> && !std::is_same_v<T, uint8_t>)
                        throw std::runtime_error("Variable '" + var + "' is of type uint64_t.");
                }
                break;
            case FLOAT:
                if (v.is_array)
                {
                    if (!std::is_same_v<T, std::vector<float>>)
                        throw std::runtime_error("Variable '" + var + "' is of type std::vector<float>.");
                }
                else
                {
                    if (!std::is_same_v<T, float>)
                        throw std::runtime_error("Variable '" + var + "' is of type float.");
                }
                break;
            case DOUBLE:
                if (v.is_array)
                {
                    if (!std::is_same_v<T, std::vector<double>>)
                        throw std::runtime_error("Variable '" + var + "' is of type std::vector<double>.");
                }
                else
                {
                    if (!std::is_same_v<T, double> && !std::is_same_v<T, float>)
                        throw std::runtime_error("Variable '" + var + "' is of type double.");
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
