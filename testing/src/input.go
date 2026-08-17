package main

import (
	"encoding/binary"
	"log"
)

const RIGHT_ALT_PRESSED     = 0x0001 // the right alt key is pressed.
const LEFT_ALT_PRESSED      = 0x0002 // the left alt key is pressed.
const RIGHT_CTRL_PRESSED    = 0x0004 // the right ctrl key is pressed.
const LEFT_CTRL_PRESSED     = 0x0008 // the left ctrl key is pressed.
const SHIFT_PRESSED         = 0x0010 // the shift key is pressed.
const NUMLOCK_ON            = 0x0020 // the numlock light is on.
const SCROLLLOCK_ON         = 0x0040 // the scrolllock light is on.
const CAPSLOCK_ON           = 0x0080 // the capslock light is on.
const ENHANCED_KEY          = 0x0100 // the key is enhanced.

const FROM_LEFT_1ST_BUTTON_PRESSED    = 0x0001
const RIGHTMOST_BUTTON_PRESSED        = 0x0002
const FROM_LEFT_2ND_BUTTON_PRESSED    = 0x0004
const FROM_LEFT_3RD_BUTTON_PRESSED    = 0x0008
const FROM_LEFT_4TH_BUTTON_PRESSED    = 0x0010

const MOUSE_MOVED    = 0x0001
const DOUBLE_CLICK   = 0x0002
const MOUSE_WHEELED  = 0x0004
const MOUSE_HWHEELED = 0x0008

var g_controls uint32

func typingReset() {
	g_controls = 0
}

func tty_Write(s string) {
    g_app.Send(s)
}

func tty_CtrlC() {
    g_app.SendCtrlC()
}

func toggleControl(pressed bool, what uint32) {
	if pressed {
		g_controls = g_controls | what
	} else {
		g_controls = g_controls & (what ^ 0xffffffff)
	}
}

func far2l_ToggleShift(pressed bool) {
	toggleControl(pressed, SHIFT_PRESSED)
	far2l_SendKeyEvent(0, 0x10, pressed)
}

func far2l_ToggleLCtrl(pressed bool) {
	toggleControl(pressed, LEFT_CTRL_PRESSED)
	far2l_SendKeyEvent(0, 0x11, pressed)
}

func far2l_ToggleRCtrl(pressed bool) {
	toggleControl(pressed, RIGHT_CTRL_PRESSED)
	far2l_SendKeyEvent(0, 0x11, pressed)
}

func far2l_ToggleLAlt(pressed bool) {
	toggleControl(pressed, LEFT_ALT_PRESSED)
	far2l_SendKeyEvent(0, 0x12, pressed)
}

func far2l_ToggleRAlt(pressed bool) {
	toggleControl(pressed, RIGHT_ALT_PRESSED)
	far2l_SendKeyEvent(0, 0x12, pressed)
}

func far2l_TypeFKey(n uint32) { far2l_TypeVK(0x6F + n, 1) }
func far2l_TypeDigit(n uint32) { far2l_TypeVK(0x60 + n, 1) }

func far2l_TypeAdd()      { far2l_TypeVK(0x6B, 1) }
func far2l_TypeSub()      { far2l_TypeVK(0x6D, 1) }
func far2l_TypeMul()      { far2l_TypeVK(0x6A, 1) }
func far2l_TypeDiv()      { far2l_TypeVK(0x6F, 1) }
func far2l_TypeSeparator(){ far2l_TypeVK(0x6C, 1) }
func far2l_TypeDecimal()  { far2l_TypeVK(0x6E, 1) }

func far2l_TypeEscape()   { far2l_TypeVK(0x1B, 1) }
func far2l_TypeEnd()      { far2l_TypeVK(0x23, 1) }
func far2l_TypeHome()     { far2l_TypeVK(0x24, 1) }
func far2l_TypeIns()      { far2l_TypeVK(0x2D, 1) }
func far2l_TypeDel()      { far2l_TypeVK(0x2E, 1) }
func far2l_TypeEnter()    { far2l_TypeVK(0x0D, 1) }
func far2l_TypeTab(count int)      { far2l_TypeVK(0x09, count) }
func far2l_TypeBack(count int)     { far2l_TypeVK(0x08, count) }
func far2l_TypePageUp(count int)   { far2l_TypeVK(0x21, count) }
func far2l_TypePageDown(count int) { far2l_TypeVK(0x22, count) }
func far2l_TypeLeft(count int)     { far2l_TypeVK(0x25, count) }
func far2l_TypeUp(count int)       { far2l_TypeVK(0x26, count) }
func far2l_TypeRight(count int)    { far2l_TypeVK(0x27, count) }
func far2l_TypeDown(count int)     { far2l_TypeVK(0x28, count) }


