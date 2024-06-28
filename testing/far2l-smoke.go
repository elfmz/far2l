package main

import (
	"log"
	"os"
	"net"
	"fmt"
	"time"
	"strings"
	"strconv"
	"os/exec"
	"bufio"
	"io"
	"io/ioutil"
	"io/fs"
	"math/rand"
	"hash"
	"crypto/sha256"
	"encoding/binary"
	"path/filepath"
    "github.com/ActiveState/termtest"
    "github.com/ActiveState/termtest/expect"
    "github.com/dop251/goja"
)

type far2l_Status struct {
	Title string
	Width uint32
	Height uint32
	CurX uint32
	CurY uint32
	CurH uint8
	CurV bool
}

type far2l_FoundString struct {
	I uint32
	X uint32
	Y uint32
}

type far2l_CellRaw struct {
	Text string
	Attributes uint64
}

type far2l_Cell struct {
	Text string
	BackTC uint32
	ForeTC uint32
	Back uint8
	Fore uint8
	IsBackTC bool
	IsForeTC bool
	ForeBlue bool
	ForeGreen bool
	ForeRed bool
	ForeIntense bool
	BackBlue bool
	BackGreen bool
	BackRed bool
	BackIntense bool
	ReverseVideo bool
	Underscore bool
	Strikeout bool
}

var g_far2l_sock string
var g_far2l_bin string
var g_socket *net.UnixConn
var g_addr *net.UnixAddr
var g_buf [4096]byte
var g_app *termtest.ConsoleProcess
var g_vm *goja.Runtime
var g_status far2l_Status
var g_far2l_running bool = false
var g_lctrl bool
var g_rctrl bool
var g_lalt bool
var g_ralt bool
var g_shift bool
var g_recv_timeout uint32 = 30
var g_test_workdir string
var g_calm bool = false
var g_last_error string

func stringFromBytes(buf []byte) string {
	last := 0
	for ; last < len(buf) && buf[last] != 0; last++ {
	}
	return string(buf[0:last])
}

func aux_BePanic() {
	g_calm = false
}

func aux_BeCalm() {
	g_calm = true
}

func aux_Inspect() string {
	out:= g_last_error
	g_last_error = ""
	return out
}

func setErrorString(err string) {
	g_last_error = err
	log.Print("\x1b[1;31mERROR: " + err + "\x1b[39;22m")
	if !g_calm {
		aux_Panic(err)
	}
}

func assertNoError(err error) bool {
	if err != nil {
		setErrorString(err.Error())
		return false
	}
	return true
}

func far2l_ReadSocket(expected_n int, extra_timeout uint32) {
	err := g_socket.SetReadDeadline(time.Now().Add(time.Duration(g_recv_timeout + extra_timeout) * time.Second))
    if err != nil {
        aux_Panic(err.Error())
	}
	n, addr, err := g_socket.ReadFromUnix(g_buf[:])
    if err != nil || n != expected_n {
		if g_addr == nil {
			if net_err, ok := err.(net.Error); ok && net_err.Timeout() {
				aux_Panic("First communication timed out, make sure application built with testing support or increase timeout by -t argument")
			}
		}
        aux_Panic(err.Error())
    }
	if g_addr == nil || *g_addr != *addr {
		g_addr = addr
		log.Printf("Peer: %v", g_addr)
	}
}

func far2l_Close() {
	if g_far2l_running {
		g_far2l_running = false
		g_app.Close()
	}
}

func far2l_StartWithSize(args []string, cols int, rows int) far2l_Status {
	if g_far2l_running {
		aux_Panic("far2l already running")
	}
    opts := termtest.Options {
        CmdName: g_far2l_bin,
		Args: append([]string{g_far2l_bin, "--test=" + g_far2l_sock}, args...),
		Environment : []string {
			"FAR2L_STD=" + filepath.Join(g_test_workdir, "far2l.log"),
			"FAR2L_TESTCTL=" + g_far2l_sock},
		ExtraOpts: []expect.ConsoleOpt{expect.WithTermRows(rows), expect.WithTermCols(cols)},
    }
	var err error
    g_app, err = termtest.New(opts)
	if err != nil {
		aux_Panic(err.Error())
	}
	g_far2l_running = true
	return far2l_RecvStatus()
}

