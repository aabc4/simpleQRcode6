
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>

typedef unsigned int UINT32;
typedef int INT32;
typedef unsigned short UINT16;
typedef unsigned char byte;
//maximum possible number of modules in a QR code,+1
#define maxQRmodules 31330
//maximum possible number of data+EC codewords,+1
#define const_maxcodewords 3707
//maximum possible number of EC codewords for a block,+1
#define maxECcodewords 31
#define DARK 0x00000000
#define DDAT 0x00000001
#define UNUS 0x00000002
#define RESR 0x00FFFFFD
#define LDAT 0x00FFFFFE
#define LITE 0x00FFFFFF
#define CMSK 0x00FFFF00

UINT32 matrix[maxQRmodules];

UINT32 dark = DARK;  UINT32 lite = LITE;  UINT32 unus = UNUS;
UINT32 ddat = DDAT;  UINT32 ldat = LDAT;  UINT32 resr = RESR;

INT32 version;
INT32 mode; //0:numeric, 1: alphanumeric, 2: byte, 3:kanji, 4: ECI
INT32 maxcodewords;
INT32 eclevel;
INT32 maskpattern;
int size; 
int size2;

int minimalpenaltymask();
unsigned short numdatacodewords();
byte numblocksingroup1();
byte group1datacodewords();
byte numblocksingroup2();
byte group2datacodewords();
int eccodewordsperblock();
int getblockpenalty(); int getratiopenalty(); int getsequencepenalty(); int getstraightpenalty();
int gettotalpenalty();
void reserveareas();
void drawversion();
void drawfinders();
void drawtiming();
void drawalign();
void drawdarkmodule(); void mask(int);
int makeQR(const char *theinput, int theinputlength, int theecimode, bool makenegative);
/*
blocksbyversionandeclevel
rowmajor - version
rowminor - eclevel
columns: number of blocks in group 1,
	Number of Data Codewords in Each of Group 1's Blocks,
	number of blocks in group 2,
*/
byte bigtable[] = {
1,19,0,1,16,0,1,13,0,1,9,0,1,34,0,1,28,0,1,22,0,1,16,0,1,55,0,1,44,0,2,17,0,2,13,0,1,80,
0,2,32,0,2,24,0,4,9,0,1,108,0,2,43,0,2,15,2,2,11,2,2,68,0,4,27,0,4,19,0,4,15,0,2,78,0,4,
31,0,2,14,4,4,13,1,2,97,0,2,38,2,4,18,2,4,14,2,2,116,0,3,36,2,4,16,4,4,12,4,2,68,2,4,43,
1,6,19,2,6,15,2,4,81,0,1,50,4,4,22,4,3,12,8,2,92,2,6,36,2,4,20,6,7,14,4,4,107,0,8,37,1,8,
20,4,12,11,4,3,115,1,4,40,5,11,16,5,11,12,5,5,87,1,5,41,5,5,24,7,11,12,7,5,98,1,7,45,3,
15,19,2,3,15,13,1,107,5,10,46,1,1,22,15,2,14,17,5,120,1,9,43,4,17,22,1,2,14,19,3,113,4,3,
44,11,17,21,4,9,13,16,3,107,5,3,41,13,15,24,5,15,15,10,4,116,4,17,42,0,17,22,6,19,16,6,2,
111,7,17,46,0,7,24,16,34,13,0,4,121,5,4,47,14,11,24,14,16,15,14,6,117,4,6,45,14,11,24,16,
30,16,2,8,106,4,8,47,13,7,24,22,22,15,13,10,114,2,19,46,4,28,22,6,33,16,4,8,122,4,22,45,
3,8,23,26,12,15,28,3,117,10,3,45,23,4,24,31,11,15,31,7,116,7,21,45,7,1,23,37,19,15,26,5,
115,10,19,47,10,15,24,25,23,15,25,13,115,3,2,46,29,42,24,1,23,15,28,17,115,0,10,46,23,10,
24,35,19,15,35,17,115,1,14,46,21,29,24,19,11,15,46,13,115,6,14,46,23,44,24,7,59,16,1,12,
121,7,12,47,26,39,24,14,22,15,41,6,121,14,6,47,34,46,24,10,2,15,64,17,122,4,29,46,14,49,
24,10,24,15,46,4,122,18,13,46,32,48,24,14,42,15,32,20,117,4,40,47,7,43,24,22,10,15,67,19,
118,6,18,47,31,34,24,34,20,15,61 };

