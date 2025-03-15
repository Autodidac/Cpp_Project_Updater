#pragma once

#include "config.hpp"
#include "build_system.hpp"
#include <iostream>
#include <cstdlib>

// 🔄 **Replace binary with the new compiled version**
inline void replace_binary_from_script(const std::string& new_binary) {
    std::cout << "[INFO] Replacing current binary...\n";

#if defined(_WIN32)
    //clean_up_build_files();

    std::ofstream batchFile(config::REPLACE_RUNNING_EXE_SCRIPT_NAME());
    if (!batchFile) {
        std::cerr << "[ERROR] Failed to create batch file for self-replacement.\n";
        return;
    }

    batchFile << "@echo off\n"
        << "timeout /t 2 >nul\n"
        << "taskkill /IM updater.exe /F >nul 2>&1\n"
        << "timeout /t 1 >nul\n"
        << "del updater.exe >nul 2>&1\n"
        << "if exist updater.exe (\n"
        << "    echo [ERROR] File lock detected. Retrying...\n"
        << "    timeout /t 1 >nul\n"
        << "    goto retry\n"
        << ")\n"
        << "rename \"" << new_binary << "\" \"updater.exe\"\n"
        << "if exist updater.exe (\n"
        << "    echo [INFO] Update successful! Restarting updater...\n"
        << "    start updater.exe\n"
        << "    exit\n"
        << ") else (\n"
        << "    echo [ERROR] Failed to rename new binary. Update aborted!\n"
        << "    exit\n"
        << ")\n";

    batchFile.close();

    std::cout << "[INFO] Running replacement script...\n";
    system(("start /min " + std::string(config::REPLACE_RUNNING_EXE_SCRIPT_NAME())).c_str());
    exit(0);

#elif defined(__linux__) || defined(__APPLE__)
    clean_up_build_files();

    std::ofstream scriptFile("replace_and_restart.sh");
    if (!scriptFile) {
        std::cerr << "[ERROR] Failed to create shell script for self-replacement.\n";
        return;
    }

    scriptFile << "#!/bin/sh\n"
        << "sleep 2\n"
        << "pkill -f updater\n"
        << "sleep 1\n"
        << "if pgrep -f updater; then\n"
        << "    echo \"[WARNING] Process still running, forcing kill...\"\n"
        << "    sleep 1\n"
        << "    pkill -9 -f updater\n"
        << "fi\n"
        << "if [ -f updater ]; then\n"
        << "    rm -f updater\n"
        << "fi\n"
        << "mv \"" << new_binary << "\" updater\n"
        << "chmod +x updater\n"
        << "echo \"[INFO] Update successful! Restarting updater...\"\n"
        << "./updater &\n"
        << "rm -- \"$0\"\n";

    scriptFile.close();

    std::cout << "[INFO] Running replacement script...\n";
    system("chmod +x replace_and_restart.sh && nohup ./replace_and_restart.sh &");
    exit(0);
#endif
}

// 🔄 **Replace the Current Binary**
inline void replace_binary(const std::string& new_binary) {
    std::cout << "[INFO] Replacing binary...\n";
    clean_up_build_files();
#if defined(_WIN32)
    replace_binary_from_script(new_binary);
    //system(("move /Y " + new_binary + " updater.exe").c_str());
#elif defined(__linux__) || defined(__APPLE__)
    replace_binary_from_script(new_binary);
    // system(("mv " + new_binary + " updater").c_str());
#endif
}

// 🔍 **Check for Updates**
bool check_for_updates(const std::string& remote_version_url)
{
    std::cout << "[INFO] Checking for updates...\n";

    std::string temp_version_path = "latest_version.txt";
    //if (!download_file(config::PROJECT_VERSION_URL(), temp_version_path)) {
    //    std::cerr << "[ERROR] Failed to download version file!\n";
    //    return false;
    //}

    // use the parameterized function values rather than default config values
    if (!download_file(remote_version_url, temp_version_path)) {
        std::cerr << "[ERROR] Failed to download version file!\n";
        return false;
    }

    //std::ifstream latest_file(temp_version_file);
    // use the parameterized function values rather than default config values
    std::ifstream latest_file(temp_version_path);
    std::ifstream current_file("version.txt");

    if (!latest_file || !current_file) {
        std::cerr << "[ERROR] Failed to open version files.\n";
        return false;
    }

    std::string latest_version, current_version;
    std::getline(latest_file, latest_version);
    std::getline(current_file, current_version);

    latest_file.close();
    current_file.close();

    std::remove(temp_version_path.c_str());

    return latest_version != current_version;
}

