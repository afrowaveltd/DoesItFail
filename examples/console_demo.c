#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "does_it_fail/test.h"
#include "does_it_fail/reporter_console.h"

static void example_pass(dif_test_context *context)
{
    DIF_EXPECT_INT_EQ(context, 5, 5);
}

static void example_failure(dif_test_context *context)
{
    DIF_EXPECT_INT_EQ(context, 5, 0);
    DIF_EXPECT_INT_EQ(context, 42, 42); /* still runs after previous failure */
}

int main(int argc, char **argv)
{
    bool demonstrate_failure = (argc > 1 && strcmp(argv[1], "--fail") == 0);
    const dif_test_case successful[] =
    {
        {"example_pass", example_pass}
    };
    const dif_test_case failing[] =
    {
        {"example_pass", example_pass},
        {"example_failure", example_failure},
        {"example_pass_again", example_pass}
    };

    dif_console_options options = {0};
    options.stream = stdout;
    options.color = true; /* Demo explicitly requests ANSI colors. */
    dif_summary summary = demonstrate_failure
        ? dif_run_suite("Demo with intentional failure", failing,
                        sizeof(failing) / sizeof(failing[0]), dif_report_console, &options)
        : dif_run_suite("Demo", successful,
                        sizeof(successful) / sizeof(successful[0]), dif_report_console, &options);
    return (summary.setup_error || summary.failed_tests != 0) ? 1 : 0;
}
