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
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <regex.h>
#include <getopt.h>
#include "astream.h"
#include "astream_log.h"

static int nr_watches = 0;
static watch_target_t targets[BUFF_SIZE];
static FILE *fp = NULL;
static int inotify_fd = -1;

static void free_res(int fd)
{
    /* removing the monitored directories from the monitoring list. */
    for (int wd = 1; wd <= nr_watches; ++wd) {
        inotify_rm_watch(fd, wd);
    }

    /* close the INOTIFY instance. */
    close(fd);
}

static void add_watch(int fd, int index)
{
    char *dir = targets[index].watch_dir;
    int wd = inotify_add_watch(fd, dir, IN_CREATE);

    if (wd == -1) {
        free_res(fd);
        astream_log(ASTREAM_LOG_ERROR, "inotify_add_watch() failed");
        exit(EXIT_FAILURE);
    }

    astream_log(ASTREAM_LOG_INFO, "begin to watching %s\n", dir);
}

/*
 * use fcntl to set the stream of the target file truely.
 */
static void do_set_stream(int stream, const char *target_file)
{
    /* set the stream. */
    int fd = open(target_file, O_RDONLY);
    if(fd < 0) {
        astream_log(ASTREAM_LOG_ERROR, "failed to open file %s\n", target_file);
        return;
    }

    if (fcntl(fd, F_SET_RW_HINT, &stream) < 0)
        astream_log(ASTREAM_LOG_ERROR, "failed to set stream for %s\n", target_file);
    else
        astream_log(ASTREAM_LOG_INFO, "set stream %d for %s done\n", stream, target_file);
    
    close(fd);
}

static void pass_stream_for_file(struct inotify_event *event)
{
    if (!(event->mask & IN_CREATE))
        return;

    /* we only focus on the IN_CREATE event. */
    int ret;
    int stream;
    int cflags;
    int len;
    char path[BUFF_SIZE];
    char *dir;
    regmatch_t pmatch;
    regex_t reg;

    dir = targets[event->wd].watch_dir;
    len = strlen(dir) + strlen(event->name) + 2;
    if (len > BUFF_SIZE)
        return ;
    snprintf(path, len, "%s/%s", dir, event->name);

    astream_log(ASTREAM_LOG_INFO, "file %s has created\n", path);

    cflags = REG_EXTENDED | REG_NEWLINE;

    /* match each rule one-by-one with new created file. */
    for (int i = 0; i < targets[event->wd].rule_num; ++i) {
        stream = targets[event->wd].stream_rule[i].stream;
        ret = regcomp(&reg, targets[event->wd].stream_rule[i].rule, cflags);
        if (ret) {
            astream_log(ASTREAM_LOG_ERROR, "failed to compile the regex "
                "expression %s\n", targets[event->wd].stream_rule[i].rule);
            regfree(&reg);
            continue;
        }

        ret = regexec(&reg, path, 1, &pmatch, 0);
        if (ret != REG_NOMATCH && pmatch.rm_so != -1) {
            astream_log(ASTREAM_LOG_INFO, "start to set stream for %s\n", path);
            do_set_stream(stream, path);
            regfree(&reg);
            return;
        }

        regfree(&reg);
    }

    astream_log(ASTREAM_LOG_INFO, "no stream rule is matched with %s\n", path);
}

/* monitor the creation movement of target file under monitored directory. */
static void inotify_accept(int fd)
{
    char buf[EVENT_BUF_LEN];
    ssize_t nr_read;
    struct inotify_event *event;

    while ((nr_read = read(fd, buf, EVENT_BUF_LEN)) > 0) {
        /* process all of the events in buffer returned by read(). */
        for (char *p = buf; p < buf + nr_read;) {
            event = (struct inotify_event *)p;
            pass_stream_for_file(event);
            p += EVENT_SIZE + event->len;
        }
    }
}

