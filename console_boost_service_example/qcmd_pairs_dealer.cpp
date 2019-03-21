#include "qcmd_pairs_dealer.h"

#include "OptionPrinter.hpp"

#include "qmainserver.h"

//debug test  像下方这样调用
//QCmd_pairs_dealer test_dealer;
//std::string out_put_string;
//if(cmd_parser::SUCCESS == test_dealer.command_line_own_parser("test.exe -v --pingpong-times=8",out_put_string))
//{
//    //一检测到 -h 就不解析其他参数了
//    test_dealer.command_line_own_parser("test.exe -vh --pingpong-times=10",out_put_string);

//    //或者 test_dealer.command_line_own_parser("test.exe -v -s 78.9",out_put_string)
//    test_dealer.command_line_own_parser("test.exe -vs 78.9",out_put_string);

//    // 错误演示
//    test_dealer.command_line_own_parser("test.exe -g 78.9",out_put_string);
//    test_dealer.command_line_own_parser("test.exe -s",out_put_string);
//}





QCmd_pairs_dealer::QCmd_pairs_dealer(void *parent)
    :m_parent(parent)
{
    argv = NULL;
    argc = -1;

    m_pingpong_times_max = 5;               //默认5次
    m_pingpong_counter = 0;

    m_reserve_float_v = -1;
    m_client_json_info.clear();

    char tmp_use[1024] = {0};


    //跨平台 考虑换行 打印信息
    sprintf(tmp_use,"This is cocurrent_app_example (%s) that built by mingw5.3.0 32bits,%s"
                    "Using (libstpool,boost1.69.0, qtlib) .%s"
                    "Writen by czz,last-modified at (%s) .%s",
                    CONSOLE_APP_SERV_VERSION,
                    NEW_LINE_BREAK,
                    NEW_LINE_BREAK,
                    CONSOLE_APP_SERV_LAST_MODIFIED,
                    NEW_LINE_BREAK);


    //拷贝
    m_str_description = tmp_use;

    //步骤一: 构造选项描述器和选项存储器
    //选项描述器,其参数为该描述器的名字
    /* 类 options_description 是存储选项详细信息的类 用于后期验证比对等
     * 用于分析客户端 传来的命令
    */
    m_pPro_desc = new my_option_dealer("All_Options");
    //步骤二: 为选项描述器增加选项
    //其参数依次为: key, value的类型，该选项的描述
    /* add_option() 支持() 操作. 多次调用add_options() 的效果等同于将所有的选项使用() 并列声明*/
    m_pPro_desc->add_options()
                     /*
                      * 无参数的选项格式 : "命令名" , "命令说明"
                      * Add an option with shorthand only
                      * -h  短格式  参照 linux 命令工具
                      * --help  长格式
                      * ("help,h", "Info message about option")
                     */
                     ("help,h", "show help message")
                     ("verbose,v",
                      "print app with verbosity")
                     ("reboot,r",
                      "reboot the app")
                     ("shutdown,d",
                      "shutdown the app")
                     ("get-clients-name,g",
                      "get all login session clients' name")
                     /* 有参数的选项格式 :
                      * "命令名" , "参数说明" , "命令说明" */
                     ("set-float,s" ,
                      boost::program_options::value<float>(&m_reserve_float_v),
                      "set a float parameter")

                     /* 参数对应已有数据(特定地址) , 并且有初始值 : */
                     //The "pingpong-times" 项体现两个新特性. 首先，我们传递变量(&m_pingpong_times_max)地址，
                     //这个变量用来保存获得的参数项的值。然后，指定一个缺省值，用在此参数项用户没有设置值的时候。
                     ("pingpong-times",
                      //boost::program_options::value<int>(&m_pingpong_times_max)->default_value(5),          //可将默认值5赋值给m_pingpong_times_max
                      boost::program_options::value<int>(&m_pingpong_times_max),
                      "send test pacakge times between server and client")
                     /* 支持短选项 ( --include-path 和 -I 等效 ) ,
                      * 多次调用存在依次vector中
                      *
                      */
                     ("json_info,j",
                      boost::program_options::value< std::vector<std::string> >(&m_client_json_info),
                      "json options receive from client(client detail info)")
                     /*
                      * example:
                      * 如果 客户端构造命令中必须 有某个选项 发送给服务端 可如下所示
                      * 第二个参数表示必须 ->required()
                      * Specifies that the value must occur.
                     */
                     //("necessary,n",
                     // boost::program_options::value<std::string>()->required(),
                     //  "necessary!")
                    ;

    set_app_version_info(CONSOLE_APP_SERV_VERSION);
}




