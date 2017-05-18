/*
	Copyright (C) 2016 inorO <ginoration@gmail.com>
*/

////mvfs CONFIG:
#define BATCHSIZE 100
////


typedef int FSID;
//typedef bson_oid_t index;


typedef struct folder_ts{
	bson_oid_t subFolderID;
	bson_oid_t subFileID;
}folder_t;


typedef struct file_ts{
	FSID fsid;
}file_t;


#define ATTR_TYPE_FOLDER 0
#define ATTR_TYPE_FILE 1
typedef struct attr_ts{
	int type;
	folder_t folder;
	file_t file;
}attr_t;


typedef struct dir_ts{
	int size;
	char **array;
}dir_t;

