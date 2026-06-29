#include "crest.h"
#include <windows.h>
#include <stddef.h>

enum CrestLanguage {
    lngOk = 0,
    lngCancel,
    lngEnabled,
    lngCLRCursor,
    lngCRColor,
    lngCURColor,
    lngRulerColor,
    lngActivs,
    lngCtrl,
    lngAlt,
    lngShift,
    lngLock,
    lngCaps,
    lngNum,
    lngScroll,
    lngOpt,
    lngColorerCor,
    lngDrawV,
    lngDrawH,
    lngShCfg,
    lngEditorCrestSetup,
    lngShowRuler
};

#define MAX_WORD                  ((WORD)0xFFFFU)
#define IS_FLAG( val,flag )       (((val)&(flag))==(flag))
#define SET_FLAG( val,flag )      (val |= (flag))

#define zero_mem(p,sz) memset((p),0,(sz))
#define _tstrcpy(a, b) lstrcpy((a), (b))

#include "farplug-wide.h"
#include "farcolor.h"

#define CR_VERSION        _T("Crest/2l v1.11")

#define CRTS_CONTROL   0x0001
#define CRTS_ALT       0x0002
#define CRTS_SHIFT     0x0004

#define CRLS_CAPS      0x0001
#define CRLS_NUM       0x0002
#define CRLS_SCROLL    0x0004

#define CRF_COLORER    0x0001
#define CRF_VERT       0x0002
#define CRF_HORZ       0x0004
#define CRF_CURSOR     0x0008
#define CRF_PLUGINMENU 0x0010
#define CRF_SHOWRULER  0x0200

//- Variables
//FAR
static struct PluginStartupInfo    Info;
FARSTDSNPRINTF apiSnprintf;
FARSTDATOI     apiAtoi;

static struct EditorInfo EInfo;

//Last used positions
static struct LastPos{
    int   curX;
    int   curY;
    int   hilight;
    int   isHilight;
    int   TopLine;
    WORD  LastFlags;
} last;

static CROptions Options;
//- Options getters

#define isColorer()     (Options.Flags&CRF_COLORER)
#define isCursor()      (Options.Flags&CRF_CURSOR)
#define isPluginMenu()  (Options.Flags&CRF_PLUGINMENU)
#define isCrest()       (Options.Flags&(CRF_VERT|CRF_HORZ))
#define isShowRuler()   (Options.Flags&CRF_SHOWRULER)

static const TCHAR szDecimalFmt  []=_T("%d");

static BOOL GetModifierKeyState(const KEY_EVENT_RECORD  * KeyEvent, DWORD vk)
{
    return KeyEvent->dwControlKeyState & ((vk == VK_CONTROL) ? (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED) :
                                          (vk == VK_MENU) ? (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED) :
                                          (vk == VK_SHIFT) ? SHIFT_PRESSED : 0);
}

__inline static BOOL isTempShow( const KEY_EVENT_RECORD  * KeyEvent )
{
    if ( (Options.TempShow&(CRTS_CONTROL|CRTS_ALT|CRTS_SHIFT)) == 0 ) return FALSE;
    if ( IS_FLAG(Options.TempShow,CRTS_CONTROL) && GetModifierKeyState(KeyEvent, VK_CONTROL) ) return TRUE;
    if ( IS_FLAG(Options.TempShow,CRTS_ALT)     && GetModifierKeyState(KeyEvent, VK_MENU)    ) return TRUE;
    if ( IS_FLAG(Options.TempShow,CRTS_SHIFT)   && GetModifierKeyState(KeyEvent, VK_SHIFT)   ) return TRUE;
    return FALSE;
}

static BOOL GetLockKeyState(const KEY_EVENT_RECORD  * KeyEvent, DWORD vk)
{
    return KeyEvent->dwControlKeyState & ((vk == VK_CAPITAL) ? CAPSLOCK_ON:
                                          (vk == VK_NUMLOCK) ? NUMLOCK_ON :
                                          (vk == VK_SCROLL)  ? SCROLLLOCK_ON : 0);
}