UINT16 formatstrings[] = { 0x77C4, 0x72F3, 0x7DAA, 0x789D, 0x662F, 0x6318, 0x6C41, 0x6976,
	0x5412, 0x5125, 0x5E7C, 0x5B4B, 0x45F9, 0x40CE, 0x4F97, 0x4AA0, 0x355F, 0x3068, 0x3F31,
	0x3A06, 0x24B4, 0x2183, 0x2EDA, 0x2BED, 0x1689, 0x13BE, 0x1CE7, 0x19D0, 0x762, 0x255, 0xD0C, 0x83B };
UINT32 versionstrings[] = { 0x7C94, 0x85BC, 0x9A99, 0xA4D3, 0xBBF6, 0xC762, 0xD847, 0xE60D,
	0xF928,	0x10B78, 0x1145D, 0x12A17, 0x13532, 0x149A6, 0x15683, 0x168C9, 0x177EC, 0x18EC4,
	0x191E1,0x1AFAB, 0x1B08E, 0x1CC1A, 0x1D33F, 0x1ED75, 0x1F250, 0x209D5, 0x216F0, 0x228BA,
	0x2379F,0x24B0B,0x2542E,0x26A64,0x27541,0x28C69 };

byte mod285logs[] = { 1, 2, 4, 8, 16, 32, 64, 128, 29, 58, 116, 232, 205, 135, 19, 38, 76, 152, 45,
		90, 180, 117, 234, 201, 143, 3, 6, 12, 24, 48, 96, 192, 157, 39, 78, 156, 37, 74, 148, 53, 106,
		212, 181, 119, 238, 193, 159, 35, 70, 140, 5, 10, 20, 40, 80, 160, 93, 186, 105, 210, 185, 111,
		222, 161, 95, 190, 97, 194, 153, 47, 94, 188, 101, 202, 137, 15, 30, 60, 120, 240, 253, 231, 211,
		187, 107, 214, 177, 127, 254, 225, 223, 163, 91, 182, 113, 226, 217, 175, 67, 134, 17, 34, 68, 136,
		13, 26, 52, 104, 208, 189, 103, 206, 129, 31, 62, 124, 248, 237, 199, 147, 59, 118, 236, 197, 151,
		51, 102, 204, 133, 23, 46, 92, 184, 109, 218, 169, 79, 158, 33, 66, 132, 21, 42, 84, 168, 77, 154,
		41, 82, 164, 85, 170, 73, 146, 57, 114, 228, 213, 183, 115, 230, 209, 191, 99, 198, 145, 63, 126,
		252, 229, 215, 179, 123, 246, 241, 255, 227, 219, 171, 75, 150, 49, 98, 196, 149, 55, 110, 220,
		165, 87, 174, 65, 130, 25, 50, 100, 200, 141, 7, 14, 28, 56, 112, 224, 221, 167, 83, 166, 81, 162,
		89, 178, 121, 242, 249, 239, 195, 155, 43, 86, 172, 69, 138, 9, 18, 36, 72, 144, 61, 122, 244, 245,
		247, 243, 251, 235, 203, 139, 11, 22, 44, 88, 176, 125, 250, 233, 207, 131, 27, 54, 108, 216, 173, 71, 142, 1 };
