#include <iostream>
#include <string>

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

int main()
{
    std::cerr << "\nRUNNING INITIAL TESTS..." << std::endl;

    std::cerr << "\nRUNNING FINAL TESTS..." << std::endl;

    std::string msg = all_tests_passed ? "\nALL TESTS PASSED :)\n" : "\nSOME TESTS FAILED :(\n";
    std::cerr << msg << std::endl;
}