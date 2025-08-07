#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
	const SearchServer& search_server,
	const std::vector<std::string>& queries) {
	std::vector<std::vector<Document>> result(queries.size());
	transform(
		std::execution::par,
		queries.begin(),
		queries.end(),
		result.begin(),
		[&search_server](const std::string& query) { return search_server.FindTopDocuments(query); }
	);
	return result;
}

std::vector<Document> ProcessQueriesJoined(
	const SearchServer& search_server,
	const std::vector<std::string>& queries) {
	std::vector<Document> result;
	result.reserve(queries.size());
	for (const auto& vec : ProcessQueries(search_server, queries)) {
		for (const auto& document : vec) {
			result.push_back(std::move(document));
		}
	}
	return result;
}