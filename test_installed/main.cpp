#include <digraphx_fast/version.h>

#include <iostream>

auto main() -> int {
    const auto ok = (DIGRAPHXFAST_VERSION_MAJOR >= 1);
    std::cout << "digraphx_fast installed test: version " << DIGRAPHXFAST_VERSION << "\n";
    return ok ? 0 : 1;
}
