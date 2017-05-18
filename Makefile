OBJS = src/fuse_main.c src/mvfs_common.c src/mvfs_connection.c src/mvfs_mongo.c

CINCLUDES = -I ./include

TARGET = mvfs

mVFS:$(OBJS)
	gcc $^ -o  $@ $(CINCLUDES) -lm `pkg-config --cflags --libs libmongoc-1.0` `pkg-config fuse3 --cflags --libs`
clean: 
	rm mvfs
