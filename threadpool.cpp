#include "threadpool.h"
#include <functional>
#include <thread>
#include <string>
#include <iostream>
#include <condition_variable>

const int Task_MAX_THRESHHOLD = 1024;
const int THREAD_MAX_THRESHHOLD = 10; 
const int THREAD_MAX_IDLE_TIME = 60; //单位:s
//线程池构造
ThreadPool::ThreadPool()
	: initThreadSize_(0)
	, threadSizeMaxThreshHold_(THREAD_MAX_THRESHHOLD)
	, idleThreadSize_(0)
	, curThreadSize_(0)
	, taskSize_(0)
	, taskQueMaxThreshHold_(Task_MAX_THRESHHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
	, isPoolRunning_(false)
{}

//线程池析构
ThreadPool::~ThreadPool()
{}

// 设置线程池的工作模式
void ThreadPool::setMood(PoolMode mode) {
	if (cheakRunningState()) return;
	poolMode_ = mode;
}


// 设置task任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold) {
	taskQueMaxThreshHold_ = threshhold;
}

// 给线程池提交任务 用户调用该接口，传入任务对象，生产任务 
Result ThreadPool::submitTask(std::shared_ptr<Task> sp) {
	//获取锁
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	//线程通信  等待任务队列有空余
	//while (taskQue_.size() == taskQueMaxThreshHold_)
	//{
	//	notFull_.wait(lock);
	//}
	//高级写法  线程通信  等待任务队列有空余
	//TODO:用户提交任务最长不能阻塞超过1s，否则判断提交任务失败，返回
	if (!notFull_.wait_for(lock, std::chrono::seconds(1), 
		[&]() {return taskQue_.size() < taskQueMaxThreshHold_; })) {//等待1s提交任务仍然没满足
		
		std::cerr << "task queue is full,submit task fail!" << std::endl;
		// return task->getResult();   // Task Result 线程执行完task，task对象就被析构掉了
		return Result(sp,false);
	}
	//如果有空余，把任务放入任务队列
	taskQue_.emplace(sp);
	taskSize_++;
	//因为新放了任务，队列肯定不空了，在notEmpty_上进行通知
	notEmpty_.notify_all();

	//cached模式 任务处理比较紧急 场景:小而快的任务  需要根据任务数量和空闲线程的数量，判断是否需要创建新的线程出来
	if (poolMode_ == PoolMode::MODE_CACHED
		&& taskSize_>idleThreadSize_ 
		&& curThreadSize_<threadSizeMaxThreshHold_) {
		// 创建新线程
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		curThreadSize_++;

	//返回任务的Result对象
	return Result(sp);


}


// 开启线程池 
void ThreadPool::start(int initThreadSize){
	
	//设置线程池的运行状态
	isPoolRunning_ = true;

	//记录初始线程个数
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	//创建线程对象
	for (int i = 0; i < initThreadSize_; i++) {

		// 创建thread线程对象的时候，把线程函数给到thread线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		//threads_.emplace_back(std::move(ptr));//unique_ptr 不允许拷贝构造和左值传递
	}

	//启动所有线程
	for (int i = 0; i < initThreadSize_; i++) {
		threads_[i]->start();//需要去执行线程函数
		idleThreadSize_++;	 //记录初始空闲线程的数量
	}
} 

//定义线程函数 线程池的所有线程从任务队列里面消费任务
void ThreadPool::threadFunc(int threadid) {//线程函数返回，相应的线程也就结束了
	
	auto lastTime = std::chrono::high_resolution_clock().now();

	for (;;) {
		std::shared_ptr<Task> task;
		{
			//先获取锁
			std::unique_lock<std::mutex> lock(taskQueMtx_);
			
			std::cout << "tid:" << std::this_thread::get_id <<
				"尝试获取任务..." << std::endl;

			//cached模式下，有可能已经创建了很多线程，但是如果空闲时间超过60s，应该把多余的线程
			//结束回收掉(超过initThreadSize_数量的线程要进行回收)
			//当前时间 - 上一次线程执行的时间>60s
			if (poolMode_ == PoolMode::MODE_CACHED) {
				// 每一秒钟返回一次 怎么区分：超时返回？还是有任务待执行返回                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               
				while (taskQue_.size() > 0) {
					//条件变量，超时返回了
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME 
							&& curThreadSize_>initThreadSize_) {
							//开始回收当前线程
							//记录线程数量的相关变量的值修改
							//把线程对象从线程列表容器中删除  没有办法 threadFunc <=> thread对象
							// threadid =>thread对象 => 删除
						}
					}
				}
			}
			else {
				//等待notEmpty条件
				notEmpty_.wait(lock, [&]() {return taskSize_ > 0; });
			}
			idleThreadSize_--; //空闲线程的数量减少

			std::cout << "tid:" << std::this_thread::get_id <<
				"获取任务成功" << std::endl;
			
			//从任务队列中取出一个任务
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;
			
			//如果依然有剩余任务，继续通知其他线程执行任务
			if (taskQue_.size() > 0) {
				notEmpty_.notify_all();
			}
			
			// 取出完一个任务，通知not_Full，任务队列不满，可以继续生产任务
			notFull_.notify_all();

		}//添加作用域的意思是为了自动析构锁，线程拿到任务就该把锁释放掉了
		//当前线程负责执行这个任务
		if (task != nullptr) {
			//task->run();   // 执行任务，把任务的返回值通过setVal方法给到Result
			task->exec();
		}
		idleThreadSize_++;//处理完任务，空闲线程数目++
		lastTime = std::chrono::high_resolution_clock().now();//更新线程执行完任务的时间
	}

}

bool ThreadPool::cheakRunningState() const {
	return isPoolRunning_;
}

//设置线程池cached模式下线程阈值
void ThreadPool::setThreadSize(int maxSize) {
	if(cheakRunningState()) return;
	if(poolMode_==PoolMode::MODE_CACHED)
		threadSizeMaxThreshHold_ = maxSize;
}




////////////////  线程方法实现
int Thread::generateId_ = 0;

//线程构造
Thread::Thread(ThreadFunc func)
	:func_(func)
	,threadId_(generateId_++)
{}
//线程析构
Thread::~Thread() {
}
//启动线程
void Thread::start() {
	// 创建一个线程来执行一个线程函数
	std::thread t(func_,threadId_);	//C++11来说 线程对象t 和 线程函数func_
	t.detach();				//设置分离线程
}

//获取线程id
int Thread::getId()const {
	return threadId_;
}



////////////// Task方法实现
Task::Task():result_(nullptr) {}

Task::~Task() = default;

void Task::exec() {
	if (result_ != nullptr) result_->setVal(run()); //这里发生多态调用
}

void Task::setResult(Result* res) {
	result_ = res;
}

//////////////  Result方法的实现
Result::Result(std::shared_ptr<Task> task, bool isValid) :isValid_(isValid), task_(task) {
	task_->setResult(this);
}

Any Result::get() {
	if (!isValid_) {
		return "";
	}

	sem_.wait();  //task任务如果没有执行完 ，这里会阻塞用户的线程

	return std::move(any_);
}

void Result::setVal(Any any) {
	// 存储task的返回值
	this->any_ = std::move(any);
	sem_.post(); // 已经获取的任务的返回值，增加信号量资源
}

