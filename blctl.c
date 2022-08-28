#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "blinker.h"

//Print the usage string, if error is not NULL, the string is preceded by:
//"Error: %{the error}\n"
void print_usage(const char *pname, const char *error)
{
	if (error)
		fprintf(stderr, "Error: %s\nUsage: %s <get | set> <var> <val?>\n",
				error, pname);
	else
		printf("Usage: %s <get | set> <var> <val>\n", pname);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	if (argc < 2) {
		print_usage(argv[0], "An operation is needed");
		return 1;
	}
	if (argc < 3) {
		print_usage(argv[1], "A variable is needed");
		return 1;
	}
	if (!strcmp(argv[1], "--help")) {
		print_usage(argv[0], NULL);
		goto ret;
	}
	int fd = open("/dev/blinker", 0);
	if (fd < 0) {
		ret = 1;
		fprintf(stderr, "Failed to open /dev/blinker");
		goto ret;
	}
	if (!strcmp(argv[1], "set")) {
		if (argc < 4) {
			print_usage(argv[1], "the set operation requires a value");
			ret = 1;
			goto fd_close_ret;
		}
		if (!strcmp(argv[1], "pin")) {
			goto fd_close_ret;
		}
		if (!strcmp(argv[1], "sleep")) {
			goto fd_close_ret;
		}
	}
	if (!strcmp(argv[1], "get")) {
		if (!strcmp(argv[2], "pin")) {
			int result = 0;
			if (ioctl(fd, BLINKER_GET_PIN, &result)) {
				fprintf(stderr, "Failed to run ioctl\n");
				puts(strerror(ioctl(result, BLINKER_GET_PIN, &result)));
				ret = 1;
			} else {
				printf("The current pin is: %d\n", result);
			}
			goto fd_close_ret;
		}
		if (!strcmp(argv[2], "sleep")) {
			unsigned long result = 0;
			if (ioctl(fd, BLINKER_GET_SLEEP, &result)) {
				fprintf(stderr, "Failed to run ioctl\n");
				ret = 1;
			} else {
				printf("The current sleep time is: %ld ms\n",
						result);
			}
			goto fd_close_ret;
		}
		usage_error(argv[0], "Unknown variable");
		ret = 1;
		goto fd_close_ret;
	}
	ret = 1;
	usage_error(argv[1], "Invalid variable")
fd_close_ret: close(fd);
ret: return ret;
}
