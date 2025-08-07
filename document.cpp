#include "document.h"

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating)
{

}

std::ostream& operator<<(std::ostream & out, const Document & document) {
    out << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating << " }";
    return out;
}

bool operator==(const Document & lhs, const Document & rhs) {
    return lhs.id == rhs.id && (std::abs(lhs.relevance - rhs.relevance) < 1e-6) && lhs.rating == rhs.rating;
}