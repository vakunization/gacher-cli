#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <queue>

#include <thread>
#include <mutex>
#include <condition_variable>

#include <curl/curl.h>
#include <json.hpp>
#include <parser.hpp>
#include <db_interface.hpp>

#define NTHREADS 8

static const std::string help_msg = 
R"(Too many or too few arguments.
Usage: gacher [options]...
Options:
 --crawl		Collect all articles from specified categories into a database.
 --get			Get a random article from database.
Before using, specify the categories you are interested in the \"config.json\" file in the \"categories\" field.
If necessary, you can change the database name created by the CLI in the \"db-name\" field.)";

struct Task_queue {
	std::queue<Category> queue;
	std::mutex mtx;
	std::condition_variable cv;
	bool finished = true;
	int active_tasks = 0;
};

struct Params_for_parser {
	int depth;
	std::string user_agent;
};

void worker(Task_queue& q, const Params_for_parser& params, std::vector<int>* out) {
	Parser p (params.depth, params.user_agent);
	std::vector<int> visited_articles;
	while (true) {
		Category category ("");
		
		std::unique_lock<std::mutex> lock(q.mtx);
		q.cv.wait(lock, [&] {return !q.queue.empty() || q.finished;});
		if (q.queue.empty() && q.finished && q.active_tasks == 0) break;
		
		if (!q.queue.empty()){
			category = q.queue.front();
			q.queue.pop();
			q.active_tasks++;
		}
		lock.unlock();

		if (category.depth == 0)
			std::cout << "Parsing: " << category.name << std::endl;

		auto [articles, subcats] = p.parse_category(category);
		
		visited_articles.insert(visited_articles.end(), articles.begin(), articles.end());

		lock.lock();
		for (const auto& item : subcats)
			q.queue.push(item);
		q.active_tasks--;
		lock.unlock();

		q.cv.notify_all();
	}
	out->insert(out->end(), visited_articles.begin(), visited_articles.end());
	return;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cout << help_msg << std::endl;
		return 1;
	}
	std::fstream config_file ("config.json", std::ios::in);
	if (config_file.is_open()) {
		nlohmann::json config;
		config_file >> config;
		if (std::string_view(argv[1]) == "--crawl") {
			Task_queue q;
			
			for (const auto& item : config["categories"])
				q.queue.emplace(item.get<std::string>());
			if (!q.queue.empty()) 
				q.finished = false;

			const Params_for_parser p {config["max-depth"].get<int>(), config["user-agent"].get<std::string>()};
			std::vector<std::vector<int>> result (NTHREADS);
			CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
			if (r != CURLE_OK) {
				std::cerr << curl_easy_strerror(r) << std::endl;
				return 1;
			}
			
			std::vector<std::thread> threads;
			for (int i = 0; i != NTHREADS; ++i)
				threads.emplace_back(worker, std::ref(q), std::cref(p), &result[i]);

			{
				std::unique_lock<std::mutex> lock(q.mtx);
				q.cv.wait(lock, [&]{return q.queue.empty() && q.active_tasks == 0;});
				q.finished = true;
			}
			q.cv.notify_all();

			for (auto& t : threads)
				t.join();

			curl_global_cleanup();

			bool something_found = false;
			int counter = 0;
			DB_interface db (config["db-name"].get<std::string>());
			for (auto& item : result) {
				if (!item.empty()){
					something_found = true;
					counter += db.save(&item);
				}
			}
			if (something_found) {
				std::cout << "All done." << std::endl
				<< "Find "<< counter << " pages. Try --get flag for for get random article." << std::endl;
			} else {
				std::cerr << "Something went wrong..." << std::endl;
			}

			return 0;
		} else if (std::string_view(argv[1]) == "--get") {
			DB_interface db (config["db-name"].get<std::string>());
			auto id = db.extract_and_delete();
			if (id) {
				std::cout << "https://en.wikipedia.org/wiki/?curid=" << *id << std::endl;
			} else {
				std::cerr << "Database is empty or not exist." << std::endl;
			}
			return 0;
		}
	} else {
		std::cerr << "Cant open \"config.json\" file." << std::endl;
		return 0;
	}
}
