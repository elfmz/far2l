package main

import (
	"log"
	"os"
	"net"
	"fmt"
	"time"
	"io/ioutil"
	"encoding/binary"
	"path/filepath"
    "github.com/ActiveState/termtest"
    "github.com/dop251/goja"
)

type far2l_Status struct {
	width uint32
	height uint32
	title string
}

type far2l_FoundString struct {
	i uint32
	x uint32
	y uint32
}

var g_socket *net.UnixConn
var g_addr *net.UnixAddr
var g_buf [4096]byte
var g_app *termtest.ConsoleProcess
var g_vm *goja.Runtime
var g_status far2l_Status

func stringFromBytes(buf []byte) string {
	last := 0
	for ; last < len(buf) && buf[last] != 0; last++ {
	}
	return string(buf[0:last])
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

	g_status.width = binary.LittleEndian.Uint32(g_buf[12:])
	g_status.height = binary.LittleEndian.Uint32(g_buf[16:])
	g_status.title = stringFromBytes(g_buf[20:])
	fmt.Println("addr:", g_addr, "width:", g_status.width, "height:", g_status.height, "title:", g_status.title)
	return g_status
}

func far2l_ReqRecvWaitString(str string, x uint32, y uint32, w uint32, h uint32, tmout uint32) far2l_FoundString {
	return far2l_ReqRecvWaitStrings([]string{str}, x, y, w, h, tmout)
}

func far2l_ReqRecvWaitStrings(str_vec []string, x uint32, y uint32, w uint32, h uint32, tmout uint32) far2l_FoundString {
	if w == 0xffffffff { w = g_status.width; }
	if h == 0xffffffff { h = g_status.height; }
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
		i:  binary.LittleEndian.Uint32(g_buf[0:]),
		x: binary.LittleEndian.Uint32(g_buf[4:]),
		y: binary.LittleEndian.Uint32(g_buf[8:]),
	}
	if out.i < uint32(len(str_vec)) {
		fmt.Println("Waited string i:", out.i, "found at x:", out.x, "y:", out.y, "-", str_vec[out.i])
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

func far2l_WriteTTY(s string) {
    g_app.Send(s)
}

func far2l_ExpectExit(code int, timeout_ms int) string {
    _, err:= g_app.ExpectExitCode(code, time.Duration(timeout_ms) * 1000000)
	if err != nil {
		fmt.Println("ExpectExit", err)
		return err.Error()
	}
	return ""
}

func far2l_Log(message string) {
	log.Print(message)
}

func far2l_Fatal(message string) {
	log.Fatal(message)
}


func main() {
	if len(os.Args) < 4 {
		log.Fatal("Usage: far2l-smoke /path/to/far2l /path/to/results/dir /path/to/test1.js [/path/to/test2.js [/path/to/test3.js ...]]\n")
	}
	far2l_bin, err := filepath.Abs(os.Args[1])
    if err != nil {
        log.Fatal(err)
    }
	far2l_sock, err := filepath.Abs(filepath.Join(os.Args[2], "far2l.sock"))
    if err != nil {
        log.Fatal(err)
    }
	far2l_log, err := filepath.Abs(filepath.Join(os.Args[2], "far2l.log"))
    if err != nil {
        log.Fatal(err)
    }

	os.Remove(far2l_sock)
	defer os.Remove(far2l_sock)

    g_socket, err = net.ListenUnixgram("unixgram", &net.UnixAddr{Name:far2l_sock, Net:"unixgram"})
    if err != nil {
        log.Fatal(err)
    }

    opts := termtest.Options {
        CmdName: far2l_bin,
		Args: []string{far2l_bin, "--tty", "--nodetect", "--mortal", "--test=" + far2l_sock, "-cd", "/usr/include"},
		Environment : []string{"FAR2L_STD=" + far2l_log},
    }

	/* initialize */
	fmt.Println("Initializing JS VM...")
	g_vm = goja.New()

	/* goja does not expose a standard "global" by default */
	_, err = g_vm.RunString("var global = (function(){ return this; }).call(null);")
	if err != nil { panic(err) }
	err = g_vm.Set("far2l_WaitStrings", far2l_ReqRecvWaitStrings)
	if err != nil { panic(err) }
	err = g_vm.Set("far2l_WaitString", far2l_ReqRecvWaitString)
	if err != nil { panic(err) }
	err = g_vm.Set("far2l_Status", far2l_ReqRecvStatus)
	if err != nil { panic(err) }
	err = g_vm.Set("far2l_WriteTTY", far2l_WriteTTY)
	if err != nil { panic(err) }
	err = g_vm.Set("far2l_Bye", far2l_ReqBye)
	if err != nil { panic(err) }
	err = g_vm.Set("far2l_ExpectExit", far2l_ExpectExit)
	if err != nil { panic(err) }
	err = g_vm.Set("far2l_Log", far2l_Log)
	if err != nil { panic(err) }
	err = g_vm.Set("far2l_Fatal", far2l_Fatal)
	if err != nil { panic(err) }



	for i := 3; i < len(os.Args); i++ {
		fmt.Println("---> Running test:", os.Args[i])
		runTest(opts, os.Args[i])
	}

/*
	fmt.Println("Talking....")
    //app.Expect("/usr/include")
	far2l_ReqRecvWaitStrings([]string{"foobar", "/usr/include"}, 0, 0, g_width, 1, 1000)

	fmt.Println("Got /usr/include....")
	far2l_ReqRecvStatus()
    app.Send("\x1b[21~")
    app.Expect("Do you want to quit FAR?")
	fmt.Println("Got exit....")
    app.Send("\r\n")
	far2l_ReqBye()
    app.ExpectExitCode(0)
*/
}

func runTest(opts termtest.Options, file string) {
	var err error
    g_app, err = termtest.New(opts)
	if err != nil {
		log.Fatal(err)
	}
	defer g_app.Close()
	fmt.Println("Waiting far2l start....")
	fst:= far2l_RecvStatus()
	far2l_ReqRecvWaitStrings([]string{"foobar", "/usr/include"}, 0, 0, fst.width, 1, 1000)

	fmt.Println("Testing....")
	data, err := ioutil.ReadFile(file)
	if err != nil { panic(err) }
	src := string(data)
	rv, err := g_vm.RunString(src)
	if err != nil { panic(err) }
	if code := rv.Export().(int64); code != 0 {
 	   fmt.Println("[FAILED] Error", code, "from test", file)
	}
}
