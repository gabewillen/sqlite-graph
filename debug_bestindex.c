#include <sqlite3.h>
#include <stdio.h>

// Mock function to add debug output to our graphBestIndex
void debug_bestindex_call(sqlite3_index_info *pInfo) {
    printf("=== xBestIndex called ===\n");
    printf("nConstraint: %d\n", pInfo->nConstraint);
    printf("nOrderBy: %d\n", pInfo->nOrderBy);
    
    for(int i = 0; i < pInfo->nConstraint; i++) {
        printf("Constraint %d: column=%d, op=%d, usable=%d\n", 
               i, pInfo->aConstraint[i].iColumn, 
               pInfo->aConstraint[i].op, 
               pInfo->aConstraint[i].usable);
    }
    
    for(int i = 0; i < pInfo->nOrderBy; i++) {
        printf("OrderBy %d: column=%d, desc=%d\n", 
               i, pInfo->aOrderBy[i].iColumn, 
               pInfo->aOrderBy[i].desc);
    }
    printf("========================\n");
}

int main() {
    printf("This is a template for debugging xBestIndex calls.\n");
    printf("The actual debugging would be added to the graphBestIndex function.\n");
    return 0;
}
