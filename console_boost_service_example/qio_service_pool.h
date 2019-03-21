#ifndef QIO_SERVICE_POOL_H
#define QIO_SERVICE_POOL_H


//下面 简述 boost::bind 的用法
//boost::bind 是对 stl 标准库中绑定器作用的强化  语法也更清晰
//bind是C++98标准库中函数适配器bind1st、bind2nd的泛化和增强，可以适配任意的可调用对象，包括函数，函数指针，函数引用，成员函数指针和函数对象。
//bind远远的超越了STL中的函数绑定其(bind1st/bind2nd)，可以绑定最多9个函数参数，而且对绑定对象的要求很低，
//可以在没有result_type内部类型定义的情况下完成对函数对象的绑定。bind库很好的增强了标准库的功能。

/*
 * 1 绑定普通函数(函数，函数指针)
 *
 * int f(int a,int b){return a+b;}//二元函数
 * int g(int a,int b,int c){return a+b*c;}//三元函数
 *
 * void test1()
{

    //绑定表达式没有使用占位符

    cout<<bind(f,1,2)()<<endl;//3 bind(f,1,2)返回一无参调用的函数对象，等价于f(1,2)

    cout<<bind(g,1,2,3)()<<endl;//7 同上

    cout<<f(1,2)<<" "<<g(1,2,3)<<endl;//3 7



    //使用占位符

    int x=0,y=1,z=2;

    cout<<bind(f,_1,9)(x)<<endl;//9 ==>>f(x,9)

    cout<<bind(f,_1,_2)(x,y)<<endl;//1 ==>>f(x,y)

    cout<<bind(f,_2,_1)(x,y)<<endl;//1 ==>>f(y,x)

    cout<<bind(f,_1,_1)(x,y)<<endl;//0 ==>>f(x,x);y参数被忽略了

    cout<<bind(g,_1,8,_2)(x,y)<<endl;//8 ==>>g(x,8,y)

    cout<<bind(g,_3,_2,_2)(x,y,z)<<endl;//3 ==>>g(z,y,y);x参数被忽略了

    //必须在绑定表达式中提供函数要求的所有参数，无论是真实参数还是占位符均可以。但不能使用超过函数参数数量的占位符，eg:在绑定f是不能使用_3；在绑定g是不能使用_4；也不能写bind(f,_1,_2,_2)这样的形式，否则会导致编译错误

}
*/
/*
 * 2 绑定函数指针 用法同上，可以还有占位符，也可以不使用占位符
 * typedef int (*f_type)(int,int);//函数指针定义
 * typedef int(*g_type)(int,int,int);//函数指针定义
 *
 * void test2()
{

    f_type pf=f;

    g_type pg=g;

    int x=1,y=2,z=3;

    cout<<bind(pf,_1,9)(x)<<endl;//10  ==>>(*pf)(x,9)

    cout<<bind(pg,_3,_2,_2)(x,y,z)<<endl;//7 ==>>(*pg)(z,y,y)

}
*/
/*
 *
 * 3 绑定成员函数
 * void test3()
{
//类的成员函数不同于普通函数，因为成员函数指针不能直接调用opreator()，它必须被绑定到一个对象或者指针上，然后才能得到this指针进而调用成员函数。因此bind需要牺牲一个占位符的位置(意味着使用成员函数是只能最多绑定8个参数)，要求用户提供一个类的实例，引用或者指针，通过对象最为第一个参数来调用成员函数。

    struct demo//使用struct仅仅是为了方便，不必写出public

    {

        int f(int a,int b){return a+b;}

    };

    demo a,&ra=a,*p=&a;//类的实例对象，引用，指针

    cout<<bind(&demo::f,a,_1,20)(10)<<endl;//30 ==>>a.f(10,20)

    cout<<bind(&demo::f,ra,_2,_1)(10,20)<<endl;//30 ==>>ra.f(20,10)

    cout<<bind(&demo::f,p,_1,_2)(10,20)<<endl;//30 ==>p->f(10,20)

    //注意：我们必须在成员函数前加上取地址操作符&，表明这是一个成员函数指针，否则会无法通过编译，这是与绑定函数的一个小小的不同。
*/
/* 还有一些其他的绑定方法 暂时不介绍了 */
//https://www.2cto.com/kf/201301/185167.html

