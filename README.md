# 分布式全栈聊天服务器的网关服务器
## 主要功能
 1. 验证码服务
 2. 用户注册
 3. 用户登录
 4. 施工中。。。。。

## 技术细节
 - 分布式
    - grpc实现多服务器间通信
    - dockerfile封装便于快速部署
 - 服务器
    - 基于asio网络库，高效的Proactor网络模型
    - 封装mysql连接池，redis连接池，模板化grpc池
    - 高效的消息传递，事件处理机制
 - docker
    - dockerfile多阶段构建
        - base-image封装基础构建环境
        - build阶段拉取代码编译构建
        - final阶段拉取新镜像，从build迁移最小依赖
    - docker缓存加速
        - 利用docker缓存机制优化base-image大小（4.9G->900M)
        - 构建缓存推送ghrc方便以后构建的加速
 - CI/CD
    - 高度可复用的GitHub Action部署脚本
    - 利用github虚拟机完成必要的代码构建工作
    - ssh到生产服务器上使用dockercompose up -d一键拉起更新实例
