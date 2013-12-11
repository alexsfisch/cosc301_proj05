/* This code is based on the fine code written by Joseph Pfeiffer for his
   fuse system tutorial. */

#include "s3fs.h"
#include "libs3_wrapper.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

#define GET_PRIVATE_DATA ((s3context_t *) fuse_get_context()->private_data)

/*
 * For each function below, if you need to return an error,
 * read the appropriate man page for the call and see what
 * error codes make sense for the type of failure you want
 * to convey.  For example, many of the calls below return
 * -EIO (an I/O error), since there are no S3 calls yet
 * implemented.  (Note that you need to return the negative
 * value for an error code.)
 */

/* *************************************** */
/*        Stage 1 callbacks                */
/* *************************************** */

/*
 * Initialize the file system.  This is called once upon
 * file system startup.
 */
void *fs_init(struct fuse_conn_info *conn) //ALMOST DONE
{
	//sommers code
    fprintf(stderr, "fs_init --- initializing file system.\n");
    s3context_t *ctx = GET_PRIVATE_DATA;
	
	//our code
	//need to completely destroy everything in this bucket
	s3fs_clear_bucket(ctx->s3bucket); //not sure about parameter
	//do we need to check that the clear did not fail?

	//create a directory object to represent your root directory and store that on S3
	s3dirent_t dir;					//create directory structur
	time_t temp;					//create temporory timestamp
	time(&temp);					//get current time
	dir.size = sizeof(s3dirent_t);		//set directory size to size of s3dirent struct
	strcpy(dir.type,"D");		//set diretory type to directory
	dir.timeAccess = temp;			//set directory last access time to temp
	dir.timeMod = temp;				//set directory last modified time to temp
	
	//update information for object array of directory

	dir.dirArray[0].timeAccess = temp;			//set directory last access time to temp
	dir.dirArray[0].timeMod = temp;				//set directory last modified time to temp
	strcpy(dir.dirArray[0].name, ".");			//set name of directory to ".". 
	strcpy(dir.dirArray[0].type, "F");			//set name of directory to ".". 


	strcpy(dir.dirArray[1].name, "");			//set name of directory to "blank"
	strcpy(dir.dirArray[1].type, "U");			//set type to unused, indicating end 


	//initialize object (put object in s3), unless fail. then return error
	if ((s3fs_put_object(ctx->s3bucket, "/", (const uint8_t *)&dir, sizeof(dir)))!=sizeof(dir)) {
		fprintf(stderr, "failed to intialize");
		return -EIO;
	} 

	//everything worked as planned. continue
	printf("%s\n","-------FINISHED INIT-------");
    return ctx;
}

/*
 * Clean up filesystem -- free any allocated data.
 * Called once on filesystem exit.
 */

void fs_destroy(void *userdata) { //DONE
    fprintf(stderr, "fs_destroy --- shutting down file system.\n");
    free(userdata);
}


/* 
 * Get file attributes.  Similar to the stat() call
 * (and uses the same structure).  The st_dev, st_blksize,
 * and st_ino fields are ignored in the struct (and 
 * do not need to be filled in).
 */