static char *trimwhitespace(char *s)
{
    char *end;

    char *str = (char *)calloc(strlen(s) + 1, sizeof(s));
    if (!str)
        return NULL;

    strcpy(str, s);

    /* trim leading space */
    while (isspace((unsigned char)*str))
        str++;

    /* all spaces ? */
    if (*str == 0)
        return str;

    /* trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    /* write a null terminator character */
    end[1] = '\0';

    return str;
}

/* parse all stream rules from a given file. */
static int parse_stream_rule(char *rule_file, watch_target_t *target)
{
    int nr_segments; /* record the numbers of segments on each line. */
    int nr_rules = 0;
    char rule[BUFF_SIZE];
    char *segment;
    char *line;
    FILE *fp;

    fp = fopen(rule_file, "r");
    if (!fp) {
        printf("error: failed to open %s\n", rule_file);
        return -1;
    }

    /* read each line from file of stream rule. */
    while (fgets(rule, BUFF_SIZE, fp) != NULL && rule[0] != '\n') {
        stream_rule_t stream_rule;
        nr_segments = 0;
        line = trimwhitespace(rule);

        /* split each line by space delimeter. */
        while ((segment = strsep(&line, " ")) != NULL) {
            if (strcmp(segment, "") == 0)
                continue;

            ++nr_segments;
            if (nr_segments == 1) {
                strcpy(stream_rule.rule, segment);
            } else if (nr_segments == 2) {
                if (strcmp(segment, "0") != 0 && (stream_rule.stream = atoi(segment)) == 0) {
                    printf("error: failed to parse the rule: %s\n", rule);
                    goto err;
                }
            } else {
                printf("error: failed to parse the rule: %s\n", rule);
                goto err;
            }
        }

        free(line);

        if (nr_rules >= MAX_STREAM_RULE_NUM) {
            printf("error: the number of current rules is bigger than %d "
                    "\n", MAX_STREAM_RULE_NUM);
            goto err;
        }

        /* add a normal rule into the list of stream rule. */
        if (nr_segments == 2)
            target->stream_rule[nr_rules++] = stream_rule;
    }

    fclose(fp);
    return nr_rules;

err:
    fclose(fp);
    free(line);
    return -1;
}

static int check_path_argument(const char *path, int type)
{
    int ret = 0;

    if (access(path, F_OK) != 0) {
        if (type == DIR_TYPE) {
            printf("error: the monitored directory %s don't exist\n", path);
            ret = -1;
        } else if (type == FILE_TYPE) {
            printf("error: the rule file %s don't exist\n", path);
            ret = -1;
        }
    }

    return ret;
}

static void astream_usage()
{
    printf("usage: astream [options]\n"
        "options:\n"
        "    -i|--inotify_directory <dir path>   one or more monitored directories\n"
        "    -r|--rule_file <file path>          one or more rule files\n"
        "    -l|--log_level <log level>          set the global log level\n"
        "    -h|--help                           show the usage of astream\n"
        "    stop                                stop the astream stop normally\n");
}

static int check_cmdline(int opt, int argc, char **argv, int *monitored_dirs_arr, 
                  int *rule_files_arr, int *nr_monitored_dirs, int *nr_rule_files)
{
    int ret = 0;
    switch (opt) {
        case 'i':
            ret = check_path_argument(optarg, DIR_TYPE);
            if (ret < 0)
                goto err;

            monitored_dirs_arr[*nr_monitored_dirs] = optind - 1;
            ++(*nr_monitored_dirs);

            while (optind < argc && argv[optind] != NULL && argv[optind][0] != '-') {
                monitored_dirs_arr[*nr_monitored_dirs] = optind;
                ++(*nr_monitored_dirs);
                ret = check_path_argument(argv[optind], DIR_TYPE);
                if (ret < 0)
                    goto err;

                ++optind;
            }
            break;
        case 'r':
            ret = check_path_argument(optarg, FILE_TYPE);
            if (ret < 0)
                goto err;
        
            rule_files_arr[*nr_rule_files] = optind - 1;
            ++(*nr_rule_files);

            while (optind < argc && argv[optind] != NULL && argv[optind][0] != '-') {
                rule_files_arr[*nr_rule_files] = optind;
                ++(*nr_rule_files);
                ret = check_path_argument(optarg, FILE_TYPE);
                if (ret < 0)
                    goto err;

                ++optind;
            }
            break;
        case 'h':
            astream_usage();
            break;
        case 'l':
            ret = atoi(optarg);
            if (ret <= 0 || ret > ASTREAM_LOG_ERROR) {
                printf("error: invalid log level\n");
                log_level_usage();
                ret = -1;
            }

            if (ret != -1)
                set_global_astream_log_level(ret);
            break;
        case '?':
        default:
            ret = -1;
            astream_usage();
            break;
    }

err:
    return ret;
}

