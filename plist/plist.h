/*
	Copyright (C) 2016 Smx 
*/

#ifndef _PLIST_H
#define _PLIST_H

#include <stdint.h>
#include <stdbool.h>

typedef struct PLIST_NODE {
	struct PLIST_NODE *parent; //previous level dad node
	struct PLIST_NODE *prev; //previous node in the same level
	struct PLIST_NODE *next; //next node in the same level
	struct PLIST_NODE *children; //next level first node
	char *tagName;
	char *textContent;
} PLIST_NODE;

typedef struct {
	off_t offset;
	size_t size;
	char *dataStart;
	char *data;
	PLIST_NODE *root;
} PLIST;

typedef enum {
	NODE_PARENT = 0,
	NODE_LEFT,
	NODE_RIGHT,
	NODE_CHILD
} NODE_TYPE;

typedef enum {
	NODE_TAGNAME = 0,
	NODE_TEXTCONTENT
} NODE_FIELD;

static inline PLIST_NODE *NODE_PREV(PLIST_NODE *node){
	if(node->prev)
		return node->prev;
	if(node->parent)
		return node->parent;
	return NULL;
}

static inline PLIST_NODE *NODE_NEXT(PLIST_NODE *node){
	if(node->next)
		return node->next;
	if(node->children)
		return node->children;
	return NULL;
}

int plist_readTo(PLIST *plist, char *to, int size);
int plist_nextLine(PLIST *plist);
void plist_rewind(PLIST *plist);
int plist_fgetc(PLIST *plist);
int plist_feof(PLIST *plist);
int plist_fseek(PLIST *plist, int offset, int origin);
int plist_isValid(PLIST *plist);
int plist_text2node(PLIST *plist, PLIST_NODE *prev, NODE_TYPE type);
int plist_open(const char *filename, PLIST *plist);
int plist_isvalid(PLIST *plist);
int plist_close(PLIST *plist);
void PLIST_SET_DEBUG(int value);
void PLIST_FORCE_LOAD(int value);
PLIST_NODE *plist_getNodeByTagName(PLIST *plist, const char *tagName, PLIST_NODE *prev);
PLIST_NODE *plist_getNodeByTextContent(PLIST *plist, const char *textContent, PLIST_NODE *prev);
PLIST_NODE *plist_getNodeByKey(PLIST *plist, const char *tagName, PLIST_NODE *prev);
PLIST_NODE *plist_getNodebyField(PLIST_NODE *node, NODE_FIELD field, const char *fieldValue);
#endif