/*
	Copyright (C) 2016 inorO <ginoration@gmail.com>
*/


#include <bson.h>
#include <bcon.h>
#include <mongoc.h>

#include <string.h>
#include <stdlib.h>

#include "mvfs_type.h"
#include "mvfs_mongo.h"


static bson_oid_t 			rootFolderID;

static folder_t				rootFolder;

#define MVFS_CD_ERROR 1


///////////////////////***************************///////////////////////
//                                                                     //
//                        MVFS static functions                        //
//                                                                     //
///////////////////////***************************/////////////////////// 


static int getIndexFromBson(const bson_t *doc,
			 const char *name,
			 bson_oid_t *out){
	bson_iter_t iter;

	bson_iter_init(&iter, doc);
	bson_iter_find(&iter, name);
	bson_oid_copy(bson_iter_oid(&iter), out);

	return 0;
}


static int bsonGetListFisrtID(const bson_t *doc,
			 bson_oid_t *out){
	bson_iter_t iter;
	bson_iter_t subIter;
	bson_iter_t idIter;

	bson_iter_init(&iter, doc);
	bson_iter_find(&iter, "list");
	if (bson_iter_recurse(&iter, &subIter)) {
		if (bson_iter_next(&subIter)) {
			bson_iter_recurse(&subIter, &idIter);
			bson_iter_find(&idIter, "id");	
			bson_oid_copy(bson_iter_oid(&idIter), out);
		}
	}

	return 0;
}


static int bsonGetListFisrtFSID(const bson_t *doc,
			 FSID *out){
	bson_iter_t iter;
	bson_iter_t subIter;
	bson_iter_t idIter;

	bson_iter_init(&iter, doc);
	bson_iter_find(&iter, "list");
	if (bson_iter_recurse(&iter, &subIter)) {
		if (bson_iter_next(&subIter)) {
			bson_iter_recurse(&subIter, &idIter);
			bson_iter_find(&idIter, "FSID");
			*out = bson_iter_int32(&idIter);
		}
	}

	return 0;
}


////// FSID functions.
/*
	FSID is an file bson_oid_t in distributed file system.
	Here I assume that it is a int32.
*/
static int FSIDnew(FSID *fsid){
	*fsid = 1;

	return 0;
}		


static void bsonAppendFSID(bson_t *bson,
			 FSID fsid){
	bson_append_int32(bson, "FSID", -1, fsid);
}
////// end FSID functions.
//////


static int CreateFolderAndCopyID(bson_oid_t *out){
	folder_t f;

	mongoInsertSubFolderAndCopyID(&f.subFolderID);
	mongoInsertSubFileAndCopyID(&f.subFileID);
	mongoInsertFolderAndCopyId(&f.subFolderID, &f.subFileID, out);

	return 0;
}


static int GetRootID(bson_oid_t *out){
	int res = 0;
	bson_t *doc;

	res = mongoGetRoot(&doc);
	if (res == 0){
		getIndexFromBson(doc, "ID", out);
	}

	return res;
}//0 success, 1 fail


static int InsertRoot(){
	bson_oid_t id;

	CreateFolderAndCopyID(&id);
	mongoInsertRoot(&id);

	return 0;
}


static int GetSubFolderID(const bson_oid_t *foid,
			 const char *name,
			 bson_oid_t *fid){
	bson_t *doc;

	if (mongoSubFolderFindSub(foid, name, &doc) == 1) return 1;
	bsonGetListFisrtID(doc, fid);

	return 0;
}//0 found, 1 fail


static int GetSubFileID(const bson_oid_t *fiid,
			 const char *name,
			 FSID *fsid){
	bson_t *doc;

	if (mongoSubFileFindSub(fiid, name, &doc) == 1) return 1;
	bsonGetListFisrtFSID(doc, fsid);

	return 0;
}//0 found, 1 fail


static int GetFolder(bson_oid_t *id,
			 folder_t *f){
	int res;
	bson_t *doc;

	res = mongoFindFolder(id, &doc);
	if (res == 0) {
		getIndexFromBson(doc, "subFolderID", &(f->subFolderID));
		getIndexFromBson(doc, "subFileID", &(f->subFileID));
	}

	return res;
}//0 success, 1 fail


static int extraInit(){

	if (GetRootID(&rootFolderID) == 1){
		InsertRoot();
		GetRootID(&rootFolderID);
	}
	GetFolder(&rootFolderID, &rootFolder);

	return 0;
}


