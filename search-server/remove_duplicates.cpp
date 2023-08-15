#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    set<set<string_view>> unique_words;
    vector<int> duplicate_ids;

    for (const int document_id : search_server) {
        const auto& freqs = search_server.GetWordFrequencies(document_id);
        if (!freqs.empty()) {
            vector<string_view> words;
            words.reserve(freqs.size());

            std::transform(freqs.begin(), freqs.end(), std::back_inserter(words), [](const pair<string_view, double> p) {
                return p.first;
                });

            set<string_view> joined_words;
            for (const auto& word : words) {
                joined_words.insert(word);
            }

            if (unique_words.count(joined_words) > 0) {
                duplicate_ids.push_back(document_id);
            }
            else {
                unique_words.insert(joined_words);
            }
        }
    }

    for (auto it = duplicate_ids.begin(); it != duplicate_ids.end(); ++it) {
        search_server.RemoveDocument(*it);
        std::cout << "Found duplicate document id " << *it << endl;
    }
}