__inline static BOOL isLockShow( const KEY_EVENT_RECORD  * KeyEvent )
{
    if ( (IS_FLAG(Options.LockShow,CRLS_CAPS)   && GetLockKeyState( KeyEvent, VK_CAPITAL )) ||
         (IS_FLAG(Options.LockShow,CRLS_NUM)    && GetLockKeyState( KeyEvent, VK_NUMLOCK )) ||
         (IS_FLAG(Options.LockShow,CRLS_SCROLL) && GetLockKeyState( KeyEvent, VK_SCROLL )) ) {
        return TRUE;
    }
    return FALSE;
}

static BOOL isShow( const INPUT_RECORD * ir )
{
    // Постоянно прилетают странные KEY_EVENT у которых все остальные поля нулевые,
    // включая dwControlKeyState.
    if (!ir || ir->EventType != KEY_EVENT || !ir->Event.KeyEvent.wVirtualKeyCode)
        return last.isHilight;

    return isTempShow(&ir->Event.KeyEvent) || isLockShow(&ir->Event.KeyEvent);
}

static void DoCursor( int x, int y, int color )
{
     struct EditorColor ec;

     if ( Options.CenterColor ){
        ec.StringNumber=y;
        ec.ColorItem=0;
        ec.StartPos=x;
        ec.EndPos=x;
        ec.Color=color;
        Info.EditorControl( ECTL_ADDCOLOR,&ec );
     }
}

__inline static void EditSetCursor(void)
{
    DoCursor(EInfo.CurPos,EInfo.CurLine,Options.CenterColor);
}

__inline static void DelCursor( int x,int y )
{
    DoCursor(x,y,0);
}

static void DoColorize(int x, int y, BOOL isSet, WORD wFlags )
{
    int n;
    struct EditorColor ec;
    struct EditorConvertPos ecp;
    int tab_x;

    zero_mem(&ec,sizeof(ec));
    ec.StringNumber=y;
    if( isSet ){
        ec.StartPos=EInfo.LeftPos;
        ec.EndPos=EInfo.LeftPos+EInfo.WindowSizeX;
        ec.Color=Options.Color;
    }else
        ec.StartPos=-1;

//Prepare to correct TABs
    ecp.StringNumber=y;
    ecp.SrcPos=x;
    Info.EditorControl(ECTL_REALTOTAB,&ecp);
    tab_x=ecp.DestPos;
//Horizontal
    if ( IS_FLAG(wFlags,CRF_HORZ) )
        Info.EditorControl( ECTL_ADDCOLOR,&ec );
//Vertical
    if ( IS_FLAG(Options.Flags,CRF_VERT) ) {
        for ( n = 0; n < EInfo.WindowSizeY; n++ ) {
             ecp.StringNumber =
             ec.StringNumber = last.TopLine + n;
             ecp.SrcPos = tab_x;
             Info.EditorControl(ECTL_TABTOREAL,&ecp);
             ec.StartPos     =
             ec.EndPos       = ecp.DestPos;
             Info.EditorControl( ECTL_ADDCOLOR,&ec );
       }
    }
//Center
    if ( (wFlags&(CRF_HORZ|CRF_VERT))==0 )
        DoCursor(x,y,isSet?Options.CenterColor:0);
}

static void Ruler()
{
    TCHAR buff[11];
    int i;
    static const TCHAR sz4dot[]=_T("....+....|");

    for( i=0; i<EInfo.WindowSizeX; i+=10 ){
        _tstrcpy(buff,sz4dot);
        buff[apiSnprintf(buff, sizeof(buff)/sizeof(buff[0]), szDecimalFmt, (i?i:1)+EInfo.LeftPos)]=_T('.');
        Info.Text(i, 0, Options.RulerColor, buff);
    }
}

__inline static void SetColorize()
{
    DoColorize(EInfo.CurPos,EInfo.CurLine,TRUE,Options.Flags);
//Store flags for restore
    last.LastFlags = Options.Flags;
}