func far2l_Start(args []string) far2l_Status {
	return far2l_StartWithSize(args, 120, 80)
}

func far2l_ReqRecvStatus() far2l_Status {
	binary.LittleEndian.PutUint32(g_buf[0:], 1)
	n, err := g_socket.WriteTo(g_buf[0:4], g_addr)
	if err != nil || n != 4 {
		aux_Panic(err.Error())
	}
	return far2l_RecvStatus()
}

func far2l_RecvStatus() far2l_Status {
	far2l_ReadSocket(2068, 0)
	g_status.Title = stringFromBytes(g_buf[20:])
	g_status.CurH = g_buf[2]
	g_status.CurV = g_buf[3] != 0
	g_status.CurX = binary.LittleEndian.Uint32(g_buf[4:])
	g_status.CurY = binary.LittleEndian.Uint32(g_buf[8:])
	g_status.Width = binary.LittleEndian.Uint32(g_buf[12:])
	g_status.Height = binary.LittleEndian.Uint32(g_buf[16:])
	return g_status
}


func far2l_ReqRecvSync(tmout uint32) bool {
	binary.LittleEndian.PutUint32(g_buf[0:], 6) // TEST_CMD_SYNC
	binary.LittleEndian.PutUint32(g_buf[4:], tmout)
	n, err := g_socket.WriteTo(g_buf[0:8], g_addr)
	if err != nil || n != 8 {
		aux_Panic(err.Error())
	}
	far2l_ReadSocket(1, tmout)
	if g_buf[0] == 0 {
		setErrorString("Sync timout")
		return false
	}
	return true
}


func far2l_ReqRecvExpectString(str string, x uint32, y uint32, w uint32, h uint32, tmout uint32) far2l_FoundString {
	return far2l_ReqRecvExpectXStrings([]string{str}, x, y, w, h, tmout, true)
}

func far2l_ReqRecvExpectStrings(str_vec []string, x uint32, y uint32, w uint32, h uint32, tmout uint32) far2l_FoundString {
	return far2l_ReqRecvExpectXStrings(str_vec, x, y, w, h, tmout, true)
}

func far2l_ReqRecvExpectNoString(str string, x uint32, y uint32, w uint32, h uint32, tmout uint32) far2l_FoundString {
	return far2l_ReqRecvExpectXStrings([]string{str}, x, y, w, h, tmout, false)
}

func far2l_ReqRecvExpectNoStrings(str_vec []string, x uint32, y uint32, w uint32, h uint32, tmout uint32) far2l_FoundString {
	return far2l_ReqRecvExpectXStrings(str_vec, x, y, w, h, tmout, false)
}

func far2l_ReqRecvExpectXStrings(str_vec []string, x uint32, y uint32, w uint32, h uint32, tmout uint32, need_presence bool) far2l_FoundString {
	if w == 0xffffffff { w = g_status.Width; }
	if h == 0xffffffff { h = g_status.Height; }
	if (need_presence) {
		binary.LittleEndian.PutUint32(g_buf[0:], 3) //TEST_CMD_WAIT_STRING
	} else {
		binary.LittleEndian.PutUint32(g_buf[0:], 4) //TEST_CMD_WAIT_NO_STRING
	}
	binary.LittleEndian.PutUint32(g_buf[4:], tmout)
	binary.LittleEndian.PutUint32(g_buf[8:], x) //left
	binary.LittleEndian.PutUint32(g_buf[12:], y) //top
	binary.LittleEndian.PutUint32(g_buf[16:], w) //width
	binary.LittleEndian.PutUint32(g_buf[20:], h) //height
	p := 0
	for i := 0; i < len(str_vec); i++ {
		str_bytes:= []byte(str_vec[i])
		for j := 0; j < len(str_bytes); j++ {
			g_buf[24 + p] = str_bytes[j]
			p++
		}
		g_buf[24 + p] = 0
		p++
	}
	if p >= 2048 {
		aux_Panic("Too long strings")
	}
	for ; p < 2048; p++ {
		g_buf[24 + p] = 0
	}

	n, err := g_socket.WriteTo(g_buf[0:24 + 2048], g_addr)
	if err != nil || n != 24 + 2048 {
		aux_Panic(err.Error())
	}
	far2l_ReadSocket(12, tmout / 1000)
	out := far2l_FoundString {
		I: binary.LittleEndian.Uint32(g_buf[0:]),
		X: binary.LittleEndian.Uint32(g_buf[4:]),
		Y: binary.LittleEndian.Uint32(g_buf[8:]),
	}
	var status string
	if out.I < uint32(len(str_vec)) {
		status = fmt.Sprintf("String at [%d : %d] - %v", out.X, out.Y, str_vec[out.I])
	} else {
		status = fmt.Sprintf("Nothing at [%d +%d : %d +%d] of %v", x, w, y, h, str_vec)
	}
	if (need_presence) {
		if out.I < uint32(len(str_vec)) {
			fmt.Println(status)
		} else {
			setErrorString(status)
		}
	} else if out.I < uint32(len(str_vec)) {
		setErrorString(status)
	} else {
		fmt.Println(status)
	}
	return out
}

