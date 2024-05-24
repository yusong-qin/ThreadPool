// 线程池项目.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <chrono>
#include <thread>

#include "threadpool.h"

using ULong = unsigned long long;

class MyTask : public Task {
public:
    MyTask(ULong begin, ULong end) :
        begin_(begin),
        end_(end) {}

    //问题1:怎么设计返回值，让他可以返回任意数据类型
    Any run() {
        std::cout << "tid:" << std::this_thread::get_id() << "begin!" << std::endl;

        ULong sum = 0;
        for (ULong i = begin_; i < end_; i++) {
            sum += i;
        }

        std::cout << "sum:" << sum << std::endl;
        
        std::cout << "tid:" << std::this_thread::get_id() << "end!" << std::endl;

        return sum;
    }

private:
    ULong begin_;
    ULong end_;
};

/*
有些场景，是希望能够获得线程执行任务的返回值的
举例:
1+.....+30000;

thread1 1+...+10000;
thread2 10000+...+20000;
thread3 20000+...+30000;

main thread : 给每一个线程分配计算的区间，并等待他们算完返回结果，合并最终的结果即可
*/


int main()
{
    ThreadPool pool;
    pool.setMood(PoolMode::MODE_CACHED);
    pool.start();


    Result res1 = pool.submitTask(std::make_shared<MyTask>(1,10000000));
    Result res2 = pool.submitTask(std::make_shared<MyTask>(10000000, 20000000));
    Result res3 = pool.submitTask(std::make_shared<MyTask>(20000000, 30000000));

    ULong sum1 = res1.get().cast<ULong>();//get 返回了一个Any类型,怎么转成具体的类型呢？
    ULong sum2 = res2.get().cast<ULong>();
    ULong sum3 = res3.get().cast<ULong>();


    //Master - Slave线程模型
    //Master线程用来分解任务，然后给各个Salve线程分配任务
    //等待各个Slave线程执行完任务，返回结果
    //Master线程合并各个任务结果，输出
    std::cout <<"THE END!" << (sum1 + sum2 + sum3) << std::endl;

    getchar();
    return 0;
}
