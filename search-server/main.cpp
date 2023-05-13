#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const int EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query,  DocumentStatus::ACTUAL);
    }
    
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus doc_status) const {
        return FindTopDocuments(raw_query, [&doc_status](int document_id, DocumentStatus status, int rating){ return status == doc_status; });
    }
    
    template <typename T>
    vector<Document> FindTopDocuments(const string& raw_query, T function_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, function_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    
    template <typename T>
    vector<Document> FindAllDocuments(const Query& query, T function_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& doc = documents_.at(document_id); 
                if (function_predicate(document_id, doc.status, doc.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
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
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

template <typename T>
void Assert_Iml(T t, const string& t_name, const string& file, const string& func, unsigned line, const string& hint){
    if(!t){
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT(" << t_name << ") failed.";
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(func) Assert_Iml((func), #func, __FILE__, __FUNCTION__, __LINE__, "")
#define ASSERT_HINT(func, hint) Assert_Iml((func), #func, __FILE__, __FUNCTION__, __LINE__, (hint))

template<typename T, typename U>
void Assert_Equal_Iml(T t, U u, const string& t_name, const string& u_name, const string& file, const string& func, unsigned line, const string& hint){
    if(t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_name << ", "s << u_name << ") failed. "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(func_1, func_2) Assert_Equal_Iml((func_1), (func_2), #func_1, #func_2, __FILE__, __FUNCTION__, __LINE__, "")
#define ASSERT_EQUAL_HINT(func_1, func_2, hint) Assert_Equal_Iml((func_1), (func_2), #func_1, #func_2, __FILE__, __FUNCTION__, __LINE__, (hint))

template<typename T>
void RunTest(T test_func, const string& test_func_name){
    test_func();
    cerr << test_func_name << " OK." << endl;
}

#define RUN_TEST(func) RunTest((func), #func)
// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

//Тест проверяет корректное добавление документов
void TestAddDocuments(){
    SearchServer server;
    int doc_id = 9;
    string document = "hello the my world";
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> ratings = {1,5,42};
    server.AddDocument(doc_id, document, status, ratings);
    ASSERT(!server.FindTopDocuments("hello world", status).empty());
}

//Тест проверяет корректную обработку стоп-слов
void TestStopWords(){
    SearchServer server;
    int doc_id = 3;
    string document = "hello the my world in Moscow";
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> ratings = {2,3,4};
    server.AddDocument(doc_id, document, status, ratings);
    server.SetStopWords("the in");
    ASSERT(server.FindTopDocuments("the in").empty());
}

//Тест проверяет корректную обработку минус-слов
void TestMinusWords(){
    SearchServer server;
    int doc_id = 5;
    string document = "hello the my world in Moscow";
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> ratings = {2,3,4};
    server.AddDocument(doc_id, document, status, ratings);
    ASSERT(server.FindTopDocuments("hello -world").empty());
}

//Тест проверяет корректную сортировку релевантности
void TestSortRelevanceDocuments(){
    SearchServer server;
    server.AddDocument(5, "doc is false", DocumentStatus::ACTUAL, {1,2,3});
    server.AddDocument(2, "my name is big Slava", DocumentStatus::ACTUAL, {1,2,3});
    server.AddDocument(4, "this dog is very big", DocumentStatus::ACTUAL, {1,2,3});
    vector<Document> docs = server.FindTopDocuments("big dog");
    ASSERT(docs[0].relevance > docs[1].relevance);
    ASSERT(docs[1].relevance > docs[2].relevance);
}

//Тест проверяет корректную обработку рейтинга
void TestRatings(){
    SearchServer server;
    server.AddDocument(5, "doc is false", DocumentStatus::ACTUAL, {1,2,3});
    ASSERT_EQUAL(server.FindTopDocuments("doc false", DocumentStatus::ACTUAL)[0].rating, 2);
}

//Тест проверяет корректную работу метода MatchDocuments()
void TestMatchDocuments(){
    SearchServer server;
    server.AddDocument(5, "doc is false", DocumentStatus::ACTUAL, {1,2,3});
    server.AddDocument(1, "doc is true", DocumentStatus::BANNED, {1,2,3});
    const auto [_1, stat1] = server.MatchDocument("false", 5);
    const auto [_2, stat2] = server.MatchDocument("true", 1);
    ASSERT_EQUAL(static_cast<int>(stat1), static_cast<int>(DocumentStatus::ACTUAL));
    ASSERT_EQUAL(static_cast<int>(stat2), static_cast<int>(DocumentStatus::BANNED));
}

//Тест проверяет корректность релевантности
void TestRelevanceDocuments(){
    SearchServer server;
    server.AddDocument(0, "doc is false", DocumentStatus::ACTUAL, {1,2,3});
    server.AddDocument(1, "my name is big niaz", DocumentStatus::ACTUAL, {1,2,3});
    server.AddDocument(2, "this dog is very big", DocumentStatus::ACTUAL, {1,2,3});
    vector<Document> docs = server.FindTopDocuments("big dog");
    //C EPSILON не работал
    ASSERT(abs(docs[0].relevance - 0.300815) < 1e-6);
    ASSERT(abs(docs[1].relevance - 0.081093) < 1e-6);
}

//Проверяет корректную работу предиката
void TestFunctionPredicate(){
    {
        SearchServer server;
        server.SetStopWords("и в на"s);
        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
        server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
        vector<Document> result = server.FindTopDocuments("пушистый ухоженный кот"s);
        ASSERT_EQUAL(result[0].id, 1);
        ASSERT(abs(result[0].relevance - 0.866434) < 1e-6);
        ASSERT_EQUAL(result[0].rating, 5);
    }

    {
        SearchServer server;
        server.SetStopWords("и в на"s);
        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
        server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
        vector<Document> result = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(result[0].id, 3);
        ASSERT(abs(result[0].relevance - 0.231049) < 1e-6);
        ASSERT_EQUAL(result[0].rating, 9);
    }

    {
        SearchServer server;
        server.SetStopWords("и в на"s);
        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
        server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
        vector<Document> result = server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL(result[0].id, 0);
        ASSERT(abs(result[0].relevance - 0.173287) < 1e-6);
        ASSERT_EQUAL(result[0].rating, 2);
    }
}

//Вызов всех тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocuments);
    RUN_TEST(TestStopWords);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestSortRelevanceDocuments);
    RUN_TEST(TestRatings);
    RUN_TEST(TestMatchDocuments);
    RUN_TEST(TestRelevanceDocuments);
    RUN_TEST(TestFunctionPredicate);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    cerr << "Testing..." << endl;

    TestSearchServer();
    
    cerr << "Search server testing finished"s << endl;
}