package main

import (
	"log"
	"os"
	"net"
	"fmt"
	"time"
	"strings"
	"os/exec"
	"io/ioutil"
	"encoding/binary"
	"path/filepath"
    "github.com/ActiveState/termtest"
    "github.com/ActiveState/termtest/expect"
    "github.com/dop251/goja"
)

type far2l_Status struct {
	Width uint32
	Height uint32
	Title string
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

var g_far2l_log string
var g_far2l_sock string
var g_far2l_bin string
var g_socket *net.UnixConn
var g_addr *net.UnixAddr
var g_buf [4096]byte
var g_app *termtest.ConsoleProcess
var g_vm *goja.Runtime
var g_status far2l_Status
var g_far2l_running bool

func stringFromBytes(buf []byte) string {
	last := 0
	for ; last < len(buf) && buf[last] != 0; last++ {
	}
	return string(buf[0:last])
}

func far2l_Start(args []string) far2l_Status {
	if g_far2l_running {
		panic("far2l already running")
	}
    opts := termtest.Options {
        CmdName: g_far2l_bin,
		Args: append([]string{g_far2l_bin, "--test=" + g_far2l_sock}, args...),
		Environment : []string{"FAR2L_STD=" + g_far2l_log},
		ExtraOpts: []expect.ConsoleOpt{expect.WithTermRows(80), expect.WithTermCols(120)},
    }
	var err error
    g_app, err = termtest.New(opts)
	if err != nil {
		log.Fatal(err)
	}
	g_far2l_running = true
	return far2l_RecvStatus()
}

func far2l_Close() {
	if g_far2l_running {
		g_far2l_running = false
		g_app.Close()
	}
}

func far2l_ReqRecvStatus() far2l_Status {
	binary.LittleEndian.PutUint32(g_buf[0:], 1)
	n, err := g_socket.WriteTo(g_buf[0:4], g_addr)
	if err != nil || n != 4 {
		panic(err)
	}
	return far2l_RecvStatus()
}

func far2l_RecvStatus() far2l_Status {
	n, addr, err := g_socket.ReadFromUnix(g_buf[:])
    if err != nil || n != 2068 {
        panic(err)
    }
	g_addr = addr

	g_status.Width = binary.LittleEndian.Uint32(g_buf[12:])
	g_status.Height = binary.LittleEndian.Uint32(g_buf[16:])
	g_status.Title = stringFromBytes(g_buf[20:])
//	fmt.Println("addr:", g_addr, "width:", g_status.Width, "height:", g_status.Height, "title:", g_status.Title)
	return g_status
}

func far2l_ReqRecvExpectString(str string, x uint32, y uint32, w uint32, h uint32, tmout uint32) far2l_FoundString {
	return far2l_ReqRecvExpectStrings([]string{str}, x, y, w, h, tmout)
}

func far2l_ReqRecvExpectStringOrDie(str string, x uint32, y uint32, w uint32, h uint32, tmout uint32) far2l_FoundString {
	return far2l_ReqRecvExpectStringsOrDie([]string{str}, x, y, w, h, tmout)
}

func far2l_ReqRecvExpectStrings(str_vec []string, x uint32, y uint32, w uint32, h uint32, tmout uint32) far2l_FoundString {
	if w == 0xffffffff { w = g_status.Width; }
	if h == 0xffffffff { h = g_status.Height; }
	binary.LittleEndian.PutUint32(g_buf[0:], 3) //TEST_CMD_WAIT_STRING
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
		panic("Too long strings")
	}
	for ; p < 2048; p++ {
		g_buf[24 + p] = 0
	}

	n, err := g_socket.WriteTo(g_buf[0:24 + 2048], g_addr)
	if err != nil || n != 24 + 2048 {
		panic(err)
	}
	n, err = g_socket.Read(g_buf[:])
    if err != nil || n != 12 {
        panic(err)
    }
	out := far2l_FoundString {
		I: binary.LittleEndian.Uint32(g_buf[0:]),
		X: binary.LittleEndian.Uint32(g_buf[4:]),
		Y: binary.LittleEndian.Uint32(g_buf[8:]),
	}
	if out.I < uint32(len(str_vec)) {
		fmt.Println("Waited string I:", out.I, "found at X:", out.X, "Y:", out.Y, "-", str_vec[out.I])
	} else {
		fmt.Println("Wait strings timeout", str_vec)
	}
	return out
}

