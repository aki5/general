#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>
int64_t
nsec(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((int64_t)tv.tv_sec * 1000000000) + (int64_t)tv.tv_usec*1000;
}