size_t QCmd_pairs_dealer::parse_cmd_from_client(const char * raw_str,
                                                size_t raw_str_len,
                                                std::string &output_result)
{
    std::string std_raw_str_cmd(raw_str);

    size_t found = std_raw_str_cmd.find(get_login_with_command());

    //解析传过来的命令是初始登陆命令 (login ...) 还是后续
    //服务器控制命令 (app.exe -v -h --pingpong=XXX 这样的)

    //登陆命令
    if(std::string::npos != found)
    {
        std::string register_name;
        size_t r_result = get_name_frm_login_cmd(std_raw_str_cmd,raw_str_len,register_name);
        if(r_result != cmd_parser::SUCCESS)
        {
            //可能命令还没接收完毕
            if(r_result == cmd_parser::ERROR_CMD_NOT_COMPLETE)
                output_result = "";
            else
            {
                qDebug() << QString("[QCmd_pairs_dealer::parse_cmd_from_client()] parse client login cmd error,check \n"
                                    "raw_str = %1").arg(raw_str);

                output_result = QCmd_pairs_dealer::get_login_failed_command();
            }

        }
        // 解析 登录名 成功的情况下 将session 加入管理
        else
        {
            if(m_parent != NULL && g_pMain_Control_server != NULL)
            {
                QSession * tmp = (QSession *)m_parent;
                tmp->set_user_name(register_name);
                g_pMain_Control_server->register_into_manager_list(tmp->shared_from_this());

                output_result = QCmd_pairs_dealer::get_login_success_command() +"[" + register_name + "]";
            }
            else
                return cmd_parser::ERROR_UNHANDLED_EXCEPTION;
        }

        return r_result;
    }
    //服务器控制命令 利用 boost::program_options 解析
    else
    {
        output_result = "";
        //首先判定 前次是否正常登陆(注册到mainserver里了)
        QSession * tmp = (QSession *)m_parent;
        if(m_parent != NULL &&
           g_pMain_Control_server != NULL)
        {
            if(false == g_pMain_Control_server->get_specified_session_with_name(tmp->get_user_name()))
            {
                output_result = "haven't logged in,parse error";
                return cmd_parser::ERROR_NOT_LOGGED_IN;
            }


            if(std::string::npos != std_raw_str_cmd.find(QCmd_pairs_dealer::get_ping_ok_frm_client_command()))
            {
                // 按照条件 决定是否构造 ping 回复命令
                if(m_pingpong_counter < m_pingpong_times_max)
                {
                    m_pingpong_counter++;
                    output_result = QCmd_pairs_dealer::get_ping_ok_frm_srv_command();
                }
                else
                    m_pingpong_counter = 0;

                return cmd_parser::SUCCESS;
            }


            //登陆过了 有记录
            return command_line_own_parser(raw_str,output_result);
        }
        else
            return cmd_parser::ERROR_UNHANDLED_EXCEPTION;
    }
}