byte mod285antilogs[] = { 0, 0, 1, 25, 2, 50, 26, 198, 3, 223, 51, 238, 27, 104, 199, 75, 4, 100, 224, 14, 52,
	141, 239, 129, 28, 193, 105, 248, 200, 8, 76, 113, 5, 138, 101, 47, 225, 36, 15, 33, 53, 147, 142, 218, 240, 18,
	130, 69, 29, 181, 194, 125, 106, 39, 249, 185, 201, 154, 9, 120, 77, 228, 114, 166, 6, 191, 139, 98, 102, 221,
	48, 253, 226, 152, 37, 179, 16, 145, 34, 136, 54, 208, 148, 206, 143, 150, 219, 189, 241, 210, 19, 92, 131, 56,
	70, 64, 30, 66, 182, 163, 195, 72, 126, 110, 107, 58, 40, 84, 250, 133, 186, 61, 202, 94, 155, 159, 10, 21, 121,
	43, 78, 212, 229, 172, 115, 243, 167, 87, 7, 112, 192, 247, 140, 128, 99, 13, 103, 74, 222, 237, 49, 197, 254, 24,
	227, 165, 153, 119, 38, 184, 180, 124, 17, 68, 146, 217, 35, 32, 137, 46, 55, 63, 209, 91, 149, 188, 207, 205, 144,
	135, 151, 178, 220, 252, 190, 97, 242, 86, 211, 171, 20, 42, 93, 158, 132, 60, 57, 83, 71, 109, 65, 162, 31, 45, 67,
	216, 183, 123, 164, 118, 196, 23, 73, 236, 127, 12, 111, 246, 108, 161, 59, 82, 41, 157, 85, 170, 251, 96, 134, 177,
	187, 204, 62, 90, 203, 89, 95, 176, 156, 169, 160, 81, 11, 245, 22, 235, 122, 117, 44, 215, 79, 174, 213, 233, 230,
	231, 173, 232, 116, 214, 244, 234, 168, 80, 88, 175 };
byte charcountlengths[] = { 10,9,8,8,12,11,16,10,14,13,16,12 };
byte getcharcountlength() {
	int i = 0; if (version > 9) i++; if (version > 26) i++;
	return charcountlengths[i * 4 + mode];
}

byte genpoly[maxECcodewords]; 
void makeECgenerator(int ord) {
	byte multi = 0; genpoly[0] = 0;
	for (int order = 2; order <= ord; order++) {
		multi = (byte)(order - 1);
		for (int i = order; i > 0; i--) genpoly[i] = genpoly[i - 1];
		genpoly[0] = (byte)((multi + genpoly[0]) % 255);
		for (int i = 1; i < order; i++)
			genpoly[i] = mod285antilogs[mod285logs[genpoly[i]] ^ mod285logs[(multi + genpoly[i + 1]) % 255]];
	}
}

typedef struct mbs {
	int bytecounter = 0;
	UINT32 truebitcounter = 0;
	byte bitcounter = 0;
	byte data[const_maxcodewords] = {};
} MyBitSet;

void drawdata(MyBitSet*);
MyBitSet datcodewords;
UINT32 getdrawbit(MyBitSet* mbs, int index) {
	int thebyte = index >> 3; 
	int thebit = index & 7;
	return ((mbs->data[thebyte] >> (7- thebit)) & 1) == 1 ? ddat : ldat;
}
void appendtobitset(MyBitSet* mbs, unsigned int dat, byte numbits) {
	for (int i = numbits - 1; i >= 0; i--) {
		if (1 == (1 & (dat >> i))) {
			mbs->data[mbs->bytecounter] |= (byte)(1 << (7 - mbs->bitcounter));
		}
		else {
			mbs->data[mbs->bytecounter] &= (byte)(~(1 << (7 - mbs->bitcounter)));
		};
		mbs->bitcounter = 7 & (mbs->bitcounter + 1);
		mbs->truebitcounter += 1;
		if (0 == mbs->bitcounter) {
			mbs->bytecounter += 1;
		};
	}
}