__inline static void DelColorize( int x,int y )
{
     if ( last.LastFlags == MAX_WORD || //not setted
          isColorer()                   //Colorer process full redraw - not need to delete color sections
        )
        return;

    DoColorize(x,y,FALSE,last.LastFlags);
}
//- CONFIG

struct FarDialogItemTpl
{
    const wchar_t *Mask;
    DWORD Flags;
    unsigned char Type;
    unsigned char X1;
    unsigned char Y1;
    unsigned char X2;
    unsigned char LngNum;
};

static BOOL DoConfigure( void )
{
    static const TCHAR szTitle     [] =CR_VERSION;
    static const TCHAR szFixFmt    [] =_T("###");
    TCHAR szColor[4];
    TCHAR szCursorColor[4];
    TCHAR szRulerColor[4];

    static const struct FarDialogItemTpl itm_tpl[] = {
        { 0,    DIF_CENTERTEXT,         DI_DOUBLEBOX,0, 0, 69, 255 },

        { 0,    0,                      DI_CHECKBOX, 1, 1, 0, lngEnabled },

        { szFixFmt, DIF_MASKEDIT,       DI_FIXEDIT,18, 2,21, 255 },
        { 0,    0,                          DI_TEXT, 3, 2, 0, lngCRColor },
        { szFixFmt, DIF_MASKEDIT,       DI_FIXEDIT,18, 3,21, 255 },
        { 0,    0,                          DI_TEXT, 3, 3, 0, lngCURColor },
        { szFixFmt, DIF_MASKEDIT,       DI_FIXEDIT,18, 4,21, 255 },
        { 0,    0,                          DI_TEXT, 3, 4, 0, lngRulerColor },

        { 0,    DIF_LEFTTEXT,               DI_TEXT, 1, 6, 0, lngActivs },
        { 0,    DIF_GROUP,              DI_CHECKBOX, 2, 7, 0, lngCtrl },
        { 0,    0,                      DI_CHECKBOX, 2, 8, 0, lngAlt },
        { 0,    0,                      DI_CHECKBOX, 2, 9, 0, lngShift },

        { 0,    DIF_LEFTTEXT,               DI_TEXT,15, 6, 0, lngLock },
        { 0,    DIF_GROUP,              DI_CHECKBOX,16, 7, 0, lngCaps },
        { 0,    0,                      DI_CHECKBOX,16, 8, 0, lngNum },
        { 0,    0,                      DI_CHECKBOX,16, 9, 0, lngScroll },

        { 0,    DIF_LEFTTEXT,          DI_SINGLEBOX,32, 1,68, lngOpt },
        { 0,    DIF_GROUP,              DI_CHECKBOX,33, 2, 0, lngColorerCor },
        { 0,    0,                      DI_CHECKBOX,33, 3, 0, lngDrawV },
        { 0,    0,                      DI_CHECKBOX,33, 4, 0, lngDrawH },
        { 0,    0,                      DI_CHECKBOX,33, 5, 0, lngCLRCursor },
        { 0,    0,                      DI_CHECKBOX,33, 6, 0, lngShowRuler },
        { 0,    0,                      DI_CHECKBOX,33, 7, 0, lngShCfg },

        { 0,    DIF_CENTERGROUP,          DI_BUTTON,0, 11, 0, lngOk },
        { 0,    DIF_CENTERGROUP,          DI_BUTTON,0, 11, 0, lngCancel },
    };
    struct  FarDialogItem itm[sizeof(itm_tpl)/sizeof(itm_tpl[0])];
    size_t i;
#define CD_OS         0
#define CD_ENABLED    1

#define CD_COLOR      2
#define CD_CCOLOR     4
#define CD_RCOLOR     6

#define CD_CONTROL    9
#define CD_ALT        10
#define CD_SHIFT      11

#define CD_CAPS       13
#define CD_NUM        14
#define CD_SCROLL     15

#define CD_OPT        16
#define CD_COLORER    17
#define CD_VERT       18
#define CD_HORZ       19
#define CD_CURSOR     20
#define CD_SHOWRULER  21
#define CD_CONFIG     22

#define CD_OK         23
#define CD_CANCEL     24

    zero_mem(&itm,sizeof(itm));
    for( i=0; i<sizeof(itm_tpl)/sizeof(itm_tpl[0]); i++ ){
        itm[i].Type=itm_tpl[i].Type;
        itm[i].X1=itm_tpl[i].X1+1;
        itm[i].Y1=itm_tpl[i].Y1+1;
        itm[i].X2=itm_tpl[i].X2+1;
        itm[i].Mask=itm_tpl[i].Mask;
        itm[i].Flags=itm_tpl[i].Flags;
        if( itm_tpl[i].LngNum!=255 )
            itm[i].PtrData = Info.GetMsg(Info.ModuleNumber,itm_tpl[i].LngNum);
    }
    itm[CD_OPT].Y2=9;
//OS caption
    itm[CD_OS].PtrData = szTitle;
//Main tasks
//Enabled
    itm[CD_ENABLED].Selected = Options.Enabled;
//Colorize cursor
    itm[CD_CURSOR].Selected  = IS_FLAG(Options.Flags,CRF_CURSOR);
//Colors
    apiSnprintf( szColor, sizeof(szColor)/sizeof(szColor[0]),  szDecimalFmt,Options.Color );
    apiSnprintf( szCursorColor, sizeof(szCursorColor)/sizeof(szCursorColor[0]), szDecimalFmt,Options.CenterColor );
    apiSnprintf( szRulerColor, sizeof(szRulerColor)/sizeof(szRulerColor[0]), szDecimalFmt,Options.RulerColor );
    itm[CD_COLOR].PtrData = szColor;
    itm[CD_CCOLOR].PtrData = szCursorColor;
    itm[CD_RCOLOR].PtrData = szRulerColor;
//Activators
    itm[CD_CONTROL].Selected   = IS_FLAG(Options.TempShow,CRTS_CONTROL);
    itm[CD_ALT].Selected       = IS_FLAG(Options.TempShow,CRTS_ALT);
    itm[CD_SHIFT].Selected     = IS_FLAG(Options.TempShow,CRTS_SHIFT);
//Lockers
    itm[CD_CAPS].Selected      = IS_FLAG(Options.LockShow,CRLS_CAPS);
    itm[CD_NUM].Selected       = IS_FLAG(Options.LockShow,CRLS_NUM);
    itm[CD_SCROLL].Selected    = IS_FLAG(Options.LockShow,CRLS_SCROLL);
//Flags
    itm[CD_COLORER].Selected   = IS_FLAG(Options.Flags,CRF_COLORER);
    itm[CD_VERT].Selected      = IS_FLAG(Options.Flags,CRF_VERT);
    itm[CD_HORZ].Selected      = IS_FLAG(Options.Flags,CRF_HORZ);
    itm[CD_CONFIG].Selected    = IS_FLAG(Options.Flags,CRF_PLUGINMENU);
    itm[CD_SHOWRULER].Selected = IS_FLAG(Options.Flags,CRF_SHOWRULER);

// !!Dialog
    HANDLE hDlg=Info.DialogInit(Info.ModuleNumber, -1, -1, 72, 14, _T("Settings"), itm, sizeof(itm)/sizeof(itm[0]), 0, 0, NULL, 0);
    if (hDlg == INVALID_HANDLE_VALUE)
        return 0;

    int Ret = Info.DialogRun(hDlg);
    if ( Ret == -1 || Ret == CD_CANCEL) {
        Info.DialogFree(hDlg);
        return FALSE;
    }

    for( i=0; i<sizeof(itm)/sizeof(itm[0]); i++ )
        itm[i].Selected = Info.SendDlgMessage(hDlg, DM_GETCHECK, i, 0) == BSTATE_CHECKED;

    Options.Flags    = 0;
    Options.TempShow = 0;
    Options.LockShow = 0;
//Main tasks
//Enabled
    Options.Enabled  = itm[CD_ENABLED].Selected;
//Colorize cursor
    if ( itm[CD_CURSOR].Selected )   SET_FLAG(Options.Flags,CRF_CURSOR);
//Colors
    Info.SendDlgMessage(hDlg, DM_GETTEXTPTR, CD_COLOR, (LONG_PTR)szColor);
    Options.Color = apiAtoi( szColor );
    if (!Options.Color) Options.Color = 31;
    Info.SendDlgMessage(hDlg, DM_GETTEXTPTR, CD_CCOLOR, (LONG_PTR)szCursorColor);
    Options.CenterColor = apiAtoi( szCursorColor );
    Info.SendDlgMessage(hDlg, DM_GETTEXTPTR, CD_RCOLOR, (LONG_PTR)szRulerColor);
    Options.RulerColor = apiAtoi( szRulerColor );
//Activators
    if ( itm[CD_CONTROL].Selected ) SET_FLAG( Options.TempShow,CRTS_CONTROL );
    if ( itm[CD_ALT].Selected )     SET_FLAG( Options.TempShow,CRTS_ALT );
    if ( itm[CD_SHIFT].Selected )   SET_FLAG( Options.TempShow,CRTS_SHIFT );
//Lockers
    if ( itm[CD_CAPS].Selected   )  SET_FLAG(Options.LockShow,CRLS_CAPS);
    if ( itm[CD_NUM].Selected    )  SET_FLAG(Options.LockShow,CRLS_NUM);
    if ( itm[CD_SCROLL].Selected )  SET_FLAG(Options.LockShow,CRLS_SCROLL);
//Flags
    if ( itm[CD_COLORER].Selected )  SET_FLAG(Options.Flags,CRF_COLORER);
    if ( itm[CD_VERT].Selected )     SET_FLAG(Options.Flags,CRF_VERT);
    if ( itm[CD_HORZ].Selected )     SET_FLAG(Options.Flags,CRF_HORZ);
    if ( itm[CD_CONFIG].Selected )   SET_FLAG(Options.Flags,CRF_PLUGINMENU);
    if ( itm[CD_SHOWRULER].Selected ) SET_FLAG(Options.Flags,CRF_SHOWRULER);

    Info.DialogFree(hDlg);

// !!Save config
    SaveConfig(&Options);
    return TRUE;
}

