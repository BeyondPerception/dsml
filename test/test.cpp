#include <iostream>
#include <string>
#include <vector>

#include <dsml.hpp>

#define PADDED_LENGTH 50

bool all_tests_passed = true;

void test(const bool &b, std::string msg)
{
    const int i = msg.size();
    msg += b ? "PASSED" : "FAILED";
    msg.insert(i, PADDED_LENGTH - msg.size(), ' ');
    all_tests_passed = all_tests_passed && b;
    std::cerr << msg << std::endl;
}

void test_simple(dsml::State &dsml)
{
    dsml.set("TEST1", (uint8_t)255);
    dsml.set("TEST2", std::vector<int64_t>{-1, 0, 1});
    dsml.set("TEST3", std::string("Hello world!"));

    // Test `set` and `get`.
    test(dsml.get<uint8_t>("TEST1") == 255, "set/get simple");
    test(dsml.get<std::vector<int64_t>>("TEST2") == std::vector<int64_t>{-1, 0, 1}, "set/get vector");
    test(dsml.get<std::string>("TEST3") == "Hello world!", "set/get string");
}

int main()
{
    dsml::State dsml("../test/config.tsv", "TEST", 1111);

    // Run initial tests.
    std::cerr << "\nRUNNING INITIAL TESTS..." << std::endl;

    // Run simple tests.
    std::cerr << "\nRUNNING SIMPLE TESTS..." << std::endl;
    test_simple(dsml);

    // Run final tests.
    std::cerr << "\nRUNNING FINAL TESTS..." << std::endl;

    // Print results.
    std::string msg = all_tests_passed ? "\nALL TESTS PASSED :)\n" : "\nSOME TESTS FAILED :(\n";
    std::cerr << msg << std::endl;
}