func far2l_TypeVK(key_code uint32, count int) {
	if count <= 1 {
		count = 1;
		log.Println("TypeVK:", key_code)
	} else {
		log.Println("TypeVK:", key_code, "(", count, "times)")
	}
	for ;count > 0; count-- {
		far2l_SendKeyEvent(0, key_code, true)
		far2l_SendKeyEvent(0, key_code, false)
	}
}

func far2l_TypeText(text string) {
	log.Println("TypeText:", text)
    for _, r := range text {
		far2l_SendKeyEvent(uint32(r), 0, true)
		far2l_SendKeyEvent(uint32(r), 0, false)
    }
}

func far2l_SendKeyEvent(utf32_code uint32, key_code uint32, pressed bool) {
	if key_code == 0 && utf32_code != 0 {
		if utf32_code >= 'a' && utf32_code <= 'z' {
			key_code = 'A' + (utf32_code - 'a')
		} else if utf32_code == '.' {
			key_code = 0xBE
		} else if utf32_code == ',' {
			key_code = 0xBC
		} else if utf32_code == '+' {
			key_code = 0xBB
		} else if utf32_code == '_' || utf32_code == '-' {
			key_code = 0xBD
		} else if utf32_code == ';' || utf32_code == ':' {
			key_code = 0xBA
		} else if utf32_code == '/' || utf32_code == '?' {
			key_code = 0xBF
		} else if utf32_code == '~' || utf32_code == '`' {
			key_code = 0xC0
		} else if utf32_code == '[' || utf32_code == '{' {
			key_code = 0xDB
		} else if utf32_code == ']' || utf32_code == '}' {
			key_code = 0xDD
		} else if utf32_code == '\\' || utf32_code == '|' {
			key_code = 0xDC
		} else if utf32_code == '\'' || utf32_code == '"' {
			key_code = 0xDE
		} else if (utf32_code <= 0x7f) {
			key_code = utf32_code
		}
	}

	binary.LittleEndian.PutUint32(g_buf[0:], 5) // TEST_CMD_SEND_KEY
	binary.LittleEndian.PutUint32(g_buf[4:], g_controls)
	binary.LittleEndian.PutUint32(g_buf[8:], utf32_code)
	binary.LittleEndian.PutUint32(g_buf[12:], key_code)
	binary.LittleEndian.PutUint32(g_buf[16:], 0)
	binary.LittleEndian.PutUint32(g_buf[20:], 0)
	if pressed { g_buf[20] = 1 }
	far2l_WriteToPeer(g_buf[0:24])
	scheduleAutoSync()
}

/////////////

func far2l_LClickWhereFound(where far2l_FoundString) {
	far2l_LClick(where.X, where.Y)
}

func far2l_RClickWhereFound(where far2l_FoundString) {
	far2l_RClick(where.X, where.Y)
}

func far2l_DblClickWhereFound(where far2l_FoundString) {
	far2l_DblClick(where.X, where.Y)
}

func far2l_LClick(x, y uint32) {
	log.Println("LClick at", x, y)
	far2l_SendMouseEvent(x, y, FROM_LEFT_1ST_BUTTON_PRESSED, 0)
	far2l_SendMouseEvent(x, y, 0, 0)
}

func far2l_RClick(x, y uint32) {
	log.Println("RClick at", x, y)
	far2l_SendMouseEvent(x, y, RIGHTMOST_BUTTON_PRESSED, 0)
	far2l_SendMouseEvent(x, y, 0, 0)
}

func far2l_DblClick(x, y uint32) {
	log.Println("DblClick at", x, y)
	far2l_SendMouseEvent(x, y, FROM_LEFT_1ST_BUTTON_PRESSED, DOUBLE_CLICK)
	far2l_SendMouseEvent(x, y, 0, 0)
}


func far2l_SendMouseEvent(x, y, btn, flags uint32) {
	binary.LittleEndian.PutUint32(g_buf[0:], 7) // TEST_CMD_SEND_MOUSE
	binary.LittleEndian.PutUint32(g_buf[4:], flags)
	binary.LittleEndian.PutUint32(g_buf[8:], g_controls)
	binary.LittleEndian.PutUint32(g_buf[12:], btn)
	binary.LittleEndian.PutUint32(g_buf[16:], x)
	binary.LittleEndian.PutUint32(g_buf[20:], y)
	far2l_WriteToPeer(g_buf[0:24])
	scheduleAutoSync()
}
