#ifndef DATA_HPP
#define DATA_HPP

#include "DataChunker.hpp"
#include <string>

class Data : protected DataChunker {
public:
  Data() = delete;
  Data(const std::string &);

  std::string getFileName() const { return this->fileName; }
  int getContentLength() const { return this->contentLength; }
  std::string getMainBody() const { return this->mainBody; }
private:
  std::string fileName;
  int contentLength;
  std::string mainBody;

  void setFileName(const std::string &);
  void setFileMainBodyInfo(const std::string &);

  std::string findVariableValue(const std::string &, const std::string &);
  std::string findMainBody(const std::string &);

  void filterData(const std::string &);
};
#endif // DATA_HPP
