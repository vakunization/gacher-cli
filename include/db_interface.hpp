#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <optional>

#include <sqlite_modern_cpp.h>

class DB_interface {
	private:
		std::string path;
		sqlite::database db;
		
	public:
		DB_interface(std::string _path) : path(_path), db(sqlite::database(path)) {
			db << R"(CREATE TABLE IF NOT EXISTS wiki (pageid INT PRIMARY KEY);)";
		}
		
		~DB_interface() {
		}

		int save(std::vector<int>* articles) {
			std::sort(articles->begin(), articles->end());
			articles->erase(std::unique(articles->begin(), articles->end()), articles->end());		
			db <<"BEGIN;";
			auto ps = db << "INSERT OR IGNORE INTO wiki (pageid) VALUES (?)";
			for (int article : *articles){
				ps << article;
				ps.execute();
			}
			db << "COMMIT;";
			return articles->size();
		}
		
		std::optional<int> extract_and_delete() {
			try {
				int pageid;
				db << "BEGIN;";
				db << "SELECT * FROM wiki ORDER BY RANDOM() LIMIT 1" >> pageid;
				db << "DELETE FROM wiki WHERE pageid = ?" << pageid;
				db << "COMMIT;";
				return std::optional<int>(pageid);					
			} catch (const sqlite::sqlite_exception& e) {
				//db << "ROLLBACK;";
				return std::nullopt;	
			}
		}
};
