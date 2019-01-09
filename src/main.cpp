#include "head.h"

int main(int argc, char *argv[])
{
	//if (argc != 3) {
	//	fprintf(stderr,"Invalid parameters,usage:./svac_tool  xxx.svac  yyy.yuv");
	//	return -1;
	//}
	//const char * inputfile = argv[1];
	//const char * outputfile = argv[2];
	const char * inputfile = "../resource/test.svac";
	const char * outputfile = "../resource/svac2.videoes";
	bool psexit = false;
	FILE *infd, *outfd;
	char ps_buf[1024] = {0};
	char *pfptr = ps_buf;

	if ((infd = fopen(inputfile, "rb")) == NULL) {
		fprintf(stderr, "%s  was not open",inputfile);
		return -1;
	}

	if ((outfd = fopen(outputfile, "wb+")) == NULL) {
		fprintf(stderr, "%s  was not open", outputfile);
		return -1;
	}

	PsParser * ps = new PsParser();
	fread(pfptr, 1, 1024, infd);

	while (!psexit) {
		//ps->PSWrite(pfptr,);

	}


	fclose(infd);
	fclose(outfd);
	return 0;
}