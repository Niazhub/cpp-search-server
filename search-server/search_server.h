#pragma once
#include <algorithm>
#include <cmath>
#include <numeric>
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <cstdint>
#include <iterator>
#include <execution>
#include <type_traits>
#include <mutex>
#include <future>

#include "concurrent_map.h"
#include "string_processing.h"
#include "read_input_functions.h"
#include "document.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double EPSILON = 1e-6;

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    SearchServer();

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const string_view& stop_words_text);

    explicit SearchServer(const string& stop_words_text);

    void AddDocument(int document_id, const string_view& document, DocumentStatus status, const vector<int>& ratings);

    vector<Document> FindTopDocuments(const string_view& raw_query, DocumentStatus status) const;

    vector<Document> FindTopDocuments(const string_view& raw_query) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const string_view& raw_query, DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const string_view& raw_query, DocumentStatus status) const;

    template <typename ExecutionPolicy>
    vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const string_view& raw_query) const;


    int GetDocumentCount() const;

    set<int>::const_iterator begin() const;

    set<int>::const_iterator end() const;

    const map<string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);

    void RemoveDocument(const std::execution::parallel_policy&, int document_id);

    tuple<vector<string_view>, DocumentStatus> MatchDocument(const string_view& raw_query, int document_id) const;

    tuple<vector<string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, const string_view& raw_query, int document_id) const;

    tuple<vector<string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, const string_view& raw_query, int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        map<string_view, double> freqs;
    };

    deque<string> bufer;

    set<string, less<>> stop_words_;

    map<string_view, map<int, double>> word_to_document_freqs_;

    map<int, DocumentData> documents_;

    set<int> document_ids_;

    bool IsStopWord(const string_view& word) const;

    static bool IsValidWord(const string_view& word);

    vector<string_view> SplitIntoWordsNoStop(const string_view& text) const;

    static int ComputeAverageRating(const vector<int>& ratings);

    struct QueryWord {
        string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const string_view& text) const;

    struct Query {
        vector<string_view> plus_words;
        vector<string_view> minus_words;
    };

    Query ParseQuery(const string_view& text, bool is_sort) const;

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(execution::sequenced_policy, const Query& query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(execution::parallel_policy, const Query& query, DocumentPredicate document_predicate) const;

    double ComputeWordInverseDocumentFreq(const string_view& word) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename ExecutionPolicy, typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const string_view& raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query, true);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
        [&](const Document& lhs, const Document& rhs) {
            return lhs.relevance > rhs.relevance
                || (abs(lhs.relevance - rhs.relevance) < EPSILON && lhs.rating > rhs.rating);
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename ExecutionPolicy>
vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy & policy, const string_view & raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
}

template <typename ExecutionPolicy>
vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const string_view& raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
vector<Document> SearchServer::FindAllDocuments(execution::sequenced_policy, const Query& query, DocumentPredicate document_predicate) const {
    map<int, double> document_to_relevance;
    for (const string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (const string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate>
vector<Document> SearchServer::FindAllDocuments(execution::parallel_policy, const Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(query.plus_words.size());

    for_each(execution::par, query.plus_words.begin(), query.plus_words.end(),
        [&](const string_view& word) {
            if (word_to_document_freqs_.count(word) == 0) {
                return;
            }

            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    ConcurrentMap<int, double>::Access access = document_to_relevance[document_id];
                    access.ref_to_value += term_freq * inverse_document_freq;
                }
            }
        });

    for (const auto& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
            ConcurrentMap<int, double>::Access access = document_to_relevance[document_id];
            access.ref_to_value = 0;
        }
    }

    std::map<int, double> result_map = document_to_relevance.BuildOrdinaryMap();

    vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : result_map) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }

    return matched_documents;
}

void AddDocument(SearchServer& search_server, int document_id, const string_view& query, DocumentStatus status, const vector<int>& ratings);