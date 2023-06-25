#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server){
    set<string> unique_words;
    vector<int> duplicate_ids;

    for (const int document_id : search_server) {
        if(!search_server.GetWordFrequencies(document_id).empty()){
            const auto& freqs = search_server.GetWordFrequencies(document_id);
            vector<string> words;
            words.reserve(freqs.size());

            for (const auto& [word, _] : freqs) {
                words.push_back(word);
            }

            string joined_words = accumulate(words.begin(), words.end(), string());

            if (unique_words.count(joined_words) > 0) {
                duplicate_ids.push_back(document_id);
            } else {
                unique_words.insert(joined_words);
            }
        }
    }

    for (auto it = duplicate_ids.begin(); it != duplicate_ids.end(); ++it) {
        search_server.RemoveDocument(*it);
        std::cout << "Found duplicate document id " << *it << endl;
    }
}