#include <boost/bind.hpp>

/*
 * shared_ptr是一种智能指针（smart pointer），作用有如同指针，但会记录有多少个shared_ptrs共同指向一个对象。
 * 这便是所谓的引用计数（reference counting）。一旦最后一个这样的指针被销毁，也就是一旦某个对象的引用计数变为0，
 * 这个对象会被自动删除。这在非环形数据结构中防止资源泄露很有帮助。
 *
 * 在boost库中， 智能指针并不只shared_ptr一个。同族但有不同的功能目标的还有如下5个：
 * scoped_ptr
 * scoped_array
 * shared_ptr
 * shared_array
 * weak_ptr
 *
 * 前面两个， 与标准C++中的智能指针auto_ptr功能基本类似， 不过它不传递所有权， 不可复制。
 * 从其名称就可以看出， 其主要目标就是在小范围， 小作用域中使用，以减少显式的delete, new配对操作，
 * 提高代码的安全性。scoped_ptr是针对指针的版本， 而scoped_array同是专门针对数组的。（记住， 删除
 * 一个指针使用delete p; 而删除一个数组使用delete[] p);
 *
 * 后面两个是重点，它们在所有对象中， 共享所指向实体的所有权。 即只要是指向同一个实体
 * 对象的shared_ptr对象， 都有权操作这个对象，并根据自己产生新的对象，并把所有权共享给新的对象。即它
 * 是满足STL对对象的基本要求可复制，可赋值的。可以与所有的STL容器，算法结合使用。顾名思义， shared_ptr
 * 是针对任意类型的指针的， 而shared_array则是专门针对任意类型的数组的。
 *
 *
 * (1)删除共享对象
 * 使用shared_ptr解决的主要问题是知道删除一个被多个客户共享的资源的正确时机
 *
    classA {
        boost::shared_ptr<int>  no_;
        public:
        A(boost::shared_ptr<int>    no) : no_(no) {}
        void    value(int i) {
            *no_=i;
        }
    };
    classB {
        boost::shared_ptr<int>  no_;
        public:
        B(boost::shared_ptr<int>    no) : no_(no) {}
        int value() const {
            return  *no_;
        }
    };

    int main() {
        boost::shared_ptr<int>  temp(new int(14));
        Aa(temp);
        Bb(temp);
        a.value(28);
        assert(b.value()==28);
    };

    类 A和 B都保存了一个shared_ptr<int>.在创建A和 B的实例时，shared_ptrtemp被传送到它们的构造函数。
    这意味着共有三个shared_ptr：a,b,和 temp，它们都引向同一个int实例。如果我们用指针来实现对一个的共享，
    A和 B必须能够在某个时间指出这个int要被删除。在这个例子中，直到main的结束，引用计数为3，当所有shared_ptr离开了作用域，计数将达到0，
    而最后一个智能指针将负责删除共享的 int.

    共享指针存入标准库容器的例子 然后自动删除的例子

    classA {
        public:
        virtual void sing()=0;
        protected:
        virtual ~A() {};
    };

    classB : public A {
        public:
        virtual void sing() {
        std::cout<< "Do re mi fa so la";
        }
    };

    boost::shared_ptr<A>  createA() {
        boost::shared_ptr<A>p(new B());
        return  p;
    }

    int main() {
        typedef     std::vector<boost::shared_ptr<A> >  container_type;
        typedef     container_type::iterator iterator;
        container_type  container;
        for(int i=0;i<10;++i) {
            container.push_back(createA());
        }

        std::cout<< "The choir is gathered: \n";
        iterator    end=container.end();
        for(iterator it=container.begin();it!=end;++it) {
            (*it)->sing();
        }
    }

    这里有两个类, A和 B,各有一个虚拟成员函数 sing. B从 A公有继承而来，并且如你所见，工厂函数 createA返回一个动态分配的B的实例，
    包装在shared_ptr<A>里。在 main里,一个包含shared_ptr<A>的std::vector被放入10个元素，最后对每个元素调用sing。
    如果我们用裸指针作为元素，那些对象需要被手工删除。而在这个例子里，删除是自动的，因为在vector的生存期中，
    每个shared_ptr的引用计数都保持为1；当 vector被销毁，所有引用计数器都将变为零，所有对象都被删除。
    有趣的是，即使A的析构函数没有声明为virtual,shared_ptr也会正确调用B的析构函数！

*/
#include <boost/shared_ptr.hpp>

