/*
	Copyright (C) 2016 Smx
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/stat.h>
#include "plist.h"

#define PLIST_READTO(plist, end) plist_readTo(plist, (char *)end, strlen(end))
#define zalloc(size) calloc(1, size)
#define strlencmp(x, y) strncmp(x, y, strlen(y))

static unsigned int PLIST_DEBUG = false;
static unsigned int PLIST_FORCE = false;

void debug(const char *format, ...);
void plist_node_free(PLIST_NODE *node);

void PLIST_SET_DEBUG(int value){
	PLIST_DEBUG = value;
}
void PLIST_FORCE_LOAD(int value){
	PLIST_FORCE = value;
}

int plist_readTo(PLIST *plist, char *to, int size){
	int count;
	for(count=0; ; plist->offset++, plist->data++, count++){
		if(!strncmp(plist->data, to, size)){
			return count;
		}
		if(plist_feof(plist)){
			return -1;
		}
	}
}

int plist_nextLine(PLIST *plist){
	int ret = PLIST_READTO(plist, "\n");
	plist->data++;
	if(ret < 0 || ++plist->offset > plist->size){
		return ret;
	}
	return true;
}

void plist_rewind(PLIST *plist){
	plist->offset = 0;
	plist->data = plist->dataStart;
}

int plist_fgetc(PLIST *plist){
	if(plist_feof(plist)){
		return -1;
	}
	plist->data++; plist->offset++;
	return (char)*(plist->data);
}

int plist_feof(PLIST *plist){
	if(plist->offset >= plist->size){
		return true;
	}
	return false;
}

void debug(const char *format, ...){
	if(PLIST_DEBUG){
		va_list args;
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}
}

int plist_fseek(PLIST *plist, int offset, int origin){
	switch(origin){
		case SEEK_SET:
			plist->offset = offset;
			plist->data = plist->dataStart + offset;
			break;
		case SEEK_CUR:
			plist->offset += offset;
			plist->data += offset;
			break;
		case SEEK_END:
			plist->offset = plist->size + offset;
			plist->data = plist->dataStart + plist->size + offset;
			break;
		default:
			return false;
			break;
	}
	return true;
}

int plist_isValid(PLIST *plist){
	int plistCheckLevel = 0;
	PLIST_READTO(plist, "<");
	if(!strlencmp(plist->data, "<?xml")){
		debug("<?xml matches\n");
		plistCheckLevel++;
		if(!plist_nextLine(plist)) return false;
	}
	if(!strlencmp(plist->data, "<!DOCTYPE plist PUBLIC")){
		debug("<!DOCTYPE plist PUBLIC matches\n");
		plistCheckLevel++;
		if(!plist_nextLine(plist)) return false;
	}
	int readCount = PLIST_READTO(plist, "<plist");
	if(readCount > 0 && !strlencmp(plist->data, "<plist")){
		debug("<plist matches\n");
		plistCheckLevel++;
	} else {
		if(PLIST_FORCE){
			plist_rewind(plist);
			int read = PLIST_READTO(plist, "<");
			if(read < 0){
				fprintf(stderr, "Not a valid plist file\n");
				return false;
			}
			return true;
		} else {
			debug("Fatal!, cannot find the root node!\n");
			return false;
		}
	}
	if(plistCheckLevel < 1) return false;
	debug("Alright then, it's a valid plist :)\n");
	return true;
}

int plist_text2node(PLIST *plist, PLIST_NODE *prev, NODE_TYPE type){
	debug("\n");
	if(*(plist->data) != '<'){
		debug("invalid node (marker is '%c', expected '<')\n", *(plist->data));
		return false;
	}
	PLIST_NODE *node = zalloc(sizeof(PLIST_NODE));
	
	if(!plist->root || !prev){
		debug("setting root node\n");
		plist->root = node;
	} else {
		switch(type){
			case NODE_RIGHT:
				prev->next = node;
				node->prev = prev;
				node->parent = prev->parent;
				break;
			case NODE_CHILD:
				prev->children = node;
				node->parent = prev;
				break;
			case NODE_LEFT:
			case NODE_PARENT:
			default:
				break;
		}
	}
	
	
	int tagNameSize = PLIST_READTO(plist, ">");
	debug("Tagsize: %d\n",tagNameSize); 
	if(tagNameSize <= 0){
		return false;
	}
	tagNameSize--;
	
	plist_fseek(plist, -tagNameSize, SEEK_CUR);
	char *tagName = zalloc(tagNameSize + 1);
	strncpy(tagName, plist->data, tagNameSize);
	debug("Tag: %s\n", tagName);
	node->tagName = tagName;
	plist_fseek(plist, tagNameSize, SEEK_CUR);
	int contentSize = PLIST_READTO(plist, "<");
	if(contentSize < 0){
		return false;
	}
	contentSize--;
	
	if(plist_fgetc(plist) != '/'){ //it's not tag end. a children starts
		debug("child node detected\n");
		plist_fseek(plist, -1, SEEK_CUR);
		plist_text2node(plist, node, NODE_CHILD);
	} else {
		plist_fseek(plist, -1-contentSize, SEEK_CUR);
		char *textContent = zalloc(contentSize + 1);
		strncpy(textContent, plist->data, contentSize);
		debug("Content: %s\n", textContent);
		node->textContent = textContent;
	}
	while(!plist_feof(plist)){
		int c = PLIST_READTO(plist, "<");
		if(c < 0) break;
		if(plist_fgetc(plist) == '/'){
			if(!strlencmp(plist->data, tagName)){
				break; //we reached the end of the level (right/left nodes)
			} else {
				continue; //it's the end of a tag in the same level
			}
		}
		debug("right node detected\n");
		plist_fseek(plist, -1, SEEK_CUR);
		plist_text2node(plist, node, NODE_RIGHT);
	}
	return true;
}

PLIST_NODE *plist_getNodebyField(PLIST_NODE *node, NODE_FIELD field, const char *fieldValue){
	debug("%s: ", node->tagName);
	if(node->textContent){
		debug("%s", node->textContent);
	}
	debug("\n");
	switch(field){
		case NODE_TAGNAME:
			if(node->tagName && !strlencmp(node->tagName, fieldValue)){
				return node;
			}
			break;
		case NODE_TEXTCONTENT:
			if(node->textContent && !strlencmp(node->textContent, fieldValue)){
				return node;
			}
			break;
	}
	PLIST_NODE *result;
	if(node->next){
		debug("going next\n");
		result = plist_getNodebyField(node->next, field, fieldValue);
		if(result){
			return result;
		}
	}
	if(node->children){
		debug("going child %s\n");
		result = plist_getNodebyField(node->children, field, fieldValue);
		if(result){
			return result;
		}
	}
	return NULL;
}

PLIST_NODE *plist_getNodeByTagName(PLIST *plist, const char *tagName, PLIST_NODE *prev){
	if(prev){
		return plist_getNodebyField(prev, NODE_TAGNAME, tagName);
	} else {
		return plist_getNodebyField(plist->root, NODE_TAGNAME, tagName);
	}
}

PLIST_NODE *plist_getNodeByTextContent(PLIST *plist, const char *textContent, PLIST_NODE *prev){
	if(prev){
		return plist_getNodebyField(prev, NODE_TEXTCONTENT, textContent);
	} else {
		return plist_getNodebyField(plist->root, NODE_TEXTCONTENT, textContent);
	}
}

PLIST_NODE *plist_getNodeByKey(PLIST *plist, const char *keyName, PLIST_NODE *prev){
	PLIST_NODE *result;
	if(prev){
		result = plist_getNodeByTextContent(plist, keyName, prev);
	} else {
		result = plist_getNodeByTextContent(plist, keyName, plist->root);
	}
	if(result && result->next){
		return result->next;
	}
	return (PLIST_NODE *)NULL;
}

void plist_node_free(PLIST_NODE *node){
	if(node && node->next){
		debug("going next\n");
		plist_node_free(node->next);
	}
	if(node && node->children){
		debug("going child %s\n");
		plist_node_free(node->children);
	} else {
		if(node){
			debug("free call\n");
			free(node);
		}
	}
}

int plist_close(PLIST *plist){
	if(plist){
		if(plist->data){
			free(plist->data);
		}
		if(plist->root){
			plist_node_free(plist->root);
		}
		free(plist);
	} else {
		return false;
	}
	return true;
}

size_t getFileSize(int fd){
	struct stat statBuf;
	if(fstat(fd, &statBuf) < 0)
		return 0;
	return statBuf.st_size;
}

int plist_open(const char *filename, PLIST *plist){
	FILE *file;
	file = fopen(filename, "r");
	if(!file){
		debug("open fail\n");
		goto clean_return;
	}
	plist->size = getFileSize(fileno(file));
	if(plist->size <= 0){
		debug("size inval\n");
		goto clean_return;
	}
	plist->data = zalloc(plist->size);
	if(!plist->data){
		debug("malloc fail\n");
		goto clean_return;
	}
	plist->dataStart = plist->data;
	if(fread (plist->data, plist->size, 1, file) < 1){
		fprintf(stderr, "premature end of file %s", filename);
		goto clean_return;
	}
	if(!(plist->data)){
		debug("data fail\n");
		goto clean_return;
	}
	if(plist_isValid(plist)){
		return plist_text2node(plist, 0, NODE_PARENT);
	} else {
		debug("verif fail\n");
		goto clean_return;
	}
	return 1;
		
	clean_return:
		if(file){
			fclose(file);
		}
		if(plist){
			plist_close(plist);
		}
		return 0;
}

