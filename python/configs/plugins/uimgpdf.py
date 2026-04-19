import os
import logging
import debugpy

from far2l.plugin import PluginBase
from far2l.fardialogbuilder import (
    Spacer,
    TEXT,
    BUTTON,
    USERCONTROL,
    HLine,
    HSizer,
    VSizer,
    DialogBuilder,
)
from far2l.far2leditor import Editor
import far2lc

log = logging.getLogger(__name__)

try:
    from pdfium.pdfium import PDFium
except:
    log.debug('PDFium load failed')
    PDFium = None

# // capabilities reported by GetConsoleImageCaps
WP_IMGCAP_RGBA          =0x001 # supports WP_IMG_RGB/WP_IMG_RGBA
WP_IMGCAP_PNG           =0x002 # supports WP_IMG_PNG
WP_IMGCAP_JPG           =0x003 # supports WP_IMG_JPG
WP_IMGCAP_ATTACH        =0x100 # supports existing image attaching
WP_IMGCAP_SCROLL        =0x200 # supports existing image scrolling
# // reserved for a while:    0x400
WP_IMGCAP_ROTMIR        =0x800 # supports existing image rotation and mirroring

# // flags used for SetConsoleImage
WP_IMG_RGBA             =0 # supported if WP_IMGCAP_RGBA
WP_IMG_RGB              =1 # supported if WP_IMGCAP_RGBA
WP_IMG_PNG              =2 # supported if WP_IMGCAP_PNG
WP_IMG_JPG              =3 # supported if WP_IMGCAP_JPG

# // SetConsoleImage attaching flags supported if WP_IMGCAP_ATTACH reported
WP_IMG_ATTACH_LEFT      =0x010000 # attach given image at left edge of existing one
WP_IMG_ATTACH_RIGHT     =0x020000 # attach given image at right edge of existing one
WP_IMG_ATTACH_TOP       =0x030000 # attach given image at top edge of existing one
WP_IMG_ATTACH_BOTTOM    =0x040000 # attach given image at bottom edge of existing one

# // Can be used only with any of WP_IMG_ATTACH_* if WP_IMGCAP_SCROLL reported
# // Scrolls image after attaching to direction opposite to attached edge
WP_IMG_SCROLL           =0x080000


# // if area fully specified - then:
# //  if WP_IMG_PIXEL_OFFSET is not set - image scaled to cover specified area [LEFT TOP RIGHT BOTTOM]
# //  if WP_IMG_PIXEL_OFFSET is set - image NOT scaled, and RIGHT and BOTTOM fields treated as pixel-level offset for displaying image
WP_IMG_PIXEL_OFFSET     =0x100000


WP_IMG_MASK_FMT         =0x00ffff
WP_IMG_MASK_ATTACH      =0x070000

# // WP_IMGTF_ROTATE_* supported if WP_IMGCAP_ROTMIR reported occupy least
WP_IMGTF_MASK_ROTATE    =0x03 # 2 bits that can be one of given values:
WP_IMGTF_ROTATE0        =0x00 # no rotation (so can use it just to move image)
WP_IMGTF_ROTATE90       =0x01 # rotate by 90 degrees
WP_IMGTF_ROTATE180      =0x02 # rotate by 180 degrees
WP_IMGTF_ROTATE270      =0x03 # rotate by 270 degrees

# // WP_IMG_MIRROR_* supported if WP_IMGCAP_ROTMIR reported and independent bit values
# // note that mirroring applied before rotation, if specified together
WP_IMGTF_MIRROR_H       =0x04  # flip image horizontally
WP_IMGTF_MIRROR_V       =0x08  # flip image vertically


