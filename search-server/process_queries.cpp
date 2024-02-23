#include "process_queries.h"

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries) {
    vector<vector<Document>> results(queries.size());

    transform(execution::par, queries.begin(), queries.end(), results.begin(), [&search_server](const auto& query) {
        return search_server.FindTopDocuments(query);
        });
    return results;
}

vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries) {
    std::vector<std::vector<Document>> results(queries.size());

    std::transform(std::execution::par, queries.begin(), queries.end(), results.begin(), [&search_server](const auto& query) {
        return search_server.FindTopDocuments(query);
        });

    size_t total_documents = 0;
    for (const auto& result : results) {
        total_documents += result.size();
    }

    std::vector<Document> vec;
    vec.reserve(total_documents);

    // Flatten the results into a single vector
    for (const auto& result : results) {
        vec.insert(vec.end(), result.begin(), result.end());
    }

    return vec;
}
