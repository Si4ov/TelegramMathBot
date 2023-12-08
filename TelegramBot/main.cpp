#include <stdio.h>
#include <tgbot/tgbot.h>
#include <vector>
#include <string>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <atomic>
#include <queue>
#include <thread>
#include <chrono>
#include <regex>



//Task interface
class TaskBaseClass
{
private:
	TgBot::Bot& BotId;
	int64_t ChatId;

public:

	TaskBaseClass(TgBot::Bot& bId, int64_t cId) : BotId(bId), ChatId(cId) {}


	virtual void InputProcess()  = 0;
	virtual void Respond()  = 0;

	virtual ~TaskBaseClass() {};

	TgBot::Bot& GetBotId() const { return BotId; }
	int64_t GetChatId() const { return ChatId; }

};

//Fibonacci task
class TaskFibonacci: public TaskBaseClass
{
private:

	std::string Command;
	int CalcData;
	
	std::string FibResult;
public:

	TaskFibonacci(std::string cmd, int cData, TgBot::Bot& bId, int64_t cId) : TaskBaseClass(bId, cId), Command(cmd), CalcData(cData) {}

	std::string CalcFibonacci(long long Data)
	{
		if (Data > 0 && Data <= 90)
		{

			long long a = 1, b = 1, Results;

			for (int i = 2; i <= Data; ++i) {
				long long temp = a + b;
				a = b;
				b = temp;
			}

			(Results = 1) ? Results = b : Results = a;
			std::string ResultString = std::string{ "The " + std::to_string(Data) + " number in Fibonacci sequence is " } +std::to_string(Results);
			return ResultString;
		}
		else if (Data < 0 || Data > 90)
		{
			std::string ResultString = std::string{ "Wrong range: it should be less than 90" };
			return ResultString;
		}
	}

	virtual void InputProcess() override{
		FibResult = CalcFibonacci(CalcData);
	}
	virtual void Respond() override {
		GetBotId().getApi().sendMessage(GetChatId(), FibResult);
	}
};

//SubDiv task
class TaskSubDiv : public TaskBaseClass
{
private:

	std::string Command;
	int CalcData;

	std::string SubDivResult;
public:

	TaskSubDiv(std::string cmd, int cData, TgBot::Bot& bId, int64_t cId) : TaskBaseClass(bId, cId), Command(cmd), CalcData(cData) {}


	std::string CalcSumDivisibleBy5(long long Data)
	{
		if (Data > 0 && Data <= 10000)
		{
			int sum = 0;
			for (int i = 0; i <= Data; ++i) {
				if (i % 5 == 0) {
					sum += i;
				};
			}

			std::string ResultString = std::string{ "The summation of all numbers that can be divided by 5 without any remainder is " } +std::to_string(sum);
			return ResultString;
		}
		else if (Data < 0 || Data > 10000)
		{
			std::string ResultString = std::string{ "Wrong range: it should be less than 10000" };
			return ResultString;
		}
	}

	virtual void InputProcess() override {
		SubDivResult = CalcSumDivisibleBy5(CalcData);
	}
	virtual void Respond() override {
		GetBotId().getApi().sendMessage(GetChatId(), SubDivResult);
	}
};


// Threadpool

class ThreadPool
{
	//LIST OF TASKS MUTEX AND COND VAR
	std::queue<std::shared_ptr<TaskBaseClass>> QueueOfTasks;

	std::mutex QueueMutex;
	std::condition_variable QueueConditionVar;

	//LIST OF THREADS
	std::vector<std::thread> Threads;


	std::atomic<bool> Quit{ false };


public:
	//CREATION LIST OF THREADS AND RUN THEM
	ThreadPool(int num_threads)
	{
		Threads.reserve(num_threads);
		for (int i = 0; i < num_threads; ++i) {
			Threads.emplace_back(&ThreadPool::RunThread, this);
		}
	};


	//ADD TASK TO Queue
	template <typename Func>
	void AddTask(const Func& TaskFunc)
	{
		std::lock_guard<std::mutex> q_lock(QueueMutex);
		QueueOfTasks.push(TaskFunc);
		QueueConditionVar.notify_one();
	}



	//Destruct
	~ThreadPool()
	{
		Quit = true;
		QueueConditionVar.notify_all();
		for (auto& Thread : Threads) {
			Thread.join();
		}
	}


private:
	//RUN THREAD
	void RunThread()
	{
		while (!Quit)
		{
			std::unique_lock<std::mutex> Lock(QueueMutex);

			QueueConditionVar.wait(Lock, [this]() -> bool { return !QueueOfTasks.empty() || Quit; });

			if (!QueueOfTasks.empty())
			{
				auto Task = QueueOfTasks.front();
				QueueOfTasks.pop();
				Lock.unlock();
				Task->InputProcess();
				Task->Respond();
			}
		}
	};

};



int main() {

	std::vector<std::string> BotCommands = { "start", "find_fibonacci", "subdiv" };

	ThreadPool PoolOfThreads(2);


	TgBot::Bot bot("YOUR TOKEN ID");

	bot.getEvents().onAnyMessage([BotCommands, &bot, &PoolOfThreads](TgBot::Message::Ptr message) {
		for (std::string command : BotCommands) {
			//check if we have command in message 
			if (message->text.find(command) != std::string::npos) {
				std::regex integerRegex("\\d+"); 
				std::smatch match;

				//check if we have int after command
				if (std::regex_search(message->text, match, integerRegex)) {
					int extractedNumber = std::stoi(match[0]);
					std::shared_ptr<TaskBaseClass>MyTask;
					if (command == "find_fibonacci") {
						MyTask = std::make_shared<TaskFibonacci>(command, extractedNumber, bot, message->chat->id);
						PoolOfThreads.AddTask(MyTask);
					}
					else if (command == "subdiv") {
						MyTask = std::make_shared<TaskSubDiv>(command, extractedNumber, bot, message->chat->id);
						PoolOfThreads.AddTask(MyTask);
					}
	
				}
				else {
					bot.getApi().sendMessage(message->chat->id, "No int after command");
				}
			}
		};
	});


	try {
		printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
		TgBot::TgLongPoll longPoll(bot);
		while (true) {
			printf("Current value: %d\n", 0);
			longPoll.start();

		}
	}
	catch (TgBot::TgException& e) {}


	return 0;
}
