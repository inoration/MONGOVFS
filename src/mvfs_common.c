/*
	Copyright (C) 2016 inorO <ginoration@gmail.com>
*/

#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <bson.h>

#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "mvfs_type.h"
#include "mvfs_connection.h"


#define GET_ATTR_ERROR 1
#define GET_FATHER_ERROR 1
#define MAKE_FILE_ERROR 1


const char					*tok = " /";
const char 					*logName = "log";
static FILE 				*logFile;



///////////////////////***************************///////////////////////
//                                                                     //
//                            static functions                         //
//                                                                     //
///////////////////////***************************///////////////////////

////// read attr functions.
static int setFolderStat(const folder_t *f,
			 struct stat *stbuf){
	stbuf->st_mode = S_IFDIR | 0755;
	stbuf->st_nlink = 2;
}


static int setFileStat(const file_t *f,
			 struct stat *stbuf){
	stbuf->st_mode = S_IFREG | 0444;
	stbuf->st_nlink = 1;
}


static int getAttrAndCopyStruct(char *path,
			 attr_t *attr){
	int res = 0;	
	char *p = strtok(path, tok);

	attr->type = ATTR_TYPE_FOLDER;
	mvfsGetRootFolder(&(attr->folder));

	while (p) {
		if (mvfsCd(attr, p) == MVFS_CD_ERROR) {
			res = GET_ATTR_ERROR;
			break;
		}
		p = strtok(NULL, tok);
	}

	return res;
}
////// end read attr functions.


static int getFatherAttr(char *path,
			 attr_t *attr,
			 char **final){
	int res = 0;
	char *p = strtok(path, tok);
	char *prep = p;

	attr->type = ATTR_TYPE_FOLDER;
	mvfsGetRootFolder(&(attr->folder));

	if (!p){
		res = GET_FATHER_ERROR;
	} else {
		p = strtok(NULL, tok);
		while (p) {
			if (mvfsCd(attr, prep) == MVFS_CD_ERROR) {
				res = GET_ATTR_ERROR;
				break;
			}
			prep = p;
			p = strtok(NULL, tok);
		}
		*final = prep;
	}

	return res;
}


static int makeFile(attr_t *attr,
			 const char *name){
	int res = 0;

	if (attr->type == ATTR_TYPE_FILE) {
		res = MAKE_FILE_ERROR;
	} else {
		mvfsMakeFile(&(attr->folder), name);
	}

	return res;
}


static int makeFolder(attr_t *attr,
			 const char *name){
	int res = 0;

	if (attr->type == ATTR_TYPE_FILE) {
		res = MAKE_FILE_ERROR;
	} else {
		mvfsMakeFolder(&(attr->folder), name);
	}

	return res;
}


static int removeDir(attr_t *attr,
			 const char *name){

	mvfsRemoveAndCd(attr, name);
	mvfsRemoveDir(&(attr->folder));

	return 0;
}


static int removeFile(attr_t *attr,
			 const char *name){

	mvfsRemoveAndCd(attr, name);

	return 0;
}


////// log functions.
static char* currentTime(){

	static time_t rawtime;
    static struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    return asctime(timeinfo);
}


static int logPrintWithTime1(const char *str){

	//fprintf(logFile, "%s : [%s]\n", currentTime(), str);
	//fflush(logFile);

	return 0;
}


static int logPrintWithTime2(const char *str1,
			 const char *str2){

	//fprintf(logFile, "%s : [%s : %s]\n", currentTime(), str1, str2);
	//fflush(logFile);

	return 0;
}


static int logClose(){

	fclose(logFile);

	return 0;
}

static int logOpen(){

	logFile = fopen(logName, "a+");

	return 0;
}

////// end log functions.


///////////////////////***************************///////////////////////
//                                                                     //
//                            MVFS functions                           //
//                                                                     //
///////////////////////***************************/////////////////////// 


int mvfs_extraInit(const char *server_port, 
			 const char *client_name, 
			 const char *db_name, 
			 const char *tok_str, 
			 const char *log_file){

	tok = strdup(tok_str);
	logName = strdup(log_file);
	mvfsExtraParameter(server_port, client_name, db_name);

	return 0;
}


