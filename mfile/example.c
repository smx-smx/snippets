/*
	A mmap file wrapper
	Copyright 2016 Smx
*/
#include <stdio.h>
#include <stdlib.h>
#include "mfile.h"

int main_read(int argc, char *argv[]){
	if(argc < 3){
		fprintf(stderr, "Usage: %s w [infile]\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	MFILE *in = mopen(argv[2], O_RDONLY);
	if(!in){
		fprintf(stderr, "mopen failed\n");
		return EXIT_FAILURE;
	}
	
	// Do stuff with file
	uint8_t *bytes = mdata(in, uint8_t);
	
	mclose(in);
	return EXIT_SUCCESS;
}

int main_write(int argc, char *argv[]){
	if(argc < 4){
		fprintf(stderr, "Usage: %s w [outfile][outsize]\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	MFILE *out = mfopen(argv[2], "w+");
	if(!out){
		fprintf(stderr, "mfopen failed\n");
		return EXIT_FAILURE;
	}
	
	unsigned long fsize = strtoul(argv[3], NULL, 10);
	if(fsize <= 0){
		fprintf(stderr, "invalid size\n");
	} else {	
		mfile_map(out, fsize);

		// Do stuff with file
		uint8_t *bytes = mdata(out, uint8_t);
		
		mclose(out);
	}
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[]){
	if(argc < 2){
		fprintf(stderr, "Usage: %s [r|w][args]\n", argv[0]);
		return EXIT_FAILURE;
	}
	char *mode = argv[1];
	switch(mode[0]){
		case 'r':
			return main_read(argc, argv);
		case 'w':
			return main_write(argc, argv);
		default:
			fprintf(stderr, "Unknown mode '%c'\n", mode[0]);
			return EXIT_FAILURE;
	}
}