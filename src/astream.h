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

#ifndef __ASTREAM_H__
#define __ASTREAM_H__

#include <ctype.h>
#include <string.h>
#include <unistd.h>

#define BUFF_SIZE 1024
#define MAX_PID_BUFFER_SIZE 32
#define MAX_STREAM_RULE_NUM 256

#define EVENT_SIZE sizeof(struct inotify_event)
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

#define FILE_TYPE 1
#define DIR_TYPE 2

#define LOCK_FILE "/var/run/astream.pid"

#ifndef F_GET_RW_HINT
#define F_LINUX_SPECIFIC_BASE 1024
#define F_GET_RW_HINT (F_LINUX_SPECIFIC_BASE + 11)
#define F_SET_RW_HINT (F_LINUX_SPECIFIC_BASE + 12)
#endif

typedef struct watch_target watch_target_t;
typedef struct stream_rule stream_rule_t;

struct stream_rule {
    char rule[BUFF_SIZE];
    int stream;
};

/*
 * a target is that a monitoried directory, and many stream rules inside a rule file
 */
struct watch_target {
    char watch_dir[BUFF_SIZE];
    stream_rule_t stream_rule[MAX_STREAM_RULE_NUM];
    int rule_num;
};
#endif