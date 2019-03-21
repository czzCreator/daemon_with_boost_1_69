#include "qsession.h"

#include "qmainserver.h"

void QSession::close_session()
{
    try
    {
        // 定时器 设定为永不超时/不可用状态
        // 如果有 超时事件在等待 这个方法会触发他们(error_code 标识为 boost::asio::error::operation_aborted)
        m_deadline_counter.expires_at(boost::posix_time::pos_infin);
        m_deadline_counter.cancel();// 取消定时器

        m_socket_.shutdown(tcp::socket::shutdown_both);
        m_socket_.close();

        //将 mainserver 中管理的 session 去除
        if(g_pMain_Control_server != NULL)
        {
            g_pMain_Control_server->remove_session_with_usrid(this->get_user_name());
        }
    }
    catch(std::exception& e)
    {
        std::cout << "[QSession::close_socket()--> Exception! message:]" << e.what() << std::endl;
    }
}


void QSession::start()
{
    // 服务端这边 在顺利 与客户端建立连接后 发送 控制命令usage介绍

    // 关于流
    // https://blog.csdn.net/man_sion/article/details/78110842

    /*
     * 产生的异常error的传递是个问题，因为异步会立刻返回，局部变量是会被销毁的。而boost::asio::placeholders::error,
     * 将会保存异常的状态，这样我们使用异步调用时如 socket::async_write_some的时候不用自己创建
     * boost::system::error_code error(非异步场合,同步时用)了，
     * 直接使用boost::asio::placeholders::error作为参数即可，同理，我们async_write_some需要返回 成功写入数据的大小，
     * 令人开心的是boost::asio::placeholders::bytes_transferred直接作为参数就可以保存数据大小。实例如：
     *
                    boost::asio::async_write(socket_,
                     boost::asio::buffer(message_),
                     boost::bind(&tcp_connection::handle_write, shared_from_this(),
                     boost::asio::placeholders::error,
                     boost::asio::placeholders::bytes_transferred));
     *
     * 参考手册上说的很明确，下面两个类就是为异步调用使用bind的时候设计的。
                     boost::asio::placeholders::error
                     boost::asio::placeholders::bytes_transferred
    */

    /*
     * 如何保证异步函数绑定的 处理函数如下面 boost::bind() 中的
     * handle_read 中参数的生存期呢？
     * (因为你不知道 handle_read 何时被调用，必须保证它
     * 在被调用时, 传递给它的参数还存在着，这就是 boost::asio::placeholders::error等的意义 )
     *
     * 在测试时发现 如下面函数
     * void conn_async_handle(const boost::system::error_code &err_code)
        {
            if(err_code)
                return;

            for(size_t iCnt=0 ; iCnt<102400 ; iCnt++)
            {
                g_data_.at(iCnt) = 65 + iCnt % 26;
            }

            size_t tmp_will_lost_cache = 80808080;
            tmp_client.async_write_some(boost::asio::buffer(g_data_),
                            boost::bind(handle_async_write,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred,
                                        tmp_will_lost_cache));
        }
     * handle_async_write 被异步调用时 变量 ‘tmp_will_lost_cache’ 能传给 handle_async_write,
     * handle_async_write() 内部能读到 tmp_will_lost_cache = 80808080
     *
    */
    m_socket_.async_read_some(boost::asio::buffer(m_data_recv),
        make_custom_alloc_handler(m_allocator_,
        boost::bind(&QSession::handle_read,
        shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred)));    //最后一个参数表示本次成功读到了多少


    // 开启 异步 dealine 定时器 设定超时时间
    // 设定定时器超时时间
    /// Set the timer's expiry time relative to now.
    /**
     * This function sets the expiry time. Any pending asynchronous wait
     * operations will be cancelled. The handler for each cancelled operation will
     * be invoked with the boost::asio::error::operation_aborted error code.
     *
     * 重新设置定时器的结束时间，这个时间相对当前时间的。
     * 注意任何在等待此定时器的异步操作都被取消，所有被取消异步操作的handler都会被调用，
     * 参数error_code被设置为boost::asio::error::operation_aborted。
     * 返回被取消异步操作的个数，同时抛出boost::system::system_error Thrown on failure。
     */
    m_deadline_counter.expires_from_now(boost::posix_time::seconds(ASYC_TIMEOUT_DEADLINE));
    // 绑定超时 事件, 并指定回调函数
    m_deadline_counter.async_wait(boost::bind(&QSession::on_deadline_checker,
                                              shared_from_this(),
                                              boost::asio::placeholders::error));
}