//测试用的
size_t QCmd_pairs_dealer::parse_cmd_from_srv(const char * raw_str,
                                                size_t raw_str_len,
                                                std::string &output_result)
{
    std::string msg_(raw_str,raw_str_len);
    output_result.clear();

    //the server answers client is login success or not with 'user_name'
    // 首次登录 获得结果
    if(0 == msg_.find("login"))
    {
        //不需要输出往服务端送 output_result 则不处理
        //debug test
        output_result = "ctrl_srv --pingpong-times=30";

        m_pingpong_counter = 0;
        qDebug() << QString("[QCmd_pairs_dealer::parse_cmd_from_srv()]: Client Get login result-->%1 ")
                    .arg(msg_.c_str());
        return cmd_parser::SUCCESS;
    }

    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////


    //the server answers all clients(connected) user_name
    if(std::string::npos != msg_.find("clients: "))
    {
        //不需要输出往服务端送 output_result 不处理

        qDebug() << QString("[QCmd_pairs_dealer::parse_cmd_from_srv()]: We get this moment Client_list_names from server-->")
                 << NEW_LINE_BREAK << msg_.c_str();
    }

    //ping: the server answers either with "ping ok from server" or "ping client_list_changed"
    if(std::string::npos != msg_.find(QCmd_pairs_dealer::get_ping_ok_frm_srv_command()) ||
       std::string::npos != msg_.find(QCmd_pairs_dealer::get_ping_client_changed_command()))
    {
        m_pingpong_counter++;
        output_result += QCmd_pairs_dealer::get_ping_ok_frm_client_command() +
                         " usr-->" +
                         ((QSession_clientSide *)m_parent)->get_user_name() + NEW_LINE_BREAK;
        output_result += "<" + std::to_string(m_pingpong_counter) + ">";

        qDebug() << QString("[QCmd_pairs_dealer::parse_cmd_from_srv()]: We get 'ping' response from server-->")
                 << NEW_LINE_BREAK << msg_.c_str() << " <" << std::to_string(m_pingpong_counter).c_str() << "> ";
    }

    return cmd_parser::SUCCESS;
}




