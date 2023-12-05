#include <tgbot/tgbot.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <atomic>
#include <queue>
#include <unordered_set>
#include <thread>
#include <chrono>


using namespace TgBot;
using namespace std;

vector<string> bot_commands{"start", "find_fibonacci", "subdiv_5"};


void CalcFibonacci(long long& Data, long long& Results)
{
	long long a = 1, b = 1;

	for (int i = 2; i <= Data; ++i) {
		long long temp = a + b;
		a = b;
		b = temp;
	}

	(Results == 1) ? Results = b : Results = a;
}

void CalcSumDivisibleBy5(long long &Data, long long& Results)
{

	int sum = 0;
	for (int i = 0; i <= Data; ++i) {
		if (i % 5 == 0) {
			sum += i;
		};
	}
	Results = sum;
}

// Threadpool

class thread_pool
{
	//LIST OF TASKS MUTEX AND COND VAR
	std::queue<std::pair<std::future<void>, int>> QueueOfTasks;

	std::mutex Queue_mutex;
	std::condition_variable Queue_ConditionVar;

	//LIST OF COMPLETED TASKS MUTEX AND COND VAR
	std::unordered_set<int> Completed_Tasks;

	std::condition_variable Completed_Tasks_cv;
	std::mutex Completed_Tasks_mutex;

	//LIST OF THREADS
	vector<std::thread> threads;


	std::atomic<bool> quite{ false };

	//LAST INDEX
	std::atomic<int> last_idx = 0;

public:
	//CREATION LIST OF THREADS AND RUN THEM
	thread_pool(int num_threads)
	{
		threads.reserve(num_threads);
		for (int i = 0; i < num_threads; ++i) {
			threads.emplace_back(&thread_pool::run, this);
		}
	};


	//ADD TASK TO Queue
	template <typename Func, typename... Args>
	int add_task(const Func& task_func, Args&&... args)
	{
		int task_idx = last_idx++;
		std::lock_guard<std::mutex> q_lock(Queue_mutex);
		QueueOfTasks.emplace(std::async(std::launch::deferred, task_func, args...), task_idx);
		Queue_ConditionVar.notify_one();
		return task_idx;
	}

	void wait(int task_id)
	{
		std::unique_lock<std::mutex> lock(Completed_Tasks_mutex);
		Completed_Tasks_cv.wait(lock, [this, task_id]() -> bool {
			return Completed_Tasks.find(task_id) != Completed_Tasks.end();
		});
	}

	void wait_all()
	{
		std::unique_lock<std::mutex> lock(Queue_mutex);
		Completed_Tasks_cv.wait(lock, [this]() -> bool {
			std::lock_guard<std::mutex> task_lock(Completed_Tasks_mutex);
			return QueueOfTasks.empty() && last_idx == Completed_Tasks.size();
		});
	}

	bool calculated(int task_id)
	{
		std::lock_guard<std::mutex> lock(Completed_Tasks_mutex);
		return Completed_Tasks.find(task_id) != Completed_Tasks.end();
	}

	//Destruct
	~thread_pool()
	{
		quite = true;
		Queue_ConditionVar.notify_all();
		for (auto& thread : threads) {
			thread.join();
		}
	}


	private:
	//RUN THREAD
	void run()
	{
		while (!quite) {
			std::unique_lock<std::mutex> lock(Queue_mutex);

			// wait if queue of tasks is empty
			Queue_ConditionVar.wait(lock, [this]()->bool { return !QueueOfTasks.empty() || quite; });

			if (!QueueOfTasks.empty()) {
				auto elem = std::move(QueueOfTasks.front());
				QueueOfTasks.pop();

				elem.first.get();

				std::lock_guard<std::mutex> lock(Completed_Tasks_mutex);


				Completed_Tasks.insert(elem.second);

				Completed_Tasks_cv.notify_all();
			}
		}
	};

};

int main() {
	thread_pool poolOfThreads(4);
	std::atomic<bool> NonCommandMessagesSubdiv{ false };
	std::atomic<bool> NonCommandMessagesFibonacci{ false };
	


	Bot bot("ENTER YOUR TOKEN");


	
	int64_t ChatID;
	

	bot.getEvents().onCommand("start", [&bot, &ChatID](Message::Ptr message) {
		bot.getApi().sendMessage(message->chat->id, "Hi " + message->chat->firstName);
	});
	bot.getEvents().onCommand("subdiv_5", [&bot, &NonCommandMessagesSubdiv, &poolOfThreads](Message::Ptr message) {
		bot.getApi().sendMessage(message->chat->id, "Write a number from 1 to 10000");

		// Set the flag to allow processing of non-command messages
		NonCommandMessagesSubdiv.store(true);

		bot.getEvents().onNonCommandMessage([&bot, &NonCommandMessagesSubdiv, &poolOfThreads](Message::Ptr message) {
			if (NonCommandMessagesSubdiv.load()) {
				long long number = std::stoi(message->text);
				if (number > 0 && number <= 10000) {
					long long results = 0;
					poolOfThreads.add_task(CalcSumDivisibleBy5, std::ref(number), std::ref(results));
					poolOfThreads.wait_all();
					bot.getApi().sendMessage(message->chat->id, "The summation of all numbers that can be divided by 5 without any remainder is " + to_string(results));
					// Reset the flag after processing the message
					NonCommandMessagesSubdiv.store(false);
				}
				else if (number < 0 || number > 10000) {
					bot.getApi().sendMessage(message->chat->id, "Wrong range");
					// Reset the flag after processing the message
					NonCommandMessagesSubdiv.store(false);
				}
			}
		});
	});
	bot.getEvents().onCommand("find_fibonacci", [&bot, &NonCommandMessagesFibonacci, &poolOfThreads](Message::Ptr message) {
		bot.getApi().sendMessage(message->chat->id, "Write a number from 1 to 90");

		// Set the flag to allow processing of non-command messages
		NonCommandMessagesFibonacci.store(true);

		bot.getEvents().onNonCommandMessage([&bot, &NonCommandMessagesFibonacci , &poolOfThreads](Message::Ptr message) {
			if (NonCommandMessagesFibonacci.load()) {
				long long number = std::stoi(message->text);
				if (number > 0 && number <= 90) {
					long long results = 0;
					poolOfThreads.add_task(CalcFibonacci, std::ref(number), std::ref(results));
					poolOfThreads.wait_all();
					bot.getApi().sendMessage(message->chat->id, "The " + to_string(number) + " number in Fibonacci sequence is " + to_string(results));
					// Reset the flag after processing the message
					NonCommandMessagesFibonacci.store(false);
				}
				else if (number < 0 || number > 90) {
					bot.getApi().sendMessage(message->chat->id, "Wrong range");
					// Reset the flag after processing the message
					NonCommandMessagesFibonacci.store(false);
				}
			}
		});
	});
	
	
	



	try {
		printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
		TgLongPoll longPoll(bot);
		while (true) {
			printf("Current value: %d\n", 0);
			longPoll.start();

		}
	}
	catch (TgException& e) {
		printf("error: %s\n", e.what());
	}
	
	poolOfThreads.wait_all();

	return 0;
}