//- REDRAWS
__inline static void DoEventRedraw(void)
{
    last.TopLine = EInfo.TopScreenLine;
    if (last.isHilight) SetColorize();
    else if (isCursor()) EditSetCursor();
}

static void DoEventFull( void *Param )
{
    int cx = EInfo.CurPos,
        cy = EInfo.CurLine;
//Need to repaint
    if ( Param == EEREDRAW_CHANGE              ||   //Text changed
        last.curX != cx                        ||   //Cursor moved
        last.curY != cy                        ||
        EInfo.TopScreenLine != last.TopLine    ||   //Screen moved
        last.isHilight != last.hilight ) { // needRepaint
    //Force full redraw
    if ( Param != EEREDRAW_ALL ) {
        Info.EditorControl( ECTL_REDRAW,EEREDRAW_ALL );
        return;
    }
    //Hilighting
    if ( isCrest() && (last.isHilight || last.isHilight != last.hilight) ) {
        //Delete old drawing
        if (last.curX != -1) {
            DelColorize( last.curX,last.curY );
            last.curX = -1;
        }
        //Draw crest
        if (last.isHilight) {
            last.curX    = cx;
            last.curY    = cy;
            last.TopLine = EInfo.TopScreenLine;
            SetColorize();
        }
        if ( !last.isHilight && isCursor() ){
            last.curX    = cx;
            last.curY    = cy;
            EditSetCursor();
        }
      } else
    //Cursor
      if ( isCursor() ) {
          if (last.curX != -1)
              DelCursor( last.curX,last.curY );
          last.curX=cx;
          last.curY=cy;
          EditSetCursor();
      }
    }
}

