#ifndef DOES_IT_FAIL_REPORTER_CONSOLE_H
#define DOES_IT_FAIL_REPORTER_CONSOLE_H

/** @file reporter_console.h @brief Optional stdio terminal reporter. */
#include <stdbool.h>
#include <stdio.h>
#include "does_it_fail/test.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Caller-owned console reporter configuration and per-test state.
 * @note When color is true, ANSI SGR sequences are emitted. Set false in
 *       CI, logs, and terminals without ANSI support.
 */
typedef struct
{
    FILE *stream;              /**< NULL means stdout. */
    bool color;
    bool failure_printed;      /**< Internal reporter state; initially false. */
} dif_console_options;

/** Reporter callback for dif_run_suite; user_data points to
 * a dif_console_options object valid until the suite returns.
 */
void dif_report_console(const dif_event *event, void *user_data);

#ifdef __cplusplus
}
#endif
#endif
