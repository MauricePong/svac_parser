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
	int psexit = 0;
	FILE *infp, *outfp;
	unsigned char esbuf[0x10000] = { 0 };
	int eslen = 0;
	if ((infp = fopen(inputfile, "rb")) == NULL) {
		fprintf(stderr, "%s  was not open", inputfile);
		return -1;
	}

	if ((outfp = fopen(outputfile, "wb+")) == NULL) {
		fprintf(stderr, "%s  was not open", outputfile);
		return -1;
	}

	PsParser *ps = new PsParser();
	if (fread(ps->psbuf, 1, 1024, infp) == 0) {
		::fclose(infp);
		::fclose(outfp);
		cout << "fread erro" << endl;
		delete ps;
		return -1;
	}

	while (0 == psexit) {
		if ((ps->psbuf[0] == 0
			&& ps->psbuf[1] == 0
			&& ps->psbuf[2] == 1
			&& ps->psbuf[3] == 0xB1)) {
			fwrite(ps->psbuf, 1, 4, outfp);
			ps->psbuf += 4; ps->pos += 4;
			//return;
		}else if (ps->psbuf[0] == 0
			&& ps->psbuf[1] == 0
			&& ps->psbuf[2] == 1
			&& ps->psbuf[3] == 0xE0) {
			psexit = ps->getEs(infp, esbuf, eslen);
			int a = eslen / 1024;
			int b = eslen % 1024;
			for (int i = 0; i < a; i++) {
				fwrite(&esbuf[1024*i], 1, 1024, outfp);
			}
			if (b != 0) {
				fwrite(&esbuf[1024*a], 1, b, outfp);
			}
		}// end else if	001e0
		else if (ps->psbuf[0] == 0
			&& ps->psbuf[1] == 0
			&& ps->psbuf[2] == 1
			&& ps->psbuf[3] == 0xC0) {
			//psexit = ps->jump(infp);
			//psexit = ps->getEs(infp, esbuf, eslen);
			psexit = ps->jump(infp);

		}// end else if	001c0
		else
		{
			cout << "jump" << endl;
			psexit = ps->jump(infp);
		}
	}

	::fclose(infp);
	::fclose(outfp);
	delete ps;
	Sleep(10000);
	return 0;
}