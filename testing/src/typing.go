package main

import (
	"encoding/binary"
)

var g_lctrl bool
var g_rctrl bool
var g_lalt bool
var g_ralt bool
var g_shift bool

func typingReset() {
	g_lctrl = false
	g_rctrl = false
	g_lalt = false
	g_ralt = false
	g_shift = false
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
	far2l_WriteToPeer(g_buf[0:24])
}