byte alphamode[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
int alphaindex(byte x) {
	int j; for (j = 0; (x != alphamode[j]) && (j < 46); j++);
	return j;
}

int __cdecl main(int argc, char** argv) { 
	makeQR(argv[argc-1], std::stoi(argv[argc - 2]), std::stoi(argv[argc - 3]), false);
	char blank[] = "#########################################\n";
	char sp[] = "####";
	for (int i = 0; i < 9; i++) {
		if (i != 4) { for (int j = 0; j < size + 8; j++) printf("#"); printf("\n"); }
		else { for (int j = 0; j < size; j++) {
			printf(sp);
			for (int k = 0; k < size; k++)
				printf((matrix[j * size + k] & CMSK) == 0 ? " " : "#");
		    printf(sp);
			printf("\n");
		} };
	};
	return 0; }

#define MAXNUMERICINPUTLENGTH 7089
#define MAXALPHAINPUTLENGTH 4296
#define MAXBYTEINPUTLENGTH 2953

MyBitSet ileaveddata;
byte message[154];

int makeQR(const char *theinput, int theinputlength, int theecimode, bool makenegative){
	if (theinputlength > MAXNUMERICINPUTLENGTH) return -1;
	mode = 0;
	int requiredbitslength = 0;   
	for (int i = 0; theinput[i] != 0; i++) {
		int j = alphaindex(theinput[i]);
		if (j>9) mode = 1;
		if (j>=46) { mode = 2; break; };
	}
	if (mode == 1) if (theinputlength > MAXALPHAINPUTLENGTH) return -1;
	if (mode == 2) if (theinputlength > MAXBYTEINPUTLENGTH) return -1;
	if (theecimode !=-1 ) for (int i = 0; i < theinputlength; i++) if (theinput[i] == '\\') theinputlength++;
	int unusedbits = 0;
	int charcountlength = 0;
	for (version = 1; version <= 40; version++)
		for (eclevel = 3; eclevel >= 0; eclevel--) {
			size = 21 + (version - 1) * 4; size2 = size * size;
			for (int i = 0; i < maxQRmodules; i++) matrix[i] = unus;//memset(matrix, unus, maxQRmodules);
			reserveareas();	drawversion(); drawfinders(); 
			drawtiming(); drawalign(); drawdarkmodule();
			
			unusedbits = 0;
			for (int i = 0; i < size2; i++) if (matrix[i] == unus) unusedbits++;

			maxcodewords = unusedbits >> 3;
			requiredbitslength = numdatacodewords() << 3;
			charcountlength = getcharcountlength();
			int availablebits = requiredbitslength - 4 - (-1!=theecimode ? 12 : 0) - charcountlength;
			int numchars = 3 * availablebits / 10;
			int alphachars = 2 * availablebits / 11;
			int bytechars = availablebits >> 3;
			if ((mode == 0 && numchars >= theinputlength)
				|| (mode == 1 && alphachars >= theinputlength)
				|| (mode == 2 && bytechars >= theinputlength)) goto Loopfinished;
		};
Loopfinished:
	if (version > 40) { return -1; }; 

	byte themodes[] = { 1,2,4,8,7 };
	if (theecimode!=-1) {
		appendtobitset(&datcodewords,7, 4);
		appendtobitset(&datcodewords,theecimode, 8);
	};
	appendtobitset(&datcodewords,themodes[mode], 4);
	appendtobitset(&datcodewords,theinputlength, charcountlength);
	int n = 0;
#define numericoffset 48
	unsigned int nextdata;
	if (mode == 0) for (n = 0; n < theinputlength;n++) {//numeric mode
		if (1 == theinputlength - n) {
			nextdata = (((byte)theinput[n]) - numericoffset);
			appendtobitset(&datcodewords,nextdata, 4);
		};
		if (2 == theinputlength - n) {
			nextdata = ((((byte)theinput[n]) - numericoffset) * 10);
			nextdata += (((byte)theinput[++n]) - numericoffset);
			appendtobitset(&datcodewords,nextdata, 7);
		};
		if (3 <= theinputlength - n) {
			nextdata = ((((byte)theinput[n]) - numericoffset) * 100);
			nextdata += ((((byte)theinput[++n]) - numericoffset) * 10);
			nextdata += (((byte)theinput[++n]) - numericoffset);
			appendtobitset(&datcodewords,nextdata, 10);
		};
	};
	if (mode == 1) { //alphanumeric mode
		for (n = 0; n < theinputlength; n++) {
			if (theinputlength - 1 == n) {
				if (0 == (n & 1)) {
					nextdata = (alphaindex(theinput[n]));
					appendtobitset(&datcodewords,nextdata, 6);
				}
			}
			else {
				nextdata = (alphaindex(theinput[n]) * 45);
				nextdata += (alphaindex(theinput[++n]));
				appendtobitset(&datcodewords,nextdata, 11);
			};
		};
	};
	if (mode == 2) for (int j = 0; j < theinputlength; j++) {
		appendtobitset(&datcodewords, theinput[j], 8);
		//escape backslashes in utf8 mode by doubling them up
		if ((theecimode !=-1 ) && (theinput[j] == '\\')) appendtobitset(&datcodewords, theinput[j], 8); };

	if (datcodewords.bytecounter < (requiredbitslength >> 3)) appendtobitset(&datcodewords, 0,1);
	while (0 != (datcodewords.bitcounter & 7)) appendtobitset(&datcodewords,0, 1);
	byte padding[] = { 236,17 }; //if message is not long enough, pad it by repeating these bytes
	int z = 1;
	while (datcodewords.bytecounter < (requiredbitslength >> 3)) appendtobitset(&datcodewords,padding[z ^= 1], 8);

	int g2b = numblocksingroup2();
	int g1b = numblocksingroup1();
	int totalblocks = g1b + g2b;
	int g1dc = group1datacodewords();
	int g2dc = group2datacodewords();
	int ecwords = eccodewordsperblock();
	int wordssofar = 0;
	makeECgenerator(ecwords);
	for (int thisblock = 0; thisblock < totalblocks; thisblock++) {
		int thisblockwords = (thisblock < g1b ? g1dc : g2dc);
		for (int i = 0; i < 154; i++) message[i] = 0; //memset(message, 0, 154);
		for (int i = 0; i < thisblockwords; i++)
			message[ecwords + i] = datcodewords.data[wordssofar + thisblockwords - 1 - i];
		wordssofar += thisblockwords;
		for (int ord = ecwords + thisblockwords - 1; ord >= ecwords; ord--) {
			if (message[ord] == 0) continue;
			byte leadalpha = mod285antilogs[message[ord]];
			for (int i = ecwords; i >= 0; i--) message[i + (ord - ecwords)] ^= mod285logs[(genpoly[i] + leadalpha) % 255];
		};
		for (int i = 0; i < ecwords; i++)
			appendtobitset(&datcodewords,message[ecwords - 1 - i], 8);
	};

	for (n = 0; n < g1dc; n++) 
		for(int j = 0; j < totalblocks; j++) 
		appendtobitset(&ileaveddata,datcodewords.data[j * g1dc + n + (j > g1b ? j - g1b : 0)], 8);
	if (g2dc > 0)
		for (int i = 1; i <= g2b; i++)
			appendtobitset(&ileaveddata, datcodewords.data[g1dc * g1b + i * g2dc - 1], 8);

	int startecwords = g1b * g1dc + g2b * g2dc;
	int bk = eccodewordsperblock();
	for (n = 0; n < bk; n++) 
		for (int i = 0; i < totalblocks; i++)
		appendtobitset(&ileaveddata,datcodewords.data[startecwords + i * bk + n], 8);

	int jk = unusedbits - ileaveddata.truebitcounter;
	if (jk > 0) appendtobitset(&ileaveddata, 0, jk);

	drawdata(&ileaveddata);
	maskpattern = minimalpenaltymask();
	mask(maskpattern);

	return 0;
};

void drawdata(MyBitSet* thedata) {
	bool godown = false;
	UINT32 i = 0; int row = size - 1; int col = size - 1;
	while (i < datcodewords.truebitcounter) {
		if (unus == matrix[col + row * size]) matrix[col + row * size] = getdrawbit(thedata, i++);
		bool oddcol = (1 == (1 & col));
		if(col < 6) oddcol=!oddcol;
		if (oddcol) {
			col++;
			if (godown) { row++; }
			else row--;
		} else col--;
		if (row < 0) {
			godown = true; row = 0;
			if (col == 8)col -= 1; col -= 2; };
		if (row >= size) {
			godown = false;
			row = size - 1;
			col -= 2;
			if (col == 5)col -= 1;
			if (col == 6)col -= 2;
		}
	}
}

UINT32 versionstring[18];
void drawversion() {
	if (version < 7) return;
	int i;
	for (i = 0; i <= 17; i++)
		versionstring[i] = (0 != ((versionstrings[version - 7]) & (1 << i))) ? dark : lite;
	i = 0;
	for (int col = 0; col < 6; col++)
		for (int row = size - 11; row < size - 8; row++) {
			matrix[col + row * size] = versionstring[i];
			matrix[row + col * size] = versionstring[i++];
		}
}
int minimalpenaltymask() {
	int i = 0; int tmp = 0;
	int imin = 0; int min = 0;
	for (i = 0; i <= 7; i++) {
		mask(i);
		tmp = gettotalpenalty();
		if (i == 0) min = tmp;
		if (tmp < min) {
			imin = i; min = tmp;
		};
		mask(i);
	}
	return imin;
}

int getblockpenalty() {
	int penalty = 0; UINT32 pp;
	for (int col = size - 2; col >= 0; col--)
		for (int row = size - 2; row >= 0; row--) {
			pp = matrix[col + row * size] & CMSK;
			if (pp == (matrix[col + 1 + row * size] & CMSK))
			if (pp == (matrix[col + size + row * size] & CMSK))
			if (pp == (matrix[col + 1 + size + row * size] & CMSK))
			penalty += 3;
		}
	return penalty;
}

UINT32 formatstring[15];
void drawformat(int masktype) {
	int i;
	for (i = 0; i <= 14; i++)
		formatstring[14 - i] = (0 != ((formatstrings[eclevel * 8 + masktype]) & (1 << i))) ? dark : lite;
	i = 0;
	for (int row = size - 1; row >= 0; row--) {
		matrix[8 + row * size] = formatstring[i++];
		if (row == size - 7) row = 9;
		if (row == 7) row = 6;
	};
	i = 0;
	for (int col = 0; col < size; col++) {
		matrix[col + 8 * size] = formatstring[i++];
		if (col == 5) col = 6;
		if (col == 7) col = size - 9;
	};
}
void mask(int masktype) {
	bool flip = false;
	drawformat(masktype);
	for (int col = 0; col < size; col++) for (int row = 0; row < size; row++) {
		switch (masktype) {
		case 0: flip = (0 == (1 & (row + col))); break;
		case 1: flip = (0 == (1 & row)); break;
		case 2: flip = (0 == (col % 3)); break;
		case 3: flip = (0 == ((row + col) % 3)); break;
		case 4: flip = (0 == (1 & ((row >> 1) + (col / 3)))); break;
		case 5: flip = (0 == (((row * col) & 1)) + ((row * col) % 3)); break;
		case 6: flip = (0 == (1 & (((row * col) & 1) + ((row * col) % 3)))); break;
		case 7: flip = (0 == (1 & (((row + col) & 1) + ((row * col) % 3)))); break;	
		default: return;
		};
			UINT32 qq = matrix[col+row*size];
			if(qq==unus) qq=ldat;
			if(flip) {
				if(qq==ddat){ qq=ldat;
				} else if(qq==ldat) qq=ddat;}; 
			matrix[col + row * size] = qq;
	} }	

int getstraightpenalty() {
	int penalty = 0; int newflags = 3;
	for (int row = size - 5; row >= 0; row--) {
		for (int col = size - 5; col >= 0; col--) {
			UINT32 thischar = matrix[col + row * size] & CMSK;
			if (thischar != (matrix[col + 1 + row * size] & CMSK)) goto norow;
			if (thischar != (matrix[col + 2 + row * size] & CMSK)) goto norow;
			if (thischar != (matrix[col + 3 + row * size] & CMSK)) goto norow;
			if (thischar != (matrix[col + 4 + row * size] & CMSK)) goto norow;
			penalty += (0!=(newflags & 1)) ? 3 : 1;
			newflags = newflags & 2;
			goto rowend;
		norow:
			newflags = newflags | 1;
		rowend:
			UINT32 otherchar = matrix[row + col * size] & CMSK;
			if (otherchar != (matrix[row + (col + 1) * size] & CMSK)) goto nocol;
			if (otherchar != (matrix[row + (col + 2) * size] & CMSK)) goto nocol;
			if (otherchar != (matrix[row + (col + 3) * size] & CMSK)) goto nocol;
			if (otherchar != (matrix[row + (col + 4) * size] & CMSK)) goto nocol;
			penalty += (0!= (newflags & 2)) ? 3 : 1;
			newflags = newflags & 1;
			goto colend;
		nocol:
			newflags = newflags | 2;
		colend:
			if (col == (0)) { newflags = 3; };
		};
	};
	return penalty;
}

UINT32 sequence[] = { DARK,CMSK,DARK,DARK,DARK,CMSK,DARK,CMSK,CMSK,CMSK,CMSK };
int getsequencepenalty() {
	int penalty = 0; int tot1;
	for (int row = 0; row < size; row++) {
		for (int col = 10; col < size; col++) {
			tot1 = 0;
			for (int j = 0; j <= 10; j++) {
				UINT32 thischar = matrix[col - j + row * size] & CMSK;
				if (thischar == sequence[j]) tot1 += 1;
				if (thischar == sequence[j + 4]) tot1 += 16;
				thischar = matrix[row + (col - j) * size] & CMSK;
				if (thischar == sequence[j]) tot1 += 256;
				if (thischar == sequence[j + 4]) tot1 += 4096;
			}
			if ((tot1 & 15) >= 11) penalty += 40;
			if (((tot1 >> 4) & 15) >= 11) penalty += 40;
			if (((tot1 >> 8) & 15) >= 11) penalty += 40;
			if (((tot1 >> 12) & 15) >= 11) penalty += 40;
		}
	}
	return penalty;
}
int getratiopenalty() {
	int darks = 0; int fig1 = 0;;
	for (int i = 0; i < size * size; i++)
		if ((matrix[i] & CMSK) == DARK) darks += 1;
	darks *= 100; darks /= size * size; darks -= 50;
	for (darks = darks>=0?darks:-darks; darks > 5; darks -= 5)
		fig1 += 10;
	return fig1; 
}

int gettotalpenalty() {
	return getblockpenalty() + getstraightpenalty() +
		getsequencepenalty() + getratiopenalty();
}
void fillbox(int startx, int starty, int boxsize, UINT32 content) {
	for (int i = 0; i < boxsize; i++) for (int j = 0; j < boxsize; j++)
		matrix[size * (starty + j) + (startx + i)] = content;
}

void drawtiming() {
	for (int i = 8; i < size - 8; i++) {
		matrix[i + 6 * size] = (1 == (i & 1)) ? lite : dark;
		matrix[6 + i * size] = matrix[i + 6 * size];
	}
}

void drawdarkmodule() {
	fillbox(8, size - 8, 1, dark);
}

void drawfinders() {
	fillbox(0, 0, 8, lite); fillbox(0, 0, 7, dark);
	fillbox(1, 1, 5, lite); fillbox(2, 2, 3, dark);
	fillbox(0, size - 8, 8, lite); fillbox(0, size - 7, 7, dark);
	fillbox(1, size - 5 - 1, 5, lite); fillbox(2, size - 3 - 2, 3, dark);
	fillbox(size - 8, 0, 8, lite); fillbox(size - 7, 0, 7, dark);
	fillbox(size - 5 - 1, 1, 5, lite); fillbox(size - 3 - 2, 2, 3, dark);
}

void reserveareas() {
	fillbox(0, 0, 9, resr);
	fillbox(1, size - 8, 8, resr);
	fillbox(size - 8, 1, 8, resr);
	if (version >= 7) {
		fillbox(0, size - 11, 3, resr);
		fillbox(3, size - 11, 3, resr);
		fillbox(size - 11, 0, 3, resr);
		fillbox(size - 11, 3, 3, resr);
	}
}

byte diffs[] = { 24,24,26,26,26,28,28 };
byte places[] = { 6,0,0,0,0,0,0 };
void drawalign() {
	if (version <= 1) return; int div = version / 7;
	int k = 0;
	places[1] = 10 + version * 4;
	if (div == 1) k = 2 * (version + 1); 
	if (div == 2) k = 2 * (version - 4); 
	if (div == 3) k = 2 * (version - 10);
	if (div == 4) k = diffs[version - 28];
	if (div == 5) k = diffs[version - 34];
	for (int i = 1; i <= div; i++)
		places[i + 1] = places[i] - k;
	for (int i = 0; i < 7 && 0 != places[i]; i++)
		for (int j = 0; j < 7 && 0 != places[j]; j++) {
			if (i == 0 && j <= 1) continue;
			if (i == 1 && j == 0) continue;
			fillbox(places[i] - 2, places[j] - 2, 5, dark);
			fillbox(places[i] - 1, places[j] - 1, 3, lite);
			fillbox(places[i], places[j], 1, dark);
		}
}

byte lbt(int f) { return bigtable[(version - 1) * 12 + eclevel * 3 + f]; }
byte numblocksingroup1() { return lbt(0); }
byte group1datacodewords() { return lbt(1); }
byte numblocksingroup2() { return lbt(2); }
byte group2datacodewords() { return numblocksingroup2() == 0 ? 0 : 1 + group1datacodewords(); }

int eccodewordsperblock() {
	return (maxcodewords - numdatacodewords()) /
		(numblocksingroup1() + numblocksingroup2());
}
unsigned short numdatacodewords() {
	return (unsigned short)(group1datacodewords() * numblocksingroup1() +
		group2datacodewords() * numblocksingroup2());
}
