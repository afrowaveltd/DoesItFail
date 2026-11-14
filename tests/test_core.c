/* Bootstrapping tests deliberately use standard assert, not DoesItFail itself. */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "does_it_fail/test.h"

#ifdef NDEBUG
#error "DoesItFail bootstrap tests require assertions; do not use -DNDEBUG."
#endif

typedef struct
{
    size_t events[5];
    int last_expected;
    int last_actual;
    const char *failure_file;
    int failure_line;
} captured_events;

static void capture(const dif_event *event, void *user_data)
{
    captured_events *capture_state = user_data;
    assert(event != NULL);
    assert(capture_state != NULL);
    assert(event->type >= DIF_SUITE_STARTED && event->type <= DIF_SUITE_FINISHED);
    capture_state->events[event->type]++;
    if (event->type == DIF_CHECK_FAILED)
    {
        capture_state->last_expected = event->expected_int;
        capture_state->last_actual = event->actual_int;
        capture_state->failure_file = event->file;
        capture_state->failure_line = event->line;
    }
}

static void test_pass(dif_test_context *context)
{
    assert(DIF_EXPECT_INT_EQ(context, 5, 5));
    assert(DIF_EXPECT_INT_EQ(context, 0, 0));
}

static void test_fail_and_continue(dif_test_context *context)
{
    bool first = DIF_EXPECT_INT_EQ(context, 5, 0);
    bool second = DIF_EXPECT_INT_EQ(context, 8, 8);
    assert(!first);
    assert(second);
}

static void test_continues(dif_test_context *context)
{
    assert(DIF_EXPECT_INT_EQ(context, 42, 42));
}

int main(void)
{
    const dif_test_case tests[] =
    {
        {"test_pass", test_pass},
        {"test_fail_and_continue", test_fail_and_continue},
        {"test_continues", test_continues}
    };
    captured_events events = {0};
    dif_summary result = dif_run_suite("Bootstrap", tests, sizeof(tests) / sizeof(tests[0]), capture, &events);
    assert(!result.setup_error);
    assert(result.total_tests == 3);
    assert(result.passed_tests == 2);
    assert(result.failed_tests == 1);
    assert(result.passed_checks == 4);
    assert(result.failed_checks == 1);
    assert(events.events[DIF_SUITE_STARTED] == 1);
    assert(events.events[DIF_TEST_STARTED] == 3);
    assert(events.events[DIF_CHECK_FAILED] == 1);
    assert(events.events[DIF_TEST_FINISHED] == 3);
    assert(events.events[DIF_SUITE_FINISHED] == 1);
    assert(events.last_expected == 5);
    assert(events.last_actual == 0);
    assert(events.failure_file != NULL);
    assert(events.failure_line > 0);

    dif_summary empty = dif_run_suite("Empty", NULL, 0, NULL, NULL);
    assert(!empty.setup_error && empty.total_tests == 0);

    dif_summary invalid = dif_run_suite("Invalid", NULL, 1, NULL, NULL);
    assert(invalid.setup_error);

    puts("DoesItFail bootstrap: all self-tests PASSED.");
    return 0;
}