//关于asio 详情 io_service 等信息 说明 见 daemon_with_boost_1_69\boost_1_69_0\boost_1_56_pdf\boost_1_56_pdf 内有关内容
//也可参考如下 https://www.cnblogs.com/lidabo/p/3790612.html
//  https://blog.csdn.net/zhangzq86/article/details/82495080
#include <boost/asio.hpp>

/*
 * 首先看看boost::thread的构造函数吧，boost::thread有两个构造函数：
（1）thread()：构造一个表示当前执行线程的线程对象；
（2）explicit thread(const boost::function0<void>& threadfunc)：
 boost::function0<void>可以简单看为：一个无返回(返回void)，无参数的函数。这里的函数也可以是类重载operator()构成的函数；
 该构造函数传入的是函数对象而并非是函数指针，这样一个具有一般函数特性的类也能作为参数传入
*/
//https://www.cnblogs.com/lidabo/p/4022715.html
#include <boost/thread.hpp>


#include "qhandle_allocator.h"
#include "qmyglobal.h"




using std::hex;
///
/// 继承CTask 方法必须重写 onRun()   onErrHandler(long reasons)  两个方法
/// 可以进一步对任务优先级等进行封装
class QRun_Io_Service_Task : public stpool::CTask
{
    ///指向 io_service 的共享指针
    typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;

    public:
        QRun_Io_Service_Task(const string &name ,const io_service_ptr ios_ref,stpool::CTaskPool *pTaskPool = NULL):
            stpool::CTask(name, pTaskPool) {
            qDebug() << __FUNCTION__ << ":" << this << "\n";

            m_io_service_ref = ios_ref;
        }

        ~QRun_Io_Service_Task() {
            qDebug() << __FUNCTION__ << ":" << this << "\n";
        }

        std::string get_task_name()
        {
            taskName();
        }

        virtual void onRun() {
            qDebug() << "task('"<< taskName().c_str() << "') is running ..."  << "\n";

            // 启动事件循环  次线程的事件循环
            // 由于有 boost::asio::io_service::work 在没有pending的消息时，io.run也不退出.
            (*m_io_service_ref).run();
        }

        virtual void onErrHandler(long reasons) {
            qCritical() << "task('"<< taskName().c_str() << "') has not been executed for reasons(" << hex << reasons << ")" << "\n";
        }

    private:
        io_service_ptr m_io_service_ref;
};





///
/// A pool of io_service objects.
class QIo_service_pool:public boost::noncopyable
{
public:

    ///显式 Construct the io_service pool.
    explicit QIo_service_pool(std::size_t pool_size,std::string poolname ="default_pool"):m_next_io_service_(0)
    {
        m_pool_name = poolname;
        m_task_ptr_collector.clear();

        if(pool_size == 0)
        {
            //do some log...

            //throw exception...
            throw std::runtime_error("QIo_service_pool size is 0");
        }


        // Give all the io_services work to do so that their run() functions will not
        // exit until they are explicitly stopped.
        for (std::size_t i = 0; i < pool_size; ++i)
        {
            io_service_ptr io_service(new boost::asio::io_service);     //智能指针
            work_ptr work(new boost::asio::io_service::work(*io_service));    //work 可拷贝
            io_services_.push_back(io_service);         //智能指针存入标准模板类
            work_.push_back(work);
        }

    }


    ~QIo_service_pool()
    {
        qDebug() << QString("[~QIo_service_pool()]: < poolname-- %1 >ready to destruct. wait()")
                    .arg(m_pool_name.c_str());

        wait();
    }

