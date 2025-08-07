#pragma once

#include "search_server.h"
#include <deque>

class RequestQueue {
public:
	RequestQueue(const SearchServer& search_server);

	template <typename DocumentPredicate>
	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

	std::vector<Document> AddFindRequest(const std::string& raw_query);

	int GetNoResultRequests() const;
private:
	struct QueryResult {
		QueryResult(bool s)
			: is_empty(s)
		{

		}
		bool is_empty;
	};
	const SearchServer& search_server_;
	std::deque<QueryResult> requests_;
	const static int sec_in_day_ = 1440;
	int current_time_ = 0;
	int empty_requests_count_ = 0;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
	++current_time_;
	if (current_time_ > sec_in_day_) {
		if (requests_.front().is_empty) {
			--empty_requests_count_;
		}
		requests_.pop_front();
	}
	const std::vector<Document>& result = search_server_.FindTopDocuments(raw_query);
	if (result.empty()) {
		++empty_requests_count_;
	}
	requests_.emplace_back(result.empty());
	return result;
}