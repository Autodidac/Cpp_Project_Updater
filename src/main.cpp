#include <iostream>
#include "updater.hpp"

namespace urls {
    const std::string github_base = "https://github.com/"; // base github url
    const std::string github_raw_base = "https://raw.githubusercontent.com/"; // raw base github url, for source downloads

    const std::string owner = "Autodidac/"; // github project developer username for url 
    const std::string repo = "Cpp_Ultimate_Project_Updater"; // whatever your github project name is
    const std::string branch = "main/"; // incase you need a different branch than githubs default branch main 

    const std::string version_url = github_raw_base + owner + repo + "/" + branch + "version.txt";
    const std::string source_url = github_base + owner + repo + "/archive/refs/heads/main.zip";
    const std::string binary_url = github_base + owner + repo + "/releases/latest/download/updater.exe";
}

int main() 
{

    if (check_for_updates(urls::version_url)) {
        std::cout << "New version available!" << "\n";
        update_project(urls::source_url, urls::binary_url);
    }
    else {
#if defined(_WIN32)
        system("cls");  // Windows
#else
        system("clear"); // Linux/macOS
#endif
        std::cout << "No updates available." << "\n";
    }
    return 0;
}