func far2l_ReqRecvReadCellRaw(x uint32, y uint32) far2l_CellRaw {
	binary.LittleEndian.PutUint32(g_buf[0:], 2) // TEST_CMD_READ_CELL
	binary.LittleEndian.PutUint32(g_buf[4:], x) // left
	binary.LittleEndian.PutUint32(g_buf[8:], y) // top
	n, err := g_socket.WriteTo(g_buf[0:12], g_addr)
	if err != nil || n != 12 {
		aux_Panic(err.Error())
	}
	far2l_ReadSocket(2056, 0)
	return far2l_CellRaw {
		Text: stringFromBytes(g_buf[8:]),
		Attributes: binary.LittleEndian.Uint64(g_buf[0:]),
	}
}

func far2l_ReqRecvReadCell(x uint32, y uint32) far2l_Cell {
	raw_cell := far2l_ReqRecvReadCellRaw(x, y)
	return far2l_Cell {
		Text:         raw_cell.Text,
		BackTC:       uint32((raw_cell.Attributes >> 40) & 0xFFFFFF),
		ForeTC:       uint32((raw_cell.Attributes >> 16) & 0xFFFFFF),
		Back:         uint8(((raw_cell.Attributes >> 4) & 0xF)),
		Fore:         uint8((raw_cell.Attributes & 0xF)),
		IsBackTC:     (raw_cell.Attributes & 0x0200) != 0,
		IsForeTC:     (raw_cell.Attributes & 0x0100) != 0,
		ForeBlue:     (raw_cell.Attributes & 0x0001) != 0,
		ForeGreen:    (raw_cell.Attributes & 0x0002) != 0,
		ForeRed:      (raw_cell.Attributes & 0x0004) != 0,
		ForeIntense:  (raw_cell.Attributes & 0x0008) != 0,
		BackBlue:     (raw_cell.Attributes & 0x0010) != 0,
		BackGreen:    (raw_cell.Attributes & 0x0020) != 0,
		BackRed:      (raw_cell.Attributes & 0x0040) != 0,
		BackIntense:  (raw_cell.Attributes & 0x0080) != 0,
		ReverseVideo: (raw_cell.Attributes & 0x4000) != 0,
		Underscore:   (raw_cell.Attributes & 0x8000) != 0,
		Strikeout:    (raw_cell.Attributes & 0x2000) != 0,
	}
}

func far2l_CheckBoundedLine(expected string, left uint32, top uint32, width uint32, trim_chars string) string {
	line:= far2l_BoundedLine(left, top, width, trim_chars)
	if line != expected {
		setErrorString(fmt.Sprintf("Line at [%d +%d : %d] not expected: '%v'", left, width, top, line))
	}
	return line
}

func far2l_BoundedLine(left uint32, top uint32, width uint32, trim_chars string) string {
	lines:= far2l_BoundedLines(left, top, width, 1, trim_chars)
	return lines[0]
}

