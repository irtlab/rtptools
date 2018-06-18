#include <unistd.h>
#include <stdlib.h>

int
main(int argc, char** argv)
{
	getopt(argc, argv, "ab:c");
	return 0;
}
