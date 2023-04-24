#include <string>

class DSML
{
public:
    DSML(std::string config);

    int register_owner(std::string program_name, std::string program_ip);

    template <typename T>
    T get(std::string var);

    template <typename T>
    void set(std::string var, T value);

private:
    template <typename T>
    void create(std::string var);

    template <typename T>
    void create(std::string var, T value);
};