int fs_getattr(const char *path, struct stat *statbuf) {
	//sommers code
    fprintf(stderr, "fs_getattr(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;

	//our code
	s3dirent_t * dir = NULL;
	char* directoryPath = strdup(dirname(path)); 	//get path and malloc
	char* directoryName = strdup(basename(path));	//get name and malloc;
	if ((s3fs_get_object(ctx->s3bucket, directoryPath, (uint8_t **)&dir, 0, 0)<0)) {
		fprintf(stderr, "invalid path");
		return -EIO;
	}

	//find specified file or directory
	int i = 0;
	while(strcmp(((*dir).dirArray[i].type),"U")!=0) {
		//check if name already exists.
		if(strcmp((*dir).dirArray[i].name, directoryName)==0) {
			//PRINT ATTRIBUTES
			printf("%s","NAME:    ");
			printf("%s\n",(*dir).dirArray[i].name);
			printf("%s","TYPE:    ");
			printf("%s\n",(*dir).dirArray[i].type);
			printf("%s","LAST ACCESSED:    ");
			printf("%s\n",(*dir).dirArray[i].timeAccess);
			printf("%s","LAST MODIFIED:    ");
			printf("%s\n",(*dir).dirArray[i].timeMod);
			printf("%s","SIZE:    ");
			printf("%i\n",(*dir).dirArray[i].size);


			//not sure if this is correct
			statbuf->st_size = (*dir).dirArray[i].size;
			statbuf->st_atime = (*dir).dirArray[i].timeAccess;
			statbuf->st_mtime = (*dir).dirArray[i].timeMod;
			/*
			statbuf->st_ctime = //what is this?
 			statbuf->st_dev =
 			statbuf->st_ino =
 			statbuf->st_mode =
 			statbuf->st_nlink = 
 			statbuf->st_uid=
 			statbuf->st_gid =
 			statbuf->st_rdev =
 			statbuf->st_blocks =
			*/
			free(dir);	 //free malloced dir
			free(directoryPath);
			free(directoryName);
			printf("%s\n","---------GET ATTRIBUTES FINISHED------------");
			return 0;
		}
		i++; 
	}
	

	fprintf(stderr, "path does not exist :(");
	return EIO; 
}


/*
 * Open directory
 *
 * This method should check if the open operation is permitted for
 * this directory
 */
int fs_opendir(const char *path, struct fuse_file_info *fi) {
	//sommers code
    fprintf(stderr, "fs_opendir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;

	//our code
	s3dirent_t * dir = NULL;
	if (s3fs_get_object(ctx->s3bucket, path, (uint8_t **)&dir, 0,0)<0) //if not a real directory, else store in dir
	{
		fprintf(stderr, "path is not valid");
		return -EIO;
	}

	//check  to see if the type == "directory"
	if (strcmp((*dir).type, "D")!=0) {
		fprintf(stderr, "not a real directory");
		return -EIO;
	}
	printf("%s\n","----finished opendir-------");
	free(dir);
    return 0;
}


/*
 * Read directory.  See the project description for how to use the filler
 * function for filling in directory items.
 */
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
         struct fuse_file_info *fi)
{
	//sommers code
    fprintf(stderr, "fs_readdir(path=\"%s\", buf=%p, offset=%d)\n", path, buf, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;

	//our code
	s3dirent_t * dir = NULL;
	if (s3fs_get_object(ctx->s3bucket, path, (uint8_t **)&dir, 0,0)<0) //if not a real directory, else store in dir
	{
		fprintf(stderr, "path is not valid");
		return -EIO;
	}
	
	//check to make sure it is a directory
	if (strcmp((*dir).type, "D")!=0) {
		fprintf(stderr, "not a real directory");
		return -EIO;
	}
	
	int i = 0;
	printf("%s", "dirarray[i].type:     ");
	printf("%s", (*dir).dirArray[i].type);
	while(strcmp(((*dir).dirArray[i].type),"U")!=0) {
		//call filler function to fill in directory name to supplied buffer
		if(filler(buf,(*dir).dirArray[i].name, NULL, 0) != 0) {
			return -ENOMEM;
		}
		printf("%s\n","?!?!?!?!?!?!?!?!?!!!?!!?!?!?!!??!?!?!?!!?!??!?!?!?!?!?!?!?!??!?!?!");
		i++; 
	}
	


	free(dir);		//free malloced dir
	printf("%s\n","-----READDIR FINISHED-------------");
    return 0;
}


/*
 * Release directory.
 */
int fs_releasedir(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_releasedir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return 0;//only changed this to a 0. is that really it?
}


/* 
 * Create a new directory.
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits (for setting in the metadata)
 * use mode|S_IFDIR.
 */
int fs_mkdir(const char *path, mode_t mode) {
	//sommers code
    fprintf(stderr, "fs_mkdir(path=\"%s\", mode=0%3o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    mode |= S_IFDIR;


	//our code
	s3dirent_t * dir = NULL;
	char* directoryPath = strdup(dirname(path)); 	//get path and malloc
	char* directoryName = strdup(basename(path));	//get name and malloc
	char* tempPath;
	//check if name already exists
	if (s3fs_get_object(ctx->s3bucket, directoryPath, (uint8_t **)&dir, 0,0)<0) //if not a real directory
	{
		fprintf(stderr, "path is not valid");
		return -EIO;
	}
	//make sure where you are trying to put is infact a directory
	if (strcmp((*dir).type, "directory")!=0) {
		fprintf(stderr, "object is not a not a directory. Can not place here");
		return -EIO;
	}
	//iterate through files and make sure name does not already exist
	int i = 0;
	while(strcmp(((*dir).dirArray[i].type),"U")!=0) {
		//check if name already exists.
		if(strcmp((*dir).dirArray[i].name, directoryName)!=0) {
			fprintf(stderr, "name already exists!");
			return -EEXIST;
		}
		i++; 
	}

	
	//if got to here, then safe to make directory and add it
	s3dirent_t newDir;							//create new directory struct

	time_t temp;								//create temporory timestamp
	time(&temp);								//get current time
	strcpy(newDir.type,"D");
	newDir.timeAccess = temp;					//set directory last access time to temp
	newDir.timeMod = temp;						//set directory last modified time to temp
	newDir.size = sizeof(s3dirent_t);			//set directory size to size of s3dirent struct
	strcpy(newDir.type, "D");					//set diretory type to directory
	strcpy(newDir.dirArray[0].name, ".");			//set name of directory to ".". 
	strcpy(newDir.dirArray[0].type, "F");			//set name of directory to ".". 


	strcpy(newDir.dirArray[1].name, "");			//set name of directory to "blank"
	strcpy(newDir.dirArray[1].type, "U");			//set type to unused, indicating end 

	//need to add something about mode_t mode????





	i = 0;
	while(strcmp(((*dir).dirArray[i].type),"U")!=0) {
		//check if name already exists.
		i++; 
	}

	objects obj;

	strcpy(obj.name, newDir.name);
	strcpy(obj.type, newDir.type);
	obj.timeAccess = newDir.timeAccess;
	obj.timeMod = newDir.timeMod;
	obj.size = newDir.size;

	(*dir).dirArray[i] = obj;
	strcpy((*dir).dirArray[i+1].type, "U"); //need to realloc this
	
	//re-upload old directory
	if ((s3fs_put_object(ctx->s3bucket, directoryPath, (const uint8_t *)&dir, sizeof(dir)))!=sizeof(dir)) {
		fprintf(stderr, "failed to add the new directory");
		return -EIO;
	} 

	//upload new directory
	if ((s3fs_put_object(ctx->s3bucket, path, (const uint8_t *)&newDir, sizeof(newDir)))!=sizeof(newDir)) {
		fprintf(stderr, "failed to add the new directory");
		return -EIO;
	} 


	//free everything malloced
	free(dir);
	free(newDir.type);
	//free(newDir.firstFile);
	free(newDir.name);
    return 0;
}


/*
 * Remove a directory. 
 */
int fs_rmdir(const char *path) {
	//sommers code
    fprintf(stderr, "fs_rmdir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;

	//our code
	s3dirent_t * dir = NULL;
	if (s3fs_get_object(ctx->s3bucket, path, (uint8_t **)&dir, 0,0)<0) //if not a real directory, else store in dir
	{
		fprintf(stderr, "path is not valid");
		return -EIO;
	}

	//first check to see if it is a directory
	if (strcmp((*dir).type, "directory")!=0) {
		fprintf(stderr, "not a real directory");
		return -EIO;
	}

	//next check to see if the directory is empty
	if (strcmp((*dir).dirArray[0].type, "U") != 0) {
		fprintf(stderr, "the directory is not empty");
		return -EIO;
	}


	


	free(dir);
    return 0;
}


/* *************************************** */
/*        Stage 2 callbacks                */
/* *************************************** */


/* 
 * Create a file "node".  When a new file is created, this
 * function will get called.  
 * This is called for creation of all non-directory, non-symlink
 * nodes.  You *only* need to handle creation of regular
 * files here.  (See the man page for mknod (2).)
 */
int fs_mknod(const char *path, mode_t mode, dev_t dev) {
    fprintf(stderr, "fs_mknod(path=\"%s\", mode=0%3o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/* 
 * File open operation
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  
 * 
 * Optionally open may also return an arbitrary filehandle in the 
 * fuse_file_info structure (fi->fh).
 * which will be passed to all file operations.
 * (In stages 1 and 2, you are advised to keep this function very,
 * very simple.)
 */
int fs_open(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_open(path\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/* 
 * Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  
 */
int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_read(path=\"%s\", buf=%p, size=%d, offset=%d)\n",
          path, buf, (int)size, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.
 */
int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_write(path=\"%s\", buf=%p, size=%d, offset=%d)\n",
          path, buf, (int)size, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.  
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 */
int fs_release(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_release(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Rename a file.
 */
int fs_rename(const char *path, const char *newpath) {
    fprintf(stderr, "fs_rename(fpath=\"%s\", newpath=\"%s\")\n", path, newpath);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Remove a file.
 */
int fs_unlink(const char *path) {
    fprintf(stderr, "fs_unlink(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}
/*
 * Change the size of a file.
 */
int fs_truncate(const char *path, off_t newsize) {
    fprintf(stderr, "fs_truncate(path=\"%s\", newsize=%d)\n", path, (int)newsize);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Change the size of an open file.  Very similar to fs_truncate (and,
 * depending on your implementation), you could possibly treat it the
 * same as fs_truncate.
 */
int fs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_ftruncate(path=\"%s\", offset=%d)\n", path, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Check file access permissions.  For now, just return 0 (success!)
 * Later, actually check permissions (don't bother initially).
 */
int fs_access(const char *path, int mask) {
    fprintf(stderr, "fs_access(path=\"%s\", mask=0%o)\n", path, mask);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return 0;
}


/*
 * The struct that contains pointers to all our callback
 * functions.  Those that are currently NULL aren't 
 * intended to be implemented in this project.
 */
struct fuse_operations s3fs_ops = {
  .getattr     = fs_getattr,    // get file attributes
  .readlink    = NULL,          // read a symbolic link
  .getdir      = NULL,          // deprecated function
  .mknod       = fs_mknod,      // create a file
  .mkdir       = fs_mkdir,      // create a directory
  .unlink      = fs_unlink,     // remove/unlink a file
  .rmdir       = fs_rmdir,      // remove a directory
  .symlink     = NULL,          // create a symbolic link
  .rename      = fs_rename,     // rename a file
  .link        = NULL,          // we don't support hard links
  .chmod       = NULL,          // change mode bits: not implemented
  .chown       = NULL,          // change ownership: not implemented
  .truncate    = fs_truncate,   // truncate a file's size
  .utime       = NULL,          // update stat times for a file: not implemented
  .open        = fs_open,       // open a file
  .read        = fs_read,       // read contents from an open file
  .write       = fs_write,      // write contents to an open file
  .statfs      = NULL,          // file sys stat: not implemented
  .flush       = NULL,          // flush file to stable storage: not implemented
  .release     = fs_release,    // release/close file
  .fsync       = NULL,          // sync file to disk: not implemented
  .setxattr    = NULL,          // not implemented
  .getxattr    = NULL,          // not implemented
  .listxattr   = NULL,          // not implemented
  .removexattr = NULL,          // not implemented
  .opendir     = fs_opendir,    // open directory entry
  .readdir     = fs_readdir,    // read directory entry
  .releasedir  = fs_releasedir, // release/close directory
  .fsyncdir    = NULL,          // sync dirent to disk: not implemented
  .init        = fs_init,       // initialize filesystem
  .destroy     = fs_destroy,    // cleanup/destroy filesystem
  .access      = fs_access,     // check access permissions for a file
  .create      = NULL,          // not implemented
  .ftruncate   = fs_ftruncate,  // truncate the file
  .fgetattr    = NULL           // not implemented
};



/* 
 * You shouldn't need to change anything here.  If you need to
 * add more items to the filesystem context object (which currently
 * only has the S3 bucket name), you might want to initialize that
 * here (but you could also reasonably do that in fs_init).
 */
int main(int argc, char *argv[]) {
    // don't allow anything to continue if we're running as root.  bad stuff.
    if ((getuid() == 0) || (geteuid() == 0)) {
    	fprintf(stderr, "Don't run this as root.\n");
    	return -1;
    }
    s3context_t *stateinfo = malloc(sizeof(s3context_t));
    memset(stateinfo, 0, sizeof(s3context_t));

    char *s3key = getenv(S3ACCESSKEY);
    if (!s3key) {
        fprintf(stderr, "%s environment variable must be defined\n", S3ACCESSKEY);
        return -1;
    }
    char *s3secret = getenv(S3SECRETKEY);
    if (!s3secret) {
        fprintf(stderr, "%s environment variable must be defined\n", S3SECRETKEY);
        return -1;
    }
    char *s3bucket = getenv(S3BUCKET);
    if (!s3bucket) {
        fprintf(stderr, "%s environment variable must be defined\n", S3BUCKET);
        return -1;
    }
    strncpy((*stateinfo).s3bucket, s3bucket, BUFFERSIZE);

    fprintf(stderr, "Initializing s3 credentials\n");
    s3fs_init_credentials(s3key, s3secret);

    fprintf(stderr, "Totally clearing s3 bucket\n");
    s3fs_clear_bucket(s3bucket);

    fprintf(stderr, "Starting up FUSE file system.\n");
	
	printf("%i\n",argc);
	printf("%s\n",argv);
	printf("%i\n",&s3fs_ops);
	printf("%s\n",stateinfo);
	
    int fuse_stat = fuse_main(argc, argv, &s3fs_ops, stateinfo);

    fprintf(stderr, "Startup function (fuse_main) returned %d\n", fuse_stat);
    
    return fuse_stat;
}
