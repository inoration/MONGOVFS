/*
	Copyright (C) 2016 inorO <ginoration@gmail.com>
*/

////////////////////////   --------------------------------mongo_func-----------------------------------

int mongoExtraParameter(const char *server_port, const char *client_name, const char *db_name);


int mongoInitFromRoot();


int mongoDestroy();


////////////////////////   --------------------------------folder_coll----------------------------------

int mongoInsertFolderAndCopyId(const bson_oid_t *foid, const bson_oid_t *fiid, bson_oid_t *out);


int mongoRemoveFolder(bson_oid_t *id);


int mongoFindFolder(bson_oid_t *id, bson_t **out);


////////////////////////   -----------------------------subFolder_coll----------------------------------

int mongoInsertSubFolderAndCopyID(bson_oid_t *foid);


int mongoPushSubFolder(const bson_oid_t *foid, const bson_oid_t *folderID, const char *name);


int mongoPullSubFolder(const bson_oid_t *foid, const char *name);


int mongoRemoveSubFolder(const bson_oid_t *foid);


int mongoSubFolderFind(const bson_oid_t *foid, bson_t **out);


int mongoSubFolderFindBatch(const bson_oid_t *foid, int start, int batchsize, bson_t **out);


int mongoSubFolderFindSub(const bson_oid_t *foid, const char *name, bson_t **out);


////////////////////////   -----------------------------subFile_coll------------------------------------

int mongoInsertSubFileAndCopyID(bson_oid_t *fiid);


int mongoPushSubFile(const bson_oid_t *fiid, const FSID fsid, const char *name);


int mongoPullSubFile(const bson_oid_t *fiid, const char *name);


int mongoRemoveSubFile(const bson_oid_t *fiid);


int mongoSubFileFind(const bson_oid_t *fiid, bson_t **out);


int mongoSubFileFindBatch(const bson_oid_t *fiid, int start, int batchsize, bson_t **out);


int mongoSubFileFindSub(const bson_oid_t *fiid, const char *name, bson_t **out);


////////////////////////   -----------------------------root_coll---------------------------------------

int mongoInsertRoot(const bson_oid_t *rid);


int mongoGetRoot(bson_t  **out);