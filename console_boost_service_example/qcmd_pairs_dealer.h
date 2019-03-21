#ifndef QCMD_PAIRS_DEALER_H
#define QCMD_PAIRS_DEALER_H


#include <QDebug>
#include <QString>


#include <string>
#include <iostream>

//实例化对象并且初始化
//每次再次调用无需重新定义stringstream类一个对象，只要清空再使用即可
//strStream.clear();
#include <sstream>

#include "qmyglobal.h"

/*
 * program options是一系列pair<name,value>组成的选项列表,它允许程序通过命令行或配置文件来读取这些参数选项.

主要组件

program_options的使用主要通过下面三个组件完成：

组件名                                     作用
options_description(选项描述器)             描述当前的程序定义了哪些选项
parse_command_line(选项分析器)              解析由命令行输入的参数
variables_map(选项存储器)                   容器,用于存储解析后的选项


构造option_description对象和variables_map对象add_options()->向option_description对象添加选项

*/

/// 关于该类介绍见下
/// http://www.radmangames.com/programming/how-to-use-boost-program_options#accessingOptionValues
/// https://blog.csdn.net/cchd0001/article/details/43967451
/// https://blog.csdn.net/yahohi/article/details/9790921
#include <boost/program_options.hpp>

#include <boost/filesystem.hpp>


namespace cmd_parser
{
    const size_t SUCCESS = 0;
    const size_t ERROR_IN_COMMAND_LINE = 1;
    const size_t ERROR_NOT_LOGGED_IN = 2;
    const size_t ERROR_CMD_NOT_COMPLETE = 3;
    const size_t ERROR_UNHANDLED_EXCEPTION = 4;
}


namespace cmd_type
{
    const std::string LOGIN_WITH_NAME_CMD = "login with ";
    const std::string LOGIN_SUCESS = "login success. ";
    const std::string LOGIN_FAILED = "login failed. ";
    const std::string LOGIN_DENY = "login deny. ";

    const std::string PING_OK_FROM_SERVER = "ping ok from srv";
    const std::string PING_CLIENT_LIST_CHANGE = "ping client_list_changed";
    const std::string PING_OK_FROM_CLIENT = "ping ok from client";

}


// linux 系统上 工具风格输入
class QCmd_pairs_dealer
{
    typedef boost::program_options::options_description my_option_dealer;

public:
    QCmd_pairs_dealer(void *parent);

    ~QCmd_pairs_dealer()
    {
        if(m_pPro_desc != NULL)
        {
            delete m_pPro_desc;
            m_pPro_desc = NULL;
        }

        clear_inner_argvs();
    }

    //使用默认的 而非自定义
    //QCmd_pairs_dealer(const QCmd_pairs_dealer& rhs);
    //QCmd_pairs_dealer & operator = (const QCmd_pairs_dealer& rhs);


    /// 服务端与客户端之间的报文解析 最好头部带上长度 要不然对方都不知道到底收多少才算完整
    //解析来自客户端的字节流
    size_t parse_cmd_from_client(const char * raw_str,
                                 size_t raw_str_len,
                                 std::string &output_result);

    //解析来自服务端的字节流
    size_t parse_cmd_from_srv(const char * raw_str,
                              size_t raw_str_len,
                              std::string &output_result);

    //先得调用这个函数 才能接下来调用 下面的获取特定配置
    size_t command_line_own_parser(const char * raw_str,
                                   std::string &output_result);


    static std::string get_login_with_command(){ return cmd_type::LOGIN_WITH_NAME_CMD; }
    static std::string get_login_success_command(){ return cmd_type::LOGIN_SUCESS; }
    static std::string get_login_failed_command(){ return cmd_type::LOGIN_FAILED; }
    static std::string get_login_deny_command(){ return cmd_type::LOGIN_DENY; }

    static std::string get_ping_ok_frm_srv_command(){ return cmd_type::PING_OK_FROM_SERVER; }
    static std::string get_ping_ok_frm_client_command() { return cmd_type::PING_OK_FROM_CLIENT; }
    static std::string get_ping_client_changed_command() { return cmd_type::PING_CLIENT_LIST_CHANGE; }


    // 取 'login with '后面的名字
    static size_t get_name_frm_login_cmd(const std::string &original_cmd,size_t raw_byte_len,std::string &username)
    {
        size_t found = original_cmd.find(get_login_with_command());

        if((found + get_login_with_command().size()) >= original_cmd.size() )
        {
            username.clear();
            return cmd_parser::ERROR_CMD_NOT_COMPLETE;
        }

        size_t name_len = raw_byte_len - get_login_with_command().size() - found;
        username = original_cmd.substr(found + get_login_with_command().size(),name_len);
        return cmd_parser::SUCCESS;
    }





    int get_pingpong_times()
    {
        return m_pingpong_times_max;
    }

    float get_reserve_float_value()
    {
        return m_reserve_float_v;
    }

    std::string get_app_version_info()
    {
        return m_app_version;
    }



private:
    size_t covert_str_2_argvs(const char * raw_str);
    void clear_inner_argvs();

    void set_app_version_info(const char * macro_app_ver_info){
        m_app_version = macro_app_ver_info;
    }

private:

    my_option_dealer *m_pPro_desc;
    std::string m_str_description;

    // 解析命令后存储的客户端 命令键值对
    boost::program_options::variables_map m_cmd_variable_map;
    // 原始键值对值 空格分开
    int argc;
    char **argv;

    // 服务端与客户端之间通讯 发送测试 pingpong 包次数 配置
    int m_pingpong_times_max;
    int m_pingpong_counter;

    // 备用 float 参数
    float m_reserve_float_v;

    // 程序版本信息
    std::string m_app_version;

    // client json info 列表
    std::vector<std::string> m_client_json_info;

    // 父对象
    void *m_parent;
};

#endif // QCMD_PAIRS_DEALER_H
