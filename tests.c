#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
#endif
#include <CuTest.h>
#include <critbit.h>
#include "nosql.h"

#include <string.h>
#include <stdio.h>

#ifndef WIN32
#include <unistd.h>
#define _unlink(s) unlink(s)
#endif


static db_entry mk_entry(const char *str) {
    db_entry ret;
    ret.size = strlen(str)+1;
    ret.data = (void*)str;
    return ret;
}

static void test_nosql_set_get(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    db_entry cur = mk_entry("HODOR");
    set_key(&tbl, "hodor", &cur);
    CuAssertIntEquals(tc, 404, get_key(&tbl, "invalid", &cur));
    memset(&cur, 0, sizeof(cur));
    CuAssertIntEquals(tc, 200, get_key(&tbl, "hodor", &cur));
    CuAssertStrEquals(tc, (const char *)cur.data, "HODOR");
}

static void test_nosql_update(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    db_entry cu1 = mk_entry("HODOR");
    db_entry cu2 = mk_entry("NODOR");
    set_key(&tbl, "hodor", &cu1);
    set_key(&tbl, "hodor", &cu2);
    memset(&cu2, 0, sizeof(cu2));
    CuAssertIntEquals(tc, 200, get_key(&tbl, "hodor", &cu2));
    CuAssertStrEquals(tc, (const char *)cu2.data, "NODOR");
}

static void test_nosql_idempotent(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    const char * strings = "HODOR\0HODOR";
    db_entry cu1 = { 6, (void *)strings };
    db_entry cu2 = { 6, (void *)(strings+6) };
    set_key(&tbl, "hodor", &cu1);
    set_key(&tbl, "hodor", &cu2);
    memset(&cu1, 0, sizeof(cu1));
    CuAssertIntEquals(tc, 200, get_key(&tbl, "hodor", &cu1));
    CuAssertStrEquals(tc, (const char *)cu1.data, "HODOR");
    CuAssertPtrEquals(tc, cu2.data, cu1.data);
}

static void test_replay_log_multi(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    db_entry cur = mk_entry("HODOR");

    _unlink("binlog.test");
    open_log(&tbl, "binlog.test");
    set_key(&tbl, "hodor", &cur);
    cur = mk_entry("NOPE!");
    set_key(&tbl, "hodor", &cur);
    fclose(tbl.binlog);
    tbl.binlog = 0;
    cb_clear(&tbl.trie);
    read_log(&tbl, "binlog.test");
    _unlink("binlog.test");
    memset(&cur, 0, sizeof(cur));
    CuAssertIntEquals(tc, 200, get_key(&tbl, "hodor", &cur));
    CuAssertStrEquals(tc, "NOPE!", cur.data);
}

static void test_replay_log(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    db_entry cur = mk_entry("HODOR");

    _unlink("binlog.test");
    open_log(&tbl, "binlog.test");
    set_key(&tbl, "hodor", &cur);
    fclose(tbl.binlog);
    tbl.binlog = 0;
    cb_clear(&tbl.trie);
    read_log(&tbl, "binlog.test");
    _unlink("binlog.test");
    memset(&cur, 0, sizeof(cur));
    CuAssertIntEquals(tc, 200, get_key(&tbl, "hodor", &cur));
    CuAssertStrEquals(tc, "HODOR", (const char *)cur.data);
}

void add_suite_critbit(CuSuite *suite);

int main(void) {
    CuString *output = CuStringNew();
    CuSuite *suite = CuSuiteNew();

    add_suite_critbit(suite);
    SUITE_ADD_TEST(suite, test_nosql_set_get);
    SUITE_ADD_TEST(suite, test_nosql_idempotent);
    SUITE_ADD_TEST(suite, test_nosql_update);
    SUITE_ADD_TEST(suite, test_replay_log);
    SUITE_ADD_TEST(suite, test_replay_log_multi);

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    return suite->failCount;
}
