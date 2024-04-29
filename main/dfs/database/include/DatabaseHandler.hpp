#ifndef DATABASE_HANDLER_HPP
#define DATABASE_HANDLER_HPP
#include "../../../variables.h"
#include <mysql/mysql.h>
#include <string>
#include <unordered_map>
#include <vector>

class DatabaseHandler {
protected:
  void checkConnection();

  void storeData(const std::string &);

  std::unordered_map<int, std::vector<std::string>> getDataByRow();
  std::unordered_map<int, std::vector<std::string>> getDataByColumn();

  ~DatabaseHandler();
private:
  void mysqlConnectionSetup();

  struct ConnectionDetails {
    const char *server = SERVER;
    const char *database = DATABASE;
    const char *username = USERNAME;
    const char *password = PASSWORD;
  };

  MYSQL *connection {nullptr};
};
#endif // DATABASE_HANDLER_HPP
