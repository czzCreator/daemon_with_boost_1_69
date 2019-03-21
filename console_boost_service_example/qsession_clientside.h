#ifndef QSESSION_CLIENTSIDE_H
#define QSESSION_CLIENTSIDE_H

#ifdef WIN32
#include <stdio.h>
#endif


#include <QDebug>


#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>
#include <boost/thread.hpp>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>


#include "qhandle_allocator.h"




#include "qcmd_pairs_dealer.h"




using namespace boost::asio::ip;

/**
 *  simple connection to server:
    - logs in just with username (no password)
    - all connections are initiated by the client: client asks, server answers
    - server disconnects any client that hasn't pinged for XXX seconds (timeout )

    Possible response:
    - login success or not
    - gets a list of all connected clients
    - ping: the server answers either with "ping ok from srv" or "ping client_list_changed"
*/


class QSession_clientSide: public boost::enable_shared_from_this<QSession_clientSide>
{
    typedef QSession_clientSide self_type;
    typedef boost::shared_ptr<QSession_clientSide> client_ptr;

    typedef boost::shared_lock<boost::shared_mutex> own_read_lock;
    typedef boost::unique_lock<boost::shared_mutex> own_write_lock;
public:

    /// 一个 io_service.run() 事件循环专门用于处理socket通信事件
    /// 一个 io_service.run() 事件循环专门用于处理 收到的数据解析 数据流解析?
    QSession_clientSide(boost::asio::io_service& work_service
             , boost::asio::io_service& io_service)
        : m_socket_(io_service),
          m_io_work_service(work_service),
          m_package_dealer_cli(this)
    {
        // start a new session
        m_is_started = true;
        m_send_command_queue.clear();
    }

    ~QSession_clientSide()
    {
        qDebug() << QString("[~QSession_clientSide()] <userid:%1> be called, client_socket wrapped by qsession will be destroy...")
                    .arg(get_user_name().c_str());
    }



    //建立连接 返回相关通信用的管理 session
    static client_ptr start_new_connect(tcp::endpoint ep, const std::string & username,
                                    boost::asio::io_service& work_service,
                                    boost::asio::io_service& io_service)
    {
        client_ptr new_client(new QSession_clientSide(work_service,io_service));
        new_client->set_user_name(username);

        new_client->start_conn_pri(ep);
        return new_client;
    }


    tcp::socket& socket()
    {
        return m_socket_;
    }


    void set_user_name(std::string user_id)
    {
        m_username = user_id;
    }

    std::string get_user_name()
    {
        return m_username;
    }

    void active_stop()
    {
        try
        {
            if(!m_is_started)
                return;

            //主动关闭 socket
            qDebug() << QString("[QSession_clientSide::active_stop()] clientside active stop,close the socket!");

            if(m_socket_.is_open())
                m_socket_.shutdown(tcp::socket::shutdown_both);
            m_socket_.close();
            m_is_started = false;
        }
        catch(std::exception& e)
        {
            std::cout << "[QSession_clientSide::active_stop()]--> Exception! message--" << e.what() << std::endl;
        }

    }

    bool is_started(){ return m_is_started; }

    void do_write_message_active();

private:
    void start_conn_pri(tcp::endpoint ep)
    {

        /// test    let large amount clients not connect to server so fast
        boost::this_thread::sleep(boost::posix_time::milliseconds(1));

        m_socket_.async_connect(ep,boost::bind(&QSession_clientSide::on_handle_connect_event,
                                               shared_from_this(),
                                               boost::asio::placeholders::error));
    }


    void on_handle_connect_event(const boost::system::error_code err_code);
    void on_handle_read_event(const boost::system::error_code& error,
        size_t bytes_transferred);
    // 这里面还得继续 异步写一些 数据(如果缓冲区中数据没有全部写完的话 得计算偏差)
    void on_handle_write_event(const boost::system::error_code& error,
                               size_t bytes_actually_trans,
                               size_t expected_size,
                               size_t offset);



    //对收到的数据内容解析处理
    void on_receive(boost::shared_ptr<std::vector<char> > buffers
        , size_t bytes_transferred);



    size_t get_sendqueue_size()
    {
        own_read_lock read_only_lock(m_send_queue_mutex);
        return m_send_command_queue.size();
    }

    // 操作发送数据队列
    void push_front_to_send_queue(const std::string data_)
    {
        own_write_lock write_only_lock(m_send_queue_mutex);
        m_send_command_queue.push_front(data_);
    }

    bool pop_back_from_send_queue(std::string &get_data)
    {
        own_write_lock write_only_lock(m_send_queue_mutex);
        if(m_send_command_queue.size() > 0)
        {
            // 获取末尾的元素
            get_data = m_send_command_queue.back();
            // 末尾元素 出队列(释放)
            m_send_command_queue.pop_back();
            return true;
        }
        else
            return false;

    }


private:
    // the  userid   map  to  this  session
    std::string m_username;

    // The io_service used to finish the work.
    boost::asio::io_service& m_io_work_service;

    // The socket used to communicate with the server.
    tcp::socket m_socket_;

    // Buffer used to store data received from the server.
    // https://blog.csdn.net/huang_xw/article/details/8248361
    boost::array<char, 1024> m_data_recv;                         // == char[1024]

    // Buffer used to store data send to the server.
    // https://blog.csdn.net/huang_xw/article/details/8248361
    // 默认一次性要发的数据不能超过缓冲区大小
    boost::array<char, 1024> m_data_send;                         // == char[1024]


    // send queue mutex
    boost::shared_mutex m_send_queue_mutex;
    // send cmd list queue
    std::list<std::string> m_send_command_queue;
    
    // The allocator to use for handler-based custom memory allocation.
    QHandle_allocator m_allocator_;

    // show session whether in use
    bool m_is_started;

    // 客户端 控制通信 协议解析对象
    QCmd_pairs_dealer m_package_dealer_cli;
};

#endif // QSESSION_CLIENTSIDE_H
