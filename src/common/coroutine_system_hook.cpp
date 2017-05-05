/*
 * Tencent is pleased to support the open source community by making Pebble available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 *
 */

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/un.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common/coroutine.h"


namespace pebble {

struct rpchook_t
{
    int user_flag;
    struct sockaddr_in dest; // maybe sockaddr_un;
    int domain; // AF_LOCAL , AF_INET

    struct timeval read_timeout;
    struct timeval write_timeout;
};

static inline pid_t GetPid()
{
    char **p = reinterpret_cast<char**>(pthread_self());
    return p ? *reinterpret_cast<pid_t*>(p + 18) : getpid();
}
static rpchook_t *g_rpchook_socket_fd[ 102400 ] = { 0 };

typedef int (*socket_pfn_t)(int domain, int type, int protocol);
typedef int (*connect_pfn_t)(int socket, const struct sockaddr *address, socklen_t address_len);
typedef int (*close_pfn_t)(int fd);

typedef ssize_t (*read_pfn_t)(int fildes, void *buf, size_t nbyte);
typedef ssize_t (*write_pfn_t)(int fildes, const void *buf, size_t nbyte);

typedef ssize_t (*sendto_pfn_t)(int socket, const void *message, size_t length,
                int flags, const struct sockaddr *dest_addr,
                socklen_t dest_len);

typedef ssize_t (*recvfrom_pfn_t)(int socket, void *buffer, size_t length,
                int flags, struct sockaddr *address,
                socklen_t *address_len);

typedef size_t (*send_pfn_t)(int socket, const void *buffer, size_t length, int flags);
typedef ssize_t (*recv_pfn_t)(int socket, void *buffer, size_t length, int flags);

typedef int (*poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);
typedef int (*setsockopt_pfn_t)(int socket, int level, int option_name,
                const void *option_value, socklen_t option_len);

typedef int (*fcntl_pfn_t)(int fildes, int cmd, ...);
typedef struct tm *(*localtime_r_pfn_t)(const time_t *timep, struct tm *result);

typedef void *(*pthread_getspecific_pfn_t)(pthread_key_t key);
typedef int (*pthread_setspecific_pfn_t)(pthread_key_t key, const void *value);

typedef int (*pthread_rwlock_rdlock_pfn_t)(pthread_rwlock_t *rwlock);
typedef int (*pthread_rwlock_wrlock_pfn_t)(pthread_rwlock_t *rwlock);
typedef int (*pthread_rwlock_unlock_pfn_t)(pthread_rwlock_t *rwlock);

static socket_pfn_t g_sys_socket_func     = (socket_pfn_t)dlsym(RTLD_NEXT, "socket");
static connect_pfn_t g_sys_connect_func = (connect_pfn_t)dlsym(RTLD_NEXT, "connect");
static close_pfn_t g_sys_close_func     = (close_pfn_t)dlsym(RTLD_NEXT, "close");

static read_pfn_t g_sys_read_func         = (read_pfn_t)dlsym(RTLD_NEXT, "read");
static write_pfn_t g_sys_write_func     = (write_pfn_t)dlsym(RTLD_NEXT, "write");

static sendto_pfn_t g_sys_sendto_func     = (sendto_pfn_t)dlsym(RTLD_NEXT, "sendto");
static recvfrom_pfn_t g_sys_recvfrom_func = (recvfrom_pfn_t)dlsym(RTLD_NEXT, "recvfrom");

static send_pfn_t g_sys_send_func         = (send_pfn_t)dlsym(RTLD_NEXT, "send");
static recv_pfn_t g_sys_recv_func         = (recv_pfn_t)dlsym(RTLD_NEXT, "recv");

static poll_pfn_t g_sys_poll_func         = (poll_pfn_t)dlsym(RTLD_NEXT, "poll");

static setsockopt_pfn_t g_sys_setsockopt_func
                                        = (setsockopt_pfn_t)dlsym(RTLD_NEXT, "setsockopt");
static fcntl_pfn_t g_sys_fcntl_func     = (fcntl_pfn_t)dlsym(RTLD_NEXT, "fcntl");

static inline unsigned long long get_tick_count() {
    uint32_t lo, hi;
    __asm__ __volatile__ (
            "rdtscp" : "=a"(lo), "=d"(hi)
            );
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

#define HOOK_SYS_FUNC(name) \
    if (!g_sys_##name##_func) { \
        g_sys_##name##_func = (name##_pfn_t)dlsym(RTLD_NEXT, #name); \
    }

static inline int64_t diff_ms(const struct timeval &begin, const struct timeval &end) {
    int64_t u = (end.tv_sec - begin.tv_sec);
    u *= 1000 * 10;
    u += (end.tv_usec - begin.tv_usec) / 100;
    return u;
}

static inline rpchook_t* get_by_fd(int fd)
{
    int array_size = sizeof(g_rpchook_socket_fd) / sizeof(g_rpchook_socket_fd[0]);
    if (fd > -1 && fd < array_size) {
        return g_rpchook_socket_fd[fd];
    }
    return NULL;
}

static inline rpchook_t* alloc_by_fd(int fd)
{
    int array_size = sizeof(g_rpchook_socket_fd) / sizeof(g_rpchook_socket_fd[0]);
    if (fd > -1 && fd < array_size) {
        rpchook_t *lp = reinterpret_cast<rpchook_t*>(calloc(1, sizeof(rpchook_t)));
        lp->read_timeout.tv_sec = 1;
        lp->write_timeout.tv_sec = 1;
        g_rpchook_socket_fd[fd] = lp;
        return lp;
    }
    return NULL;
}

static inline void free_by_fd(int fd)
{
    int array_size = sizeof(g_rpchook_socket_fd) / sizeof(g_rpchook_socket_fd[0]);
    if (fd > -1 && fd < array_size) {
        rpchook_t *lp = g_rpchook_socket_fd[fd];
        if (lp) {
            g_rpchook_socket_fd[fd] = NULL;
            free(lp);
        }
    }
    return;
}

int socket(int domain, int type, int protocol)
{
    HOOK_SYS_FUNC(socket);

    if (!co_is_enable_sys_hook()) {
        return g_sys_socket_func(domain, type, protocol);
    }
    int fd = g_sys_socket_func(domain, type, protocol);
    if (fd < 0) {
        return fd;
    }

    rpchook_t *lp = alloc_by_fd(fd);
    lp->domain = domain;

    fcntl(fd, F_SETFL, g_sys_fcntl_func(fd, F_GETFL, 0));

    return fd;
}

int co_accept(int fd, struct sockaddr *addr, socklen_t *len)
{
    int cli = accept(fd, addr, len);
    if (cli < 0)
    {
        return cli;
    }
    // rpchook_t *lp = alloc_by_fd(cli);
    return cli;
}

int connect(int fd, const struct sockaddr *address, socklen_t address_len)
{
    HOOK_SYS_FUNC(connect);

    if (!co_is_enable_sys_hook()) {
        return g_sys_connect_func(fd, address, address_len);
    }

    int ret = g_sys_connect_func(fd, address, address_len);

    if (address_len == sizeof(sockaddr_un)) {
        const struct sockaddr_un *p = (const struct sockaddr_un *)address;
        if (strstr(p->sun_path, "connagent_unix_domain_socket")) {
        }
    }
    rpchook_t *lp = get_by_fd(fd);
    if (lp) {
        if (sizeof(lp->dest) >= address_len) {
            memcpy(&(lp->dest), address, static_cast<int>(address_len));
        }
    }
    return ret;
}

int close(int fd)
{
    HOOK_SYS_FUNC(close);

    if (!co_is_enable_sys_hook()) {
        return g_sys_close_func(fd);
    }

    free_by_fd(fd);
    int ret = g_sys_close_func(fd);

    return ret;
}
ssize_t read(int fd, void *buf, size_t nbyte)
{
    HOOK_SYS_FUNC(read);

    if (!co_is_enable_sys_hook()) {
        return g_sys_read_func(fd, buf, nbyte);
    }
    rpchook_t *lp = get_by_fd(fd);

    if (!lp || (O_NONBLOCK & lp->user_flag)) {
        ssize_t ret = g_sys_read_func(fd, buf, nbyte);
        return ret;
    }
    int timeout = (lp->read_timeout.tv_sec * 1000)
                + (lp->read_timeout.tv_usec / 1000);

    struct pollfd pf = { 0 };
    pf.fd = fd;
    pf.events = (POLLIN | POLLERR | POLLHUP);

    int pollret = poll(&pf, 1, timeout);

    ssize_t readret = g_sys_read_func(fd, reinterpret_cast<char*>(buf), nbyte);

    if (readret < 0) {
        co_log_err("CO_ERR: read fd %d ret %ld errno %d poll ret %d timeout %d",
                    fd, readret, errno, pollret, timeout);
    }

    return readret;
}
ssize_t write(int fd, const void *buf, size_t nbyte)
{
    HOOK_SYS_FUNC(write);

    if (!co_is_enable_sys_hook()) {
        return g_sys_write_func(fd, buf, nbyte);
    }
    rpchook_t *lp = get_by_fd(fd);

    if (!lp || (O_NONBLOCK & lp->user_flag)) {
        ssize_t ret = g_sys_write_func(fd, buf, nbyte);
        return ret;
    }

    size_t wrotelen = 0;
    int timeout = (lp->write_timeout.tv_sec * 1000)
                + (lp->write_timeout.tv_usec / 1000);

    ssize_t writeret = g_sys_write_func(fd, (const char*)buf + wrotelen, nbyte - wrotelen);

    if (writeret > 0) {
        wrotelen += writeret;
    }
    while (wrotelen < nbyte) {

        struct pollfd pf = {0};
        pf.fd = fd;
        pf.events = (POLLOUT | POLLERR | POLLHUP);
        poll(&pf, 1, timeout);

        writeret = g_sys_write_func(fd, (const char*)buf + wrotelen, nbyte - wrotelen);

        if (writeret <= 0) {
            break;
        }
        wrotelen += writeret;
    }
    return wrotelen;
}

ssize_t sendto(int socket, const void *message, size_t length,
               int flags, const struct sockaddr *dest_addr,
               socklen_t dest_len)
{
    HOOK_SYS_FUNC(sendto);
    if (!co_is_enable_sys_hook()) {
        return g_sys_sendto_func(socket, message, length, flags, dest_addr, dest_len);
    }

    rpchook_t *lp = get_by_fd(socket);
    if (!lp || (O_NONBLOCK & lp->user_flag))
    {
        return g_sys_sendto_func(socket, message, length, flags, dest_addr, dest_len);
    }

    ssize_t ret = g_sys_sendto_func(socket, message, length, flags, dest_addr, dest_len);
    if (ret < 0 && EAGAIN == errno) {
        int timeout = (lp->write_timeout.tv_sec * 1000)
                    + (lp->write_timeout.tv_usec / 1000);

        struct pollfd pf = {0};
        pf.fd = socket;
        pf.events = (POLLOUT | POLLERR | POLLHUP);
        poll(&pf, 1, timeout);

        ret = g_sys_sendto_func(socket, message, length, flags, dest_addr, dest_len);
    }
    return ret;
}

ssize_t recvfrom(int socket, void *buffer, size_t length,
                int flags, struct sockaddr *address,
                socklen_t *address_len)
{
    HOOK_SYS_FUNC(recvfrom);
    if (!co_is_enable_sys_hook()) {
        return g_sys_recvfrom_func(socket, buffer, length, flags, address, address_len);
    }

    rpchook_t *lp = get_by_fd(socket);
    if (!lp || (O_NONBLOCK & lp->user_flag)) {
        return g_sys_recvfrom_func(socket, buffer, length, flags, address, address_len);
    }

    int timeout = (lp->read_timeout.tv_sec * 1000)
                + (lp->read_timeout.tv_usec / 1000);

    struct pollfd pf = {0};
    pf.fd = socket;
    pf.events = (POLLIN | POLLERR | POLLHUP);
    poll(&pf, 1, timeout);

    ssize_t ret = g_sys_recvfrom_func(socket, buffer, length, flags, address, address_len);
    return ret;
}

ssize_t send(int socket, const void *buffer, size_t length, int flags)
{
    HOOK_SYS_FUNC(send);

    if (!co_is_enable_sys_hook()) {
        return g_sys_send_func(socket, buffer, length, flags);
    }
    rpchook_t *lp = get_by_fd(socket);

    if (!lp || (O_NONBLOCK & lp->user_flag)) {
        return g_sys_send_func(socket, buffer, length, flags);
    }
    size_t wrotelen = 0;
    int timeout = (lp->write_timeout.tv_sec * 1000)
                + (lp->write_timeout.tv_usec / 1000);

    ssize_t writeret = g_sys_send_func(socket, buffer, length, flags);

    if (writeret > 0) {
        wrotelen += writeret;
    }

    while (wrotelen < length) {

        struct pollfd pf = {0};
        pf.fd = socket;
        pf.events = (POLLOUT | POLLERR | POLLHUP);
        poll(&pf, 1, timeout);

        writeret = g_sys_send_func(socket,
                (const char*)buffer + wrotelen, length - wrotelen, flags);

        if (writeret <= 0) {
            break;
        }
        wrotelen += writeret;
    }

    return wrotelen;
}

ssize_t recv(int socket, void *buffer, size_t length, int flags)
{
    HOOK_SYS_FUNC(recv);

    if (!co_is_enable_sys_hook()) {
        return g_sys_recv_func(socket, buffer, length, flags);
    }
    rpchook_t *lp = get_by_fd(socket);

    if (!lp || (O_NONBLOCK & lp->user_flag)) {
        return g_sys_recv_func(socket, buffer, length, flags);
    }
    int timeout = (lp->read_timeout.tv_sec * 1000)
                + (lp->read_timeout.tv_usec / 1000);

    struct pollfd pf = {0};
    pf.fd = socket;
    pf.events = (POLLIN | POLLERR | POLLHUP);

    int pollret = poll(&pf, 1, timeout);

    ssize_t readret = g_sys_recv_func(socket, buffer, length, flags);

    if (readret < 0) {
        co_log_err("CO_ERR: read fd %d ret %ld errno %d poll ret %d timeout %d",
                    socket, readret, errno, pollret, timeout);
    }

    return readret;
}

int poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    HOOK_SYS_FUNC(poll);

    if (!co_is_enable_sys_hook()) {
        return g_sys_poll_func(fds, nfds, timeout);
    }

    return co_poll(co_get_epoll_ct(), fds, nfds, timeout);
}
int setsockopt(int fd, int level, int option_name,
               const void *option_value, socklen_t option_len)
{
    HOOK_SYS_FUNC(setsockopt);

    if (!co_is_enable_sys_hook()) {
        return g_sys_setsockopt_func(fd, level, option_name, option_value, option_len);
    }
    rpchook_t *lp = get_by_fd(fd);
    if (lp && SOL_SOCKET == level) {
        struct timeval *val = (struct timeval*)option_value;
        if (SO_RCVTIMEO == option_name) {
            memcpy(&lp->read_timeout, val, sizeof(*val));
        }
        else if (SO_SNDTIMEO == option_name) {
            memcpy(&lp->write_timeout, val, sizeof(*val));
        }
    }
    return g_sys_setsockopt_func(fd, level, option_name, option_value, option_len);
}


int fcntl(int fildes, int cmd, ...)
{
    HOOK_SYS_FUNC(fcntl);

    if (fildes < 0) {
        return __LINE__;
    }

    va_list arg_list;
    va_start(arg_list, cmd);

    int ret = -1;
    rpchook_t *lp = get_by_fd(fildes);
    switch (cmd) {
        case F_DUPFD:
        {
            int param = va_arg(arg_list, int);
            ret = g_sys_fcntl_func(fildes, cmd, param);
            break;
        }
        case F_GETFD:
        {
            ret = g_sys_fcntl_func(fildes, cmd);
            break;
        }
        case F_SETFD:
        {
            int param = va_arg(arg_list, int);
            ret = g_sys_fcntl_func(fildes, cmd, param);
            break;
        }
        case F_GETFL:
        {
            ret = g_sys_fcntl_func(fildes, cmd);
            break;
        }
        case F_SETFL:
        {
            int param = va_arg(arg_list, int);
            int flag = param;
            if (co_is_enable_sys_hook() && lp) {
                flag |= O_NONBLOCK;
            }
            ret = g_sys_fcntl_func(fildes, cmd, flag);
            if (0 == ret && lp) {
                lp->user_flag = param;
            }
            break;
        }
        case F_GETOWN:
        {
            ret = g_sys_fcntl_func(fildes, cmd);
            break;
        }
        case F_SETOWN:
        {
            int param = va_arg(arg_list, int);
            ret = g_sys_fcntl_func(fildes, cmd, param);
            break;
        }
        case F_GETLK:
        {
            struct flock *param = va_arg(arg_list, struct flock *);
            ret = g_sys_fcntl_func(fildes, cmd, param);
            break;
        }
        case F_SETLK:
        {
            struct flock *param = va_arg(arg_list, struct flock *);
            ret = g_sys_fcntl_func(fildes, cmd, param);
            break;
        }
        case F_SETLKW:
        {
            struct flock *param = va_arg(arg_list, struct flock *);
            ret = g_sys_fcntl_func(fildes, cmd, param);
            break;
        }
    }

    va_end(arg_list);

    return ret;
}

} // namespace pebble