/*
	Copyright (C) 2016 inorO <ginoration@gmail.com>
*/


#include <bson.h>
#include <bcon.h>
#include <mongoc.h>

#include <string.h>
#include <stdlib.h>

#include "mvfs_type.h"
#include "mvfs_low.h"


static mongoc_client_t		*client;
static mongoc_database_t	*db; 
static mongoc_collection_t	*folder_coll, //collection
							*children_coll,
							*content_coll,
							*root_coll;

const char					*serverN = "mongodb://localhost:27017",
							*clientN = "MVFSC",
							*dbN = "MVFS_TEST",
							*rootN = "root",
							*folderN = "folder",
							*childrenN = "subFolder",
							*contentN = "subFile",
							*emptyListJ = "{\"list\" : []}",
							*optListOneJ = "{list.$ : 1}";

static char                 rootFolderID[25];

static folder_t				rootFolder;

const bson_t 				*optListOne;


int FSIDnew(FSID *fsid){
	*fsid = 1;

	return 0;
}		


void bsonAppendFSID(bson_t *bson,
			 FSID fsid){
	bson_append_int32(bson, "FSID", -1, fsid);
}


int bsonAppendOidStr(bson_t *bson,
			 const char *name,
			 const char *oidStr){
	bson_oid_t oid;
	bson_oid_init_from_string(&oid, oidStr);
	BSON_APPEND_OID(bson, name, &oid);
	return 0;
}


int bsonInitOidAndCopy(bson_oid_t *oid,
			 char *oidStr){
	bson_oid_init(oid, NULL);
	bson_oid_to_string(oid, oidStr);
}


cJSON *bsonToJson(const bson_t *bson){ //new json from bson
	char *str = bson_as_json(bson, NULL);
	cJSON *json = cJSON_Parse(str);
	bson_free(str);

	return json;
}


int jsonGetOidS(const cJSON *json,
			 const char *name,
			 char *oidStr){
	strcpy(oidStr, cJSON_GetObjectItem(cJSON_GetObjectItem(json, name), "$oid")->valuestring);

	return 0;
}


cJSON *jsonGetListFisrtJ(cJSON *json){
	cJSON *res;
	cJSON *list;

	list = cJSON_GetObjectItem(json, "list");
	res = cJSON_GetArrayItem(list, 0);

	return res;
}


cJSON *mongoFindOneJ(mongoc_collection_t *coll,
			 const bson_t *query){ //mongo find one (return json)
	const bson_t *doc;
	mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(coll, query, NULL, NULL);
	cJSON *json;
	if (mongoc_cursor_next(cursor, &doc)) {
		json = bsonToJson(doc);
	} else {
		json = NULL;
	}

	 mongoc_cursor_destroy (cursor);

	return json;
}

cJSON *mongoFindListOneJ(mongoc_collection_t *coll,
			 const bson_t *query){ //mongo find one (return json)
	const bson_t *doc;
	mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(coll, query, optListOne, NULL);
	cJSON *json;
	if (mongoc_cursor_next(cursor, &doc)) {
		json = bsonToJson(doc);
	} else {
		json = NULL;
	}

	mongoc_cursor_destroy (cursor);

	return json;
}

/*
int mongoInsertJsonAndCopyId(mongoc_collection_t *coll,
			 const char *jsonStr,
			 char *oidStr){
	bson_oid_t oid;
	bson_t *query = bson_new((const uint8_t *)jsonStr, -1, NULL);
	bsonInitOidAndCopy(&oid, oidStr);
	BSON_APPEND_OID(query, "_id", &oid);
	mongoc_collection_insert(coll, MONGOC_INSERT_NONE, query, NULL, NULL);
	bson_destroy(query);

	return 0;
}
*/



///////////////////////***************************///////////////////////
//                         Collction operations
///////////////////////***************************///////////////////////


////////////////////////  folder_coll
//

