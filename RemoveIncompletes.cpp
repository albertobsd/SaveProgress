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

SaveProgress *sp;

int main(int argc, char **argv)	{
	if(argc < 2)	{
		fprintf(stderr,"Missing argument");
		exit(EXIT_FAILURE);
	}
	sp = new SaveProgress(argv[1]);
	sp->RemoveReservedUncompleted();
}