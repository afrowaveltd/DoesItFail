#include "does_it_fail/test.h"

struct dif_test_context
{
    size_t passed_checks;
    size_t failed_checks;
    const char *suite_name;
    const char *test_name;
    size_t test_index;
    size_t test_count;
    dif_reporter reporter;
    void *user_data;
};

static void emit(dif_reporter reporter, void *user_data, const dif_event *event)
{
    if (reporter != NULL)
    {
        reporter(event, user_data);
    }
}

bool dif_expect_int_eq(dif_test_context *context,
                       int expected,
                       int actual,
                       const char *file,
                       int line)
{
    if (context == NULL)
    {
        return false;
    }

    if (expected == actual)
    {
        ++context->passed_checks;
        return true;
    }

    ++context->failed_checks;

    dif_event event = {0};
    event.type = DIF_CHECK_FAILED;
    event.suite_name = context->suite_name;
    event.test_name = context->test_name;
    event.test_index = context->test_index;
    event.test_count = context->test_count;
    event.file = file;
    event.line = line;
    event.expected_int = expected;
    event.actual_int = actual;
    emit(context->reporter, context->user_data, &event);
    return false;
}

dif_summary dif_run_suite(const char *suite_name,
                          const dif_test_case *tests,
                          size_t count,
                          dif_reporter reporter,
                          void *user_data)
{
    dif_summary summary = {0};
    summary.total_tests = count;

    if (count > 0 && tests == NULL)
    {
        summary.setup_error = true;
        return summary;
    }

    for (size_t i = 0; i < count; ++i)
    {
        if (tests[i].name == NULL || tests[i].function == NULL)
        {
            summary.setup_error = true;
            return summary;
        }
    }

    const char *name = (suite_name == NULL) ? "Unnamed suite" : suite_name;
    dif_event event = {0};
    event.type = DIF_SUITE_STARTED;
    event.suite_name = name;
    event.test_count = count;
    emit(reporter, user_data, &event);

    for (size_t i = 0; i < count; ++i)
    {
        event = (dif_event){0};
        event.type = DIF_TEST_STARTED;
        event.suite_name = name;
        event.test_name = tests[i].name;
        event.test_index = i + 1;
        event.test_count = count;
        emit(reporter, user_data, &event);

        dif_test_context context = {0};
        context.suite_name = name;
        context.test_name = tests[i].name;
        context.test_index = i + 1;
        context.test_count = count;
        context.reporter = reporter;
        context.user_data = user_data;

        tests[i].function(&context);
        summary.passed_checks += context.passed_checks;
        summary.failed_checks += context.failed_checks;
        bool passed = (context.failed_checks == 0);
        if (passed)
        {
            ++summary.passed_tests;
        }
        else
        {
            ++summary.failed_tests;
        }

        event = (dif_event){0};
        event.type = DIF_TEST_FINISHED;
        event.suite_name = name;
        event.test_name = tests[i].name;
        event.test_index = i + 1;
        event.test_count = count;
        event.test_passed = passed;
        emit(reporter, user_data, &event);
    }

    event = (dif_event){0};
    event.type = DIF_SUITE_FINISHED;
    event.suite_name = name;
    event.test_count = count;
    event.summary = summary;
    emit(reporter, user_data, &event);
    return summary;
}
