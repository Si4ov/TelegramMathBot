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




std::string CalcFibonacci(long long& Data, long long& Results)
{
	long long a = 1, b = 1;

	for (int i = 2; i <= Data; ++i) {
		long long temp = a + b;
		a = b;
		b = temp;
	}

	(Results == 1) ? Results = b : Results = a;
	std::string resultString = { "The " + std::to_string(Data) + " number in Fibonacci sequence is " };
	return resultString;
}

std::string CalcSumDivisibleBy5(long long &Data, long long& Results)
{

	int sum = 0;
	for (int i = 0; i <= Data; ++i) {
		if (i % 5 == 0) {
			sum += i;
		};
	}
	Results = sum;
	std::string resultString = { "The summation of all numbers that can be divided by 5 without any remainder is " };
	return resultString;
}

// Threadpool

class ThreadPool
{
	//LIST OF TASKS MUTEX AND COND VAR
	std::queue<std::pair<std::future<void>, int>> QueueOfTasks;

	std::mutex Queue_mutex;
	std::condition_variable Queue_ConditionVar;

	//LIST OF THREADS
	std::vector<std::thread> Threads;


	std::atomic<bool> Quite{ false };

	//LAST INDEX
	std::atomic<int> last_idx = 0;

public:
	//CREATION LIST OF THREADS AND RUN THEM
	ThreadPool(int num_threads)
	{
		Threads.reserve(num_threads);
		for (int i = 0; i < num_threads; ++i) {
			threads.emplace_back(&ThreadPool::RunThread, this);
		}
	};


	//ADD TASK TO Queue
	template <typename Func, typename... Args>
	void AddTask(const Func& task_func, Args&... args)
	{
		int task_idx = last_idx++;
		std::lock_guard<std::mutex> q_lock(Queue_mutex);
		QueueOfTasks.emplace(std::async(std::launch::deferred, task_func, args...), task_idx);
		Queue_ConditionVar.notify_one();

	}



	//Destruct
	~ThreadPool()
	{
		Quite = true;
		Queue_ConditionVar.notify_all();
		for (auto& thread : threads) {
			thread.join();
		}
	}


private:
	//RUN THREAD
	void RunThread()
	{
		while (!quite) {
			std::unique_lock<std::mutex> lock(Queue_mutex);

			// wait if queue of tasks is empty
			Queue_ConditionVar.wait(lock, [this]()->bool { return !QueueOfTasks.empty() || quite; });

			if(!QueueOfTasks.empty()) {
				auto elem = std::move(QueueOfTasks.front());
				QueueOfTasks.pop();

				elem.first.get();
			}
		}
	};

};

template <typename Callable>
void BotEvent(TgBot::Bot& DataBot, ThreadPool& poolOfThreadsData, Callable& CalculatedMethod , int range, bool& isProcessing)
{
	DataBot.getEvents().onNonCommandMessage([&DataBot, &poolOfThreadsData, &range, CalculatedMethod, &isProcessing](TgBot::Message::Ptr message) {
		std::this_thread::sleep_for(std::chrono::seconds(5));
		if (isProcessing)
		{
			DataBot.getApi().sendMessage(message->chat->id, "Range = " + std::to_string(range));
			long long number = std::stoi(message->text);
			DataBot.getApi().sendMessage(message->chat->id, "Calculate...");
			if (number > 0 && number <= range) {
				long long results = 0;

				poolOfThreadsData.AddTask(CalculatedMethod, number, results);

				DataBot.getApi().sendMessage(message->chat->id, "Result: " + std::to_string(results));
				isProcessing = false;
			}
			else if (number < 0 || number > range) {
				DataBot.getApi().sendMessage(message->chat->id, "Wrong range: "+ std::to_string(range));
				isProcessing = false;
			}
		}
	});
	
}

int main() {

	std::vector<std::string> bot_commands = { "start", "find_fibonacci", "subdiv_5" };

	ThreadPool poolOfThreads(2);


	TgBot::Bot bot("YOUR TOKEN ID");

	bool Controller = true;

	bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
		bot.getApi().sendMessage(message->chat->id, "Hi " + message->chat->firstName);
	});
	bot.getEvents().onCommand("subdiv_5", [&bot, &poolOfThreads, &Controller](TgBot::Message::Ptr message) {
		bot.getApi().sendMessage(message->chat->id, "Write a number from 1 to 10000");

		//Call event
		BotEvent(bot, poolOfThreads, CalcSumDivisibleBy5, 1000, Controller);
		Controller = true;
		
	});
	bot.getEvents().onCommand("find_fibonacci", [&bot, &poolOfThreads, &Controller](TgBot::Message::Ptr message) {
		bot.getApi().sendMessage(message->chat->id, "Write a number from 1 to 90");

		//Call event
		BotEvent(bot, poolOfThreads, CalcFibonacci, 90 , Controller);
		Controller = true;
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
