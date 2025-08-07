#include "search_server.h"

SearchServer::SearchServer(std::string_view stop_words_text)
	: SearchServer(SplitIntoWords(std::string(stop_words_text)))
{

}

SearchServer::SearchServer(const std::string& stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text))
{

}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
	if ((document_id < 0) || (documents_.count(document_id) > 0)) {
		throw std::invalid_argument("Invalid document_id");
	}
	const auto& words = SplitIntoWordsNoStop(std::string{ document });
	const double inv_word_count = 1.0 / words.size();
	// std::cout << inv_word_count << std::endl;
	documents_.emplace(document_id, DocumentData{ std::set(words.begin(), words.end()), SearchServer::ComputeAverageRating(ratings), status });
	for (const std::string& word : words) {
		std::string_view str(*documents_[document_id].content.find(word));
		word_to_document_freqs_[str][document_id] += inv_word_count;
		word_frequencies_to_document[document_id][str] += inv_word_count;
	}
	document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
	return FindTopDocuments(raw_query,
		[status](int document_id, DocumentStatus document_status, int rating)
		{
			return document_status == status;
		});
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
	return MatchDocument(std::execution::seq, raw_query, document_id);
}


void SearchServer::RemoveDocument(int document_id) {
	if (word_frequencies_to_document.count(document_id) == 0) {
		return;
	}
	document_ids_.erase(document_id);
	documents_.erase(document_id);
	for (const auto& [word, frequency] : word_frequencies_to_document.at(document_id)) {
		word_to_document_freqs_.at(word).erase(document_id);
	}
	word_frequencies_to_document.erase(document_id);
}

std::set<int>::iterator SearchServer::begin() {
	return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end() {
	return document_ids_.end();
}

int SearchServer::GetDocumentCount() const {
	return static_cast<int>(documents_.size());
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
	static std::map<std::string_view, double> result;
	auto document_id_position = find(document_ids_.begin(), document_ids_.end(), document_id);
	if (document_id_position == document_ids_.end()) {
		return result;
	}
	return word_frequencies_to_document.at(document_id);
}

bool SearchServer::IsStopWord(std::string_view word) const {
	return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
	return std::none_of(word.begin(), word.end(), [](char c) {
		return c >= '\0' && c < ' ';
		});
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
	std::vector<std::string> words;
	for (const std::string& word : SplitIntoWords(text)) {
		if (!IsValidWord(word)) {
			throw std::invalid_argument("Word " + word + " is invalid");
		}
		if (!IsStopWord(word)) {
			words.push_back(word);
		}
	}
	return words;
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
	Query result;
	for (std::string_view word : SplitIntoWordsView(text)) {
		const auto& query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			if (query_word.is_minus) {
				result.minus_words.insert(query_word.data);
			}
			else {
				result.plus_words.insert(query_word.data);
			}
		}
	}
	return result;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
	if (text.empty()) {
		throw std::invalid_argument("Query word is empty");
	}
	std::string_view word = text;
	bool is_minus = false;
	if (word[0] == '-') {
		is_minus = true;
		word = word.substr(1);
	}
	if (word.empty() || word[0] == '-' || !IsValidWord(std::string{ word })) {
		std::string error_text(word);
		throw std::invalid_argument("Query word " + error_text + " is invalid");
	}

	return { word, is_minus, IsStopWord(std::string { word }) };
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
	if (ratings.empty()) {
		return 0;
	}
	int rating_sum = 0;
	for (const int rating : ratings) {
		rating_sum += rating;
	}
	return rating_sum / static_cast<int>(ratings.size());
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
