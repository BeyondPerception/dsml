#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <dsml.hpp>

// MAC and Linux have different names for telling the OS that you have more
// data to send.
#ifdef __linux__
    #define MSG_HAVEMORE MSG_MORE
#endif

using namespace dsml;

State::State(std::string config, std::string program_name, int port) : self(program_name)
{
    // Check if configuration file exists.
    if (!std::filesystem::exists(config))
    {
        throw std::runtime_error("Configuration file does not exist.");
    }

    bool needs_socket = false, needs_recv = false;

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

        // Parse line.
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

    // Create wakeup pipes.
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        perror("pipe()");
        throw std::runtime_error("Failed to create wakeup pipe.");
    }

    recv_socket_list.push_back(pipefd[0]);
    recv_wakeup_fd = pipefd[1];
    if (pipe(pipefd) < 0)
    {
        perror("pipe()");
        throw std::runtime_error("Failed to create wakeup pipe.");
    }

    client_socket_list.push_back(pipefd[0]);
    identification_wakeup_fd = pipefd[1];
    server_socket = -1;

    // Handle `needs_recv`.
    if (needs_recv)
    {
        recv_thread_running = true;
        recv_thread = std::thread(&State::recv_loop, this);
        recv_thread.detach();
    }

    // Handle `needs_socket`.
    if (needs_socket)
    {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
            perror("socket()");
            throw std::runtime_error("Could not create socket.");
        }

        // Enable SO_REUSEADDR so that we do not get bind() errors if the previous
        // socket is stuck in TIME_WAIT or has not been released by the OS.
        int enable = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        {
            perror("setsockopt()");
            exit(1);
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
        accept_thread = std::thread(&State::accept_loop, this);
        accept_thread.detach();

        identification_thread_running = true;
        identification_thread = std::thread(&State::identification_loop, this);
        identification_thread.detach();
    }
}

void State::recv_loop()
{
    while (recv_thread_running)
    {
        std::unique_lock lk(socket_list_m);

        // Set up poll structures.
        pollfd pfds[recv_socket_list.size()];
        for (int i = 0; i < recv_socket_list.size(); ++i)
        {
            pfds[i].fd = recv_socket_list[i];
            pfds[i].events = POLLIN;
        }

        int ret = poll(pfds, recv_socket_list.size(), -1);

        for (int i = 0; i < recv_socket_list.size(); ++i)
        {
            if (pfds[i].revents & POLLIN)
            {
                if (i == 0)
                {
                    char buf[1];
                    read(pfds[i].fd, buf, 1);
                }
                else
                {
                    if (recv_message(pfds[i].fd) < 0)
                    {
                        close(pfds[i].fd);
                        recv_socket_list.erase(recv_socket_list.begin() + i);
                    }
                }
            }
        }
    }
}

void State::accept_loop()
{
    while (accept_thread_running)
    {
        accept_connection();
    }
}

void State::identification_loop()
{
    while (identification_thread_running)
    {
        std::unique_lock lk(client_socket_list_m);

        // Set up poll structures.
        pollfd pfds[client_socket_list.size()];
        for (int i = 0; i < client_socket_list.size(); ++i)
        {
            pfds[i].fd = client_socket_list[i];
            pfds[i].events = POLLIN;
        }

        int ret = poll(pfds, client_socket_list.size(), -1);

        for (int i = 0; i < client_socket_list.size(); ++i)
        {
            if (pfds[i].revents & POLLIN)
            {
                if (i == 0)
                {
                    char buf[1];
                    read(pfds[i].fd, buf, 1);
                }
                else
                {
                    if (recv_interest(pfds[i].fd) < 0)
                    {
                        close(pfds[i].fd);
                        client_socket_list.erase(client_socket_list.begin() + i);
                    }
                }
            }
        }
    }
}

void State::wakeup_thread(std::mutex &m, int fd, std::function<void()> action)
{
    std::mutex cv_m;
    std::condition_variable cv;
    std::atomic<bool> acquired = false, through = false;
    std::thread t([&]()
    {
        m.lock();
        acquired = true;
        std::unique_lock lk(cv_m);
        cv.wait(lk);
        through = true;
        m.unlock();
    });
    while (!acquired)
    {
        write(fd, "a", 1);
    }
    action();
    while (!through)
    {
        cv.notify_all();
    }
    t.join();
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

    // Enable TCP KeepAlive to ensure that we are notified if the leader goes down.
    int enable = 1;
    if (setsockopt(new_socket, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(int)) < 0)
    {
        perror("setsockopt()");
        return -1;
    }

    struct linger lo = {1, 0};
    if (setsockopt(new_socket, SOL_SOCKET, SO_LINGER, &lo, sizeof(lo)) < 0)
    {
        perror("setsockopt()");
        return -1;
    }

#ifdef __APPLE__
    if (setsockopt(new_socket, SOL_SOCKET, SO_NOSIGPIPE, (void *)&enable, sizeof(int)) < 0)
    {
        perror("setsockopt()");
        return -1;
    }
#endif

    wakeup_thread(client_socket_list_m, identification_wakeup_fd, [this, new_socket]()
    {
        client_socket_list.push_back(new_socket);
    });

    return 0;
}

