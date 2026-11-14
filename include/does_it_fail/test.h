#ifndef DOES_IT_FAIL_TEST_H
#define DOES_IT_FAIL_TEST_H

/** @file test.h
 * @brief Minimal portable C17 test runner with structured events.
 * @note Provisional learning API: no ABI stability or release is promised.
 */
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dif_test_context dif_test_context;

/** A test receives a borrowed context valid only during that invocation. */
typedef void (*dif_test_function)(dif_test_context *context);

typedef struct
{
    const char *name;             /**< Borrowed, NUL-terminated name. */
    dif_test_function function;   /**< Must not be NULL. */
} dif_test_case;

typedef struct
{
    size_t total_tests;
    size_t passed_tests;
    size_t failed_tests;
    size_t passed_checks;
    size_t failed_checks;
    bool setup_error;
} dif_summary;

typedef enum
{
    DIF_SUITE_STARTED,
    DIF_TEST_STARTED,
    DIF_CHECK_FAILED,
    DIF_TEST_FINISHED,
    DIF_SUITE_FINISHED
} dif_event_type;

/**
 * @brief Transient, read-only notification sent synchronously to a reporter.
 * @note The callback must not retain this pointer. Strings are borrowed.
 *       expected_int/actual_int are meaningful for DIF_CHECK_FAILED only.
 *       test_passed is meaningful for DIF_TEST_FINISHED only.
 */
typedef struct
{
    dif_event_type type;
    const char *suite_name;
    const char *test_name;
    size_t test_index;     /**< 1-based display index. */
    size_t test_count;
    const char *file;
    int line;
    int expected_int;
    int actual_int;
    bool test_passed;
    dif_summary summary;
} dif_event;

typedef void (*dif_reporter)(const dif_event *event, void *user_data);

/**
 * @brief Compare two int values without terminating the process.
 * @param context Borrowed active test context, must not be NULL.
 * @param expected Expected integer value.
 * @param actual Actual integer value.
 * @param file Borrowed file name, typically __FILE__.
 * @param line Source line, typically __LINE__.
 * @return true on equality; otherwise records failure and returns false.
 */
bool dif_expect_int_eq(dif_test_context *context,
                       int expected,
                       int actual,
                       const char *file,
                       int line);

/** Capture file and line. Each value expression is evaluated once, but
 * C does not guarantee the order of evaluation of function arguments.
 * Avoid arguments with mutually dependent side effects.
 */
#define DIF_EXPECT_INT_EQ(context, expected, actual) \
    dif_expect_int_eq((context), (expected), (actual), __FILE__, __LINE__)

/**
 * @brief Run every registered test, continuing after ordinary check failures.
 * @param suite_name Borrowed NUL-terminated suite name (NULL is allowed).
 * @param tests Borrowed test array, required when count is nonzero.
 * @param count Number of test entries.
 * @param reporter Optional event callback; NULL suppresses output.
 * @param user_data Opaque borrowed callback context.
 * @return Counters plus setup_error for invalid test registration.
 * @note Does not catch abort, fatal signals, or undefined behavior.
 * @note All callbacks run synchronously on the calling thread.
 */
dif_summary dif_run_suite(const char *suite_name,
                          const dif_test_case *tests,
                          size_t count,
                          dif_reporter reporter,
                          void *user_data);

#ifdef __cplusplus
}
#endif
#endif
