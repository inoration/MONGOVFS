/*
	Copyright (C) 2016 inorO <ginoration@gmail.com>
*/


#include <bson.h>
#include <bcon.h>
#include <mongoc.h>

#include "mvfs_type.h"


static mongoc_client_t		*client;
static mongoc_database_t	*db; 
static mongoc_collection_t	*folder_coll, //collection
							*subFolder_coll,
							*subFile_coll,
							*root_coll;

const char 					*serverN = "mongodb://localhost:27017",
							*clientN = "MVFSC",
							*dbN = "MVFS_TEST",
							*rootN = "root",
							*folderN = "folder",
							*subFolderN = "subFolder",
							*subFileN = "subFile",
							*emptyListJ = "{\"list\" : []}";
							//*optListOneJ = "{list.$ : 1}";

const bson_t 				*optListOne;



///////////////////////***************************///////////////////////
//                                                                     //
//                         static functions                            //
//                                                                     //
///////////////////////***************************/////////////////////// 


////// index functions:
/*
	I use ObjectID as index to find file in mongo database.
	You may like to use your own bson_oid_t. If so, edit here.
*/
static int bsonAppendIndex(bson_t *bson,
			 const char *name,
			 const bson_oid_t *oid){

	BSON_APPEND_OID(bson, name, oid);

	return 0;
}


static int bsonGenerateIndex(bson_oid_t *oid){

	bson_oid_init(oid, NULL);

	return 0;
}
////// end index functions.
//////


static int mongoFindOne(mongoc_collection_t *coll,
			 const bson_t *query,
			 const bson_t **out){ //mongo find one (return json)
	int res = 0;

	mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(coll, query, NULL, NULL);
	if (!mongoc_cursor_next(cursor, out)) res = 1;
	mongoc_cursor_destroy (cursor);

	return res;
}//0 success 1 fail


static int mongoFindListOne(mongoc_collection_t *coll,
			 const bson_t *query,
			 bson_t **out){
	int res = 0;

	mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(coll, query, optListOne, NULL);
	if (!mongoc_cursor_next(cursor, out)) res = 1;

	mongoc_cursor_destroy (cursor);

	return res;
}


static int mongoFindListBatch(mongoc_collection_t *coll,
			 const bson_t *query,
			 int start,
			 int size,
			 bson_t **out){
	int res = 0;
	const bson_t *optListBatch = BCON_NEW(
		"projection",
		"{", 
			"list", 
			"{", 
				"$slice", "[", BCON_INT64(start), BCON_INT64(size), "]", 
			"}",
		"}"
		);

	mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(coll, query, optListBatch, NULL);
	if (!mongoc_cursor_next(cursor, out)) res = 1;

	mongoc_cursor_destroy (cursor);

	return res;
}

///////////////////////***************************///////////////////////
//                                                                     //
//                        collection functions                         //
//                                                                     //
///////////////////////***************************/////////////////////// 


////////////////////////   ----------------------------------------------------------------------------- 
////////////////////////   --------------------------------mongo_func-----------------------------------
////////////////////////   ----------------------------------------------------------------------------- 
//


int mongoExtraParameter(const char *server_port, 
			 const char *client_name, 
			 const char *db_name){

	serverN = strdup(server_port);
	clientN = strdup(client_name);
	dbN = strdup(db_name);

	return 0;
}


int mongoInitFromRoot(){

	mongoc_init();
	client = mongoc_client_new(serverN);
	//mongoc_client_set_appname(client, clientN);
	db = mongoc_client_get_database(client, dbN);
	folder_coll = mongoc_client_get_collection(client, dbN, folderN);
	subFolder_coll = mongoc_client_get_collection(client, dbN, subFolderN);
	subFile_coll = mongoc_client_get_collection(client, dbN, subFileN);
	root_coll = mongoc_client_get_collection(client, dbN, rootN);
	optListOne = BCON_NEW("projection","{", "list.$", BCON_BOOL(true), "}");

	return 0;
}


int mongoDestroy(){

	mongoc_collection_destroy(subFolder_coll);
	mongoc_collection_destroy(folder_coll);
	mongoc_collection_destroy(subFile_coll);
	mongoc_collection_destroy(root_coll);
	mongoc_database_destroy (db);
	mongoc_client_destroy(client);
	mongoc_cleanup();

	return 0;
}


////////////////////////   ----------------------------------------------------------------------------- 
////////////////////////   --------------------------------folder_coll----------------------------------
////////////////////////   ----------------------------------------------------------------------------- 
//