// 从队列中取数据发送
void QSession::do_write_message_active()
{
    // 获取末尾的元素
    std::string get_element_ref_atBack;
    if(pop_back_from_send_queue(get_element_ref_atBack))
    {
        size_t msg_size = get_element_ref_atBack.size();
        size_t original_offset = 0;


        //不允许命令超过 1024byte 缓冲区就这么大目前!
        //回传的数据 过大 会越界崩溃
        std::copy(get_element_ref_atBack.begin(),get_element_ref_atBack.begin()+msg_size,m_data_send.begin());

        m_socket_.async_write_some(boost::asio::buffer((void *)&m_data_send[0],msg_size),
                make_custom_alloc_handler(m_allocator_,
                                          boost::bind(&QSession::handle_write,
                                                      shared_from_this(),
                                                      boost::asio::placeholders::error,
                                                      boost::asio::placeholders::bytes_transferred,
                                                      msg_size,
                                                      original_offset)));
    }
}


void QSession::handle_read(const boost::system::error_code& error,
    size_t bytes_transferred)
{
    if (!error)
    {
        //智能指针的初始化方式 一般都是这样  buf出作用域 指向的new出的东西就会析构
        boost::shared_ptr<std::vector<char> > buf(new std::vector<char>);

        buf->resize(bytes_transferred);

        //STL 标准拷贝函数 一般用于对标准容器的拷贝
        // copy只负责复制，不负责申请空间，所以复制前必须有足够的空间
        /*
         * 如果要把一个序列（sequence）拷贝到一个容器（container）中去，通常用std::copy算法，代码如下：
         * [cpp] view plain copy
         * std::copy(start, end, std::back_inserter(container));
         *
         *
         * 但是，如果container的大小小于输入序列的长度N的话，这段代码会导致崩溃（crash）
         * 通常，考虑到在一个已有的元素上直接copy覆盖更高效。刻意这样做
         * std::copy(start, end, container.begin());
        */
        //将 m_socket_.async_read_some 读到的存储在 boost::asio::buffer(m_data_) 拷贝到 buf 中
        std::copy(m_data_recv.begin(), m_data_recv.begin() + bytes_transferred, buf->begin());


        //用于向外 发布io事件
        /// 关于post 的用法 可看 https://www.cnblogs.com/itdef/p/5290693.html
        /// 主要向 这个io_service 里塞入某个event handle 排队等处理
        /// bind 函数当第一个参数是成员函数时,第二个参数则为对应类对象的指针;
        /// 如果第一个参数只是普通函数,则第二个参数及以后都是该函数参数
        ///
        /// 这里不知道对方发送多少字节才算是发送完毕 才能开始解析
        m_io_work_service.post(boost::bind(&QSession::on_receive
            , shared_from_this(), buf, bytes_transferred));


        /// 程序到这里 你会发现 整个程序设计 就是围绕 io_service 为核心 线程上运行io_service.run() 事件循环
        /// 其他调用这个io_service 的 线程可以利用 此 io_service 往 运行io_service.run() 的线程post 回掉函数让其执行

        //继续异步读一些数据
        m_socket_.async_read_some(boost::asio::buffer(m_data_recv),
            make_custom_alloc_handler(m_allocator_,
            boost::bind(&QSession::handle_read,
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)));

        // 重新设置 recv 超时时间， 避免超时
        // 此处会触发 on_deadline_checker 但不是真正的超时导致 回调函数被执行
        int numbers_cancelled = m_deadline_counter.expires_from_now(boost::posix_time::seconds(ASYC_TIMEOUT_DEADLINE));
//        if(numbers_cancelled)
//            qDebug() << QString("[QSession::handle_read()] reset timer -->'m_deadline_counter' ,\n"
//                                "The number of asynchronous operations that were cancelled-->%1 ")
//                        .arg(numbers_cancelled);

        // 重新启动定时器( 重新调用expires_from_now 会将之前的 handle回调立刻返回 error置为 boost::asio::error::operation_aborted  )
        m_deadline_counter.async_wait(boost::bind(&QSession::on_deadline_checker,
                                                  shared_from_this(),
                                                  boost::asio::placeholders::error));

        //重置 timer_out_counter(有读到东西 说明连接还是正常的没有超时)
        m_timeout_counts = 0;
    }
    else
    {
        if(error == boost::asio::error::operation_aborted)
            qDebug() << QString("[QSession::handle_read()] failed: An async read operation was canceled "
                                " due to the closure of the socket");
        else if(error == boost::asio::error::connection_reset)
            //远端的TCP协议层发送RESET终止链接，暴力关闭套接字。常常发生于远端进程强制关闭时，操作系统释放套接字资源
            qDebug() << QString("[QSession::handle_read()] failed: the opposite socket not close normally,it 'RESET");
        else if(error == boost::asio::error::eof)
        {
            //如 返回的是 end of file 对方可能是正常调用了 socket close
            qDebug() << QString("[QSession::handle_read()] failed, the opposite socket close normally."
                                "error code:%1 ."
                                "detail message:%2 ")
                        .arg(error.value())
                        .arg(error.message().c_str());

            close_session();
        }
        else
            //读失败 不一定能 检测到对方socket 已关闭(即对方显式调用了 socket close),
            //而写 有可能(如果对方已经 关闭了 socket 再往对端写 会触发 connection_aborted(10053))
            //tcp协议就是这样的，某些情况下强行断开，对端并不会马上发现对端已关闭(本机除外)，你主动发数据的时候才会发现连接已断
            //当然最好还是自己有个超时机制
            qDebug() << QString("[QSession::handle_read()] failed,error code:%1 ."
                                "detail message:%2 ")
                        .arg(error.value())
                        .arg(error.message().c_str());
    }
}



