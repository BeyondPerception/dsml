#include <iostream>
#include <vector>

#include <dsml.hpp>

int main(int argc, char *argv[])
{
    dsml::State dsml("../demo/config.tsv", "CN", 1111);

    std::cout << "SETTING..." << std::endl;

    dsml.set("TEST1", std::vector<int8_t>{1, 2, 3});
    dsml.set("TEST2", (uint8_t)255);
    dsml.set("TEST3", std::string("Hello world!"));

    std::cout << "GETTING..." << std::endl;

    std::vector<int8_t> test1 = dsml.get<std::vector<int8_t>>("TEST1");
    int test2 = dsml.get<uint8_t>("TEST2");
    std::string test3 = dsml.get<std::string>("TEST3");

    for (auto &i : test1)
    {
        std::cout << (int)i << "\n";
    }
    std::cout << test2 << "\n";
    std::cout << test3 << "\n";

    system("read");

    std::cout << "Changing TEST2\n";

    dsml.set("TEST2", (uint8_t)7);

    while (true)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    return 0;
}
