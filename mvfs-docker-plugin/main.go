package main

import (
	"flag"
	"fmt"
	//"os"

	"github.com/docker/go-plugins-helpers/volume"
)


var (
	basePath =  flag.String("basepath", "/home/win", "Basic dir.")
	name = flag.String("mp", "MVFS_TEST", "Name of mountpoint.")
)

func main() {
	flag.Parse()

	d := newMvfsDriver(*basePath)
	h := volume.NewHandler(d)
	fmt.Println(h.ServeUnix("mvfs", -1))
}
