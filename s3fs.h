#ifndef __USERSPACEFS_H__
#define __USERSPACEFS_H__

#include <sys/stat.h>
#include <stdint.h>   // for uint32_t, etc.
#include <sys/time.h> // for struct timeval

/* This code is based on the fine code written by Joseph Pfeiffer for his
   fuse system tutorial. */

/* Declare to the FUSE API which version we're willing to speak */
#define FUSE_USE_VERSION 26

#define S3ACCESSKEY "S3_ACCESS_KEY_ID"
#define S3SECRETKEY "S3_SECRET_ACCESS_KEY"
#define S3BUCKET "S3_BUCKET"

#define BUFFERSIZE 1024

// store filesystem state information in this struct
typedef struct {
    char s3bucket[BUFFERSIZE];
} s3context_t;

/*
 * Other data type definitions (e.g., a directory entry
 * type) should go here.
 */


typedef struct {
	char name[256];				//name
	char type[1];				//type
	time_t timeAccess;			//time last accessed
	time_t timeMod;				//time last modified
	off_t size;					//the file size
	uid_t userID;				//user ID
	gid_t groupID;				//group ID
	mode_t protection;			//protection
	//char path[1024];			//path
	
} s3dirent_t;




#endif // __USERSPACEFS_H__