func far2l_BoundedLines(left uint32, top uint32, width uint32, height uint32, trim_chars string) []string {
	lines:= []string{}
	if width == 0xffffffff {
		width = g_status.Width
	}
	if height == 0xffffffff {
		height = g_status.Height
	}
	for y := top; y < top + height; y++ {
		line:= ""
		for x := left; x < left + width; x++ {
			cell := far2l_ReqRecvReadCellRaw(x, y)
			line += cell.Text
		}
		if trim_chars != "" {
			lines = append(lines, strings.Trim(line, trim_chars))
		} else {
			lines = append(lines, line)
		}
	}
	return lines
}

func far2l_SurroundedLines(x uint32, y uint32, boundary_chars string, trim_chars string) []string {
	var left, top, width, height uint32
	far2l_ReqRecvStatus()

	for left = x; left > 0 && !far2l_CellCharMatches(left - 1, y, boundary_chars); left-- {
	}

	for width = 1; left + width < g_status.Width && !far2l_CellCharMatches(left + width, y, boundary_chars); width++ {
	}

	// top & bottom edges has some quirks due to they may contains caption, hints, time etc..
	for top = y; top > 0 &&
		!far2l_CellCharMatches(left, top - 1, boundary_chars) &&
		!far2l_CellCharMatches(left + width / 2, top - 1, boundary_chars) &&
		!far2l_CellCharMatches(left + width - 1, top - 1, boundary_chars); top-- {
	}

	for height = 1; top + height < g_status.Height &&
		!far2l_CellCharMatches(left, top + height, boundary_chars) &&
		!far2l_CellCharMatches(left + width / 2, top + height, boundary_chars) &&
		!far2l_CellCharMatches(left + width - 1, top + height, boundary_chars); height++ {
	}

	return far2l_BoundedLines(left, top, width, height, trim_chars)
}

func far2l_CellCharMatches(x uint32, y uint32, chars string) bool {
	cell := far2l_ReqRecvReadCellRaw(x, y)
	return cell.Text != "" && strings.Contains(chars, cell.Text)
}

