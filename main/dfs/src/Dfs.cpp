#include "../include/Dfs.hpp"
#include "../../data/include/Contents.hpp"
#include "../../data/include/Chunker.hpp"

#include "../../utils/include/query/SimpleQueryParser.hpp"
#include "../../storage/include/AwsS3.hpp"
#include "../../constants.h"

#include <algorithm>
#include <boost/beast.hpp>
#include <boost/beast/core/detail/base64.hpp>

using namespace Nexus;

size_t Dfs::getUserId(const std::string &username) {
  auto jsonData = Utils::SimpleJsonParser::JsonBuilder()
    .singleData("file", "GetUserId")
    .singleData("username", username)
    .getJsonData();

  auto queryData = Utils::SimpleQueryParser::parseQuery(DFS_QUERIES_DIR, jsonData);
  MetaData metaData;
  auto queryOutput = metaData.getQueryDataMap(queryData);
  return std::stoi(queryOutput.at("id").at(0));
}

DfsResult Dfs::storeDataApi(const std::string &fileName,
                            const std::string &fileContent,
                            const std::string &username) { 
  std::string output, errorMsg;
  DfsResult::ResultType resultType = DfsResult::ResultType::SUCCESS;

  try {
    size_t userId = getUserId(username);
    Data::Contents contents {fileName, fileContent, userId};
    Data::Chunker chunker {contents};
    auto &chunks = chunker.getChunks();

    Storage::AwsS3 awsS3;
    awsS3.storeData(chunks);
    output = "true";
  } catch (std::exception &e) {
    errorMsg = e.what();
    output = "false";
    resultType = DfsResult::ResultType::FAILED;
  }

  return {output, errorMsg, resultType};
}

size_t Dfs::getFileId(const std::string &fileName,
                      const FileType &fileType,
                      const size_t &userId) {
  std::string queryFileName;

  if (fileType == FileType::NORMAL) {
    queryFileName = "GetFileIdFromName";
  } else if (fileType == FileType::TRASH) {
    queryFileName = "GetTrashFileIdFromName";
  }
  auto jsonData = Utils::SimpleJsonParser::JsonBuilder()
    .singleData("file", queryFileName)
    .singleData("name", fileName)
    .singleData("user_id", std::to_string(userId))
    .getJsonData();

  auto queryData = Utils::SimpleQueryParser::parseQuery(DFS_QUERIES_DIR, jsonData);
  MetaData metaData;
  auto queryOutput = metaData.getQueryDataMap(queryData);
  return std::stoi(queryOutput.at("id").at(0));
}

std::string Dfs::getCacheData(const size_t &fileId) {
  std::string output;
  try {
    auto chunks = Data::Cache::getData(fileId);
    for (const auto &chunk : chunks) {
      output += chunk->getDecryptedData();
    }
  } catch (std::exception &e) {
    throw std::runtime_error(e.what());
  }
  return output;
}

const bool Dfs::checkIfVideoOrImage(const std::string &fileName) const {
  const std::vector<std::string> imageExtensions = {
    "jpg", "jpeg", "png", "gif", "bmp", "tiff", "webp", "svg"
  };
  const std::vector<std::string> videoExtensions = {
    "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "3gp", "mpeg"
  };

  size_t extPos = fileName.rfind(".");
  std::string ext = fileName.substr(extPos + 1);
  auto itOne = std::find(imageExtensions.begin(), imageExtensions.end(), ext);
  auto itTwo = std::find(videoExtensions.begin(), videoExtensions.end(), ext);

  return (itOne != imageExtensions.end()) || (itTwo != videoExtensions.end());
}

std::string Dfs::base64Encode(const std::string &data) {
  using namespace boost::beast::detail;

  std::string output;
  output.resize(base64::encoded_size(data.size()));
  base64::encode(&output[0], data.data(), data.size());

  return output;
}

DfsResult Dfs::getDataApi(const std::string &fileName,
                          const FileType &fileType,
                          const std::string &username) {
  std::string output, errorMsg;
  DfsResult::ResultType resultType = DfsResult::ResultType::SUCCESS;
  std::future<void> cacheThread;

  try {
    size_t userId = getUserId(username);
    size_t fileId = getFileId(fileName, fileType, userId);
    /* bool isVideoOrImage = checkIfVideoOrImage(fileName); */
    try {
      output = getCacheData(fileId);
    } catch (const std::exception &) {
      Storage::AwsS3 awsS3;
      if (fileType == FileType::NORMAL) {
        auto chunks = awsS3.getData(fileId);

        for (const auto &chunk : chunks) {
          output += chunk->getDecryptedData();
        }
        Data::Cache::storeData(chunks);
      } else if (fileType == FileType::TRASH) {
        auto chunks = awsS3.getBackupData(fileId);

        for (const auto &chunk : chunks) {
          output += chunk->getDecryptedData();
        }
      }
    }

    /* if (isVideoOrImage) { */
    /*   output = base64Encode(output); */
    /*   std::cout << output << std::endl; */
    /* } */ 
  } catch (std::exception &e) {
    errorMsg = e.what();
    output = "Failed to fetch data";
    resultType = DfsResult::ResultType::FAILED;
  }

  return {output, errorMsg, resultType};
}

