package main

import (
	"log"
	"os"
	"fmt"
	"time"
	"io"
	"io/fs"
	"bufio"
	"math/rand"
	"hash"
	"crypto/sha256"
	"path/filepath"
)

func aux_Chmod(name string, mode os.FileMode) bool {
	performAutoSync()
	return assertNoError(os.Chmod(name, mode))
}

func aux_Chown(name string, uid, gid int) bool {
	performAutoSync()
	return assertNoError(os.Chown(name, uid, gid))
}

func aux_Chtimes(name string, atime time.Time, mtime time.Time) bool {
	performAutoSync()
	return assertNoError(os.Chtimes(name, atime, mtime))
}

func aux_Mkdir(name string, perm os.FileMode) bool {
	performAutoSync()
	return assertNoError(os.Mkdir(name, perm))
}

func aux_MkdirTemp(dir, pattern string) string {
	performAutoSync()
	out, err:= os.MkdirTemp(dir, pattern)
	if !assertNoError(err) {
		return ""
	}
	return out
}

func aux_Remove(name string) bool {
	performAutoSync()
	return assertNoError(os.Remove(name))
}

func aux_RemoveAll(name string) bool {
	performAutoSync()
	return assertNoError(os.RemoveAll(name))
}

func aux_Rename(oldpath, newpath string) bool {
	performAutoSync()
	return assertNoError(os.Rename(oldpath, newpath))
}

func aux_ReadFile(name string) []byte {
	performAutoSync()
	out, err:= os.ReadFile(name)
	if !assertNoError(err) {
		return []byte{}
	}
	return out
}

func aux_WriteFile(name string, data []byte, perm os.FileMode) bool {
	performAutoSync()
	return assertNoError(os.WriteFile(name, data, perm))
}

func aux_Truncate(name string, size int64) bool {
	performAutoSync()
	return assertNoError(os.Truncate(name, size))
}

func aux_ReadDir(name string) []os.DirEntry {
	performAutoSync()
	out, err := os.ReadDir(name)
	if !assertNoError(err) {
		return []os.DirEntry{}
	}
	return out
}

func aux_Symlink(oldname, newname string) bool {
	performAutoSync()
	return assertNoError(os.Symlink(oldname, newname))
}

func aux_Readlink(name string) string {
	performAutoSync()
	out, err:= os.Readlink(name)
	if !assertNoError(err) {
		return ""
	}
	return out
}

func aux_MkdirAll(path string, perm os.FileMode) bool {
	performAutoSync()
	return assertNoError(os.MkdirAll(path, perm))
}

func aux_MkdirsAll(pathes []string, perm os.FileMode) bool {
	performAutoSync()
	out:= true
	for _, path := range pathes {
		if !aux_MkdirAll(path, perm) {
			out = false
		}
	}
	return out
}


type LimitedRandomReader struct {
    remain uint64
}

func (r *LimitedRandomReader) Read(p []byte) (n int, err error) {
    if r.remain == 0 {
        return 0, io.EOF
    }
	piece := len(p)
	if piece == 0 {
		return 0, nil
	}
	if uint64(piece) > r.remain {
		piece = int(r.remain)
	}
	piece, err = rand.Read(p[0 : piece])
	if err == nil {
		if uint64(piece) < r.remain {
			r.remain -= uint64(piece)
		} else {
			r.remain = 0
		}
	}
    return piece, err
}

func aux_Mkfile(path string, mode os.FileMode, min_size uint64, max_size uint64) bool {
	performAutoSync()
	f, err := os.OpenFile(path, os.O_WRONLY | os.O_CREATE | os.O_TRUNC, mode)
	if err != nil {
		setErrorString(fmt.Sprintf("Error %v creating %v", err, path))
		return false
	}
	defer f.Close()

	lrr := LimitedRandomReader {
		remain : min_size,
	}
	if max_size > min_size {
		lrr.remain+= rand.Uint64() % (max_size - min_size)
	}

	_, err = io.Copy(f, &lrr)
	if err != nil {
		setErrorString(fmt.Sprintf("Error %v writing %v", err, path))
		return false
	}

	return true
}

