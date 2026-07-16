#ifndef DOCTOR_HPP
#define DOCTOR_HPP

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>

inline bool runDoctorCommand() {
    std::cout << "=========================================\n";
    std::cout << " 🩺 NEXUS Environment Doctor Tool\n";
    std::cout << "=========================================\n\n";

    bool allPassed = true;

    // Check 1: Verify g++ compiler
    std::cout << "[doctor] Check 1: Verifying g++ compiler existence... ";
    int gppCheck = std::system("g++ --version > nul 2>&1");
    if (gppCheck == 0) {
        std::cout << "PASSED ✓\n";
    } else {
        std::cout << "FAILED ✗\n";
        std::cout << "  Error: 'g++' was not found on your system PATH.\n";
        std::cout << "  Please install MinGW/MSYS2 and add it to your PATH environment variable.\n";
        allPassed = false;
    }

    // Check 2: Verify project directory folders
    std::cout << "[doctor] Check 2: Verifying standard repository folders... ";
    std::vector<std::string> requiredDirs = {"cli", "compiler", "runtime", "tests", "docs", "examples", "spec"};
    std::vector<std::string> missingDirs;
    for (const auto& dir : requiredDirs) {
        // Attempt to check folder presence by opening a dummy file check or executing directory listing
        std::string testFile = dir + "/.doctor_tmp";
        std::ofstream testOut(testFile);
        if (testOut.is_open()) {
            testOut.close();
            std::remove(testFile.c_str());
        } else {
            missingDirs.push_back(dir);
        }
    }

    if (missingDirs.empty()) {
        std::cout << "PASSED ✓\n";
    } else {
        std::cout << "FAILED ✗\n";
        std::cout << "  Error: Missing folders: ";
        for (const auto& d : missingDirs) {
            std::cout << d << " ";
        }
        std::cout << "\n";
        allPassed = false;
    }

    // Check 3: Verify build scripts presence
    std::cout << "[doctor] Check 3: Verifying build scripts... ";
    std::ifstream buildScript("build.ps1");
    if (buildScript.is_open()) {
        std::cout << "PASSED ✓\n";
        buildScript.close();
    } else {
        std::cout << "FAILED ✗\n";
        std::cout << "  Error: 'build.ps1' was not found in the current working directory.\n";
        allPassed = false;
    }

    std::cout << "\n-----------------------------------------\n";
    if (allPassed) {
        std::cout << " 🎉 NEXUS Environment is healthy and fully ready! ✓\n";
    } else {
        std::cout << " ❌ Some issues were found. Please resolve them to run NEXUS properly.\n";
    }
    std::cout << "-----------------------------------------\n";

    return allPassed;
}

#endif // DOCTOR_HPP