State::~State()
{
    recv_thread_running = false;
    accept_thread_running = false;
    identification_thread_running = false;

    write(identification_wakeup_fd, "a", 1);
    write(recv_wakeup_fd, "a", 1);

    close(identification_wakeup_fd);
    close(recv_wakeup_fd);
    close(server_socket);

    for (auto socket : recv_socket_list)
    {
        close(socket);
    }

    for (auto socket : client_socket_list)
    {
        close(socket);
    }

    for (auto &var : vars)
    {
        free(var.second.data);
    }
}

int State::register_owner(std::string variable_owner, int socket)
{
    wakeup_thread(socket_list_m, recv_wakeup_fd, [this, socket]()
    {
        recv_socket_list.push_back(socket);
    });

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
    // Connect to IP/port.
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
    Variable v = {type, is_array, 1, owner, -1, nullptr, std::chrono::system_clock::now()};

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
    case FLOAT:
        return sizeof(float);
    case DOUBLE:
        return sizeof(double);
    case STRING:
        return sizeof(char);
    default:
        throw std::runtime_error("Invalid type.");
    }
}

void State::notify_subscribers(std::string var)
{
    std::unique_lock lk(subscriber_list_m);

    // Send the message to all subscribers.
    for (int i = subscriber_list[var].size() - 1; i >= 0; --i)
    {
        int socket = subscriber_list[var][i];
        int ret = send_message(socket, var);
        if (ret < 0)
        {
            subscriber_list[var].erase(subscriber_list[var].begin() + i);
            wakeup_thread(client_socket_list_m, identification_wakeup_fd, [this, socket]()
            {
                client_socket_list.erase(std::remove(client_socket_list.begin(), client_socket_list.end(), socket), client_socket_list.end());
            });
            close(socket);
        }
    }
}

int read_all_bytes(int socket, void *buf, size_t size)
{
    size_t bytes_read = 0;
    while (bytes_read < size)
    {
        int ret = read(socket, (char *)buf + bytes_read, size - bytes_read);
        if (ret < 0)
        {
            return ret;
        }
        bytes_read += ret;
    }
    return bytes_read;
}

int State::recv_message(int socket)
{
    int var_name_size, var_data_size;
    int err;

    // Read the size of the variable name.
    if ((err = read_all_bytes(socket, &var_name_size, sizeof(var_name_size))) <= 0)
    {
        return (err == 0) ? -1 : err;
    }

    // Read the variable name.
    std::string var(var_name_size, 0);
    if ((err = read_all_bytes(socket, &var[0], var_name_size)) < 0)
    {
        return err;
    }

    // Check if the variable exists.
    if (vars.find(var) == vars.end())
    {
        return -1;
    }

    std::unique_lock lk(var_locks[var]);

    // Read the size of the data.
    if ((err = read_all_bytes(socket, &var_data_size, sizeof(var_data_size))) < 0)
    {
        return err;
    }

    // Free the old data.
    free(vars[var].data);

    // Allocate memory for the new data.
    vars[var].data = malloc(var_data_size);
    if (vars[var].data == nullptr)
    {
        perror("malloc()");
        return -1;
    }

    // Read the data.
    if ((err = read_all_bytes(socket, vars[var].data, var_data_size)) < 0)
    {
        return err;
    }

    // Update the size of the variable.
    vars[var].size = var_data_size / type_size(vars[var].type);
    vars[var].last_updated = std::chrono::system_clock::now();

    var_cvs[var].notify_all();

    return 0;
}