func aux_Mkfiles(pathes []string, mode os.FileMode, min_size uint64, max_size uint64) bool {
	out:= true
	for _, path := range pathes {
		if !aux_Mkfile(path, mode, min_size, max_size) {
			out = false
		}
	}
	return out
}

var g_hash_data, g_hash_name, g_hash_mode, g_hash_link, g_hash_times bool
var g_hash_hide_path string
var g_hash hash.Hash

func EnhashString(s string) {
	g_hash.Write([]byte(s))
}

func EnhashFileData(path string) {
	f, err := os.Open(path)
	if err != nil {
		EnhashString("ERR:" + err.Error())
		log.Printf("EnhashFileData:" + err.Error())
		return
	}
	defer f.Close()
	if _, err := io.Copy(g_hash, f); err != nil {
		EnhashString("ERR:" + err.Error())
		log.Printf("EnhashFileData:" + err.Error())
	}
}

func EnhashFSObject(path string) bool {
	var fi fs.FileInfo
	var err error
	if g_hash_link {
		fi, err = os.Lstat(path)
	} else {
		fi, err = os.Stat(path)
	}
	if g_hash_name && path != g_hash_hide_path {
		EnhashString(path)
	}
	if err != nil {
		log.Printf("Stat error %s while enhashing %s", err.Error(), path)
		return false
	}
	if g_hash_mode {
		EnhashString(fi.Mode().String())
	}
	if g_hash_times {
		// one second precision to avoid rounding errors
		EnhashString( fi.ModTime().Format(time.Stamp))
	}
	if g_hash_data {
		if (fi.Mode() & fs.ModeSymlink) != 0 {
			if dst, err := os.Readlink(path); err == nil {
				EnhashString(dst)
			} else {
				EnhashString(err.Error())
			}
			return false
		}
		if fi.Mode().IsRegular() {
			EnhashFileData(path)
			return false
		}
	}
	return fi.IsDir()
}

func walkHash(path string, de fs.DirEntry, err error) error {
	return nil
}

func aux_CheckFilesDataSame(path1, path2 string) bool {
	hash1:= aux_HashPath(path1, true, false, false, false, false)
	hash2:= aux_HashPath(path2, true, false, false, false, false)
	if (hash1 != hash2) {
		setErrorString("Files data differs: " + path1 + ":" + hash1 + " vs " + path2 + ":" + hash2)
		return false
	}
	log.Println("Files are same: " + path1 + " vs " + path2 + " " + hash2)
	return true
}

func aux_HashPath(path string, hash_data bool, hash_name bool, hash_link bool, hash_mode bool, hash_times bool) string {
	return aux_HashPathes([]string{path}, hash_data, hash_name, hash_link, hash_mode, hash_times)
}

func aux_HashPathes(pathes []string, hash_data bool, hash_name bool, hash_link bool, hash_mode bool, hash_times bool) string {
	performAutoSync()
	g_hash_data = hash_data
	g_hash_name = hash_name
	g_hash_link = hash_link
	g_hash_mode = hash_mode
	g_hash_times = hash_times
	g_hash = sha256.New()
	for _, path := range pathes {
		g_hash_hide_path = path
		if EnhashFSObject(path) {
			filepath.WalkDir(path, walkHash)
		}
	}
	return fmt.Sprintf("%x", g_hash.Sum(nil))
}

func aux_Exists(path string) bool {
	performAutoSync()
	_, err := os.Lstat(path)
	return err == nil
}

func aux_CountExisting(pathes []string) int {
	performAutoSync()
	out:= 0
	for _, path := range pathes {
		if aux_Exists(path) {
			out++
		}
	}
	return out
}


func aux_SaveTextFile(fpath string, lines []string) {
	performAutoSync()
	file, err := os.Create(fpath)
	if err != nil {
		setErrorString(err.Error())
		return 
	}
    defer file.Close()

	for _, line := range lines {
		_, err := file.WriteString(line + "\n")
		if err != nil {
			log.Fatal(err)
		}
	}
}



func aux_LoadTextFile(fpath string) []string {
    var lines []string
	file, err := os.Open(fpath)
	if err != nil {
		setErrorString(err.Error())
		return lines
	}
    defer file.Close()

    scanner := bufio.NewScanner(file)
    for scanner.Scan() {
        lines = append(lines, scanner.Text())
    }
    return lines
}
