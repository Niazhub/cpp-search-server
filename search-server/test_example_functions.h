#pragma once

#include <utility>
#include <string>
#include "search_server.h"

void TestExcludeStopWordsFromAddedDocumentContent();

void TestAddDocuments();

void TestMinusWords();

void TestSortRelevanceDocuments();

void TestRatings();

void TestMatchDocuments();

void TestRelevanceDocuments();

void TestFunctionPredicateFilter();

void TestStatusFilter();

void TestSearchServer();

template <typename T>
void Assert_Iml(T t, const string& t_name, const string& file, const string& func, unsigned line, const string& hint) {
    if (!t) {
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
void Assert_Equal_Iml(T t, U u, const string& t_name, const string& u_name, const string& file, const string& func, unsigned line, const string& hint) {
    if (t != u) {
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
void RunTest(T test_func, const string& test_func_name) {
    test_func();
    cerr << test_func_name << " OK." << endl;
}

#define RUN_TEST(func) RunTest((func), #func) 