class Plugin(PluginBase):
    label = "Python PDF view"
    openFrom = ["PLUGINSMENU", "COMMANDLINE", "VIEWER"]

    def adv(self, cmd, par):
        self.info.AdvControl(
            self.info.ModuleNumber,
            cmd,
            self.ffi.cast("void *", par),
            self.ffi.NULL
        )

    def GetFarRect(self):
        data = self.ffi.new("SMALL_RECT *")
        self.adv(self.ffic.ACTL_GETFARRECT, data)
        return data

    @staticmethod
    def OpenFilePlugin(parent, info, ffi, ffic, Name, Data, DataSize, OpMode):
        name = ffi.string(ffi.cast("wchar_t *", Name))
        try:
            img = Image.open(name)
        except:
            log.debug(f"unsupported image type fqname=%s", fqname)
            return False
        self = Plugin(parent, info, ffi, ffic)
        try:
            self.View(name, img)
        finally:
            return False

    def OpenPlugin(self, OpenFrom):
        if OpenFrom == 5:
            fqname = Editor(self).GetFileName()
        elif OpenFrom == 6:
            data = self.ffi.new("struct ViewerInfo *")
            data.StructSize = self.ffi.sizeof("struct ViewerInfo")
            self.info.ViewerControl(self.ffic.VCTL_GETINFO, data)
            fqname = self.f2s(data.FileName)
        elif OpenFrom == 1:
            pnli, pnlidata = self.panel.GetCurrentPanelItem()
            fname = self.f2s(pnli.FindData.lpwszFileName)
            fqname = os.path.join(self.panel.GetPanelDir(), fname)
        else:
            log.debug(f"unsupported open from {OpenFrom}")
            return

        try:
            self.View(fqname)
        except Exception:
            log.exception('uimgpdf')
        finally:
            return

    def PageToImage(self, doc, canvas_w, canvas_h, pageno):
        page = doc.Page(pageno)
        width = int(page.width())
        height = int(page.height())
        log.debug('page: %d size: %dx%d', pageno, width, height)
        try:
            #data = page.thumbnail(nbytes=False, raw=True, bitmap=False)
            #if data[1] is None:
            #    log.debug('no thumbnail')
            bmp = page.render(0, 0, width, height, 0, 0)
            try:
                img = bmp.image()
                res = img.resize((canvas_w, canvas_h))
                rgb_img = res.convert('RGB')
                rgb_img_bytes = rgb_img.tobytes()
                rgb_img_buf = self.ffi.new('unsigned char[]', rgb_img_bytes)
                return rgb_img_buf
            finally:
                bmp.close()
        finally:
            page.close()

    def View(self, fqname):
        winport = self.ffi.cast("struct WINPORTDECL *", far2lc.WINPORT())
        wgi = self.ffi.new("WinportGraphicsInfo *")
        ok = winport.GetConsoleImageCaps(self.ffi.NULL, self.ffi.sizeof("WinportGraphicsInfo"), wgi)
        if not ok:
            log.debug('GetConsoleImageCaps failed')
            return
        if (wgi.Caps & WP_IMGCAP_RGBA) == 0:
            log.debug("backend doesn't support graphics")
            return
        if wgi.PixPerCell.X <= 0 or wgi.PixPerCell.Y <= 0:
            log.debug("bad cell size ( %d x %d )", wgi.PixPerCell.X, wgi.PixPerCell.Y)
            return
        log.debug("backend cell size %dx%d and caps:0x%08X", wgi.PixPerCell.X, wgi.PixPerCell.Y, wgi.Caps)

        WINPORT_IMAGE_ID = self.ffi.new('char[]', b"uimage")

        try:
            dll = PDFium()
        except Exception:
            log.traceback('PDFium create failed')
        password = self.ffi.NULL
        log.debug('File: %s', fqname)
        try:
            doc = dll.document(fqname.encode(), password)
        except IOError as ex:
            log.error('Error: %s [%s]', ex.errno, ex.args)
            return

        #debugpy.breakpoint()
        area = self.GetFarRect()
        imgarea = self.ffi.new("SMALL_RECT *", (0, 0, area.Right, area.Bottom-1))
        canvas_w = imgarea.Right * wgi.PixPerCell.X
        canvas_h = imgarea.Bottom * wgi.PixPerCell.Y

        self.pageno = -1
        self.rgb_img_buf = None
        self.firsttime = True

        def ShowImage():
            if not self.firsttime:
                winport.DeleteConsoleImage(self.ffi.NULL, WINPORT_IMAGE_ID)
            self.firsttime = False
            rc = winport.SetConsoleImage(
                self.ffi.NULL,
                WINPORT_IMAGE_ID,
                WP_IMG_RGB,
                imgarea,
                canvas_w,
                canvas_h,
                self.rgb_img_buf
            )
            log.debug(f'ShowImage:{rc}')

        def DialogProc(hDlg, Msg, Param1, Param2):
            if Msg == self.ffic.DN_ENTERIDLE:
                log.debug(f'idle:{self.pageno}')
                if self.pageno < 0:
                    self.pageno = -self.pageno
                    self.rgb_img_buf = self.PageToImage(doc, canvas_w, canvas_h, self.pageno-1)
                    ShowImage()
            elif Msg == self.ffic.DN_INITDIALOG:
                dlg.SetFocus(dlg.ID_image)
                log.debug("initdialog")
            elif Msg == self.ffic.DN_CLOSE:
                winport.DeleteConsoleImage(self.ffi.NULL, WINPORT_IMAGE_ID)
                log.debug("close")
            elif Msg == self.ffic.DN_BTNCLICK:
                pass
            elif Msg == self.ffic.DN_KEY:
                log.debug("key down")
                if Param2 == self.ffic.KEY_LEFT:
                    pass
                elif Param2 == self.ffic.KEY_UP:
                    pass
                elif Param2 == self.ffic.KEY_RIGHT:
                    pass
                elif Param2 == self.ffic.KEY_DOWN:
                    pass
                elif Param2 == self.ffic.KEY_ENTER:
                    pass
                elif Param2 == self.ffic.KEY_ESC:
                    pass
                elif Param2 == self.ffic.KEY_PGUP:
                    pageno = self.pageno
                    if pageno > 1:
                        pageno -= 1
                    else:
                        pageno = doc.PageCount()
                    self.pageno = -pageno
                    log.debug(f'pgup pageno:{self.pageno}')
                    return 1
                elif Param2 == self.ffic.KEY_PGDN:
                    pageno = self.pageno+1
                    if pageno > doc.PageCount():
                        pageno = 1
                    self.pageno = -pageno
                    log.debug(f'pgdn pageno:{self.pageno}')
                    return 1
            elif Msg == self.ffic.DN_MOUSECLICK:
                #if Param1 == dlg.ID_vlist:
                #    return 1
                pass
            elif Msg == self.ffic.DN_MOUSEEVENT:
                #MOUSE_EVENT_RECORD *me = (const MOUSE_EVENT_RECORD *)Param2;
                #(me->dwButtonState & RIGHTMOST_BUTTON_PRESSED) != 0 && (me->dwEventFlags & MOUSE_MOVED)  == 0
                #(me->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0
                return 1
            elif Msg == self.ffic.DN_RESIZECONSOLE:
                self.info.SendDlgMessage(hDlg, self.ffic.DM_CLOSE, 999, 0)
            elif Msg == self.ffic.DN_DRAGGED:
                return 0
            return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        @self.ffi.callback("FARWINDOWPROC")
        def _DialogProc(hDlg, Msg, Param1, Param2):
            try:
                return DialogProc(hDlg, Msg, Param1, Param2)
            except:
                log.exception('dialogproc')
                return self.info.DefDlgProc(hDlg, Msg, Param1, Param2)

        while True:
            area = self.GetFarRect()
            log.debug("area L:%d T:%d R:%d B:%d", area.Left, area.Top, area.Right, area.Bottom)

            b = DialogBuilder(
                self,
                _DialogProc,
                "Python image",
                "helptopic",
                self.ffic.FDLG_NODRAWSHADOW|self.ffic.FDLG_NODRAWPANEL,
                VSizer(
                    USERCONTROL('image', area.Right+1, area.Bottom),
                    HSizer(
                        BUTTON("vok", "OK", default=True, flags=self.ffic.DIF_CENTERGROUP),
                        BUTTON("vcancel", "Cancel", flags=self.ffic.DIF_CENTERGROUP),
                    ),
                ),
                (0, 0, 0, 0)
            )
            dlg = b.build_nobox(0, 0, area.Right+1, area.Bottom)

            res = self.info.DialogRun(dlg.hDlg)
            self.info.DialogFree(dlg.hDlg)
            if res != 999:
                break
