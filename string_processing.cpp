#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    words.reserve(text.length() / 5);
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(std::move(word));
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(std::move(word));
    }

    return words;
}

std::set<std::string_view> SplitIntoWordsView(std::string_view str) {
    std::set<std::string_view> result;
    while (true) {
        size_t space = str.find(' ');
        result.insert(str.substr(0, space));
        if (space == str.npos) {
            break;
        }
        str.remove_prefix(space + 1);
    }
    return result;
}