#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "CppUTest/CommandLineTestRunner.h"

#include "cmp/cmp.h"
#include "cmp_mem_access/cmp_mem_access.h"
#include "../msg_dispatcher.h"

#define CMP_WRITE_C_STRING(cmp, str) cmp_write_str(cmp, str, strlen(str))

extern "C" void message_a_cb(cmp_ctx_t *cmp, void *arg)
{
    int32_t i;
    bool ret = cmp_read_int(cmp, &i);
    CHECK_TRUE(ret);
    mock().actualCall("callback_a").withParameter("arg", arg).withParameter("cmp", i);
}

extern "C" void message_b_cb(cmp_ctx_t *cmp, void *arg)
{
    int32_t i;
    bool ret = cmp_read_int(cmp, &i);
    CHECK_TRUE(ret);
    mock().actualCall("callback_b").withParameter("arg", arg).withParameter("cmp", i);
}

int arg_a;
int arg_b;

const struct msg_dispatcher_entry_s disp_tab[] = {
    {"callback_a", message_a_cb, &arg_a},
    {"b", message_b_cb, &arg_b},
    {NULL, NULL, NULL}
};

TEST_GROUP(MSGDispatcher)
{
    char buffer[1000];
    cmp_mem_access_t buffer_mem;
    cmp_ctx_t cmp;

    void setup(void)
    {
        cmp_mem_access_init(&cmp, &buffer_mem, buffer, sizeof(buffer));
        cmp_write_array(&cmp, 2); // all datagrams are arrays of two
    }

    void teardown()
    {
        mock().clear();
    }
};

TEST(MSGDispatcher, Nonexistant)
{
    CMP_WRITE_C_STRING(&cmp, "nonexistant");
    cmp_write_s32(&cmp, 42);
    msg_dispatcher(buffer, cmp_mem_access_get_pos(&buffer_mem), disp_tab);
    mock().checkExpectations();
};

TEST(MSGDispatcher, NonexistantPartialMatchShouldNotCall)
{
    CMP_WRITE_C_STRING(&cmp, "callb");
    cmp_write_s32(&cmp, 42);
    msg_dispatcher(buffer, cmp_mem_access_get_pos(&buffer_mem), disp_tab);
    mock().checkExpectations();
};

TEST(MSGDispatcher, DatagramNotArray)
{
    cmp_mem_access_init(&cmp, &buffer_mem, buffer, sizeof(buffer));
    cmp_write_s32(&cmp, 0);
    CMP_WRITE_C_STRING(&cmp, "callback_a");
    cmp_write_s32(&cmp, 42);
    msg_dispatcher(buffer, cmp_mem_access_get_pos(&buffer_mem), disp_tab);
    mock().checkExpectations();
};

TEST(MSGDispatcher, InvalidDatagramArrayLen)
{
    cmp_mem_access_init(&cmp, &buffer_mem, buffer, sizeof(buffer));
    cmp_write_array(&cmp, 1);
    CMP_WRITE_C_STRING(&cmp, "callback_a");
    cmp_write_s32(&cmp, 42);
    msg_dispatcher(buffer, cmp_mem_access_get_pos(&buffer_mem), disp_tab);
    mock().checkExpectations();
};

TEST(MSGDispatcher, CallA)
{
    CMP_WRITE_C_STRING(&cmp, "callback_a");
    cmp_write_s32(&cmp, 42);
    mock().expectOneCall("callback_a").withParameter("arg", &arg_a).withParameter("cmp", 42);
    msg_dispatcher(buffer, cmp_mem_access_get_pos(&buffer_mem), disp_tab);
    mock().checkExpectations();
};

TEST(MSGDispatcher, CallB)
{
    CMP_WRITE_C_STRING(&cmp, "b");
    cmp_write_s32(&cmp, 42);
    mock().expectOneCall("callback_b").withParameter("arg", &arg_b).withParameter("cmp", 42);
    msg_dispatcher(buffer, cmp_mem_access_get_pos(&buffer_mem), disp_tab);
    mock().checkExpectations();
};

