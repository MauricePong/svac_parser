#include "head.h"

int main(int argc, char *argv[])
{
	//if (argc != 3) {
	//	fprintf(stderr,"Invalid parameters,usage:./svac_tool  xxx.svac  yyy.yuv");
	//	return -1;
	//}
	//const char * inputfile = argv[1];
	//const char * outputfile = argv[2];
	//const char * inputfile = "../resource/test.svac";
	//const char * outputfile = "../resource/svac2.videoes";
	const char * inputfile = "../resource/test01.ps";
	const char * outputfile = "../resource/video.h264";
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
	naked_tuple val;
	int i = 0;
	int j = 0;
	size_t len = 0;
	while (!psexit) {
		if ((len = fread(pfptr, 1, 1024, infd)) ==  0) {
			break;
		}
		cout << "read:" << ++j << " times : " << len << "byte" << endl;
		ps->PSWrite(pfptr, 1024);
		val = ps->naked_payload();
		if ((true == get<0>(val)) && (0xE0 == get<1>(val))) {
			fwrite(get<4>(val), 1, len, outfd);
			cout << "write:" << ++i<<" times : "<<len<<"byte"<< endl;
		}
	}
	fclose(infd);
	fclose(outfd);
	delete ps;
	while (1);
	return 0;
}