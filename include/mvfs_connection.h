/*
	Copyright (C) 2016 inorO <ginoration@gmail.com>
*/

int mvfsExtraParameter(const char *server_port, const char *client_name, const char *db_name);


int mvfsInitFromRoot();


int mvfsDestroy();


int mvfsGetRootFolder(folder_t *out);


#define MVFS_CD_ERROR 1
int mvfsCd(attr_t *attr, const char *name);


int mvfsReadFolder(const folder_t *folder, dir_t *out, int start, int batchSize);


int mvfsDeleteDirT(dir_t *dir);


int mvfsMakeFile(folder_t *folder, const char *name);


int mvfsMakeFolder(folder_t *folder, const char *name);


int mvfsRemoveAndCd(attr_t *attr, const char *name);


int mvfsRemoveDir(folder_t *folder);


