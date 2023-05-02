#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <dsml.hpp>

#define PADDED_LENGTH 50

bool all_tests_passed = true;

/**
 * Test a boolean expression and print the result.
 *
 * @param b Input boolean.
 * @param msg Message to print.
 */
void test(const bool &b, std::string msg)
{
    const int i = msg.size();
    msg += b ? "PASSED" : "FAILED";
    msg.insert(i, PADDED_LENGTH - msg.size(), ' ');
    all_tests_passed = all_tests_passed && b;
    std::cerr << msg << std::endl;
}

/**
 * Run simple tests.
 *
 * @param dsml Instance of `dsml::State`.
 */
void test_simple(dsml::State &dsml)
{
    // Set variables.
    dsml.set("TEST1", (int8_t)-1);
    dsml.set("TEST2", (int16_t)-2);
    dsml.set("TEST3", (int32_t)-3);
    dsml.set("TEST4", (int64_t)-4);
    dsml.set("TEST5", (uint8_t)5);
    dsml.set("TEST6", (uint16_t)6);
    dsml.set("TEST7", (uint32_t)7);
    dsml.set("TEST8", (uint64_t)8);
    dsml.set("TEST9", (float)-9.0);
    dsml.set("TEST10", (double)-10.0);
    dsml.set("TEST11", std::vector<int8_t>{-1, 0, 1});
    dsml.set("TEST12", std::string("Hello world!"));

    // Test `set` with non-existent variable.
    try
    {
        dsml.set("TEST13", -1);
        test(false, "set non-existent variable");
    }
    catch (...)
    {
        test(true, "set non-existent variable");
    }

    // Test `get` with non-existent variable.
    try
    {
        dsml.get<int8_t>("TEST13");
        test(false, "get non-existent variable");
    }
    catch (...)
    {
        test(true, "get non-existent variable");
    }

    // Test `set` and `get` with correct types.
    test(dsml.get<int8_t>("TEST1") == -1, "set/get INT8 correct");
    test(dsml.get<int16_t>("TEST2") == -2, "set/get INT16 correct");
    test(dsml.get<int32_t>("TEST3") == -3, "set/get INT32 correct");
    test(dsml.get<int64_t>("TEST4") == -4, "set/get INT64 correct");
    test(dsml.get<uint8_t>("TEST5") == 5, "set/get UINT8 correct");
    test(dsml.get<uint16_t>("TEST6") == 6, "set/get UINT16 correct");
    test(dsml.get<uint32_t>("TEST7") == 7, "set/get UINT32 correct");
    test(dsml.get<uint64_t>("TEST8") == 8, "set/get UINT64 correct");
    test(dsml.get<float>("TEST9") == -9.0, "set/get FLOAT correct");
    test(dsml.get<double>("TEST10") == -10.0, "set/get DOUBLE correct");
    test(dsml.get<std::vector<int8_t>>("TEST11") == std::vector<int8_t>{-1, 0, 1}, "set/get ARRAY correct");
    test(dsml.get<std::string>("TEST12") == "Hello world!", "set/get STRING correct");

    // Test `set` and `get` with incorrect types.
    try
    {
        dsml.get<std::string>("TEST1");
        test(false, "set/get INT8 incorrect");
    }
    catch (...)
    {
        test(true, "set/get INT8 incorrect");
    }
    try
    {
        dsml.get<std::string>("TEST2");
        test(false, "set/get INT16 incorrect");
    }
    catch (...)
    {
        test(true, "set/get INT16 incorrect");
    }
    try
    {
        dsml.get<std::string>("TEST3");
        test(false, "set/get INT32 incorrect");
    }
    catch (...)
    {
        test(true, "set/get INT32 incorrect");
    }
    try
    {
        dsml.get<std::string>("TEST4");
        test(false, "set/get INT64 incorrect");
    }
    catch (...)
    {
        test(true, "set/get INT64 incorrect");
    }
    try
    {
        dsml.get<std::string>("TEST5");
        test(false, "set/get UINT8 incorrect");
    }
    catch (...)
    {
        test(true, "set/get UINT8 incorrect");
    }
    try
    {
        dsml.get<std::string>("TEST6");
        test(false, "set/get UINT16 incorrect");
    }
    catch (...)
    {
        test(true, "set/get UINT16 incorrect");
    }
    try
    {
        dsml.get<std::string>("TEST7");
        test(false, "set/get UINT32 incorrect");
    }
    catch (...)
    {
        test(true, "set/get UINT32 incorrect");
    }
    try
    {
        dsml.get<std::string>("TEST8");
        test(false, "set/get UINT64 incorrect");
    }
    catch (...)
    {
        test(true, "set/get UINT64 incorrect");
    }
    try
    {
        dsml.get<std::string>("TEST9");
        test(false, "set/get FLOAT incorrect");
    }
    catch (...)
    {
        test(true, "set/get FLOAT incorrect");
    }
    try
    {
        dsml.get<std::string>("TEST10");
        test(false, "set/get DOUBLE incorrect");
    }
    catch (...)
    {
        test(true, "set/get DOUBLE incorrect");
    }
    try
    {
        dsml.get<std::string>("TEST11");
        test(false, "set/get ARRAY incorrect");
    }
    catch (...)
    {
        test(true, "set/get ARRAY incorrect");
    }
    try
    {
        dsml.get<int8_t>("TEST12");
        test(false, "set/get STRING incorrect");
    }
    catch (...)
    {
        test(true, "set/get STRING incorrect");
    }
}

