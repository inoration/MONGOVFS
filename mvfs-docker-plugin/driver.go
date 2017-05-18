package main


import (
	"fmt"
	"os"
	"path"
	"os/exec"
	"github.com/docker/go-plugins-helpers/volume"
)


type mvfsDriver struct {
	path string
}


func pathNotExist(path string) bool {
	if _, err := os.Stat(path); err != nil {
		return os.IsNotExist(err)
	}
	return false
}


func createOrExist(path string) mvfsDriver {
	if pathNotExist(path) {
		os.MkdirAll(path, 0777)
	}
	exec.Command("mVFS", path).CombinedOutput()
	fmt.Println(path)
	d := mvfsDriver{path}
	return d
}


func newMvfsDriver(basePath string) mvfsDriver{
	p := path.Join(basePath, "MVFS_TEST")
	d := createOrExist(p)
	return d
}


func (d mvfsDriver) Create(r volume.Request) volume.Response {
	return volume.Response{}
}


func (d mvfsDriver) Remove(r volume.Request) volume.Response {
	return volume.Response{}
}



func (d mvfsDriver) Path(r volume.Request) volume.Response {
	return volume.Response{Mountpoint: d.path}
}


func (d mvfsDriver) Mount(r volume.MountRequest) volume.Response {
	return volume.Response{Mountpoint: d.path}
}


func (d mvfsDriver) Unmount(r volume.UnmountRequest) volume.Response {
	return volume.Response{}
}


func (d mvfsDriver) Get(r volume.Request) volume.Response {
	return volume.Response{} 
}



func (d mvfsDriver) List(r volume.Request) volume.Response {
	return volume.Response{} 
}


func (d mvfsDriver) Capabilities(r volume.Request) volume.Response {
	var res volume.Response
	res.Capabilities = volume.Capability{Scope: "local"}
	return res
}
