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

#ifndef __ASTREAM_LOG_H__
#define __ASTREAM_LOG_H__

enum log_level {
    ASTREAM_LOG_DEBUG = 1,
    ASTREAM_LOG_INFO,
    ASTREAM_LOG_WARN,
    ASTREAM_LOG_ERROR,
};

void log_level_usage(void);
int set_global_astream_log_level(enum log_level level);

void astream_log(enum log_level level, const char *format, ...);
#endif