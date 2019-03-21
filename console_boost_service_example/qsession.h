#ifndef QSESSION_H
#define QSESSION_H

#include <QDebug>



///什么时候该使用enable_shared_from_this模板类
///
/// https://blog.csdn.net/chdhust/article/details/46854377
/// 简要说就是可以在某个类的对象内部使用 该对象智能指针 shared_ptr
/// 对于智能指针 值得注意-->对一个指针进行智能指针包装后,在使用shared_ptr时一定不能通过同一个指针对象创建一个以上的shared_ptr对象
///
#include <boost/enable_shared_from_this.hpp>

#include <qhandle_allocator.h>

// https://my.oschina.net/cloudgame/blog/387150
// http://blog.jqian.net/post/boost-asio.html
#include <boost/asio.hpp>

#include <boost/bind.hpp>
#include <boost/array.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>


#include "qcmd_pairs_dealer.h"


#define ASYC_TIMEOUT_DEADLINE  10   //单位s
#define TIMEOUT_LIMIT_COUNTS   30


/// 关于同步 异步 阻塞 非阻塞 Reactor(反应器--针对同步I/O)  Proactor(前摄器--针对异步I/O) 十分值得看
/// https://my.oschina.net/JJREN/blog/51966


using boost::asio::ip::tcp;

/// 这个类专门用于接收 新连接 和 某个客户端通信 所以叫做 session
class QSession: public boost::enable_shared_from_this<QSession>
{
    typedef boost::shared_lock<boost::shared_mutex> own_read_lock;
    typedef boost::unique_lock<boost::shared_mutex> own_write_lock;
public:
    /// 一个 io_service.run() 事件循环专门用于处理socket通信事件
    /// 一个 io_service.run() 事件循环专门用于处理 收到的数据解析 数据流解析?
    QSession(boost::asio::io_service& work_service
             , boost::asio::io_service& io_service)
        : m_socket_(io_service)
        , m_io_work_service(work_service)
        , m_deadline_counter(work_service)
        , m_package_dealer(this)
    {
        m_send_command_queue.clear();
        m_username.clear();
        m_timeout_counts = 0;

        m_offset_complete = 0;
        m_pComplete_cmd = new char[2048];
        memset(m_pComplete_cmd,0,2048);
    }


    ~QSession()
    {
        qDebug() << QString("[~QSession()] <userid:%1> be called, socket wrapped by qsession will destroy...")
                    .arg(get_user_name().c_str());

        if(m_pComplete_cmd != NULL)
        {
            delete[] m_pComplete_cmd;
            m_pComplete_cmd = NULL;

            m_offset_complete = -1;
        }
    }


    tcp::socket& socket()
    {
        return m_socket_;
    }

    /// https://blog.csdn.net/csfreebird/article/details/22087817
    /// 一般是不需要手动关闭的
    /// https://blog.csdn.net/ycf8788/article/details/54604336
    /*
     * socket的释放建议:  1、如果已经建立链接，则最好要优先调用shutdown接口，然后调用cancel和close。
     *                  2、如果正在连接或者未连接，调用cancel和close即可。
     *
     * io_service的关闭建议：1、优先关闭其他相关的资源
     *                     2、调用stop接口
     *                     3、需要等待工作者线程退出，再释放io_service对象。（目前没有找到合适的方式知道工作者线程何时退出完毕，有了解的欢迎分享）
    */
    void close_session();


    void set_user_name(const std::string &user_name){ m_username = user_name; }
    std::string get_user_name(){ return m_username; }

    boost::asio::io_service& get_session_work_io_service() { return m_io_work_service; }

    void start();

private:
    // 从队列中取数据发送
    void do_write_message_active();


    void handle_read(const boost::system::error_code& error,
        size_t bytes_transferred);


    //类似 handle_read 的实现 如果是异步写 最终也是用 boost::asio::placeholders::error 替代一般 error
    void handle_write(const boost::system::error_code& error,
                      size_t bytes_actually_trans,
                      size_t expected_size,
                      size_t offset);

    //对收到的数据内容解析处理
    void on_receive(boost::shared_ptr<std::vector<char> > buffers
        , size_t bytes_transferred);

    // 超时定时器 事件处理
    void on_deadline_checker(const boost::system::error_code &error);



    size_t get_sendqueue_size()
    {
        own_read_lock read_only_lock(m_send_queue_mutex);
        return m_send_command_queue.size();
    }
    // 操作发送数据队列
    void push_front_to_send_queue(const std::string &data_)
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
        {
            return false;
        }
    }

private:
    // the  userid   map  to  this  session
    std::string m_username;

    // The io_service used to finish the work.
    boost::asio::io_service& m_io_work_service;

    // The socket used to communicate with the client.
    tcp::socket m_socket_;

    // Buffer used to store data received from the client.
    // https://blog.csdn.net/huang_xw/article/details/8248361
    boost::array<char, 1024> m_data_recv;                         // == char[1024]

    size_t m_offset_complete;
    char * m_pComplete_cmd;

    // Buffer used to store data send to the client.
    // https://blog.csdn.net/huang_xw/article/details/8248361
    boost::array<char, 1024> m_data_send;                         // == char[1024]


    // send queue mutex
    boost::shared_mutex m_send_queue_mutex;
    // send response list queue
    std::deque<std::string> m_send_command_queue;


    // The allocator to use for handler-based custom memory allocation.
    QHandle_allocator m_allocator_;

    // 服务端 控制通信 协议解析对象
    QCmd_pairs_dealer m_package_dealer;

    // 连接 /recv 超时 定时器
    boost::asio::deadline_timer m_deadline_counter;
    size_t  m_timeout_counts;
};

typedef boost::shared_ptr<QSession> qsession_ptr;


#endif // QSESSION_H