    // Run all io_service objects in the pool.
    void run()
    {
        //将io_service.run 构造成任务放到线程池中跑

        /*
         * 原有方法 仅供参考
        // Create a pool of threads to run all of the io_services.
        std::vector<boost::shared_ptr<boost::thread> > threads;
        for (std::size_t i = 0; i < io_services_.size(); ++i)
        {
            boost::shared_ptr<boost::thread> thread(new boost::thread(
                                                        boost::bind(&boost::asio::io_service::run, io_services_[i])));
            threads.push_back(thread);
        }

        // Wait for all threads in the pool to exit.
        for (std::size_t i = 0; i < threads.size(); ++i)
            threads[i]->join();   这里导致阻塞
         */

        for(std::size_t i = 0; i < io_services_.size(); ++i)
        {
            // Create a task and set its destination pool to our created pool
            QString qstr = QString("%1_runT_")
                    .arg(m_pool_name.c_str());
            qstr += QString::number(i,16);

            std::string t_name = qstr.toStdString();

            QRun_Io_Service_Task *task  = new QRun_Io_Service_Task(t_name,io_services_[i]);
            task->setPool(g_pGlobal_task_pool);

            task->queue();  //任务安排到线程池中 准备执行  task->onRun() 中事件循环被调用
            m_task_ptr_collector.push_back(task);
        }


    }


    // wait task in the pool finish( will block 阻塞)
    // 应该不需要调用. 因为主线程的线程池 会在退出前等待所有任务完毕
    // join 的工作放到最后
    void wait()
    {
        //等待任务结束 阻塞 (主线程调用这个函数 就有种主线程 join() 等待次线程的意思)
        for(std::size_t iCnt = 0; iCnt < m_task_ptr_collector.size(); ++iCnt)
        {
            m_task_ptr_collector[iCnt]->wait();

            /// 结束返回时 被析构
            qDebug() << QString("[QIo_service_pool::wait] ready to destruct task <%1:%2>")
                         .arg(iCnt).arg(m_pool_name.c_str());
            delete m_task_ptr_collector[iCnt];
        }

        m_task_ptr_collector.clear();
    }


    // Stop all io_service objects in the pool.
    void stop()
    {
        // 也会使 线程池中几个运行io_service.run() 事件循环的线程退出
        // Explicitly stop all io_services.
        for (std::size_t i = 0; i < io_services_.size(); ++i)
            io_services_[i]->stop();
    }


    // Get an io_service to use.
    boost::asio::io_service& get_io_service()
    {
        // Use a round-robin scheme to choose the next io_service to use.
        boost::asio::io_service& io_service = *io_services_[m_next_io_service_];
        ++m_next_io_service_;
        if (m_next_io_service_ == io_services_.size())
            m_next_io_service_ = 0;
        return io_service;
    }



private:

private:
    ///指向 io_service 的共享指针
    typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
    /// 配合 io_service 使用
    /// 由于io_service并不会主动常见调度线程，需要我们手动分配，常见的方式是给其分配一个线程，然后执行run函数(事件循环)。
    /// 但run函数在io事件完成后会退出，线程会终止，后续基于该对象的异步io任务无法得到调度。
    /// 解决这个问题的方法是通过一个asio::io_service::work对象来守护io_service。
    /// 这样，即使所有io任务都执行完成，也不会退出，继续等待新的io任务。
    /// 例:
    /// boost::asio::io_service io;
    /// boost::asio::io_service::work work(io);
    /// io.run();
    ///
    /// 备注： 利用boost.deadline_timer() 可以很方便的实现定时器功能
    typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

    /// The pool of io_services.
    std::vector<io_service_ptr> io_services_;

    /// The work that keeps the io_services running.
    std::vector<work_ptr> work_;

    /// The next io_service to use for a connection.
    std::size_t m_next_io_service_;

    /// tasklist collector
    std::vector<QRun_Io_Service_Task *> m_task_ptr_collector;

    /// description:io_service_pool name
    std::string m_pool_name;

};


#endif // QIO_SERVICE_POOL_H