DfsResult Dfs::getDataApi(const std::string &fileName,
                          const std::string &username) {
  return getDataApi(fileName, FileType::NORMAL, username);
}

DfsResult Dfs::getTrashDataApi(const std::string &fileName,
                               const std::string &username) {
  return getDataApi(fileName, FileType::TRASH, username);
}

DfsResult Dfs::deleteDataApi(const std::string &fileName,
                             const FileType &fileType,
                             const std::string &username) {
  std::string output, errorMsg;
  DfsResult::ResultType resultType = DfsResult::ResultType::SUCCESS;

  try {
    size_t userId = getUserId(username);
    size_t fileId = getFileId(fileName, fileType, userId);
    Storage::AwsS3 awsS3;
    if (fileType == FileType::NORMAL) {
      awsS3.deleteData(fileId);
    } else if (fileType == FileType::TRASH) {
      Data::Cache::deleteData(fileId);
      awsS3.deleteBackupData(fileId);
    }
    output = "Data deleted successfully";

  } catch (std::exception &e) {
    errorMsg = e.what();
    output = "Unable to delete the provided data";
    resultType = DfsResult::ResultType::FAILED;
  }
  return {output, errorMsg, resultType};
}

DfsResult Dfs::deleteDataApi(const std::string &fileName,
                             const std::string &username) {
  return deleteDataApi(fileName, FileType::NORMAL, username);
}

DfsResult Dfs::deleteTrashDataApi(const std::string &fileName,
                                  const std::string &username) {
  return deleteDataApi(fileName, FileType::TRASH, username);
}

DfsResult Dfs::listDataApi(const FileType &fileType,
                           const std::string &username) {
  DfsResult::MessageMap messageMap;

  std::string output, errorMsg;
  DfsResult::ResultType resultType = DfsResult::ResultType::SUCCESS;

  try {
    std::string queryFile;
    size_t userId = getUserId(username);
    if (fileType == FileType::NORMAL) {
      queryFile = "ListFileData";
    } else if (fileType == FileType::TRASH) {
      queryFile = "ListTrashFileData";
    }
    auto jsonData = Utils::SimpleJsonParser::JsonBuilder()
      .singleData("file", queryFile)
      .singleData("user_id", std::to_string(userId))
      .getJsonData();

    auto queryData = Utils::SimpleQueryParser::parseQuery(DFS_QUERIES_DIR, jsonData);
    MetaData metaData;
    auto queryOutput = metaData.getQueryDataMap(queryData);

    for (const auto &key : queryOutput) {
      messageMap.insert({key.first, key.second});
    }
  } catch (const std::exception &e) {
    errorMsg = e.what();
    output = "Failed to fetch the data";
    resultType = DfsResult::ResultType::FAILED;
    return {output, errorMsg, resultType};
  }
  return {messageMap, errorMsg, resultType};
}

DfsResult Dfs::listDataApi(const std::string &username) {
  return listDataApi(FileType::NORMAL, username);
}

DfsResult Dfs::listTrashDataApi(const std::string &username) {
  return listDataApi(FileType::TRASH, username);
}

DfsResult Dfs::addUser(const std::string &username,
                       const std::string &password) {
  std::string output, errorMsg;
  DfsResult::ResultType resultType = DfsResult::ResultType::SUCCESS;

  try {
    User::addUser(username, password);
    output = "true";
  } catch (const std::exception &e) {
    errorMsg = e.what();
    output = "false";
    resultType = DfsResult::ResultType::FAILED;
  }
  return {output, errorMsg, resultType};
}

DfsResult Dfs::checkUser(const std::string &username,
                    const std::string &password) {
  std::string output, errorMsg;
  DfsResult::ResultType resultType = DfsResult::ResultType::SUCCESS;

  try {
    bool result = User::checkUser(username, password);
    if (result) {
      output = "true";
    } else {
      output = "false";
    }
  } catch (const std::exception &e) {
    errorMsg = e.what();
    output = "Unable to check user";
    resultType = DfsResult::ResultType::FAILED;
  }
  return {output, errorMsg, resultType};
}
