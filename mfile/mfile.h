/*
	A mmap file wrapper
	Copyright 2016 Smx
*/
#ifndef __MFILE_H
#define __MFILE_H
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/mman.h>

/* Gets the size of the memory mapped file */
#define msize(mfile) mfile->statBuf.st_size
/* Gets the data, casted to the type specified in the second argument */
#define mdata(mfile, type) (type *)(mfile->pMem)
/* Gets the file handler (for mfopen) */
#define mfh(mfile) mfile->fh
/* Gets the file offset */
#define moff(mfile, ptr) (off_t)((uintptr_t)ptr - (uintptr_t)mfile->pMem)

typedef struct {
	int fd;
	FILE *fh;
	char *path;
	size_t size;
	int prot;
	struct stat statBuf;
	void *pMem;
} MFILE;

MFILE *mfile_new();

void *mfile_map(MFILE *file, size_t size);
void *mfile_map_private(MFILE *file, size_t size);

MFILE *mopen(const char *path, int oflags);
MFILE *mopen_private(const char *path, int oflags);

MFILE *mfopen(const char *path, const char *mode);
MFILE *mfopen_private(const char *path, const char *mode);

int mclose(MFILE *mfile);
#endif