//db.folder.insert({"_id" : ObjectId(""), "subFolderID" : ObjectId(""), "subFileID" : ObjectId("")})
int mongoInsertFolderAndCopyId(const bson_oid_t *foid,
			 const bson_oid_t *fiid,
			 bson_oid_t *out){
	bson_t *query = bson_new();

	bsonAppendIndex(query, "subFolderID", foid);
	bsonAppendIndex(query, "subFileID", fiid);
	bsonGenerateIndex(out);
	bsonAppendIndex(query, "_id", out);

	mongoc_collection_insert(folder_coll, MONGOC_INSERT_NONE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


//may not need update


//db.folder.remove({"_id" : ObjectID("")})
int mongoRemoveFolder(bson_oid_t *id){
	bson_t *query = bson_new();

	bsonAppendIndex(query, "_id", id);

	mongoc_collection_remove(folder_coll, MONGOC_REMOVE_SINGLE_REMOVE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


//db.folder.find({"_id" : ObjectID("")})
int mongoFindFolder(bson_oid_t *id,
			 bson_t **out){
	int res = 0;
	bson_t *query = bson_new();

	bsonAppendIndex(query, "_id", id);
	res = mongoFindOne(folder_coll, query, out);

	bson_destroy(query);

	return res;
}

////////////////////////   ----------------------------------------------------------------------------- 
////////////////////////   -----------------------------subFolder_coll----------------------------------
////////////////////////   ----------------------------------------------------------------------------- 
//

//db.subFolder.insert({"_id" : ObjectID(""), "list" : []})
int mongoInsertSubFolderAndCopyID(bson_oid_t *out){
	bson_t *query = bson_new_from_json((const uint8_t *)emptyListJ, -1, NULL);

	bsonGenerateIndex(out);
	bsonAppendIndex(query, "_id", out);

	mongoc_collection_insert(subFolder_coll, MONGOC_INSERT_NONE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


//db.subFolder.update({"_id" : ObjectId("")}, {"$push": {"list" : {"name" : "name", "id" : ObjectId("")}}})
int mongoPushSubFolder(const bson_oid_t *foid,
			 const bson_oid_t *folderID,
			 const char *name){
	bson_t *query = bson_new();
	bson_t *doc = bson_new();

	doc = BCON_NEW("$push", 
    "{", 
        "list", 
            "{", 
                "name",     BCON_UTF8(name), 
                "id", 		BCON_OID(folderID),
            "}", 
    "}");
	bsonAppendIndex(query, "_id", foid);

	mongoc_collection_update(subFolder_coll, MONGOC_UPDATE_NONE, query, doc, NULL, NULL);

	bson_destroy(query);
	bson_destroy(doc);

	return 0;
}


//db.subFolder.update({"_id" : ObjectId("")}, {"$pull": {"list" : {"name" : ""}})
int mongoPullSubFolder(const bson_oid_t *foid,
			 const char *name){
	bson_t *query = bson_new();
	bson_t *doc = bson_new();

	doc = BCON_NEW("$pull", 
    "{", 
        "list", 
            "{", 
                "name",     BCON_UTF8(name), 
            "}", 
    "}");
	bsonAppendIndex(query, "_id", foid);

	mongoc_collection_update(subFolder_coll, MONGOC_UPDATE_NONE, query, doc, NULL, NULL);

	bson_destroy(query);
	bson_destroy(doc);

	return 0;
}


//db.subFolder.update({"_id" : ObjectId(""), "list.name" : "oldName"}, {"$set": {"list.$.name" : "newName"})
//temporarily not implement


//db.subFolder.remove({"_id" : ObjectID("")})
int mongoRemoveSubFolder(const bson_oid_t *foid){
	bson_t *query = bson_new();

	bsonAppendIndex(query, "_id", foid);

	mongoc_collection_remove(subFolder_coll, MONGOC_REMOVE_SINGLE_REMOVE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


//db.subFolder.find({"_id" : ObjectID("")})
int mongoSubFolderFind(const bson_oid_t *foid,
			 bson_t **out){
	int res = 0;
	bson_t *query = bson_new();

	bsonAppendIndex(query, "_id", foid);
	res = mongoFindOne(subFolder_coll, query, out);

	bson_destroy(query);

	return res;
}

//db.subFolder.find({"_id" : ObjectID("")}, {'list': {'$slice': [start, size]]})
int mongoSubFolderFindBatch(const bson_oid_t *foid,
			 int start,
			 int size,
			 bson_t **out){
	int res = 0;
	bson_t *query = bson_new();

	bsonAppendIndex(query, "_id", foid);
	res = mongoFindListBatch(subFolder_coll, query, start, size, out);

	bson_destroy(query);

	return res;
}

//db.subFolder.find({"_id" : ObjectID(""), "list.name" : "name"}, {"list.$" : 1})
int mongoSubFolderFindSub(const bson_oid_t *foid,
			 const char *name,
			 bson_t **out){
	int res;
	bson_t *query = bson_new();

	bsonAppendIndex(query, "_id", foid);
	BSON_APPEND_UTF8(query, "list.name", name);
	res = mongoFindListOne(subFolder_coll, query, out);

	bson_destroy(query);

	return res;
}


////////////////////////   ----------------------------------------------------------------------------- 
////////////////////////   -----------------------------subFile_coll------------------------------------
////////////////////////   ----------------------------------------------------------------------------- 
//


//db.subFile.insert({"_id" : ObjectID(""), "list" : []})
int mongoInsertSubFileAndCopyID(bson_oid_t *fiid){
	bson_t *query = bson_new_from_json((const uint8_t *)emptyListJ, -1, NULL);

	bsonGenerateIndex(fiid);
	bsonAppendIndex(query, "_id", fiid);

	mongoc_collection_insert(subFile_coll, MONGOC_INSERT_NONE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


//db.subFile.update({"_id" : ObjectId("")}, {"$push": {"list" : {"name" : "name", "FSID" : NumberInt()}}})
int mongoPushSubFile(const bson_oid_t *fiid,
			 const FSID fsid,
			 const char *name){
	bson_t *query = bson_new();
	bson_t *doc = bson_new();

	doc = BCON_NEW("$push", 
    "{", 
        "list", 
            "{", 
                "name",     BCON_UTF8(name), 
                "FSID", 	BCON_INT32(fsid),
            "}", 
    "}");
	bsonAppendIndex(query, "_id", fiid);

	mongoc_collection_update(subFile_coll, MONGOC_UPDATE_NONE, query, doc, NULL, NULL);

	bson_destroy(query);
	bson_destroy(doc);
	return 0;
}


//db.subFile.update({"_id" : ObjectId("")}, {"$pull": {"list" : {"name" : ""}}})
int mongoPullSubFile(const bson_oid_t *fiid,
			 const char *name){
	bson_t *query = bson_new();
	bson_t *doc = bson_new();

	doc = BCON_NEW("$pull", 
    "{", 
        "list", 
            "{", 
                "name",     BCON_UTF8(name), 
            "}", 
    "}");
	bsonAppendIndex(query, "_id", fiid);

	mongoc_collection_update(subFile_coll, MONGOC_UPDATE_NONE, query, doc, NULL, NULL);

	bson_destroy(query);
	bson_destroy(doc);

	return 0;
}


//db.content.update({"_id" : ObjectId(""), "list.name" : "oldName"}, {"$set": {"list.$.name" : "newName"})
//temporarily not implement


//db.subFile.remove({"_id" : ObjectID("")})
int mongoRemoveSubFile(const bson_oid_t *fiid){
	bson_t *query = bson_new();

	bsonAppendIndex(query, "_id", fiid);

	mongoc_collection_remove(subFile_coll, MONGOC_REMOVE_SINGLE_REMOVE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


//db.subFile.find({"_id" : ObjectID("")})
int mongoSubFileFind(const bson_oid_t *fiid,
			 bson_t **out){
	int res = 0;
	bson_t *query = bson_new();

	bsonAppendIndex(query, "_id", fiid);

	res = mongoFindOne(subFile_coll, query, out);

	bson_destroy(query);

	return res;
}


//db.subFile.find({"_id" : ObjectID("")}, {'list': {'$slice': [start, size]]})
int mongoSubFileFindBatch(const bson_oid_t *fiid,
			 int start,
			 int size,
			 bson_t **out){
	int res = 0;
	bson_t *query = bson_new();

	bsonAppendIndex(query, "_id", fiid);
	res = mongoFindListBatch(subFile_coll, query, start, size, out);

	bson_destroy(query);

	return res;
}


//db.subFile.find({"_id" : ObjectID(""), "list.name" : "name"}, {"list.$" : 1})
int mongoSubFileFindSub(const bson_oid_t *fiid,
			 const char *name,
			 bson_t **out){
	int res = 0;
	bson_t *query = bson_new();

	bsonAppendIndex(query, "_id", fiid);
	BSON_APPEND_UTF8(query, "list.name", name);
	res = mongoFindListOne(subFile_coll, query, out);

	bson_destroy(query);

	return res;
}


////////////////////////   ----------------------------------------------------------------------------- 
////////////////////////   -----------------------------root_coll---------------------------------------
////////////////////////   ----------------------------------------------------------------------------- 
//

//db.root.insert({"_id" : ObjectID(""), "ID" : ObjectID("")})
int mongoInsertRoot(const bson_oid_t *rid){
	bson_t *query = bson_new();

	bsonAppendIndex(query, "ID", rid);
	mongoc_collection_insert(root_coll, MONGOC_INSERT_NONE, query, NULL, NULL);
/*
	mongoInsertChildrenAndCopyID(rootFolder.childrenID);
	mongoInsertContentAndCopyID(rootFolder.contentID);
	mongoInsertFolderAndCopyId(rootFolder.childrenID, rootFolder.contentID, oidStr);
	bsonAppendOidStr(query, "ID", oidStr);
	mongoc_collection_insert(root_coll, MONGOC_INSERT_NONE, query, NULL, NULL);
*/
	bson_destroy(query);

	return 0;
}

//db.root.find({})
int mongoGetRoot(bson_t **out){
	int res = 0;

	bson_t *query = bson_new();
	res = mongoFindOne(root_coll, query, out);

	bson_destroy(query);

	return res;
}