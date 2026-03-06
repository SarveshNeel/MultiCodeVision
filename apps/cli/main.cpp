#include <filesystem>
#include <iostream>
#include <string>

#include <mcv/util/logging.hpp>
#include <mcv/util/filesystem.hpp>
#include <mcv/core/GlobalVariables.hpp>

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage:\n"
                  << "  ./app <image_path> [--batch]\n"
                  << "  ./app <directory_path> [--batch]\n"
                  << "\nOptions:\n"
                  << "  --batch   Process without opening preview windows\n";
        return 1;
    }

    fs::path input = argv[1];

    if (argc >= 3) {
        std::string opt = argv[2];
        if (opt == "--batch")
            showWindow = false;
    }

    if (!fs::exists(input)) {
        std::cerr << "[ERROR] Path does not exist: " << input << std::endl;
        return 1;
    }

    run_cli(input);

    return 0;
}