static int GetFullFolder(const folder_t *f,
			 dir_t *dir,
			 int start,
			 int batchSize) {
	uint32_t len;
	bson_t *doc;
	bson_iter_t iter;
	bson_iter_t subIter;
	bson_iter_t idIter;

	dir->size = 0;
	mongoSubFolderFindBatch(&(f->subFolderID), start, batchSize, &doc);
	bson_iter_init_find(&iter, doc, "list");
	if (bson_iter_recurse(&iter, &subIter)) {
		while (bson_iter_next(&subIter)) {
			bson_iter_recurse(&subIter, &idIter);
			bson_iter_find(&idIter, "name");
			dir->array[dir->size] = strdup(bson_iter_utf8(&idIter, NULL));
			++dir->size;
		}
	}
	mongoSubFileFindBatch(&(f->subFileID), start, batchSize, &doc);
	bson_iter_init_find(&iter, doc, "list");
	if (bson_iter_recurse(&iter, &subIter)) {
		while (bson_iter_next(&subIter)) {
			bson_iter_recurse(&subIter, &idIter);
			bson_iter_find(&idIter, "name");
			dir->array[dir->size] = strdup(bson_iter_utf8(&idIter, NULL));
			++dir->size;
		}
	}

	return 0;
}


///////////////////////***************************///////////////////////
//                                                                     //
//                            MVFS Operations                          //
//                                                                     //
///////////////////////***************************/////////////////////// 


int mvfsExtraParameter(const char *server_port, 
			 const char *client_name, 
			 const char *db_name){

	mongoExtraParameter(server_port, client_name, db_name);

	return 0;
}


int mvfsInitFromRoot(){

	mongoInitFromRoot();

	extraInit();

	return 0;
}


int mvfsDestroy(){

	mongoDestroy();

	return 0;
}


int mvfsGetRootFolder(folder_t *out){

	memcpy(out, &rootFolder, sizeof(folder_t));

	return 0;
}


int mvfsCd(attr_t *attr, 
			 const char *name){
	FSID fsid;
	bson_oid_t id;

	if (attr->type == ATTR_TYPE_FILE)  return MVFS_CD_ERROR;
	if (GetSubFolderID(&((attr->folder).subFolderID), name, &id) == 0) {
		GetFolder(&id, &(attr->folder));
	} else if (GetSubFileID(&((attr->folder).subFileID), name, &fsid) == 0) {
		attr->type = ATTR_TYPE_FILE;
		//attr->file.fsid = fsid;
	} else {
		return MVFS_CD_ERROR;
	}	

	return 0;
}

int mvfsReadFolder(const folder_t *folder,
			 dir_t *out,
			 int start,
			 int batchSize){

	GetFullFolder(folder, out, start, batchSize);

	return 0;
}

/*
int mvfsDeleteDirT(dir_t *dir){
	free(dir->array);
	//free(dir);

	return 0;
}*/

int mvfsMakeFile(folder_t *folder,
			 const char *name){
	FSID fsid;

	FSIDnew(&fsid);
	mongoPushSubFile(&(folder->subFileID), fsid, name);

	return 0;
}


int mvfsMakeFolder(folder_t *folder,
			 const char *name){
	bson_oid_t fiid, foid, fid;

	mongoInsertSubFolderAndCopyID(&foid);
	mongoInsertSubFileAndCopyID(&fiid);
	mongoInsertFolderAndCopyId(&foid, &fiid, &fid);
	mongoPushSubFolder(&(folder->subFolderID), &fid, name);

	return 0;
}


int mvfsRemoveAndCd(attr_t *attr, 
			 const char *name){
	bson_oid_t id;
	FSID fsid;

	if (attr->type == ATTR_TYPE_FILE)  return MVFS_CD_ERROR;
	if (GetSubFolderID(&((attr->folder).subFolderID), name, &id) == 0) {
		mongoPullSubFolder(&((attr->folder).subFolderID), name);
		GetFolder(&id, &(attr->folder));
		mongoRemoveFolder(&id);
	} else if (GetSubFileID(&((attr->folder).subFileID), name, &fsid) == 0) {
		attr->type = ATTR_TYPE_FILE;
		mongoPullSubFile(&((attr->folder).subFileID), name);
		//attr->file.fsid = fsid;
	} else {
		return MVFS_CD_ERROR;
	}


	return 0;
}


int mvfsRemoveDir(folder_t *folder){
	mongoRemoveSubFolder(&(folder->subFolderID));
	mongoRemoveSubFile(&(folder->subFileID));

	return 0;
}