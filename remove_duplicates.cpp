#include "remove_duplicates.h"
#include "search_server.h"
#include <set>

void RemoveDuplicates(SearchServer& search_server) {
    for (auto iter = search_server.begin(); iter != search_server.end(); ++iter) {
        auto& document = *iter;
        for (auto document_id : search_server.FindDuplicates(document.first)) {
            search_server.RemoveDocument(document_id);
        }
    }
}