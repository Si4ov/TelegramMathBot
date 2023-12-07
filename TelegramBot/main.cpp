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

		(Results == 1) ? Results = b : Results = a;
		std::string resultString = std::string{ "The " + std::to_string(Data) + " number in Fibonacci sequence is " } + std::to_string(Results);
		return resultString;
	}
	else if (Data < 0 || Data > 90)
	{
		std::string resultString = std::string{ "Wrong range: it should be less than 90" };
		return resultString;
	}
}

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

		std::string resultString = std::string{ "The summation of all numbers that can be divided by 5 without any remainder is "} + std::to_string(sum);
		return resultString;
	}
	else if (Data < 0 || Data > 10000)
	{
		std::string resultString = std::string{ "Wrong range: it should be less than 10000" };
		return resultString;
	}
}


class TaskClass
{

private:
	std::vector<std::string> BotCommands = { "start", "find_fibonacci", "subdiv" };

public:

	std::string Command;
	long long CalcData;

	TaskClass(  std::string& cmd, int cData ) : Command(cmd), CalcData(cData){}


	std::string operator()()
	{
		std::string result;

		if (Command == BotCommands[1])
		{
			result = CalcFibonacci(CalcData);

			return result;

		}
		else if (Command == BotCommands[2])
		{
			result = CalcSumDivisibleBy5(CalcData);

			return result;
		}
		return result;
	}
};


// Threadpool

class ThreadPool
{
	//LIST OF TASKS MUTEX AND COND VAR
	std::queue<std::shared_ptr<TaskClass>> QueueOfTasks;

	std::mutex Queue_mutex;
	std::condition_variable Queue_ConditionVar;

	//LIST OF THREADS
	std::vector<std::thread> Threads;


	std::atomic<bool> Quite{ false };


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
	std::string AddTask(const Func& task_func)
	{
		std::lock_guard<std::mutex> q_lock(Queue_mutex);
		auto task = std::make_shared<TaskClass>(task_func);
		QueueOfTasks.push(task);
		Queue_ConditionVar.notify_one();

		return task->operator()();
	}



	//Destruct
	~ThreadPool()
	{
		Quite = true;
		Queue_ConditionVar.notify_all();
		for (auto& thread : Threads) {
			thread.join();
		}
	}


private:
	//RUN THREAD
	void RunThread()
	{
		while (!Quite)
		{
			std::unique_lock<std::mutex> lock(Queue_mutex);

			Queue_ConditionVar.wait(lock, [this]() -> bool { return !QueueOfTasks.empty() || Quite; });

			if (!QueueOfTasks.empty())
			{
				auto task = std::move(QueueOfTasks.front());
				QueueOfTasks.pop();
				lock.unlock();
				(*task)();
			}
		}
	};

};



int main() {

	std::vector<std::string> BotCommands = { "start", "find_fibonacci", "subdiv" };

	ThreadPool poolOfThreads(2);


	TgBot::Bot bot("YOUR TOKEN ID");

	bot.getEvents().onAnyMessage([BotCommands, &bot, &poolOfThreads](TgBot::Message::Ptr message) {
		for (std::string command : BotCommands) {
			//check if we have command in message 
			if (message->text.find(command) != std::string::npos) {
				std::regex integerRegex("\\d+"); 
				std::smatch match;

				//check if we have int after command
				if (std::regex_search(message->text, match, integerRegex)) {
					int extractedNumber = std::stoi(match[0]);
					TaskClass MyTask(command, extractedNumber);
					//push task to pool and save result executed on () operator
					std::string result = poolOfThreads.AddTask(MyTask);

					bot.getApi().sendMessage(message->chat->id, result);
					
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
