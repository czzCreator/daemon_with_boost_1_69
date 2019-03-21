#ifndef QMAINSERVER_H
#define QMAINSERVER_H

#include <QDebug>

//关于 boost中 锁的介绍可看以下
/// https://www.cnblogs.com/happenlee/p/9747743.html
/// https://www.cnblogs.com/lidabo/p/3785176.html
/// boost库中提供了mutex类与lock类，通过组合可以轻易的构建读写锁与互斥锁。
/// mutex是对象类, lock是模板类.
#include <boost/thread/mutex.hpp>

#include "qio_service_pool.h"

#include "qsession.h"
#include "qsession_clientside.h"

#define MAIN_SERVER_PORT 18080

class QMainServer: public boost::enable_shared_from_this<QMainServer>
{
public:

    /* 简单来说：
     * shared_lock是读锁。被锁后仍允许其他线程执行同样被shared_lock的代码
     * unique_lock是写锁。被锁后不允许其他线程执行被shared_lock或unique_lock的代码。它可以同时限制unique_lock与share_lock
     *
     * 适用于多线程 并发 --->多读 少写的情况
    */
    typedef boost::shared_lock<boost::shared_mutex> own_read_lock;
    typedef boost::unique_lock<boost::shared_mutex> own_write_lock;
    typedef boost::shared_ptr<QMainServer> main_service_ptr;

    /// 一个pool io_service.run() 事件循环专门用于处理socket通信事件 读写流等
    /// 一个pool io_service.run() 事件循环专门用于处理 收到的数据解析 数据流解析等其他内容?
    QMainServer(short port,std::size_t io_service_pool_size)
        :
          m_io_service_pool(io_service_pool_size,"sock_comm_pool"),
          m_io_service_work_pool(io_service_pool_size,"data_deal_pool"),
          m_acceptor_(m_io_service_pool.get_io_service(),tcp::endpoint(tcp::v4(),port))
    {
        m_session_manager_list.clear();   //初始化

        qsession_ptr new_session(new QSession(
                                     m_io_service_work_pool.get_io_service(),
                                     m_io_service_pool.get_io_service()));


        m_acceptor_.async_accept(new_session->socket(),
                                 boost::bind(&QMainServer::handle_accept,
                                             this,
                                             boost::asio::placeholders::error,
                                             new_session ));

    }

    ~QMainServer()
    {
        //清除管理队列中所有元素
        m_session_manager_list.erase(m_session_manager_list.begin(),
                                     m_session_manager_list.end());
    }



    void handle_accept(const boost::system::error_code &error,
                       qsession_ptr new_pSession)
    {
        /// accept 出错情况下
        if (error)
        {
            // do some thing . log....
            return;
        }


        // confused??
        qDebug() << QString("[QMainServer::handle_accept] handle_accept coming...,new_pSession.use_count():%1 ")
                    .arg(new_pSession.use_count());


        // 发送异步请求准备收数据
        new_pSession->start();


        //reset() 相当于释放当前所控制的对象
        //reset(T* p) 相当于释放当前所控制的对象，然后接管p所指的对象
        //reset(T*, Deleter) 和上面一样
        new_pSession.reset(new QSession(
                               m_io_service_work_pool.get_io_service(),
                               m_io_service_pool.get_io_service()));

        // 继续接收新连接
        m_acceptor_.async_accept(new_pSession->socket(),
                                 boost::bind(&QMainServer::handle_accept,
                                             this,
                                             boost::asio::placeholders::error,
                                             new_pSession ));



    }

    void run_task()
    {
        //old obsolete way
        /*
        io_thread_.reset(new boost::thread(boost::bind(&io_service_pool::run
                                                       , &io_service_pool_)));
        work_thread_.reset(new boost::thread(boost::bind(&io_service_pool::run
                                                         , &io_service_work_pool_)));
        */

        //启动 次线程的 io_service 事件循环
        m_io_service_pool.run();
        m_io_service_work_pool.run();

    }


    //debug test
    void run_test_client()
    {
        //多客户端 同时发起连接
        for(int iCnt=0 ; iCnt<3000 ; iCnt++)
        {
            QString client_session_name = QString("crs_%1").arg(iCnt);
            std::string tmp_std_username = client_session_name.toStdString();

            tcp::endpoint epp(boost::asio::ip::address_v4::from_string("127.0.0.1"), 18080);
            QSession_clientSide::start_new_connect(epp,tmp_std_username,
                                                   m_io_service_work_pool.get_io_service(),
                                                   m_io_service_pool.get_io_service());
        }

    }



    void stop()
    {
        m_io_service_pool.stop();
        m_io_service_work_pool.stop();
    }


    std::string get_all_login_session_names(size_t &list_size)
    {
        list_size = 0;
        own_read_lock lock_guard_(m_list_read_write_mutex);

        std::string all_login_names;
        std::map<std::string,qsession_ptr>::iterator iter_n = m_session_manager_list.begin();
        while(iter_n != m_session_manager_list.end())
        {
            all_login_names += iter_n->first;
            all_login_names += NEW_LINE_BREAK;
            list_size++;
            iter_n++;
        }

        return all_login_names;
    }


    bool get_specified_session_with_name(const std::string &userid)
    {
        own_read_lock lock_guard_(m_list_read_write_mutex);

        std::map<std::string,qsession_ptr>::iterator iter_find = m_session_manager_list.find(userid);
        if(iter_find != m_session_manager_list.end())
            return true;
        else
            return false;
    }


    void register_into_manager_list(qsession_ptr new_session)
    {
        own_write_lock lock_guard_(m_list_read_write_mutex);

        if(!new_session->get_user_name().empty())
            m_session_manager_list.insert(std::map<std::string,qsession_ptr>::value_type(
                                              new_session->get_user_name(),
                                              new_session));

        qDebug() << QString("[QMainServer::register_into_manager_list()--> m_session_manager_list.size():%1]")
                    .arg(m_session_manager_list.size());
    }


    void remove_session_with_usrid(const std::string &userid)
    {
        own_write_lock lock_guard_(m_list_read_write_mutex);

        std::map<std::string,qsession_ptr>::iterator key_iter= m_session_manager_list.find(userid);
        if(key_iter != m_session_manager_list.end())
            m_session_manager_list.erase(key_iter);

        qDebug() << QString("[QMainServer::remove_session_with_usrid(): --> m_session_manager_list.size():%1]")
                    .arg(m_session_manager_list.size());
    }

private:
    QIo_service_pool m_io_service_pool;
    QIo_service_pool m_io_service_work_pool;
    tcp::acceptor m_acceptor_;


    //建立读写锁(互斥量) 保护 m_session_manager_list 对象
    boost::shared_mutex m_list_read_write_mutex;         //用于保护 m_session_manager_list 的共享互斥量 相当于key
    //建立连接 且成功获取 对方登录名的 session 放入管理队列
    std::map<std::string,qsession_ptr> m_session_manager_list;
};


#endif // QMAINSERVER_H
