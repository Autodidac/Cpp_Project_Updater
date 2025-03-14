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

    // download current source
    inline bool download_file(const std::string& url, const std::string& output_path) {
        std::cout << "Downloading: " << url << " -> " << output_path <<  "\n";

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
        std::cout << "Checking for updates from: " << latest_version_url <<  "\n";

        std::string temp_version_file = "latest_version.txt";
        if (!download_file(latest_version_url, temp_version_file)) {
            return false;
        }

        std::ifstream latest_file(temp_version_file);
        std::ifstream current_file(local_version_file);

        if (!latest_file || !current_file) {
            std::cerr << "ERROR: Failed to open version files." <<  "\n";
            return false;
        }

        std::string latest_version, current_version;
        std::getline(latest_file, latest_version);
        std::getline(current_file, current_version);

        latest_file.close();
        current_file.close();

        // Clean up temporary file
        if (std::remove(temp_version_file.c_str()) != 0) {
            std::cerr << "WARNING: Failed to delete temporary version file: " << temp_version_file <<  "\n";
        }

        return latest_version != current_version;
    }


    // unzip source code
    inline bool extract_zip(const std::string& archive_file) {
        std::cout << "Extracting source files from " << archive_file <<  "\n";

#if defined(_WIN32)
        std::string extract_command = "unzip -o " + archive_file;
#elif defined(__linux__) || defined(__APPLE__)
        std::string extract_command = "tar -xf " + archive_file;
#endif

        return system(extract_command.c_str()) == 0;
    }


    // replace binary
    inline void replace_binary(const std::string& new_binary) {
        std::cout << "Replacing current binary..." <<  "\n";

#if defined(_WIN32)
        std::ofstream batchFile("replace_and_restart.bat");
        if (!batchFile) {
            std::cerr << "ERROR: Failed to create batch file for self-replacement." <<  "\n";
            return;
        }

        batchFile << "@echo off\n"
            << "timeout /t 2 >nul\n"
            << "del updater.exe >nul 2>&1\n"
            << "rename \"" << new_binary << "\" \"updater.exe\"\n"
            << "start updater.exe\n"
            << "del %0\n"; // Deletes the batch file after execution
        batchFile.close();

        system("start replace_and_restart.bat");
        exit(0);

#elif defined(__linux__) || defined(__APPLE__)
        std::ofstream scriptFile("replace_and_restart.sh");
        if (!scriptFile) {
            std::cerr << "ERROR: Failed to create shell script for self-replacement." <<  "\n";
            return;
        }

        scriptFile << "#!/bin/sh\n"
            << "sleep 2\n"
            << "rm -f updater\n"
            << "mv \"" << new_binary << "\" updater\n"
            << "chmod +x updater\n"
            << "./updater &\n"
            << "rm -- \"$0\"\n";
        scriptFile.close();

        system("chmod +x replace_and_restart.sh && nohup ./replace_and_restart.sh &");
        exit(0);
#endif
    }

    // generate ninja build file
    inline bool generate_ninja_build_file() {
        std::ofstream buildFile("build.ninja");
        if (!buildFile) {
            std::cerr << "ERROR: Failed to create build.ninja file.\n";
            return false;
        }

#if defined(_WIN32)
        std::string compiler = "cl";
#elif defined(__linux__) || defined(__APPLE__)
        std::string compiler = "g++";
#endif

        buildFile << "rule cxx\n"
            << "  command = " << compiler << " -std=c++20 -o updater_new src/main.cpp\n"
            << "  description = Building updater...\n"
            << "\n"
            << "build updater_new: cxx src/main.cpp\n";

        buildFile.close();

        return true;
    }


    // build from source
    inline void update_from_source(const std::string& source_zip_url) {
        std::cout << "Updating from source..." << std::endl;

        std::string archive_file = "source_code.zip";
        if (!download_file(source_zip_url, archive_file)) {
            std::cerr << "ERROR: Failed to download source archive." << std::endl;
            return;
        }

        if (!extract_zip(archive_file)) return;

        // DELETE source_code.zip AFTER extracting
        if (std::remove(archive_file.c_str()) != 0) {
            std::cerr << "WARNING: Failed to delete " << archive_file << std::endl;
        }

        // MOVE FILES from the extracted folder to the current directory
        std::string extracted_folder = "Cpp_Project_Updater-main";

#if defined(_WIN32)
        std::string move_command = "xcopy /E /Y /Q " + extracted_folder + "\\* .";
#else
        std::string move_command = "cp -r " + extracted_folder + "/* .";
#endif

        if (system(move_command.c_str()) != 0) {
            std::cerr << "ERROR: Failed to move extracted files.\n";
            return;
        }

#if defined(_WIN32)
        std::string remove_command = "rmdir /s /q " + extracted_folder;
#else
        std::string remove_command = "rm -rf " + extracted_folder;
#endif

        if (system(remove_command.c_str()) != 0) {
            std::cerr << "WARNING: Failed to delete extracted folder: " << extracted_folder << std::endl;
        }

        // Ensure build.ninja is created
        if (!generate_ninja_build_file()) {
            std::cerr << "ERROR: Failed to create build.ninja\n";
            return;
        }

        // Run Ninja inside the extracted directory
#if defined(_WIN32)
        std::string ninja_command = "cd " + extracted_folder + " && ninja";
#else
        std::string ninja_command = "cd " + extracted_folder + " && ./ninja";
#endif

        if (system(ninja_command.c_str()) != 0) {
            std::cerr << "ERROR: Compilation failed! Update aborted." << std::endl;
            return;
        }

        std::string new_binary = "updater_new";
#if defined(_WIN32)
        new_binary += ".exe";
#endif

        replace_binary(new_binary);
    }


} // namespace updater

#endif // UPDATER_HPP
