#include "qmyglobal.h"

#include "qmainserver.h"

stpool::CTaskPool *g_pGlobal_task_pool = NULL;

QMainServer *g_pMain_Control_server = NULL;


std::string g_qApp_name="";



QMyUtility::QMyUtility()
{
}



void QMyUtility::restart_app()
{
    qApp->exit(app_exit_code::APP_RESTART_CMD);
}


void QMyUtility::shutdown_app_normally()
{
    qApp->exit(app_exit_code::APP_SHUTDOWN_NORMALLY);
}
