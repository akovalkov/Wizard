#define DOCTEST_CONFIG_IMPLEMENT
#include <cstring>
#include <doctest/doctest.h>
#include "helper.h"

GlobalFixture fixture;

int main(int argc, char** argv) {

    doctest::Context context(argc, argv);
    for(auto i = 0; i != argc; ++i) {
        if(std::strcmp(argv[i], "--test-dir") == 0 && i + 1 < argc) {
            fixture.setDirectories(argv[i + 1]);
        }
    }
    return context.run(); // run
}