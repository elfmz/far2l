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
	"io/ioutil"
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

var g_far2l_log string
var g_far2l_snapshot string
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

func stringFromBytes(buf []byte) string {
	last := 0
	for ; last < len(buf) && buf[last] != 0; last++ {
	}
	return string(buf[0:last])
}


func far2l_ReadSocket(expected_n int, extra_timeout uint32) {
	err := g_socket.SetReadDeadline(time.Now().Add(time.Duration(g_recv_timeout + extra_timeout) * time.Second))
    if err != nil {
        panic(err)
	}
	n, addr, err := g_socket.ReadFromUnix(g_buf[:])
    if err != nil || n != expected_n {
		if g_addr == nil {
			if net_err, ok := err.(net.Error); ok && net_err.Timeout() {
				panic("First communication timed out, make sure application built with testing support or increase timeout by -t argument")
			}
		}
        panic(err)
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
		panic(err)
	}
	g_far2l_running = true
	return far2l_RecvStatus()
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
	far2l_ReadSocket(12, tmout / 1000)
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

func far2l_CheckBoundedLineOrDie(expected string, left uint32, top uint32, width uint32, trim_chars string) string {
	line:= far2l_BoundedLine(left, top, width, trim_chars)
	if line != expected {
		panic(fmt.Sprintf("Line at [%d +%d : %d] not expected: '%v'", left, width, top, line))
	}
	return line
}

func far2l_BoundedLine(left uint32, top uint32, width uint32, trim_chars string) string {
	lines:= far2l_BoundedLines(left, top, width, 1, trim_chars)
	return lines[0]
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

	// top & bottom edges has some quirks due to they may contains caption, hints, time etc..
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

func log_Info(message string) {
	log.Print(message)
}

func log_Fatal(message string) {
	panic(message)
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
	binary.LittleEndian.PutUint32(g_buf[0:], 4) // TEST_CMD_SEND_KEY
	binary.LittleEndian.PutUint32(g_buf[4:], controls)
	binary.LittleEndian.PutUint32(g_buf[8:], utf32_code)
	binary.LittleEndian.PutUint32(g_buf[12:], key_code)
	binary.LittleEndian.PutUint32(g_buf[16:], 0)
	binary.LittleEndian.PutUint32(g_buf[20:], 0)
	if pressed { g_buf[20] = 1 }
	n, err := g_socket.WriteTo(g_buf[0:24], g_addr)
	if err != nil || n != 24 {
		panic(err)
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
		return err.Error()
	}
	return ""
}

func aux_RunCmdOrDie(args []string) {
	out:= aux_RunCmd(args)
	if out != "" {
		panic("RunCmdOrDie: " + out)
	}
}

func aux_Sleep(msec uint32) {
	time.Sleep(time.Duration(msec) * time.Millisecond)
}

func initVM() {
	/* initialize */
	fmt.Println("Initializing JS VM...")
	g_vm = goja.New()

	/* goja does not expose a standard "global" by default */
	_, err:= g_vm.RunString("var global = (function(){ return this; }).call(null);")
	if err != nil { panic(err) }

	setVMFunction("StartApp", far2l_Start)
	setVMFunction("AppStatus", far2l_ReqRecvStatus)
	setVMFunction("ReadCellRaw", far2l_ReqRecvReadCellRaw)
	setVMFunction("ReadCell", far2l_ReqRecvReadCell)
	setVMFunction("CheckCellChar", far2l_CheckCellChar)
	setVMFunction("CheckCellCharOrDie", far2l_CheckCellCharOrDie)
	
	setVMFunction("BoundedLines", far2l_BoundedLines)
	setVMFunction("BoundedLine", far2l_BoundedLine)
	setVMFunction("CheckBoundedLineOrDie", far2l_CheckBoundedLineOrDie)
	setVMFunction("SurroundedLines", far2l_SurroundedLines)

	setVMFunction("ExpectStrings", far2l_ReqRecvExpectStrings)
	setVMFunction("ExpectStringsOrDie", far2l_ReqRecvExpectStringsOrDie)
	setVMFunction("ExpectString", far2l_ReqRecvExpectString)
	setVMFunction("ExpectStringOrDie", far2l_ReqRecvExpectStringOrDie)

	setVMFunction("ExpectAppExit", far2l_ExpectExit)
	setVMFunction("ExpectAppExitOrDie", far2l_ExpectExitOrDie)

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

	setVMFunction("LogInfo", log_Info)
	setVMFunction("LogFatal", log_Fatal)

	setVMFunction("RunCmd", aux_RunCmd)
	setVMFunction("RunCmdOrDie", aux_RunCmdOrDie)
	setVMFunction("Sleep", aux_Sleep)
}

func setVMFunction(name string, value interface{}) {
	err := g_vm.Set(name, value)
	if err != nil {
		panic(fmt.Sprintf("VMSet(%s): %v", name, err))
	}
}

func main() {
	var err error
	if len(os.Args) < 4 {
		log.Fatal("Usage: far2l-smoke [-t TIMEOUT_SEC] /path/to/far2l /path/to/results/dir /path/to/test1.js [/path/to/test2.js [/path/to/test3.js ...]]\n")
	}
	arg_ofs:= 1
	for ;arg_ofs < len(os.Args); arg_ofs++ {
		if os.Args[arg_ofs] == "-t" && arg_ofs + 1 < len(os.Args) {
			arg_ofs++
			v, err := strconv.Atoi(os.Args[arg_ofs])
			if err != nil || v < 0 { panic("timeout must be positive integer value") }
			g_recv_timeout = uint32(v)
		} else {
			break
		}
	}

	g_far2l_sock, err = filepath.Abs(filepath.Join(os.Args[arg_ofs + 1], "far2l.sock"))
	if err != nil { log.Fatal(err) }
	g_far2l_log, err = filepath.Abs(filepath.Join(os.Args[arg_ofs + 1], "far2l.log"))
	if err != nil { log.Fatal(err) }
	g_far2l_snapshot, err = filepath.Abs(filepath.Join(os.Args[arg_ofs + 1], "snapshot.log"))
	if err != nil { log.Fatal(err) }

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

	for i := arg_ofs + 2; i < len(os.Args); i++ {
		fmt.Println("---> Running test:", os.Args[i])
		runTest(os.Args[i])
	}
}

func saveSnapshotOnExit() {
	if g_app != nil {
		f, err := os.Create(g_far2l_snapshot)
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
	data, err := ioutil.ReadFile(file)
	if err != nil { panic(err) }
	src := string(data)
	rv, err := g_vm.RunString(src)
	if err != nil { panic(err) }
	if code := rv.Export().(int64); code != 0 {
 	   fmt.Println("[FAILED] Error", code, "from test", file)
	}
}
