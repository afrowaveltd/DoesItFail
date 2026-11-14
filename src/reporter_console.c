#include "does_it_fail/reporter_console.h"

#define DIF_ANSI_GREEN "\033[32m"
#define DIF_ANSI_RED   "\033[31m"
#define DIF_ANSI_RESET "\033[0m"

static void print_status(FILE *stream, bool color, bool passed)
{
    const char *word = passed ? "PASSED" : "FAILED";
    if (!color)
    {
        fprintf(stream, "%s\n", word);
        return;
    }
    fprintf(stream, "%s%s%s\n",
            passed ? DIF_ANSI_GREEN : DIF_ANSI_RED, word, DIF_ANSI_RESET);
}

void dif_report_console(const dif_event *event, void *user_data)
{
    if (event == NULL)
    {
        return;
    }

    dif_console_options default_options = {0};
    dif_console_options *options = (user_data == NULL) ? &default_options : user_data;
    FILE *stream = (options->stream == NULL) ? stdout : options->stream;

    switch (event->type)
    {
        case DIF_SUITE_STARTED:
            fprintf(stream, "\n============================================================\n"
                            " DoesItFail | %s\n"
                            "============================================================\n",
                    event->suite_name);
            break;
        case DIF_TEST_STARTED:
            options->failure_printed = false;
            fprintf(stream, "Running test %02zu: %-37s ",
                    event->test_index, event->test_name);
            fflush(stream);
            break;
        case DIF_CHECK_FAILED:
            if (!options->failure_printed)
            {
                print_status(stream, options->color, false);
                options->failure_printed = true;
            }
            fprintf(stream,
                    "    Integer equality check failed\n"
                    "      Expected value: %d\n"
                    "      Actual value:   %d\n"
                    "      Location:       %s:%d\n",
                    event->expected_int, event->actual_int,
                    event->file == NULL ? "(unknown)" : event->file, event->line);
            break;
        case DIF_TEST_FINISHED:
            if (!options->failure_printed)
            {
                print_status(stream, options->color, event->test_passed);
            }
            break;
        case DIF_SUITE_FINISHED:
            fprintf(stream,
                    "------------------------------------------------------------\n"
                    "Tests: %zu | Passed: %zu | Failed: %zu\n"
                    "Checks: passed %zu | failed %zu\n",
                    event->summary.total_tests,
                    event->summary.passed_tests,
                    event->summary.failed_tests,
                    event->summary.passed_checks,
                    event->summary.failed_checks);
            break;
        default:
            break;
    }
}