//- FAR interface
#ifdef __cplusplus
extern "C" {
#endif
SHAREDSYMBOL int WINAPI EXP_NAME(ProcessEditorInput)( const INPUT_RECORD * ir)
{
  //Enabled
    if ( !Options.Enabled ) return 0;
  //Look for lockers\activators
    last.isHilight = isShow(ir);
  //Force repaint if changes
    if ( last.isHilight != last.hilight ) {
        Info.EditorControl( ECTL_REDRAW,EEREDRAW_ALL );
        last.hilight = last.isHilight;
        if( last.isHilight && isShowRuler() )
            Ruler();
    }

    return 0;
}

SHAREDSYMBOL int WINAPI EXP_NAME(ProcessEditorEvent)( int Event, void *Param )
{
//Enabled
    if ( !Options.Enabled ) return 0;
//Other events
    if ( Event == EE_READ) {
        last.isHilight = FALSE;
        last.hilight = FALSE;
        last.LastFlags = MAX_WORD;
        last.curX = -1;
    }
    if ( Event != EE_REDRAW ) {
        return 0;
    }

    Info.EditorControl( ECTL_GETINFO, &EInfo );

    if (isColorer())
        DoEventRedraw();
    else
        DoEventFull(Param);

    return 0;
}

SHAREDSYMBOL HANDLE WINAPI EXP_NAME(OpenPlugin)( int from, INT_PTR Item)
{
    BOOL center, crest;

    (void)Item;
//Process emulated config
    if ( from != OPEN_EDITOR ) return INVALID_HANDLE_VALUE;
  //Save current status
    center = Options.Enabled && isCursor();
    crest  = Options.Enabled && last.isHilight;
  //Do configure
    if ( !DoConfigure() ) return INVALID_HANDLE_VALUE;
  //Check statuses disabled
    if ( last.curX != -1 && !isColorer() ) {       //Not drawed
        Info.EditorControl( ECTL_GETINFO, &EInfo );
        //Crest disabled|changed
        if ( crest > (Options.Enabled && last.isHilight) ) {
            if (last.curX != -1)
                DelColorize( last.curX,last.curY );
            last.curX = -1;
      } else
      //Center disabled|changed
        if ( center > (Options.Enabled && isCursor()) ) {
            if (last.curX != -1) DelCursor( last.curX,last.curY );
            last.curX = -1;
        }
    }
  //Do repaint
    Info.EditorControl( ECTL_REDRAW,EEREDRAW_ALL );
    return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL int WINAPI EXP_NAME(Configure)( int ItemNumber )
{
//Process normal config
    return ItemNumber == 0 && DoConfigure();
}

SHAREDSYMBOL void WINAPI EXP_NAME(SetStartupInfo)( const struct PluginStartupInfo *pInfo )
{
    Options.Enabled       =FALSE;
    Options.Color         =0x47;
    Options.TempShow      =CRTS_ALT;
    Options.LockShow      =CRLS_SCROLL;
    Options.Flags         =CRF_VERT | CRF_HORZ | CRF_PLUGINMENU | CRF_SHOWRULER | CRF_COLORER;
    Options.CenterColor   =0xF0;
    Options.RulerColor    =0x0F;
    last.curX      = -1;
    last.curY      = -1;
    last.hilight   = FALSE;
    last.isHilight = FALSE;
    last.LastFlags = MAX_WORD;

    Info = *pInfo;
	apiSnprintf = pInfo->FSF->snprintf;
	apiAtoi = pInfo->FSF->atoi;
    RestoreConfig(&Options);
}

SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)( struct PluginInfo *pInfo )
{
    static const TCHAR* mnu;

    mnu=Info.GetMsg(Info.ModuleNumber,lngEditorCrestSetup);

    zero_mem(pInfo,sizeof(*pInfo));

    pInfo->StructSize                = sizeof(*pInfo);
    pInfo->Flags                     = PF_EDITOR | PF_DISABLEPANELS;

    if ( isPluginMenu() ) {
        pInfo->PluginMenuStrings         = &mnu;
        pInfo->PluginMenuStringsNumber   = 1;
    }

    pInfo->PluginConfigStrings       = &mnu;
    pInfo->PluginConfigStringsNumber = 1;

}
#ifdef __cplusplus
}
#endif
