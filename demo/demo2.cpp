#include <iostream>

#include <dsml.hpp>

int main(int argc, char *argv[])
{
    dsml::State dsml("../demo/config.tsv", "DN", 1112);

    dsml.register_owner("CN", "127.0.0.1", 1111);

    auto test1 = dsml.get<std::vector<int8_t>>("TEST1");
    int test2 = dsml.get<uint8_t>("TEST2");

    std::cout << (int) test1[0] << (int) test1[1] << (int) test1[2] << " " << test2 << "\n";

    return 0;
}