void QSession::handle_write(const boost::system::error_code& error,
                            size_t bytes_actually_trans,
                            size_t expected_size,
                            size_t offset)
{

    if (error)
    {
        //do something log
        //细化 连接错误
        if(error == boost::asio::error::connection_aborted)
            qDebug() << QString("[QSession::handle_write] An established connection was aborted by the software in your host computer,\n"
                                "possibly due to a data transmission time-out or protocol error,\n"
                                "error code:%1. "
                                "error message: %2 ")
                        .arg(error.value())
                        .arg(error.message().c_str());

        if(error == boost::asio::error::connection_refused)
            qDebug() << QString("[QSession::handle_write] connection refused by the opposite side,\n"
                                "error code:%1. \n"
                                "error message: %2 ")
                        .arg(error.value())
                        .arg(error.message().c_str());

        //连接异常
        if(error == boost::asio::error::connection_aborted ||
           error == boost::asio::error::connection_refused)
            close_session();


        return;
    }
    //以下正常写入
    else
    {
        if(!m_socket_.is_open())
            return;

        size_t resend_size = expected_size - bytes_actually_trans;
        if(resend_size > 0)
        {
            size_t new_offset = offset + bytes_actually_trans;
            m_socket_.async_write_some(boost::asio::buffer((void *)&m_data_send[new_offset],resend_size),
                                       make_custom_alloc_handler(m_allocator_,
                                                                 boost::bind(&QSession::handle_write,
                                                                             shared_from_this(),
                                                                             boost::asio::placeholders::error,
                                                                             boost::asio::placeholders::bytes_transferred,
                                                                             resend_size,
                                                                             new_offset)));

            qDebug() << QString("[QSession::handle_write()] -->\n "
                                "resend_size:%1, new_offset:%2, bytes haven't been sended:%3 \n ")
                        .arg(resend_size).arg(new_offset).arg((char *)&m_data_send[new_offset]);
        }
        //说明本次记录发完了?
        //处理发送队列中还存在的数据
        else if(resend_size == 0)
        {
            // 处理发送队列中还存在的数据
//            qDebug() << QString("[QSession::handle_write()] --> \n "
//                                "we have send all data in buffer,check m_send_command_queue.size():%1")
//                        .arg(get_sendqueue_size());


            // 从队列中取数据发送(继续发送 如果队列中还有数据的话)
            do_write_message_active();
        }
    }
}



