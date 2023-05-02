#include <iostream>
#include <vector>

#include <dsml.hpp>

int main(int argc, char *argv[])
{
    dsml::State dsml("../demo/config.tsv", "CN", 1111);

    std::vector<int8_t> x;
    x.push_back(1);
    x.push_back(2);
    x.push_back(3);

    dsml.set("TEST1", x);
    dsml.set("TEST2", (uint8_t)255);

    std::vector<int8_t> test1 = dsml.get<std::vector<int8_t>>("TEST1");
    int test2 = dsml.get<uint8_t>("TEST2");

    std::cout << (int)test1[0] << "\n";
    std::cout << (int)test1[1] << "\n";
    std::cout << (int)test1[2] << "\n";
    std::cout << test2 << "\n";

    return 0;
}
