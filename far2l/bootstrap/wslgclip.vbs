Option Explicit

' Получить объект ClipboardData
Dim objClipboardData
Set objClipboardData = CreateObject("htmlfile").ParentWindow.ClipboardData

' Получить данные из буфера обмена как массив байтов в кодировке UTF-16LE
Dim strData
strData = objClipboardData.GetData("Text")
Dim arrBytes: arrBytes = StrConv(strData, vbFromUnicode)

' Преобразовать массив байтов из UTF-16LE в UTF-8
Dim objStream, strUtf8
Set objStream = CreateObject("ADODB.Stream")
objStream.Type = 1 ' adTypeBinary
objStream.Open
objStream.Write arrBytes
objStream.Position = 0
objStream.Type = 2 ' adTypeText
objStream.Charset = "UTF-8"
strUtf8 = objStream.ReadText
objStream.Close
Set objStream = Nothing

' Вывести данные в кодировке UTF-8
WScript.StdOut.Write strUtf8
