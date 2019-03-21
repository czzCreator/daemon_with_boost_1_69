#ifndef QMYGLOBAL_H
#define QMYGLOBAL_H

#include <QCoreApplication>
#include <QDebug>
#include <QProcess>


#include "stdio.h"
#include "string.h"


#define CONSOLE_APP_SERV_VERSION "1.0.0"
#define CONSOLE_APP_SERV_LAST_MODIFIED  "2019/03/03"

//添加 stpool 线程池库需要调用到的头文件
#define CPU_CORE_NUMBER 4
#include "stpool_caps.h"        //线程池配置能力 控制文件
#include "stpool.h"
#include "stpoolc++.h"
#include "stpool_group.h"       //线程池组控制


#include "qlog_message_handler.h"



//跨平台 考虑换行 打印信息
#ifdef _WIN32
//define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        //define something for Windows (64-bit only)
        #define NEW_LINE_BREAK "\r\n"
        #define DOUBLE_LINE_BREAK  "\r\n\r\n"
    #else
        //define something for Windows (32-bit only)
        #define NEW_LINE_BREAK "\r\n"
        #define DOUBLE_LINE_BREAK  "\r\n\r\n"
    #endif

#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR
        // iOS Simulator
    #elif TARGET_OS_IPHONE
        // iOS device
    #elif TARGET_OS_MAC
        // Other kinds of Mac OS
    #else
    #   error "Unknown Apple platform"
    #endif

#elif __ANDROID__
    // android

#elif __linux__
    //linux
    #define NEW_LINE_BREAK "\n"
    #define DOUBLE_LINE_BREAK  "\n\n"

#elif __unix__ // all unices not caught above
// Unix
#elif defined(_POSIX_VERSION)
// POSIX
#else
#   error "Unknown compiler"
#endif



namespace app_exit_code
{
    const int APP_SHUTDOWN_NORMALLY = 0x000001FF;
    const int APP_RESTART_CMD = 0x000002FF;
}






/*
 *
 * 1．  operator 用于类型转换函数：
类型转换函数的特征：

1）  型转换函数定义在源类中；
2）  须由 operator 修饰，函数名称是目标类型名或目标类名；
3）  函数没有参数，没有返回值，但是有return
语句，在return语句中返回目标类型数据或调用目标类的构造函数。

inline operator stpool_t*() 如这种写法
*/
extern stpool::CTaskPool *g_pGlobal_task_pool;

class QMainServer;
extern QMainServer *g_pMain_Control_server;

extern std::string g_qApp_name;

class QMyUtility
{
public:
    QMyUtility();

    static void restart_app();
    static void shutdown_app_normally();
};

#endif // QMYGLOBAL_H
