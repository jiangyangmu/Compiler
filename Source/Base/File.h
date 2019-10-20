#pragma once

#include <string>

std::string GetCurrentWorkDirectory();

std::string GetCanonicalFileDirectory(std::string fileName);

std::string GetFileContent(const char * fileName);

void SetFileContent(const char * fileName,
                    const std::string & content);

std::string ChangeFileExtention(const std::string & filename,
                                std::string from,
                                std::string to);