#include "../include/Contents.hpp"
#include "../../constants.h"
#include <iostream>

using namespace Data;

void Contents::checkStringData(const std::string &stringData) {
  if (stringData.empty()) {
    throw std::runtime_error("(Contents) : Empty string found");
  }
}

void Contents::setFileName(const std::string &fileName) {
  checkStringData(fileName);
  this->fileName = fileName;
}

void Contents::setFileContent(const std::string &fileContent) {
  this->fileContent = fileContent;
}

void Contents::setFileType() {
  size_t pos = this->fileName.rfind(".");
  this->fileType = this->fileName.substr(pos + 1, this->fileLength - (pos + 1));
}

void Contents::setFileLength() {
  this->fileLength = this->fileContent.length();
}

void Contents::setFileMemorySize() {
  size_t baseSize = sizeof(std::string);
  size_t memoryAllocSize = this->fileContent.capacity() * sizeof(char);
  this->fileMemorySize = memoryAllocSize + baseSize;
}

void Contents::setUserId(const size_t &userId) {
  this->userId = userId;
}

void Contents::setFileDetails(const std::string &fileName,
                              const std::string &fileContent,
                              const size_t &userId) {
  setFileName(fileName);
  setFileContent(fileContent);

  setFileType();
  setFileLength();
  setFileMemorySize();
  setUserId(userId);
}

void Contents::setFileDetails(const Utils::SimpleJsonParser::JsonDataParams &jsonDataParams,
                              const size_t &userId) {
  std::string fileName = std::get<std::string>(jsonDataParams.at(FILE_NAME));
  std::string fileContent = std::get<std::string>(jsonDataParams.at(FILE_CONTENT));

  setFileName(fileName);
  setFileContent(fileContent);

  setFileType();
  setFileLength();
  setFileMemorySize();
  setUserId(userId);
}

Contents::Contents(const Utils::SimpleJsonParser::JsonDataParams &jsonDataParams,
                   const size_t &userId) {
  if (jsonDataParams.size() > 1) {
    setFileDetails(jsonDataParams, userId);
  } else {
    throw std::runtime_error("(Contents) : insufficient json data fields");
  }
}

Contents::Contents(const std::string &fileName,
                   const std::string &fileContent,
                   const size_t &userId) {
  setFileDetails(fileName, fileContent, userId);
}