func far2l_CheckCellChar(x uint32, y uint32, chars string) string {
	cell := far2l_ReqRecvReadCellRaw(x, y)
	if cell.Text == "" || !strings.Contains(chars, cell.Text) {
		setErrorString(fmt.Sprintf("Cell at %d:%d = '%s' doesnt represent any of: '%s'", x, y, cell.Text, chars))
	}
	return cell.Text
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

func aux_SaveTextFile(fpath string, lines []string) {
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

func far2l_BoundedLinesMatchTextFile(left uint32, top uint32, width uint32, height uint32, fpath string) bool {
	screen_lines:= far2l_BoundedLines(left, top, width, height, "")
	file_lines:= aux_LoadTextFile(fpath)
	for i:= 0; i != len(screen_lines) && i != len(file_lines); i++ {
		if screen_lines[i] != file_lines[i] {
			fpath_actual:= fpath + ".actual"
			aux_SaveTextFile(fpath_actual, screen_lines)
			setErrorString(fmt.Sprintf("Line %d is wrong: '%s'\nActual lines saved into: %s", i, screen_lines[i], fpath_actual))
			return false
		}
	}
	if len(screen_lines) != len(file_lines) {
		fpath_actual:= fpath + ".actual"
		aux_SaveTextFile(fpath_actual, screen_lines)
		setErrorString(fmt.Sprintf("Wrong lines count: %d != %d\nActual lines saved into: %s", len(screen_lines), len(file_lines), fpath_actual))
		return false
	}
	return true
}

func far2l_BoundedLinesSaveAsTextFile(left uint32, top uint32, width uint32, height uint32, fpath string) {
	screen_lines:= far2l_BoundedLines(left, top, width, height, "")
	aux_SaveTextFile(fpath, screen_lines)
}

func far2l_ReqBye() {
	binary.LittleEndian.PutUint32(g_buf[0:], 0)
	n, err := g_socket.WriteTo(g_buf[0:4], g_addr)
	if err != nil || n != 4 {
		aux_Panic(err.Error())
	}
}

func far2l_ExpectExit(code int, timeout_ms int) string {
	far2l_ReqBye()
    _, err:= g_app.ExpectExitCode(code, time.Duration(timeout_ms) * 1000000)
	if err != nil {
		setErrorString(fmt.Sprintf("ExpectExit: %v", err))
		return "ERROR:" + err.Error()
	}
	far2l_Close()
	return ""
}

func aux_Log(message string) {
	log.Print(message)
}

func aux_Panic(message string) {
	panic("\x1b[1;31m" + message + "\x1b[39;22m")
}

func tty_Write(s string) {
    g_app.Send(s)
}

func tty_CtrlC() {
    g_app.SendCtrlC()
}

func far2l_ToggleShift(pressed bool) {
	g_shift = pressed
	far2l_SendKeyEvent(0, 0x10, pressed)
}

func far2l_ToggleLCtrl(pressed bool) {
	g_lctrl = pressed
	far2l_SendKeyEvent(0, 0x11, pressed)
}

func far2l_ToggleRCtrl(pressed bool) {
	g_rctrl = pressed
	far2l_SendKeyEvent(0, 0x11, pressed)
}

func far2l_ToggleLAlt(pressed bool) {
	g_lalt = pressed
	far2l_SendKeyEvent(0, 0x12, pressed)
}

func far2l_ToggleRAlt(pressed bool) {
	g_ralt = pressed
	far2l_SendKeyEvent(0, 0x12, pressed)
}

func far2l_TypeFKey(n uint32) { far2l_TypeVK(0x6F + n) }
func far2l_TypeDigit(n uint32) { far2l_TypeVK(0x60 + n) }

func far2l_TypeAdd()      { far2l_TypeVK(0x6B) }
func far2l_TypeSub()      { far2l_TypeVK(0x6D) }
func far2l_TypeMul()      { far2l_TypeVK(0x6A) }
func far2l_TypeDiv()      { far2l_TypeVK(0x6F) }
func far2l_TypeSeparator(){ far2l_TypeVK(0x6C) }
func far2l_TypeDecimal()  { far2l_TypeVK(0x6E) }

func far2l_TypeBack()     { far2l_TypeVK(0x08) }
func far2l_TypeEnter()    { far2l_TypeVK(0x0D) }
func far2l_TypeEscape()   { far2l_TypeVK(0x1B) }
func far2l_TypePageUp()   { far2l_TypeVK(0x21) }
func far2l_TypePageDown() { far2l_TypeVK(0x22) }
func far2l_TypeEnd()      { far2l_TypeVK(0x23) }
func far2l_TypeHome()     { far2l_TypeVK(0x24) }
func far2l_TypeLeft()     { far2l_TypeVK(0x25) }
func far2l_TypeUp()       { far2l_TypeVK(0x26) }
func far2l_TypeRight()    { far2l_TypeVK(0x27) }
func far2l_TypeDown()     { far2l_TypeVK(0x28) }
func far2l_TypeIns()      { far2l_TypeVK(0x2D) }
func far2l_TypeDel()      { far2l_TypeVK(0x2E) }


func far2l_TypeVK(key_code uint32) {
	far2l_SendKeyEvent(0, key_code, true)
	far2l_SendKeyEvent(0, key_code, false)
}


func far2l_TypeText(text string) {
    for _, r := range text {
		far2l_SendKeyEvent(uint32(r), 0, true)
		far2l_SendKeyEvent(uint32(r), 0, false)
    }
}

func far2l_SendKeyEvent(utf32_code uint32, key_code uint32, pressed bool) {
	if key_code == 0 && utf32_code != 0 {
		if utf32_code >= 'a' && utf32_code <= 'z' {
			key_code = 'A' + (utf32_code - 'a')
		} else if (utf32_code <= 0x7f) {
			key_code = utf32_code
		}
	}
	var controls uint32 = 0
	if g_lctrl { controls |= 0x0008 } // LEFT_CTRL_PRESSED
	if g_rctrl { controls |= 0x0004 } // RIGHT_CTRL_PRESSED
	if g_lalt  { controls |= 0x0002 } // LEFT_ALT_PRESSED
	if g_ralt  { controls |= 0x0001 } // RIGHT_ALT_PRESSED
	if g_shift { controls |= 0x0010 } // SHFIT_PRESSED
	binary.LittleEndian.PutUint32(g_buf[0:], 5) // TEST_CMD_SEND_KEY
	binary.LittleEndian.PutUint32(g_buf[4:], controls)
	binary.LittleEndian.PutUint32(g_buf[8:], utf32_code)
	binary.LittleEndian.PutUint32(g_buf[12:], key_code)
	binary.LittleEndian.PutUint32(g_buf[16:], 0)
	binary.LittleEndian.PutUint32(g_buf[20:], 0)
	if pressed { g_buf[20] = 1 }
	n, err := g_socket.WriteTo(g_buf[0:24], g_addr)
	if err != nil || n != 24 {
		aux_Panic(err.Error())
	}
}

func aux_RunCmd(args []string) string {
	prog, err := exec.LookPath(args[0])
	if err != nil {
		return err.Error()
	}
	cmd := exec.Command(prog, args...)
	err = cmd.Run()
	if (err != nil) {
		setErrorString("RunCmd: " + err.Error())
	}
	return ""
}

func aux_Sleep(msec uint32) {
	time.Sleep(time.Duration(msec) * time.Millisecond)
}

func aux_WorkDir() string {
	return g_test_workdir
}

func aux_Chmod(name string, mode os.FileMode) bool {
	return assertNoError(os.Chmod(name, mode))
}

func aux_Chown(name string, uid, gid int) bool {
	return assertNoError(os.Chown(name, uid, gid))
}

func aux_Chtimes(name string, atime time.Time, mtime time.Time) bool {
	return assertNoError(os.Chtimes(name, atime, mtime))
}

func aux_Mkdir(name string, perm os.FileMode) bool {
	return assertNoError(os.Mkdir(name, perm))
}

func aux_MkdirTemp(dir, pattern string) string {
	out, err:= os.MkdirTemp(dir, pattern)
	if !assertNoError(err) {
		return ""
	}
	return out
}

func aux_Remove(name string) bool {
	return assertNoError(os.Remove(name))
}

func aux_RemoveAll(name string) bool {
	return assertNoError(os.RemoveAll(name))
}

func aux_Rename(oldpath, newpath string) bool {
	return assertNoError(os.Rename(oldpath, newpath))
}

func aux_ReadFile(name string) []byte {
	out, err:= os.ReadFile(name)
	if !assertNoError(err) {
		return []byte{}
	}
	return out
}

func aux_WriteFile(name string, data []byte, perm os.FileMode) bool {
	return assertNoError(os.WriteFile(name, data, perm))
}

func aux_Truncate(name string, size int64) bool {
	return assertNoError(os.Truncate(name, size))
}

func aux_ReadDir(name string) []os.DirEntry {
	out, err := os.ReadDir(name)
	if !assertNoError(err) {
		return []os.DirEntry{}
	}
	return out
}

func aux_Symlink(oldname, newname string) bool {
	return assertNoError(os.Symlink(oldname, newname))
}

func aux_Readlink(name string) string {
	out, err:= os.Readlink(name)
	if !assertNoError(err) {
		return ""
	}
	return out
}

func aux_MkdirAll(path string, perm os.FileMode) bool {
	return assertNoError(os.MkdirAll(path, perm))
}

func aux_MkdirsAll(pathes []string, perm os.FileMode) bool {
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

func aux_HashPath(path string, hash_data bool, hash_name bool, hash_link bool, hash_mode bool, hash_times bool) string {
	return aux_HashPathes([]string{path}, hash_data, hash_name, hash_link, hash_mode, hash_times)
}

func aux_HashPathes(pathes []string, hash_data bool, hash_name bool, hash_link bool, hash_mode bool, hash_times bool) string {
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
	_, err := os.Lstat(path)
	return err == nil
}

func aux_CountExisting(pathes []string) int {
	out:= 0
	for _, path := range pathes {
		if aux_Exists(path) {
			out++
		}
	}
	return out
}


func initVM() {
	/* initialize */
	fmt.Println("Initializing JS VM...")
	g_vm = goja.New()

	/* goja does not expose a standard "global" by default */
	_, err:= g_vm.RunString("var global = (function(){ return this; }).call(null);")
	if err != nil { aux_Panic(err.Error()) }

	setVMFunction("BePanic", aux_BePanic)
	setVMFunction("BeCalm", aux_BeCalm)
	setVMFunction("Inspect", aux_Inspect)

	setVMFunction("StartApp", far2l_Start)
	setVMFunction("StartAppWithSize", far2l_StartWithSize)

	setVMFunction("AppStatus", far2l_ReqRecvStatus)
	setVMFunction("Sync", far2l_ReqRecvSync)

	setVMFunction("ReadCellRaw", far2l_ReqRecvReadCellRaw)
	setVMFunction("ReadCell", far2l_ReqRecvReadCell)
	setVMFunction("CellCharMatches", far2l_CellCharMatches)
	setVMFunction("CheckCellChar", far2l_CheckCellChar)
	
	setVMFunction("BoundedLines", far2l_BoundedLines)
	setVMFunction("BoundedLine", far2l_BoundedLine)
	setVMFunction("CheckBoundedLine", far2l_CheckBoundedLine)
	setVMFunction("SurroundedLines", far2l_SurroundedLines)

	setVMFunction("ExpectStrings", far2l_ReqRecvExpectStrings)
	setVMFunction("ExpectString", far2l_ReqRecvExpectString)
	setVMFunction("ExpectNoStrings", far2l_ReqRecvExpectNoStrings)
	setVMFunction("ExpectNoString", far2l_ReqRecvExpectNoString)

	setVMFunction("ExpectAppExit", far2l_ExpectExit)

	setVMFunction("ToggleShift", far2l_ToggleShift)	
	setVMFunction("ToggleLCtrl", far2l_ToggleLCtrl)	
	setVMFunction("ToggleRCtrl", far2l_ToggleRCtrl)
	setVMFunction("ToggleLAlt", far2l_ToggleLAlt)	
	setVMFunction("ToggleRAlt", far2l_ToggleRAlt)
	setVMFunction("TypeText", far2l_TypeText)
	setVMFunction("TypeVK", far2l_TypeVK)
	setVMFunction("TypeFKey", far2l_TypeFKey)
	setVMFunction("TypeDigit", far2l_TypeDigit)
	setVMFunction("TypeAdd", far2l_TypeAdd)
	setVMFunction("TypeSub", far2l_TypeSub)
	setVMFunction("TypeMul", far2l_TypeMul)
	setVMFunction("TypeDiv", far2l_TypeDiv)
	setVMFunction("TypeSeparator", far2l_TypeSeparator)
	setVMFunction("TypeDecimal", far2l_TypeDecimal)

	setVMFunction("TypeEnter", far2l_TypeEnter)
	setVMFunction("TypeEscape", far2l_TypeEscape)
	setVMFunction("TypePageUp", far2l_TypePageUp)
	setVMFunction("TypePageDown", far2l_TypePageDown)
	setVMFunction("TypeEnd", far2l_TypeEnd)
	setVMFunction("TypeHome", far2l_TypeHome)
	setVMFunction("TypeLeft", far2l_TypeLeft)
	setVMFunction("TypeUp", far2l_TypeUp)
	setVMFunction("TypeRight", far2l_TypeRight)
	setVMFunction("TypeDown", far2l_TypeDown)
	setVMFunction("TypeIns", far2l_TypeIns)
	setVMFunction("TypeDel", far2l_TypeDel)
	setVMFunction("TypeBack", far2l_TypeBack)

	setVMFunction("TTYWrite", tty_Write)
	setVMFunction("TTYCtrlC", tty_CtrlC)

	setVMFunction("Log", aux_Log)
	setVMFunction("Panic", aux_Panic)

	setVMFunction("RunCmd", aux_RunCmd)
	setVMFunction("Sleep", aux_Sleep)

	setVMFunction("WorkDir", aux_WorkDir)
	setVMFunction("Chmod", aux_Chmod)
	setVMFunction("Chown", aux_Chown)
	setVMFunction("Chtimes", aux_Chtimes)
	setVMFunction("Mkdir", aux_Mkdir)
	setVMFunction("MkdirTemp", aux_MkdirTemp)
	setVMFunction("Remove", aux_Remove)
	setVMFunction("RemoveAll", aux_RemoveAll)
	setVMFunction("Rename", aux_Rename)
	setVMFunction("ReadFile", aux_ReadFile)
	setVMFunction("WriteFile", aux_WriteFile)
	setVMFunction("Truncate", aux_Truncate)
	setVMFunction("ReadDir", aux_ReadDir)
	setVMFunction("Symlink", aux_Symlink)
	setVMFunction("Readlink", aux_Readlink)
	setVMFunction("MkdirAll", aux_MkdirAll)
	setVMFunction("MkdirsAll", aux_MkdirsAll)
	setVMFunction("Mkfile", aux_Mkfile)
	setVMFunction("Mkfiles", aux_Mkfiles)
	setVMFunction("HashPath", aux_HashPath)
	setVMFunction("HashPathes", aux_HashPathes)
	setVMFunction("Exists", aux_Exists)
	setVMFunction("CountExisting", aux_CountExisting)
	setVMFunction("LoadTextFile", aux_LoadTextFile)
	setVMFunction("SaveTextFile", aux_SaveTextFile)
	setVMFunction("BoundedLinesMatchTextFile", far2l_BoundedLinesMatchTextFile)
	setVMFunction("BoundedLinesSaveAsTextFile", far2l_BoundedLinesSaveAsTextFile)
}

func setVMFunction(name string, value interface{}) {
	err := g_vm.Set(name, value)
	if err != nil {
		aux_Panic(fmt.Sprintf("VMSet(%s): %v", name, err))
	}
}

func main() {
	var err error
	arg_ofs:= 1
	for ;arg_ofs < len(os.Args); arg_ofs++ {
		if os.Args[arg_ofs] == "-t" && arg_ofs + 1 < len(os.Args) {
			arg_ofs++
			v, err := strconv.Atoi(os.Args[arg_ofs])
			if err != nil || v < 0 { aux_Panic("timeout must be positive integer value") }
			g_recv_timeout = uint32(v)
		} else {
			break
		}
	}

	if len(os.Args) < arg_ofs + 2 {
		log.Fatal("Usage: far2l-smoke [-t TIMEOUT_SEC] /path/to/far2l /path/to/test1 [/path/to/test2 [/path/to/test3 ...]]\n")
	}

	g_far2l_sock = fmt.Sprintf("/tmp/far2l%d.sock", os.Getpid())
//filepath.Join(workdir, "far2l.sock")
	os.Remove(g_far2l_sock)
	defer os.Remove(g_far2l_sock)

    g_socket, err = net.ListenUnixgram("unixgram", &net.UnixAddr{Name:g_far2l_sock, Net:"unixgram"})
    if err != nil {
        log.Fatal(err)
    }

	initVM()

	g_far2l_bin, err = filepath.Abs(os.Args[arg_ofs])
    if err != nil {
        log.Fatal(err)
    }

	for i := arg_ofs + 1; i < len(os.Args); i++ {
		name := filepath.Base(os.Args[i])
		fmt.Println("\x1b[1;32m---> Running test: " + name + "\x1b[39;22m")
		testdir, err := filepath.Abs(os.Args[i])
		if err != nil { log.Fatal(err) }
		g_test_workdir = filepath.Join(testdir, "workdir")
		runTest(filepath.Join(testdir, "test.js"))
	}
}

func saveSnapshotOnExit() {
	if g_app != nil {
		f, err := os.Create(g_test_workdir + "/snapshot.txt")
		if err == nil {
			f.WriteString(g_app.Snapshot())
		}
	}
}

func runTest(file string) {
	defer far2l_Close()
	defer saveSnapshotOnExit()

	g_lctrl = false
	g_rctrl = false
	g_lalt = false
	g_ralt = false
	g_shift = false
	g_calm = false
	g_last_error = ""
	data, err := ioutil.ReadFile(file)
	if err != nil { aux_Panic(err.Error()) }
	src := string(data)
	rv, err := g_vm.RunString(src)
	if err != nil { aux_Panic(err.Error()) }
	if code := rv.Export().(int64); code != 0 {
 	   fmt.Println("[FAILED] Error", code, "from test", file)
	}
}
