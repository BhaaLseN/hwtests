// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <stdio.h>
#include <stdarg.h>
#include <network.h>

#pragma once

#define START_TEST() privStartTest(__FILE__, __LINE__)
#define DO_TEST(condition, fail_msg, ...) privDoTest(condition, __FILE__, __LINE__, fail_msg, __VA_ARGS__)
#define END_TEST() privEndTest()
#define SIMPLE_TEST()

struct TestStatus
{
	TestStatus(const char* file, int line) : num_passes(0), num_failures(0), num_subtests(0), file(file), line(line)
	{
	}

	int num_passes;
	int num_failures;
	int num_subtests;

	const char* file;
	int line;
};

static TestStatus status(NULL, 0);
static int number_of_tests = 0;

extern int client_socket;
extern int server_socket;

static void network_vprintf(const char* str, va_list args)
{
	char buffer[4096];
//	int len = vsnprintf(buffer, 4096, str, args);
	int len = vsprintf(buffer, str, args);
	net_send(client_socket, buffer, len+1, 0);
}

static void network_printf(const char* str, ...)
{
	va_list args;
	va_start(args, str);
	network_vprintf(str, args);
	va_end(args);
}

static void privStartTest(const char* file, int line)
{
	status = TestStatus(file, line);

	number_of_tests++;
}

static void privDoTest(bool condition, const char* file, int line, const char* fail_msg, ...)
{
	va_list arglist;
	va_start(arglist, fail_msg);

	++status.num_subtests;

	if (condition)
	{
		++status.num_passes;
	}
	else
	{
		++status.num_failures;

		// TODO: vprintf forwarding doesn't seem to work?
		network_printf("Subtest %d failed in %s on line %d: ", status.num_subtests, file, line);
		network_vprintf(fail_msg, arglist);
		network_printf("\n");
	}
	va_end(arglist);
}

static void privEndTest()
{
	if (0 == status.num_failures)
	{
		network_printf("Test %d passed (%d subtests)\n", number_of_tests, status.num_subtests);
	}
	else
	{
		network_printf("Test %d failed (%d subtests, %d failures)\n", number_of_tests, status.num_subtests, status.num_failures);
	}
}

static void privSimpleTest(bool condition, const char* file, int line, const char* fail_msg, ...)
{
	// TODO
}

#define SERVER_PORT 16784

static void network_init()
{
	struct sockaddr_in my_name;

	my_name.sin_family = AF_INET;
	my_name.sin_port = htons(SERVER_PORT);
	my_name.sin_addr.s_addr = htonl(INADDR_ANY);

	net_init();

	server_socket = net_socket(AF_INET, SOCK_STREAM, 0);
	int yes = 1;
	net_setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	while(net_bind(server_socket, (struct sockaddr*)&my_name, sizeof(my_name)) < 0)
	{
	}

	net_listen(server_socket, 0);

	struct sockaddr_in client_info;
	socklen_t ssize = sizeof(client_info);
	client_socket = net_accept(server_socket, (struct sockaddr*)&client_info, &ssize);
}

static void network_shutdown()
{
	net_close(client_socket);
	net_close(server_socket);
}