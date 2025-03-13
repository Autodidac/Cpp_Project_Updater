#include "updater.hpp"

namespace urls {
    const std::string github_base = "https://github.com/";
    const std::string github_raw_base = "https://raw.githubusercontent.com/";

    const std::string owner = "Autodidac/";
    const std::string repo = "Cpp_Project_Updater";
    const std::string branch = "main/";

    const std::string version_url = github_raw_base + owner + repo + "/" + branch + "latest_version.txt";
    const std::string source_url = github_base + owner + repo + "/archive/refs/heads/main.zip";
}

int main() {
    const std::string local_version = "version.txt";

    if (updater::check_for_updates(urls::version_url, local_version)) {
        std::cout << "New version available!" << std::endl;
        updater::update_from_source(urls::source_url);
    }
    else {
        std::cout << "No updates available." << std::endl;
    }
    return 0;
}
