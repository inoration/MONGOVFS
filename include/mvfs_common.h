/*
	Copyright (C) 2016 inorO <ginoration@gmail.com>
*/

int mvfs_extraInit(const char *server_port, const char *client_name, const char *db_name, const char *tok_str, const char *log_file);


void *mvfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg);


void mvfs_destroy(void *p);


int mvfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);


int mvfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags);


int mvfs_mknod(const char *path, mode_t mode, dev_t dev);


int mvfs_mkdir(const char *path, mode_t mode);


int mvfs_rmdir(const char *path);


int mvfs_unlink(const char *path);