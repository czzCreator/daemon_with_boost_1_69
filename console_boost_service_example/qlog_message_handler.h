#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <QFile>
#include <QMutex>
#include <QtGlobal>

//注意:外部在调用这个类的时候 可以配合信号把 信息 messmage 通知出去

#define DEFAULT_LOG_RESERVE_DAY 20          //默认存储20天的日志
#define DEFAULT_PROC_LOG_DIR "./log"


//似乎不关心具体的日志名称 用宏定义不同日志类别
//#define DEBUG_LOGNAME_FULL_PATH     DEFAULT_TRANS_ELECTRIC_I2_SERVICE_LOG_DIR##"/trans_I2_debug.log"
#define APP_LOG_PREFIX  "stoolTest"

#define DEBUG_LOG_DIR_MACRO     "/debug"
#define WARNING_LOG_DIR_MACRO     "/warning"
#define CRITICAL_LOG_DIR_MACRO     "/critical"
#define INFO_LOG_DIR_MACRO     "/info"

#define DEBUG_LOG_NAME  APP_LOG_PREFIX"debug.log"
#define WARNING_LOG_NAME  APP_LOG_PREFIX"warning.log"
#define CRITICAL_LOG_NAME  APP_LOG_PREFIX"critical.log"
#define INFO_LOG_NAME  APP_LOG_PREFIX"info.log"



namespace message_handler {

class  QLog_MessageHandler: public QFile
{
private:
    QLog_MessageHandler();
    ~QLog_MessageHandler();

public:
    static void setLogReserve_day(int reserve_days = DEFAULT_LOG_RESERVE_DAY);
    static void setLogDir(QString fileDir = DEFAULT_PROC_LOG_DIR);

    /*
     * Installs a Qt message handler which has been defined previously.
     * This method is deprecated, use qInstallMessageHandler instead.
     * 不同版本 要求实现的回调函数 参数不同，高版本比较好
    */
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    static void FormatMessage_myown(QtMsgType type, const QMessageLogContext &context, const QString &msg);
#else
    static void FormatMessage_myown(QtMsgType type,const char* msg);
#endif

private:
    static QString m_logDir;
    static int m_nDaysOfLog; //日志保留天数
};

}


#endif // MESSAGEHANDLER_H
