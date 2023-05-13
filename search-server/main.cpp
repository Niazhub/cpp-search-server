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
    vector<Document> documents = server.FindTopDocuments("hello -world");
    ASSERT(documents.empty());
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
        const auto& result = server.FindTopDocuments("пушистый ухоженный кот"s);
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
        const auto& result = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
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
        const auto& result = server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
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