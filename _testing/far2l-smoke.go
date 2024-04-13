package main

import (
	"log"
	"os"
	"net"
	"fmt"
	"time"
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

func far2l_ReqRecvExpectStrings(str_vec []string, x uint32, y uint32, w uint32, h uint32, tmout uint32) far2l_FoundString {
	if w == 0xffffffff { w = g_status.Width; }
	if h == 0xffffffff { h = g_status.Height; }
	binary.LittleEndian.PutUint32(g_buf[0:], 3)
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

	err = g_vm.Set("ExpectStrings", far2l_ReqRecvExpectStrings)
	if err != nil { panic(err) }
	err = g_vm.Set("ExpectString", far2l_ReqRecvExpectString)
	if err != nil { panic(err) }

	err = g_vm.Set("ExpectAppExit", far2l_ExpectExit)
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
