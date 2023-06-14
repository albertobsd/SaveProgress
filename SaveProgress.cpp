#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "SaveProgress.h"
#include "gmp256k1/Int.h"
#include "gmp256k1/IntGroup.h"
#include "gmp256k1/Random.h"



SaveProgress::SaveProgress(const char *filename)	{
	ReadVariablesFromFileName(filename);
}


SaveProgress::SaveProgress(const char *prefixfilename,bool ram, Int *range_start, Int *range_end, Int *subrange_length,uint8_t type)	{
	int len;
	this->version = 0x01000000;
	this->type = type;
	save_ram = ram;
	this->range_start = new Int(range_start);
	this->range_end = new Int(range_end);
	this->subrange_length = new Int(subrange_length);
	items_reserved_used = 0;
	items_complete_used = 0;
	
	CalculateItems();

	len = strlen(prefixfilename);
	
	prefix = (char*) calloc(len+1,sizeof(char));
	if(prefix == NULL)	{
		fprintf(stderr,"[E] Error Calloc\n");
		exit(EXIT_FAILURE);
	}
	memcpy(prefix,prefixfilename,len);
	

	if(save_ram)	{
		data_reserved = (uint8_t*) calloc(length,sizeof(char));
		data_complete = (uint8_t*) calloc(length,sizeof(char));
		if(data_reserved == NULL || data_complete == NULL)	{
			fprintf(stderr,"[E] Error Calloc\n");
			exit(EXIT_FAILURE);
		}
	}
	else	{
		data_reserved = NULL;
		data_complete = NULL;
	}
	
	GenerateFileName();
	
	filedescriptor = fopen(filename,"rb");
	if(filedescriptor == NULL)	{
		// The file doesn't exist we need to initialized it
		InitFile();
	}
	else	{
		printf("File %s already exists reading variables from it... ",filename);
		fflush(stdout);
		ReadVariablesFromFile();
		fclose(filedescriptor);
		printf(" Done!\n");
	}
	/* At this point the file must exist */
	filedescriptor = fopen(filename,"r+b");
	if(filedescriptor == NULL)	{
		//WTF? 	
		fprintf(stderr,"[E] Can't reopen the file\n");
		exit(EXIT_FAILURE);
	}
#if defined(_WIN64) && !defined(__CYGWIN__)
	mutex = CreateMutex(NULL, FALSE, NULL);
	if(mutex == NULL)	{
		fprintf(stderr,"[E] Error CreateMutex\n");
		exit(EXIT_FAILURE);
	}
#else
	mutex =(pthread_mutex_t*) calloc(1,sizeof(pthread_mutex_t));
	if(mutex == NULL)	{
		fprintf(stderr,"[E] Error Calloc\n");
		exit(EXIT_FAILURE);
	}
	pthread_mutex_init(mutex,NULL);
#endif
}

SaveProgress::~SaveProgress()	{
	fclose(filedescriptor);
#if defined(_WIN64) && !defined(__CYGWIN__)
	CloseHandle(mutex);
#else
	pthread_mutex_destroy(mutex);
#endif
	
	if(save_ram)	{
		if(data_reserved != NULL)	{
			free(data_reserved);
		}
		if(data_complete != NULL)	{
			free(data_complete);
		}
	}
	if(prefix != NULL)	{
		free(filename);
	}
	if(filename != NULL)	{
		free(filename);
	}
}

void SaveProgress::CalculateItems()	{
	char *text;
	//int sizeinbits;
	Int diff,r;
	diff.Sub(range_end,range_start);
	Int d(&diff);
	d.Div(subrange_length,&r);
	if(!r.IsZero())	{
		text = subrange_length->GetBase16();
		fprintf(stderr,"[E] Error range_end - range_start is not divible by %s\n",text);
		free(text);
		exit(EXIT_FAILURE);
	}
	//sizeinbits = d.GetBitLength();
	items = d.GetInt64();
	length = (items + 7) / 8;
	uint64_t KB = (2*length) / 1024;
	uint32_t MB = KB / 1024;
	float GB = (float)MB / 1024;
	text =  d.GetBase16();
	printf("[I] Needed number of subranges = %s\n[I] Number of space needed %lu bytes (%lu KB, %u MB %f GB)\n",text,length,KB, MB, GB);
	free(text);
}

void SaveProgress::GenerateFileName()	{
	int len;
	char *str_range_start;
	char *str_range_end;
	char *str_subrange_length;
	
	str_range_start = range_start->GetBase16();
	str_range_end = range_end->GetBase16();
	str_subrange_length = subrange_length->GetBase16();
	len = strlen(prefix) + strlen(str_range_start) +strlen(str_range_end) + strlen(str_subrange_length) + 10;
	filename = (char*) calloc(length,sizeof(char));
	if(filename == NULL)	{
		printf("[E] Error calloc()\n");
		exit(EXIT_FAILURE);
	}
	snprintf(filename,len,"%s_%s_%s_%s.dat",prefix,str_range_start,str_range_end,str_subrange_length);
}



