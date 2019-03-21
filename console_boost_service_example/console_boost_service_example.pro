QT += core
QT -= gui

CONFIG += c++11

TARGET = ctrl_srv
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    qhandle_allocator.cpp \
    qio_service_pool.cpp \
    stpoolc++.cpp \
    qmyglobal.cpp \
    qsession.cpp \
    qmainserver.cpp \
    qcmd_pairs_dealer.cpp \
    3rd_src/CustomOptionDescription.cpp \
    3rd_src/OptionPrinter.cpp \
    qsession_clientside.cpp \
    qlog_message_handler.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


#
#   工程pro 文件中有具体说明
#   本工程用于探索 boost.asio 的用法
#   =========================================================
#   this project is used for learn how to use boost.asio in Qt
#   there are some differents between asio(lib) and boost.asio .  see
#   Asio and Boost.Asio
#   Asio comes in two variants: (non-Boost) Asio and Boost.Asio. The differences between the two are outlined below.
#   https://think-async.com/Asio/AsioAndBoostAsio.html
#

#
#   编译boost库
#   boost库版本介绍
#   我这里使用的是boost1.69.0，其他的版本也是一样的
#   1、生成b2.exe和bjam.exe可执行文件
#   打开安装Qt 后的命令行工具Qt X.X for Desktop ，进入boost库所在的目录 找到build.bat,然后执行以下命令编译b2和bjam

#   build gcc
#
#   我的build.bat在 daemon_with_boost_1_69\boost_1_69_0\tools\build\src\engine
#   编译完之后在当前目录会生成一个bin.ntx86的目录，进入后有b2.exe和bjam.exe可执行文件，将这两个文件拷贝到boost源代码的根目录下
#   切换到 boost 根目录 然后 执行
#
#   (--toolset= 选择工具集  --with-选择性编译需要的boost相关库 boost.asio 可能会依赖以下, --prefix安装位置 还可使用 --show-libraries 看有哪些库可以用--with来编译)
#   (只构建指定库：--with-<library>)
#   (bjam --show-libraries 列出所有的库)
#   (不构建指定库：--without-<library>)
#   (1、安装路径：--prefix=<PREFIX>)  默认：C:/Boost（Linux：/usr/local）
#   (2、库文件安装路径：--libdir=<DIR>)  默认：<EPREFIX>/lib
#   (3、头文件安装路径：--includedir=<HDRDIR>)   默认：<PREFIX>/include
#   (4、构建类型：--build-type=<type>)    debug release shared static mutil
#   (5、中间文件存放路径：--build-dir=DIR)    默认：Boost根目录
#   (6、指定编译器：--toolset=toolset)

#   bjam --toolset=gcc --with-system
#                      --with-thread                    (有用到 boost::thread 里的要包括)
#                      --with-date_time
#                      --with-regex
#                      --with-serialization
#                      --with-program_options           (程序里 qcmd_pairs_dealer 有用到 命令行处理的类 要包括)
#                      --with-filesystem            (程序里 boost::filesystem::basename() 有用到 命令行处理的类 要包括)
#                      --prefix=D:\XXX\XXX  install
#

#   包含 boost 相关头文件(hpp文件已经包含了声明以及实现)
INCLUDEPATH += ../boost_1_69_0_builded_installed/include/boost-1_69

#   包含第三方源码
INCLUDEPATH += ./3rd_src

# 加入之前研究的 开源线程池模块
INCLUDEPATH += ./include/stpool_include
LIBS += -L./3rd_lib/stpool_lib  -lstpool


#   包含 boost.asio 相关的依赖的库
win32{

    LIBS += -lws2_32 -lwsock32

}

unix{

    LIBS +=

}
LIBS += -L../boost_1_69_0_builded_installed/lib \
#(程序里 qcmd_pairs_dealer 有用到 命令行处理的类 要包括)
        -lboost_program_options-mgw53-mt-x32-1_69 \
        -lboost_regex-mgw53-mt-x32-1_69 \
        -lboost_filesystem-mgw53-mt-x32-1_69 \
# 测试时有用到 boost/thread.hpp 下属的库
        -lboost_thread-mgw53-mt-x32-1_69

MOC_DIR += ./tmp/moc_dir
UI_DIR += ./tmp/ui_dir
OBJECTS_DIR += ./tmp/obj_dir

HEADERS += \
    qhandle_allocator.h \
    qio_service_pool.h \
    stpoolc++.h \
    qmyglobal.h \
    qsession.h \
    qmainserver.h \
    qcmd_pairs_dealer.h \
    3rd_src/CustomOptionDescription.hpp \
    3rd_src/OptionPrinter.hpp \
    qsession_clientside.h \
    qlog_message_handler.h