static int check_parse_result(int argc, int nr_arguments, const int *help, int log_level)
{
    if (*help && nr_arguments == argc - 1) /* for astream -h */
        return 0;

    /* for astream -i xx -r xx [-l 1] */
    if (!(*help) && nr_arguments >= 4 && nr_arguments == argc - 1)
        return 0;

    if (log_level && nr_arguments == argc - 1) {
        printf("warning: -r and -i options are required\n");
        return -1;
    }

    printf("warning: useless parameters passed in\n");
    return -1;
}

static int parse_cmdline(int argc, char **argv, int *help)
{
    const char *opt_str = "i:r:l:h";
    int ret = 0;
    int log_level = 0;
    int opt, nr_targets = 0;
    int nr_arguments = 0, nr_monitored_dirs = 0, nr_rule_files = 0;
    /*
     *  the index array of monitored directories.
     */
    int monitored_dirs_arr[BUFF_SIZE];
    /*
     *  the index array of the given rule files.
     */
    int rule_files_arr[BUFF_SIZE];

    struct option long_options[] = {
        {"inotify_directory", required_argument, NULL, 'i'},
        {"rule_file", required_argument, NULL, 'r'},
        {"help", no_argument, NULL, 'h'},
        {"log_level", required_argument, NULL, 'l'},
        {NULL, 0, NULL, 0},
    };

    if (argc == 1) {
        astream_usage();
        ret = -1;
        goto err;
    }

    while ((opt = getopt_long(argc, argv, opt_str, long_options, NULL)) != -1) {
        if (opt == 'h')
            *help = 1;

        ret = check_cmdline(opt, argc, argv, monitored_dirs_arr, rule_files_arr, 
                            &nr_monitored_dirs, &nr_rule_files);
        if (ret < 0) {
            goto err;
        }

        ++nr_arguments;
        if (opt == 'l') {
            log_level = 1;
            ++nr_arguments;
        }
    }

    nr_arguments += nr_monitored_dirs;
    nr_arguments += nr_rule_files;

    ret = check_parse_result(argc, nr_arguments, help, log_level);
    if (ret < 0) {
        astream_usage();
        goto err;
    }

    if (nr_monitored_dirs != nr_rule_files) {
        ret = -1;
        printf("error: please input one pair of monitored directory and its "
                "matching rule file or more\n");
        astream_usage();
        goto err;
    } else {
        /* start to parse all the stream rules inside rule files */
        while (nr_targets < nr_monitored_dirs) {
            watch_target_t target;
            realpath(argv[monitored_dirs_arr[nr_targets]], target.watch_dir);
            target.rule_num = parse_stream_rule(argv[rule_files_arr[nr_targets]], &target);
            if (target.rule_num == -1)
                return -1;

            targets[++nr_targets] = target;
        }
    }

    nr_watches = nr_targets;
    astream_log(ASTREAM_LOG_INFO, "The total number of monitored targets "
                "is %d\n", nr_watches);
err:
    return ret;
}

