#pragma once

#include "WinCmd.h"
#include <fstream>

void DrawGraph(std::wstring tag, std::string dotProgram)
{
    std::ofstream ofs(tag + L".txt", std::ifstream::out);
    ofs << dotProgram;
    ofs.close();
    // Show graphviz graph.
    Run(L"..\\Tools\\draw.bat "+ tag + L".txt");
}