/**
 * Run hard tests.
 *
 * @param dsml1 First instance of `dsml::State`.
 * @param dsml2 Second instance of `dsml::State`.
 */
void test_hard(dsml::State &dsml1, dsml::State &dsml2)
{
    // Test `get`.
    test(dsml2.get<int8_t>("TEST1") == -1, "set/get INT8");
    test(dsml2.get<int16_t>("TEST2") == -2, "set/get INT16");
    test(dsml2.get<int32_t>("TEST3") == -3, "set/get INT32");
    test(dsml2.get<int64_t>("TEST4") == -4, "set/get INT64");
    test(dsml2.get<uint8_t>("TEST5") == 5, "set/get UINT8");
    test(dsml2.get<uint16_t>("TEST6") == 6, "set/get UINT16");
    test(dsml2.get<uint32_t>("TEST7") == 7, "set/get UINT32");
    test(dsml2.get<uint64_t>("TEST8") == 8, "set/get UINT64");
    test(dsml2.get<float>("TEST9") == -9.0, "set/get FLOAT");
    test(dsml2.get<double>("TEST10") == -10.0, "set/get DOUBLE");
    test(dsml2.get<std::vector<int8_t>>("TEST11") == std::vector<int8_t>{-1, 0, 1}, "set/get ARRAY");
    test(dsml2.get<std::string>("TEST12") == "Hello world!", "set/get STRING");

    // Set variables.
    dsml1.set("TEST1", (int8_t)42);
    std::vector<int8_t> v = dsml1.get<std::vector<int8_t>>("TEST11");
    v.push_back(42);
    dsml1.set("TEST11", v);
    dsml1.set("TEST12", std::string("Goodbye world!"));

    // Test `get` again.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    test(dsml2.get<int8_t>("TEST1") == 42, "set/get INT8 again");
    test(dsml2.get<std::vector<int8_t>>("TEST11") == std::vector<int8_t>{-1, 0, 1, 42}, "set/get ARRAY again");
    test(dsml2.get<std::string>("TEST12") == "Goodbye world!", "set/get STRING again");

    // Set variables.
    dsml2.set("TEST1", (int8_t)24);
    dsml2.set("TEST11", std::vector<int8_t>{-24, 24});
    dsml2.set("TEST12", std::string("..."));

    // Test `get` after requesting updates.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    test(dsml1.get<int8_t>("TEST1") == 24, "set/get INT8 request update");
    test(dsml1.get<std::vector<int8_t>>("TEST11") == std::vector<int8_t>{-24, 24}, "set/get ARRAY request update");
    test(dsml1.get<std::string>("TEST12") == "...", "set/get STRING request update");
}

int main()
{
    // Create first instance of `dsml::State`.
    dsml::State dsml1("../test/config.tsv", "DSML1", 1111);

    // Run simple tests.
    std::cerr << "\nRUNNING SIMPLE TESTS..." << std::endl;
    test_simple(dsml1);

    // Create second instance of `dsml::State`.
    dsml::State dsml2("../test/config.tsv", "DSML2", 1112);
    dsml2.register_owner("DSML1", "127.0.0.1", 1111);

    // Run hard tests.
    std::cerr << "\nRUNNING HARD TESTS..." << std::endl;
    test_hard(dsml1, dsml2);

    // Print results.
    const std::string msg = all_tests_passed ? "\nALL TESTS PASSED :)\n" : "\nSOME TESTS FAILED :(\n";
    std::cerr << msg << std::endl;
}