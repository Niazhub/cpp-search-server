#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <optional>
 
using namespace std;
 
const int MAX_RESULT_DOCUMENT_COUNT = 5;
 
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
    Document() = default;
 
    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }
 
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};
 
template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
 
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
 
class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
            if(!IsValidWords(stop_words_)){ //!IsValidWords(MakeUniqueNonEmptyStrings(stop_words)), какой способ лучше? Безопасно передавать туда поле stop_words_?
                throw invalid_argument("Стоп-слов содержит недопустимые символы");
            }
    }
 
    explicit SearchServer(const string& stop_words)
        : SearchServer(SplitIntoWords(stop_words))  // Invoke delegating constructor from string container
    {
    }

    inline static constexpr int INVALID_DOCUMENT_ID = -1;
 
    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        if (document_id < 0 || documents_.count(document_id) > 0) {
            throw invalid_argument("ID документа меньше нуля или такой ID уже существует");
        }
        const auto words = SplitIntoWordsNoStop(document);
        if (!IsValidWords(words)) {
            throw invalid_argument("Слова в документе содержат недопустимые символы");
        }
 
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }
 
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query,
                                      DocumentPredicate document_predicate) const {
        if (!IsValidWords(SplitIntoWords(raw_query))) {
            throw invalid_argument("Ваши слова в документе содержат недопустимые символы");
        }
        const auto query = ParseQuery(raw_query);
        if (!query) {
            throw invalid_argument("Корректонсть вашего запроса оставляет желать лучшего");
        }
        vector<Document> result = FindAllDocuments(*query, document_predicate);
 
        sort(result.begin(), result.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
            result.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return result;
    }
 
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }
 
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }
 
    int GetDocumentCount() const {
        return documents_.size();
    }
 
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
        const auto query = ParseQuery(raw_query);
        if (!query) {
            throw invalid_argument("Корректонсть вашего запроса оставляет желать лучшего");
        }
        vector<string> matched_words;
        for (const string& word : query->plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query->minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        tuple<vector<string>, DocumentStatus> result = {matched_words, documents_.at(document_id).status};
        return result;
    }
 
    int GetDocumentId(int index) const {
        int i = 0;
        for (const auto& [id, doc_data] : documents_) {
            if (i == index) {
                return id;
            } else {
                ++i;
            }
        }
        throw out_of_range("Индекс переданного документа выходит за пределы допустимого диапазона");
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
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
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }
 
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
 
    optional<QueryWord> ParseQueryWord(string text) const {
        QueryWord word;
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        if (text.size() == 0 || text[0] == '-' || !IsValidWord(text)) {
            return nullopt;
        }
        word = {text, is_minus, IsStopWord(text)};
 
        return word;
    }
 
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };
 
    optional<Query> ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const auto query_word = ParseQueryWord(word);
            if (!query_word) {
                return nullopt;
            }
            if (!query_word->is_stop) {
                if (query_word->is_minus) {
                    query.minus_words.insert(query_word->data);
                } else {
                    query.plus_words.insert(query_word->data);
                }
            }
        }
        return query;
    }
 
    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
 
    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
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
 
    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }
 
    static bool IsValidWords(vector<string> words) {
        for (const string& word : words) {
            if (!IsValidWord(word)) {
                return false;
            }
        }
        return true;
    }

    static bool IsValidWords(set<string> words) {
        for (const string& word : words) {
            if (!IsValidWord(word)) {
                return false;
            }
        }
        return true;
    }
};
 
// ==================== для примера =========================
 
void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    SearchServer search_server("и в на"s);
    // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
    // о неиспользуемом результате его вызова
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    try{
        search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    } catch(const invalid_argument& err){
        cout << "Error: " << err.what() << endl;
    }
    try{
        search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    } catch(const invalid_argument& err){
        cout << "Error: " << err.what() << endl;
    }
    try{
        search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, {1, 3, 2});
    } catch(const invalid_argument& err){
        cout << "Error: " << err.what() << endl;
    }
    try{
        for(const auto& document : search_server.FindTopDocuments("--пушистый"s)){
            PrintDocument(document);
        }
    } catch(const invalid_argument& err){
        cout << "Error: " << err.what() << endl;
    }
}