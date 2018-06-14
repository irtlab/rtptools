#include <sys/time.h>

int
main(int argc, char** argv)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return 0;
}
