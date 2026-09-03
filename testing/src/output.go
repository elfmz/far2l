package main

import (
	"log"
	"fmt"
	"time"
	"strings"
	"context"
	"encoding/binary"
)

type far2l_FoundString struct {
	I uint32
	X uint32
	Y uint32
}

var g_default_expect_tmout uint32 = 10000

func prepareXYWH(x int32, y int32, w int32, h int32, what string) (int32, int32, int32, int32) {
	if x < 0 || y < 0 || w <= 0 || h <= 0 {
		far2l_ReqRecvStatus()
	}
	saved_x:= x
	saved_y:= y
	saved_w:= w
	saved_h:= h
	if x < 0 { x = int32(g_status.Width) + x; }
	if y < 0 { y = int32(g_status.Height) + y; }
	if w <= 0 { w = int32(g_status.Width) + w; }
	if h <= 0 { h = int32(g_status.Height) + h; }
	if x < 0 { x = 0; aux_Warn(fmt.Sprintf("%s - underflow: x=%d width=%d", what, saved_x, g_status.Width)); }
	if y < 0 { y = 0; aux_Warn(fmt.Sprintf("%s - underflow: y=%d height=%d", what, saved_y, g_status.Height)); }
	if w < 0 { w = 0; aux_Warn(fmt.Sprintf("%s - underflow: w=%d width=%d", what, saved_w, g_status.Width)); }
	if h < 0 { h = 0; aux_Warn(fmt.Sprintf("%s - underflow: h=%d height=%d", what, saved_h, g_status.Height)); }
	return x, y, w, h
}

func far2l_ReqRecvExpectString(str string, x int32, y int32, w int32, h int32, tmout uint32) far2l_FoundString {
	return far2l_ReqRecvExpectXStrings([]string{str}, x, y, w, h, tmout, true)
}

func far2l_ReqRecvExpectStrings(str_vec []string, x int32, y int32, w int32, h int32, tmout uint32) far2l_FoundString {
	return far2l_ReqRecvExpectXStrings(str_vec, x, y, w, h, tmout, true)
}

func far2l_ReqRecvExpectNoString(str string, x int32, y int32, w int32, h int32, tmout uint32) far2l_FoundString {
	return far2l_ReqRecvExpectXStrings([]string{str}, x, y, w, h, tmout, false)
}

func far2l_ReqRecvExpectNoStrings(str_vec []string, x int32, y int32, w int32, h int32, tmout uint32) far2l_FoundString {
	return far2l_ReqRecvExpectXStrings(str_vec, x, y, w, h, tmout, false)
}

func far2l_ReqRecvExpectXStrings(str_vec []string, x int32, y int32, w int32, h int32, tmout uint32, need_presence bool) far2l_FoundString {
	if tmout == 0 {
		tmout = g_default_expect_tmout
	}
	performAutoSync()
	if x < 0 || y < 0 || w <= 0 || h <= 0 {
		far2l_ReqRecvStatus()
	}
	x, y, w, h = prepareXYWH(x, y, w, h, "ReqRecvExpectXStrings")

	if (need_presence) {
		binary.LittleEndian.PutUint32(g_buf[0:], 3) //TEST_CMD_WAIT_STRING
	} else {
		binary.LittleEndian.PutUint32(g_buf[0:], 4) //TEST_CMD_WAIT_NO_STRING
	}
	binary.LittleEndian.PutUint32(g_buf[4:], tmout)
	binary.LittleEndian.PutUint32(g_buf[8:], uint32(x)) //left
	binary.LittleEndian.PutUint32(g_buf[12:], uint32(y)) //top
	binary.LittleEndian.PutUint32(g_buf[16:], uint32(w)) //width
	binary.LittleEndian.PutUint32(g_buf[20:], uint32(h)) //height
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
	if p >= far2lTestTextMax {
		aux_Panic("Too long strings")
	}
	for ; p < far2lTestTextMax; p++ {
		g_buf[24 + p] = 0
	}

	far2l_WriteToPeer(g_buf[0:far2lWaitStringPacketSize])
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
			log.Println(status)
		} else {
			setErrorString(status)
		}
	} else if out.I < uint32(len(str_vec)) {
		setErrorString(status)
	} else {
		log.Println(status)
	}
	return out
}

