#include "qmyglobal.h"

#include "qmainserver.h"


//debug test
#include "qhandle_allocator.h"




/*  test
boost::asio::io_service io_tmp_test_ioSrv;
boost::asio::ip::tcp::endpoint epp(boost::asio::ip::address_v4::from_string("127.0.0.1"),MAIN_SERVER_PORT);
tcp::socket tmp_client(io_tmp_test_ioSrv);
boost::array<char, 102400> g_data_;

void handle_async_write(const boost::system::error_code& error,
                        size_t bytes_transferred,
                        size_t expected_bytes_total)
{
    if (error)
    {
        // do something... log...
        qDebug() << QString("[handle_async_write] something error when async write,error:%1 ")
                    .arg(error.message().c_str());

        return;
    }

    qDebug() << QString(" boost client test: success async_write-->%1 bytes, test expected_bytes_total-->%2")
                .arg(bytes_transferred).arg(expected_bytes_total) ;
}



void conn_async_handle(const boost::system::error_code &err_code)
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
*/




void uninit_app();



int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    g_qApp_name = qApp->applicationName().toStdString();


//    //定义日志文件位置(配置文件中读出 参数初始化日志)  日志的打印会拖慢 服务器响应客户端的速度
//    message_handler::QLog_MessageHandler::setLogDir(DEFAULT_PROC_LOG_DIR);
//    message_handler::QLog_MessageHandler::setLogReserve_day(DEFAULT_LOG_RESERVE_DAY);

//#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
//    qInstallMessageHandler(message_handler::QLog_MessageHandler::FormatMessage_myown);
//#else
//    qInstallMsgHandler(message_handler::QLog_MessageHandler::FormatMessage_myown);
//#endif


    /////////////////////////////////////////////////////////////////////
    //debug test  show  .usage
    QHandle_allocator hehe_test;
    void *test_allocate_size = hehe_test.allocate_own(2048);
    int rResult = hehe_test.get_lasttime_used_size();

    hehe_test.deallocate(test_allocate_size);
    rResult = hehe_test.get_lasttime_used_size();

    test_allocate_size = hehe_test.allocate_own(512);
    rResult = hehe_test.get_lasttime_used_size();
    /////////////////////////////////////////////////////////////////////



    // 线程池优先级的用法可以参照 stpool c 的例子
    // Set the necessary capabilities masks for threadpools
    // eCAP_F_OVERLOAD 代表任务过多时 线程池的行为 超过一个阈值是抛弃任务，强行插入任务，还是挤掉旧的任务(实际测试发现加入这个不能create成功)
    long eCaps = eCAP_F_DYNAMIC| eCAP_F_PRIORITY | eCAP_F_THROTTLE | eCAP_F_SUSPEND |
                 eCAP_F_WAIT_ANY | eCAP_F_WAIT_ALL | eCAP_F_DISABLEQ | eCAP_F_REMOVE_BYPOOL |
                 eCAP_F_ROUTINE |eCAP_F_CUSTOM_TASK | eCAP_F_ROUTINE | eCAP_F_TASK_WAIT |
                 eCAP_F_GROUP;

    //创建全局使用的线程池
    //需要用户手动调用 stpool_resume或者 封装stpool_resume 的函数 to mark pool active(因为创建了SUSPEND = true)
    //只有一个优先级队列
    //create 函数会在制造工厂中根据eCaps匹配最佳的线程池模板 创建一个 否则 g_pGlobal_task_pool->m_pPool == NULL
    g_pGlobal_task_pool = stpool::CTaskPool::create("mypool", eCaps, CPU_CORE_NUMBER*4, CPU_CORE_NUMBER*2, 1, true);


    // 2 代表使用 2个工作线程(里面两个池子,每个池子2线程) 2个socket处理线程
    QMainServer main_server(MAIN_SERVER_PORT,2);
    g_pMain_Control_server = &main_server;
    main_server.run_task();



    //前提是线程池里有任务
    /**
         * Wake up the pool to run the tasks
         */
    g_pGlobal_task_pool->resume();
    qInfo() << QString("[In main(): wakp up threadpool 'mypool' to do work ...\n");


    std::string tmp_pool_status_str = g_pGlobal_task_pool->statString();
    qDebug() << QString("[In main]: threadpool status->\n %1").arg(tmp_pool_status_str.c_str());



    //debug test
    //测试时发现 在同时在一个进程里 又启动客户端 又启动服务端 都在同一个 main_server的池子中 会影响服务端性能
    //并发数 在 200+左右 后续连接会被拒绝...
    //main_server.run_test_client();



    //'主进程' 事件循环 开启 (返回时就是退出)
    int r_mainEveLoopResult = a.exec();


    //准备析构 线程池 停止主服务 析构主服务
    uninit_app();


    //结束前根据 主事件循环退出码决定要做的动作
    switch(r_mainEveLoopResult)
    {
    case app_exit_code::APP_RESTART_CMD:
    {
        qInfo() << QString("[In main()]: the app now going to restart itself!!");

        //要启动当前程序的另一个进程，有要使二者没有"父子"关系
        bool restart_result = QProcess::startDetached(qApp->applicationFilePath(), QStringList());
        if(!restart_result)
            qDebug() << QString("[In main()]: QProcess restart app failed! check!!");

        break;
    }

    case app_exit_code::APP_SHUTDOWN_NORMALLY:
    {
        qInfo() << QString("[In main()]: the app now going to exit normally....");
        break;
    }

    default:
        break;
    }


    return r_mainEveLoopResult;
}





void uninit_app()
{
    //禁止用于再添加新的任务到线程池
    /**
         * Turn the throttle on
         * 如果把下面这句注释了   例子中 task_run()的 stpool_task_queue() 会再次将任务加入线程池 形成递归循环不停执行任务
         * 下面这句主要作用就是禁止用户自己将任务重新分配到线程池执行
         */
    g_pGlobal_task_pool->enableThrottle();
    qInfo() << QString("[In main(): Disable user to deliver new task to threadpool from now on ...\n");

    //清掉线程池里排队的 未完成任务
    /**
         * Remove all pendings task
         */
    g_pGlobal_task_pool->removeAll();
    qInfo() << QString("[In main():threadpool remove all pendings task ...\n");


    //停止服务 禁止继续接收新连接提供服务
    //通知所有线程中 跑 io_service 任务(事件)循环的线程退出
    if(g_pMain_Control_server)
        g_pMain_Control_server->stop();
    qInfo() << QString("[In main():main_server ready to stop all io_service eventloop,stop accept new socket...\n");



    //等待任务全部结束
    /**
         * Wait for all tasks' completions
         */
    g_pGlobal_task_pool->waitAll();
    qInfo() << QString("[In main(): threadpool Wait for all tasks' completions  ...\n");


    /**
         * Release the pool
         */
    g_pGlobal_task_pool->release();
    qInfo() << QString("[In main(): Release the threadpool  ...\n");
}

