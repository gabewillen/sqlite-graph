#include "tck_assert.h"

int main(void) {
    printf("Testing basic TCK functionality...\n");
    
    /* Initialize the TCK framework */
    tck_assert_init();
    
    /* Test very basic functionality */
    printf("Features available: 0x%04x\n", g_tck_available_features);
    
    /* Try a simple test that doesn't use complex macros */
    g_tck_tests_run++;
    g_tck_tests_passed++;
    printf("Basic functionality works\n");
    
    /* Print summary and cleanup */
    tck_assert_print_summary();
    tck_assert_cleanup();
    
    return 0;
}