func far2l_ReqRecvReadCellRaw(x, y int32) far2l_CellRaw {
	performAutoSync()
	binary.LittleEndian.PutUint32(g_buf[0:], 2) // TEST_CMD_READ_CELL
	binary.LittleEndian.PutUint32(g_buf[4:], uint32(x)) // left
	binary.LittleEndian.PutUint32(g_buf[8:], uint32(y)) // top
	far2l_WriteToPeer(g_buf[0:12])
	far2l_ReadSocket(far2lReadCellPacketSize, 0)
	return far2l_CellRaw {
		Text: stringFromBytes(g_buf[8:]),
		Attributes: binary.LittleEndian.Uint64(g_buf[0:]),
	}
}

func far2l_ReqRecvReadCell(x, y int32) far2l_Cell {
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

func far2l_CheckBoundedLine(expected string, left, top, width int32, trim_chars string) string {
	line:= far2l_BoundedLine(left, top, width, trim_chars)
	if line != expected {
		setErrorString(fmt.Sprintf("Line at [%d +%d : %d] not expected: '%v'", left, width, top, line))
	}
	return line
}

func far2l_BoundedLine(left, top, width int32, trim_chars string) string {
	lines:= far2l_BoundedLines(left, top, width, 1, trim_chars)
	return lines[0]
}

func far2l_BoundedLines(left, top, width, height int32, trim_chars string) []string {
	left, top, width, height = prepareXYWH(left, top, width, height, "BoundedLines")
	return far2l_GetBoundedLines(left, top, width, height, trim_chars)
}

func far2l_GetBoundedLines(left, top, width, height int32, trim_chars string) []string {
	performAutoSync()
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
	log.Printf("Got %d lines at [%d +%d : %d +%d]\n", len(lines), left, width, top, height)
	return lines
}

func far2l_SurroundedLines(x, y int32, boundary_chars string, trim_chars string) []string {
	var left, top, width, height int32
	far2l_ReqRecvStatus()

	for left = x; left > 0 && !far2l_CellCharMatches(left - 1, y, boundary_chars); left-- {
	}

	for width = 1; left + width < int32(g_status.Width) && !far2l_CellCharMatches(left + width, y, boundary_chars); width++ {
	}

	// top & bottom edges has some quirks due to they may contains caption, hints, time etc..
	for top = y; top > 0 &&
		!far2l_CellCharMatches(left, top - 1, boundary_chars) &&
		!far2l_CellCharMatches(left + width / 2, top - 1, boundary_chars) &&
		!far2l_CellCharMatches(left + width - 1, top - 1, boundary_chars); top-- {
	}

	for height = 1; top + height < int32(g_status.Height) &&
		!far2l_CellCharMatches(left, top + height, boundary_chars) &&
		!far2l_CellCharMatches(left + width / 2, top + height, boundary_chars) &&
		!far2l_CellCharMatches(left + width - 1, top + height, boundary_chars); height++ {
	}

	return far2l_GetBoundedLines(left, top, width, height, trim_chars)
}

func far2l_CellCharMatches(x, y int32, chars string) bool {
	cell := far2l_ReqRecvReadCellRaw(x, y)
	return cell.Text != "" && strings.Contains(chars, cell.Text)
}

func far2l_CheckCellChar(x, y int32, chars string) string {
	cell := far2l_ReqRecvReadCellRaw(x, y)
	if cell.Text == "" || !strings.Contains(chars, cell.Text) {
		setErrorString(fmt.Sprintf("Cell at %d:%d = '%s' doesnt represent any of: '%s'", x, y, cell.Text, chars))
	}
	return cell.Text
}

func far2l_BoundedLinesMatchTextFile(left, top, width, height int32, fpath string) bool {
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

func far2l_BoundedLinesSaveAsTextFile(left, top, width, height int32, fpath string) {
	screen_lines:= far2l_BoundedLines(left, top, width, height, "")
	aux_SaveTextFile(fpath, screen_lines)
}


func far2l_ExpectExit(code int, tmout int) string {
	if tmout == 0 {
		tmout = int(g_default_expect_tmout)
	}
	far2l_ReqBye()

	ctx, cancel := context.WithTimeout(context.Background(), time.Duration(tmout) * time.Millisecond)
	defer func() {
		if (g_app != nil) {
			app:= g_app
			g_app = nil
			app.Close()
		}
		cancel() 
	}()
	select {
		case result:= <-g_channel:
			if result != 0 {
				setErrorString(fmt.Sprintf("ExpectExit: ERROR %d", result))
				return fmt.Sprintf("ERROR: %d", result)
			}
		case <-ctx.Done():
			setErrorString(fmt.Sprintf("ExpectExit: TIMEOUT"))
			return "ERROR: TIMEOUT"
	}
	log.Println("ExpectExit: DONE")
	return ""
}

func far2l_SetDefaultExpectTimeout(tmout uint32) {
	g_default_expect_tmout = tmout
}
