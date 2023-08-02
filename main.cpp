/* решение задачи "Выводим результаты поиска страницами" из темы "Итераторы" */
#include <deque>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <optional>
#include <stdexcept>

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
        if (!IsValidText(stop_words_)) {
            throw invalid_argument("Некорректное содержание в списке 'cnоп-слов'");
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, // обработаь наличие спецсимволов
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        if (document_id < 0 || documents_.count(document_id) != 0) {
            throw out_of_range("Некорректный id документа");
        }
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        documents_q_.push_back(document_id);
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        bool isMinus = true;
        vector<Document> documents;
        const Query query = ParseQuery(raw_query);
        if (!SearchServer::IsValidText(query.plus_words) || !SearchServer::IsValidText(query.minus_words, isMinus)) {
            throw invalid_argument("Некорректное содержание в списке слов запроса");
        }
        documents = FindAllDocuments(query, document_predicate);

        sort(documents.begin(), documents.end(),
             [](const Document& lhs, const Document& rhs) {
                double ALLOWABLE_ERROR = 1e-6;
                 if (abs(lhs.relevance - rhs.relevance) < ALLOWABLE_ERROR) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(
            raw_query, [status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]] int rating) {
                return document_status == status;
            });
    }
    
    int GetDocumentId(int index) {
        return documents_q_.at(index);
    }
    
    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        bool isMinus = true;
        const Query query = ParseQuery(raw_query);
        if (!SearchServer::IsValidText(query.plus_words) || !SearchServer::IsValidText(query.minus_words, isMinus)) {
            throw invalid_argument("Некорректное содержание в списке слов запроса");
        }
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
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> documents_q_;
    
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
    
    static bool IsValidWord(const string& word, bool isMinus = false) {
        size_t last_ch = word.size() - 1;
        if (isMinus && word[0] == '-') {
            return false;
        }
        if (word[last_ch] == '-') {
            return false;
        }
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }
    
    template <typename Container>
    static bool IsValidText(const Container& text, bool isMinus = false) {
        for (const auto& word : text) {
            if (!SearchServer::IsValidWord(word, isMinus) || word == "") {
                return false;
            }
        }

        return true;
    }
    
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Некорректное содержание в списке слов документа"); //заходит спецсимвол в бар...
            }
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

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
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
};

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& server) : search_server(server) {
        // напишите реализацию
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        return ManageRequest(search_server.FindTopDocuments(raw_query, document_predicate));
    }
    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status) {
        return ManageRequest(search_server.FindTopDocuments(raw_query, status));
    }
    vector<Document> AddFindRequest(const string& raw_query) {
        return ManageRequest(search_server.FindTopDocuments(raw_query));
    }
    int GetNoResultRequests() const {
        return count_if(requests_.begin(), requests_.end(), [](QueryResult res) {return !res.request_type;});
    }
private:
    struct QueryResult {
        bool request_type;
    };
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server;
    
    vector<Document> ManageRequest(const vector<Document>& documents) {
        if (requests_.size() >= min_in_day_) {
            requests_.pop_front();
        }
        if (!documents.empty()) {
            requests_.push_back({true});
        } else {
            
            requests_.push_back({false});
        }
        return documents;
    }
};

template <typename Iter>
class IteratorRange {
    public:
    
    IteratorRange(Iter begin, size_t size) : range_begin(begin), range_end(begin + size)
    {}
     
    auto begin() const {
    return range_begin;
    }
    
    auto end() const {
    return range_end;
    }
    
    size_t size() const {
    return distance(range_begin, range_end);
    }
     
    private:
    
    Iter range_begin;
    Iter range_end;
};


template <typename Iter>
class Paginator {
public:
    
    Paginator(Iter begin, Iter end, size_t size) {
        for (auto i = begin; i != end; advance(i, size)) {
            if (end - i < size) {
                size -= (end - i);
            }
            pages.push_back(IteratorRange<Iter>(i, size));
        }
    }
    
    auto begin() const {
    return pages.begin();
    }
    
    auto end() const {
    return pages.end();
    }
    
    size_t size() const {
    return pages.size();
    }
    
private:
    
    vector<IteratorRange<Iter>> pages;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

ostream& operator<<(ostream& os, const Document& document) {
        os << "{ " << "document_id" << " = " << document.id <<
        ", relevance = " << document.relevance <<
        ", rating = " << document.rating << " }";
        return os;
    }

template <typename Iter>
ostream& operator<<(ostream& os, const IteratorRange<Iter>& range) {
    for (auto i = range.begin(); i != range.end(); ++i) {
        os << *i;
    }
    return os;
}


int main() {
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    return 0;
}
