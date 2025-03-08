#include "MysqlDao.hpp"
#include "configMgr.hpp"
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/statement.h>
#include <memory>
#include <string>

MysqlDao::MysqlDao(){
    auto &cfg = ConfigMgr::GetInstance();
    const auto& host = cfg["Mysql"]["Host"];
    const auto& port = cfg["Mysql"]["Port"];
    const auto& pwd = cfg["Mysql"]["Passwd"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user = cfg["Mysql"]["User"];
    pool_.reset(new MySqlPool(host + ":" + port, user, pwd, schema, 5));
}
MysqlDao::~MysqlDao(){
    pool_->Close();
}

int MysqlDao::RegUser(const std::string &name,  const std::string &email, const std::string &pwd){
    auto con = pool_->getConnection();
    std::unique_ptr<sql::ResultSet> res = nullptr;

    Defer defer([&con,&res, this](){
        if(res){
            res->close();
        }
        pool_->returnConnection(std::move(con));
    });
    try{
        if(con == nullptr){
            return -1;
        }
        
        //准备调用存储过程
        std::unique_ptr<sql::PreparedStatement> stmt(con->con_->prepareStatement(
            "CALL reg_user(?,?,?,@result)"
        ));
        
        //设置输出参数
        stmt->setString(1, name); 
        stmt->setString(2, email);
        stmt->setString(3, pwd);
        
        //执行存储过程
        stmt->execute();
        
        std::unique_ptr<sql::Statement> stmtResult(con->con_->createStatement());
        res.reset(stmtResult->executeQuery("SELECT @result AS result"));
        
        if(res->next()){
            int result = res->getInt("result");
            std::cout << "result is " << result  << std::endl;
            return result;
        }
        return -1;
    }
    catch(sql::SQLException &e){
        std::cerr << "SQL EXCEPTION: " << e.what(); 
        std::cerr << " (ErrorCode: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() <<")" << std::endl;
        
        return -1;
    }
}
bool MysqlDao::CheckEmail(const std::string &name, const std::string &email) {
    auto con = pool_->getConnection();
    std::unique_ptr<sql::ResultSet> res = nullptr;
    Defer defer([&con, &res, this](){
        if(res){
            res->close();
        }
        pool_->returnConnection(std::move(con));
    });
    try{
        if(con == nullptr){
            return false;
        }
        
        std::unique_ptr<sql::PreparedStatement> stmt(con->con_->prepareStatement(
            "SELECT email FROM user WHERE name = ?"
        ));
        
        //绑定参数
        stmt->setString(1, name);
        
        //执行查询
        res.reset(stmt->executeQuery());

        //遍历结果集
        if(res->next()){
            std::cout << "Check email" << res->getString("email") << std::endl;
            if (email != res->getString("email")) {
                return false;
            }
            
            return true;
        }
        std::cout << "cehck email failed, user name does not exist" << std::endl;
        return false;
    }
    catch(sql::SQLException &e){
        std::cerr << "SQL EXCEPTION: " << e.what(); 
        std::cerr << " (ErrorCode: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() <<")" << std::endl;
        
        return false;
    }
    return false;

}
bool MysqlDao::UpdatePwd(const std::string &name, const std::string &newpwd) {
    auto con = pool_->getConnection();
    Defer defer([&con, this](){
        pool_->returnConnection(std::move(con));
    });
    try{
        if(con == nullptr){
            return false;
        }
        
        std::unique_ptr<sql::PreparedStatement> stmt(con->con_->prepareStatement(
            "UPDATE user SET pwd = ? WHERE name = ?"
        ));
        
        //绑定参数
        stmt->setString(1, newpwd);
        stmt->setString(2, name);
        
        //执行更新
        int updateCount = stmt->executeUpdate();

        std::cout << "update rows " << updateCount << std::endl;
        return true;
    }
    catch(sql::SQLException &e){
        std::cerr << "SQL EXCEPTION: " << e.what(); 
        std::cerr << " (ErrorCode: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() <<")" << std::endl;
        
        return false;
    }
}
bool MysqlDao::CheckPwd(const std::string &email, const std::string &pwd,
                        UerInfo &info){
    auto con = pool_->getConnection();
    std::unique_ptr<sql::ResultSet> res = nullptr;
    Defer defer([&con, &res,  this]() { 
        if(res) res->close();
        pool_->returnConnection(std::move(con)); });
    try {
        if (con == nullptr) {
            return false;
        }
    std:
        std::unique_ptr<sql::PreparedStatement> stmt(
            con->con_->prepareStatement("SELECT * FROM user WHERE email = ? "));

        // 绑定参数
        stmt->setString(1, email);

        // 执行查询
        res.reset(stmt->executeQuery());
        std::string originPwd = "";
        while (res->next()) {
            originPwd = res->getString("pwd");
            std::cout << " Passwd is " << originPwd << std::endl;
            break;
        }

        if (pwd != originPwd) {
            return false;
        }

        info.name = res->getString("name");
        info.email = email;
        info.pwd = originPwd;
        info.uid = res->getInt("uid");

        return true;
    } catch (sql::SQLException &e) {
        std::cerr << "SQL EXCEPTION: " << e.what();
        std::cerr << " (ErrorCode: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << ")" << std::endl;

        return false;
    }
                        }