//db.folder.insert({"_id" : ObjectId(""), "children" : ObjectId(""), "content" : ObjectId("")})
static int mongoInsertFolderAndCopyId(const char *chid,
			 const char *coid,
			 char *oidStr){
	bson_t *query = bson_new();
	bsonAppendOidStr(query, "children", chid);
	bsonAppendOidStr(query, "content", coid);
	bson_oid_t oid;
	bsonInitOidAndCopy(&oid, oidStr);
	BSON_APPEND_OID(query, "_id", &oid);

	mongoc_collection_insert(folder_coll, MONGOC_INSERT_NONE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


//do not need update


//db.folder.remove({"_id" : ObjectID("")})
static int mongoRemoveFolder(char *oidStr){
	bson_t *query = bson_new();
	bsonAppendOidStr(query, "_id", oidStr);

	mongoc_collection_remove(folder_coll, MONGOC_REMOVE_SINGLE_REMOVE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


//db.folder.find({"_id" : ObjectID("")})
static int mongoFindFolder(char *oidStr,
			 cJSON **out){
	bson_t *query = bson_new();
	bsonAppendOidStr(query, "_id", oidStr);

	*out = mongoFindOneJ(folder_coll, query);
/*
	cJSON *json = mongoFindOneJ(folder_coll, query);
	const char *strID = cJSON_GetObjectItem(json, "children")->valuestring;
	strcpy(out->childrenID, strID);
	strID = cJSON_GetObjectItem(json, "content")->valuestring;
	strcpy(out->contentID, strID);
*/

	bson_destroy(query);
	//cJSON_Delete(json);

	return 0;
}


////////////////////////  children_coll
//

//db.children.insert({"_id" : ObjectID(""), "list" : []})
static int mongoInsertChildrenAndCopyID(char *oidStr){
	bson_t *query = bson_new_from_json((const uint8_t *)emptyListJ, -1, NULL);
	bson_oid_t oid;
	bsonInitOidAndCopy(&oid, oidStr);
	BSON_APPEND_OID(query, "_id", &oid);

	mongoc_collection_insert(children_coll, MONGOC_INSERT_NONE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


//db.children.update({"_id" : ObjectId("")}, {"$push": {"list" : {"name" : "name", "id" : ObjectId("")}}})
static int mongoPushChildren(const char *chid,
			 const char *folderID,
			 const char *name){
	bson_t *query = bson_new();
	bson_t *doc = bson_new();
	bson_t *pushb = bson_new();
	bson_t *listb = bson_new();

	bsonAppendOidStr(listb, "id", folderID);
	BSON_APPEND_UTF8(listb, "name", name);
	bson_append_document(pushb, "list", -1, listb);
	bson_append_document(doc, "$push", -1, pushb);

	bsonAppendOidStr(query, "_id", chid);

	mongoc_collection_update(children_coll, MONGOC_UPDATE_NONE, query, doc, NULL, NULL);
	
	bson_destroy(listb);
	bson_destroy(pushb);
	bson_destroy(query);
	bson_destroy(doc);

	return 0;
}


//db.children.update({"_id" : ObjectId("")}, {"$pull": {"list" : {"name" : ""}})
static int mongoPullChildren(const char *chid,
			 const char *name){
	bson_t *query = bson_new();
	bson_t *doc = bson_new();
	bson_t *pullb = bson_new();
	bson_t *listb = bson_new();

	BSON_APPEND_UTF8(listb, "name", name);
	bson_append_document(pullb, "list", -1, listb);
	bson_append_document(doc, "$pull", -1, pullb);
	bsonAppendOidStr(query, "_id", chid);

	mongoc_collection_update(children_coll, MONGOC_UPDATE_NONE, query, doc, NULL, NULL);

	bson_destroy(listb);
	bson_destroy(pullb);
	bson_destroy(query);
	bson_destroy(doc);

	return 0;
}


//db.children.update({"_id" : ObjectId(""), "list.name" : "oldName"}, {"$set": {"list.$.name" : "newName"})
//temporarily not implement


//db.children.remove({"_id" : ObjectID("")})
static int mongoRemoveChildren(const char *oidStr){
	bson_t *query = bson_new();
	bsonAppendOidStr(query, "_id", oidStr);

	mongoc_collection_remove(children_coll, MONGOC_REMOVE_SINGLE_REMOVE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


//db.children.find({"_id" : ObjectID("")})
static int mongoChildrenFind(const char *oidStr,
			 cJSON **json){
	bson_t *query = bson_new();
	bsonAppendOidStr(query, "_id", oidStr);

	*json = mongoFindOneJ(children_coll, query);

	bson_destroy(query);

	return 0;
}


//db.children.find({"_id" : ObjectID(""), "list.name" : "name"}, {"list.$" : 1})
static int mongoChildrenFindSub(const char *oidStr,
			 const char *name,
			 cJSON **json){
	bson_t *query = bson_new();
	bsonAppendOidStr(query, "_id", oidStr);
	BSON_APPEND_UTF8(query, "list.name", name);

	*json = mongoFindListOneJ(children_coll, query);

	bson_destroy(query);

	return 0;
}


//////////////////////// content_coll
//


//db.content.insert({"_id" : ObjectID(""), "list" : []})
static int mongoInsertContentAndCopyID(char *oidStr){
	bson_t *query = bson_new_from_json((const uint8_t *)emptyListJ, -1, NULL);
	bson_oid_t oid;
	bsonInitOidAndCopy(&oid, oidStr);
	BSON_APPEND_OID(query, "_id", &oid);

	mongoc_collection_insert(content_coll, MONGOC_INSERT_NONE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


//db.content.update({"_id" : ObjectId("")}, {"$push": {"list" : {"name" : "name", "FSID" : NumberInt()}}})
static int mongoPushContent(const char *coid,
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

	bsonAppendOidStr(query, "_id", coid);
	mongoc_collection_update(content_coll, MONGOC_UPDATE_NONE, query, doc, NULL, NULL);

	bson_destroy(query);
	bson_destroy(doc);
	return 0;
}


//db.content.update({"_id" : ObjectId("")}, {"$pull": {"list" : {"name" : ""}}})
static int mongoPullcontent(const char *coid,
			 const char *name){
	bson_t *query = bson_new();
	bson_t *doc = bson_new();
	bson_t *pullb = bson_new();
	bson_t *listb = bson_new();

	BSON_APPEND_UTF8(listb, "name", name);
	bson_append_document(pullb, "list", -1, listb);
	bson_append_document(doc, "$pull", -1, pullb);
	bsonAppendOidStr(query, "_id", coid);

	mongoc_collection_update(content_coll, MONGOC_UPDATE_NONE, query, doc, NULL, NULL);

	bson_destroy(listb);
	bson_destroy(pullb);
	bson_destroy(query);
	bson_destroy(doc);

	return 0;
}

//db.content.update({"_id" : ObjectId(""), "list.name" : "oldName"}, {"$set": {"list.$.name" : "newName"})
//temporarily not implement


//db.content.remove({"_id" : ObjectID("")})
static int mongoRemoveContent(const char *oidStr){
	bson_t *query = bson_new();
	bsonAppendOidStr(query, "_id", oidStr);

	mongoc_collection_remove(content_coll, MONGOC_REMOVE_SINGLE_REMOVE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


//db.content.find({"_id" : ObjectID("")})
static int mongoContentFind(const char *oidStr,
			 cJSON **json){
	bson_t *query = bson_new();
	bsonAppendOidStr(query, "_id", oidStr);

	*json = mongoFindOneJ(content_coll, query);

	bson_destroy(query);

	return 0;
}


//db.content.find({"_id" : ObjectID(""), "list.name" : "name"}, {"list.$" : 1})
static int mongoContentFindSub(const char *oidStr,
			 const char *name,
			 cJSON **json){
	bson_t *query = bson_new();
	bsonAppendOidStr(query, "_id", oidStr);
	BSON_APPEND_UTF8(query, "list.name", name);

	*json = mongoFindListOneJ(content_coll, query);

	bson_destroy(query);

	return 0;
}

////////////////////////  root_coll
//


static int mongoInsertRoot(){
	bson_t *query = bson_new();
	char oidStr[25];

	mongoInsertChildrenAndCopyID(rootFolder.childrenID);
	mongoInsertContentAndCopyID(rootFolder.contentID);
	mongoInsertFolderAndCopyId(rootFolder.childrenID, rootFolder.contentID, oidStr);
	bsonAppendOidStr(query, "ID", oidStr);
	mongoc_collection_insert(root_coll, MONGOC_INSERT_NONE, query, NULL, NULL);

	bson_destroy(query);

	return 0;
}


static int mongoGetRootJ(cJSON **out){
	bson_t *query = bson_new();

	*out = mongoFindOneJ(root_coll, query);

	bson_destroy(query);

	return 0;
}

///////////////////////***************************///////////////////////
//                                                                     //
//                            MVFS Operations                          //
//                                                                     //
///////////////////////***************************/////////////////////// 

//get root id
int mvfsGetRootID(char *oidStr){
	cJSON *json;
	mongoGetRootJ(&json);
	if (json == NULL) {
		return 1;
	} else {
		jsonGetOidS(json, "ID", oidStr);
	}

	cJSON_Delete(json);

	return 0;
}//0 success, 1 fail


int mvfsGetFolder(char *oidStr,
			 folder_t *out){
	cJSON *json;
	mongoFindFolder(oidStr, &json);

	if (json == NULL) return 1;
	jsonGetOidS(json, "children", out->childrenID);
	jsonGetOidS(json, "content", out->contentID);

	cJSON_Delete(json);

	return 0;
}//0 success, 1 fail


int mvfsGetChildrenID(const char *chid,
			 const char *name,
			 char *oidStr){
	cJSON *json;
	cJSON *listOne;

	mongoChildrenFindSub(chid, name, &json);
	if (json == NULL)  return 1;
	listOne = jsonGetListFisrtJ(json);
	//strcpy(oidStr, cJSON_GetObjectItem(listOne, "id")->valuestring);
	jsonGetOidS(listOne, "id", oidStr);

	//cJSON_Delete(listOne);
	cJSON_Delete(json);

	return 0;
}//0 found, 1 fail


int mvfsGetContentID(const char *coid,
			 const char *name,
			 FSID *fsid){
	cJSON *json;
	cJSON *listOne;

	mongoContentFindSub(coid, name, &json);
	if (json == NULL)  return 1;
	listOne = jsonGetListFisrtJ(json);
	*fsid = cJSON_GetObjectItem(listOne, "FSID")->valueint;

	//cJSON_Delete(listOne);
	cJSON_Delete(json);

	return 0;
}//0 found, 1 fail


int mvfsGetFullFolder(const folder_t *f,
			 dir_t *dir) {
	cJSON *fullCh, *fullCo,
		  *fullChA, *fullCoA;
	int chS, coS, i, j;

	mongoChildrenFind(f->childrenID, &fullCh);
	mongoContentFind(f->contentID, &fullCo);
	fullChA = cJSON_GetObjectItem(fullCh, "list");
	fullCoA = cJSON_GetObjectItem(fullCo, "list");
	chS = cJSON_GetArraySize(fullChA);
	coS = cJSON_GetArraySize(fullCoA);
	dir->size = chS + coS;
	if (dir->size) dir->array = (const char**)malloc(dir->size * sizeof(char **));
	j = 0;
	for (i = 0; i < chS; ++i, ++j) {
		dir->array[j] = cJSON_GetObjectItem(cJSON_GetArrayItem(fullChA, i), "name")->valuestring;
	}
	for (i = 0; i < coS; ++i, ++j) {
		dir->array[j] = cJSON_GetObjectItem(cJSON_GetArrayItem(fullCoA, i), "name")->valuestring;
	}
	dir->chj = (void*)fullCh;
	dir->coj = (void*)fullCo;

	//cJSON_Delete(fullCh);
	//cJSON_Delete(fullCo);

	return 0;
}


/*
int mvfsCreateFolder(const char *name){
	char chid[25], coid[25], folderID[25]; 
	mongoInsertJsonAndCopyId(children_coll, emptyListJ, chid);
	mongoInsertJsonAndCopyId(content_coll, emptyListJ, coid);

}
*/

/*
*db.children.find({"_id" : ObjectId(""), "list.name" : "name"}, {"list.$" : 1})
*//*
int mvfsFindChildren(const char *chid,
			 const char *name,
			 char *folderID){
	bson_t *query = bson_new();
	bson_oid_t oid;
	bson_oid_init_from_string(chid);
	BSON_APPEND_OID(query, "_id", &oid);
	BSON_APPEND_UTF8(query, "name", name);

	bson_destroy(query);

	return 0;
}//0 : success. 1 : not found.
*/

int extraInit(){
	if (mvfsGetRootID(rootFolderID) == 1){
		mongoInsertRoot();
		mvfsGetRootID(rootFolderID);
	}
	mvfsGetFolder(rootFolderID, &rootFolder);
	
	//opt : listOne:
	//optListOne = bson_new_from_json((const uint8_t *)optListOneJ,  -1, NULL);
	optListOne = BCON_NEW("projection","{", "list.$", BCON_BOOL(true), "}");

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

	serverN = strdup(server_port);
	clientN = strdup(client_name);
	dbN = strdup(db_name);

	return 0;
}


int mvfsInitFromRoot(){

	mongoc_init();
	client = mongoc_client_new(serverN);
	mongoc_client_set_appname(client, clientN);
	db = mongoc_client_get_database(client, dbN);
	folder_coll = mongoc_client_get_collection(client, dbN, folderN);
	children_coll = mongoc_client_get_collection(client, dbN, childrenN);
	content_coll = mongoc_client_get_collection(client, dbN, contentN);
	root_coll = mongoc_client_get_collection(client, dbN, rootN);

	extraInit();

	return 0;
}


int mvfsDestroy(){
	mongoc_collection_destroy(children_coll);
	mongoc_collection_destroy(folder_coll);
	mongoc_collection_destroy(content_coll);
	mongoc_collection_destroy(root_coll);
	mongoc_database_destroy (db);
	mongoc_client_destroy(client);
	mongoc_cleanup();

	return 0;
}


int mvfsGetRootFolder(folder_t *out){
	memcpy(out, &rootFolder, sizeof(folder_t));

	return 0;
}


int mvfsCd(attr_t *attr, 
			 const char *name){
	char oidStr[25];
	FSID fsid;

	if (attr->type == ATTR_TYPE_FILE)  return MVFS_CD_ERROR;
	if (mvfsGetChildrenID((attr->folder).childrenID, name, oidStr) == 0) {
		mvfsGetFolder(oidStr, &(attr->folder));
	} else if (mvfsGetContentID((attr->folder).contentID, name, &fsid) == 0) {
		attr->type = ATTR_TYPE_FILE;
		//attr->file.fsid = fsid;
	} else {
		return MVFS_CD_ERROR;
	}	

	return 0;
}



int mvfsReadFolder(const folder_t *folder,
			 dir_t *out){
	mvfsGetFullFolder(folder, out);

	return 0;
}


int mvfsDeleteDirT(dir_t *dir){
	cJSON_Delete((cJSON*)(dir->chj));
	cJSON_Delete((cJSON*)(dir->coj));
	free(dir->array);
	//free(dir);

	return 0;
}


//NO JUDGE
int mvfsMakeFile(folder_t *folder,
			 const char *name){
	FSID fsid;
	FSIDnew(&fsid);
	mongoPushContent(folder->contentID, fsid, name);

	return 0;
}


//NO JUDGE
int mvfsMakeFolder(folder_t *folder,
			 const char *name){
	char chid[25], coid[25], oidStr[25];
	mongoInsertChildrenAndCopyID(chid);
	mongoInsertContentAndCopyID(coid);
	mongoInsertFolderAndCopyId(chid, coid, oidStr);
	mongoPushChildren(folder->childrenID, oidStr, name);

	return 0;
}


int mvfsRemoveAndCd(attr_t *attr, 
			 const char *name){
	char oidStr[25];
	FSID fsid;

	if (attr->type == ATTR_TYPE_FILE)  return MVFS_CD_ERROR;
	if (mvfsGetChildrenID((attr->folder).childrenID, name, oidStr) == 0) {
		mongoPullChildren((attr->folder).childrenID, name);
		mvfsGetFolder(oidStr, &(attr->folder));
	} else if (mvfsGetContentID((attr->folder).contentID, name, &fsid) == 0) {
		attr->type = ATTR_TYPE_FILE;
		mongoPullcontent((attr->folder).contentID, name);
		//attr->file.fsid = fsid;
	} else {
		return MVFS_CD_ERROR;
	}


	return 0;
}


int mvfsRemoveDir(folder_t *folder){
	mongoRemoveContent(folder->contentID);
	mongoRemoveChildren(folder->childrenID);

	return 0;
}



