/*
	Copyright (C) 2016 Smx
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "plist.h"

int main(int argc, char *argv[]){
	if(argc < 2){
		fprintf(stderr, "Usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	PLIST *plist = calloc(1, sizeof(PLIST));
	if(!plist_open(argv[1], plist)){
		fprintf(stderr, "err\n");
		return EXIT_FAILURE;
	}
	
	PLIST_NODE *string = NULL;
	do {
		string = plist_getNodeByTagName(plist, "string", string);
		if(!string){
			break;
		}
		printf("> [%p] %s\n", string, string->textContent);
		
		string = NODE_NEXT(string);
	} while(string != NULL);
	
	return EXIT_SUCCESS;
}