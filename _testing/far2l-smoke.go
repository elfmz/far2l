package main

import (
	"log"
	"os"
	"net"
	"fmt"
	"encoding/binary"
	"path/filepath"
    "github.com/ActiveState/termtest"
)

var g_socket *net.UnixConn
var g_addr *net.UnixAddr
var g_buf [4096]byte
var g_width uint32
var g_height uint32
var g_title string

func stringFromBytes(buf []byte) string {
	last := 0
	for ; last < len(buf) && buf[last] != 0; last++ {
	}
	return string(buf[0:last])
}

func far2l_ReqRecvStatus() {
	binary.LittleEndian.PutUint32(g_buf[0:], 1)
	n, err := g_socket.WriteTo(g_buf[0:4], g_addr)
	if err != nil || n != 4 {
		panic(err)
	}
	far2l_RecvStatus()
}

func far2l_RecvStatus() {
	n, addr, err := g_socket.ReadFromUnix(g_buf[:])
    if err != nil || n != 2068 {
        panic(err)
    }
	g_addr = addr
	g_width = binary.LittleEndian.Uint32(g_buf[12:])
	g_height = binary.LittleEndian.Uint32(g_buf[16:])
	g_title = stringFromBytes(g_buf[20:])
	fmt.Println("addr:", g_addr, "width:", g_width, "height:", g_height, "title:", g_title)
}

func far2l_ReqRecvWaitString(str string, x uint32, y uint32, w uint32, h uint32, tmout uint32) {
	far2l_ReqRecvWaitStrings([]string{str}, x, y, w, h, tmout)
}

func far2l_ReqRecvWaitStrings(str_vec []string, x uint32, y uint32, w uint32, h uint32, tmout uint32) {
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
	found_i := binary.LittleEndian.Uint32(g_buf[0:])
	found_x := binary.LittleEndian.Uint32(g_buf[4:])
	found_y := binary.LittleEndian.Uint32(g_buf[8:])
	fmt.Println("Waited string i:", found_i, "found at x:", found_x, "y:", found_y)
}


func far2l_ReqBye() {
	binary.LittleEndian.PutUint32(g_buf[0:], 0)
	n, err := g_socket.WriteTo(g_buf[0:4], g_addr)
	if err != nil || n != 4 {
		panic(err)
	}
}

func main() {
	if len(os.Args) != 4 {
		log.Fatal("Usage: far2l-smoke /path/to/far2l /path/to/test /path/to/results/dir\n")
	}
	far2l_bin, err := filepath.Abs(os.Args[1])
    if err != nil {
        log.Fatal(err)
    }
	far2l_sock, err := filepath.Abs(filepath.Join(os.Args[3], "far2l.sock"))
    if err != nil {
        log.Fatal(err)
    }
	far2l_log, err := filepath.Abs(filepath.Join(os.Args[3], "far2l.log"))
    if err != nil {
        log.Fatal(err)
    }

	os.Remove(far2l_sock)
	defer os.Remove(far2l_sock)

    g_socket, err = net.ListenUnixgram("unixgram", &net.UnixAddr{far2l_sock, "unixgram"})
    if err != nil {
        log.Fatal(err)
    }

    opts := termtest.Options {
        CmdName: far2l_bin,
		Args: []string{far2l_bin, "--tty", "--nodetect", "--mortal", "--test=" + far2l_sock, "-cd", "/usr/include"},
		Environment : []string{"FAR2L_STD=" + far2l_log},
    }
    app, err := termtest.New(opts)
	if err != nil {
		log.Fatal(err)
	}
	fmt.Println("Waiting for status....")
	far2l_RecvStatus()
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
}