func far2l_ReqRecvExpectStringsOrDie(str_vec []string, x uint32, y uint32, w uint32, h uint32, tmout uint32) far2l_FoundString {
	out := far2l_ReqRecvExpectStrings(str_vec, x, y, w, h, tmout)
	if out.I == 0xffffffff {
		panic(fmt.Sprintf("Couldn't find at [%d +%d : %d +%d] any of expected strings: %v", x, w, y, h, str_vec))
	}
	return out
}


func far2l_ReqRecvReadCellRaw(x uint32, y uint32) far2l_CellRaw {
	binary.LittleEndian.PutUint32(g_buf[0:], 2) // TEST_CMD_READ_CELL
	binary.LittleEndian.PutUint32(g_buf[4:], x) // left
	binary.LittleEndian.PutUint32(g_buf[8:], y) // top
	n, err := g_socket.WriteTo(g_buf[0:12], g_addr)
	if err != nil || n != 12 {
		panic(err)
	}
	n, err = g_socket.Read(g_buf[:])
    if err != nil || n != 2056 {
        panic(err)
    }
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

func far2l_BoundedLines(left uint32, top uint32, width uint32, height uint32, trim_chars string) []string {
	lines:= []string{}
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

	for left = x; left > 0 && far2l_CheckCellChar(left - 1, y, boundary_chars) == ""; left-- {
	}

	for width = 1; left + width < g_status.Width && far2l_CheckCellChar(left + width, y, boundary_chars) == ""; width++ {
	}

	// top & bottom edges has some quirks due to they may contains captions, hints, time etc..
	for top = y; top > 0 &&
		far2l_CheckCellChar(left, top - 1, boundary_chars) == "" &&
		far2l_CheckCellChar(left + width / 2, top - 1, boundary_chars) == "" &&
		far2l_CheckCellChar(left + width - 1, top - 1, boundary_chars) == ""; top-- {
	}

	for height = 1; top + height < g_status.Height &&
		far2l_CheckCellChar(left, top + height, boundary_chars) == "" &&
		far2l_CheckCellChar(left + width / 2, top + height, boundary_chars) == "" &&
		far2l_CheckCellChar(left + width - 1, top + height, boundary_chars) == ""; height++ {
	}

	return far2l_BoundedLines(left, top, width, height, trim_chars)
}

func far2l_CheckCellChar(x uint32, y uint32, chars string) string {
	cell := far2l_ReqRecvReadCellRaw(x, y)
	if cell.Text != "" && strings.Contains(chars, cell.Text) {
		return cell.Text
	}
	return ""
}

func far2l_CheckCellCharOrDie(x uint32, y uint32, chars string) string {
	out:= far2l_CheckCellChar(x, y, chars)
	if out == "" {
		panic(fmt.Sprintf("Cell at %d:%d doesnt represent any of: %s", x, y, chars))
	}
	return out
}


func far2l_ReqBye() {
	binary.LittleEndian.PutUint32(g_buf[0:], 0)
	n, err := g_socket.WriteTo(g_buf[0:4], g_addr)
	if err != nil || n != 4 {
		panic(err)
	}
}

func far2l_ExpectExit(code int, timeout_ms int) string {
	far2l_ReqBye()
    _, err:= g_app.ExpectExitCode(code, time.Duration(timeout_ms) * 1000000)
	if err != nil {
		fmt.Println("ExpectExit", err)
		return err.Error()
	}
	return ""
}

func far2l_ExpectExitOrDie(code int, timeout_ms int) {
	out:= far2l_ExpectExit(code, timeout_ms)
	if out != "" {
		panic("ExpectExit: " + out)
	}
}

func LogInfo(message string) {
	log.Print(message)
}

func LogFatal(message string) {
	log.Fatal(message)
}

func WriteTTY(s string) {
    g_app.Send(s)
}

func CtrlC() {
    g_app.SendCtrlC()
}

func RunCmd(args []string) string {
	prog, err := exec.LookPath(args[0])
	if err != nil {
		return err.Error()
	}
	cmd := exec.Command(prog, args...)
	err = cmd.Run()
	if (err != nil) {
		return err.Error()
	}
	return ""
}

func RunCmdOrDie(args []string) {
	out:= RunCmd(args)
	if out != "" {
		panic("RunCmdOrDie: " + out)
	}
}

func Sleep(msec uint32) {
	time.Sleep(time.Duration(msec) * time.Millisecond)
}

func initVM() {
	/* initialize */
	fmt.Println("Initializing JS VM...")
	g_vm = goja.New()

	/* goja does not expose a standard "global" by default */
	_, err:= g_vm.RunString("var global = (function(){ return this; }).call(null);")
	if err != nil { panic(err) }

	err = g_vm.Set("StartApp", far2l_Start)
	if err != nil { panic(err) }
	err = g_vm.Set("AppStatus", far2l_ReqRecvStatus)
	if err != nil { panic(err) }

	err = g_vm.Set("ReadCellRaw", far2l_ReqRecvReadCellRaw)
	if err != nil { panic(err) }
	err = g_vm.Set("ReadCell", far2l_ReqRecvReadCell)
	if err != nil { panic(err) }
	err = g_vm.Set("CheckCellChar", far2l_CheckCellChar)
	if err != nil { panic(err) }
	err = g_vm.Set("CheckCellCharOrDie", far2l_CheckCellCharOrDie)
	if err != nil { panic(err) }
	err = g_vm.Set("BoundedLines", far2l_BoundedLines)
	if err != nil { panic(err) }
	err = g_vm.Set("SurroundedLines", far2l_SurroundedLines)
	if err != nil { panic(err) }

	err = g_vm.Set("ExpectStrings", far2l_ReqRecvExpectStrings)
	if err != nil { panic(err) }
	err = g_vm.Set("ExpectStringsOrDie", far2l_ReqRecvExpectStringsOrDie)
	if err != nil { panic(err) }
	err = g_vm.Set("ExpectString", far2l_ReqRecvExpectString)
	if err != nil { panic(err) }
	err = g_vm.Set("ExpectStringOrDie", far2l_ReqRecvExpectStringOrDie)
	if err != nil { panic(err) }

	err = g_vm.Set("ExpectAppExit", far2l_ExpectExit)
	if err != nil { panic(err) }
	err = g_vm.Set("ExpectAppExitOrDie", far2l_ExpectExitOrDie)
	if err != nil { panic(err) }

	err = g_vm.Set("LogInfo", LogInfo)
	if err != nil { panic(err) }
	err = g_vm.Set("LogFatal", LogFatal)
	if err != nil { panic(err) }
	err = g_vm.Set("WriteTTY", WriteTTY)
	if err != nil { panic(err) }
	err = g_vm.Set("CtrlC", CtrlC)
	if err != nil { panic(err) }
	err = g_vm.Set("RunCmd", RunCmd)
	if err != nil { panic(err) }
	err = g_vm.Set("RunCmdOrDie", RunCmdOrDie)
	if err != nil { panic(err) }
	err = g_vm.Set("Sleep", Sleep)
	if err != nil { panic(err) }
}

func main() {
	var err error
	if len(os.Args) < 4 {
		log.Fatal("Usage: far2l-smoke /path/to/far2l /path/to/results/dir /path/to/test1.js [/path/to/test2.js [/path/to/test3.js ...]]\n")
	}
	g_far2l_sock, err = filepath.Abs(filepath.Join(os.Args[2], "far2l.sock"))
    if err != nil {
        log.Fatal(err)
    }
	g_far2l_log, err = filepath.Abs(filepath.Join(os.Args[2], "far2l.log"))
    if err != nil {
        log.Fatal(err)
    }
	os.Remove(g_far2l_sock)
	defer os.Remove(g_far2l_sock)

    g_socket, err = net.ListenUnixgram("unixgram", &net.UnixAddr{Name:g_far2l_sock, Net:"unixgram"})
    if err != nil {
        log.Fatal(err)
    }

	initVM()

	g_far2l_bin, err = filepath.Abs(os.Args[1])
    if err != nil {
        log.Fatal(err)
    }

	for i := 3; i < len(os.Args); i++ {
		fmt.Println("---> Running test:", os.Args[i])
		runTest(os.Args[i])
	}
}

func runTest(file string) {
	defer far2l_Close()
	data, err := ioutil.ReadFile(file)
	if err != nil { panic(err) }
	src := string(data)
	rv, err := g_vm.RunString(src)
	if err != nil { panic(err) }
	if code := rv.Export().(int64); code != 0 {
 	   fmt.Println("[FAILED] Error", code, "from test", file)
	}
}
