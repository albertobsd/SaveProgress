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

SaveProgress *sp1,*sp2;

int main(int argc, char **argv)	{
	if(argc < 3)	{
		fprintf(stderr,"Missing argument");
		exit(EXIT_FAILURE);
	}
	sp1 = new SaveProgress(argv[1]);
	sp2 = new SaveProgress(argv[2]);
	
	/*
		Here we need to check if the ranges of the files overlaps
		If they overlaps Merge B in to A
		or argv[1] <- argv[2]
		
		By the momment if type mismatch send error...
		we can handle that in some future update
	*/
	Int A,B,C,D;
	Int OverlapStart,OverlapEnd;
	char *tempA,*tempB;
	
	A = sp1->GetRangeStart();
	B = sp1->GetRangeEnd();

	C = sp2->GetRangeStart();
	D = sp2->GetRangeEnd();
	/*
	tempA = A.GetBase16();
	tempB = B.GetBase16();
	
	printf("From %s to %s\n",tempA,tempB);
	
	tempA = C.GetBase16();
	tempB = D.GetBase16();
	printf("From %s to %s\n",tempA,tempB);
	*/
	if(B.IsLower(&C) || D.IsLower(&A))	{
		printf("The files ranges don't Overlaps\n");
		//Nothing to do
	}
	else	{
		if(A.IsEqual(&C) && B.IsEqual(&D))	{
			printf("Files are FULL overlap\n");
			//Merge all the file
			OverlapStart = A;
			OverlapEnd = B;
			//Set the memory as aligned
		}
		else	{
			printf("Files Overlap partially\n");
			//Merge only overlaping range
			if(A.IsLowerOrEqual(&C) && C.IsLowerOrEqual(&B) )	{
				OverlapStart = C;
			}
			if(C.IsLowerOrEqual(&A) && A.IsLowerOrEqual(&D) )	{
				OverlapStart = A;
			}
			if(B.IsLowerOrEqual(&D))	{
				OverlapEnd = B;
			}
			else	{
				OverlapEnd = D;
			}
			/* 
				Here we need to determine if the memory is aligned or not
				If it is not aligned we need to determine the BYTE offset from 1 to 7 ?
				
				
				
			*/
			
		}
		//for partially overlap we need to check if the memory is aligned 
		if( 1 /* Check for memory aligned*/ )	{
			/* Get one byte of block of bytes from sp1 and sp2 respectly
			
			Do untile there is no more bytes
			if(Byte1 != byte2)
				byteNew = Byte1 | byte2
				write byteNew On sp1			
			*/
			
		}
		else	{
			/* 
				If the memory is not aligned this mean that there is some Offset in each byte from sp2 to sp1
				And those bit offset inside of each Set are different
				
				example:
				
				OverlapStart on SP1 points to some BYTE in its bit 2
				example bit 2 in SP1
				XX100000
				  |
				  |- Start bit
				  
				But OvrlapStart on SP2 point to some BYTE in its bit different than 2
				
				example bit 6 in SP2
				XXXXXX10
				      |
				      |- Start bit				
				
				In this example we need to calculate and intermediate BYTE
				
				NEWBYTE = BYTE_0_SP2 & 3 // This erase the bit XXXXXX bits on  SP2 byte 0 of the overlap items
				
				NEWBYTE will be 000000??, where the ? are the bits that we want to merge in SP1 BYTE 0 of the overlap
				now we need to move those bits to the aligned position of the BYTE 0 on SP1  
				
				NEWBYTE	<< 4
				
				Result 00??0000 
				
				Now we need to fill the last four bits in NEWBYTE with the first four bits of the BYTE 1 on SP2
				AUXBYTE = ????XXXX  (The XXXX part will be procesed on the next cycle)
				
				AUXBYTE = BYTE_1_SP2 >> 4
				
				Now AUXBYTE = 0000????

				NEWBYTE = NEWBYTE | AUXBYTE this is something like 00??0000 | 0000????
				
				Now NEWBYTE have something like 00??????
				
				We need to do now the Merge with AND operation
				
				NEWBYTE = NEWBYTE | BYTE_0_SP1
				
				NOW we need to validate if BYTE_0_SP1 == NEWBYTE 
				if the condition is TRUE we need to UPDATE the FIRST BYTE on SP1
				
				We need to repeat this process for all the BYTES in the Overlap between SP1 and SP2
								
			*/
			/*
				IF THE BIT offset inside each range is the same, the things are more easy
				
				ONLY first and maybe LAST BYTE need some special calculation
				
				All intermediate bytes can be copy Directly
			*/
		}
	}
	return 0;
}