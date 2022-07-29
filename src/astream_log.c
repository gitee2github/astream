/*
* Copyright (c) 2021-2022 Huawei Technologies Co., Ltd.
* astream is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*     http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include "astream_log.h"

static enum log_level g_log_level;

void log_level_usage(void)
{
    printf("[1] for debug level\n"
           "[2] for info level\n"
           "[3] for warning level\n"
           "[4] for error level\n"
           "error is the default level\n");
}

int set_global_astream_log_level(enum log_level level)
{
    if (level < 0 || level > ASTREAM_LOG_ERROR) {
        printf("error: invalid log level [%d]\n", level);
        log_level_usage();
        return -EINVAL;
    }
    g_log_level = level;

    return 0;
}

void astream_log(enum log_level level, const char *format, ...)
{
    va_list args;

    if (level < g_log_level)
        return;

    va_start(args, format);
    switch (level) {
        case ASTREAM_LOG_DEBUG:
            openlog("[astream_debug]", LOG_CONS, LOG_USER);
            vsyslog(LOG_INFO, format, args);
            break;
        case ASTREAM_LOG_INFO:
            openlog("[astream_info] ", LOG_CONS, LOG_USER);
            vsyslog(LOG_INFO, format, args);
            break;
        case ASTREAM_LOG_WARN:
            openlog("[astream_warn] ", LOG_CONS, LOG_USER);
            vsyslog(LOG_WARNING, format, args);
            break;
        case ASTREAM_LOG_ERROR:
            openlog("[astream_error] ", LOG_CONS, LOG_USER);
            vsyslog(LOG_ERR, format, args);
            break;
        default:
            openlog("[astream_error] ", LOG_CONS, LOG_USER);
            vsyslog(LOG_ERR, "invalid log level\n", args);
    }

    va_end(args);
    closelog();
    
    return;
}