//对收到的数据内容解析处理
void QSession::on_receive(boost::shared_ptr<std::vector<char> > buffers
    , size_t bytes_transferred)
{
    char* data_stream = &(*buffers->begin());

    // in here finish the work.
    memcpy(m_pComplete_cmd + m_offset_complete,data_stream,bytes_transferred);
    std::cout << DOUBLE_LINE_BREAK <<" Server ("
              << m_username << ") receive :"
              << bytes_transferred <<" bytes."
              << "message :" << &(m_pComplete_cmd[m_offset_complete]) << std::endl;


    /// 通信流程 协议相关
    std::string parse_out_result;
    size_t r_result = m_package_dealer.parse_cmd_from_client(m_pComplete_cmd,strlen(m_pComplete_cmd),parse_out_result);
    if(cmd_parser::SUCCESS != r_result)
    {
        if(cmd_parser::ERROR_CMD_NOT_COMPLETE == r_result)
        {
            m_offset_complete += bytes_transferred;

            return;
        }
        else
            qDebug() << DOUBLE_LINE_BREAK
                     <<"Server (parse_cmd_from_client) failed,check  "
                    << NEW_LINE_BREAK
                    << "parse out detail: "
                    << parse_out_result.c_str()
                    << NEW_LINE_BREAK;
    }
    else
    {
        // 本条命令解析完毕 重新初始化
        // 为下次解析做准备
        m_offset_complete = 0;
        memset(m_pComplete_cmd,0,2048);
    }


    push_front_to_send_queue(parse_out_result);
    /// 回应客户端相关数据
    do_write_message_active();
}



//用于 异步递归等待
/// 关于 m_deadline_counter:  expires_at()  expires_from_now() 的说明
/// https://blog.csdn.net/bigwriteshark/article/details/82885550
void QSession::on_deadline_checker(const boost::system::error_code& error)
{
    if(error == boost::asio::error::operation_aborted)
    {
        //等待被取消 ,可能是有人重新设置了超时
//        qDebug() << QString("[QSession::on_deadline_checker()] : pending asynchronous wait "
//                            "operations has be cancelled");

        return;
    }



    //读失败 不一定能 检测到对方socket 已关闭(即对方显式调用了 socket close),
    //如 返回的是 end of file 你就不知道对方上次发完数据后到底是再 没发数据 还是已经关了
    //而写 有可能
    //每间隔 ASYC_TIMEOUT_DEADLINE 通过 write 检查下socket 是否有效
    //qDebug() << QString("[QSession::on_deadline_checker()] timeout counter:%1").arg(m_timeout_counts);
    if(m_timeout_counts >= TIMEOUT_LIMIT_COUNTS)
    {
        //关闭 对应session  以及封装的 socket
        close_session();

        return;
    }


    m_timeout_counts++;
    // 以下形成 递归式的异步调用
    // 重设超时
    m_deadline_counter.expires_from_now(boost::posix_time::seconds(ASYC_TIMEOUT_DEADLINE));
    // 重新启动定时器
    m_deadline_counter.async_wait(boost::bind(&QSession::on_deadline_checker,
                                              shared_from_this(),
                                              boost::asio::placeholders::error));
}


