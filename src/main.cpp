#include "head.h"

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr,"Invalid parameters,usage:./svac_tool  xxx.svac  yyy.yuv");
		return -1;
	}
	const char * inputfile = argv[1];
	const char * outputfile = argv[2];
	FILE *infd, *outfd;
	if ((infd = fopen(inputfile, "rb")) == NULL) {
		fprintf(stderr, "%s  was not open",inputfile);
		return -1;
	}

	if ((outfd = fopen(outputfile, "wb+")) == NULL) {
		fprintf(stderr, "%s  was not open", outputfile);
		return -1;
	}

	fclose(infd);
	fclose(outfd);
	return 0;
}