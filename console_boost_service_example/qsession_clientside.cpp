#include "qsession_clientside.h"







void QSession_clientSide::do_write_message_active()
{
    if(!m_is_started)
        return;


    // 获取末尾的元素
    std::string get_element_ref_atBack;
    if(pop_back_from_send_queue(get_element_ref_atBack))
    {
        size_t msg_size = get_element_ref_atBack.size();
        size_t original_offset = 0;


        //不允许命令(包括命令长度)超过 1024byte 缓冲区就这么大目前!
        std::copy(get_element_ref_atBack.begin(),get_element_ref_atBack.begin()+msg_size,m_data_send.begin());
        m_socket_.async_write_some(boost::asio::buffer((void *)&m_data_send[0],msg_size),
                make_custom_alloc_handler(m_allocator_,
                                          boost::bind(&QSession_clientSide::on_handle_write_event,
                                                      shared_from_this(),
                                                      boost::asio::placeholders::error,
                                                      boost::asio::placeholders::bytes_transferred,
                                                      msg_size,
                                                      original_offset)));
    }
}



void QSession_clientSide::on_handle_connect_event(const boost::system::error_code err_code)
{
    if(err_code)
    {
        // do something log
        qDebug() << QString("[on_handle_connect_event()] Client connect to Server error,"
                            "error_code: %1 . message: %2 ")
                    .arg(err_code.value())
                    .arg(err_code.message().c_str());

        active_stop();
        return;
    }


    //通信连接上的情况下 开始准备递归异步读 (读对端发来的数据)
    m_socket_.async_read_some(boost::asio::buffer(m_data_recv),
                              make_custom_alloc_handler(m_allocator_,
                                                        boost::bind(&QSession_clientSide::on_handle_read_event,
                                                                    shared_from_this(),
                                                                    boost::asio::placeholders::error,
                                                                    boost::asio::placeholders::bytes_transferred)));



    /// 通信流程 协议相关
    //连接上的情况下 主动发起 登录请求(相当于用自己的用户名注册 便于服务端管理所有 已连接的会话)
    std::string login_command = QCmd_pairs_dealer::get_login_with_command() + m_username;
    push_front_to_send_queue(login_command);
    do_write_message_active();
}



void QSession_clientSide::on_handle_read_event(const boost::system::error_code& error,
    size_t bytes_transferred)
{
    if (error)
    {
        //do some logging
        qDebug() << QString("[QSession_clientSide::on_handle_read_event()] err when read,"
                            "err_code:%1. "
                            "err_message:%2 ")
                    .arg(error.value())
                    .arg(error.message().c_str());

        active_stop();
        return;
        // ==eof 说明对方 应该正常关闭了 socket
    }

    if(!m_is_started)
    {
        qDebug() << QString("[QSession_clientSide::on_handle_read_event()] session 'm_is_started' == false ");
        return;
    }



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
    m_io_work_service.post(boost::bind(&QSession_clientSide::on_receive
                                       , shared_from_this(), buf, bytes_transferred));


    /// 程序到这里 你会发现 整个程序设计 就是围绕 io_service 为核心 线程上运行io_service.run() 事件循环
    /// 其他调用这个io_service 的 线程可以利用 此 io_service 往 运行io_service.run() 的线程post 回掉函数让其执行

    //继续异步读一些数据
    //因此，NEVER start your second async_read_some before the first has completed.
    m_socket_.async_read_some(boost::asio::buffer(m_data_recv),
                              make_custom_alloc_handler(m_allocator_,
                                                        boost::bind(&QSession_clientSide::on_handle_read_event,
                                                                    shared_from_this(),
                                                                    boost::asio::placeholders::error,
                                                                    boost::asio::placeholders::bytes_transferred)));

}



// 这里面还得继续 异步写一些 数据(如果缓冲区中数据没有全部写完的话 得计算偏差)
void QSession_clientSide::on_handle_write_event(const boost::system::error_code& error,
                           size_t bytes_actually_trans,
                           size_t expected_size,
                           size_t offset)
{
    if (error)
    {
        //do some logging
        qDebug() << QString("[QSession_clientSide::on_handle_write_event()] err when write,err_message:%1 ")
                    .arg(error.message().c_str());

        active_stop();
        return;
    }

    if(!m_is_started)
    {
        qDebug() << QString("[QSession_clientSide::on_handle_write_event()] session 'm_is_started' == false ");
        return;
    }

    size_t resend_size = expected_size - bytes_actually_trans;
    if(resend_size > 0)
    {
        size_t new_offset = offset + bytes_actually_trans;
        m_socket_.async_write_some(boost::asio::buffer((void *)&m_data_send[new_offset],resend_size),
                                   make_custom_alloc_handler(m_allocator_,
                                                             boost::bind(&QSession_clientSide::on_handle_write_event,
                                                                         shared_from_this(),
                                                                         boost::asio::placeholders::error,
                                                                         boost::asio::placeholders::bytes_transferred,
                                                                         resend_size,
                                                                         new_offset)));

        qDebug() << QString("[QSession_clientSide::on_handle_write_event()] -->\n "
                            "resend_size:%1, new_offset:%2, bytes haven't been sended:%3 \n ")
                    .arg(resend_size).arg(new_offset).arg((char *)&m_data_send[new_offset]);
    }
    //说明本次记录发完了?
    //处理发送队列中还存在的数据
    else if(resend_size == 0)
    {
        // 处理发送队列中还存在的数据
//        qDebug() << QString("[QSession_clientSide::on_handle_write_event()] --> \n "
//                            "we have send all data in buffer,check m_send_command_queue.size():%1")
//                    .arg(get_sendqueue_size());


        do_write_message_active();

        //发送队列中的数据全部发送完了 则尝试读对方的应答命令??
        //不需要 原因是 on_handle_read_event 中会递归异步读

    }
}



//对收到的数据内容解析处理
void QSession_clientSide::on_receive(boost::shared_ptr<std::vector<char> > buffers
                                     , size_t bytes_transferred)
{
    char* data_stream = &(*buffers->begin());

    // in here finish the work.
    std::cout << DOUBLE_LINE_BREAK <<" client receive :"
              << bytes_transferred << " bytes."
              << " message :" << data_stream << std::endl;

    //process the msg
    char conv_whole_str[bytes_transferred + 1] = {0};
    memcpy(conv_whole_str,data_stream,bytes_transferred);


    std::string output_result;
    if(cmd_parser::SUCCESS != m_package_dealer_cli.parse_cmd_from_srv(conv_whole_str,bytes_transferred,output_result))
    {
        qDebug() << DOUBLE_LINE_BREAK <<"Client (parse_cmd_from_srv) failed,check  "
                 << NEW_LINE_BREAK << "parse out detail: "
                 << output_result.c_str() << NEW_LINE_BREAK;
    }


    /// 如果 output_result 解析出来有值就往服务端送
    /// 有的服务端发来的命令不需要应答
    /// 回应服务端相关数据
    if(!output_result.empty())
    {
        push_front_to_send_queue(output_result);
        do_write_message_active();
    }
}










