#pragma once

#include <iostream>
#include <vector>
#include <iterator>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
        : first_(begin)
        , last_(end)
        , size_(distance(first_, last_)) {
    }

    Iterator begin() const {
        return first_;
    }

    Iterator end() const {
        return last_;
    }

    size_t size() const {
        return size_;
    }

private:
    Iterator first_, last_;
    size_t size_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range) {
    for (Iterator it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}

template<typename Iterator>
class Paginator {
public:
    Paginator(Iterator first, Iterator second, size_t page_size) {
        size_t total_documents_number = distance(first, second);
        for (size_t i = 0; i < total_documents_number / page_size; ++i) {
            pag_.emplace_back(first, first + page_size);
            advance(first, page_size);
        }
        if (distance(first, second) != 0) {
            pag_.emplace_back(first, second);
        }
    }

    auto begin() const {
        return pag_.begin();
    }

    auto end() const {
        return pag_.end();
    }

    size_t size() {
        return distance(pag_.begin(), pag_.end());
    }

private:
    std::vector<IteratorRange<Iterator>> pag_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}