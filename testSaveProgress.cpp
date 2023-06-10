#include<stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
//#include <time.h>

#if defined(_WIN64) && !defined(__CYGWIN__)
#include "getopt.h"
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <sys/random.h>
#endif

#ifdef __unix__
#ifdef __CYGWIN__
#else
#include <linux/random.h>
#endif
#endif


#include "util.h"

#include "gmp256k1/Int.h"
#include "gmp256k1/IntGroup.h"
#include "gmp256k1/Random.h"
#include "SaveProgress.h"


SaveProgress *sp;

int main()	{
	char *temp;
	Int range_start,range_end,subrange_length;
	range_start.SetBase16("20000000000000000");
	range_end.SetBase16("40000000000000000");
	subrange_length.SetBase16("100000000");
	sp = new SaveProgress("keyhunt_saveprogress",true,&range_start,&range_end,&subrange_length,1);
	
	temp = range_start.GetBase16();
	if(sp->CheckReservedSubrange(&range_start))	{
		printf("Range %s is actually Reserved\n",temp);
	}
	else	{
		printf("Range %s is NOT actually Reserved\n",temp);
	}
	free(temp);
	
	temp = range_start.GetBase16();
	if(sp->CheckCompleteSubrange(&range_start))	{
		printf("Range %s is actually Complete\n",temp);
	}
	else	{
		printf("Range %s is NOT actually Complete\n",temp);
	}
	free(temp);
	
	sp->ReserveSubrange(&range_start);
	sp->CompleteSubrange(&range_start);
	
	temp = range_start.GetBase16();
	if(sp->CheckReservedSubrange(&range_start))	{
		printf("Range %s is actually Reserved\n",temp);
	}
	else	{
		printf("Range %s is NOT actually Reserved\n",temp);
	}
	free(temp);
	
	temp = range_start.GetBase16();
	if(sp->CheckCompleteSubrange(&range_start))	{
		printf("Range %s is actually Complete\n",temp);
	}
	else	{
		printf("Range %s is NOT actually Complete\n",temp);
	}
	free(temp);
	
	
	for(int i = 0; i < 15;i++)	{
		range_start.Add(&subrange_length);
		
		temp = range_start.GetBase16();
		if(sp->CheckReservedSubrange(&range_start))	{
			printf("Range %s is actually Reserved\n",temp);
		}
		else	{
			printf("Range %s is NOT actually Reserved\n",temp);
		}
		free(temp);
	
	
		temp = range_start.GetBase16();
		if(sp->CheckCompleteSubrange(&range_start))	{
			printf("Range %s is actually Complete\n",temp);
		}
		else	{
			printf("Range %s is NOT actually Complete\n",temp);
		}
		free(temp);
		
		sp->ReserveSubrange(&range_start);
		sp->CompleteSubrange(&range_start);
		
		temp = range_start.GetBase16();
		if(sp->CheckReservedSubrange(&range_start))	{
			printf("Range %s is actually Reserved\n",temp);
		}
		else	{
			printf("Range %s is NOT actually Reserved\n",temp);
		}
		free(temp);
		
		temp = range_start.GetBase16();
		if(sp->CheckCompleteSubrange(&range_start))	{
			printf("Range %s is actually Complete\n",temp);
		}
		else	{
			printf("Range %s is NOT actually Complete\n",temp);
		}
		free(temp);
	}
}