int State::send_message(int socket, std::string var)
{
    std::unique_lock lk(var_locks[var]);
    int var_name_size = var.size(),
        var_data_size = vars[var].size * type_size(vars[var].type);
    int err;

    // Send the size of the variable name.
    if ((err = send(socket, &var_name_size, sizeof(var_name_size), MSG_HAVEMORE | MSG_NOSIGNAL)) < 0)
    {
        return err;
    }

    // Send the variable name.
    if ((err = send(socket, &var[0], var_name_size, MSG_HAVEMORE | MSG_NOSIGNAL)) < 0)
    {
        return err;
    }

    // Send the size of the data.
    if ((err = send(socket, &var_data_size, sizeof(var_data_size), MSG_HAVEMORE | MSG_NOSIGNAL)) < 0)
    {
        return err;
    }

    // Send the data.
    if ((err = send(socket, vars[var].data, var_data_size, MSG_NOSIGNAL)) < 0)
    {
        return err;
    }

    return 0;
}

int State::recv_interest(int socket)
{
    int var_name_size, var_data_size;
    bool is_request;
    int err;

    // Check if this is an interest message or an update request.
    if ((err = read_all_bytes(socket, &is_request, 1)) <= 0)
    {
        return (err == 0) ? -1 : err;
    }

    // Read the size of the variable name.
    if ((err = read_all_bytes(socket, &var_name_size, sizeof(var_name_size))) < 0)
    {
        return err;
    }

    // Read the variable name.
    std::string var(var_name_size, 0);
    if ((err = read_all_bytes(socket, &var[0], var_name_size)) < 0)
    {
        return err;
    }

    // Check if the variable exists.
    if (vars.find(var) == vars.end())
    {
        return -1;
    }

    std::unique_lock lk(var_locks[var]);

    // Update request.
    if (is_request)
    {
        // Read the size of the data.
        if ((err = read_all_bytes(socket, &var_data_size, sizeof(var_data_size))) < 0)
        {
            return err;
        }

        // Free the old data.
        free(vars[var].data);

        // Allocate memory for the new data.
        vars[var].data = malloc(var_data_size);
        if (vars[var].data == nullptr)
        {
            perror("malloc()");
            return -1;
        }

        // Read the data.
        if ((err = read_all_bytes(socket, vars[var].data, var_data_size)) < 0)
        {
            return err;
        }

        // Update the size of the variable.
        vars[var].size = var_data_size / type_size(vars[var].type);
        vars[var].last_updated = std::chrono::system_clock::now();

        var_cvs[var].notify_all();
    }
    // Interest message.
    else
    {
        // Add the socket to the subscriber list.
        std::unique_lock lk(subscriber_list_m);
        subscriber_list[var].push_back(socket);
    }

    lk.unlock();
    notify_subscribers(var);

    return 0;
}

int State::send_interest(int socket, std::string var)
{
    int var_name_size = var.size();
    int err;

    // Send our interest value.
    bool request = false;
    if ((err = send(socket, &request, 1, MSG_HAVEMORE | MSG_NOSIGNAL)) < 0)
    {
        return err;
    }

    // Send the size of the variable name.
    if ((err = send(socket, &var_name_size, sizeof(var_name_size), MSG_HAVEMORE | MSG_NOSIGNAL)) < 0)
    {
        return err;
    }

    // Send the variable name.
    if ((err = send(socket, &var[0], var_name_size, MSG_NOSIGNAL)) < 0)
    {
        return err;
    }

    return 0;
}

int State::request_update(int socket, std::string var, void *data, int data_size)
{
    int var_name_size = var.size();
    int err;

    // Send request value;
    bool request = true;
    if ((err = send(socket, &request, 1, MSG_HAVEMORE | MSG_NOSIGNAL)) < 0)
    {
        return err;
    }

    // Send the size of the variable name.
    if ((err = send(socket, &var_name_size, sizeof(var_name_size), MSG_HAVEMORE | MSG_NOSIGNAL)) < 0)
    {
        return err;
    }

    // Send the variable name.
    if ((err = send(socket, &var[0], var_name_size, MSG_HAVEMORE | MSG_NOSIGNAL)) < 0)
    {
        return err;
    }

    // Send the size of the data.
    if ((err = send(socket, &data_size, sizeof(data_size), MSG_HAVEMORE | MSG_NOSIGNAL)) < 0)
    {
        return err;
    }

    // Send the data.
    if ((err = send(socket, data, data_size, MSG_NOSIGNAL)) < 0)
    {
        return err;
    }

    return 0;
}

void State::check_var_exists(std::string var)
{
    if (vars.find(var) == vars.end())
    {
        throw std::runtime_error("Variable " + var + " does not exist.");
    }

    if (vars[var].owner_socket < 0 && vars[var].owner != self)
    {
        throw std::runtime_error("Variable " + var + " has no owner registered.");
    }
}
