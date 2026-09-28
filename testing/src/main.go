package main

import (
	"errors"
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
	"syscall"
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
var g_channel chan int
var g_vm *goja.Runtime
var g_status far2l_Status
var g_recv_timeout uint32 = 30
var g_test_workdir string
var g_calm bool = false
var g_last_error string
var g_autosync uint32 = 10000
var g_autosync_needed bool = false

const far2lTestTextMax = 2048
const far2lStatusPacketSize = 20 + far2lTestTextMax
const far2lReadCellPacketSize = 8 + far2lTestTextMax
const far2lWaitStringPacketSize = 24 + far2lTestTextMax
const far2lSocketBufferSize = 1024 * 1024

func far2l_ConfigureSocketBuffers(socket *net.UnixConn) error {
	rawConn, err := socket.SyscallConn()
	if err != nil {
		return err
	}

	var socketErr error
	err = rawConn.Control(func(fd uintptr) {
		if err := syscall.SetsockoptInt(int(fd), syscall.SOL_SOCKET, syscall.SO_RCVBUF, far2lSocketBufferSize); err != nil {
			socketErr = err
			return
		}
		socketErr = syscall.SetsockoptInt(int(fd), syscall.SOL_SOCKET, syscall.SO_SNDBUF, far2lSocketBufferSize)
	})
	if err != nil {
		return err
	}
	return socketErr
}

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

func isRetryableSocketWriteError(err error) bool {
	if err == nil {
		return false
	}
	if netErr, ok := err.(net.Error); ok && (netErr.Timeout() || netErr.Temporary()) {
		return true
	}
	return errors.Is(err, syscall.ENOBUFS) || errors.Is(err, syscall.EAGAIN) || errors.Is(err, syscall.EWOULDBLOCK)
}

func far2l_WriteToPeer(data []byte) {
	deadline := time.Now().Add(2 * time.Second)
	for {
		n, err := g_socket.WriteTo(data, g_addr)
		if err == nil && n == len(data) {
			return
		}
		if err == nil {
			aux_Panic(fmt.Sprintf("short write %d/%d", n, len(data)))
		}
		if isRetryableSocketWriteError(err) && time.Now().Before(deadline) {
			time.Sleep(2 * time.Millisecond)
			continue
		}
		aux_Panic(err.Error())
	}
}

func far2l_Close() {
	if g_app != nil {
		app:= g_app
		log.Println("Stopping application due to ExpectExit wasnt called")
		app.Stop()
		_ = <-g_channel
		app.Close()
		g_app = nil
	}
}

func termTask(args []string, cols int, rows int) {
    opts := termtest.Options {
        CmdName: g_far2l_bin, // g_far2l_bin, 
		Args: append([]string{"--test=" + g_far2l_sock}, args...),
		Environment : append(os.Environ(), []string {
			"FAR2L_STD=" + filepath.Join(g_test_workdir, "far2l.log"),
			"FAR2L_TESTCTL=" + g_far2l_sock}...),
		ExtraOpts: []expect.ConsoleOpt{expect.WithTermRows(rows), expect.WithTermCols(cols)},
    }
	var err error
    g_app, err = termtest.New(opts)
	if err != nil {
		aux_Warn(err.Error())
	} else {
		g_app.Wait()
		code:= g_app.Cmd().ProcessState.ExitCode()
		g_channel <- code
	}
}

func far2l_StartWithSize(args []string, cols int, rows int) far2l_Status {
	if g_app != nil {
		aux_Panic("far2l already running")
	}
	g_channel = make(chan int)
	go termTask(args, cols, rows)
	return far2l_RecvStatus()
}

func far2l_Start(args []string) far2l_Status {
	return far2l_StartWithSize(args, 120, 80)
}

func far2l_ReqRecvStatus() far2l_Status {
	binary.LittleEndian.PutUint32(g_buf[0:], 1)
	far2l_WriteToPeer(g_buf[0:4])
	return far2l_RecvStatus()
}

func far2l_RecvStatus() far2l_Status {
	far2l_ReadSocket(far2lStatusPacketSize, 0)
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
	far2l_WriteToPeer(g_buf[0:8])
	far2l_ReadSocket(1, tmout)
	return g_buf[0] != 0
}

func far2l_Sync(tmout uint32) bool {
	if tmout == 0 {
		tmout = g_autosync
	}
	log.Println("Sync:", tmout)
	g_autosync_needed = false
	if !far2l_ReqRecvSync(tmout) {
		setErrorString("Sync timout")
		return false
	}
	return true
}

func far2l_AutoSync(tmout uint32) uint32 {
	prev:= g_autosync
	g_autosync = tmout
	log.Println("AutoSync:", prev, "->", tmout)
	return prev
}

func performAutoSync() {
	if !g_autosync_needed {
		return
	}
	g_autosync_needed = false
	if g_autosync == 0 || g_app == nil {
		return
	}
	if ! far2l_ReqRecvSync(g_autosync) {
		aux_Warn("failed to perform autosync")
	}
}

func scheduleAutoSync() {
	g_autosync_needed = true
}

func far2l_ReqBye() {
	binary.LittleEndian.PutUint32(g_buf[0:], 0)
	far2l_WriteToPeer(g_buf[0:4])
}

func aux_Log(message string) {
	log.Print(message)
}

func aux_Warn(warn string) {
	log.Print("\x1b[1;33mWARN: " + warn + "\x1b[39;22m")
}

func aux_Panic(message string) {
	if (g_app != nil) {
		aux_Sleep(100)
		if (g_app != nil) {
			log.Println("------------------- SNAPSHOT -------------------")
			fmt.Println(strings.TrimLeft(g_app.Snapshot(), " \r\n"))
			log.Println("------------------------------------------------")
		}
	}
	log.Println("\x1b[1;31m" + message + "\x1b[39;22m")
//	log.Println("Backtrace:", g_vm.CaptureCallStack(-1, []goja.StackFrame{}))
	g_vm.Interrupt(message)
//	panic("\x1b[1;31m" + message + "\x1b[39;22m")
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

func aux_Snapshot(name string) {
	if g_app != nil {
		aux_Sleep(100)
		if g_app != nil {
			f, err := os.Create(g_test_workdir + "/snapshot-" + name + ".txt")
			defer f.Close()
			if err == nil {
				f.WriteString(g_app.Snapshot())
			}
		}
	}
}

func initVM() {
	/* initialize */
	log.Println("Initializing JS VM...")
	g_vm = goja.New()

	/* goja does not expose a standard "global" by default */
	_, err:= g_vm.RunString("var global = (function(){ return this; }).call(null);")
	if err != nil { aux_Panic(err.Error()) }

	setVMFunction("BePanic", aux_BePanic)
	setVMFunction("BeCalm", aux_BeCalm)
	setVMFunction("Inspect", aux_Inspect)
	setVMFunction("Snapshot", aux_Snapshot)

	setVMFunction("StartApp", far2l_Start)
	setVMFunction("StartAppWithSize", far2l_StartWithSize)
	setVMFunction("CloseApp", far2l_Close)

	setVMFunction("AppStatus", far2l_ReqRecvStatus)
	setVMFunction("Sync", far2l_Sync)
	setVMFunction("AutoSync", far2l_AutoSync)

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
	setVMFunction("SetDefaultExpectTimeout", far2l_SetDefaultExpectTimeout)

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
	setVMFunction("TypeTab", far2l_TypeTab)

	setVMFunction("LClickWhereFound", far2l_LClickWhereFound)
	setVMFunction("RClickWhereFound", far2l_RClickWhereFound)
	setVMFunction("DblClickWhereFound", far2l_DblClickWhereFound)
	setVMFunction("LClick", far2l_LClick)
	setVMFunction("RClick", far2l_RClick)
	setVMFunction("DblClick", far2l_DblClick)

	setVMFunction("TTYWrite", tty_Write)
	setVMFunction("TTYCtrlC", tty_CtrlC)

	setVMFunction("Log", aux_Log)
	setVMFunction("Warn", aux_Warn)
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
	setVMFunction("CheckFilesDataSame", aux_CheckFilesDataSame)
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
	log.SetFlags(log.LUTC | log.Ltime | log.Lmicroseconds)
	g_far2l_sock = fmt.Sprintf("/tmp/far2l%d.sock", os.Getpid())
//filepath.Join(workdir, "far2l.sock")
	os.Remove(g_far2l_sock)
	defer os.Remove(g_far2l_sock)

    g_socket, err = net.ListenUnixgram("unixgram", &net.UnixAddr{Name:g_far2l_sock, Net:"unixgram"})
    if err != nil {
        log.Fatal(err)
    }
	if err = far2l_ConfigureSocketBuffers(g_socket); err != nil {
		log.Fatal(err)
	}

	initVM()

	g_far2l_bin, err = filepath.Abs(os.Args[arg_ofs])
    if err != nil {
        log.Fatal(err)
    }

	for i := arg_ofs + 1; i < len(os.Args); i++ {
		name := filepath.Base(os.Args[i])
		fmt.Println() // empty line witout timestamp
		log.Println("\x1b[1;32m---> Running test: " + name + "\x1b[39;22m")
		testdir, err := filepath.Abs(os.Args[i])
		if err != nil { log.Fatal(err) }
		g_test_workdir = filepath.Join(testdir, "workdir")
		runTest(filepath.Join(testdir, "test.js"))
	}
}

func runTest(file string) {
	defer far2l_Close()
	defer aux_Snapshot("exit")
	typingReset()
	g_autosync = 10000
	g_calm = false
	g_last_error = ""
	data, err := ioutil.ReadFile(file)
	if err != nil {
		panic("[FAILED] Error '" + err.Error() + "' reading test" + file)
	}
	src := string(data)
	rv, err := g_vm.RunString(src)
	if err != nil {
		panic("[FAILED] Error '" + err.Error() + "' running test" + file)
	}
	if code := rv.Export().(int64); code != 0 {
		log.Println("[FAILED] Error", code, "from test", file)
	} else {
		log.Println("[DONE]", file)
	}
}