void *mvfs_init(struct fuse_conn_info *conn,
			 struct fuse_config *cfg){
	(void)conn;
	(void)cfg;

	logOpen();
	mvfsInitFromRoot();
	logPrintWithTime1("MVFS init!");

	return NULL;
}


void mvfs_destroy(void *p){

	mvfsDestroy();
	logPrintWithTime1("MVFS destroy!");
	logClose();
}


int mvfs_getattr(const char *path, 
			 struct stat *stbuf,
			 struct fuse_file_info *fi){

	logPrintWithTime2("MVFS get attr1 :", path);
	(void) fi;
	int res = 0;
	char *cp = strdup(path); // copy path
	attr_t attr;
	memset(stbuf, 0, sizeof(struct stat));

	if (getAttrAndCopyStruct(cp, &attr) == GET_ATTR_ERROR) {
		res = -ENOENT;
	} else {
		switch (attr.type) {
			case ATTR_TYPE_FOLDER:
				setFolderStat(&(attr.folder), stbuf);
				break;
			case ATTR_TYPE_FILE:
				setFileStat(&(attr.file), stbuf);
				break;
		}
	}

	free(cp);

	logPrintWithTime2("MVFS get attr :", path);

	return res;
}


int mvfs_readdir(const char *path, 
			 void *buf, 
			 fuse_fill_dir_t filler,
			 off_t offset, 
			 struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags){
	(void) offset;
	(void) fi;
	(void) flags;
	int i;
	int res = 0;
	dir_t dir;
	attr_t attr;
	char *cp = strdup(path); // copy path
	char namebuf[200];
	int p = 0;
	int lenentry, off, nextoff = 0;
	struct stat st;

	getAttrAndCopyStruct(cp, &attr);
	dir.array = (char**)malloc(2 * BATCHSIZE * sizeof(char**));
	mvfsReadFolder(&(attr.folder), &dir, p, BATCHSIZE);
	p += BATCHSIZE;

	while (dir.size) {
		
		for (int i = 0; i < dir.size; ++i) {
			strcpy(namebuf, (dir.array)[i]);
			lenentry = ((24+strlen(namebuf)+7)&~7);
			off = nextoff; 
			
			nextoff += lenentry;
			
			if (off<offset) continue;
			if (filler(buf, namebuf, &st, nextoff, 0)) goto stop;
		}
		for (int i = 0; i < dir.size; ++i) {
			free((dir.array)[i]);
		}
		dir.size = 0;

		mvfsReadFolder(&(attr.folder), &dir, p, BATCHSIZE);
		p += BATCHSIZE;
	}

	stop:

	for (int i = 0; i < dir.size; ++i) {
			free((dir.array)[i]);
	}

	free(cp);

	logPrintWithTime2("MVFS read dir :", path);

	return res;
}


int mvfs_mknod(const char *path, 
			 mode_t mode, 
			 dev_t dev){
	(void) mode;
	(void) dev;
	int res = 0;
	attr_t attr;
	char *cp = strdup(path);
	char *name;

	getFatherAttr(cp, &attr, &name);
	makeFile(&attr, name);

	free(cp);

	logPrintWithTime2("MVFS make node :", path);

	return res;
}


int mvfs_mkdir(const char *path, 
			 mode_t mode){
	(void)mode;
	int res = 0;
	attr_t attr;
	char *cp = strdup(path);
	char *name;

	getFatherAttr(cp, &attr, &name);
	makeFolder(&attr, name);

	free(cp);
	
	logPrintWithTime2("MVFS make dir :", path);

	return res;
}


int mvfs_rmdir(const char *path){
	int res = 0;
	attr_t attr;
	char *cp = strdup(path);
	char *name;

	getFatherAttr(cp, &attr, &name);
	removeDir(&attr, name);

	free(cp);

	logPrintWithTime2("MVFS remove dir :", path);

	return res;
}


int mvfs_unlink(const char *path){
	int res = 0;
	attr_t attr;
	char *cp = strdup(path);
	char *name;

	getFatherAttr(cp, &attr, &name);
	removeFile(&attr, name);

	free(cp);

	logPrintWithTime2("MVFS remove file :", path);

	return res;
}