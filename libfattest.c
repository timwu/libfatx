#include <stdio.h>
#include "libfatx.h"

int
main(int argc, char* argv[])
{
	fatx_t fatx;
	if (argc < 2) {
		printf("Not enough args!\n");
		return -1;
	}
	fatx = fatx_init(argv[1]);
	if (fatx == NULL) {
		printf("Failed to instatiate fatx.\n");
		return -1;
	}
	fatx_printInfo(fatx);
	fatx_free(fatx);
	return 0;
}
