#include <iostream>

#include <dsml.hpp>

int main(int argc, char *argv[])
{
    dsml::State dsml ("../demo/config.tsv", "CN");

    dsml.set("TEST1", (int8_t)-128);
    dsml.set("TEST2", (uint8_t)255);

    int test1 = dsml.get<int8_t>("TEST1");
    int test2 = dsml.get<uint8_t>("TEST2");

    std::cout << test1 << " " << test2 << "\n";

    return 0;
}
