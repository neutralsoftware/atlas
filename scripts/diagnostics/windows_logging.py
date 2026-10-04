from pathlib import Path
import re
import subprocess
import tempfile


root = Path(__file__).resolve().parents[2]
source = (root / "editor/main.cpp").read_text()
logging = re.search(r"struct RuntimeLogOutput\s*\{.*?\}\s*runtimeLogOutput;", source, re.S).group(0)
harness = r'''
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
struct QString {
    std::wstring value;
    std::wstring toStdWString() const { return value; }
};
''' + logging + r'''
int main() {
    auto output = std::cout.rdbuf();
    auto error = std::cerr.rdbuf();
    {
        RuntimeLogOutput log;
        log.open(QString{L"runtime.log"});
        std::cout << "runtime output\n";
        std::cerr << "runtime error\n";
        if (std::fputs("Qt diagnostic remains writable\n", stderr) < 0)
            throw std::runtime_error("CRT stderr was invalidated");
        std::ifstream file("runtime.log");
        std::string text((std::istreambuf_iterator<char>(file)), {});
        if (text != "runtime output\nruntime error\n")
            throw std::runtime_error("Runtime logging did not flush both streams");
    }
    if (std::cout.rdbuf() != output || std::cerr.rdbuf() != error)
        throw std::runtime_error("Runtime logging did not restore streams");
    {
        RuntimeLogOutput log;
        log.open(QString{L"missing-directory/runtime.log"});
        if (std::cout.rdbuf() != output || std::cerr.rdbuf() != error)
            throw std::runtime_error("Failed log creation changed the streams");
        std::cerr << "Log creation failure remains safe\n";
    }
    std::cout << "Windows runtime logging passed\n";
}
'''
with tempfile.TemporaryDirectory() as directory:
    test = Path(directory) / "logging.cpp"
    test.write_text(harness)
    subprocess.run(["cl", "/nologo", "/std:c++20", "/EHsc", str(test),
                    "/Fe:logging.exe"], cwd=directory, check=True)
    subprocess.run([str(Path(directory) / "logging.exe")], cwd=directory, check=True)