void SaveProgress::InitFile()	{
	uint8_t buffer[96];
	uint64_t varui64 = 0,remaining_bytes = (2*length);
	uint64_t chunk_size = 100 * 1024 * 1024; // 100 MB
	uint64_t num_chunks = (2*length) / chunk_size;
	size_t bytes_written,bytes_to_write;
	uint8_t *null_byte;
	printf("[I] Creating new file %s\n",filename);
	filedescriptor = fopen(filename,"wb");
	if(filedescriptor == NULL)	{
		printf("[E] File can't be created\n");
		exit(EXIT_FAILURE);
	}
	memset(magicbytes,0,MAGICBYTESLENGTH);
	snprintf((char*)magicbytes,MAGICBYTESLENGTH,"%s",prefix);
	
	/* We need to write the Magic bytes...*/
	bytes_written = fwrite(magicbytes, MAGICBYTESLENGTH, 1, filedescriptor);
	if(bytes_written != 1) {
		printf("Failed to write the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	/* Writing the version */
	bytes_written = fwrite(&version, sizeof(uint32_t), 1, filedescriptor);
	if(bytes_written != 1) {
		printf("Failed to write the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	/* Writing the type */
	bytes_written = fwrite(&type, sizeof(uint8_t), 1, filedescriptor);
	if(bytes_written != 1) {
		printf("Failed to write the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	/* Writing some zerobyte 3 times more  just to fill the data*/ 
	bytes_written = fwrite(&varui64, sizeof(uint8_t), 3, filedescriptor);
	if(bytes_written != 3) {
		printf("Failed to write the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	/*
		Is the size of the previous variables don't change in the future we have the next Header
		[MAGIC BYTES (24 bytes)][Version (4 bytes)][Type (1 byte) 00 00 00] Total 32 bytes Header
	*/
	
	
	/* Writing the length */
	bytes_written = fwrite(&length, sizeof(uint64_t), 1, filedescriptor);
	if(bytes_written != 1) {
		printf("Failed to write the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	/* Writing the items */
	bytes_written = fwrite(&items, sizeof(uint64_t), 1, filedescriptor);
	if(bytes_written != 1) {
		printf("Failed to write the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}

	/* 	
		In the InitFile items_reserved_used value is 0 we use this to write 6 times this value
		This include 4 variables and two emptied slots of 64 bits
	*/
	
	
	/* Writing the a 64 bit variable ten times more */
	for(int j = 0; j < 10; j++)	{
		bytes_written = fwrite(&varui64, sizeof(uint64_t), 1, filedescriptor);
		if(bytes_written != 1) {
			printf("Failed to write the file\n");
			fclose(filedescriptor);
			exit(EXIT_FAILURE);
		}
	}
	/*
	
	VAR 1 is LENGTH
	VAR 2 is ITEMS
	
| VAR1  | VAR2  | VAR3  | VAR4 |     64
| VAR5  | VAR6  | VAR7  | VAR8 |     96
|000000000000000000000000000000|	128       Fill of zeros for future variables 

	We need space in file for 6 Variables more (from VAR3 to VAR 8 ) Plus 32 empty bytes more
	
	*/
	
	
	range_start->Get32Bytes(buffer);
	range_end->Get32Bytes(buffer+32);
	subrange_length->Get32Bytes(buffer+64);
	bytes_written = fwrite(buffer, 96, 1, filedescriptor);
	if(bytes_written != 1) {
		printf("Failed to write the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(type){
		//if type is 0x01 we need to fill the needed storage for the full data 
	
		null_byte = (uint8_t*) calloc(chunk_size,sizeof(uint8_t));
		if(null_byte == NULL)	{
			fprintf(stderr,"[E] calloc()\n");
			fclose(filedescriptor);
			exit(EXIT_FAILURE);
		}
		if(length % chunk_size != 0)	{
			num_chunks++;
		}
		for(uint64_t i = 0; i < num_chunks; i++)	{
			bytes_to_write = (remaining_bytes >= chunk_size) ? chunk_size : remaining_bytes;
			bytes_written = fwrite(null_byte, 1, bytes_to_write, filedescriptor);
			if(bytes_written != bytes_to_write) {
				printf("Failed to write the file\n");
				fclose(filedescriptor);
				exit(EXIT_FAILURE);
			}
			remaining_bytes -= bytes_written;
		}
		free(null_byte);
	}
	fclose(filedescriptor);
}

bool SaveProgress::CompleteSubrange(Int *subrange)	{
	if(subrange->IsGreater(range_end))	{
		return false;
	}
	uint64_t bit;
	uint64_t byte;
	uint8_t mask,current,value;
	Int diff;
	diff.Sub(subrange,range_start);
	diff.Div(subrange_length);
	bit = diff.GetInt64();
	byte = bit >> 3;
	mask = 1 << (bit % 8);
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(mutex, INFINITE);
#else
	pthread_mutex_lock(mutex);
#endif
	if(save_ram)	{
		current = data_complete[byte];
	}
	else	{
		current = ReadByteComplete(byte);
	}
	value = current | mask;
	printf("Bit %lu, byte %lu, mask %.2x, current value %.2x new value %.2x\n",bit,byte,mask,current,value);
	fflush(stdout);
	if(value != current)	{
		if(save_ram)	{
			data_complete[byte] = value;
		}
		items_complete_used++;
		UpdateFileCompleteSubrange(byte,value);
	}
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(mutex);
#else
	pthread_mutex_unlock(mutex);
#endif
	return true;
}

void SaveProgress::UpdateFileCompleteSubrange(uint64_t byte, uint8_t value)	{
	/* Writing items_reserved_used */
	if(fseek(filedescriptor,OFFSET_FILE_ITEMS_C_USED,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	if(fwrite(&items_complete_used,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to update the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	uint64_t offset = OFFSET_FILE_DATASETS + length +byte;
	if(fseek(filedescriptor,offset,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		exit(EXIT_FAILURE);
	}
	if(fwrite(&value,sizeof(uint8_t),1,filedescriptor) != 1)	{
		printf("Failed to update the file\n");
		exit(EXIT_FAILURE);
	}
	fflush(filedescriptor);
}

bool SaveProgress::ReserveSubrange(Int *subrange)	{
	if(subrange->IsGreater(range_end))	{
		return false;
	}
	uint64_t bit;
	uint64_t byte;
	uint8_t mask,current,value;
	Int diff;
	diff.Sub(subrange,range_start);
	diff.Div(subrange_length);
	bit = diff.GetInt64();
	byte = bit >> 3;
	mask = 1 << (bit % 8);
	
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(mutex, INFINITE);
#else
	pthread_mutex_lock(mutex);
#endif
	
	if(save_ram)	{
		current = data_reserved[byte];
	}
	else	{
		current = ReadByteReserved(byte);
	}
	value = current | mask;
	/*
	printf("Bit %lu, byte %lu, mask %.2x, current value %.2x new value %.2x\n",bit,byte,mask,current,value);
	fflush(stdout);
	*/
	if(value != current)	{ //In theory the user should not call this function is the subrange is already marked as reserve
		if(save_ram)	{
			data_reserved[byte] = value;
		}
		items_reserved_used++;
		UpdateFileReserveSubrange(byte,value);
	}
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(mutex);
#else
	pthread_mutex_unlock(mutex);
#endif
	return true;
}

void SaveProgress::UpdateFileReserveSubrange(uint64_t byte, uint8_t value)	{
	
	/* Writing items_reserved_used */
	if(fseek(filedescriptor,OFFSET_FILE_ITEMS_R_USED,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	if(fwrite(&items_reserved_used,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to update the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	uint64_t offset = OFFSET_FILE_DATASETS + byte;
	if(fseek(filedescriptor,offset,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	if(fwrite(&value,sizeof(uint8_t),1,filedescriptor) != 1)	{
		printf("Failed to update the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}	
	fflush(filedescriptor);
}

void SaveProgress::ReadVariablesFromFile()	{
	/*
		here we need the current variables in the file and compare it with the expected ones
		if some change we need to abort or check if is possible to continue in some cases
	*/
	Int file_range_start,file_range_end,file_subrange_length;
	char file_magicbytes[24];
	uint64_t file_length,file_items;
	uint32_t file_version;
	uint8_t file_type,bufferInt[32];
	if(fseek(filedescriptor,0,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}

	if(fread(file_magicbytes,24,1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	//memcpy(,file_magicbytes,MAGICBYTESLENGTH);

	if(fread(&file_version,sizeof(uint32_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&file_type,sizeof(uint8_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fseek(filedescriptor,OFFSET_FILE_LENGTH,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&file_length,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&file_items,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}


	if(fread(&items_reserved_used,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	
	if(fread(&items_complete_used,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&last_item_forward_reserve,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&last_item_backward_reserve,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&last_item_forward_complete,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&last_item_backward_complete,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fseek(filedescriptor,OFFSET_FILE_RANGE_START,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}


	if(fread(bufferInt,32,1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	file_range_start.Set32Bytes(bufferInt);
	
	if(fread(bufferInt,32,1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	file_range_end.Set32Bytes(bufferInt);


	if(fread(bufferInt,32,1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}	
	file_subrange_length.Set32Bytes(bufferInt);
	
	if(save_ram && type ){
		if(fread(data_reserved,length,1,filedescriptor) != 1)	{
			printf("Failed to read the file\n");
			fclose(filedescriptor);
			exit(EXIT_FAILURE);
		}
		if(fread(data_complete,length,1,filedescriptor) != 1)	{
			printf("Failed to read the file\n");
			fclose(filedescriptor);
			exit(EXIT_FAILURE);
		}
	}

}


uint8_t SaveProgress::ReadByteReserved(uint64_t byte)	{
	uint8_t currentbyte;
	uint64_t offset  = OFFSET_FILE_DATASETS + byte;
	if(fseek(filedescriptor,offset,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		exit(EXIT_FAILURE);
	}
	if(fread(&currentbyte,sizeof(uint8_t),1,filedescriptor) != 1)	{
		printf("Failed to update the file\n");
		exit(EXIT_FAILURE);
	}
	return currentbyte;
}

uint8_t SaveProgress::ReadByteComplete(uint64_t byte)	{
	uint8_t currentbyte;
	uint64_t offset  = OFFSET_FILE_DATASETS + length + byte;
	if(fseek(filedescriptor,offset,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		exit(EXIT_FAILURE);
	}
	if(fread(&currentbyte,sizeof(uint8_t),1,filedescriptor) != 1)	{
		printf("Failed to update the file\n");
		exit(EXIT_FAILURE);
	}
	return currentbyte;
}

bool SaveProgress::CheckAndReserveSubrange(Int *subrange)	{
	uint64_t bit;
	uint64_t byte;
	uint8_t mask,current,value;
	Int diff;
	diff.Sub(subrange,range_start);
	diff.Div(subrange_length);
	bit = diff.GetInt64();
	byte = bit >> 3;
	mask = 1 << (bit % 8);
	if(save_ram)	{
		//The data should be on RAM
		
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(mutex, INFINITE);
#else
		pthread_mutex_lock(mutex);
#endif
		
		current = data_reserved[byte];
		if(!(current & mask))	{
			value = current | mask;
			data_reserved[byte] = value;
			UpdateFileReserveSubrange(byte,value);
#if defined(_WIN64) && !defined(__CYGWIN__)
			ReleaseMutex(mutex);
#else
			pthread_mutex_unlock(mutex);
#endif
			return true;
		}
		else	{
#if defined(_WIN64) && !defined(__CYGWIN__)
			ReleaseMutex(mutex);
#else
			pthread_mutex_unlock(mutex);
#endif
			return false;
		}
	}
	else	{
		//We need to retreive the data from file
		uint64_t offset  = OFFSET_FILE_DATASETS + byte;
		pthread_mutex_lock(mutex);
		current = ReadByteReserved(byte);
		if(!(current & mask))	{
			value = current | mask;
			UpdateFileReserveSubrange(byte,value);
#if defined(_WIN64) && !defined(__CYGWIN__)
			ReleaseMutex(mutex);
#else
			pthread_mutex_unlock(mutex);
#endif
			return true;
		}
		else{
#if defined(_WIN64) && !defined(__CYGWIN__)
			ReleaseMutex(mutex);
#else
			pthread_mutex_unlock(mutex);
#endif
			return false;
		}
	}
}

bool SaveProgress::CheckReservedSubrange(Int *subrange)	{
	uint64_t bit;
	uint64_t byte;
	uint8_t mask,current;
	Int diff;
	diff.Sub(subrange,range_start);
	diff.Div(subrange_length);
	bit = diff.GetInt64();
	byte = bit >> 3;
	mask = 1 << (bit % 8);
	if(save_ram)	{
		//The data should be on RAM
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(mutex, INFINITE);
#else
		pthread_mutex_lock(mutex);
#endif
		current = data_reserved[byte];
#if defined(_WIN64) && !defined(__CYGWIN__)
		ReleaseMutex(mutex);
#else
		pthread_mutex_unlock(mutex);
#endif
		return (bool)(current & mask);
	}
	else	{
		//We need to retreive the data from file
		uint64_t offset  = OFFSET_FILE_DATASETS + byte;
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(mutex, INFINITE);
#else
		pthread_mutex_lock(mutex);
#endif
		
		if(fseek(filedescriptor,offset,SEEK_SET) != 0)	{
			printf("Failed to seek the file\n");
			exit(EXIT_FAILURE);
		}
		if(fread(&current,sizeof(uint8_t),1,filedescriptor) != 1)	{
			printf("Failed to update the file\n");
			exit(EXIT_FAILURE);
		}
#if defined(_WIN64) && !defined(__CYGWIN__)
		ReleaseMutex(mutex);
#else
		pthread_mutex_unlock(mutex);
#endif
		return (bool)(current & mask);
	}
}

bool SaveProgress::CheckCompleteSubrange(Int *subrange)	{
	uint64_t byte,bit;
	uint8_t mask,current;
	Int diff;
	diff.Sub(subrange,range_start);
	diff.Div(subrange_length);
	bit = diff.GetInt64();
	byte = bit >> 3;
	mask = 1 << (bit % 8);
	if(save_ram)	{
		//The data should be on RAM
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(mutex, INFINITE);
#else
		pthread_mutex_lock(mutex);
#endif
		current = data_complete[byte];
#if defined(_WIN64) && !defined(__CYGWIN__)
		ReleaseMutex(mutex);
#else
		pthread_mutex_unlock(mutex);
#endif
		return (bool)(current & mask);
	}
	else	{
		//We need to retreive the data from file
		uint64_t offset  = OFFSET_FILE_DATASETS +length + byte;
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(mutex, INFINITE);
#else
		pthread_mutex_lock(mutex);
#endif
		
		if(fseek(filedescriptor,offset,SEEK_SET) != 0)	{
			printf("Failed to seek the file\n");
			exit(EXIT_FAILURE);
		}
		if(fread(&current,sizeof(uint8_t),1,filedescriptor) != 1)	{
			printf("Failed to update the file\n");
			exit(EXIT_FAILURE);
		}
#if defined(_WIN64) && !defined(__CYGWIN__)
		ReleaseMutex(mutex);
#else
		pthread_mutex_unlock(mutex);
#endif
		return (bool)(current & mask);
	}
}

bool SaveProgress::GetAndReserveNextForwardAvailable(Int *subrange)	{
	if(last_item_forward_reserve >= length - last_item_backward_reserve)	{
		//There is no more items available
		return false;
	}
	uint64_t byte,bit;
	uint8_t mask,current,value;
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(mutex, INFINITE);
#else
	pthread_mutex_lock(mutex);
#endif
	if(type)	{	//If type is 1 we need to update the counter and bit in file
		//INCOMPLETE
		
		bit = last_item_forward_reserve;
		byte = bit >> 3;
		mask = 1 << (bit % 8);
		if(save_ram)	{	//we also need to update the bit in RAM
			current = data_reserved[byte];
			if(!(current & mask)){
				value = current | mask;
				data_reserved[byte] = value;
				items_reserved_used++;
				UpdateFileReserveSubrange(byte,value);
			}
		}
		else	{
			current = ReadByteReserved(byte);
			if(!(current & mask)){
				value = current | mask;
				items_reserved_used++;
				UpdateFileReserveSubrange(byte,value);
			}
		}
	}
	else{
		subrange->Set(subrange_length);
		subrange->Mult(last_item_forward_reserve);
		subrange->Add(range_start);
		last_item_forward_reserve++;
		UpdateLastItemForwardReserve();
	}
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(mutex);
#else
	pthread_mutex_unlock(mutex);
#endif
	return true;
}

bool SaveProgress::GetAndReserveNextBackwardAvailable(Int *subrange)	{
	if(last_item_backward_reserve >= length - last_item_forward_reserve)	{
		//There is no more items available
		return false;
	}
	uint64_t byte,bit;
	uint8_t mask,current,value;
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(mutex, INFINITE);
#else
	pthread_mutex_lock(mutex);
#endif

	if(type)	{	//If type is 1 we need to update the counter and bit in file
		bit = length - last_item_backward_reserve;
		byte = bit >> 3;
		mask = 1 << (bit % 8);
		if(save_ram)	{	//we also need to update the bit in RAM
			current = data_reserved[byte];
			if(!(current & mask)){
				value = current | mask;
				data_reserved[byte] = value;
				items_reserved_used++;
				UpdateFileReserveSubrange(byte,value);
			}
		}
		else	{
			current = ReadByteReserved(byte);
			if(!(current & mask)){
				value = current | mask;
				items_reserved_used++;
				UpdateFileReserveSubrange(byte,value);
			}
		}
	}
	else	{
		subrange->Set(subrange_length);
		subrange->Mult(length - last_item_backward_reserve);
		subrange->Add(range_start);
		last_item_backward_reserve++;
		UpdateLastItemBackwardReserve();
	}
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(mutex);
#else
	pthread_mutex_unlock(mutex);
#endif
	return true;
}

void SaveProgress::UpdateLastItemBackwardReserve()	{
	if(fseek(filedescriptor,OFFSET_FILE_LAST_ITEM_F_R,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		exit(EXIT_FAILURE);
	}
	if(fwrite(&last_item_forward_reserve,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to update the file\n");
		exit(EXIT_FAILURE);
	}
}

void SaveProgress::UpdateLastItemForwardReserve()	{
	if(fseek(filedescriptor,OFFSET_FILE_LAST_ITEM_B_R,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		exit(EXIT_FAILURE);
	}
	if(fwrite(&last_item_backward_reserve,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to update the file\n");
		exit(EXIT_FAILURE);
	}
}

void SaveProgress::UpdateItemsReservedUsed()	{
	if(fseek(filedescriptor,OFFSET_FILE_ITEMS_R_USED,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		exit(EXIT_FAILURE);
	}
	if(fwrite(&items_reserved_used,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to update the file\n");
		exit(EXIT_FAILURE);
	}
}

bool SaveProgress::GetAndReserveNextRandomAvailable(Int *subrange)	{
	if(items_reserved_used >= length || type == 0)	{	// There is no random in type 0
		//There is no more items available
		return false;
	}
	uint64_t byte,bit;
	uint8_t mask,current,value;
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(mutex, INFINITE);
#else
	pthread_mutex_lock(mutex);
#endif
	subrange->Set(subrange_length);
	subrange->Mult(length - last_item_backward_reserve);
	subrange->Add(range_start);
	if(type)	{	//If type is 1 we need to update the counter and bit in file
		bit = length - last_item_backward_reserve;
		byte = bit >> 3;
		mask = 1 << (bit % 8);
		if(save_ram)	{	//we also need to update the bit in RAM
			current = data_reserved[byte];
			if(!(current & mask)){
				value = current | mask;
				data_reserved[byte] = value;
				items_reserved_used++;
				UpdateFileReserveSubrange(byte,value);
			}
		}
		else	{
			current = ReadByteReserved(byte);
			if(!(current & mask)){
				value = current | mask;
				items_reserved_used++;
				UpdateFileReserveSubrange(byte,value);
			}
		}
	}
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(mutex);
#else
	pthread_mutex_unlock(mutex);
#endif
	return true;
}

void SaveProgress::ReadVariablesFromFileName(const char *tempfilename)	{
	/*
		here we need the current variables in the file and compare it with the expected ones
		if some change we need to abort or check if is possible to continue in some cases
	*/
	uint8_t bufferInt[32];
	filedescriptor = fopen(tempfilename,"rb");
	if(filedescriptor == NULL)	{
		fprintf(stderr,"Can't open the file %s\n",tempfilename);
		exit(EXIT_FAILURE);
	}
	
	this->range_start = new Int();
	this->range_end = new Int();
	this->subrange_length = new Int();
	
	if(fseek(filedescriptor,0,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}

	if(fread(magicbytes,24,1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}

	if(fread(&version,sizeof(uint32_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&type,sizeof(uint8_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fseek(filedescriptor,OFFSET_FILE_LENGTH,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&length,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&items,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}


	if(fread(&items_reserved_used,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&items_complete_used,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&last_item_forward_reserve,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&last_item_backward_reserve,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&last_item_forward_complete,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	if(fread(&last_item_backward_complete,sizeof(uint64_t),1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}

	if(fseek(filedescriptor,OFFSET_FILE_RANGE_START,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}

	if(fread(bufferInt,32,1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	range_start->Set32Bytes(bufferInt);
	
	if(fread(bufferInt,32,1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	range_end->Set32Bytes(bufferInt);

	if(fread(bufferInt,32,1,filedescriptor) != 1)	{
		printf("Failed to read the file\n");
		fclose(filedescriptor);
		exit(EXIT_FAILURE);
	}
	
	subrange_length->Set32Bytes(bufferInt);

	/*
	if(type == 1 && save_ram)	{
		if(fread(data_reserved,length,1,filedescriptor) != 1)	{
			printf("Failed to read the file\n");
			fclose(filedescriptor);
			exit(EXIT_FAILURE);
		}
		if(fread(data_complete,length,1,filedescriptor) != 1)	{
			printf("Failed to read the file\n");
			fclose(filedescriptor);
			exit(EXIT_FAILURE);
		}
	}
	*/
}

void SaveProgress::ShowHeaders()	{
	char *temp;
	printf("Magic Bytes: %s\n",magicbytes);
	printf("Version: 0x%x\n",version);
	printf("Type: %i\n",type);
	printf("Items: %lu\n",items);
	printf("Length: %lu\n",length);
	
	if(type == 1)	{
		printf("Items reserved used: %lu\n",items_reserved_used);
		printf("Items complete used: %lu\n",items_complete_used);
	}
	printf("Last item fordward reserve: %lu\n",last_item_forward_reserve);
	printf("Last item backward reserve: %lu\n",last_item_backward_reserve);		
	printf("Last item fordward complete: %lu\n",last_item_forward_complete);
	printf("Last item backward complete: %lu\n",last_item_backward_complete);
	
	temp = range_start->GetBase16();
	printf("Star range: %s\n",temp);
	free(temp);

	temp = range_end->GetBase16();
	printf("End range: %s\n",temp);
	free(temp);

	temp = subrange_length->GetBase16();
	printf("Length subrange: %s\n",temp);
	free(temp);
}

void SaveProgress::ShowProgress()	{
	Int range_from,range_to;
	char *str_from,*str_to;
	if(type == 1)	{
		//Here we need to read the file or the values from ram if save_ram is enable:
		bool counting_zeros;
		bool counting_ones;
		uint64_t total_zeros = 0;
		uint64_t total_ones = 0;
		unsigned char mask,current_value,current_byte;
		uint64_t bit_counter = 0,offset = 0,i;
		int j,limit;
		counting_ones = true;
		counting_zeros = true;

		for (i = 0; i < length; i++) {
			if(save_ram)	{
				current_byte = data_complete[i];
			}
			else	{
				current_byte = ReadByteComplete(i);
			}
			limit =  (i < length-1) ? 8 : items - bit_counter; 
			for (j = 0; j < 8; j++) {
				mask = (1 << j);
				current_value = current_byte & mask;
				if (current_value == 0) {
					total_zeros++;
					if(counting_ones)	{
						counting_ones = false;
						counting_zeros = true;
						if(total_ones > 0)	{
							CalculateRange(offset,&range_from);
							CalculateRange(offset+total_ones,&range_to);
							range_to.Sub(1);
							str_from = range_from.GetBase16();
							str_to = range_to.GetBase16();
							printf("Segment Complete from %s to %s\n",str_from,str_to);
							free(str_from);
							free(str_to);
							total_ones = 0;
							offset = bit_counter;

						}
					}
				} else {
					total_ones++;
					if(counting_zeros)	{
						counting_ones = true;
						counting_zeros = false;
						if(total_zeros > 0)	{
							CalculateRange(offset,&range_from);
							CalculateRange(offset+total_zeros,&range_to);
							range_to.Sub(1);
							str_from = range_from.GetBase16();
							str_to = range_to.GetBase16();
							printf("Segment NOT Complete from %s to %s\n",str_from,str_to);
							free(str_from);
							free(str_to);
							total_zeros = 0;
							offset = bit_counter;
						}
					}
				}
				bit_counter++;
			}
		}
		
		if(counting_zeros)	{
			CalculateRange(offset,&range_from);
			CalculateRange(offset+total_zeros,&range_to);
			range_to.Sub(1);
			str_from = range_from.GetBase16();
			str_to = range_to.GetBase16();
			printf("Segment NOT Complete from %s to %s\n",str_from,str_to);
			free(str_from);
			free(str_to);
		}
		else	{
			CalculateRange(offset,&range_from);
			CalculateRange(offset+total_ones,&range_to);
			range_to.Sub(1);
			str_from = range_from.GetBase16();
			str_to = range_to.GetBase16();
			printf("Segment Complete from %s to %s\n",str_from,str_to);
			free(str_from);
			free(str_to);
		}
	}
	else	{
		//Here we only need to show the Forward and backward values
		if(last_item_forward_complete > 0)	{
			if(last_item_forward_complete+ last_item_backward_complete >= items)	{
				//All range complete
				str_from = range_start->GetBase16();
				str_to = range_end->GetBase16();
				printf("Segment Complete from %s to %s\n",str_from,str_to);
				free(str_from);
				free(str_to);
				return;
			}
			else	{
				CalculateRange(0,&range_from);
				CalculateRange(last_item_forward_complete,&range_to);
				range_to.Sub(1);
				str_from = range_from.GetBase16();
				str_to = range_to.GetBase16();
				printf("Segment Complete from %s to %s\n",str_from,str_to);
				free(str_from);
				free(str_to);
				
				CalculateRange(last_item_forward_complete,&range_from);
				CalculateRange(items-last_item_backward_complete,&range_to);
				range_to.Sub(1);
				str_from = range_from.GetBase16();
				str_to = range_to.GetBase16();
				printf("Segment NOT Complete from %s to %s\n",str_from,str_to);
				free(str_from);
				free(str_to);
			}
		}
		else{
			CalculateRange(0,&range_from);
			CalculateRange(items-last_item_backward_complete,&range_to);
			range_to.Sub(1);
			str_from = range_from.GetBase16();
			str_to = range_to.GetBase16();
			printf("Segment NOT Complete from %s to %s\n",str_from,str_to);
			free(str_from);
			free(str_to);	
		}
		if(last_item_forward_complete > 0)	{
			CalculateRange(items-last_item_backward_complete,&range_from);
			CalculateRange(items,&range_to);
			range_to.Sub(1);
			str_from = range_from.GetBase16();
			str_to = range_to.GetBase16();
			printf("Segment Complete from %s to %s\n",str_from,str_to);
			free(str_from);
			free(str_to);	
		}		
	}
}

void SaveProgress::CalculateRange(uint64_t offset,Int *range)	{
	range->Set(subrange_length);
	range->Mult(offset);
	range->Add(range_start);	
}

void SaveProgress::RemoveReservedUncompleted()	{
	if(type == 1)	{
		uint8_t bufferReserved[SAVEPROGRESS_BUFFER_LENGTH];
		uint8_t bufferComplete[SAVEPROGRESS_BUFFER_LENGTH];
		uint64_t offset;
		int current_chunk_size;
		int number_of_chunks = (int) (length/SAVEPROGRESS_BUFFER_LENGTH);
		int last_chunk_size = length % SAVEPROGRESS_BUFFER_LENGTH;
		bool need_update;
		if(last_chunk_size == 0)	{
			last_chunk_size = SAVEPROGRESS_BUFFER_LENGTH;
		}
		else	{
			number_of_chunks++;
		}
		for(int i = 0; i < number_of_chunks;i++)	{
			current_chunk_size = (i < number_of_chunks - 1) ? SAVEPROGRESS_BUFFER_LENGTH :last_chunk_size; 
			offset = i * SAVEPROGRESS_BUFFER_LENGTH;
			ReadReserved(bufferReserved,offset,current_chunk_size);
			ReadComplete(bufferComplete,offset,current_chunk_size);
			need_update = false;
			for(int j = 0; j < current_chunk_size;  j++)	{
				if(bufferReserved[j] != bufferComplete[j])	{
					need_update = true;
					break;
				}
			}
			if(need_update)	{
				WriteReserved(bufferReserved,offset,current_chunk_size);
			}
		}
		last_item_forward_reserve = last_item_forward_complete;
		last_item_backward_reserve = last_item_backward_complete;
		items_reserved_used = items_complete_used;
		UpdateItemsReservedUsed();
		UpdateLastItemForwardReserve();
		UpdateLastItemBackwardReserve();
	}
	else	{
		last_item_forward_reserve = last_item_forward_complete;
		last_item_backward_reserve = last_item_backward_complete;
		UpdateLastItemForwardReserve();
		UpdateLastItemBackwardReserve();
	}
}

void SaveProgress::ReadReserved(uint8_t *buffer,uint64_t offset,int size)	{
	if(save_ram)	{
		memcpy(buffer,data_reserved + offset,size);
	}
	else	{
		if(fseek(filedescriptor,OFFSET_FILE_DATASETS + offset,SEEK_SET) != 0)	{
			printf("Failed to seek the file\n");
			exit(EXIT_FAILURE);
		}
		if(fread(buffer,sizeof(uint8_t),size,filedescriptor) != size)	{
			printf("Failed to read the file\n");
			exit(EXIT_FAILURE);
		}
	}
}

void SaveProgress::ReadComplete(uint8_t *buffer,uint64_t offset,int size)	{
	if(save_ram)	{
		memcpy(buffer,data_complete + offset,size);
	}
	else	{
		if(fseek(filedescriptor,OFFSET_FILE_DATASETS + length + offset,SEEK_SET) != 0)	{
			printf("Failed to seek the file\n");
			exit(EXIT_FAILURE);
		}
		if(fread(buffer,sizeof(uint8_t),size,filedescriptor) != size)	{
			printf("Failed to read the file\n");
			exit(EXIT_FAILURE);
		}
	}
}

void SaveProgress::WriteReserved(uint8_t *buffer,uint64_t offset,int size)	{
	if(fseek(filedescriptor,OFFSET_FILE_DATASETS + offset,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		exit(EXIT_FAILURE);
	}
	if(fwrite(buffer,sizeof(uint8_t),size,filedescriptor) != size)	{
		printf("Failed to write the file\n");
		exit(EXIT_FAILURE);
	}
	if(save_ram)	{
		memcpy(data_reserved + offset,buffer,size);
	}
}

void SaveProgress::WriteComplete(uint8_t *buffer,uint64_t offset,int size)	{
	if(fseek(filedescriptor,OFFSET_FILE_DATASETS + length + offset,SEEK_SET) != 0)	{
		printf("Failed to seek the file\n");
		exit(EXIT_FAILURE);
	}
	if(fwrite(buffer,sizeof(uint8_t),size,filedescriptor) != size)	{
		printf("Failed to write the file\n");
		exit(EXIT_FAILURE);
	}
	if(save_ram)	{
		memcpy(data_complete + offset,buffer,size);
	}
}

Int SaveProgress::GetRangeStart()	{
	Int r(range_start);
	return r;
}

Int SaveProgress::GetRangeEnd()	{
	Int r(range_end);
	return r;
}