size_t QCmd_pairs_dealer::command_line_own_parser(const char * raw_str, std::string &output_result)
{
    try
    {
        output_result.clear();
        //重新初始化
        /* variables_map 类是用来存储具体传入参数数据的类 , 和map很相似. 使用as接口来传出数据 */
        m_cmd_variable_map.clear();

        size_t convt_result = covert_str_2_argvs(raw_str);
        if(cmd_parser::SUCCESS != convt_result)
        {
            output_result += "ERROR_IN_COMMAND_LINE, covert_str_2_argvs() failed,check";
            return convt_result;
        }

        /*******************************************************************************/

        std::string usage_instruc_string;
        // 能到这一步 前面的 已经保证 covert_str_2_argvs() 成功了 argv 有值
        std::string appName = boost::filesystem::basename(argv[0]);

        //暂时不对传入的 app名称做验证
//        if(appName.find(g_qApp_name) == std::string::npos)
//        {
//            output_result += "App_name: [" +appName + "] incorrect, correct app_name is " + g_qApp_name;
//            return cmd_parser::ERROR_IN_COMMAND_LINE;
//        }


        // 先打印出用法 string 以便下面要用(要输出或者直接使用 usage string)
        std::stringstream tmp_stream;
        rad::OptionPrinter::printStandardAppDesc(appName,
                                                 tmp_stream,
                                                 (*m_pPro_desc),
                                                 NULL);                 //暂时没有使用位置参数功能

        // 正常情况输出完毕会退出循环
        // 要使用 getline 而非 << >> 等重载符号 否则 stringstream处理字符串时，会跳过空白制表符
        // http://blog.wangcaiyong.com/2016/10/16/stringstream/
        std::string read_line_usage;

        //跨平台 考虑换行 打印信息
        while(std::getline(tmp_stream,read_line_usage))
            usage_instruc_string += read_line_usage + NEW_LINE_BREAK;

        try
        {

            //步骤三: 先对命令行输入的参数做解析,而后将其存入选项存储器
            //如果输入了未定义的选项，程序会抛出异常，所以对解析代码要用try-catch块包围
            boost::program_options::store( boost::program_options::parse_command_line(argc,argv,(*m_pPro_desc)),
                                           m_cmd_variable_map);

         /*
         * --help option
         * 如果检测到 -h 就直接成功返回了 后面参数不继续解析了
         */
            if(m_cmd_variable_map.count("help"))
            {
                //            qDebug() << "This is just a template app that should be modified"
                //                     << " and added to in order to create a useful command"
                //                     << " line application" << "\n\n";
                //打印到终端 便于查看
                std::cout << m_str_description << std::endl;
                std::cout << usage_instruc_string << std::endl;

                output_result += m_str_description + NEW_LINE_BREAK;
                output_result += usage_instruc_string + NEW_LINE_BREAK;

                return cmd_parser::SUCCESS;
            }

            // throws on error, so do after help in case
            // there are any problems
            boost::program_options::notify(m_cmd_variable_map);
        }
        catch(boost::program_options::required_option& e)
        {
            rad::OptionPrinter::formatRequiredOptionError(e);
            qDebug() << "ERROR: " << e.what() << NEW_LINE_BREAK;
            qDebug() << usage_instruc_string.c_str() << NEW_LINE_BREAK;

            output_result += "ERROR: " + std::string(e.what()) + DOUBLE_LINE_BREAK;
            output_result += usage_instruc_string + NEW_LINE_BREAK;

            return cmd_parser::ERROR_IN_COMMAND_LINE;
        }
        catch(boost::program_options::error& e)
        {
            qDebug() << "ERROR: " << e.what() << DOUBLE_LINE_BREAK;
            qDebug() << usage_instruc_string.c_str() << NEW_LINE_BREAK;

            output_result += "ERROR: " + std::string(e.what()) + DOUBLE_LINE_BREAK;
            output_result += usage_instruc_string + NEW_LINE_BREAK;

            return cmd_parser::ERROR_IN_COMMAND_LINE;
        }

        // can do this without fear because it is required to be present
        // std::cout << "Necessary = "
        //          << vm["necessary"].as<std::string>() << std::endl;


        if ( m_cmd_variable_map.count("verbose") )
        {
            qDebug() << "VERBOSE PRINTING:" << NEW_LINE_BREAK << get_app_version_info().c_str() << DOUBLE_LINE_BREAK;
            output_result += "VERBOSE PRINTING:" + std::string(NEW_LINE_BREAK) + get_app_version_info() + DOUBLE_LINE_BREAK;
        }

        if( m_cmd_variable_map.count("set-float") )
        {
            qDebug() << "Get Client set-float: " << NEW_LINE_BREAK <<  m_cmd_variable_map["set-float"].as<float>() << DOUBLE_LINE_BREAK;

            m_reserve_float_v = m_cmd_variable_map["set-float"].as<float>();

            std::ostringstream _buffer;
            _buffer << m_reserve_float_v;
            output_result += "Get Client set-float: " + std::string(NEW_LINE_BREAK) + _buffer.str() + DOUBLE_LINE_BREAK;
        }

        if( m_cmd_variable_map.count("get-clients-name") )
        {
            qDebug() << "Get Client request-->get all login session clients' name " << NEW_LINE_BREAK;


            if(g_pMain_Control_server)
            {
                size_t list_size=0;
                std::string all_login_names(g_pMain_Control_server->get_all_login_session_names(list_size));

                std::ostringstream _buffer;
                _buffer << list_size;
                output_result += "clients:  (size--" + _buffer.str() + ")" + std::string(NEW_LINE_BREAK);

                //其实这个 暂时不需要 过多的名字组成的字符串 导致串过长 会使 session 发送结束缓冲区越界
                //output_result += all_login_names;
            }
        }

        if( m_cmd_variable_map.count("pingpong-times") )
        {
            qDebug() << "(Server do) set pingpong-times cmd,now pingpong-times: " << NEW_LINE_BREAK
                     << m_cmd_variable_map["pingpong-times"].as<int>() << DOUBLE_LINE_BREAK;

            m_pingpong_times_max = m_cmd_variable_map["pingpong-times"].as<int>();

            std::ostringstream _buffer;
            _buffer << m_pingpong_times_max;
            output_result += "(Server do) set pingpong-times cmd,now pingpong-times: " +
                             std::string(NEW_LINE_BREAK) +
                             _buffer.str() + DOUBLE_LINE_BREAK;

            m_pingpong_counter++;
            output_result += QCmd_pairs_dealer::get_ping_ok_frm_srv_command();

        }





        //最后处理 重启 模块,关闭 模块命令
        //优先处理 重启命令(之后的命令就不处理 无效了); 之后才是关闭 app 命令
        if( m_cmd_variable_map.count("reboot") )
        {
            qDebug() << "Get Client reboot app request... " << NEW_LINE_BREAK;
            output_result += "Get Client reboot app request... ready to reboot..." + std::string(NEW_LINE_BREAK);

            QMyUtility::restart_app();

            return cmd_parser::SUCCESS;
        }


        if( m_cmd_variable_map.count("shutdown") )
        {
            qDebug() << "Get Client shutdown app request... " << NEW_LINE_BREAK;
            output_result += "Get Client shutdown app request... ready to shutdown..." + std::string(NEW_LINE_BREAK);

            QMyUtility::shutdown_app_normally();

            return cmd_parser::SUCCESS;
        }

        //    std::cout << "Required Positional, add: " << add
        //              << " like: " << like << std::endl;

        //    if ( json_info.size() > 0 )
        //    {
        //        std::cout << "The specified json_info words: ";
        //        std::string separator = " ";
        //        if (vm.count("verbose"))
        //        {
        //            separator = "__**__";
        //        }
        //        for(size_t i=0; i<json_info.size(); ++i)
        //        {
        //            std::cout << json_info[i] << separator;
        //        }
        //        std::cout << std::endl;

        //    }

        //    if ( vm.count("manual") )
        //    {
        //        std::cout << "Manually extracted value: "
        //                  << vm["manual"].as<std::string>() << std::endl;
        //    }

    }
    catch(std::exception &e)
    {
        std::cout << "Unhandled Exception reached the top of 'command_line_own_parser': "
                  << e.what() << ", application will now exit" << std::endl;

        output_result += "Unhandled Exception reached the top of 'command_line_own_parser': " +
                         std::string(e.what()) + ", application will now exit. " + NEW_LINE_BREAK;

        return cmd_parser::ERROR_UNHANDLED_EXCEPTION;
    }

    return cmd_parser::SUCCESS;
}




