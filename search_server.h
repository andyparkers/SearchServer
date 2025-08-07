#pragma once

#include "concurrent_map.h"
#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <set>
#include <vector>
#include <list>
#include <utility>
#include <stdexcept>
#include <tuple>
#include <execution>
#include <future>
#include <type_traits>
#include <string_view>

class SearchServer {
public:
	SearchServer(std::string_view stop_words_text);

	SearchServer(const std::string& stop_words_text);

	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words)
		: stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
	{
		if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
			throw std::invalid_argument("Some of stop words are invalid");
		}
	}

	void AddDocument(int document_id, std::string_view, DocumentStatus status, const std::vector<int>& ratings);

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;

	std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

	std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

	template <typename ExecutionPolicy, typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query, DocumentPredicate document_predicate) const;

	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query, DocumentStatus status) const;

	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view, int document_id) const;

	template<class ExecutionPolicy>
	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(ExecutionPolicy&& policy,
		std::string_view raw_query,
		int document_id) const;

	void RemoveDocument(int document_id);

	template<class ExecutionPolicy>
	void RemoveDocument(ExecutionPolicy&& policy, int document_id);

	std::set<int>::iterator begin();

	std::set<int>::iterator end();

	int GetDocumentCount() const;

	const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

private:
	const static int MAX_RESULT_DOCUMENT_COUNT = 5;

	struct DocumentData {
		std::set<std::string> content;
		int rating;
		DocumentStatus status;
	};

	struct Query {
		std::set<std::string_view> plus_words;
		std::set<std::string_view> minus_words;
	};

	struct QueryWord {
		std::string_view data;
		bool is_minus;
		bool is_stop;
	};

	const std::set<std::string, std::less<>> stop_words_;
	std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
	std::map<int, DocumentData> documents_;
	std::set<int> document_ids_;
	std::map<int, std::map<std::string_view, double>> word_frequencies_to_document;

	bool IsStopWord(std::string_view word) const;

	static bool IsValidWord(std::string_view word);

	std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

	Query ParseQuery(std::string_view text) const;

	QueryWord ParseQueryWord(std::string_view text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

	double ComputeWordInverseDocumentFreq(const std::string& word) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const;
};

template<class ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(ExecutionPolicy&& policy,
	std::string_view raw_query,
	int document_id) const {
	const auto& query = ParseQuery(raw_query);
	std::vector<std::string_view> matched_words;
	matched_words.reserve(raw_query.length());
	if (std::any_of(std::execution::seq,
		query.minus_words.begin(),
		query.minus_words.end(),
		[&](auto word) { return word_to_document_freqs_.at(word).count(document_id); }
	)) {
		return { {}, documents_.at(document_id).status };
	}
	std::copy_if(std::execution::seq,
		query.plus_words.begin(),
		query.plus_words.end(),
		std::back_inserter(matched_words),
		[&](auto word) { return word_to_document_freqs_.at(word).count(document_id); }
	);
	matched_words.shrink_to_fit();
	return { matched_words, documents_.at(document_id).status };
}

template< class ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {
	auto it = document_ids_.find(document_id);
	if (it == document_ids_.end()) {
		return;
	}

	auto& items = word_frequencies_to_document.at(document_id);

	std::vector<const std::string_view*> words(items.size());
	std::transform(policy, items.begin(), items.end(), words.begin(), [](auto& p) { return &p.first; });

	std::for_each(policy, words.begin(), words.end(),
		[&](auto word) {
			word_to_document_freqs_.at(*word).erase(document_id);
		}
	);

	document_ids_.erase(document_id);
	documents_.erase(document_id);
	word_frequencies_to_document.erase(document_id);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query, DocumentPredicate document_predicate) const {
	const auto query = ParseQuery(raw_query);
	auto matched_documents = FindAllDocuments(policy, query, document_predicate);

	sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
		if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
			return lhs.rating > rhs.rating;
		}
		else {
			return lhs.relevance > rhs.relevance;
		}
		});
	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}

	return matched_documents;
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query, DocumentStatus status) const {
	return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
		return document_status == status;
		});
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
	return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query) const {
	return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const {
	return FindAllDocuments(query, document_predicate);
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
	std::map<int, double> document_to_relevance;
	for (std::string_view word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		const double inverse_document_freq = ComputeWordInverseDocumentFreq(std::string(word));
		for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
			const auto& document_data = documents_.at(document_id);
			if (document_predicate(document_id, document_data.status, document_data.rating)) {
				document_to_relevance[document_id] += term_freq * inverse_document_freq;
			}
		}
	}

	for (std::string_view word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
			document_to_relevance.erase(document_id);
		}
	}

	std::vector<Document> matched_documents;
	for (const auto [document_id, relevance] : document_to_relevance) {
		matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
	}
	return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const {
	ConcurrentMap<int, double> document_to_relevance(10000);
	{
		static constexpr int PART_COUNT = 16;
		const auto part_length = query.plus_words.size() / PART_COUNT;
		auto part_begin = query.plus_words.begin();
		auto part_end = std::next(part_begin, part_length);

		auto function = [&](std::string_view word) {
			if (word_to_document_freqs_.count(word) != 0) {
				const double inverse_document_freq = ComputeWordInverseDocumentFreq(std::string(word));
				for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
					const auto& document_data = documents_.at(document_id);
					if (document_predicate(document_id, document_data.status, document_data.rating)) {
						document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
					}
				}
			}
		};

		std::vector<std::future<void>> futures;
		for (int i = 0;	i < PART_COUNT;	++i,
			part_begin = part_end, part_end = (i == PART_COUNT - 1 ? query.plus_words.end() : next(part_begin, part_length))) {
			futures.push_back(std::async([=] {
				std::for_each(std::execution::par, part_begin, part_end, function);
				}));
		}
	}

	{
		static constexpr int PART_COUNT = 8;
		const auto part_length = query.minus_words.size() / PART_COUNT;
		auto part_begin = query.minus_words.begin();
		auto part_end = std::next(part_begin, part_length);

		auto function = [&](std::string_view word) {
			if (word_to_document_freqs_.count(word) > 0) {
				for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
					document_to_relevance.erase(document_id);
				}
			}
		};

		std::vector<std::future<void>> futures;
		for (int i = 0;	i < PART_COUNT;	++i,
			part_begin = part_end, part_end = (i == PART_COUNT - 1 ? query.minus_words.end() : next(part_begin, part_length))) {
			futures.push_back(std::async([=] {
				std::for_each(part_begin, part_end, function);
				}));
		}
	}

	std::vector<Document> matched_documents;
	for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
		matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
	}

	return matched_documents;
}
