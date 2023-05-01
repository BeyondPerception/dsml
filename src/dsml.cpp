#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <dsml.hpp>

// MAC and Linux have different names for telling the OS that you have more
// data to send.
#ifdef __linux__
#define MSG_HAVEMORE MSG_MORE
#endif

using namespace dsml;

State::State(std::string config, std::string program_name) : self(program_name)
{
    if (std::filesystem::exists(config) == false)
        throw std::runtime_error("Configuration file does not exist.");

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

        create_var(var, type_map[type], owner, is_array == "true");
        i++;
    }
}

State::~State()
{
    for (auto &var : vars)
    {
        switch (var.second.type)
        {
        case INT8:
            if (var.second.is_array)
                delete[] static_cast<int8_t *>(var.second.data);
            else
                delete static_cast<int8_t *>(var.second.data);
            break;
        case INT16:
            if (var.second.is_array)
                delete[] static_cast<int16_t *>(var.second.data);
            else
                delete static_cast<int16_t *>(var.second.data);
            break;
        case INT32:
            if (var.second.is_array)
                delete[] static_cast<int32_t *>(var.second.data);
            else
                delete static_cast<int32_t *>(var.second.data);
            break;
        case INT64:
            if (var.second.is_array)
                delete[] static_cast<int64_t *>(var.second.data);
            else
                delete static_cast<int64_t *>(var.second.data);
            break;
        case UINT8:
            if (var.second.is_array)
                delete[] static_cast<uint8_t *>(var.second.data);
            else
                delete static_cast<uint8_t *>(var.second.data);
            break;
        case UINT16:
            if (var.second.is_array)
                delete[] static_cast<uint16_t *>(var.second.data);
            else
                delete static_cast<uint16_t *>(var.second.data);
            break;
        case UINT32:
            if (var.second.is_array)
                delete[] static_cast<uint32_t *>(var.second.data);
            else
                delete static_cast<uint32_t *>(var.second.data);
            break;
        case UINT64:
            if (var.second.is_array)
                delete[] static_cast<uint64_t *>(var.second.data);
            else
                delete static_cast<uint64_t *>(var.second.data);
            break;
        }
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
        switch (type)
        {
        case INT8:
            v.data = new int8_t;
            break;
        case INT16:
            v.data = new int16_t;
            break;
        case INT32:
            v.data = new int32_t;
            break;
        case INT64:
            v.data = new int64_t;
            break;
        case UINT8:
            v.data = new uint8_t;
            break;
        case UINT16:
            v.data = new uint16_t;
            break;
        case UINT32:
            v.data = new uint32_t;
            break;
        case UINT64:
            v.data = new uint64_t;
            break;
        default:
            v.data = nullptr;
        }
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
    if (vars[var].data != nullptr)
    {
        free(vars[var].data);
    }

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
    // Check if the variable exists.
    if (vars.find(var) == vars.end())
    {
        return -1;
    }

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