size_t QCmd_pairs_dealer::covert_str_2_argvs(const char * raw_str)
{
    if(raw_str == NULL)
    {
        qDebug() << QString("[QCmd_pairs_dealer::init_cmd_frm_client()] error: raw_str==NULL");
        return cmd_parser::ERROR_IN_COMMAND_LINE;
    }

    QString qstr_tmp_raw(raw_str);
    QStringList list_qstr = qstr_tmp_raw.split(" ",QString::SkipEmptyParts);

    if(list_qstr.count() > 0)
    {
        clear_inner_argvs();

        argc = list_qstr.size();
        argv = new char *[argc];
        for(int iCnt=0 ; iCnt<argc ; iCnt++)
        {
            // 32 bytes should be enough
            argv[iCnt] = new char[32];
            memset(argv[iCnt],0,32);
            memcpy(argv[iCnt],list_qstr[iCnt].toLocal8Bit().data(),strlen(list_qstr[iCnt].toLocal8Bit().data()));

        }

        return cmd_parser::SUCCESS;
    }
    else
    {
        argc = -1;
        argv = NULL;

        qDebug() << QString("[QCmd_pairs_dealer::init_cmd_frm_client()] error: list_qstr.count() <= 0");
        return cmd_parser::ERROR_IN_COMMAND_LINE;
    }
}


void QCmd_pairs_dealer::clear_inner_argvs()
{
    if(argc != -1 && argv != NULL)
    {
        for(int iCnt=0 ; iCnt<argc ; iCnt++)
        {
            if(argv[iCnt])
                delete argv[iCnt];
        }
        delete argv;
    }
}
