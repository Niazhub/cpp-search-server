#include "search_server.h"

SearchServer::SearchServer()
{
}

SearchServer::SearchServer(const string_view& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const string_view& document, DocumentStatus status, const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    bufer.emplace_back(document);
    const auto words = SplitIntoWordsNoStop(bufer.back());
    const double inv_word_count = 1.0 / words.size();
    map<string_view, double> fr;
    for (const string_view& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        fr[word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, fr });
    document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(const string_view& raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const string_view& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

vector<Document> SearchServer::FindTopDocuments(execution::parallel_policy, const string_view& raw_query, DocumentStatus status) const {
    return FindTopDocuments(execution::par, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {  return document_status == status; });
}

vector<Document> SearchServer::FindTopDocuments(execution::parallel_policy, const string_view& raw_query) const {
    return FindTopDocuments(execution::par, raw_query, DocumentStatus::ACTUAL);
}

vector<Document> SearchServer::FindTopDocuments(execution::sequenced_policy, const string_view& raw_query, DocumentStatus status) const {
    return FindTopDocuments(execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {  return document_status == status; });
}

vector<Document> SearchServer::FindTopDocuments(execution::sequenced_policy, const string_view& raw_query) const {
    return FindTopDocuments(execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (documents_.count(document_id) > 0) {
        return documents_.at(document_id).freqs;
    }
    static const map<string_view, double> e_m;
    return e_m;
}

void SearchServer::RemoveDocument(int document_id) {
    for (const auto& [word, _] : documents_.at(document_id).freqs) {
        auto& doc_freqs = word_to_document_freqs_[word];
        doc_freqs.erase(document_id);
        if (doc_freqs.empty()) {
            word_to_document_freqs_.erase(word);
        }
    }
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    vector<const string_view*> words_to_erase(documents_.at(document_id).freqs.size());

    transform(
        execution::par,
        documents_.at(document_id).freqs.begin(), documents_.at(document_id).freqs.end(),
        words_to_erase.begin(),
        [this, &document_id](const auto& word_freq) { return &word_freq.first; }
    );

    for_each(execution::par, words_to_erase.begin(), words_to_erase.end(),
        [this, &document_id](const auto& word_freq) {
            word_to_document_freqs_[*word_freq].erase(document_id);
        });

    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query, true);

    for (const string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { {}, documents_.at(document_id).status };
        }
    }

    vector<string_view> matched_words;

    for (const string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, const string_view& raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy&, const string_view& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query, false);

    vector<string_view> matched_words(query.plus_words.size());

    if (std::any_of(execution::par, query.minus_words.begin(), query.minus_words.end(), [&](const string_view& word) {
        return word_to_document_freqs_.at(word).count(document_id);
        })) {

        return { {}, documents_.at(document_id).status };
    }


    auto last = copy_if(execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&](const string_view& word) {
        return word_to_document_freqs_.at(word).count(document_id);
        });

    matched_words.erase(last, matched_words.end());
    sort(execution::par, matched_words.begin(), matched_words.end());
    matched_words.erase(unique(matched_words.begin(), matched_words.end()), matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const string_view& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string_view& word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view& text) const {
    vector<string_view> words;
    for (const string_view& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string_view& text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string(text) + " is invalid");
    }
    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const string_view& text, bool is_sort) const {
    Query query;
    for (const string_view& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }

    if (is_sort) {
        sort(execution::par, query.plus_words.begin(), query.plus_words.end());
        query.plus_words.erase(unique(query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());

        sort(execution::par, query.minus_words.begin(), query.minus_words.end());
        query.minus_words.erase(unique(query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());
    }

    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string_view& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void AddDocument(SearchServer& search_server, int document_id, const string_view& query, DocumentStatus status, const vector<int>& ratings) {
    search_server.AddDocument(document_id, query, status, ratings);
}