#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>

#include <dsml.hpp>

// MAC and Linux have different names for telling the OS that you have more
// data to send.
#ifdef __linux__
    #define MSG_HAVEMORE MSG_MORE
#endif

using namespace dsml;

State::State(std::string config, std::string program_name, int port = 0) : self(program_name)
{
    if (!std::filesystem::exists(config))
    {
        throw std::runtime_error("Configuration file does not exist.");
    }

    bool needs_socket, needs_recv;

    std::ifstream config_file(config);
    std::string line;
    int i = 1;
    while (std::getline(config_file, line))
    {
        // Skip comment lines and empty lines.
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
        {
            ++i;
            continue;
        }

        std::istringstream iss(line);
        std::string var, type, owner, is_array;

        if (!(iss >> var >> type >> owner >> is_array))
        {
            throw std::runtime_error("Invalid line in configuration file on line " + std::to_string(i));
        }
        if (is_array != "true" && is_array != "false")
        {
            throw std::runtime_error("Invalid array specification in configuration file on line " + std::to_string(i));
        }
        if (type_map.find(type) == type_map.end())
        {
            throw std::runtime_error("Invalid type in configuration file on line " + std::to_string(i));
        }

        if (owner == self)
        {
            needs_socket = true;
        }
        else
        {
            needs_recv = true;
        }

        create_var(var, type_map[type], owner, is_array == "true");
        ++i;
    }

    if (needs_recv)
    {
        recv_thread_running = true;
        recv_thread = std::thread([this]()
        {
            while (recv_thread_running)
            {
                // Setup poll structs
                pollfd *pfds = new pollfd[socket_list.size()];
                for (int i = 0; i < socket_list.size(); i++)
                {
                    pfds[i].fd = socket_list[i];
                    pfds[i].events = POLLIN;
                }

                int ret = poll(pfds, socket_list.size(), -1);

                for (int i = 0; i < socket_list.size(); i++)
                {
                    // Check if socket available is wakeup socket.
                    if (i == 0)
                    {
                    }
                    if (pfds[i].revents & POLLIN)
                    {
                        recv_message(pfds[i].fd);
                    }
                }

                delete[] pfds;
            }
        });
    }
    if (needs_socket)
    {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
            perror("socket()");
            throw std::runtime_error("Could not create socket.");
        }

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        int ret;
        ret = bind(server_socket, (struct sockaddr *)&address, sizeof(address));
        if (ret < 0)
        {
            perror("bind()");
            throw std::runtime_error("Could not bind socket.");
        }
        ret = listen(server_socket, 5);
        if (ret < 0)
        {
            perror("listen()");
            throw std::runtime_error("Could not listen on socket.");
        }

        accept_thread_running = true;
        accept_thread = std::thread([this]()
        {
            while (accept_thread_running)
            {
                accept_connection();
            }
        });
    }
}

int State::accept_connection()
{
    struct sockaddr_in addr;
    size_t addr_len = sizeof(addr);

    // Accept connection.
    int new_socket = accept(server_socket, (struct sockaddr *)&addr, (socklen_t *)&addr_len);
    if (new_socket < 0)
    {
        perror("accept()");
        return new_socket;
    }

    // Add socket to list.
    socket_list.push_back(new_socket);

    return 0;
}

State::~State()
{
    recv_thread_running = false;
    accept_thread_running = false;
    recv_thread.join();
    accept_thread.join();
    for (auto &var : vars)
    {
        free(var.second.data);
    }
}

int State::register_owner(std::string variable_owner, int socket)
{
    for (auto &var : vars)
    {
        if (var.second.owner == variable_owner)
        {
            var.second.owner_socket = socket;
        }
    }
    return 0;
}

int State::register_owner(std::string variable_owner, std::string owner_ip, int owner_port)
{
    // Connect to ip/port.
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        return sock;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(owner_port);

    int ret;
    if ((ret = inet_pton(AF_INET, owner_ip.c_str(), &serv_addr.sin_addr)) <= 0)
    {
        perror("inet_pton()");
        return ret;
    }

    if ((ret = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        perror("connect()");
        return ret;
    }

    return register_owner(variable_owner, sock);
}

void State::create_var(std::string var, Type type, std::string owner, bool is_array)
{
    Variable v = {type, is_array, 0, owner, -1, nullptr};

    if (!is_array)
    {
        v.data = malloc(type_size(type));
    }
    else
    {
        if (v.type == STRING)
        {
            throw std::runtime_error("Invalid line in configuration file. Cannot have array of strings.");
        }

        v.data = nullptr;
    }

    vars[var] = v;
}

size_t State::type_size(Type type)
{
    switch (type)
    {
    case INT8:
        return sizeof(int8_t);
    case INT16:
        return sizeof(int16_t);
    case INT32:
        return sizeof(int32_t);
    case INT64:
        return sizeof(int64_t);
    case UINT8:
        return sizeof(uint8_t);
    case UINT16:
        return sizeof(uint16_t);
    case UINT32:
        return sizeof(uint32_t);
    case UINT64:
        return sizeof(uint64_t);
    case STRING:
        return sizeof(char);
    default:
        throw std::runtime_error("Invalid type.");
    }
}

int State::recv_message(int socket)
{
    size_t var_name_size, var_data_size;
    std::string var;
    int err;

    // Read the size of the variable name.
    if ((err = read(socket, &var_name_size, sizeof(var_name_size))) < 0)
    {
        return err;
    }

    // Read the variable name.
    var.resize(var_name_size);
    if ((err = read(socket, &var, var_name_size)) < 0)
    {
        return err;
    }

    // Check if the variable exists.
    if (vars.find(var) == vars.end())
    {
        return -1;
    }

    // Free the old data.
    free(vars[var].data);

    // Allocate memory for the new data.
    vars[var].data = malloc(var_data_size);

    // Read the size of the data.
    if ((err = read(socket, &var_data_size, sizeof(var_data_size))) < 0)
    {
        return err;
    }

    // Read the data.
    if ((err = read(socket, vars[var].data, var_data_size)) < 0)
    {
        return err;
    }

    // Update the size of the variable.
    vars[var].size = var_data_size / type_size(vars[var].type);

    return 0;
}

int State::send_message(int socket, std::string var)
{
    size_t var_name_size = sizeof(var),
           var_data_size = vars[var].size * type_size(vars[var].type);
    int err;

    // Send the size of the variable name.
    if ((err = send(socket, &var_name_size, sizeof(var_name_size), MSG_HAVEMORE)) < 0)
    {
        return err;
    }

    // Send the variable name.
    if ((err = send(socket, &var, var_name_size, MSG_HAVEMORE)) < 0)
    {
        return err;
    }

    // Send the size of the data.
    if ((err = send(socket, &var_data_size, sizeof(var_data_size), MSG_HAVEMORE)) < 0)
    {
        return err;
    }

    // Send the data.
    if ((err = send(socket, vars[var].data, var_data_size, 0)) < 0)
    {
        return err;
    }

    return 0;
}
