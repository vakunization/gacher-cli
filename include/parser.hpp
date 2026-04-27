#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <utility>

#include <thread>
#include <chrono>
#include <algorithm>

#include <curl/curl.h>
#include "json.hpp"

struct Category {
	std::string name;
	int depth;
	
	Category(const std::string& _name, const int _depth = 0) : name(_name), depth(_depth) {
		std::replace(name.begin(), name.end(), ' ', '_');
	}
	/*Category& operator= = default; /*(const Category& r) {
		name = r.name;
		depth = r.depth;
		return *this;
	}*/
};

class Parser {
	private:
		const int crawl_depth;
		CURL* curl;
		const std::string user_agent;
		std::string buffer; // buffer for curl
		struct curl_slist *headers = nullptr; 

	public:	
		Parser(int _crawl_depth, std::string _user_agent) :
												crawl_depth(_crawl_depth),
												curl(nullptr),
												user_agent(_user_agent),
												headers(nullptr) {
			curl = curl_easy_init();
			if (!curl) throw std::runtime_error("Faled CURL initialization");

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);//
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

			headers = curl_slist_append(headers, user_agent.c_str());
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}

		~Parser() {
		    if (curl) curl_easy_cleanup(curl);
		    curl_slist_free_all(headers);
		}
		
	private:
		static size_t writer(char *data, size_t size, size_t nmemb, std::string *writer_data) {
			if (writer_data == NULL) return 0;
			writer_data->append(data, size * nmemb);
			return size * nmemb;
		} //callback for libcurl
		
		std::string build_url (const Category& c, const std::string& continue_token) {
			std::string url = "https://en.wikipedia.org/w/api.php?action=query&list=categorymembers&cmtitle=" + c.name + "&cmlimit=max&format=json";
			if (!continue_token.empty()) url += "&cmcontinue=" + continue_token;
			return url;
		}
				
		std::optional<nlohmann::json> request(const Category& cat, const std::string& continue_token = "") {
			std::string url = build_url(cat, continue_token);
			
			buffer.clear();
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			
			for (int c = 1; c <= 5; ++c) {
				if (curl_easy_perform(curl) == CURLE_OK && !buffer.empty()) {
					try {
						nlohmann::json j = nlohmann::json::parse(buffer);
						if (!j.contains("error")) return std::make_optional(j);
						else std::this_thread::sleep_for(std::chrono::seconds(5));
					}
					catch (const nlohmann::json::parse_error& e) {
						std::cerr << "Warning: JSON parse error: " << e.what() << std::endl;
						break;
					}

				} else std::this_thread::sleep_for(std::chrono::seconds(c));
			}	
			return std::nullopt;
		}
		
	public:
		std::pair<std::vector<int>, std::vector<Category>> parse_category(Category& cat) {
			std::vector<int> visited_articles;
			std::vector<Category> visited_categories;
			
			if (cat.depth > crawl_depth || cat.name == "") 
				return {visited_articles, visited_categories};

			auto j = request(cat);
			if (!j) return {visited_articles, visited_categories};

			visited_articles.reserve((*j)["query"]["categorymembers"].size());
			while (true) {
				const auto& cref = *j;
				for (const auto& item : cref["query"]["categorymembers"]) {
					if (item["ns"] == 0) {
						visited_articles.push_back(item["pageid"].get<int>());
					} else if (item["ns"] == 14) {
						Category subcat {item["title"].get<std::string>(), cat.depth + 1};
						visited_categories.push_back(subcat);
					}
				}
				if (cref.contains("continue") && cref["continue"].contains("cmcontinue")) {
					std::string continue_token = cref["continue"]["cmcontinue"];
					j = request(cat, continue_token);
					if (!j) return {visited_articles, visited_categories};
				} else break;
			}
			return {visited_articles, visited_categories};
		}
};