// 🚀 **Install Updater from Binary**
inline bool install_from_binary(const std::string& binary_url) {
    std::cout << "[INFO] Installing from precompiled binary...\n";

    if (!download_file(binary_url, "updater_new.exe")) {
        std::cerr << "[ERROR] Failed to download precompiled binary!\n";
        return false;
    }
    replace_binary("updater_new.exe");
}

// 🚀 **Install Updater from Source**
inline void install_from_source() {
    std::cout << "[INFO] Installing from source...\n";

    if (!setup_7zip()) {
        std::cerr << "[ERROR] 7-Zip installation failed. Cannot continue.\n";
        return;
    }

    if (!download_file(config::PROJECT_SOURCE_URL(), "source_code.zip") || !extract_archive("source_code.zip")) {
        std::cerr << "[ERROR] Failed to download or extract source archive.\n";
        return;
    }

    std::string extracted_folder = config::REPO + "-main";

    // ✅ Debug: Check if files exist before moving
    std::cout << "[DEBUG] Listing extracted files before moving:\n";
#if defined(_WIN32)
    system(("dir /s /b " + extracted_folder).c_str());
#else
    system(("ls -l " + extracted_folder).c_str());
#endif

#if defined(_WIN32)
    // ✅ Use `xcopy` instead of `robocopy`
    std::string move_command = "xcopy /E /Y /Q " + extracted_folder + "\\* .";
#else
    // ✅ Use `mv -T` to move the entire directory correctly
    std::string move_command = "mv -T " + extracted_folder + " ./";
#endif

    if (system(move_command.c_str()) != 0) {
        std::cerr << "[ERROR] Failed to move extracted files.\n";
        return;
    }

#if defined(_WIN32)
    std::string remove_command = "cmd.exe /C \"rmdir /s /q " + extracted_folder + "\"";
#else
    std::string remove_command = "rm -rf " + extracted_folder;
#endif

    if (system(remove_command.c_str()) != 0) {
        std::cerr << "[WARNING] Failed to delete extracted folder: " << extracted_folder << std::endl;
    }

    if (!setup_llvm_clang() || !setup_ninja()) {
        std::cerr << "[ERROR] LLVM/Ninja installation failed. Aborting.\n";
        return;
    }

    if (!generate_ninja_build_file()) {
        std::cerr << "[ERROR] Failed to generate Ninja build file. Aborting.\n";
        return;
    }

    std::cout << "[INFO] Running Ninja...\n";
    if (system("ninja") != 0) {
        std::cerr << "[ERROR] Compilation failed! Update aborted.\n";
        return;
    }

    // you can reverse the redunant fallback order from "binary to source" to "source to binary" in download/usage order
    // by using this and altering the update_from_source() function differently, add binary_url to install_from_source(const std::string& binary_url) 
    //install_from_binary(binary_url);
    replace_binary("updater_new.exe");
}

// 🔄 **Update from Source (Minimal)**
void update_project(const std::string& source_url, const std::string& binary_url){
    if (!check_for_updates(source_url)) {
        std::cout << "[INFO] No updates available.\n";
        return;
    }
#ifdef LEAVE_NO_FILES_ALWAYS_REDOWNLOAD
#if defined(_WIN32)
    system("del /F /Q replace_updater.bat >nul 2>&1"); 
    system(("rmdir /s /q \"" + std::string(config::REPO.c_str()) + "-main\" >nul 2>&1").c_str());

#else
    system("rm -rf replace_updater");
#endif
#endif

    std::cout << "[INFO] Updating from source...\n";
    // get rid of this line if you're looking to reverse redundant fallback order
   // if (!install_from_binary(binary_url))
  //  {
        install_from_source();
   // }
}


// 📌 **Choose Installation Method**
//inline void install_update() {
//#if defined(INSTALL_METHOD_BINARY)
//    install_from_binary(binary_url);
//#elif defined(INSTALL_METHOD_SOURCE)
//    install_from_source();
//#else
//    std::cerr << "[ERROR] No valid installation method defined in config.hpp!\n";
//#endif
//}


