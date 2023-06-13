#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#if defined(_WIN64) && !defined(__CYGWIN__)
#include "getopt.h"
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <sys/random.h>
#endif

#include "util.h"

#include "gmp256k1/Int.h"
#include "gmp256k1/IntGroup.h"
#include "gmp256k1/Random.h"
#include "SaveProgress.h"

SaveProgress *spA,*spB;

int main(int argc, char **argv)	{
	if(argc < 3)	{
		fprintf(stderr,"Missing argument");
		exit(EXIT_FAILURE);
	}
	spA = new SaveProgress(argv[1]);
	spB = new SaveProgress(argv[2]);
	/*
		Here we need to check if the ranges of the files overlaps
		If they overlaps Merge B in to A
		This is A <- B
		or argv[1] <- argv[2]
		
		By the momment if type mismatch send error...
		we can handle that in some future update
	*/
	
	
}