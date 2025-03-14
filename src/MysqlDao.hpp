#pragma once
#include "const.h"
#include <jdbc/cppconn/connection.h>
#include <memory>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include <atomic>

class SqlConnection {
    friend class MySqlPool;
    friend class MysqlDao;
  public:
    SqlConnection(sql::Connection *con, int64_t lastTime)
        : con_(con), lastTime_(lastTime) {}

  private:
    std::unique_ptr<sql::Connection> con_;
    int64_t lastTime_;
};
class MySqlPool{
  public:
    MySqlPool(const std::string &url, const std::string &user,
              const std::string &pass, const std::string &schema, int poolSize)
        : url_(url),
          user_(user),
          pass_(pass),
          schema_(schema),
          poolSize_(poolSize),
          bStop_(false) {
        try {
            for (int i = 0; i < poolSize_; ++i) {

                sql::mysql::MySQL_Driver *driver =
                    sql::mysql::get_mysql_driver_instance();
                auto con(driver->connect(url_, user_, pass_));
                con->setSchema(schema_);

                // 获取当前时间戳
                auto currentTime =
                    std::chrono::system_clock::now().time_since_epoch();
                // 转换成秒
                long long timeStap =
                    std::chrono::duration_cast<std::chrono::seconds>(
                        currentTime)
                        .count();
                pool_.push(std::make_unique <SqlConnection>(con, timeStap));
            }
            
            // 启动一个线程，定时检查连接是否有效
            checkThread_ = std::thread([this](){
                    while(!bStop_){
                        checckConnection();
                        std::this_thread::sleep_for(std::chrono::seconds(60));
                    }
            });
        } catch (sql::SQLException &e) {
            std::cout << "mysql init failed" << std::endl;
        }
    }
    void checckConnection(){
            std::lock_guard<std::mutex> lock(mutex_);
            // 获取当前时间戳
            auto currentTime =
                std::chrono::system_clock::now().time_since_epoch();
            // 转换成秒
            long long timeStap =
                std::chrono::duration_cast<std::chrono::seconds>(currentTime)
                    .count();
            for(int i = 0; i < pool_.size(); ++i){
                auto con = std::move(pool_.front());
                pool_.pop();
                Defer defer([this, &con]{
                    pool_.push(std::move(con));
                });
                
                if(timeStap - con->lastTime_ < 5){
                    continue;
                }
                
                try{
                    std::unique_ptr<sql::Statement> stmt(con->con_->createStatement());
                    stmt->execute("select 1");
                    con->lastTime_ = timeStap;
                    std::cout << " execute timer alive query, query is " << timeStap << std::endl;
                }
                catch(sql::SQLException &e){
                    std::cout << "Error keeping connection alive :" << e.what()<< std::endl;
                    sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
                    auto *newCon = driver->connect(url_, user_, pass_);
                    newCon->setSchema(schema_);
                    con->con_.reset(newCon);
                    con->lastTime_ = timeStap;
                }
            }
        }

        std::unique_ptr<SqlConnection> getConnection() {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [this] { return bStop_ || !pool_.empty(); });
            if (bStop_) {
                return nullptr;
            }
            std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
            pool_.pop();
            return con;
        }

        void returnConnection(std::unique_ptr<SqlConnection> con){
            if (bStop_)
                return;
            std::unique_lock<std::mutex> lock(mutex_);
            pool_.push(std::move(con));
            cond_.notify_one();
        }
        
        void Close(){
            bStop_ = true;
            cond_.notify_all();
        }
        ~MySqlPool(){
            std::lock_guard<std::mutex> lock(mutex_);
            while(pool_.empty() == false){
                pool_.pop();
            }
        }

private:
    std::string url_;
    std::string user_;
    std::string pass_;
    std::string schema_;
    int poolSize_;
    std::queue<std::unique_ptr<SqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic_bool bStop_;
    std::thread checkThread_;
};

struct UerInfo{
    std::string name;
    std::string pwd;
    int uid;
    std::string email;
};

class MysqlDao{
public:
    MysqlDao();
    ~MysqlDao();
    int RegUser(const std::string &name,  const std::string &email, const std::string &pwd);
    bool CheckEmail(const std::string &name , const std::string &email);
    bool UpdatePwd(const std::string &name, const std::string &newpwd);
    bool CheckPwd(const std::string &email, const std::string &pwd, UerInfo &info);
private:
    std::unique_ptr<MySqlPool> pool_;
};