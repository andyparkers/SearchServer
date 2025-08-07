#pragma once

#include <string>
#include <iostream>
#include <cmath>

struct Document {
    Document();

    Document(int id, double relevance, int rating);

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

std::ostream& operator<<(std::ostream& out, const Document& document);

bool operator==(const Document& lhs, const Document& rhs);