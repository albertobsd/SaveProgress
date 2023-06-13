/*
	This class is an helper content to Save progress in disk
	- Optionally in RAM at the same time
*/

#ifndef SAVEPROGRESS
#define SAVEPROGRESS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "gmp256k1/Int.h"
#include "gmp256k1/IntGroup.h"
#include "gmp256k1/Random.h"

#define MAGICBYTESLENGTH 24


#define OFFSET_FILE_LENGTH 32
#define OFFSET_FILE_ITEMS_R_USED 48
#define OFFSET_FILE_ITEMS_C_USED 56
#define OFFSET_FILE_LAST_ITEM_F_R 64
#define OFFSET_FILE_LAST_ITEM_B_R 72
#define OFFSET_FILE_LAST_ITEM_F_C 80
#define OFFSET_FILE_LAST_ITEM_B_C 88
#define OFFSET_FILE_RANGE_START 128
#define OFFSET_FILE_DATASETS 224

#define SAVEPROGRESS_BUFFER_LENGTH 524288

class SaveProgress {
	public:
		SaveProgress(const char *tempfilename);
		SaveProgress(const char *prefixfilename,bool ram, Int *range_start, Int *range_end, Int *subrange_lenght,uint8_t type);
		~SaveProgress();

		bool ReserveSubrange(Int *subrange);	//Mark the subrange as Reserved
		bool CompleteSubrange(Int *subrange);	//Mark the subrange as Complete
		
		bool CheckAndReserveSubrange(Int *subrange);	//Check if the range was not reserved previously and mark it as reserved
		
		bool CheckReservedSubrange(Int *subrange);	//Check if the subrange was reserved
		bool CheckCompleteSubrange(Int *subrange);	//Check if the subrange was completed
		
		bool GetAndReserveNextForwardAvailable(Int *subrange);
		bool GetAndReserveNextBackwardAvailable(Int *subrange);
		bool GetAndReserveNextRandomAvailable(Int *subrange);
		void ShowHeaders();
		void ShowProgress();
	
	private:

		bool save_ram;		//Set true if the data need to be stored on RAM

		uint32_t version;	//Maybe useful in the future is there are important changes in this Lib
		uint8_t type;	
					/*
					The Type of save progress is an Important variable because
					it can change how many space is going to be used
						0	ONLY FORWARD or BACKWARD, 
							This only need a few bytes for all the range
							
						1	ALSO RANDOM, This need two bits per subrange in the Main RANGE
							Example: It need 2GB for puzzle 66 with a subrange of 32 bits
					*/
		
		
		uint64_t length;	//length in bytes of the data array
		uint64_t items;		//length in bits of the data array
		
		/*
			items_reserved_used and items_completed_used are only used if type == 1
		*/
		uint64_t items_reserved_used;		//Number of items reserved already used
		uint64_t items_complete_used;		//Number of items complete already used
		
		/*
			last_item_forward_reserve, last_item_backward_reserve
			last_item_forward_complete and last_item_backward_complete 
			are used if type is 1 or 0
		*/
		uint64_t last_item_forward_reserve;			//Last seen unset item from the beggining
		uint64_t last_item_backward_reserve;		//Last seen unset item from the ending

		uint64_t last_item_forward_complete;   //Last seen unset item from the beggining
		uint64_t last_item_backward_complete;  //Last seen unset item from the ending

		Int *range_start;
		Int *range_end;
		Int *subrange_length;

		uint8_t *data_reserved;		// if save_ram is true this value must be innitalized
		uint8_t *data_complete;		// if save_ram is true this value must be innitalized
		
		char *prefix;		// prefix for this file, example keyhunt_reserved, keyhunt_complete
		char *filename;		// filename where the data is going to be saved
		FILE *filedescriptor;
		
		char magicbytes[MAGICBYTESLENGTH];  //MagicBytes for file
		
		//Mutex for I/O operations RAM and FILE
#if defined(_WIN64) && !defined(__CYGWIN__)
		HANDLE mutex;
#else
		pthread_mutex_t *mutex;		
#endif

		void CalculateItems();
		void InitFile();
		void GenerateFileName();
		void UpdateFileCompleteSubrange(uint64_t byte, uint8_t value);
		void UpdateFileReserveSubrange(uint64_t byte, uint8_t value);
		void ReadVariablesFromFile();
		
		
		void UpdateItemsReservedUsed();
		void UpdateLastItemForwardReserve();
		void UpdateLastItemBackwardReserve();
		
		uint8_t ReadByteReserved(uint64_t byte);
		uint8_t ReadByteComplete(uint64_t byte);
		
		void CalculateRange(uint64_t offset,Int *range);
		
		void ReadVariablesFromFileName(const char *filename);
};


/*
File Format headers and data

0               16      24     31  END Offset
|MAGIC BYTES            |V |TYP|     32
| VAR1  | VAR2  | VAR3  | VAR4 |     64
| VAR5  | VAR6  | VAR7  | VAR8 |     96
|000000000000000000000000000000|	128       Fill of zeros for future variables 
|RAW 32 BYTES RANGE START      |    160
|RAW 32 BYTES RANGE END        |    192
|RAW 32 BYTES SUBRANGE LENGTH  |    224
|DATA .........................|           if TYPE is 0x01 this two dataset are going to be in the file
|.....for reserved(Size LENGTH)|
|DATA .........................|  
|.....for complete(Size LENGTH)|



MAGIC BYTES is a 24 bytes char array pading nullbytes of unused bytes
VER is usigned integer of 32 bits (uint32_t)
TYPE is usigned char repeated 4 times to fill the gap up to the first 32 bytes

VAR1 is length
VAR2 is items
VAR3 is items_reserved_used
VAR4 is items_complete_used
VAR5 is last_item_forward_reserved
VAR6 is last_item_backward_reserved
VAR7 is last_item_forward_complete
VAR8 is last_item_backward_complete

Empty 32 bytes fill of NULL this space is reseved for future changes.

RAW 32 BYTES are from the function Int.Get32Bytes()

DATA is only used if the TYPE is SET to 0x01
DATA is divided in two sets
	first set is reserved data
	second set is the complete data
	each set only use 1 bit per subrange

*/

#endif // SAVEPROGRESS