/*
#ifndef UPDATER_HPP
#define UPDATER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include "config.hpp"

#if defined(_WIN32)
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/stat.h>
#endif

namespace updater {

    // Extract source archive
    inline bool extract_zip(const std::string& archive_file) {
        std::cout << "Extracting source files from " << archive_file << "\n";

        // Verify the ZIP file exists before extracting
        std::ifstream zip_check(archive_file);
        if (!zip_check) {
            std::cerr << "[ERROR] Archive file not found: " << archive_file << "\n";
            return false;
        }
        zip_check.close();

#if defined(_WIN32)
        std::string extract_command = "powershell -Command \""
            "if (Test-Path " + archive_file + ") { "
            "Expand-Archive -Path " + archive_file + " -DestinationPath . -Force } else { exit 1 }\"";
#elif defined(__linux__) || defined(__APPLE__)
        std::string extract_command = "tar -xf " + archive_file;
#endif

        int result = system(extract_command.c_str());
        if (result != 0) {
            std::cerr << "[ERROR] Failed to extract archive: " << archive_file << "\n";
            return false;
        }

        std::cout << "[INFO] Extraction complete.\n";
        return true;
    }

    inline bool download_llvm_clang() {
        std::cout << "[INFO] Checking for existing Clang installation...\n";

        std::string llvm_dir = "llvm/";
        std::string clang_binary = llvm_dir + "bin/clang++";
#if defined(_WIN32)
        clang_binary += ".exe";
#endif

        std::ifstream clang_check(clang_binary);
        if (clang_check) {
            std::cout << "[INFO] Clang is already installed. Skipping download.\n";
            return true;
        }
        clang_check.close();

        std::cout << "[INFO] Clang not found. Downloading LLVM + Clang...\n";

        std::string llvm_url = config::LLVM_URL;
        std::string llvm_zip = "llvm.zip";

        // **Ensure the `llvm/` directory exists before extracting**
#if defined(_WIN32)
        system("if not exist llvm mkdir llvm");
#else
        system("mkdir -p llvm");
#endif

        if (!download_file(llvm_url, llvm_zip)) {
            std::cerr << "[ERROR] Failed to download LLVM + Clang!\n";
            return false;
        }

        // Validate ZIP file before extraction
        std::ifstream checkFile(llvm_zip, std::ios::binary | std::ios::ate);
        if (!checkFile || checkFile.tellg() < 100000000) {  // Ensure file is at least ~100 MB
            std::cerr << "[ERROR] Downloaded file is corrupt or too small: " << llvm_zip << "\n";
            checkFile.close();
            std::remove(llvm_zip.c_str());
            return false;
        }
        checkFile.close();

        std::cout << "[INFO] Extracting Clang + LLVM...\n";

#if defined(_WIN32)
        std::string extract_command = "powershell -Command \"Expand-Archive -Path '" + llvm_zip + "' -DestinationPath '" + llvm_dir + "' -Force\"";
#elif defined(__linux__) || defined(__APPLE__)
        std::string extract_command = "tar -xf " + llvm_zip + " -C " + llvm_dir;
#endif

        int extraction_result = system(extract_command.c_str());
        if (extraction_result != 0) {
            std::cerr << "[ERROR] Failed to extract LLVM + Clang! Archive might be corrupt.\n";
            return false;
        }

        std::cout << "[INFO] Cleaning up LLVM installation files...\n";
        std::remove(llvm_zip.c_str());

        std::cout << "[INFO] LLVM + Clang installed successfully!\n";
        return true;
    }


    // Check for updates by comparing version files
    inline bool check_for_updates(const std::string& latest_version_url, const std::string& local_version_file) {
        std::cout << "Checking for updates from: " << latest_version_url << "\n";

        std::string temp_version_file = "latest_version.txt";
        if (!download_file(latest_version_url, temp_version_file)) {
            return false;
        }

        std::ifstream latest_file(temp_version_file);
        std::ifstream current_file(local_version_file);

        if (!latest_file || !current_file) {
            std::cerr << "ERROR: Failed to open version files.\n";
            return false;
        }

        std::string latest_version, current_version;
        std::getline(latest_file, latest_version);
        std::getline(current_file, current_version);

        latest_file.close();
        current_file.close();

        std::remove(temp_version_file.c_str());

        return latest_version != current_version;
    }

    // Clean up build files
    inline void clean_up_build_files() {
        std::cout << "[INFO] Cleaning up build artifacts...\n";

#if defined(_WIN32)
        system("del /F /Q build.ninja build_with_msvc.bat main.obj ninja.zip >nul 2>&1");
#elif defined(__linux__) || defined(__APPLE__)
        system("rm -f build.ninja build_with_msvc.bat main.obj ninja.zip");
#endif

        std::cout << "[INFO] Cleanup complete.\n";
    }

    // Replace binary with the new compiled version
    inline void replace_binary(const std::string& new_binary) {
        std::cout << "Replacing current binary...\n";

#if defined(_WIN32)
        clean_up_build_files();

        std::ofstream batchFile(config::REPLACE_SCRIPT_WINDOWS);
        if (!batchFile) {
            std::cerr << "ERROR: Failed to create batch file for self-replacement.\n";
            return;
        }

        batchFile << "@echo off\n"
            << "timeout /t 2 >nul\n"
            << "taskkill /IM updater.exe /F >nul 2>&1\n"
            << "timeout /t 1 >nul\n"
            << "del updater.exe >nul 2>&1\n"
            << "if exist updater.exe (\n"
            << "    timeout /t 1 >nul\n"
            << "    goto retry\n"
            << ")\n"
            << "rename \"" << new_binary << "\" \"updater.exe\"\n"
            << "if exist updater.exe (\n"
            << "    start updater.exe\n"
            << "    exit\n"
            << ") else (\n"
            << "    echo ERROR: Failed to rename new binary. Update aborted!\n"
            << "    exit\n"
            << ")\n";

        batchFile.close();

        system(("start /min " + std::string(config::REPLACE_SCRIPT_WINDOWS)).c_str());
        exit(0);

#elif defined(__linux__) || defined(__APPLE__)
        clean_up_build_files();

        std::ofstream scriptFile("replace_and_restart.sh");
        if (!scriptFile) {
            std::cerr << "ERROR: Failed to create shell script for self-replacement.\n";
            return;
        }

        scriptFile << "#!/bin/sh\n"
            << "sleep 2\n"
            << "pkill -f updater\n"
            << "sleep 1\n"
            << "if pgrep -f updater; then\n"
            << "    sleep 1\n"
            << "    pkill -9 -f updater\n"
            << "fi\n"
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

    // Generate Ninja build file
    inline bool generate_ninja_build_file() {
        std::ofstream buildFile("build.ninja");
        if (!buildFile) {
            std::cerr << "ERROR: Failed to create build.ninja file.\n";
            return false;
        }

        std::string llvm_dir = "llvm/";
        std::string compiler = "\"" + llvm_dir + "bin/clang++\"";  // Ensure proper path format
        std::string linker = "\"" + llvm_dir + "bin/lld\"";
        std::string stdlib_flag = "-nostdinc++ -stdlib=libc++ -I \"" + llvm_dir + "include/c++/v1\"";
        std::string include_flag = "-I include";
        std::string output_flag = "-o " + std::string(config::OUTPUT_BINARY);

        buildFile << "rule cxx\n"
            << "  command = " << compiler << " " << include_flag << " " << stdlib_flag << " " << output_flag
            << " -std=c++20 -fuse-ld=" << linker << " " << config::SOURCE_MAIN_FILE << "\n"
            << "  description = Building updater with self-contained Clang...\n"
            << "\n"
            << "build " << config::OUTPUT_BINARY << ": cxx " << config::SOURCE_MAIN_FILE << "\n";

        buildFile.close();
        return true;
    }




    // Download and install Ninja
    inline bool download_ninja() {
        std::cout << "Downloading Ninja build system...";

#if defined(_WIN32)
        std::string ninja_url = "https://github.com/ninja-build/ninja/releases/latest/download/ninja-win.zip";
        std::string ninja_zip = "ninja.zip";
        std::string ninja_binary = "ninja.exe";
#elif defined(__linux__) || defined(__APPLE__)
        std::string ninja_url = "https://github.com/ninja-build/ninja/releases/latest/download/ninja-linux.zip";
        std::string ninja_zip = "ninja.zip";
        std::string ninja_binary = "ninja";
#endif

        if (!download_file(ninja_url, ninja_zip)) {
            std::cerr << "ERROR: Failed to download Ninja.";
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


    // generate ninja build file
//    inline bool generate_ninja_build_file() {
//        std::ofstream buildFile("build.ninja");
//        if (!buildFile) {
//            std::cerr << "ERROR: Failed to create build.ninja file.\n";
//            return false;
//        }
//
//#if defined(_WIN32)
//        std::string compiler_setup = "call \"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat\" && ";
//        std::string compiler = "cl";
//        std::string include_flag = "/I include"; // Add include path for Windows
//        std::string output_flag = "/Feupdater_new"; // Use correct output flag for MSVC
//        std::string exception_flag = "/EHsc"; // Enable C++ exception handling
//
//        // Create a batch script that sets up MSVC and runs Ninja
//        std::ofstream batchFile("build_with_msvc.bat");
//        if (!batchFile) {
//            std::cerr << "ERROR: Failed to create batch file for MSVC setup.\n";
//            return false;
//        }
//
//        batchFile << "@echo off\n"
//            << "call \"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat\"\n"
//            << "ninja\n";
//        batchFile.close();
//
//        buildFile << "rule cxx\n"
//            << "  command = " << compiler << " " << include_flag << " " << output_flag << " " << exception_flag << " -std:c++20 src/main.cpp\n"
//            << "  description = Building updater...\n"
//            << "\n"
//            << "build updater_new: cxx src/main.cpp\n";
//
//#else
//        std::string compiler = "g++";
//        std::string include_flag = "-I include"; // Add include path for Linux/macOS
//        std::string output_flag = "-o updater_new";
//
//        buildFile << "rule cxx\n"
//            << "  command = " << compiler << " " << include_flag << " " << output_flag << " -std=c++20 src/main.cpp\n"
//            << "  description = Building updater...\n"
//            << "\n"
//            << "build updater_new: cxx src/main.cpp\n";
//#endif
//
//        buildFile.close();
//
//        return true;
//    }


    // verify clang is installed
    inline void update_from_source(const std::string& source_zip_url) {
        std::cout << "Updating from source..." << std::endl;

        std::string archive_file = "source_code.zip";
        if (!download_file(source_zip_url, archive_file)) {
            std::cerr << "[ERROR] Failed to download source archive." << std::endl;
            return;
        }

        if (!extract_zip(archive_file)) {
            std::cerr << "[ERROR] Extraction failed! Cannot proceed with update." << std::endl;
            return;
        }

        if (!verify_clang_installation()) {
            std::cerr << "[ERROR] Clang/LLD setup failed. Cannot proceed with update.\n";
            return;
        }

        if (!generate_ninja_build_file()) {
            std::cerr << "ERROR: Failed to create build.ninja\n";
            return;
        }

        std::cout << "[INFO] Listing extracted directories before running Ninja...\n";
#if defined(_WIN32)
        system("dir /s /b llvm\\bin");
#else
        system("ls -lR llvm/bin");
#endif

#if defined(_WIN32)
        std::string ninja_command = "set PATH=%CD%\\llvm\\bin;%PATH% && ninja";
#else
        std::string ninja_command = "export PATH=\"$PWD/llvm/bin:$PATH\" && ./ninja";
#endif

        std::cout << "[INFO] Running Ninja build system...\n";

        if (system(ninja_command.c_str()) != 0) {
            std::cerr << "[ERROR] Compilation failed! Update aborted.\n";
            return;
        }

        std::string new_binary = "updater_new";
#if defined(_WIN32)
        new_binary += ".exe";
#endif

        replace_binary(new_binary);
    }


    // build and update from source code
    inline void update_from_source(const std::string& source_zip_url) {
        std::cout << "Updating from source..." << std::endl;

        std::string archive_file = "source_code.zip";
        if (!download_file(source_zip_url, archive_file)) {
            std::cerr << "[ERROR] Failed to download source archive." << std::endl;
            return;
        }

        if (!extract_zip(archive_file)) {
            std::cerr << "[ERROR] Extraction failed! Cannot proceed with update." << std::endl;
            return;
        }

        std::string extracted_folder = "Cpp_Project_Updater-main";

#if defined(_WIN32)
        std::string move_command = "xcopy /E /Y /Q " + extracted_folder + "\\* .";
#else
        std::string move_command = "cp -r " + extracted_folder + "/* .";
#endif

        if (system(move_command.c_str()) != 0) {
            std::cerr << "[ERROR] Failed to move extracted files.\n";
            return;
        }

#if defined(_WIN32)
        std::string remove_command = "rmdir /s /q " + extracted_folder;
#else
        std::string remove_command = "rm -rf " + extracted_folder;
#endif

        if (system(remove_command.c_str()) != 0) {
            std::cerr << "[WARNING] Failed to delete extracted folder: " << extracted_folder << std::endl;
        }

        if (!verify_clang_installation()) {
            std::cerr << "[ERROR] Clang/LLD setup failed. Cannot proceed with update.\n";
            return;
        }

        if (!generate_ninja_build_file()) {
            std::cerr << "ERROR: Failed to create build.ninja\n";
            return;
        }

#if defined(_WIN32)
        std::string ninja_command = "set PATH=%CD%\\llvm\\bin;%PATH% && ninja";
#else
        std::string ninja_command = "export PATH=\"$PWD/llvm/bin:$PATH\" && ./ninja";
#endif

        if (system(ninja_command.c_str()) != 0) {
            std::cerr << "[ERROR] Compilation failed! Update aborted.\n";
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
*/