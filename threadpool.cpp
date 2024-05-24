#include "threadpool.h"
#include <functional>
#include <thread>
#include <string>
#include <iostream>
#include <condition_variable>

const int Task_MAX_THRESHHOLD = 1024;
const int THREAD_MAX_THRESHHOLD = 10; 
const int THREAD_MAX_IDLE_TIME = 60; //��λ:s
//�̳߳ع���
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

//�̳߳�����
ThreadPool::~ThreadPool()
{}

// �����̳߳صĹ���ģʽ
void ThreadPool::setMood(PoolMode mode) {
	if (cheakRunningState()) return;
	poolMode_ = mode;
}


// ����task�������������ֵ
void ThreadPool::setTaskQueMaxThreshHold(int threshhold) {
	taskQueMaxThreshHold_ = threshhold;
}

// ���̳߳��ύ���� �û����øýӿڣ�������������������� 
Result ThreadPool::submitTask(std::shared_ptr<Task> sp) {
	//��ȡ��
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	//�߳�ͨ��  �ȴ���������п���
	//while (taskQue_.size() == taskQueMaxThreshHold_)
	//{
	//	notFull_.wait(lock);
	//}
	//�߼�д��  �߳�ͨ��  �ȴ���������п���
	//TODO:�û��ύ�����������������1s�������ж��ύ����ʧ�ܣ�����
	if (!notFull_.wait_for(lock, std::chrono::seconds(1), 
		[&]() {return taskQue_.size() < taskQueMaxThreshHold_; })) {//�ȴ�1s�ύ������Ȼû����
		
		std::cerr << "task queue is full,submit task fail!" << std::endl;
		// return task->getResult();   // Task Result �߳�ִ����task��task����ͱ���������
		return Result(sp,false);
	}
	//����п��࣬����������������
	taskQue_.emplace(sp);
	taskSize_++;
	//��Ϊ�·������񣬶��п϶������ˣ���notEmpty_�Ͻ���֪ͨ
	notEmpty_.notify_all();

	//cachedģʽ ������ȽϽ��� ����:С���������  ��Ҫ�������������Ϳ����̵߳��������ж��Ƿ���Ҫ�����µ��̳߳���
	if (poolMode_ == PoolMode::MODE_CACHED
		&& taskSize_>idleThreadSize_ 
		&& curThreadSize_<threadSizeMaxThreshHold_) {
		// �������߳�
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		curThreadSize_++;

	//���������Result����
	return Result(sp);


}


// �����̳߳� 
void ThreadPool::start(int initThreadSize){
	
	//�����̳߳ص�����״̬
	isPoolRunning_ = true;

	//��¼��ʼ�̸߳���
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	//�����̶߳���
	for (int i = 0; i < initThreadSize_; i++) {

		// ����thread�̶߳����ʱ�򣬰��̺߳�������thread�̶߳���
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		//threads_.emplace_back(std::move(ptr));//unique_ptr ���������������ֵ����
	}

	//���������߳�
	for (int i = 0; i < initThreadSize_; i++) {
		threads_[i]->start();//��Ҫȥִ���̺߳���
		idleThreadSize_++;	 //��¼��ʼ�����̵߳�����
	}
} 

//�����̺߳��� �̳߳ص������̴߳��������������������
void ThreadPool::threadFunc(int threadid) {//�̺߳������أ���Ӧ���߳�Ҳ�ͽ�����
	
	auto lastTime = std::chrono::high_resolution_clock().now();

	for (;;) {
		std::shared_ptr<Task> task;
		{
			//�Ȼ�ȡ��
			std::unique_lock<std::mutex> lock(taskQueMtx_);
			
			std::cout << "tid:" << std::this_thread::get_id <<
				"���Ի�ȡ����..." << std::endl;

			//cachedģʽ�£��п����Ѿ������˺ܶ��̣߳������������ʱ�䳬��60s��Ӧ�ðѶ�����߳�
			//�������յ�(����initThreadSize_�������߳�Ҫ���л���)
			//��ǰʱ�� - ��һ���߳�ִ�е�ʱ��>60s
			if (poolMode_ == PoolMode::MODE_CACHED) {
				// ÿһ���ӷ���һ�� ��ô���֣���ʱ���أ������������ִ�з���                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               
				while (taskQue_.size() > 0) {
					//������������ʱ������
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME 
							&& curThreadSize_>initThreadSize_) {
							//��ʼ���յ�ǰ�߳�
							//��¼�߳���������ر�����ֵ�޸�
							//���̶߳�����߳��б�������ɾ��  û�а취 threadFunc <=> thread����
							// threadid =>thread���� => ɾ��
						}
					}
				}
			}
			else {
				//�ȴ�notEmpty����
				notEmpty_.wait(lock, [&]() {return taskSize_ > 0; });
			}
			idleThreadSize_--; //�����̵߳���������

			std::cout << "tid:" << std::this_thread::get_id <<
				"��ȡ����ɹ�" << std::endl;
			
			//�����������ȡ��һ������
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;
			
			//�����Ȼ��ʣ�����񣬼���֪ͨ�����߳�ִ������
			if (taskQue_.size() > 0) {
				notEmpty_.notify_all();
			}
			
			// ȡ����һ������֪ͨnot_Full��������в��������Լ�����������
			notFull_.notify_all();

		}//������������˼��Ϊ���Զ����������߳��õ�����͸ð����ͷŵ���
		//��ǰ�̸߳���ִ���������
		if (task != nullptr) {
			//task->run();   // ִ�����񣬰�����ķ���ֵͨ��setVal��������Result
			task->exec();
		}
		idleThreadSize_++;//���������񣬿����߳���Ŀ++
		lastTime = std::chrono::high_resolution_clock().now();//�����߳�ִ���������ʱ��
	}

}

bool ThreadPool::cheakRunningState() const {
	return isPoolRunning_;
}

//�����̳߳�cachedģʽ���߳���ֵ
void ThreadPool::setThreadSize(int maxSize) {
	if(cheakRunningState()) return;
	if(poolMode_==PoolMode::MODE_CACHED)
		threadSizeMaxThreshHold_ = maxSize;
}




////////////////  �̷߳���ʵ��
int Thread::generateId_ = 0;

//�̹߳���
Thread::Thread(ThreadFunc func)
	:func_(func)
	,threadId_(generateId_++)
{}
//�߳�����
Thread::~Thread() {
}
//�����߳�
void Thread::start() {
	// ����һ���߳���ִ��һ���̺߳���
	std::thread t(func_,threadId_);	//C++11��˵ �̶߳���t �� �̺߳���func_
	t.detach();				//���÷����߳�
}

//��ȡ�߳�id
int Thread::getId()const {
	return threadId_;
}



////////////// Task����ʵ��
Task::Task():result_(nullptr) {}

Task::~Task() = default;

void Task::exec() {
	if (result_ != nullptr) result_->setVal(run()); //���﷢����̬����
}

void Task::setResult(Result* res) {
	result_ = res;
}

//////////////  Result������ʵ��
Result::Result(std::shared_ptr<Task> task, bool isValid) :isValid_(isValid), task_(task) {
	task_->setResult(this);
}

Any Result::get() {
	if (!isValid_) {
		return "";
	}

	sem_.wait();  //task�������û��ִ���� ������������û����߳�

	return std::move(any_);
}

void Result::setVal(Any any) {
	// �洢task�ķ���ֵ
	this->any_ = std::move(any);
	sem_.post(); // �Ѿ���ȡ������ķ���ֵ�������ź�����Դ
}

