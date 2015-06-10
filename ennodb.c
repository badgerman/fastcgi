#include "cgiapp.h"
#include "nosql.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <critbit.h>

static const char * binlog = "binlog";

static const char * get_prefix(const char *path) {
    const char * result = strrchr(path, '/');
    return result ? result+1 : 0;
}

static int http_success(FCGX_Stream *out, db_entry *body) {
    FCGX_FPrintF(out,
                 "Status: 200 OK\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: %u\r\n"
                 "\r\n", body ? (unsigned int)body->size : 0);
    if (body) {
        FCGX_PutStr(body->data, (int)body->size, out);
    }
    return 200;
}

static int http_not_found(FCGX_Stream *out, const char *body) {
    int http_result = 404;
    size_t len = body ? strlen(body) : 0;

    FCGX_FPrintF(out,
                 "Status: %d Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: %u\r\n"
                 "\r\n"
                 "%s", http_result, (unsigned int)len, body ? body : "");
    return http_result;
}

static void done(void *self) {
    db_table *pl = (db_table *)self;
    fclose(pl->binlog);
    free(self);
}

static void signal_handler(int sig);

static int init(void * self)
{
    db_table *pl = (db_table *)self;
    if (read_log(pl, binlog) < 0) {
        printf("could not read %s, aborting.\n", binlog);
        abort();
    }
    if (open_log(pl, binlog) != 0) {
        printf("could not open %s, aborting.\n", binlog);
        abort();
    }
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);
    return 0;
}

static int process(void *self, FCGX_Request *req)
{
    const char *script, *prefix, *method;
    db_table *pl = (db_table *)self;
    assert(self && req);

    method = FCGX_GetParam("REQUEST_METHOD", req->envp);
    script = FCGX_GetParam("SCRIPT_NAME", req->envp);
    prefix = get_prefix(script);

    printf("%s request for %s\n", method, prefix);
    if (strcmp(method, "GET")==0) {
        db_entry entry;
        if (get_key(pl, prefix, &entry)==200) {
            http_success(req->out, &entry);
        }
        else {
            http_not_found(req->out, "");
        }

    }
    else if (strcmp(method, "POST")==0) {
        char buffer[MAXENTRY];
        db_entry entry;
        size_t size = sizeof(buffer), len = 0;
    
        for (char *b = buffer; size; ) {
            int result = FCGX_GetStr(b, (int)size, req->in);
            if (result > 0) {
                size_t bytes = (size_t)result;
                b+=bytes;
                size-=bytes;
                len+=bytes;
            } else {
                break;
            }
        }
        entry.size = len;
        entry.data = malloc(len);
        memcpy(entry.data, buffer, len);
        set_key(pl, prefix, &entry);
        http_success(req->out, NULL);
    }
    else {
        // invalid method
        http_not_found(req->out, NULL);
    }
    return 0;
}

static struct app myapp = {
    0, init, done, process
};

static void signal_handler(int sig) {
    if (sig==SIGINT) {
        printf("received SIGINT\n");
        done(myapp.data);
        abort();
    }
    if (sig==SIGHUP) {
        printf("received SIGHUP\n");
        fflush(((db_table *)myapp.data)->binlog);
    }
}

struct app * create_app(int argc, char **argv) {
    if (argc>1) {
        binlog = argv[1];
    }
    myapp.data = calloc(1, sizeof(db_table));
    return &myapp;
}
