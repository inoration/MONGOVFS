/*
	Copyright (C) 2016 inorO <ginoration@gmail.com>
*/

/*
	build with 	FUSElib <https://github.com/libfuse/libfuse>
				mongolib <http://mongoc.org/>
*/

#define FUSE_USE_VERSION 30


#include <fuse.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>

#include "mvfs_common.h"


static struct options_struct{
	int show_help; //1-show help 0-not
	char *server_port;
	char *client_name;
	char *db_name;
	char *tok_str;
	char *log_file;
}options;


#define MVFS_OPT(t, p) {t, offsetof(struct options_struct, p), 1}
static const struct fuse_opt  Opts[] = {
	MVFS_OPT("--name=%s", db_name),
	MVFS_OPT("--tokstr=%s", tok_str),
	MVFS_OPT("--logfile=%s", log_file),
	MVFS_OPT("--server=%s", server_port),
	MVFS_OPT("--client=%s", client_name),

	FUSE_OPT_END
};


static struct fuse_operations mfsOpt = {
	.init 		= mvfs_init,
	.destroy 	= mvfs_destroy,
	.getattr 	= mvfs_getattr,
	.readdir 	= mvfs_readdir,
	.mknod	 	= mvfs_mknod,
	.mkdir 		= mvfs_mkdir,
	.unlink 	= mvfs_unlink,
	.rmdir 		= mvfs_rmdir

/* You may add operation here. like:
	//.open 		= mvfs_open
	//.rename		= mvfs_rename
*/

};


int optionInit(struct fuse_args *args){

	options.show_help = 0;
	options.server_port = strdup("mongodb://localhost:27017");
	options.client_name = strdup("MVFSC");
	options.db_name = strdup("MVFS_TEST");
	options.tok_str = strdup(" /");
	options.log_file = strdup("/home/win/mvfslog/log");
	//options.log_file = strdup("/mvfs/mvfs_log/");

	return fuse_opt_parse(args, &options, Opts, NULL);
}


int showHelp(const char *name){
	printf("usage: %s [options] <mountpoint>\n\n", name);
	printf("File-system specific options:\n"

	       "    --server=<s>        Port of Server\n"
	       "                        (default: \"mongodb://localhost:27017\")\n"

	       "    --client=<s>        Name of client\n"
	       "                        (default: \"MVFSC\")\n"

	       "    --name=<s>          Name of the mountpoint\n"
	       "                        (default: \"MVFS_TEST\")\n"

	       "    --tokstr=<s>        String for spliting path string\n"
	       "                        (default: \" /\")\n"

	       "    --logfile=<s>       File path to save the log\n"
	       "                        (default: \"/mvfs/mvfs_log\")\n"

	       "    --help              Show help\n"
	       "                        (default: flase)\n"

	       "\n");
}


int main(int argc, char *argv[]){

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	assert(optionInit(&args) != -1);

	if (options.show_help){
		showHelp(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0] = (char*) "";
	}

	mvfs_extraInit(options.server_port, options.client_name, options.db_name, options.tok_str, options.log_file);

	return fuse_main(args.argc, args.argv, &mfsOpt, NULL);
}