#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <dsml.hpp>

// MAC and Linux have different names for telling the OS that you have more
// data to send.
#ifdef __linux__
#define MSG_HAVEMORE MSG_MORE
#endif

using namespace dsml;

State::State(std::string config, std::string program_name, int port = 0) : self(program_name)
{
    if (std::filesystem::exists(config) == false)
        throw std::runtime_error("Configuration file does not exist.");

    bool needs_socket;
    bool needs_recv;

    std::ifstream config_file(config);
    std::string line;
    int i = 1;
    while (std::getline(config_file, line))
    {
        // Skip comment lines and empty lines.
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
        {
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

        if (owner != self)
        {
            needs_recv = true;
        }

        create_var(var, type_map[type], owner, is_array == "true");
        i++;
    }

    if (needs_recv)
    {
        recv_thread_running = true;
        recvThread = std::thread([this]() {
            while (recv_thread_running)
            {
                recv_message();
            }
        });
    }
    if (needs_socket)
    {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
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
            throw std::runtime_error("Could not bind socket.");
        }
        ret = listen(server_socket, 5);
        if (ret < 0)
        {
            throw std::runtime_error("Could not listen on socket.");
        }

        accept_thread_running = true;
        acceptThread = std::thread([this]() {
            while (accept_thread_running)
            {
                accept_connection();
            }
        });
    }
}

State::~State()
{
    recv_thread_running = false;
    accept_thread_running = false;
    recvThread.join();
    acceptThread.join();
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
        return sock;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(owner_port);

    int ret;
    if ((ret = inet_pton(AF_INET, owner_ip.c_str(), &serv_addr.sin_addr)) <= 0)
    {
        return ret;
    }

    if ((ret = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        return ret;
    }

    return register_owner(variable_owner, sock);
}

void State::create_var(std::string var, Type type, std::string owner, bool is_array)
{
    Variable v;
    v.type = type;
    v.is_array = is_array;
    v.owner = owner;
    v.size = 0;
    v.owner_socket = -1;
    if (!is_array)
    {
        v.data = malloc(type_size(type));        
    }
    else
    {
        if (v.type == STRING)
            throw std::runtime_error("Invalid line in configuration file. Cannot have array of strings.");
        v.data = nullptr;
    }
    vars[var] = v;
}

uint8_t State::type_size(Type type)
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

int State::receive_message(int socket)
{
    return 0;
}

int State::send_message(int socket, std::string var)
{
    size_t var_size = sizeof(var), data_size = vars[var].size * type_size(vars[var].type);
    int err;

    // Send the size of the variable name.
    if ((err = send(socket, &var_size, sizeof(var_size), MSG_HAVEMORE)) < 0)
    {
        return err;
    }

    // Send the variable name.
    if ((err = send(socket, &var, var_size, MSG_HAVEMORE)) < 0)
    {
        return err;
    }

    // Send the size of the data.
    if ((err = send(socket, &data_size, sizeof(data_size), MSG_HAVEMORE)) < 0)
    {
        return err;
    }

    // Send the data.
    if ((err = send(socket, vars[var].data, data_size, 0)) < 0)
    {
        return err;
    }

    return 0;
}
