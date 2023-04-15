#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

struct Document {
    int id;
    double relevance;
};

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
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

class SearchServer {
    
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        document_count_++; 
        const vector<string> words = SplitIntoWordsNoStop(document);
        double TF = 1.0 / words.size();
        for(const string& word : words){
            word_to_documents_[word][document_id] += TF;
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    
private:
    struct MinusWords{
        string text;
        bool minus;
        bool stop_words;
    };
    
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };
    int document_count_ = 0;

    map<string, map<int, double>> word_to_documents_;

    set<string> stop_words_;

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
    
    MinusWords IsMinusWords(string word) const{
        bool minus = false;
        if (word[0] == '-') {
            minus = true;
            word = word.substr(1);
        }
        return {word, minus, IsStopWord(word)};
    }

    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            const MinusWords query = IsMinusWords(word);
            if (!query.stop_words) {
                if (query.minus) {
                    query_words.minus_words.insert(query.text);
                } else {
                    query_words.plus_words.insert(query.text);
                }
            }
        }
        return query_words;
    }
    
    double IDF(const string& plus_word) const{
        return log(static_cast<double>(document_count_)/static_cast<double>(word_to_documents_.at(plus_word).size()));
    }
    
    vector<Document> FindAllDocuments(const Query& query_words) const {
        map<int, double> document_to_relevance;
        vector<Document> relevance;
        
        for (const string& plus_word : query_words.plus_words) {
            if(word_to_documents_.count(plus_word) != 0){
                for(const auto& [id, tf] : word_to_documents_.at(plus_word)){
                    document_to_relevance[id] += IDF(plus_word) * tf;
                }
            }
        }
        
        for (const string& minus_word : query_words.minus_words){
            if(word_to_documents_.count(minus_word)){
                for(const auto& [id, rel] : word_to_documents_.at(minus_word)){
                    document_to_relevance.erase(id);
                }
            }
        }
        
        for(const auto& rel : document_to_relevance){
            relevance.push_back({rel.first, rel.second});
        }
        return relevance;
    }

};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}