#ifndef QHANDLE_ALLOCATOR_H
#define QHANDLE_ALLOCATOR_H

#include <stdio.h>
#include <cstdlib>
#include <iostream>

//添加 boost 内容 (添加头文件即可使用 hpp== 实现+声明)
//noncopyable: 程序轻松实现一个不可复制的类
/*
 * 在c++定义一个类时。假设不明白定义复制构造函数和复制赋值操作符。编译器会为我们自己主动生成这两个函数。

比如

class empty_class{ } ；

实际上类似于

class empty_class

{

public:

    empty_class(const empty_class &){...}

    empty_class & operator=(const empty_class &){...}

};

但有时候我们不要类的复制语义，希望禁止复制类的实现。比較常见的做法就是私有化复制构造函数和赋值操作符，

手写代码也非常easy。如：

class do_not_copy

{

private:

    do_not_copy(const do_not_copy &);

    void operator=(const do_not_copy &);

};



但假设程序中有大量这种类，反复写代码也是非常麻烦的，解决方法也不够优雅。

noncopyable为实现不可复制的类提供了简单清晰的解决方式：从 boost::noncopyable 派生就可以。
*/
#include <boost/noncopyable.hpp>

// https://www.cnblogs.com/xiaobingqianrui/p/9068226.html   参考内存对齐的功能
// https://blog.csdn.net/markl22222/article/details/38051483
#include <boost/aligned_storage.hpp>



// Class to manage the memory to be used for handler-based custom allocation.
// It contains a single block of memory which may be returned for allocation
// requests. If the memory is in use when an allocation request is made, the
// allocator delegates allocation to the global heap.
//相当于内存池功能，避免大量异步I/O时内存new delete的开销。
//手动管理 对齐分配 1KB 的堆内存
class QHandle_allocator: private boost::noncopyable
{
public:
    QHandle_allocator():m_in_use_(false),m_allocate_size_lasttime(0)
    {

    }


    std::size_t get_lasttime_used_size(){return m_allocate_size_lasttime;}


    //避免针对 小内存 m_storage_.size 频繁调用的时候不会 频繁重复分配
    void * allocate_own(std::size_t size)
    {
        if(!m_in_use_ && size<m_storage_.size)
        {
            m_in_use_ = true;

            m_allocate_size_lasttime = m_storage_.size;
            return m_storage_.address();  //返回已有地址
        }
        else
        {
            // C++中的new/delete与operator new/operator delete
            // https://www.cnblogs.com/luxiaoxun/archive/2012/08/10/2631812.html
            m_allocate_size_lasttime = size;
            return ::operator new(size);        //相当于malloc 的功能
        }
    }

    void deallocate(void* pointer)
    {
        if (pointer == m_storage_.address())
        {
            m_in_use_ = false;
            m_allocate_size_lasttime = m_storage_.size;
        }
        else
        {
            m_allocate_size_lasttime = 0;
            ::operator delete(pointer);
        }
    }


private:
    // Storage space used for handler-based custom memory allocation.
    // 用于对齐堆内存?
    boost::aligned_storage<1024> m_storage_;

    // Whether the handler-based custom allocation storage has been used.
    bool m_in_use_;

    // last time used memory bytes length
    std::size_t m_allocate_size_lasttime;
};




// Wrapper class template for handler objects to allow handler memory
// allocation to be customised. Calls to operator() are forwarded to the
// encapsulated handler.
//函数对象，handle的再次封装，封装了内存分配和释放
//以下是模板类, 专门对上面类的调用,又封装一层
template <typename Handler>
class QCustom_dispatch_handler
{
public:
    //通过该构造函数可以发现，每次异步读写操作会实例化一次(和qsession 中的 async_read_some 等结合使用)
    QCustom_dispatch_handler(QHandle_allocator &a,Handler h_)
        :m_alloctor(a),m_handler_(h_)
    {

    }

    //模板函数
    //重载的函数操作符,对对象使用起来就像对象是一个函数一样
    //主要作用 除了构造函数外，其他用 QCustom_dispatch_handler'对象' 调用括号将参数传给 Handler
    /*
     * class test
       {
            void operator()(int x)
            {
                cout<<x<<endl;
            }
       }

       int main()
       {
            test t;
            t(10);
            return 0;
        }
    */
    //实际上由内部的handle执行。外部只是一层函数对象进行包装。具体来说 实际调用中 如qsession 中
    // async_read_some的 bind函数 返回的函数对象
    template <typename Arg1>
    void operator()(Arg1 arg1)
    {
        m_handler_(arg1);               //函数对象
    }

    template <typename Arg1, typename Arg2>
    void operator()(Arg1 arg1, Arg2 arg2)
    {
        m_handler_(arg1, arg2);         //函数对象
    }


    /*
     * 友元函数是可以直接访问类的私有成员的 '非成员函数!!'，但是它可以访问类中的私有成员。友元的作用在于提高程序的运行效率，
     * 但是，它破坏了类的封装性和隐藏性，使得非成员函数可以访问类的私有成员
    */
    /* 为handler默认的分配函数
     * http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference/asio_handler_allocate.html
     * 异步操作需要分配一个临时对象，这对象和handle相对应。这些对象默认实现方式是new delete
     * 所有handle相关的临时对象，将在调用该handle之前执行deallocated解分配。所以允许对对象的内存重用。
     **/
    // 自动调用
    friend void* asio_handle_allcate(std::size_t size,
                                     QCustom_dispatch_handler<Handler> *this_handler)
    {
        //size 超过大小  不会重复利用已经分配的堆内存
        return this_handler->m_alloctor.allocate_own(size);
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/,
                                        QCustom_dispatch_handler<Handler>* this_handler)
    {
        this_handler->m_alloctor.deallocate(pointer);
    }


private:

    QHandle_allocator &m_alloctor;    //这里是引用 注意 因为 QHandle_allocator  noncopyable(从其他地方获取)
    Handler m_handler_;             //外部某种类     //函数对象
};




//最后 构造一个全局的 模板函数 可用来手动分配堆内存
//封装之前的结构
// Helper function to wrap a handler object to add custom allocation.
template <typename Handler>
inline QCustom_dispatch_handler<Handler> make_custom_alloc_handler(
    QHandle_allocator& a, Handler h)
{
    //调用这个函数 会 初始化 QCustom_dispatch_handler<Handler> 对象 调用相应构造函数
    //make_custom_alloc_handler构造了一个custom_alloc_handler，
    //并且把这个handler传给asio的async_read_some等函数。
    //async_read_some会调用custom_alloc_handler::asio_handler_allocate分配一块内存并保存这个custom_alloc_handler。
    //这个custom_alloc_handler就是前面所说的必须支持operator()(...)
    //以及asio_handler_allocate和asio_handler_deallocate的class object。

    return QCustom_dispatch_handler<Handler>(a, h);
}



#endif // QHANDLE_ALLOCATOR_H
