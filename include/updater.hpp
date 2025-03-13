#ifndef UPDATER_HPP
#define UPDATER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#if defined(_WIN32)
    #include <windows.h>
    #include <wininet.h>
    #pragma comment(lib, "wininet.lib")
#elif defined(__linux__) || defined(__APPLE__)
    #include <sys/stat.h>
#endif

namespace updater {

inline bool download_file(const std::string& url, const std::string& output_path) {
    std::cout << "Downloading: " << url << " -> " << output_path << std::endl;

#if defined(_WIN32)
    HINTERNET hInternet = InternetOpenA("Updater", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
    if (!hInternet) return false;

    HINTERNET hFile = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::ofstream outFile(output_path, std::ios::binary);
    if (!outFile) {
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        outFile.write(buffer, bytesRead);
    }

    outFile.close();
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    return true;
#elif defined(__linux__) || defined(__APPLE__)
    std::string command = "wget -O " + output_path + " " + url;
    return system(command.c_str()) == 0;
#endif
}

// download and compare version files
inline bool check_for_updates(const std::string& latest_version_url, const std::string& local_version_file) {
    std::cout << "Checking for updates from: " << latest_version_url << std::endl;

    std::string temp_version_file = "latest_version.txt";
    if (!download_file(latest_version_url, temp_version_file)) {
        return false;
    }

    std::ifstream latest_file(temp_version_file);
    std::ifstream current_file(local_version_file);

    if (!latest_file || !current_file) {
        std::cerr << "ERROR: Failed to open version files." << std::endl;
        return false;
    }

    std::string latest_version, current_version;
    std::getline(latest_file, latest_version);
    std::getline(current_file, current_version);

    latest_file.close();
    current_file.close();

    return latest_version != current_version;
}

inline bool extract_zip(const std::string& archive_file) {
    std::cout << "Extracting source files from " << archive_file << std::endl;

#if defined(_WIN32)
    std::string extract_command = "tar -xf " + archive_file;
#elif defined(__linux__) || defined(__APPLE__)
    std::string extract_command = "unzip -o " + archive_file;
#endif

    return system(extract_command.c_str()) == 0;
}

// download/install ninja c++ compiler (a lightweight and cross platform open source compiler)
inline bool download_ninja() {
    std::string ninja_url, ninja_binary;

#if defined(_WIN32)
    ninja_url = "https://github.com/ninja-build/ninja/releases/latest/download/ninja-win.zip";
    ninja_binary = "ninja.exe";
#elif defined(__linux__) || defined(__APPLE__)
    ninja_url = "https://github.com/ninja-build/ninja/releases/latest/download/ninja-linux.zip";
    ninja_binary = "ninja";
#endif

    std::string ninja_zip = "ninja.zip";

    if (!download_file(ninja_url, ninja_zip)) {
        std::cerr << "ERROR: Failed to download Ninja build system.";
        return false;
    }

#if defined(_WIN32)
    std::string extract_command = "unzip -o " + ninja_zip;
#elif defined(__linux__) || defined(__APPLE__)
    std::string extract_command = "tar -xf " + ninja_zip;
#endif

    if (system(extract_command.c_str()) != 0) {
        std::cerr << "ERROR: Failed to extract Ninja.";
        return false;
    }

    return true;
}

inline bool generate_ninja_build_file() {
    std::ofstream buildFile("build.ninja");
    if (!buildFile) {
        std::cerr << "ERROR: Failed to create build.ninja file.";
        return false;
    }

#if defined(_WIN32)
    std::string compiler = "cl";
#elif defined(__linux__) || defined(__APPLE__)
    std::string compiler = "g++";
#endif

    buildFile << "rule cxx"
        << "  command = " << compiler << " -std=c++20 -o updater_new main.cpp"
        << "  description = Building updater..."
        << ""
        << "build updater_new: cxx main.cpp";

    buildFile.close();
    return true;
}

inline bool compile_with_ninja() {
    if (!download_ninja()) return false;
    if (!generate_ninja_build_file()) return false;

    std::cout << "Compiling with Ninja...";
    if (system("./ninja") != 0) {
        std::cerr << "ERROR: Compilation with Ninja failed.";
        return false;
    }

    return true;
}

// compile
inline void update_from_source(const std::string& source_zip_url) {
    std::cout << "Updating from source..." << std::endl;

    std::string archive_file = "source_code.zip";
    if (!download_file(source_zip_url, archive_file)) {
        std::cerr << "ERROR: Failed to download source archive.";
        return;
    }

    if (!extract_zip(archive_file)) return;

    if (!compile_with_ninja()) {
        std::cerr << "ERROR: Compilation failed! Update aborted.";
        return;
    }

    std::cout << "Update successful. Restarting application...";
    system("./updater_new");
    exit(0);
}

} // namespace updater

#endif // UPDATER_HPP
