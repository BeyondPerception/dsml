#include <dsml.hpp>

int main(int argc, char *argv[])
{
    dsml::State dsml("./demo/config.tsv", "DN");

    dsml.register_owner("CN", "127.0.0.1", 1111);

    return 0;
}
