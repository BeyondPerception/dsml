#include <iostream>

#include <dsml.hpp>

int main(int argc, char *argv[])
{
    dsml::State dsml ("./demo/config.tsv");

    dsml.set("TEST1", 5);
    dsml.set("TEST2", 255);

    int test1 = dsml.get<int8_t>("TEST1");
    int test2 = dsml.get<int8_t>("TEST1");

    std::cout << test1 << " " << test2 << "\n";

    return 0;
}
