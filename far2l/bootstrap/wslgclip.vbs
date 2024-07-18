Set objHTML = CreateObject("HTMLFile")
clipboardText = objHTML.ParentWindow.ClipboardData.GetData("Text")

Set stream = CreateObject("ADODB.Stream")
stream.Type = 2
stream.Mode = 3
stream.Charset = "utf-8"
stream.Open
stream.WriteText clipboardText
stream.Position = 0

stream.Charset = "utf-8"
utf8Text = stream.ReadText

stream.Close
Set stream = Nothing

WScript.StdOut.Write utf8Text