static void start_inotify(int argc)
{
    /* init inotify instance. */
    inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        astream_log(ASTREAM_LOG_ERROR, "inotify_init() failed\n");
        return;
    }

    /* add all monitored directories one by one. */
    for (int i = 1; i <= nr_watches; ++i) {
        add_watch(inotify_fd, i);
    }

    astream_log(ASTREAM_LOG_INFO, "the astream is started successful, and "
                "begin to monitor\n");

    /* receive notification and handle it. */
    inotify_accept(inotify_fd);
}

static void signalHandler(int signum)
{
    /* stop the astream daemon and release some sources when we get a signal. */
    if (fp != NULL) {
        fclose(fp);
        flock(fp->_fileno, LOCK_UN);
    }

    if (inotify_fd == 0)
        free_res(inotify_fd);

    astream_log(ASTREAM_LOG_INFO, "the astream daemon has been stopped\n");
    exit(EXIT_SUCCESS);
}

static void astream_stop() {
    char buf[MAX_PID_BUFFER_SIZE];
    FILE *fp;
    pid_t pid;

    fp = fopen(LOCK_FILE, "r");
    if (!fp) {
        astream_log(ASTREAM_LOG_ERROR, "failed to open file %s\n", LOCK_FILE);
        return;
    }

    /* get the pid of astream daemon from LOCK_FILE, and send a signal. */
    if (fgets(buf, MAX_PID_BUFFER_SIZE, fp) != NULL) {
        pid = atoi(buf);
        if (pid > 0) {
            kill(pid, SIGUSR1);// send a SIGUSR1 signal
            printf("the astream daemon has been stopped\n");
        } else {
            astream_log(ASTREAM_LOG_ERROR, "failed to stop astream daemon\n");
        }
    }

    fclose(fp);
    remove(LOCK_FILE);
}

int main(int argc, char **argv)
{
    int help = 0;
    int ret;
    int len;
    char buf[MAX_PID_BUFFER_SIZE];

    if (argc == 2 && strcmp(argv[argc - 1], "stop") == 0) {
        astream_stop();
        return 0;
    }

    /* check in case of astream daemon starts again. */
    fp = fopen(LOCK_FILE, "r");
    if (fp) {
        if (flock(fp->_fileno, LOCK_EX | LOCK_NB) != 0) {
            printf("error: astream daemon is running, if you need to restart "
                    "it, stop it first using [astream stop] command\n");
            goto err;
        }
        fclose(fp);
    }
    
    fp = fopen(LOCK_FILE, "w");
    if (!fp) {
        printf("error: failed to open file %s\n", LOCK_FILE);
        goto err;
    }

    /* lock the file with the file lock. */
    if(flock(fp->_fileno, LOCK_EX | LOCK_NB) != 0) {
         printf("error: the file %s is not locked\n", LOCK_FILE);
         goto err;
    }

    set_global_astream_log_level(ASTREAM_LOG_INFO);

    /* verify the permission of the user */
    if (geteuid() != 0) {
        printf("error: please run astream daemon under root\n");
        goto err;
    }

    /* parse the arguments from command-line. */
    ret = parse_cmdline(argc, argv, &help);
    if (help || ret < 0) {
        goto err;
    }

    /* start this process with daemon. 
     * the first argument denote the daemon uses the current directory as its working 
     * directory，but not the root; the second argument means redirect all 
     * the output message to /dev/null, so use syslog instead.
     */
    ret = daemon(1, 0);
    if (ret < 0) {
        printf("error: failed to start the astream daemon\n");
        goto err;
    }

    /* get the pid of this dameon, and store it into the LOCK_FILE. */
    len = snprintf(buf, MAX_PID_BUFFER_SIZE, "%d\n", (int)getpid());
    write(fp->_fileno, buf, len);

    signal(SIGUSR1, signalHandler);

    /* start to watch the directories. */
    start_inotify(argc);

    return 0;
err:
    if(fp)
        fclose(fp);
    return -1;
}