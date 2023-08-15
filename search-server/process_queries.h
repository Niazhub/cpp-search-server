#pragma once
#include <vector>
#include <execution>
#include <algorithm>
#include <string>
#include "document.h"
#include "search_server.h"

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries);

vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries);