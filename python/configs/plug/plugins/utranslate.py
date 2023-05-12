import shlex
import logging
import optparse
from far2l.plugin import PluginBase
from far2l import fardialogbuilder as dlgb
from far2lpackages import googletranslate

log = logging.getLogger(__name__)


class OptionParser(optparse.OptionParser):
    def error(self, msg):
        log.error("utranslate: {}".format(msg))
        raise ValueError("bad option")


class Plugin(PluginBase):
    label = "Python utranslate"
    openFrom = ["PLUGINSMENU", "EDITOR"]

    @staticmethod
    def HandleCommandLine(line):
        return line in ("translate", "spell")

    def CommandLine(self, line):
        cmd, line = line.split(" ", 1)
        if cmd == "spell":
            args = shlex.split(line)
            parser = OptionParser()
            parser.add_option("--lang", action="store", dest="lang", default="en")
            parser.add_option("--text", action="store", dest="text", default="en")
            try:
                opts, args = parser.parse_args(args)
            except ValueError:
                return
            log.debug("spell: opts={} args={}".format(opts, args))
            try:
                t = googletranslate.get_translate(opts.text, opts.lang, opts.lang)
            except:
                log.exception("spell")
                return
            log.debug("spell result: {}".format(t))
        elif cmd == "translate":
            args = shlex.split(line)
            parser = optparse.OptionParser()
            parser.add_option("--from", action="store", dest="lfrom", default="en")
            parser.add_option("--to", action="store", dest="lto", default="en")
            parser.add_option("--text", action="store", dest="text", default="")
            try:
                opts, args = parser.parse_args(args)
            except ValueError:
                return
            log.debug("translate: opts={} args={}".format(opts, args))
            try:
                t = googletranslate.get_translate(opts.text, opts.lfrom, opts.lto)
            except:
                log.exception("translate")
                return
            log.debug("translate result: {}".format(t))
        else:
            log.error("bad command:{} line={}".format(cmd, line))
            return
        s = t["result"][0]
        self.info.FSF.CopyToClipboard(self.s2f(s))

    def Dialog(self):
        @self.ffi.callback("FARWINDOWPROC")
        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_INITDIALOG:
                try:
                    s = self.info.FSF.PasteFromClipboard()
                    s = self.f2s(s)
                except:
                    s = "And do beautiful things"
                try:
                    dlg.SetText(dlg.ID_text, s)
                    dlg.SetCheck(dlg.ID_spell, 1)
                    dlg.SetText(dlg.ID_from, "en")
                    dlg.SetText(dlg.ID_to, "pl")
                    dlg.SetFocus(dlg.ID_text)
                except:
                    log.exception("Init Dialog")
            elif Msg == self.ffic.DN_BTNCLICK:
                if dlg.ID_perform == Param1:
                    text = dlg.GetText(dlg.ID_text)
                    spell = dlg.GetCheck(dlg.ID_spell)
                    translate = dlg.GetCheck(dlg.ID_translate)
                    lfrom = dlg.GetText(dlg.ID_from)
                    lto = dlg.GetText(dlg.ID_to)
                    log.debug(
                        "DN_BTNCLICK(spell={}, translate={} nfrom={} nto={} text={})".format(
                            spell, translate, lfrom, lto, text
                        )
                    )
                    try:
                        if spell:
                            t = googletranslate.get_translate(text, lfrom, lfrom)
                        else:
                            t = googletranslate.get_translate(text, lfrom, lto)
                        log.debug("translate result: {}".format(t))
                        s = t["result"][0]
                        self.info.FSF.CopyToClipboard(self.s2f(s))
                    except:
                        log.exception("translate")
                        s = "connection error"
                    dlg.SetText(dlg.ID_result, s)
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        b = dlgb.DialogBuilder(
            self,
            DialogProc,
            "Python utranslate",
            "helptopic",
            0,
            dlgb.VSizer(
                dlgb.HSizer(
                    dlgb.TEXT(None, "Text:"),
                    dlgb.Spacer(),
                    dlgb.EDIT("text", 60, maxlength=120),
                ),
                dlgb.HSizer(
                    dlgb.TEXT(None, "Operation:"),
                    dlgb.Spacer(),
                    dlgb.RADIOBUTTON("spell", "Spell", True, flags=self.ffic.DIF_GROUP),
                    dlgb.Spacer(),
                    dlgb.RADIOBUTTON("translate", "Translate"),
                ),
                dlgb.HSizer(
                    dlgb.TEXT(None, "From:"),
                    dlgb.Spacer(),
                    dlgb.EDIT("from", 4, maxlength=4),
                ),
                dlgb.HSizer(
                    dlgb.TEXT(None, "To:"),
                    dlgb.Spacer(),
                    dlgb.EDIT("to", 4, maxlength=4),
                ),
                dlgb.HLine(),
                dlgb.HSizer(
                    dlgb.TEXT(None, "Result:"),
                    dlgb.Spacer(),
                    dlgb.EDIT("result", 60, maxlength=120),
                ),
                dlgb.HLine(),
                dlgb.HSizer(
                    dlgb.BUTTON(
                        "perform",
                        "Perform",
                        default=True,
                        flags=self.ffic.DIF_CENTERGROUP | self.ffic.DIF_BTNNOCLOSE,
                    ),
                    dlgb.BUTTON("close", "Close", flags=self.ffic.DIF_CENTERGROUP),
                ),
            ),
        )
        dlg = b.build(-1, -1)

        res = self.info.DialogRun(dlg.hDlg)
        log.debug("translate res={0}".format(res))

        self.info.DialogFree(dlg.hDlg)

    def OpenPlugin(self, OpenFrom):
        if OpenFrom == 5:
            # EDITOR
            self.Dialog()
            return -1
        _MsgItems = [
            self.s2f("Python Translate/Spellcheck from="),
            self.s2f(""),
            self.s2f("From shell:"),
            self.s2f(
                "  py:translate --from=en --to=pl --text='And do beautiful things'"
            ),
            self.s2f("  py:spell --lang=en --text='And do beautifull things'"),
            self.s2f("From editor:"),
            self.s2f("  copy text to clipboard, execute plugin"),
            self.s2f(""),
            self.s2f("Result is in clipboard :)"),
            self.s2f(""),
            self.s2f("\x01"),
            self.s2f("&Ok"),
        ]
        MsgItems = self.ffi.new("wchar_t *[]", _MsgItems)
        self.info.Message(
            self.info.ModuleNumber,  # GUID
            self.ffic.FMSG_WARNING | self.ffic.FMSG_LEFTALIGN,  # Flags
            self.s2f("Contents"),  # HelpTopic
            MsgItems,  # Items
            len(MsgItems),  # ItemsNumber
            1,  # ButtonsNumber
        )
        return -1
