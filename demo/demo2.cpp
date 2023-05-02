#include <iostream>

#include <dsml.hpp>

int main(int argc, char *argv[])
{
    dsml::State dsml("../demo/config.tsv", "DN", 1111);

    dsml.register_owner("CN", "127.0.0.1", 1111);

    int test1 = dsml.get<int8_t>("TEST1");
    int test2 = dsml.get<uint8_t>("TEST2");

    std::cout << test1 << " " << test2 << "\n";

    return 0;
}
