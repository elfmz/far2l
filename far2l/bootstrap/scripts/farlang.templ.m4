m4_include(`farversion.m4')m4_dnl
#hpp file name
lang.inc
#number of languages
8
#id:0 language file name, language name, language description
FarRus.lng Russian "Russian (Русский)"
#id:1 language file name, language name, language description
FarEng.lng English "English"
#id:2 language file name, language name, language description
FarCze.lng Czech "Czech (Čeština)"
#id:3 language file name, language name, language description
FarGer.lng German "German (Deutsch)"
#id:4 language file name, language name, language description
FarHun.lng Hungarian "Hungarian (Magyar)"
#id:5 language file name, language name, language description
FarPol.lng Polish "Polish (Polski)"
#id:6 language file name, language name, language description
FarSpa.lng Spanish "Spanish (Español)"
#id:7 language file name, language name, language description
FarUkr.lng Ukrainian "Ukrainian (Український)"

#head of the hpp file
#hhead:
#hhead:

#tail of the hpp file
#htail:
#htail:
#and so on as much as needed

enum:LangMsg

#--------------------------------------------------------------------
#now come the lng feeds
#--------------------------------------------------------------------
#first comes the text name from the enum which can be preceded with
#comments that will go to the hpp file
#h://This comment will appear before Yes
#he://This comment will appear after Yes
#Yes
#now come the lng lines for all the languages in the order defined
#above, they can be preceded with comments as shown below
#l://This comment will appear in all the lng files before the lng line
#le://This comment will appear in all the lng files after the lng line
#ls://This comment will appear only in Russian lng file before the lng line
#lse://This comment will appear only in Russian lng file after the lng line
#"Да"
#ls://This comment will appear only in English lng file before the lng line
#lse://This comment will appear only in English lng file after the lng line
#"Yes"
#ls://This comment will appear only in Czech lng file before the lng line
#lse://This comment will appear only in Czech lng file after the lng line
#upd:"Ano"
#
#lng lines marked with "upd:" will cause a warning to be printed to the
#screen reminding that this line should be updated/translated


Yes
`l://Version: 'MAJOR`.'MINOR` build 'PATCH
l:
"Да"
"Yes"
"Ano"
"Ja"
"Igen"
"Tak"
"Si"
"Так"

No
"Нет"
"No"
"Ne"
"Nein"
"Nem"
"Nie"
"No"
"Ні"

Ok
"OK"
"OK"
"Ok"
"OK"
"OK"
"OK"
"Aceptar"
"OK"

HYes
l:
"&Да"
"&Yes"
"&Ano"
"&Ja"
"I&gen"
"&Tak"
"&Si"
"&Так"

HNo
"&Нет"
"&No"
"&Ne"
"&Nein"
"Ne&m"
"&Nie"
"&No"
"&Ні"

HOk
"&OK"
"&OK"
"&Ok"
"&OK"
"&OK"
"&OK"
"&Aceptar"
"&OK"

Cancel
l:
"Отмена"
"Cancel"
"Storno"
"Abbrechen"
"Mégsem"
"Anuluj"
"Cancelar"
"Відміна"

Retry
"Повторить"
"Retry"
"Znovu"
"Wiederholen"
"Újra"
"Ponów"
"Reiterar"
"Повторити"

Skip
"Пропустить"
"Skip"
"Přeskočit"
"Überspringen"
"Kihagy"
"Omiń"
"Omitir"
"Пропустити"

Abort
"Прервать"
"Abort"
"Zrušit"
"Abbrechen"
"Megszakít"
"Zaniechaj"
"Abortar"
"Перервати"

Ignore
"Игнорировать"
"Ignore"
"Ignorovat"
"Ignorieren"
"Mégis"
"Zignoruj"
"Ignorar"
"Ігнорувати"

Delete
"Удалить"
"Delete"
"Smazat"
"Löschen"
"Töröl"
"Usuń"
"Borrar"
"Вилучити"

Split
"Разделить"
"Split"
"Rozdělit"
"Zerteilen"
"Feloszt"
"Podziel"
"Dividir"
"Розділити"

Remove
"Удалить"
"Remove"
"Odstranit"
"Entfernen"
"Eltávolít"
"Usuń"
"Remover"
"Видалити"

HCancel
l:
"&Отмена"
"&Cancel"
"&Storno"
"&Abbrechen"
"Még&sem"
"&Anuluj"
"&Cancelar"
"&Відміна"

HRetry
"&Повторить"
"&Retry"
"&Znovu"
"&Wiederholen"
"Ú&jra"
"&Ponów"
"&Reiterar"
"&Повторити"

HSkip
"П&ропустить"
"&Skip"
"&Přeskočit"
"Über&springen"
"Ki&hagy"
"&Omiń"
"&Omitir"
"П&ропустити"

HSkipAll
"Пропустить &все"
"S&kip all"
"Přeskočit &vše"
"Alle übersprin&gen"
"Kihagy &mind"
"Omiń &wszystkie"
"Omitir &Todo"
"Пропустити &усе"

HAbort
"Прер&вать"
"&Abort"
"Zr&ušit"
"&Abbrechen"
"Megsza&kít"
"&Zaniechaj"
"Ab&ortar"
"Перер&вати"

HIgnore
"&Игнорировать"
"&Ignore"
"&Ignorovat"
"&Ignorieren"
"Mé&gis"
"Z&ignoruj"
"&Ignorar"
"&Ігнорувати"

HDelete
"&Удалить"
"&Delete"
"S&mazat"
"&Löschen"
"&Töröl"
"&Usuń"
"&Borrar"
"&Вилучити"

HRemove
"&Удалить"
"R&emove"
"&Odstranit"
"Ent&fernen"
"Eltá&volít"
"U&suń"
"R&emover"
"&Видалити"

HSplit
"Раз&делить"
"Sp&lit"
"&Rozdělit"
"&Zerteilen"
"Fel&oszt"
"Po&dziel"
"Dividir"
"Роз&ділити"


Warning
l:
"Предупреждение"
"Warning"
"Varování"
"Warnung"
"Figyelem"
"Ostrzeżenie"
"Advertencia"
"Попередження"

Error
"Ошибка"
"Error"
"Chyba"
"Fehler"
"Hiba"
"Błąd"
"Error"
"Помилка"

Quit
l:
"Выход"
"Quit"
"Konec"
"Beenden"
"Kilépés"
"Zakończ"
"Salir"
"Вихід"

AskQuit
"Вы хотите завершить работу в FAR?"
"Do you want to quit FAR?"
"Opravdu chcete ukončit FAR?"
"Wollen Sie FAR beenden?"
"Biztosan kilép a FAR-ból?"
"Czy chcesz zakończyć pracę z FARem?"
"Desea salir de FAR?"
"Ви хочете завершити роботу в FAR?"

Background
"&В фон"
"&Background"
"&Background"
"&Background"
"&Background"
"&Background"
"&Background"
"&У тлі"

GetOut
"&Выбраться"
"&Get out"
"&Get out"
"&Get out"
"&Get out"
"&Get out"
"&Get out"
"&Вибратися"

F1
l:
l://functional keys - 6 characters max
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

F2
"ПользМ"
"UserMn"
"UživMn"
"BenuMn"
"FhMenü"
"Menu"
"Menú "
"КористМ"

F3
"Просм"
"View"
"Zobraz"
"Betr."
"Megnéz"
"Zobacz"
"Ver "
"Перегл"

F4
"Редакт"
"Edit"
"Edit"
"Bearb"
"Szerk."
"Edytuj"
"Editar"
"Редакт"

F5
"Копир"
"Copy"
"Kopír."
"Kopier"
"Másol"
"Kopiuj"
"Copiar"
"Копію"

F6
"Перен"
"RenMov"
"PřjPřs"
"Versch"
"AtnMoz"
"ZmNazw"
"RenMov"
"Перен"

F7
"Папка"
"MkFold"
"VytAdr"
"VerzEr"
"ÚjMapp"
"UtwKat"
"CrDIR "
"Тека"

F8
"Удален"
"Delete"
"Smazat"
"Lösch."
"Töröl"
"Usuń"
"Borrar"
"Видалн"

F9
"КонфМн"
"ConfMn"
"KonfMn"
"KonfMn"
"KonfMn"
"Konfig"
"BarMnu"
"КонфМн"

F10
"Выход"
"Quit"
"Konec"
"Beend."
"Kilép"
"Koniec"
"Salir"
"Вихід"

F11
"Модули"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Модулі"

F12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant. "
"Екрани"

AltF1
l:
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda "
"Ліва"

AltF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha "
"Права"

AltF3
"Смотр."
"View.."
"Zobr.."
"Betr.."
"Néző.."
"Zobacz"
"Ver..."
"Див."

AltF4
"Редак."
"Edit.."
"Edit.."
"Bear.."
"Szrk.."
"Edytuj"
"Edita."
"Редаг."

AltF5
"Печать"
"Print"
"Tisk"
"Druck"
"Nyomt"
"Drukuj"
"Imprim"
"Друк"

AltF6
"Ссылка"
"MkLink"
"VytLnk"
"LinkEr"
"ÚjLink"
"Dowiąż"
"CrVinc"
"Посилн"

AltF7
"Искать"
"Find"
"Hledat"
"Suchen"
"Keres"
"Znajdź"
"Buscar"
"Шукати"

AltF8
"Истор"
"Histry"
"Histor"
"Histor"
"ParElő"
"Histor"
"Histor"
"Істор"

AltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Tryb"
"Video"
"Відео""

AltF10
"Дерево"
"Tree"
"Strom"
"Baum"
"MapKer"
"Drzewo"
"Arbol"
"Дерево"

AltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"HsPodg"
"HisVer"
"ІстПр"

AltF12
"ИстПап"
"FoldHs"
"AdrsHs"
"BearHs"
"MapElő"
"HsKat"
"HisDir"
"ІстТек"

CtrlF1
l:
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda "
"Ліва"

CtrlF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha "
"Права"

CtrlF3
"Имя   "
"Name  "
"Název "
"Name  "
"Név"
"Nazwa"
"Nombre"
"Ім'я"

CtrlF4
"Расшир"
"Extens"
"Přípon"
"Erweit"
"Kiterj"
"Rozsz"
"Extens"
"Розшир"

CtrlF5
"Запись"
"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
"Запис"

CtrlF6
"Размер"
"Size"
"Veliko"
"Größe"
"Méret"
"Rozm"
"Tamaño"
"Розмір"

CtrlF7
"Несорт"
"Unsort"
"Neřadi"
"Unsort"
"NincsR"
"BezSor"
"SinOrd"
"Несорт"

CtrlF8
"Создан"
"Creatn"
"Vytvoř"
"Erstel"
"Keletk"
"Utworz"
"Creado"
"Стврно"

CtrlF9
"Доступ"
"Access"
"Přístu"
"Zugrif"
"Hozzáf"
"Użycie"
"Acceso"
"Доступ"

CtrlF10
"Описан"
"Descr"
"Popis"
"Beschr"
"Megjgy"
"Opis"
"Descr"
"Опсний"

CtrlF11
"Владел"
"Owner"
"Vlastn"
"Besitz"
"Tulajd"
"Właśc"
"Dueño"
"Володр"

CtrlF12
"Сорт"
"Sort"
"Třídit"
"Sort."
"RendMd"
"Sortuj"
"Orden"
"Сорт"

ShiftF1
l:
"Добавл"
"Add"
"Přidat"
"Hinzu"
"Tömört"
"Dodaj"
"Añadir"
"Добавл"

ShiftF2
"Распак"
"Extrct"
"Rozbal"
"Extrah"
"Kibont"
"Rozpak"
"Extrae"
"Розпак"

ShiftF3
"АрхКом"
"ArcCmd"
"ArcPří"
"ArcBef"
"TömPar"
"Polec"
"ArcCmd"
"АрхКом"

ShiftF4
"Редак."
"Edit.."
"Edit.."
"Erst.."
"ÚjFájl"
"Edytuj"
"Editar"
"Редаг."

ShiftF5
"Копир"
"Copy"
"Kopír."
"Kopier"
"Másol"
"Kopiuj"
"Copiar"
"Копію."

ShiftF6
"Переим"
"Rename"
"Přejme"
"Umbene"
"ÁtnMoz"
"ZmNazw"
"RenMov"
"Перейм"

ShiftF7
""
""
""
""
""
""
""
""

ShiftF8
"Удален"
"Delete"
"Smazat"
"Lösch."
"Töröl"
"Usuń"
"Borrar"
"Видалн"

ShiftF9
"Сохран"
"Save"
"Uložit"
"Speich"
"Mentés"
"Zapisz"
"Guarda"
"Збереж"

ShiftF10
"Послдн"
"Last"
"Posled"
"Letzte"
"UtsMnü"
"Ostatn"
"Ultimo"
"Останн"

ShiftF11
"Группы"
"Group"
"Skupin"
"Gruppe"
"Csoprt"
"Grupa"
"Grupo"
"Групи"

ShiftF12
"Выбран"
"SelUp"
"VybPrv"
"AuswOb"
"KijFel"
"SelUp"
"SelUp"
"Обранй"

AltShiftF1
l:
l:// Main AltShift
""
""
""
""
""
""
""
""

AltShiftF2
""
""
""
""
""
""
""
""

AltShiftF3
""
""
""
""
""
""
""
""

AltShiftF4
""
""
""
""
""
""
""
""

AltShiftF5
""
""
""
""
""
""
""
""

AltShiftF6
""
""
""
""
""
""
""
""

AltShiftF7
""
""
""
""
""
""
""
""

AltShiftF8
""
""
""
""
""
""
""
""

AltShiftF9
"КонфПл"
"ConfPl"
"KonfPl"
"KonfPn"
"PluKnf"
"KonfPl"
"ConfPl"
"КонфПл"

AltShiftF10
""
""
""
""
""
""
""
""

AltShiftF11
""
""
""
""
""
""
""
""

AltShiftF12
""
""
""
""
""
""
""
""

CtrlShiftF1
l:
l://Main CtrlShift
""
""
""
""
""
""
""
""

CtrlShiftF2
""
""
""
""
""
""
""
""

CtrlShiftF3
"Просм"
"View"
"Zobraz"
"Betr"
"Megnéz"
"Podgląd"
"Ver "
"Пергл"

CtrlShiftF4
"Редакт"
"Edit"
"Edit"
"Bearb"
"Szerk."
"Edycja"
"Editar"
"Редаг"

CtrlShiftF5
""
""
""
""
""
""
""
""

CtrlShiftF6
""
""
""
""
""
""
""
""

CtrlShiftF7
""
""
""
""
""
""
""
""

CtrlShiftF8
""
""
""
""
""
""
""
""

CtrlShiftF9
""
""
""
""
""
""
""
""

CtrlShiftF10
""
""
""
""
""
""
""
""

CtrlShiftF11
""
""
""
""
""
""
""
""

CtrlShiftF12
""
""
""
""
""
""
""
""

CtrlAltF1
l:
l:// Main CtrlAlt
""
""
""
""
""
""
""
""

CtrlAltF2
""
""
""
""
""
""
""
""

CtrlAltF3
""
""
""
""
""
""
""
""

CtrlAltF4
""
""
""
""
""
""
""
""

CtrlAltF5
""
""
""
""
""
""
""
""

CtrlAltF6
""
""
""
""
""
""
""
""

CtrlAltF7
""
""
""
""
""
""
""
""

CtrlAltF8
""
""
""
""
""
""
""
""

CtrlAltF9
""
""
""
""
""
""
""
""

CtrlAltF10
""
""
""
""
""
""
""
""

CtrlAltF11
""
""
""
""
""
""
""
""

CtrlAltF12
""
""
""
""
""
""
""
""

CtrlAltShiftF1
l:
l:// Main CtrlAltShift
""
""
""
""
""
""
""
""

CtrlAltShiftF2
""
""
""
""
""
""
""
""

CtrlAltShiftF3
""
""
""
""
""
""
""
""

CtrlAltShiftF4
""
""
""
""
""
""
""
""

CtrlAltShiftF5
""
""
""
""
""
""
""
""

CtrlAltShiftF6
""
""
""
""
""
""
""
""

CtrlAltShiftF7
""
""
""
""
""
""
""
""

CtrlAltShiftF8
""
""
""
""
""
""
""
""

CtrlAltShiftF9
""
""
""
""
""
""
""
""

CtrlAltShiftF10
""
""
""
""
""
""
""
""

CtrlAltShiftF11
""
""
""
""
""
""
""
""

CtrlAltShiftF12
le://End of functional keys
""
""
""
""
""
""
""
""

HistoryTitle
l:
"История команд"
"History"
"Historie"
"Historie der letzten Befehle"
"Parancs előzmények"
"Historia"
"Historial"
"Історія команд"


FolderHistoryTitle
"История папок"
"Folders history"
"Historie adresářů"
"Zuletzt besuchte Ordner"
"Mappa előzmények"
"Historia katalogów"
"Historial directorios"
"Історія тек"

ViewHistoryTitle
"История просмотра"
"File view history"
"Historie prohlížení souborů"
"Zuletzt betrachtete Dateien"
"Fájl előzmények"
"Historia podglądu plików"
"Historial visor"
"Історія перегляду"

ViewHistoryIsCreate
"Создать файл?"
"Create file?"
"Vytvořit soubor?"
"Datei erstellen?"
"Fájl létrehozása?"
"Utworzyć plik?"
"Crear archivo?"
"Створити файл?"

HistoryView
"Просмотр"
"View"
"Zobrazit"
"Betr"
"Nézett"
"Zobacz"
"Ver   "
"Перегляд"

HistoryEdit
"Редактор"
"Edit"
"Editovat"
"Bearb"
"Szerk."
"Edytuj"
"Editar"
"Редактор"

HistoryExt
"Внешний"
"Ext."
"Rozšíření"
"Ext."
"Kit."
"Ext."
"Ext."
"Зовнішній"

HistoryClear
l:
"История будет полностью очищена. Продолжить?"
"All records in the history will be deleted. Continue?"
"Všechny záznamy v historii budou smazány. Pokračovat?"
"Die gesamte Historie wird gelöscht. Fortfahren?"
"Az előzmények minden eleme törlődik. Folytatja?"
"Wszystkie wpisy historii będą usunięte. Kontynuować?"
"Los datos en el historial serán borrados. Continuar?"
"Історія буде повністю очищена. Продовжити?"


Clear
"&Очистить"
"&Clear history"
"&Vymazat historii"
"Historie &löschen"
"Elő&zmények törlése"
"&Czyść historię"
"&Limpiar historial"
"&Очистити"

ConfigSystemTitle
l:
"Системные параметры"
"System settings"
"Nastavení systému"
"Grundeinstellungen"
"Rendszer beállítások"
"Ustawienia systemowe"
"Opciones de sistema"
"Системні параметри"

ConfigRO
"&Снимать атрибут R/O c CD файлов"
"&Clear R/O attribute from CD files"
"Z&rušit atribut R/O u souborů na CD"
"Schreibschutz von CD-Dateien ent&fernen"
"&Csak olvasható attr. törlése CD fájlokról"
"Wyczyść atrybut &R/O przy kopiowaniu z CD"
"&Borrar atributos R/O de archivos de CD"
"&Зняти атрибут R/O з CD файлів"

ConfigSudoEnabled
"Разрешить повышение привилегий"
"Enable s&udo privileges elevation"
upd:"Enable sudo privileges elevation"
upd:"Enable sudo privileges elevation"
upd:"Enable sudo privileges elevation"
upd:"Enable sudo privileges elevation"
upd:"Enable sudo privileges elevation"
"Дозволити підвишення привілеїв"

ConfigSudoConfirmModify
"Подтверждать все операции записи"
"Always confirm modify operations"
upd:"Always confirm modify operations"
upd:"Always confirm modify operations"
upd:"Always confirm modify operations"
upd:"Always confirm modify operations"
upd:"Always confirm modify operations"
"Підтверджувати усі операції запису"

ConfigSudoPasswordExpiration
"Время действия пароля (сек):"
"Password expiration (sec):"
upd:"Password expiration (sec):"
upd:"Password expiration (sec):"
upd:"Password expiration (sec):"
upd:"Password expiration (sec):"
upd:"Password expiration (sec):"
"Час дії пароля (сек):"

SudoTitle
"Операция требует повышения привилегий"
"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
"Операція вимагає підвищення привілеїв"

SudoPrompt
"Введите пароль для sudo"
"Enter sudo password"
upd:"Enter sudo password"
upd:"Enter sudo password"
upd:"Enter sudo password"
upd:"Enter sudo password"
upd:"Enter sudo password"
"Введіть пароль для sudo"

SudoConfirm
"Подтвердите использование привилегий"
"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
"Підтвердіть використання привілеїв"

ConfigRecycleBin
"Удалять в &Корзину"
"&Delete to Recycle Bin"
"&Mazat do Koše"
"In Papierkorb &löschen"
"&Törlés a Lomtárba"
"&Usuwaj do Kosza"
"Borrar hacia &papelera de reciclaje"
"Видаляти у &Кошик"

ConfigRecycleBinLink
"У&далять символические ссылки"
"Delete symbolic &links"
"Mazat symbolické &linky"
"Symbolische L&inks löschen"
"Szimbolikus l&inkek törlése"
"Usuń &linki symboliczne"
"Borrar en&laces simbólicos"
"В&идаляти символічні посилання"

CopyWriteThrough
"Выключить кэ&ширование записи"
"Disable &write cache"
upd:"Disable &write cache"
upd:"Disable &write cache"
upd:"Disable &write cache"
upd:"Disable &write cache"
upd:"Disable &write cache"
"Вимкнути ке&шування запису"

CopyXAttr
"Копировать расширенные а&ттрибуты"
"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
"Копіювати розширені а&трибути"

ConfigOnlyFilesSize
"Учитывать только размер файлов"
"Use only files size in estimation"
upd:"Use only files size in estimation"
upd:"Use only files size in estimation"
upd:"Use only files size in estimation"
upd:"Use only files size in estimation"
upd:"Use only files size in estimation"
"Враховувати лише розмір файлів"

ConfigScanJunction
"Ск&анировать символические ссылки"
"Scan s&ymbolic links"
"Prohledávat s&ymbolické linky"
"S&ymbolische Links scannen"
"Szimbolikus linkek &vizsgálata"
"Skanuj linki s&ymboliczne"
"Explorar enlaces simbólicos"
"Ск&анувати символічні посилання"

ConfigInactivity
"&Время бездействия"
"&Inactivity time"
"&Doba nečinnosti"
"Inaktivitäts&zeit"
"A FAR kilé&p"
"Czas &bezczynności"
"Desact&ivar FAR en..."
"&Час бездіяльності"

ConfigInactivityMinutes
"минут"
"minutes"
"minut"
"Minuten"
"perc tétlenség után"
"&minut"
"minutos"
"хвилин"

ConfigSaveHistory
"Сохранять &историю команд"
"Save commands &history"
"Ukládat historii &příkazů"
"&Befehlshistorie speichern"
"Parancs elő&zmények mentése"
"Zapisz historię &poleceń"
"Guardar &historial de comandos"
"Зберігати &історію команд"

ConfigSaveFoldersHistory
"Сохранять историю п&апок"
"Save &folders history"
"Ukládat historii &adresářů"
"&Ordnerhistorie speichern"
"M&appa előzmények mentése"
"Zapisz historię &katalogów"
"Guardar historial de directorios"
"Зберігати історію т&ек"

ConfigSaveViewHistory
"Сохранять историю п&росмотра и редактора"
"Save &view and edit history"
"Ukládat historii Zobraz a Editu&j"
"Betrachter/&Editor-Historie speichern"
"Nézőke és &szerkesztő előzmények mentése"
"Zapisz historię podglądu i &edycji"
"Guardar historial de &visor y editor"
"Зберігати історію п&ерегляду та редагування"

ConfigRegisteredTypes
"Использовать стандартные &типы файлов"
"Use Windows &registered types"
"Používat regi&strované typy Windows"
"&Registrierte Windows-Dateitypen verwenden"
"&Windows reg. fájltípusok használata"
"Użyj zare&jestrowanych typów Windows"
"Usar extensiones &registradas de Windows"
"Використовувати стандартні &типи файлів"

ConfigCloseCDGate
"Автоматически монтироват&ь CDROM"
"CD drive auto &mount"
"Automatické př&ipojení CD disků"
"CD-Laufwerk auto&matisch schließen"
"CD tálca a&utomatikus behúzása"
"&Montuj CD automatycznie"
"CD-ROM: automontar unidad"
"Автоматично монтуват&и CDROM"

ConfigUpdateEnvironment
"Автообновление переменных окружения"
"Automatic update of environment variables"
upd:"Automatic update of environment variables"
upd:"Automatic update of environment variables"
upd:"Automatic update of environment variables"
upd:"Automatic update of environment variables"
upd:"Automatic update of environment variables"
"Автооновленння змінних оточення"

ConfigAutoSave
"Автозапись кон&фигурации"
"Auto &save setup"
"Automatické ukládaní &nastavení"
"Setup automatisch &"speichern"
"B&eállítások automatikus mentése"
"Automatycznie &zapisuj ustawienia"
"Auto&guardar configuración"
"Автозапис кон&фігурації"

ConfigPanelTitle
l:
"Настройки панели"
"Panel settings"
"Nastavení panelů"
"Panels einrichten"
"Panel beállítások"
"Ustawienia panelu"
"Configuración de paneles"
"Налаштування панели"

ConfigHidden
"Показывать скр&ытые и системные файлы"
"Show &hidden and system files"
"Ukázat &skryté a systémové soubory"
"&Versteckte und Systemdateien anzeigen"
"&Rejtett és rendszerfájlok mutatva"
"Pokazuj pliki &ukryte i systemowe"
"Mostrar archivos ocultos y de sistema"
"Показувати при&ховані та системні файли"

ConfigHighlight
"&Раскраска файлов"
"Hi&ghlight files"
"Zvý&razňovat soubory"
"Dateien mark&ieren"
"Fá&jlok kiemelése"
"W&yróżniaj pliki"
"Resaltar archivos"
"&Розфарбовка файлів"

ConfigAutoChange
"&Автосмена папки"
"&Auto change folder"
"&Automaticky měnit adresář"
"Ordner &automatisch wechseln (Baumansicht)"
"&Automatikus mappaváltás"
"&Automatycznie zmieniaj katalog"
"&Auto cambiar directorio"
"&Автозміна теки"

ConfigSelectFolders
"Пометка &папок"
"Select &folders"
"Vybírat a&dresáře"
"&Ordner auswählen"
"A ma&ppák is kijelölhetők"
"Zaznaczaj katalo&gi"
"Seleccionar &directorios"
"Позначка &тек"

ConfigSortFolderExt
"Сортировать имена папок по рас&ширению"
"Sort folder names by e&xtension"
"Řadit adresáře podle přípony"
"Ordner nach Er&weiterung sortieren"
"Mappák is rendezhetők &kiterjesztés szerint"
"Sortuj nazwy katalogów wg r&ozszerzeń"
"Ordenar directorios por extensión"
"Сортувати імена папок по роз&ширенню"

ConfigReverseSort
"Разрешить &обратную сортировку"
"Allow re&verse sort modes"
"Do&volit změnu směru řazení"
"&Umgekehrte Sortiermodi zulassen"
"Fordí&tott rendezés engedélyezése"
"Włącz &możliwość odwrotnego sortowania"
"Permitir modo de orden in&verso"
"Дозволити &зворотне сортування"

ConfigAutoUpdateLimit
"Отключать автооб&новление панелей,"
"&Disable automatic update of panels"
"Vypnout a&utomatickou aktualizaci panelů"
"Automatisches Panelupdate &deaktivieren"
"Pan&el automatikus frissítése kikapcsolva,"
"&Wyłącz automatyczną aktualizację paneli"
"Desactiva actualización automát. de &paneles"
"Відключати автоо&новлення панелей,"

ConfigAutoUpdateLimit2
"если объектов больше"
"if object count exceeds"
"jestliže počet objektů překročí"
"wenn mehr Objekte als"
"ha több elem van, mint:"
"jeśli zawierają więcej obiektów niż"
"si conteo de objetos es excedido"
"якщо об'ектів більше"

ConfigAutoUpdateRemoteDrive
"Автообновление с&етевых дисков"
"Network drives autor&efresh"
"Automatická obnova síťových disků"
"Netzw&erklauferke autom. aktualisieren"
"Hálózati meghajtók autom. &frissítése"
"Auto&odświeżanie dysków sieciowych"
"Autor&efrescar unidades de Red"
"Автооновлення м&ережевих дисків"

ConfigShowColumns
"Показывать &заголовки колонок"
"Show &column titles"
"Zobrazovat &nadpisy sloupců"
"S&paltentitel anzeigen"
"Oszlop&nevek mutatva"
"Wyświetl tytuły &kolumn"
"Mostrar títulos de &columnas"
"Показувати &заголовки колонок"

ConfigShowStatus
"Показывать &строку статуса"
"Show &status line"
"Zobrazovat sta&vový řádek"
"&Statuszeile anzeigen"
"Á&llapotsor mutatva"
"Wyświetl &linię statusu"
"Mostrar línea de e&stado"
"Показувати &рядок статусу"

ConfigShowTotal
"Показывать су&ммарную информацию"
"Show files &total information"
"Zobrazovat &informace o velikosti souborů"
"&Gesamtzahl für Dateien anzeigen"
"Fájl össze&s információja mutatva"
"Wyświetl &całkowitą informację o plikach"
"Mostrar información comple&ta de archivos"
"Показувати су&марну інформацію"

ConfigShowFree
"Показывать с&вободное место"
"Show f&ree size"
"Zobrazovat vo&lné místo"
"&Freien Speicher anzeigen"
"Sza&bad lemezterület mutatva"
"Wyświetl ilość &wolnego miejsca"
"Mostrar espacio lib&re"
"Показувати в&ільне місце"

ConfigShowScrollbar
"Показывать по&лосу прокрутки"
"Show scroll&bar"
"Zobrazovat &posuvník"
"Scroll&balken anzeigen"
"Gördítősá&v mutatva"
"Wyświetl &suwak"
"Mostrar &barra de desplazamiento"
"Показувати см&угу прокручування"

ConfigShowScreensNumber
"Показывать количество &фоновых экранов"
"Show background screens &number"
"Zobrazovat počet &obrazovek na pozadí"
"&Nummer von Hintergrundseiten anzeigen"
"&Háttérképernyők száma mutatva"
"Wyświetl ilość &ekranów w tle"
"Mostrar &número de pantallas de fondo"
"Показувати кількість &фонових екранів"

ConfigShowSortMode
"Показывать букву режима сор&тировки"
"Show sort &mode letter"
"Zobrazovat písmeno &módu řazení"
"Buchstaben der Sortier&modi anzeigen"
"Rendezési mó&d betűjele mutatva"
"Wyświetl l&iterę trybu sortowania"
"Mostrar letra para &modo de orden"
"Показувати літеру режиму сор&тування"

ConfigInterfaceTitle
l:
"Настройки интерфейса"
"Interface settings"
"Nastavení rozhraní"
"Oberfläche einrichten"
"Kezelőfelület beállítások"
"Ustawienia interfejsu"
"Opciones de interfaz"
"Налаштування інтерфейсу"

ConfigInputTitle
l:
"Настройки ввода"
"Input settings"
upd:"Input settings"
upd:"Input settings"
upd:"Input settings"
upd:"Input settings"
upd:"Input settings"
"Налаштування введення"

ConfigClock
"&Часы в панелях"
"&Clock in panels"
"&Hodiny v panelech"
"&Uhr in Panels anzeigen"
"Ór&a a paneleken"
"&Zegar"
"&Reloj en paneles"
"&Годинник у панелях"

ConfigViewerEditorClock
"Ч&асы при редактировании и просмотре"
"C&lock in viewer and editor"
"H&odiny v prohlížeči a editoru"
"U&hr in Betrachter und Editor anzeigen"
"Ó&ra a nézőkében és szerkesztőben"
"Zegar w &podglądzie i edytorze"
"Re&loj en visor y editor"
"Г&одинник під час редагування та перегляду"

ConfigMouse
"Мы&шь"
"M&ouse"
"M&yš"
"M&aus aktivieren"
"&Egér kezelése"
"M&ysz"
"Rat&ón"
"Ми&ша"

ConfigXLats
"Правила &транслитерации:"
"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
"Правила &транслітерації:"

ConfigXLatDialogs
"Транслитерация &диалоговой навигации"
"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Транслітерація &діалогової навігації"

ConfigXLatFastFileFind
"Транслитерация быстрого поиска &файла"
"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Транслітерація швидкого пошуку &файлу"

ConfigKeyBar
"Показывать &линейку клавиш"
"Show &key bar"
"Zobrazovat &zkratkové klávesy"
"Tast&enleiste anzeigen"
"&Funkcióbillentyűk sora mutatva"
"Wyświetl pasek &klawiszy"
"Mostrar barra de &funciones"
"Показувати &лінійку клавіш"

ConfigMenuBar
"Всегда показывать &меню"
"Always show &menu bar"
"Vždy zobrazovat hlavní &menu"
"&Menüleiste immer anzeigen"
"A &menüsor mindig látszik"
"Zawsze pokazuj pasek &menu"
"Mostrar siempre barra de &menú"
"Завжди показувати &меню"

ConfigSaver
"&Сохранение экрана"
"&Screen saver"
"Sp&ořič obrazovky"
"Bildschirm&schoner"
"&Képernyőpihentető"
"&Wygaszacz ekranu"
"&Salvapantallas"
"&Збереження екрана"

ConfigSaverMinutes
"минут"
"minutes"
"minut"
"Minuten"
"perc tétlenség után"
"m&inut"
"minutos"
"хвилин"

ConfigConsoleChangeFont
"Выбрать шри&фт"
"Change &font"
upd:"Change &font"
upd:"Change &font"
upd:"Change &font"
upd:"Change &font"
upd:"Change &font"
"Вибрати шри&фт"

ConfigConsolePaintSharp
"Отключить сглаживание"
"Disable antialiasing"
upd:"Disable antialiasing"
upd:"Disable antialiasing"
upd:"Disable antialiasing"
upd:"Disable antialiasing"
upd:"Disable antialiasing"
"Відключити згладжування"

ConfigExclusiveKeys
"&Экслюзивная обработка нажатий, включающих:"
"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
"&Ексклюзивне оброблення натискань, що включають:"

ConfigExclusiveCtrlLeft
"Левый Ctrl"
"Left Ctrl"
upd:"Left Ctrl"
upd:"Left Ctrl"
upd:"Left Ctrl"
upd:"Left Ctrl"
upd:"Left Ctrl"
"Лівий Ctrl"

ConfigExclusiveCtrlRight
"Правый Ctrl"
"Right Ctrl"
upd:"Right Ctrl"
upd:"Right Ctrl"
upd:"Right Ctrl"
upd:"Right Ctrl"
upd:"Right Ctrl"
"Правий Ctrl"

ConfigExclusiveAltLeft
"Левый Alt "
"Left Alt "
upd:"Left Alt "
upd:"Left Alt "
upd:"Left Alt "
upd:"Left Alt "
upd:"Left Alt "
"Лівий Alt "

ConfigExclusiveAltRight
"Правый Alt "
"Right Alt "
upd:"Right Alt "
upd:"Right Alt "
upd:"Right Alt "
upd:"Right Alt "
upd:"Right Alt "
"Правий Alt "

ConfigExclusiveWinLeft
"Левый Win "
"Left Win "
upd:"Left Win "
upd:"Left Win "
upd:"Left Win "
upd:"Left Win "
upd:"Left Win "
"Лівий Win "

ConfigExclusiveWinRight
"Правый Win "
"Right Win "
upd:"Right Win "
upd:"Right Win "
upd:"Right Win "
upd:"Right Win "
upd:"Right Win "
"Правий Win "

ConfigCopyTotal
"Показывать &общий индикатор копирования"
"Show &total copy progress indicator"
"Zobraz. ukazatel celkového stavu &kopírování"
"Zeige Gesamtfor&tschritt beim Kopieren"
"Másolás összesen folyamat&jelző"
"Pokaż &całkowity postęp kopiowania"
"Mostrar indicador de progreso de copia &total"
"Показувати &загальний індикатор копіювання"

ConfigCopyTimeRule
"Показывать информацию о времени &копирования"
"Show cop&ying time information"
"Zobrazovat informace o čase kopírování"
"Zeige Rest&zeit beim Kopieren"
"Má&solási idő mutatva"
"Pokaż informację o c&zasie kopiowania"
"Mostrar información de tiempo de copiado"
"Показувати інформацію про час &копіювання"

ConfigDeleteTotal
"Показывать общий индикатор удаления"
"Show total delete progress indicator"
upd:"Show total delete progress indicator"
upd:"Show total delete progress indicator"
upd:"Show total delete progress indicator"
upd:"Show total delete progress indicator"
"Mostrar indicador de progreso de borrado total"
"Показувати загальний індикатор видалення"

ConfigPgUpChangeDisk
"Использовать Ctrl-PgUp для в&ыбора диска"
"Use Ctrl-Pg&Up to change drive"
"Použít Ctrl-Pg&Up pro změnu disku"
"Strg-Pg&Up wechselt das Laufwerk"
"A Ctrl-Pg&Up meghajtót vált"
"Użyj Ctrl-Pg&Up do zmiany napędu"
"Usar Ctrl-Pg&Up para cambiar unidad"
"Використовувати Ctrl-PgUp для в&ибору диска"

ConfigWindowTitle
"Заголовок окна FAR:"
"FAR window title:"
upd:"FAR window title:"
upd:"FAR window title:"
upd:"FAR window title:"
upd:"FAR window title:"
"Título de ventana de FAR:"
"Заголовок вікна FAR:"

ConfigDlgSetsTitle
l:
"Настройки диалогов"
"Dialog settings"
"Nastavení dialogů"
"Dialoge einrichten"
"Párbeszédablak beállítások"
"Ustawienia okien dialogowych"
"Opciones de diálogo"
"Налаштування діалогів"

ConfigDialogsEditHistory
"&История в строках ввода диалогов"
"&History in dialog edit controls"
"H&istorie v dialozích"
"&Historie in Eingabefelder von Dialogen"
"&Beviteli sor előzmények mentése"
"&Historia w polach edycyjnych"
"&Historial en controles de diálogo de edición"
"&Історія у рядках введення діалогів"

ConfigMaxHistoryCount
"&Макс количество записей:"
"&Max history items:"
upd:"&Max history items:"
upd:"&Max history items:"
upd:"&Max history items:"
upd:"&Max history items:"
upd:"&Max history items:"
"&Макс кількість записів:"

ConfigDialogsEditBlock
"&Постоянные блоки в строках ввода"
"&Persistent blocks in edit controls"
"&Trvalé bloky v editačních polích"
"Dauer&hafte Markierungen in Eingabefelder"
"Maradó b&lokkok a beviteli sorokban"
"&Trwałe bloki podczas edycji"
"&Bloques persistentes en controles de edición"
"&Постійні блоки в рядках введення"

ConfigDialogsDelRemovesBlocks
"Del удаляет б&локи в строках ввода"
"&Del removes blocks in edit controls"
"&Del maže položky v editačních polích"
"&Entf löscht Markierungen"
"A &Del törli a beviteli sorok blokkjait"
"&Del usuwa blok podczas edycji"
"&Del remueve bloques en controles de edición"
"Del видаляє б&локи в рядках введення"

ConfigDialogsAutoComplete
"&Автозавершение в строках ввода"
"&AutoComplete in edit controls"
"Automatické dokončování v editač&ních polích"
"&Automatisches Vervollständigen"
"Beviteli sor a&utomatikus kiegészítése"
"&Autouzupełnianie podczas edycji"
"Autocompl&etar en controles de edición"
"&Автозавершення в рядках введення"

ConfigDialogsEULBsClear
"Backspace &удаляет неизмененный текст"
"&Backspace deletes unchanged text"
"&Backspace maže nezměněný text"
"&Rücktaste (BS) löscht unveränderten Text"
"A Ba&ckspace törli a változatlan szöveget"
"&Backspace usuwa nie zmieniony tekst"
"&Backspace elimina texto no cambiado"
"Backspace &видаляє незмінений текст"

ConfigDialogsMouseButton
"Клик мыши &вне диалога закрывает диалог"
"Mouse click &outside a dialog closes it"
"Kl&iknutí myší mimo dialog ho zavře"
"Dial&og schließen wenn Mausklick ausserhalb"
"&Egérkattintás a párb.ablakon kívül: bezárja"
"&Kliknięcie myszy poza oknem zamyka je"
"Click en ratón afuera del diálogo lo cierra"
"Клік миши &поза діалогом закриває діалог"

ConfigVMenuTitle
l:
"Настройки меню"
"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
"Налаштування меню"

ConfigVMenuLBtnClick
"Клик левой кнопки мыши вне меню"
"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
"Клік лівої кнопки миши поза меню"

ConfigVMenuRBtnClick
"Клик правой кнопки мыши вне меню"
"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
"Клік правої кнопки миши поза меню"

ConfigVMenuMBtnClick
"Клик средней кнопки мыши вне меню"
"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
"Клік середньої кнопки миши поза меню"

ConfigVMenuClickCancel
"Закрыть с отменой"
"Cancel menu"
upd:"Cancel menu"
upd:"Cancel menu"
upd:"Cancel menu"
upd:"Cancel menu"
upd:"Cancel menu"
"Закрити зі скасуванням"

ConfigVMenuClickApply
"Выполнить текущий пункт"
"Execute selected item"
upd:"Execute selected item"
upd:"Execute selected item"
upd:"Execute selected item"
upd:"Execute selected item"
upd:"Execute selected item"
"Виконати поточний пункт"

ConfigVMenuClickIgnore
"Ничего не делать"
"Do nothing"
upd:"Do nothing"
upd:"Do nothing"
upd:"Do nothing"
upd:"Do nothing"
upd:"Do nothing"
"Нічого не робити"

ConfigCmdlineTitle
l:
"Настройки командной строки"
"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
"Opciones de línea de comando"
"Налаштування командного рядка"

ConfigCmdlineEditBlock
"&Постоянные блоки"
"&Persistent blocks"
upd:"Persistent blocks"
upd:"Persistent blocks"
upd:"Persistent blocks"
upd:"Persistent blocks"
"Bloques &persistentes"
"&Постійні блоки"

ConfigCmdlineDelRemovesBlocks
"Del удаляет б&локи"
"&Del removes blocks"
upd:"Del removes blocks"
upd:"Del removes blocks"
upd:"Del removes blocks"
upd:"Del removes blocks"
"&Del remueve bloques"
"Del видаляє б&локи"

ConfigCmdlineAutoComplete
"&Автозавершение"
"&AutoComplete"
upd:"AutoComplete"
upd:"AutoComplete"
upd:"AutoComplete"
upd:"AutoComplete"
"&AutoCompletar"
"&Автозавершення"

ConfigCmdlineWaitKeypress
"&Ожидать нажатие перед закрытием"
"&Wait keypress before close"
upd:"&Wait keypress before close"
upd:"&Wait keypress before close"
upd:"&Wait keypress before close"
upd:"&Wait keypress before close"
upd:"&Wait keypress before close"
"&Очікувать натискання перед закриттям"

ConfigCmdlineWaitKeypress_Never
"Никогда"
"Never"
upd:"Never"
upd:"Never"
upd:"Never"
upd:"Never"
"Never"
"Ніколи"

ConfigCmdlineWaitKeypress_OnError
"При ошибке"
"On error"
upd:"On error"
upd:"On error"
upd:"On error"
upd:"On error"
"On error"
"При помилці"

ConfigCmdlineWaitKeypress_Always
"Всегда"
"Always"
upd:"Always"
upd:"Always"
upd:"Always"
upd:"Always"
"Always"
"Завжди"

ConfigCmdlineUseShell
"Использовать &шелл"
"Use &shell"
upd:"Use shell"
upd:"Use shell"
upd:"Use shell"
upd:"Use shell"
upd:"Use shell"
"Використовувати &шелл"

ConfigCmdlineUsePromptFormat
"Установить &формат командной строки"
"Set command line &prompt format"
"Nastavit formát &příkazového řádku"
"&Promptformat der Kommandozeile"
"Parancssori &prompt formátuma"
"Wy&gląd znaku zachęty linii poleceń"
"Formato para línea de comando (&prompt)"
"Встановити &формат командного рядка"

ConfigCmdlinePromptFormatAdmin
"Администратор"
"Administrator"
upd:"Administrator"
upd:"Administrator"
upd:"Administrator"
upd:"Administrator"
"Administrador"
"Адміністратор"

ConfigAutoCompleteTitle
l:
"Настройка автозавершения"
"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
"Opciones de autocompletar"
"Налаштування автозавершення"

ConfigAutoCompleteExceptions
l:
"Шаблоны &исключений"
"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
"Шаблони &винятковий"

ConfigAutoCompleteShowList
l:
"Показывать &список"
"&Show list"
upd:"&Show list"
upd:"&Show list"
upd:"&Show list"
upd:"&Show list"
"Mo&strar lista"
"Показувати &список"

ConfigAutoCompleteModalList
l:
"&Модальный режим"
"&Modal mode"
upd:"&Modal mode"
upd:"&Modal mode"
upd:"&Modal mode"
upd:"&Modal mode"
"Clase de &Modo"
"&Модальний режим"

ConfigAutoCompleteAutoAppend
l:
"&Подставлять первый подходящий вариант"
"&Append first matched item"
upd:"&Append first matched item"
upd:"&Append first matched item"
upd:"&Append first matched item"
upd:"&Append first matched item"
"&Agregar primer ítem coincidente"
"&підставляти перший відповідний варіант"

ConfigInfoPanelTitle
l:
"Настройка информационной панели"
"InfoPanel settings"
upd:"InfoPanel settings"
upd:"InfoPanel settings"
upd:"InfoPanel settings"
upd:"InfoPanel settings"
"Opciones de panel de información"
"налаштування інформаційної панелі"

ConfigInfoPanelCNTitle
"Формат вывода имени &компьютера"
upd:"ComputerName &format"
upd:"ComputerName &format"
upd:"ComputerName &format"
upd:"ComputerName &format"
upd:"ComputerName &format"
"&Formato NombreComputadora"
"формат виведення імені &комп'ютера"

ConfigInfoPanelCNPhysicalNetBIOS
upd:"Physical NetBIOS"
upd:"Physical NetBIOS"
upd:"Physical NetBIOS"
upd:"Physical NetBIOS"
upd:"Physical NetBIOS"
upd:"Physical NetBIOS"
"NetBios físico"
"Фізичний NetBIOS"

ConfigInfoPanelCNPhysicalDnsHostname
upd:"Physical DNS hostname"
upd:"Physical DNS hostname"
upd:"Physical DNS hostname"
upd:"Physical DNS hostname"
upd:"Physical DNS hostname"
upd:"Physical DNS hostname"
"DNS hostname físico"
"фізичне ім'я хосту DNS"

ConfigInfoPanelCNPhysicalDnsDomain
upd:"Physical DNS domain"
upd:"Physical DNS domain"
upd:"Physical DNS domain"
upd:"Physical DNS domain"
upd:"Physical DNS domain"
upd:"Physical DNS domain"
"Dominio DNS físico"
"фізичний домен DNS"

ConfigInfoPanelCNPhysicalDnsFullyQualified
upd:"Physical DNS fully-qualified"
upd:"Physical DNS fully-qualified"
upd:"Physical DNS fully-qualified"
upd:"Physical DNS fully-qualified"
upd:"Physical DNS fully-qualified"
upd:"Physical DNS fully-qualified"
"DNS calificado físico"
"фізичний DNS повністю кваліфікований"

ConfigInfoPanelCNNetBIOS
upd:"NetBIOS"
upd:"NetBIOS"
upd:"NetBIOS"
upd:"NetBIOS"
upd:"NetBIOS"
upd:"NetBIOS"
"NetBios"
"NetBios"

ConfigInfoPanelCNDnsHostname
upd:"DNS hostname"
upd:"DNS hostname"
upd:"DNS hostname"
upd:"DNS hostname"
upd:"DNS hostname"
upd:"DNS hostname"
"DNS hostname"
"DNS ім'я хоста"

ConfigInfoPanelCNDnsDomain
upd:"DNS domain"
upd:"DNS domain"
upd:"DNS domain"
upd:"DNS domain"
upd:"DNS domain"
upd:"DNS domain"
"Dominio DNS"
"DNS домен"

ConfigInfoPanelCNDnsFullyQualified
upd:"DNS fully-qualified"
upd:"DNS fully-qualified"
upd:"DNS fully-qualified"
upd:"DNS fully-qualified"
upd:"DNS fully-qualified"
upd:"DNS fully-qualified"
"DNS Calificado"
upd:"DNS повністю кваліфікований"

ConfigInfoPanelUNTitle
"Формат вывода имени &пользователя"
upd:"UserName &format"
upd:"UserName &format"
upd:"UserName &format"
upd:"UserName &format"
upd:"UserName &format"
"&Formato nombre de usuario"
"Формат виводу імені &користувача"

ConfigInfoPanelUNUnknown
"По умолчанию"
"Default"
upd:"Default"
upd:"Default"
upd:"Default"
upd:"Default"
"Por defecto"
"За замовчуванням"

ConfigInfoPanelUNFullyQualifiedDN
"Полностью определённое имя домена"
"Fully Qualified Domain Name"
upd:"Fully Qualified Domain Name"
upd:"Fully Qualified Domain Name"
upd:"Fully Qualified Domain Name"
upd:"Fully Qualified Domain Name"
"Nombre dominio completamente calificado"
"повністю визначене ім'я домену"

ConfigInfoPanelUNSamCompatible
upd:"Sam Compatible"
upd:"Sam Compatible"
upd:"Sam Compatible"
upd:"Sam Compatible"
upd:"Sam Compatible"
upd:"Sam Compatible"
"Compatible con Sam"
upd:"Sam Compatible"

ConfigInfoPanelUNDisplay
upd:"Display Name"
upd:"Display Name"
upd:"Display Name"
upd:"Display Name"
upd:"Display Name"
upd:"Display Name"
"Mostrar Nombre"
upd:"Display Name"

ConfigInfoPanelUNUniqueId
"Уникальный идентификатор"
upd:"Unique Id"
upd:"Unique Id"
upd:"Unique Id"
upd:"Unique Id"
upd:"Unique Id"
"ID nico"
"Унікальний ідентифікатор"

ConfigInfoPanelUNCanonical
"Канонический вид"
"Canonical Name"
upd:"Canonical Name"
upd:"Canonical Name"
upd:"Canonical Name"
upd:"Canonical Name"
"Nombre Cannico"
"Каноничний вид"

ConfigInfoPanelUNUserPrincipal
"Основное имя пользователя"
upd:"User Principial Name"
upd:"User Principial Name"
upd:"User Principial Name"
upd:"User Principial Name"
upd:"User Principial Name"
"Nombre principal usuario"
"Основне ім'я користувача"

ConfigInfoPanelUNServicePrincipal
upd:"Service Principal"
upd:"Service Principal"
upd:"Service Principal"
upd:"Service Principal"
upd:"Service Principal"
upd:"Service Principal"
"Servicio principal"
upd:"Service Principal"

ConfigInfoPanelUNDnsDomain
upd:"Dns Domain"
upd:"Dns Domain"
upd:"Dns Domain"
upd:"Dns Domain"
upd:"Dns Domain"
upd:"Dns Domain"
"Dominio DNS"
upd:"Dns Domain"

ViewConfigTitle
l:
"Программа просмотра"
"Viewer"
"Prohlížeč"
"Betrachter"
"Nézőke"
"Podgląd"
"Visor"
"Програма перегляду"

ViewConfigExternalF3
"Запускать внешнюю программу просмотра по F3 вместо Alt-F3"
"Use external viewer for F3 instead of Alt-F3"
upd:"Use external viewer for F3 instead of Alt-F3"
upd:"Use external viewer for F3 instead of Alt-F3"
"Alt-F3 helyett F3 indítja a külső nézőkét"
upd:"Use external viewer for F3 instead of Alt-F3"
"Usar visor externo con F3 en lugar de Alt-F3"
"запускати зовнішню програму перегляду F3 замість Alt-F3"

ViewConfigExternalCommand
"&Команда просмотра:"
"&Viewer command:"
"&Příkaz prohlížeče:"
"Befehl für e&xternen Betracher:"
"Nézőke &parancs:"
"&Polecenie:"
"Comando &visor:"
"&Команда перегляду:"

ViewConfigInternal
" Встроенная программа просмотра "
" Internal viewer "
" Interní prohlížeč "
" Interner Betracher "
" Belső nézőke "
" Podgląd wbudowany "
"Visor interno"
" Вбудована программа перегляду "

ViewConfigSavePos
"&Сохранять позицию файла"
"&Save file position"
"&Ukládat pozici v souboru"
"Dateipositionen &speichern"
"&Fájlpozíció mentése"
"&Zapamiętaj pozycję w pliku"
"&Guardar posición de archivo"
"&Зберігати позицію файлу"

ViewConfigSaveShortPos
"Сохранять &закладки"
"Save &bookmarks"
"Ukládat &záložky"
"&Lesezeichen speichern"
"Könyv&jelzők mentése"
"Zapisz z&akładki"
"Guardar &marcadores"
"Зберігати &закладки"

ViewAutoDetectCodePage
"&Автоопределение кодовой страницы"
"&Autodetect code page"
upd:"&Autodetekovat znakovou sadu"
upd:"Zeichentabelle &automatisch erkennen"
"&Kódlap automatikus felismerése"
"Rozpozn&aj tablicę znaków"
"&Autodetectar tabla de caracteres"
"&Автовизначення кодової сторінки"

ViewConfigTabSize
"Раз&мер табуляции"
"Tab si&ze"
"Velikost &Tabulátoru"
"Ta&bulatorgröße"
"Ta&bulátor mérete"
"Rozmiar &tabulatora"
"Tamaño de &tabulación"
"Роз&мір табуляції"

ViewConfigScrollbar
"Показывать &полосу прокрутки"
"Show scro&llbar"
"Zobrazovat posu&vník"
"Scro&llbalken anzeigen"
"Gör&dítősáv mutatva"
"Pokaż &pasek przewijania"
"Mostrar barra de desp&lazamiento"
"Показувати &полосу прокрутки"

ViewConfigArrows
"Показывать стрелки с&двига"
"Show scrolling arro&ws"
"Zobrazovat &skrolovací šipky"
"P&feile bei Scrollbalken zeigen"
"Gördítőn&yilak mutatva"
"Pokaż strzał&ki przewijania"
"Mostrar flechas de despla&zamiento"
"Показувати стрілки з&суву"


ViewShowKeyBar
"Показывать &линейку клавиш"
"Show &key bar"
"Zobrazovat &zkratkové klávesy"
"Tast&enleiste anzeigen"
"&Funkcióbillentyűk sora mutatva"
"Wyświetl pasek &klawiszy"
"Mostrar barra de &funciones"
"Показувати &лінійку клавиш"

ViewShowTitleBar
"Показывать &заголовок"
"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
"Показувати &заголовок"

ViewConfigPersistentSelection
"Постоянное &выделение"
"&Persistent selection"
"Trvalé &výběry"
"Dauerhafte Text&markierungen"
"&Maradó blokkok"
"T&rwałe zaznaczenie"
"Selección &persistente"
"Постійне &виділення"

ViewConfigDefaultCodePage
"Выберите &кодовую страницу по умолчанию:"
"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
"Виберіть &кодову сторінку за замовчуванням:"

EditConfigTitle
l:
"Редактор"
"Editor"
"Editor"
"Editor"
"Szerkesztő"
"Edytor"
"Editor"
"Редактор"

EditConfigEditorF4
"Запускать внешний редактор по F4 вместо Alt-F4"
"Use external editor for F4 instead of Alt-F4"
upd:"Use external editor for F4 instead of Alt-F4"
upd:"Use external editor for F4 instead of Alt-F4"
"Alt-F4 helyett F4 indítja a külső szerkesztőt"
upd:"Use external editor for F4 instead of Alt-F4"
"Usar editor externo con F4 en lugar de Alt-F4"
"Запускати зовнішній редактор F4 замість Alt-F4"

EditConfigEditorCommand
"&Команда редактирования:"
"&Editor command:"
"&Příkaz editoru:"
"Befehl für e&xternen Editor:"
"&Szerkesztő parancs:"
"&Polecenie:"
"Comando &editor:"
"&Команда редагування:"

EditConfigInternal
" Встроенный редактор "
" Internal editor "
" Interní editor "
" Interner Editor "
" Belső szerkesztő "
" Edytor wbudowany "
"Editor interno"
" Вбудований редактор "

EditConfigExpandTabsTitle
"Преобразовывать &табуляцию:"
"Expand &tabs:"
"Rozšířit Ta&bulátory mezerami"
"&Tabs expandieren:"
"&Tabulátorból szóközök:"
"Zamiana znaków &tabulacji:"
"Expandir &tabulación a espacios"
"Перетворити &табуляцію:"

EditConfigDoNotExpandTabs
l:
"Не преобразовывать табуляцию"
"Do not expand tabs"
"Nerozšiřovat tabulátory mezerami"
"Tabs nicht expandieren"
"Ne helyettesítse a tabulátorokat"
"Nie zamieniaj znaków tabulacji"
"No expandir tabulacines"
"Не перетворювати табуляцію"

EditConfigExpandTabs
"Преобразовывать новые символы табуляции в пробелы"
"Expand newly entered tabs to spaces"
"Rozšířit nově zadané tabulátory mezerami"
"Neue Tabs zu Leerzeichen expandieren"
"Újonnan beírt tabulátorból szóközök"
"Zamień nowo dodane znaki tabulacji na spacje"
"Expandir nuevas tabulaciones ingresadas a espacios"
"Перетворити нові символи табуляції на пробіли"

EditConfigConvertAllTabsToSpaces
"Преобразовывать все символы табуляции в пробелы"
"Expand all tabs to spaces"
"Rozšířit všechny tabulátory mezerami"
"Alle Tabs zu Leerzeichen expandieren"
"Minden tabulátorból szóközök"
"Zastąp wszystkie tabulatory spacjami"
"Expandir todas las tabulaciones a espacios"
"Перетворити всі символи табуляції на пробіли"

EditConfigPersistentBlocks
"&Постоянные блоки"
"&Persistent blocks"
"&Trvalé bloky"
"Dauerhafte Text&markierungen"
"&Maradó blokkok"
"T&rwałe bloki"
"Bloques &persistente"
"&Постійні блоки"

EditConfigDelRemovesBlocks
l:
"Del удаляет б&локи"
"&Del removes blocks"
"&Del maže bloky"
"&Entf löscht Textmark."
"A &Del törli a blokkokat"
"&Del usuwa bloki"
"Del &remueve bloques"
"Del видаляє б&локи"

EditConfigAutoIndent
"Авто&отступ"
"Auto &indent"
"Auto &Odsazování"
"Automatischer E&inzug"
"Automatikus &behúzás"
"Automatyczne &wcięcia"
"Auto &dentar"
"Авто&відступ"

EditConfigSavePos
"&Сохранять позицию файла"
"&Save file position"
"&Ukládat pozici v souboru"
"Dateipositionen &speichern"
"Fájl&pozíció mentése"
"&Zapamiętaj pozycję kursora w pliku"
"&Guardar posición de archivo"
"&Зберігати позицію файлу"

EditConfigSaveShortPos
"Сохранять &закладки"
"Save &bookmarks"
"Ukládat zá&ložky"
"&Lesezeichen speichern"
"Könyv&jelzők mentése"
"Zapisz &zakładki"
"Guardar &marcadores"
"Зберігати &закладки"

EditCursorBeyondEnd
"Ку&рсор за пределами строки"
"&Cursor beyond end of line"
"&Kurzor za koncem řádku"
upd:"&Cursor hinter dem Ende"
"Kurzor a sor&végjel után is"
"&Kursor za końcem linii"
"&Cursor después de fin de línea"
"Ку&рсор за межами рядка"

EditAutoDetectCodePage
"&Автоопределение кодовой страницы"
"&Autodetect code page"
upd:"&Autodetekovat znakovou sadu"
upd:"Zeichentabelle &automatisch erkennen"
"&Kódlap automatikus felismerése"
"Rozpozn&aj tablicę znaków"
"&Autodetectar tabla de caracteres"
"&Автовизначення кодової сторінки"

EditShareWrite
"Разрешить редактирование открытых для записи &файлов"
"Allow editing files ope&ned for writing"
upd:"Allow editing files opened for &writing"
upd:"Allow editing files opened for &writing"
"Írásra m&egnyitott fájlok szerkeszthetők"
upd:"Allow editing files opened for &writing"
"Permitir escritura de archivos abiertos para edición"
"Дозволити редагування відкритих для запису &файлів"

EditLockROFileModification
"Блокировать р&едактирование файлов с атрибутом R/O"
"Lock editing of read-only &files"
"&Zamknout editaci souborů určených jen pro čtení"
"Bearbeiten von &Dateien mit Schreibschutz verhindern"
"Csak olvasható fájlok s&zerkesztése tiltva"
"Nie edytuj plików tylko do odczytu"
"Bloquear edición de &archivos de sólo lectura"
"Блокувати р&едагування файлів з атрибутом R/O"

EditWarningBeforeOpenROFile
"Пре&дупреждать при открытии файла с атрибутом R/O"
"&Warn when opening read-only files"
"&Varovat při otevření souborů určených jen pro čtení"
"Beim Öffnen von Dateien mit Schreibschutz &warnen"
"Figyelmeztet &csak olvasható fájl megnyitásakor"
"&Ostrzeż przed otwieraniem plików tylko do odczytu"
"Advertencia al abrir archivos de sólo lectura"
"Зап&обігати відкриванню файлу з атрибутом R/O"

EditConfigTabSize
"Раз&мер табуляции"
"Tab si&ze"
"Velikost &Tabulátoru"
"Ta&bulatorgröße"
"Tab&ulátor mérete"
"Rozmiar ta&bulatora"
"Tamaño de tabulación"
"Роз&мір табуляції"

EditConfigScrollbar
"Показывать &полосу прокрутки"
"Show scro&llbar"
"Zobr&azovat posuvník"
"Scro&llbalken anzeigen"
"&Gördítősáv mutatva"
"Pokaż %pasek przewijania"
"Mostrar barra de desp&lazamiento"
"Показувати &смугу прокручування"

EditShowWhiteSpace
"Пробельные символы"
"Show white space"
upd:"Show white space"
upd:"Show white space"
upd:"Show white space"
upd:"Show white space"
"Mostrar espacios en blanco"
"Пробільні символи"

EditShowKeyBar
"Показывать &линейку клавиш"
"Show &key bar"
"Zobrazovat &zkratkové klávesy"
"Tast&enleiste anzeigen"
"&Funkcióbillentyűk sora mutatva"
"Wyświetl pasek &klawiszy"
"Mostrar barra de &funciones"
"Показувати &лінійку клавіш"

EditShowTitleBar
"Показывать &заголовок"
"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
"Показувати &заголовок"

EditConfigPickUpWord
"Cлово под к&урсором"
"Pick &up the word"
upd:"Pick &up the word"
upd:"Pick &up the word"
upd:"Pick &up the word"
upd:"Pick &up the word"
"Pick &up the word"
"Слово під к&урсором"

EditConfigDefaultCodePage
"Выберите &кодовую страницу по умолчанию:"
"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
"Виберіть &кодову сторінку за промовчанням:"

NotifConfigTitle
l:
"Уведомления"
"Notifications"
"Notifications"
"Notifications"
"Notifications"
"Notifications"
"Notifications"
"Повідомлення"

NotifConfigOnFileOperation
"Уведомлять о завершении &файловой операции"
"Notify on &file operation completion"
upd:"Notify on &file operation completion"
upd:"Notify on &file operation completion"
upd:"Notify on &file operation completion"
upd:"Notify on &file operation completion"
upd:"Notify on &file operation completion"
"Повідомляти про завершення &файлової операції"

NotifConfigOnConsole
"Уведомлять о завершении &консольной команды"
"Notify on &console command completion"
upd:"Notify on &console command completion"
upd:"Notify on &console command completion"
upd:"Notify on &console command completion"
upd:"Notify on &console command completion"
upd:"Notify on &console command completion"
"Повідомляти про завершення &консольної команди"

NotifConfigOnlyIfBackground
"Уведомлять только когда в &фоне"
"Notify only if in &background"
upd:"Notify only if in &background"
upd:"Notify only if in &background"
upd:"Notify only if in &background"
upd:"Notify only if in &background"
upd:"Notify only if in &background"
"Повідомляти лише коли у &фоні"

ConsoleCommandComplete
"Консольная команда выполнена"
"Console command complete"
upd:"Console command complete"
upd:"Console command complete"
upd:"Console command complete"
upd:"Console command complete"
upd:"Console command complete"
"Консольна команда виконана"

ConsoleCommandFailed
"Консольная команда завершена с ошибкой"
"Console command failed"
upd:"Console command failed"
upd:"Console command failed"
upd:"Console command failed"
upd:"Console command failed"
upd:"Console command failed"
"Консольна команда завершена з помилкою"

FileOperationComplete
"Файловая операция выполнена"
"File operation complete"
upd:"File operation complete"
upd:"File operation complete"
upd:"File operation complete"
upd:"File operation complete"
upd:"File operation complete"
"Файлова операція виконана"

SaveSetupTitle
l:
"Конфигурация"
"Save setup"
"Uložit nastavení"
"Einstellungen speichern"
"Beállítások mentése"
"Zapisz ustawienia"
"Guardar configuración"
"Конфігурація"

SaveSetupAsk1
"Вы хотите сохранить"
"Do you wish to save"
"Přejete si uložit"
"Wollen Sie die aktuellen Einstellungen"
"Elmenti a jelenlegi"
"Czy chcesz zapisać"
"Desea guardar la configuración"
"Ви хочете зберегти"

SaveSetupAsk2
"текущую конфигурацию?"
"current setup?"
"aktuální nastavení?"
"speichern?"
"beállításokat?"
"bieżące ustawienia?"
"actual de FAR?"
"Поточну конфігурацію?"

SaveSetup
"Сохранить"
"Save"
"Uložit"
"Speichern"
"Mentés"
"Zapisz"
"Guardar"
"Зберегти"

CopyDlgTitle
l:
"Копирование"
"Copy"
"Kopírovat"
"Kopieren"
"Másolás"
"Kopiuj"
"Copiar"
"Копіювання"

MoveDlgTitle
"Переименование/Перенос"
"Rename/Move"
"Přejmenovat/Přesunout"
"Verschieben/Umbenennen"
"Átnevezés-Mozgatás"
"Zmień nazwę/przenieś"
"Renombrar/Mover"
"Перейменування/Перенесення"

LinkDlgTitle
"Ссылка"
"Link"
"Link"
"Link erstellen"
"Link létrehozása"
"Dowiąż"
"Enlace"
"Посилання"

CopyAccessMode
"Копировать &режим доступа к файлам"
"Copy files &access mode"
upd:"Copy files &access mode"
upd:"Copy files &access mode"
upd:"Copy files &access mode"
upd:"Copy files &access mode"
upd:"Copy files &access mode"
"Копіювати &режим доступу до файлів"

CopyIfFileExist
"Уже су&ществующие файлы:"
"Already e&xisting files:"
"Již e&xistující soubory:"
"&Dateien überschreiben:"
"Már &létező fájloknál:"
"Dla już &istniejących:"
"Archivos ya e&xistentes:"
"Вже іс&нуючі файли:"

CopyAsk
"&Запрос действия"
"&Ask"
"Ptát s&e"
"Fr&agen"
"Kér&dez"
"&Zapytaj"
"Pregunt&ar"
"&Запит дії"

CopyAskRO
"Запрос подтверждения для &R/O файлов"
"Also ask on &R/O files"
"Ptát se také na &R/O soubory"
"Bei Dateien mit Sch&reibschutz fragen"
"&Csak olvasható fájloknál is kérdez"
"&Pytaj także o pliki tylko do odczytu"
"Preguntar también en archivos de Sólo Lectu&ra"
"Запит підтвердження для &R/O файлів"

CopyOnlyNewerFiles
"Только &новые/обновлённые файлы"
"Only ne&wer file(s)"
"Pouze &novější soubory"
"Nur &neuere Dateien"
"Cs&ak az újabb fájlokat"
"Tylko &nowsze pliki"
"Sólo archivo(s) más nuev&os"
"Тільки &нові/оновлені файли"

LinkType
"&Тип ссылки:"
"Link t&ype:"
"&Typ linku:"
"Linkt&yp:"
"Link &típusa:"
"&Typ linku:"
"Tipo de &enlace"
"&Тип посилання:"

LinkTypeJunction
"&связь каталогов"
"directory &junction"
"křížení a&dresářů"
"Ordner&knotenpunkt"
"Mappa &csomópont"
"directory &junction"
"unión de directorio"
"&зв'язок каталогів"

LinkTypeHardlink
"&жёсткая ссылка"
"&hard link"
"&pevný link"
"&Hardlink"
"&Hardlink"
"link &trwały"
"enlace duro"
"&жорстке посилання"

LinkTypeSymlink
"си&мволическая ссылка"
"&symbolic link"
"symbolický link"
"Symbolischer Link"
"Szimbolikus link"
"link symboliczny"
"enlace simbólico"
"си&мволічне посилання"

LinkTypeSymlinkFile
"символическая ссылка (&файл)"
"symbolic link (&file)"
"symbolický link (&soubor)"
"Symbolischer Link (&Datei)"
"Szimbolikus link (&fájl)"
"link symboliczny (do &pliku)"
"enlace simbólico (&archivo)"
"символічне посилання (&файл)"

LinkTypeSymlinkDirectory
"символическая ссылка (&папка)"
"symbolic link (fol&der)"
"symbolický link (&adresář)"
"Symbolischer Link (Or&dner)"
"Szimbolikus link (&mappa)"
"link symboliczny (do &folderu)"
"enlace simbólico (&directorios)"
"символічна посилання (&тека)"

CopySymLinkText
"Символические сс&ылки:"
"With s&ymlinks:"
upd:"With s&ymlinks:"
upd:"With s&ymlinks:"
upd:"With s&ymlinks:"
upd:"With s&ymlinks:"
upd:"With s&ymlinks:"
"Символічні по&силання:"

LinkCopyAsIs
"Всегда копировать &ссылку"
"Always copy &link"
upd:"Always copy &link"
upd:"Always copy &link"
upd:"Always copy &link"
upd:"Always copy &link"
upd:"Always copy &link"
"Завжди копіювати &посилання"

LinkCopySmart
"&Умно копировать ссылку или файл"
"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
"&Розумно копіювати посилання або файл"

LinkCopyContent
"Копировать как &файл"
"Always copy target &file"
upd:"Always copy target &file"
upd:"Always copy target &file"
upd:"Always copy target &file"
upd:"Always copy target &file"
upd:"Always copy target &file"
"Копіювати як &файл"

CopySparseFiles
"Создавать &разреженные файлы"
"Produce &sparse files"
upd:"Produce &sparse files"
upd:"Produce &sparse files"
upd:"Produce &sparse files"
upd:"Produce &sparse files"
upd:"Produce &sparse files"
"Створювати &розріджені файли"

CopyUseCOW
"Использовать копирование-&при-записи если возможно"
"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
"Використовувати копіювання-&та-записи якщо це можливо"

CopyMultiActions
"Обр&абатывать несколько имён файлов"
"Process &multiple destinations"
"&Zpracovat více míst určení"
"&Mehrere Ziele verarbeiten"
"Tö&bbszörös cél létrehozása"
"Przetwarzaj &wszystkie cele"
"Procesar &múltiples destinos"
"Виб&рати кілька імен файлів"

CopyDlgCopy
"&Копировать"
"&Copy"
"&Kopírovat"
"&Kopieren"
"&Másolás"
"&Kopiuj"
"&Copiar"
"&Копіювати"

CopyDlgTree
"F10-&Дерево"
"F10-&Tree"
"F10-&Strom"
"F10-&Baum"
"F10-&Fa"
"F10-&Drzewo"
"F10-&Arbol"
"F10-&Дерево"

CopyDlgCancel
"&Отменить"
"Ca&ncel"
"&Storno"
"Ab&bruch"
"Még&sem"
"&Anuluj"
"Ca&ncelar"
"&Скасувати"

CopyDlgRename
"&Переименовать"
"&Rename"
"Přej&menovat"
"&Umbenennen"
"Át&nevez-Mozgat"
"&Zmień nazwę"
"&Renombrar"
"&Перейменувати"

CopyDlgLink
"&Создать ссылку"
"Create &link"
upd:"Create &link"
upd:"Create &link"
upd:"Create &link"
upd:"Create &link"
upd:"Create &link"
"&Створити посилання"

CopyDlgTotal
"Всего"
"Total"
"Celkem"
"Gesamt"
"Összesen"
"Razem"
"Total"
"Всього"

CopyScanning
"Сканирование папок..."
"Scanning folders..."
"Načítání adresářů..."
"Scanne Ordner..."
"Mappák olvasása..."
"Przeszukuję katalogi..."
"Explorando directorios..."
"Сканування тек..."

CopyPrepareSecury
"Применение прав доступа..."
"Applying access rights..."
"Nastavuji přístupová práva..."
"Anwenden der Zugriffsrechte..."
"Hozzáférési jogok alkalmazása..."
"Ustawianie praw dostępu..."
"Aplicando derechos de acceso..."
"Застосування прав доступу..."

CopyUseFilter
"Исполь&зовать фильтр"
"&Use filter"
"P&oužít filtr"
"Ben&utze Filter"
"Szűrő&vel"
"&Użyj filtra"
"&Usar filtros"
"Викори&стовувати фільтр"

CopySetFilter
"&Фильтр"
"Filt&er"
"Filt&r"
"Filt&er"
"S&zűrő"
"Filt&r"
"Fi&ltro"
"&Фільтр"

CopyFile
l:
"Копировать"
"Copy"
"Kopírovat"
"Kopiere"
upd:"másolása"
"Skopiuj"
"Copiar"
"Копіювати"

MoveFile
"Переименовать или перенести"
"Rename or move"
"Přejmenovat nebo přesunout"
"Verschiebe"
upd:"átnevezése-mozgatása"
"Zmień nazwę lub przenieś"
"Renombrar o mover"
"Перейменувати або перенести"

LinkFile
"Создать ссылку на"
"Create link to"
upd:"Create link to"
upd:"Create link to"
upd:"Create link to"
upd:"Create link to"
upd:"Create link to"
"Створити посилання на"

CopyFiles
"Копировать %d элемент%ls"
"Copy %d item%ls"
"Kopírovat %d polož%ls"
"Kopiere %d Objekt%ls"
" %d elem másolása"
"Skopiuj %d plików"
"Copiar %d ítem%ls"
"Копіювати %d елемент%ls"

MoveFiles
"Переименовать или перенести %d элемент%ls"
"Rename or move %d item%ls"
"Přejmenovat nebo přesunout %d polož%ls"
"Verschiebe %d Objekt%ls"
" %d elem átnevezése-mozgatása"
"Zmień nazwę lub przenieś %d plików"
"Renombrar o mover %d ítem%ls"
"Перейменувати або перенести %d елемент %ls"

LinkFiles
"Создать ссылки на %d элемент%ls"
"Create links to %d item%ls"
upd:"Create links to %d item%ls"
upd:"Create links to %d item%ls"
upd:"Create links to %d item%ls"
upd:"Create links to %d item%ls"
upd:"Create links to %d item%ls"
"Створити посилання на %d елемент %ls"

CMLTargetTO
" &в:"
" t&o:"
" d&o:"
" na&ch:"
" ide:"
" d&o:"
" &hacia:"
" &в:"

CMLTargetIN
" &в:"
" in:"
upd:" &in:"
upd:" &in:"
upd:" &in:"
upd:" &in:"
upd:" &in:"
" &в:"

CMLItems0
""
""
"u"
""
""
""
""
""

CMLItemsA
"а"
"s"
"ek"
"e"
""
"s"
"s"
"а"


CMLItemsS
"ов"
"s"
"ky"
"e"
""
"s"
"s"
"ів"

CopyIncorrectTargetList
l:
"Указан некорректный список целей"
"Incorrect target list"
"Nesprávný seznam cílů"
"Ungültige Liste von Zielen"
"Érvénytelen céllista"
"Błędna lista wynikowa"
"Lista destino incorrecta"
"Вказано некоректний список цілей"

CopyCopyingTitle
l:
"Копирование"
"Copying"
"Kopíruji"
"Kopieren"
"Másolás"
"Kopiowanie"
"Copiando"
"Копіювання"

CopyMovingTitle
"Перенос"
"Moving"
"Přesouvám"
"Verschieben"
"Mozgatás"
"Przenoszenie"
"Moviendo"
"Перенесення"

CopyCannotFind
l:
"Файл не найден"
"Cannot find the file"
"Nelze nalézt soubor"
"Folgende Datei kann nicht gefunden werden:"
"A fájl nem található:"
"Nie mogę odnaleźć pliku"
"No se puede encontrar el archivo"
"Файл не знайдено"

CannotCopyFolderToItself1
l:
"Нельзя копировать папку"
"Cannot copy the folder"
"Nelze kopírovat adresář"
"Folgender Ordner kann nicht kopiert werden:"
"A mappa:"
"Nie można skopiować katalogu"
"No se puede copiar el directorio"
"Не можна копіювати папку"

CannotCopyFolderToItself2
"в саму себя"
"onto itself"
"sám na sebe"
"Ziel und Quelle identisch."
"nem másolható önmagába/önmagára"
"do niego samego"
"en sí mismo"
"у саму себе"

CannotCopyToTwoDot
l:
"Нельзя копировать файл или папку"
"You may not copy files or folders"
"Nelze kopírovat soubory nebo adresáře"
"Kopieren von Dateien oder Ordnern ist maximal"
"Nem másolhatja a fájlt vagy mappát"
"Nie można skopiować plików"
"Usted no puede copiar archivos o directorios"
"Не можна копіювати файл або папку"

CannotMoveToTwoDot
"Нельзя перемещать файл или папку"
"You may not move files or folders"
"Nelze přesunout soubory nebo adresáře"
"Verschieben von Dateien oder Ordnern ist maximal"
"Nem mozgathatja a fájlt vagy mappát"
"Nie można przenieść plików"
"Usted no puede mover archivos o directorios"
"Не можна переміщувати файл або папку"

CannotCopyMoveToTwoDot
"выше корневого каталога"
"higher than the root folder"
"na vyšší úroveň než kořenový adresář"
"bis zum Wurzelverzeichnis möglich."
"a gyökér fölé"
"na poziom wyższy niż do korzenia"
"más alto que el directorio raíz"
"вище кореневого каталогу"

CopyCannotCreateFolder
l:
"Ошибка создания папки"
"Cannot create the folder"
"Nelze vytvořit adresář"
"Folgender Ordner kann nicht erstellt werden:"
"A mappa nem hozható létre:"
"Nie udało się utworzyć katalogu"
"No se puede crear el directorio"
"Помилка створення папки"

CopyCannotChangeFolderAttr
"Невозможно установить атрибуты для папки"
"Failed to set folder attributes"
"Nastavení atributů adresáře selhalo"
"Fehler beim Setzen der Ordnerattribute"
"A mappa attribútumok beállítása sikertelen"
"Nie udało się ustawić atrybutów folderu"
"Error al poner atributos en directorio"
"Неможливо встановити атрибути для папки"

CopyCannotRenameFolder
"Невозможно переименовать папку"
"Cannot rename the folder"
"Nelze přejmenovat adresář"
"Folgender Ordner kann nicht umbenannt werden:"
"A mappa nem nevezhető át:"
"Nie udało się zmienić nazwy katalogu"
"No se puede renombrar el directorio"
"Неможливо перейменувати папку"

CopyIgnore
"&Игнорировать"
"&Ignore"
"&Ignorovat"
"&Ignorieren"
"Mé&gis"
"&Ignoruj"
"&Ignorar"
"&Ігнорувати"

CopyIgnoreAll
"Игнорировать &все"
"Ignore &All"
"Ignorovat &vše"
"&Alle ignorieren"
"Min&d"
"Ignoruj &wszystko"
"Ignorar &Todo"
"Ігнорувати &все"

CopyRetry
"&Повторить"
"&Retry"
"&Opakovat"
"Wiede&rholen"
"Ú&jra"
"&Ponów"
"&Reiterar"
"&Повторити"

CopySkip
"П&ропустить"
"&Skip"
"&Přeskočit"
"Ausla&ssen"
"&Kihagy"
"&Omiń"
"&Omitir"
"П&ропустити"

CopySkipAll
"&Пропустить все"
"S&kip all"
"Př&eskočit vše"
"Alle aus&lassen"
"Mi&nd"
"Omiń w&szystkie"
"O&mitir todos"
"&Пропустити все"

CopyCancel
"&Отменить"
"&Cancel"
"&Storno"
"Abb&rechen"
"Még&sem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

CopyCannotCreateLink
l:
"Ошибка создания ссылки"
"Cannot create the link"
"Nelze vytvořit symbolický link"
"Folgender Link kann nicht erstellt werden:"
"A link nem hozható létre:"
"Nie udało się utworzyć linku"
"No se puede crear el enlace simbólico"
"Помилка створення посилання"

CopyFolderNotEmpty
"Папка назначения должна быть пустой"
"Target folder must be empty"
"Cílový adresář musí být prázdný"
"Zielordner muss leer sein."
"A célmappának üresnek kell lennie"
"Folder wynikowy musi być pusty"
"Directorio destino debe estar vacío"
"Папка призначення має бути порожньою"

CopyCannotCreateJunctionToFile
"Невозможно создать связь. Файл уже существует:"
"Cannot create junction. The file already exists:"
"Nelze vytvořit křížový odkaz. Soubor již existuje:"
"Knotenpunkt wurde nicht erstellt. Datei existiert bereits:"
"A csomópont nem hozható létre. A fájl már létezik:"
"Nie można utworzyć połączenia - plik już istnieje:"
"No se puede unir. El archivo ya existe:"
"Неможливо створити зв'язок. Файл уже існує:"

CopyCannotCreateSymlinkAskCopyContents
"Невозможно создать связь. Копировать данные вместо связей?"
"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
"Неможливо створити зв'язок. Копіювати дані замість зв'язків?"

CopyCannotCreateVolMount
l:
"Ошибка монтирования диска"
"Volume mount points error"
"Chyba připojovacích svazků"
"Fehler im Mountpoint des Datenträgers"
"Kötet mountpont hiba"
"Błąd montowania napędu"
"Error en puntos de montaje de volumen"
"Помилка монтування диска"

CopyMountVolFailed
"Ошибка при монтировании диска '%ls'"
"Attempt to volume mount '%ls'"
"Pokus o připojení svazku '%ls'"
"Versuch Datenträger '%ls' zu aktivieren"
""%ls" kötet mountolása"
"Nie udało się zamontować woluminu '%ls'"
"Intento de montaje de volumen '%ls'"
"Помилка при монтуванні диска '%ls'"

CopyMountVolFailed2
"на '%ls'"
"at '%ls' failed"
"na '%ls' selhal"
"fehlgeschlagen bei '%ls'"
"nem sikerült: "%ls""
"w '%ls' nie udało się"
"a '%ls' ha fallado"
"на '%ls'"

CopyMountName
"disk_%c"
"Disk_%c"
"Disk_%c"
"Disk_%c"
"Disk_%c"
"Disk_%c"
"Disco_%c"
"disk_%c"

CannotCopyFileToItself1
l:
"Нельзя копировать файл"
"Cannot copy the file"
"Nelze kopírovat soubor"
"Folgende Datei kann nicht kopiert werden:"
"A fájl"
"Nie można skopiować pliku"
"Imposible copiar el archivo"
"Не можна копіювати файл"

CannotCopyFileToItself2
"в самого себя"
"onto itself"
"sám na sebe"
"Ziel und Quelle identisch."
"nem másolható önmagára"
"do niego samego"
"en sí mismo"
"у самого себе"

CopyStream1
l:
"Исходный файл содержит более одного потока данных,"
"The source file contains more than one data stream."
"Zdrojový soubor obsahuje více než jeden datový proud."
"Die Quelldatei enthält mehr als einen Datenstream"
"A forrásfájl több stream-et tartalmaz,"
"Plik źródłowy zawiera więcej niż jeden strumień danych."
"El archivo origen contiene más de un flujo de datos."
"Вихідний файл містить більше одного потоку даних,"

CopyStream2
"но вы не используете системную функцию копирования."
"but since you do not use a system copy routine."
"protože nepoužíváte systémovou kopírovací rutinu."
"aber Sie verwenden derzeit nicht die systemeigene Kopierroutine."
"de nem a rendszer másolórutinját használja."
"ale ze względu na rezygnację z systemowej procedury kopiowania."
"pero desde que usted no usa la rutina de copia del sistema."
"але ви не використовуєте системну функцію копіювання."

CopyStream3
"но том назначения не поддерживает этой возможности."
"but the destination volume does not support this feature."
"protože cílový svazek nepodporuje tuto vlastnost."
"aber der Zieldatenträger unterstützt diese Fähigkeit nicht."
"de a célkötet nem támogatja ezt a lehetőséget."
"ale napęd docelowy nie obsługuje tej funkcji."
"pero el volumen de destino no soporta esta opción."
"але тому призначення не підтримує цієї можливості."

CopyStream4
"Часть сведений не будет сохранена."
"Some data will not be preserved as a result."
"To bude mít za následek, že některá data nebudou uchována."
"Ein Teil der Daten bleiben daher nicht erhalten."
"Az adatok egy része el fog veszni."
"Nie wszystkie dane zostaną zachowane."
"Algunos datos no serán preservados como un resultado."
"Частина відомостей не буде збережена."

CopyDirectoryOrFile
l:
"Подразумевается имя папки или файла?"
"Does it specify a folder name or file name?"
upd:"Does it specify a folder name or file name?"
upd:"Does it specify a folder name or file name?"
upd:"Does it specify a folder name or file name?"
upd:"Does it specify a folder name or file name?"
"Si especifica nombre de carpeta o nombre de archivo?"
"Має на увазі ім'я теки або файлу?"

CopyDirectoryOrFileDirectory
"Папка"
"Folder"
upd:"Folder"
upd:"Folder"
upd:"Folder"
upd:"Folder"
"Carpeta"
"Тека"

CopyDirectoryOrFileFile
"Файл"
"File"
upd:"File"
upd:"File"
upd:"File"
upd:"File"
"Archivo"
"Файл"

CopyFileExist
l:
"Файл уже существует"
"File already exists"
"Soubor již existuje"
"Datei existiert bereits"
"A fájl már létezik:"
"Plik już istnieje"
"El archivo ya existe"
"Файл вже існує"

CopySource
"&Новый"
"&New"
"&Nový"
"&Neue Datei"
"Ú&j verzió:"
"&Nowy"
"Nuevo"
"&Новий"

CopyDest
"Су&ществующий"
"E&xisting"
"E&xistující"
"Be&stehende Datei"
"Létező &verzió:"
"&Istniejący"
"Existente"
"Іс&нуючий"

CopyOverwrite
"В&место"
"&Overwrite"
"&Přepsat"
"Über&schr."
"&Felülír"
"N&adpisz"
"&Sobrescribir"
"З&амість"

CopySkipOvr
"&Пропустить"
"&Skip"
"Přes&kočit"
"Über&spr."
"&Kihagy"
"&Omiń"
"&Omitir"
"&Пропустити"

CopyAppend
"&Дописать"
"A&ppend"
"Př&ipojit"
"&Anhängen"
"Hoz&záfűz"
"&Dołącz"
"A&gregar"
"&Дописати"

CopyResume
"Возоб&новить"
"&Resume"
"Po&kračovat"
"&Weiter"
"Fol&ytat"
"Ponó&w"
"&Resumir"
"Від&новити"

CopyRename
"&Имя"
"R&ename"
upd:"R&ename"
upd:"R&ename"
"Á&tnevez"
upd:"R&ename"
"Renombrar"
"&Ім'я"

CopyCancelOvr
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"&Відмінити"

CopyRememberChoice
"&Запомнить выбор"
"&Remember choice"
"Zapama&tovat volbu"
"Auswahl me&rken"
"Mind&ent a kiválasztott módon"
"&Zapamiętaj ustawienia"
"&Recordar elección"
"&Запам'ятати вибір"

CopyRenameTitle
"Переименование"
"Rename"
upd:"Rename"
upd:"Rename"
"Átnevezés"
upd:"Rename"
"Renombrar"
"Перейменування"

CopyRenameText
"&Новое имя:"
"&New name:"
upd:"&New name:"
upd:"&New name:"
"Ú&j név:"
upd:"&New name:"
"&Nuevo nombre:"
"&Нове ім'я:"

CopyFileRO
l:
"Файл имеет атрибут \"Только для чтения\""
"The file is read only"
"Soubor je určen pouze pro čtení"
"Folgende Datei ist schreibgeschützt:"
"A fájl csak olvasható:"
"Ten plik jest tylko-do-odczytu"
"El archivo es de sólo lectura"
"Файл має атрибут \"Тільки для читання\""

CopyAskDelete
"Вы хотите удалить его?"
"Do you wish to delete it?"
"Opravdu si ho přejete smazat?"
"Wollen Sie sie dennoch löschen?"
"Biztosan törölni akarja?"
"Czy chcesz go usunąć?"
"Desea borrarlo igual?"
"Ви хочете видалити його?"

CopyDeleteRO
"&Удалить"
"&Delete"
"S&mazat"
"&Löschen"
"&Törli"
"&Usuń"
"&Borrar"
"&Вилучити"

CopyDeleteAllRO
"&Все"
"&All"
"&Vše"
"&Alle Löschen"
"Min&det"
"&Wszystkie"
"&Todos"
"&Усе"

CopySkipRO
"&Пропустить"
"&Skip"
"Přes&kočit"
"Über&springen"
"&Kihagyja"
"&Omiń"
"&Omitir"
"&Пропустити"

CopySkipAllRO
"П&ропустить все"
"S&kip all"
"Př&eskočit vše"
"A&lle überspringen"
"Mind&et"
"O&miń wszystkie"
"O&mitir todos"
"П&ропустити все"

CopyCancelRO
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

CannotCopy
l:
"Ошибка копирования"
"Cannot copy"
"Nelze kopírovat"
"Konnte nicht kopieren"
"Nem másolható"
"Nie mogę skopiować"
"No se puede copiar %ls"
"Помилка копіювання"

CannotMove
"Ошибка переноса"
"Cannot move"
"Nelze přesunout"
"Konnte nicht verschieben"
"Nem mozgatható"
"Nie mogę przenieść"
"No se puede mover %ls"
"Помилка перенесення"

CannotLink
"Ошибка создания ссылки"
"Cannot link"
"Nelze linkovat"
"Konnte nicht verlinken"
"Nem linkelhető"
"Nie mogę dowiązać"
"No se puede enlazar %ls"
"Помилка створення посилання"

CannotCopyTo
"в"
"to"
"do"
"nach"
"ide:"
"do"
"hacia %ls"
"в"

CopyEncryptWarn1
"Файл"
"The file"
"Soubor"
"Die Datei"
"A fájl"
"Plik"
"El archivo"
"Файл"

CopyEncryptWarn2
"нельзя скопировать или переместить, не потеряв его шифрование."
"cannot be copied or moved without losing its encryption."
"nemůže být zkopírován nebo přesunut bez ztráty jeho šifrování."
"kann nicht bewegt werden ohne ihre Verschlüsselung zu verlieren."
"csak titkosítása elvesztésével másolható vagy mozgatható."
"nie może zostać skopiowany/przeniesiony bez utraty szyfrowania"
"no puede copiarse o moverse sin perder el cifrado."
"Не можна скопіювати або перемістити, не втративши його шифрування."

CopyEncryptWarn3
"Можно пропустить эту ошибку или отменить операцию."
"You can choose to ignore this error and continue, or cancel."
"Můžete tuto chybu ignorovat a pokračovat, nebo operaci ukončit."
"Sie können dies ignorieren und fortfahren oder abbrechen."
"Ennek ellenére folytathatja vagy felfüggesztheti."
"Możesz zignorować błąd i kontynuować lub anulować operację."
"Usted puede ignorar este error y continuar, o cancelar."
"Можна пропустити цю помилку або скасувати операцію."

CopyReadError
l:
"Ошибка чтения данных из"
"Cannot read data from"
"Nelze číst data z"
"Kann Daten nicht lesen von"
"Nem olvasható adat innen:"
"Nie mogę odczytać danych z"
"No se puede leer datos desde"
"Помилка читання даних з"

CopyWriteError
"Ошибка записи данных в"
"Cannot write data to"
"Nelze zapsat data do"
"Dann Daten nicht schreiben in"
"Nem írható adat ide:"
"Nie mogę zapisać danych do"
"No se puede escribir datos hacia"
"Помилка запису даних"

CopyProcessed
l:
"Обработано файлов: %d"
"Files processed: %d"
"Zpracováno souborů: %d"
"Dateien verarbeitet: %d"
" %d fájl kész"
"Przetworzonych plików: %d"
"Archivos procesados: %d"
"Оброблено файли: %d"

CopyProcessedTotal
"Обработано файлов: %d из %d"
"Files processed: %d of %d"
"Zpracováno souborů: %d z %d"
"Dateien verarbeitet: %d von %d"
" %d fájl kész %d fájlból"
"Przetworzonych plików: %d z %d"
"Archivos procesados: %d de %d"
"Оброблено файли: %d з %d"

CopyMoving
"Перенос файла"
"Moving the file"
"Přesunuji soubor"
"Verschiebe die Datei"
"Fájl mozgatása"
"Przenoszę plik"
"Moviendo el archivo"
"Перенесення файлу"

CopyCopying
"Копирование файла"
"Copying the file"
"Kopíruji soubor"
"Kopiere die Datei"
"Fájl másolása"
"Kopiuję plik"
"Copiando el archivo"
"Копіювання файлу"

CopyTo
"в"
"to"
"do"
"nach"
"ide:"
"do"
"Hacia"
"в"

CopyErrorDiskFull
l:
"Диск заполнен. Вставьте следующий"
"Disk full. Insert next"
"Disk je plný. Vložte dalšíí"
"Datenträger voll. Bitte nächsten einlegen"
"A lemez megtelt, kérem a következőt"
"Dysk pełny. Włóż następny"
"Disco lleno. Inserte el próximo"
"Диск заповнений. Вставте наступний"

DeleteTitle
l:
"Удаление"
"Delete"
"Smazat"
"Löschen"
"Törlés"
"Usuń"
"Borrar"
"Видалення"

AskDeleteFolder
"Вы хотите удалить папку"
"Do you wish to delete the folder"
"Přejete si smazat adresář"
"Wollen Sie den Ordner löschen"
"Törölni akarja a mappát?"
"Czy chcesz wymazać katalog"
"Desea borrar el directorio"
"Ви хочете видалити теку"

AskDeleteFile
"Вы хотите удалить файл"
"Do you wish to delete the file"
"Přejete si smazat soubor"
"Wollen Sie die Datei löschen"
"Törölni akarja a fájlt?"
"Czy chcesz usunąć plik"
"Desea borrar el archivo"
"Ви хочете видалити файл"

AskDelete
"Вы хотите удалить"
"Do you wish to delete"
"Přejete si smazat"
"Wollen Sie folgendes Objekt löschen"
"Törölni akar"
"Czy chcesz usunąć"
"Desea borrar"
"Ви хочете видалити"

AskDeleteRecycleFolder
"Вы хотите переместить в Корзину папку"
"Do you wish to move to the Recycle Bin the folder"
"Přejete si přesunout do Koše adresář"
"Wollen Sie den Ordner in den Papierkorb verschieben"
"A Lomtárba akarja dobni a mappát?"
"Czy chcesz przenieść katalog do Kosza"
"Desea mover hacia la papelera de reciclaje el directorio"
"Ви хочете перемістити в Кошик теку"

AskDeleteRecycleFile
"Вы хотите переместить в Корзину файл"
"Do you wish to move to the Recycle Bin the file"
"Přejete si přesunout do Koše soubor"
"Wollen Sie die Datei in den Papierkorb verschieben"
"A Lomtárba akarja dobni a fájlt?"
"Czy chcesz przenieść plik do Kosza"
"Desea mover hacia la papelera de reciclaje el archivo"
"Ви хочете перемістити в Кошик файл"

AskDeleteRecycle
"Вы хотите переместить в Корзину"
"Do you wish to move to the Recycle Bin"
"Přejete si přesunout do Koše"
"Wollen Sie das Objekt in den Papierkorb verschieben"
"A Lomtárba akar dobni"
"Czy chcesz przenieść do Kosza"
"Desea mover hacia la papelera de reciclaje"
"Ви хочете перемістити в Кошик"

DeleteWipeTitle
"Уничтожение"
"Wipe"
"Vymazat"
"Sicheres Löschen"
"Kisöprés"
"Wymaż"
"Limpiar"
"Знищення"

AskWipeFolder
"Вы хотите уничтожить папку"
"Do you wish to wipe the folder"
"Přejete si vymazat adresář"
"Wollen Sie den Ordner sicher löschen"
"Ki akarja söpörni a mappát?"
"Czy chcesz wymazać katalog"
"Desea limpiar el directorio"
"Ви хочете знищити теку"

AskWipeFile
"Вы хотите уничтожить файл"
"Do you wish to wipe the file"
"Přejete si vymazat soubor"
"Wollen Sie die Datei sicher löschen"
"Ki akarja söpörni a fájlt?"
"Czy chcesz wymazać plik"
"Desea limpiar el archivo"
"Ви хочете знищити файл"

AskWipe
"Вы хотите уничтожить"
"Do you wish to wipe"
"Přejete si vymazat"
"Wollen Sie das Objekt sicher löschen"
"Ki akar söpörni"
"Czy chcesz wymazać"
"Desea limpiar"
"Ви хочете знищити"

DeleteLinkTitle
"Удаление ссылки"
"Delete link"
"Smazat link"
"Link löschen"
"Link törlése"
"Usuń link"
"Borrar enlace"
"Видалення посилання"

AskDeleteLink
"является ссылкой на"
"is a symbolic link to"
"je symbolicky link na"
"ist ein symbolischer Link auf"
"szimlinkelve ide:"
"jest linkiem symbolicznym do"
"es un enlace simbólico al"
"є посиланням на"

AskDeleteLinkFolder
"папку"
"folder"
"adresář"
"Ordner"
"mappa"
"folder"
"directorio"
"теку"

AskDeleteLinkFile
"файл"
"file"
"soubor"
"Date"
"fájl"
"plik"
"archivo"
"файл"

AskDeleteItems
"%d элемент%ls"
"%d item%ls"
"%d polož%ls"
"%d Objekt%ls"
"%d elemet%ls"
"%d plik%ls"
"%d ítem%ls"
"%d елемент%ls"

AskDeleteItems0
""
""
"ku"
""
""
""
""
""

AskDeleteItemsA
"а"
"s"
"ky"
"e"
""
"i"
"s"
"а"

AskDeleteItemsS
"ов"
"s"
"ek"
"e"
""
"ów"
"s"
"ів"

DeleteFolderTitle
l:
"Удаление папки "
"Delete folder"
"Smazat adresář"
"Ordner löschen"
"Mappa törlése"
"Usuń folder"
"Borrar directorio"
"Видалення теки "

WipeFolderTitle
"Уничтожение папки "
"Wipe folder"
"Vymazat adresář"
"Ordner sicher löschen"
"Mappa kisöprése"
"Wymaż folder"
"Limpiar directorio"
"Знищення теки "

DeleteFilesTitle
"Удаление файлов"
"Delete files"
"Smazat soubory"
"Dateien löschen"
"Fájlok törlése"
"Usuń pliki"
"Borrar archivos"
"Видалення файлів"

WipeFilesTitle
"Уничтожение файлов"
"Wipe files"
"Vymazat soubory"
"Dateien sicher löschen"
"Fájlok kisöprése"
"Wymaż pliki"
"Limpiar archivos"
"Знищення файлів"

DeleteFolderConfirm
"Данная папка будет удалена:"
"The following folder will be deleted:"
"Následující adresář bude smazán:"
"Folgender Ordner wird gelöscht:"
"A mappa törlődik:"
"Następujący folder zostanie usunięty:"
"El siguiente directorio será borrado:"
"Ця тека буде видалена:"

WipeFolderConfirm
"Данная папка будет уничтожена:"
"The following folder will be wiped:"
"Následující adresář bude vymazán:"
"Folgender Ordner wird sicher gelöscht:"
"A mappa kisöprődik:"
"Następujący folder zostanie wymazany:"
"El siguiente directorio será limpiado:"
"Ця тека буде знищена:"

DeleteWipe
"Уничтожить"
"Wipe"
"Vymazat"
"Sicheres Löschen"
"Kisöpör"
"Wymaż"
"Limpiar"
"Знищити"

DeleteRecycle
"Переместить"
"Move"
upd:"Move"
upd:"Move"
upd:"Move"
upd:"Move"
upd:"Move"
"Перемістити"

DeleteFileDelete
"&Удалить"
"&Delete"
"S&mazat"
"&Löschen"
"&Töröl"
"&Usuń"
"&Borrar"
"&Вилучити"

DeleteFileWipe
"&Уничтожить"
"&Wipe"
"V&ymazat"
"&Sicher löschen"
"Kisö&pör"
"&Wymaż"
"&Limpiar"
"&Знищити"

DeleteFileAll
"&Все"
"&All"
"&Vše"
"&Alle"
"Min&det"
"&wszystkie"
"&Todos"
"&Усе"

DeleteFileSkip
"&Пропустить"
"&Skip"
"Přes&kočit"
"Über&springen"
"&Kihagy"
"&Omiń"
"&Omitir"
"&Пропустити"

DeleteFileSkipAll
"П&ропустить все"
"S&kip all"
"Př&eskočit vše"
"A&lle überspr."
"Mind&et"
"O&miń wszystkie"
"O&mitir todos"
"П&ропустити все"

DeleteFileCancel
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

DeleteLinkDelete
l:
"Удалить ссылку"
"Delete link"
"Smazat link"
"Link löschen"
"Link törlése"
"Usuń link"
"Borrar enlace"
"Видалити посилання"

DeleteLinkUnlink
"Разорвать ссылку"
"Break link"
"Poškozený link"
"Link auflösen"
"Link megszakítása"
"Przerwij link"
"Romper enlace"
"Розірвати посилання"

DeletingTitle
l:
"Удаление"
"Deleting"
"Mazání"
"Lösche"
"Törlés"
"Usuwam"
"Borrando"
"Видалення"

Deleting
l:
"Удаление файла или папки"
"Deleting the file or folder"
"Mazání souboru nebo adresáře"
"Löschen von Datei oder Ordner"
"Fájl vagy mappa törlése"
"Usuwam plik/katalog"
"Borrando el archivo o directorio"
"Видалення файлу або теки"

DeletingWiping
"Уничтожение файла или папки"
"Wiping the file or folder"
"Vymazávání souboru nebo adresáře"
"Sicheres löschen von Datei oder Ordner"
"Fájl vagy mappa kisöprése"
"Wymazuję plik/katalog"
"Limpiando el archivo o directorio"
"Знищення файлу або теки"

DeleteRO
l:
"Файл имеет атрибут \"Только для чтения\""
"The file is read only"
"Soubor je určen pouze pro čtení"
"Folgende Datei ist schreibgeschützt:"
"A fájl csak olvasható:"
"Ten plik jest tylko do odczytu"
"El archivo es de sólo lectura"
"Файл має атрибут \"Тільки для читання\""

AskDeleteRO
"Вы хотите удалить его?"
"Do you wish to delete it?"
"Opravdu si ho přejete smazat?"
"Wollen Sie sie dennoch löschen?"
"Mégis törölni akarja?"
"Czy chcesz go usunąć?"
"Desea borrarlo?"
"Ви хочете видалити його?"

AskWipeRO
"Вы хотите уничтожить его?"
"Do you wish to wipe it?"
"Opravdu si ho přejete vymazat?"
"Wollen Sie sie dennoch sicher löschen?"
"Mégis ki akarja söpörni?"
"Czy chcesz go wymazać?"
"Desea limpiarlo?"
"Ви хочете знищити його?"

DeleteHardLink1
l:
"Файл имеет несколько жёстких ссылок"
"Several hard links link to this file."
"Více pevných linků ukazuje na tento soubor."
"Mehrere Hardlinks zeigen auf diese Datei."
"Több hardlink kapcsolódik a fájlhoz, a fájl"
"Do tego pliku prowadzi wiele linków trwałych."
"Demasiados enlaces rígidos a este archivo."
"Файл має кілька жорстких посилань"

DeleteHardLink2
"Уничтожение файла приведёт к обнулению всех ссылающихся на него файлов."
"Wiping this file will void all files linking to it."
"Vymazání tohoto souboru zneplatní všechny soubory, které na něj linkují."
"Sicheres Löschen dieser Datei entfernt ebenfalls alle Links."
"kisöprése a linkelt fájlokat is megsemmisíti."
"Wymazanie tego pliku wymaże wszystkie pliki dolinkowane."
"Limpiando este archivo invalidará todos los archivos enlazados."
"Знищення файлу призведе до обнулення всіх файлів, що посилаються."

DeleteHardLink3
"Уничтожать файл?"
"Do you wish to wipe this file?"
"Opravdu chcete vymazat tento soubor?"
"Wollen Sie diese Datei sicher löschen?"
"Biztosan kisöpri a fájlt?"
"Czy wymazać plik?"
"Desea limpiar este archivo"
"Знищувати файл?"

CannotDeleteFile
l:
"Ошибка удаления файла"
"Cannot delete the file"
"Nelze smazat soubor"
"Datei konnte nicht gelöscht werden"
"A fájl nem törölhető"
"Nie mogę usunąć pliku"
"No se puede borrar el archivo"
"Помилка видалення файлу"

CannotDeleteFolder
"Ошибка удаления папки"
"Cannot delete the folder"
"Nelze smazat adresář"
"Ordner konnte nicht gelöscht werden"
"A mappa nem törölhető"
"Nie mogę usunąć katalogu"
"No se puede borrar el directorio"
"Помилка видалення теки"

DeleteRetry
"&Повторить"
"&Retry"
"&Znovu"
"Wiede&rholen"
"Ú&jra"
"&Ponów"
"&Reiterar"
"&Повторити"

DeleteSkip
"П&ропустить"
"&Skip"
"Přes&kočit"
"Über&springen"
"&Kihagy"
"Po&miń"
"&Omitir"
"П&ропустити"

DeleteSkipAll
"Пропустить &все"
"S&kip all"
"Přeskočit &vše"
"A&lle überspr."
"Min&d"
"Pomiń &wszystkie"
"Omitir &Todo"
"Пропустити &все"

DeleteCancel
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

CannotGetSecurity
l:
"Ошибка получения прав доступа к файлу"
"Cannot get file access rights for"
"Nemohu získat přístupová práva pro"
"Kann Zugriffsrechte nicht lesen für"
"A fájlhoz nincs hozzáférési joga:"
"Nie mogę pobrać praw dostępu dla"
"No se puede tener permisos de acceso a archivo"
"Помилка отримання доступу до файлу"

CannotSetSecurity
"Ошибка установки прав доступа к файлу"
"Cannot set file access rights for"
"Nemohu nastavit přístupová práva pro"
"Kann Zugriffsrechte nicht setzen für"
"A fájl hozzáférési jogát nem állíthatja:"
"Nie mogę ustawić praw dostępu dla"
"No se puede poner permisos de acceso a archivo"
"Помилка встановлення прав доступу до файлу"

EditTitle
l:
"Редактор"
"Editor"
"Editor"
"Editor"
"Szerkesztő"
"Edytor"
"Editor"
"Редактор"

AskReload
"уже загружен. Как открыть этот файл?"
"already loaded. How to open this file?"
"již otevřen. Jak otevřít tento soubor?"
"bereits geladen. Wie wollen Sie die Datei öffnen?"
"fájl már be van töltve. Hogyan szerkeszti?"
"został już załadowany. Załadować ponownie?"
"ya está cargado. Como abrir este archivo?"
"вже завантажено. Як відкрити цей файл?"

Current
"&Текущий"
"&Current"
"&Stávající"
"A&ktuell"
"A mostanit &folytatja"
"&Bieżący"
"A&ctual"
"&Поточний"

Reload
"Пере&грузить"
"R&eload"
"&Znovu načíst"
"Aktualisie&ren"
"Újra&tölti"
"&Załaduj"
"R&ecargar"
"Пере&вантажити"

NewOpen
"&Новая копия"
"&New instance"
"&Nová instance"
"&Neue Instanz"
"Ú&j példányban"
"&Nowa instancja"
"&Nueva instancia"
"&Нова копія"

EditCannotOpen
"Ошибка открытия файла"
"Cannot open the file"
"Nelze otevřít soubor"
"Kann Datei nicht öffnen"
"A fájl nem nyitható meg"
"Nie mogę otworzyć pliku"
"No se puede abrir el archivo"
"Помилка відкриття файлу"

EditReading
"Чтение файла"
"Reading the file"
"Načítám soubor"
"Lesen der Datei"
"Fájl olvasása"
"Czytam plik"
"Leyendo el archivo"
"Читання файлу"

EditAskSave
"Файл был изменён. Сохранить?"
"File has been modified. Save?"
upd:"Soubor byl modifikován. Save?"
upd:"Datei wurde verändert. Save?"
upd:"A fájl megváltozott. Save?"
upd:"Plik został zmodyfikowany. Save?"
"El archivo ha sido modificado. Desea guardarlo?"
"Файл було змінено. Зберегти?"

EditAskSaveExt
"Файл был изменён внешней программой. Сохранить?"
"The file was changed by an external program. Save?"
upd:"Soubor byl změněný externím programem. Save?"
upd:"Die Datei wurde durch ein externes Programm verändert. Save?"
upd:"A fájlt egy külső program megváltoztatta. Save?"
upd:"Plik został zmieniony przez inny program. Save?"
"El archivo ha sido cambiado por un programa externo. Desea guardarlo?"
"Файл було змінено зовнішньою програмою. Зберегти?"

EditBtnSaveAs
"Сохр&анить как..."
"Save &as..."
"Ulož&it jako..."
"Speichern &als..."
"Mentés más&ként..."
"Zapisz &jako..."
"Guardar como..."
"Збе&регти як..."

EditRO
l:
"имеет атрибут \"Только для чтения\""
"is a read-only file"
"je určen pouze pro čtení"
"ist eine schreibgeschützte Datei"
"csak olvasható fájl"
"jest plikiem tylko do odczytu"
"es un archivo de sólo lectura"
"має атрибут \"Тільки для читання\""

EditExists
"уже существует"
"already exists"
"již existuje"
"ist bereits vorhanden"
"már létezik"
"już istnieje"
"ya existe"
"вже існує"

EditOvr
"Вы хотите перезаписать его?"
"Do you wish to overwrite it?"
"Přejete si ho přepsat?"
"Wollen Sie die Datei überschreiben?"
"Felül akarja írni?"
"Czy chcesz go nadpisać?"
"Desea sobrescribirlo?"
"Ви хочете перезаписати його?"

EditSaving
"Сохранение файла"
"Saving the file"
"Ukládám soubor"
"Speichere die Datei"
"Fájl mentése"
"Zapisuję plik"
"Guardando el archivo"
"Збереження файлу"

EditStatusLine
"Строка"
"Line"
"Řádek"
"Zeile"
"Sor"
"linia"
"Línea"
"Рядок"

EditStatusCol
"Кол"
"Col"
"Sloupec"
"Spal"
"Oszlop"
"kolumna"
"Col"
"Кол"

EditRSH
l:
"предназначен только для чтения"
"is a read-only file"
"je určen pouze pro čtení"
"ist eine schreibgeschützte Datei"
"csak olvasható fájl"
"jest plikiem tylko do odczytu"
"es un archivo de sólo lectura"
"призначений лише для читання"

EditFileGetSizeError
"Не удалось определить размер."
"File size could not be determined."
upd:"File size could not be determined."
upd:"File size could not be determined."
"A fájlméret megállapíthatatlan."
upd:"File size could not be determined."
"Tamaño de archivo no puede ser determinado"
"Не вдалося визначити розмір."

EditFileLong
"имеет размер %ls,"
"has the size of %ls,"
"má velikost %ls,"
"hat eine Größe von %ls,"
"mérete %ls,"
"ma wielkość %ls,"
"tiene el tamaño de %ls,"
"має розмір %ls,"

EditFileLong2
"что превышает заданное ограничение в %ls."
"which exceeds the configured maximum size of %ls."
"která překračuje nastavenou maximální velikost %ls."
"die die konfiguierte Maximalgröße von %ls überschreitet."
"meghaladja %ls beállított maximumát."
"przekraczającą ustalone maksimum %ls."
"cual excede el tamaño máximo configurado de %ls."
"що перевищує задане обмеження у %ls."

EditROOpen
"Вы хотите редактировать его?"
"Do you wish to edit it?"
"Opravdu si ho přejete upravit?"
"Wollen Sie sie dennoch bearbeiten?"
"Mégis szerkeszti?"
"Czy chcesz go edytować?"
"Desea editarlo?"
"Ви хочете редагувати його?"

EditCanNotEditDirectory
l:
"Невозможно редактировать папку"
"It is impossible to edit the folder"
"Nelze editovat adresář"
"Es ist nicht möglich den Ordner zu bearbeiten"
"A mappa nem szerkeszthető"
"Nie można edytować folderu"
"Es imposible editar el directorio"
"Неможливо редагувати теку"

EditSearchTitle
l:
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keresés"
"Szukaj"
"Buscar"
"Пошук"

EditSearchFor
"&Искать"
"&Search for"
"&Hledat"
"&Suchen nach"
"&Keresés:"
"&Znajdź"
"&Buscar por"
"&Шукати"

EditSearchCase
"&Учитывать регистр"
"&Case sensitive"
"&Rozlišovat velikost písmen"
"G&roß-/Kleinschrb."
"&Nagy/kisbetű érz."
"&Uwzględnij wielkość liter"
"Sensible min/ma&y"
"&Враховувати регістр"

EditSearchWholeWords
"Только &целые слова"
"&Whole words"
"&Celá slova"
"&Ganze Wörter"
"Csak e&gész szavak"
"Tylko całe słowa"
"&Palabras completas"
"Тільки &цілі слова"

EditSearchReverse
"Обратн&ый поиск"
"Re&verse search"
"&Zpětné hledání"
"Richtung um&kehren"
"&Visszafelé keres"
"Szukaj w &odwrotnym kierunku"
"Búsqueda in&versa"
"Зворотн&ий пошук"

EditSearchSelFound
"&Выделять найденное"
"Se&lect found"
"Vy&ber nalezené"
"Treffer &markieren"
"&Találat kijelölése"
"W&ybierz znalezione"
"Se&leccionado encontrado"
"&Виділяти знайдене"

EditSearchRegexp
"&Регулярные выражения"
"Re&gular expressions"
upd:"Re&gular expressions"
upd:"Re&gular expressions"
upd:"Re&gular expressions"
upd:"Re&gular expressions"
"Expresiones re&gulares"
"&Регулярні вирази"

EditSearchSearch
"Искать"
"Search"
"Hledat"
"Suchen"
"Kere&sés"
"&Szukaj"
"Buscar"
"Шукати"

EditSearchCancel
"Отменить"
"Cancel"
"Storno"
"Abbruch"
"&Mégsem"
"&Anuluj"
"Cancelar"
"Скасувати"

EditReplaceTitle
l:
"Замена"
"Replace"
"Nahradit"
"Ersetzen"
"Keresés és csere"
"Zamień"
"Reemplazar"
"Заміна"

EditReplaceWith
"Заменить &на"
"R&eplace with"
"Nahradit &s"
"&Ersetzen mit"
"&Erre cseréli:"
"Zamień &na"
"R&eemplazar con"
"Замінити &на"

EditReplaceReplace
"&Замена"
"&Replace"
"&Nahradit"
"E&rsetzen"
"&Csere"
"Za&mień"
"&Reemplazar"
"&Заміна"

EditSearchingFor
l:
"Искать"
"Searching for"
"Vyhledávám"
"Suche nach"
"Keresett szöveg:"
"Szukam"
"Buscando por"
"Шукати"

EditNotFound
"Строка не найдена"
"Could not find the string"
"Nemůžu najít řetězec"
"Konnte Zeichenkette nicht finden"
"A szöveg nem található:"
"Nie mogę odnaleźć ciągu"
"No se puede encontrar la cadena"
"Рядок не знайдено"

EditAskReplace
l:
"Заменить"
"Replace"
"Nahradit"
"Ersetze"
"Ezt cseréli:"
"Zamienić"
"Reemplazar"
"Замінити"

EditAskReplaceWith
"на"
"with"
"s"
"mit"
"erre a szövegre:"
"na"
"con"
"на"

EditReplace
"&Заменить"
"&Replace"
"&Nahradit"
"E&rsetzen"
"&Csere"
"&Zamień"
"&Reemplazar"
"&Замінити"

EditReplaceAll
"&Все"
"&All"
"&Vše"
"&Alle"
"&Mindet"
"&Wszystkie"
"&Todos"
"&Усе"

EditSkip
"&Пропустить"
"&Skip"
"Přes&kočit"
"Über&springen"
"&Kihagy"
"&Omiń"
"&Omitir"
"&Пропустити"

EditCancel
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"Mé&gsem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

EditOpenCreateLabel
"&Открыть/создать файл:"
"&Open/create file:"
"Otevřít/vytvořit soubor:"
"Öffnen/datei erstellen:"
"Fájl megnyitása/&létrehozása:"
"&Otwórz/utwórz plik:"
"&Abrir/crear archivo:"
"&Відкрити/створити файл:"

EditOpenAutoDetect
"&Автоматическое определение"
"&Automatic detection"
upd:"Automatic detection"
upd:"Automatic detection"
"&Automatikus felismerés"
"&Wykryj automatycznie"
"Deteccion &automática"
"&Автоматичне визначення"

EditGoToLine
l:
"Перейти"
"Go to position"
"Jít na pozici"
"Gehe zu Zeile"
"Sorra ugrás"
"Idź do linii"
"Ir a posición"
"Перейти"

BookmarksTitle
l:
"Закладки"
"Bookmarks"
"Adresářové zkratky"
"Ordnerschnellzugriff"
"Mappa gyorsbillentyűk"
"Skróty katalogów"
"Accesos a directorio"
"Закладки"

PluginsTitle
l:
"Плагины"
"Plugins"
upd:"Plugins"
upd:"Plugins"
upd:"Plugins"
upd:"Plugins"
upd:"Plugins"
"Плагіни"

VTStop
l:
"Завершение фоновой оболочки."
"Closing back shell."
upd:"Closing back shell."
upd:"Closing back shell."
upd:"Closing back shell."
upd:"Closing back shell."
upd:"Closing back shell."
"Завершення фонової оболонки."

VTStopTip
l:
"Подсказка: чтобы закрыть far2l - введите 'exit far'."
"TIP: To close far2l - type 'exit far'."
upd:"TIP: To close far2l - type 'exit far'."
upd:"TIP: To close far2l - type 'exit far'."
upd:"TIP: To close far2l - type 'exit far'."
upd:"TIP: To close far2l - type 'exit far'."
upd:"TIP: To close far2l - type 'exit far'."
"Підказка: щоб закрити far2l - введіть 'exit far'."

VTStartTipNoCmdTitle
l:
"При наборе команды:                                                       "
"While typing command:                                                     "
upd:"While typing command:                                                     "
upd:"While typing command:                                                     "
upd:"While typing command:                                                     "
upd:"While typing command:                                                     "
upd:"While typing command:                                                     "
"При наборі команди:                                                       "

VTStartTipNoCmdCtrlO
l:
" Ctrl+O - переключения панель/терминал.                                   "
" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
" Ctrl+O - перемикання панель/термінал.                                    "

VTStartTipNoCmdCtrlArrow
l:
" Ctrl+Вверх/+Вниз/+Влево/+Вправо - изменение размера панелей.             "
" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
" Ctrl+Вгору/+Вниз/+Вліво/+Вправо - зміна розміру панелей.                 "

VTStartTipNoCmdShiftTAB
l:
" Двойной Shift+TAB - автодополнение от bash.                              "
" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
"Подвійний Shift+TAB - автодоповнення від bash.                            "

VTStartTipNoCmdFn
l:
" F3, F4, F8 - просмотр/редактор/очистка лога терминала при выкл. панелях. "
" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
" F3, F4, F8 - перегляд/редактор/очищення лога терміналу при вимкн. панелях."

VTStartTipNoCmdMouse
l:
" Ctrl+Shift+MouseScrollUp - автозавершающийся просмотр лога терминала.    "
" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
" Ctrl+Shift+MouseScrollUp - автозавершення перегляду лога терміналу.      "

VTStartTipPendCmdTitle
l:
"В процессе исполнения команды:                                            "
"While executing command:                                                  "
upd:"While executing command:                                                  "
upd:"While executing command:                                                  "
upd:"While executing command:                                                  "
upd:"While executing command:                                                  "
upd:"While executing command:                                                  "
"У процесі виконання команди:                                              "

VTStartTipPendCmdFn
l:
" Ctrl+Shift+F3/+F4 - пауза и открытие просмотра/редактора лога терминала. "
" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
" Ctrl+Shift+F3/+F4 - пауза та відкриття перегляду/редактора лога терміналу."

VTStartTipPendCmdCtrlAltC
l:
" Ctrl+Alt+C - завершить все процессы в этой оболочке.                     "
" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
" Ctrl+Alt+C - завершити всі процеси в цій оболонці.                       "

VTStartTipPendCmdCtrlAltZ
l:
" Ctrl+Alt+Z - отправить процесс far2l в фон, освободив терминал.          "
" Ctrl+Alt+Z - detach far2l application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2l application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2l application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2l application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2l application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2l application to background releasing terminal.  "
" Ctrl+Alt+Z - надіслати процес far2l у фон, звільнивши термінал.          "

VTStartTipPendCmdMouse
l:
" MouseScrollUp - автозавершающийся просмотр лога терминала.               "
" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
" MouseScrollUp - перегляд лога терміналу, що завершується автоматично.    "

BookmarkBottom
"Редактирование: Del,Ins,F4,Shift+Вверх,Shift+Вниз"
"Edit: Del,Ins,F4,Shift+Up,Shift+Down"
"Edit: Del,Ins,F4,Shift+Up,Shift+Down"
"Bearb.: Entf,Einf,F4,Shift+Up,Shift+Down"
"Szerk.: Del,Ins,F4,Shift+Up,Shift+Down"
"Edycja: Del,Ins,F4,Shift+Up,Shift+Down"
"Editar: Del,Ins,F4,Shift+Up,Shift+Down"
"Редагування: Del,Ins,F4,Shift+Вгору,Shift+Вниз"

ShortcutNone
"<отсутствует>"
"<none>"
"<není>"
"<keiner>"
"<nincs>"
"<brak>"
"<nada>"
"<відсутня>"

ShortcutPlugin
"<плагин>"
"<plugin>"
"<plugin>"
"<Plugin>"
"<plugin>"
"<plugin>"
"<plugin>"
"<плагін>"

FSShortcut
"Введите новую закладку:"
"Enter bookmark path:"
"Zadejte novou zkratku:"
"Neue Verknüpfung:"
"A gyorsbillentyűhöz rendelt mappa:"
"Wprowadź nowy skrót:"
"Ingrése nuevo acceso:"
"Введіть нову закладку:"

NeedNearPath
"Перейти в ближайшую доступную папку?"
"Jump to the nearest existing folder?"
"Skočit na nejbližší existující adresář?"
"Zum nahesten existierenden Ordner springen?"
"Ugrás a legközelebbi létező mappára?"
"Przejść do najbliższego istniejącego folderu?"
"Saltar al próximo directorio existente"
"Перейти до доступної теки?"

SaveThisShortcut
"Запомнить эту закладку?"
"Save this bookmark?"
"Uložit tyto zkratky?"
"Verknüpfung speichern?"
"Mentsem a gyorsbillentyűket?"
"Zapisać skróty?"
"Guardar estos accesos"
"Запам'ятати цю закладку?"

EditF1
l:
l://functional keys - 6 characters max, 12 keys, "OEM" is F8 dupe!
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

EditF2
"Сохран"
"Save"
"Uložit"
"Speich"
"Mentés"
"Zapisz"
"Guarda"
"Збргти"

EditF3
""
""
""
""
""
""
""
""

EditF4
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

EditF5
""
""
""
""
""
""
""
""

EditF6
"Просм"
"View"
"Zobraz"
"Betr."
"Megnéz"
"Zobacz"
"Ver "
"Прогл"

EditF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Szukaj"
"Buscar"
"Пошук"

EditF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Latin 2"
"ANSI"
"ANSI"

EditF9
""
""
""
""
""
""
""
""

EditF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

EditF11
"Модули"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Модулі"

EditF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant. "
"Екрани"

EditF8DOS
le:// don't count this - it's a F8 another text
"OEM"
"OEM"
"OEM"
"OEM"
"OEM"
"CP-1250"
"OEM"
"OEM"

ViewF5Processed
le:// don't count this - it's a F5 another text
"Обработ"
"Proc-d"
"Proc-d"
"Proc-d"
"Proc-d"
"Proc-d"
"Proc-d"
"Обробно"

ViewF5Raw
le:// don't count this - it's a F5 another text
"Сырой"
"Raw"
"Raw"
"Raw"
"Raw"
"Raw"
"Raw"
"Сирий"

EditShiftF1
l:
l://Editor: Shift
""
""
""
""
""
""
""
""

EditShiftF2
"Сохр.в"
"SaveAs"
"UlJako"
"SpeiUn"
"Ment.."
"Zapisz"
"Grbcom"
"Збер.в"

EditShiftF3
""
""
""
""
""
""
""
""

EditShiftF4
"Редак."
"Edit.."
"Edit.."
"Bear.."
"Szrk.."
"Edytuj"
"Editar."
"Редаг."

EditShiftF5
""
""
""
""
""
""
""
""

EditShiftF6
""
""
""
""
""
""
""
""

EditShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"TovKer"
"Następ"
"Próximo"
"Далі"

EditShiftF8
"КодСтр"
"CodePg"
upd:"ZnSady"
upd:"Tabell"
"Kódlap"
"Tabela"
"CodePag"
"КодСтор"

EditShiftF9
""
""
""
""
""
""
""
""

EditShiftF10
"СхрВых"
"SaveQ"
"UlKone"
"SaveQ"
"MentKi"
"ZapKon"
"GrdySal"
"ЗбрВих"

EditShiftF11
""
""
""
""
""
""
""
""

EditShiftF12
""
""
""
""
""
""
""
""

EditAltF1
l:
l://Editor: Alt
""
""
""
""
""
""
""
""

EditAltF2
""
""
""
""
""
""
""
""

EditAltF3
""
""
""
""
""
""
""
""

EditAltF4
""
""
""
""
""
""
""
""

EditAltF5
"Печать"
"Print"
"Tisk"
"Druck"
"Nyomt"
"Drukuj"
"Imprim"
"Друк"

EditAltF6
""
""
""
""
""
""
""
""

EditAltF7
"Назад"
"Prev"
"Předch"
"Letzt"
"VisKer"
"Poprz"
"Previo"
"Назад"

EditAltF8
"Строка"
"Goto"
"Jít na"
"GeheZu"
"Ugrás"
"IdźDo"
"Ir a.."
"Рядок"

EditAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

EditAltF10
"Закр.Far"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"Закр.Far"

EditAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"Historia"
"HisVer"
"ІстПр"

EditAltF12
""
""
""
""
""
""
""
""

EditCtrlF1
l:
l://Editor: Ctrl
""
""
""
""
""
""
""
""

EditCtrlF2
""
""
""
""
""
""
""
""

EditCtrlF3
""
""
""
""
""
""
""
""

EditCtrlF4
""
""
""
""
""
""
""
""

EditCtrlF5
""
""
""
""
""
""
""
""

EditCtrlF6
""
""
""
""
""
""
""
""

EditCtrlF7
"Замена"
"Replac"
"Nahraď"
"Ersetz"
"Csere"
"Zamień"
"Remplz"
"Заміна"

EditCtrlF8
""
""
""
""
""
""
""
""

EditCtrlF9
""
""
""
""
""
""
""
""

EditCtrlF10
"Позиц"
"GoFile"
"JdiSou"
"GehDat"
"FájlPz"
"GoFile"
"IrArch"
"Позиц"

EditCtrlF11
""
""
""
""
""
""
""
""

EditCtrlF12
""
""
""
""
""
""
""
""

EditAltShiftF1
l:
l://Editor: AltShift
""
""
""
""
""
""
""
""

EditAltShiftF2
""
""
""
""
""
""
""
""

EditAltShiftF3
""
""
""
""
""
""
""
""

EditAltShiftF4
""
""
""
""
""
""
""
""

EditAltShiftF5
""
""
""
""
""
""
""
""

EditAltShiftF6
""
""
""
""
""
""
""
""

EditAltShiftF7
""
""
""
""
""
""
""
""

EditAltShiftF8
""
""
""
""
""
""
""
""

EditAltShiftF9
"Конфиг"
"Config"
"Nastav"
"Konfig"
"Beáll."
"Konfig"
"Config"
"Конфіг"

EditAltShiftF10
""
""
""
""
""
""
""
""

EditAltShiftF11
""
""
""
""
""
""
""
""

EditAltShiftF12
""
""
""
""
""
""
""
""

EditCtrlShiftF1
l:
l://Editor: CtrlShift
""
""
""
""
""
""
""
""

EditCtrlShiftF2
""
""
""
""
""
""
""
""

EditCtrlShiftF3
""
""
""
""
""
""
""
""

EditCtrlShiftF4
""
""
""
""
""
""
""
""

EditCtrlShiftF5
""
""
""
""
""
""
""
""

EditCtrlShiftF6
""
""
""
""
""
""
""
""

EditCtrlShiftF7
""
""
""
""
""
""
""
""

EditCtrlShiftF8
""
""
""
""
""
""
""
""

EditCtrlShiftF9
""
""
""
""
""
""
""
""

EditCtrlShiftF10
""
""
""
""
""
""
""
""

EditCtrlShiftF11
""
""
""
""
""
""
""
""

EditCtrlShiftF12
""
""
""
""
""
""
""
""

EditCtrlAltF1
l:
l:// Editor: CtrlAlt
""
""
""
""
""
""
""
""

EditCtrlAltF2
""
""
""
""
""
""
""
""

EditCtrlAltF3
""
""
""
""
""
""
""
""

EditCtrlAltF4
""
""
""
""
""
""
""
""

EditCtrlAltF5
""
""
""
""
""
""
""
""

EditCtrlAltF6
""
""
""
""
""
""
""
""

EditCtrlAltF7
""
""
""
""
""
""
""
""

EditCtrlAltF8
""
""
""
""
""
""
""
""

EditCtrlAltF9
""
""
""
""
""
""
""
""

EditCtrlAltF10
""
""
""
""
""
""
""
""

EditCtrlAltF11
""
""
""
""
""
""
""
""

EditCtrlAltF12
""
""
""
""
""
""
""
""

EditCtrlAltShiftF1
l:
l:// Editor: CtrlAltShift
""
""
""
""
""
""
""
""

EditCtrlAltShiftF2
""
""
""
""
""
""
""
""

EditCtrlAltShiftF3
""
""
""
""
""
""
""
""

EditCtrlAltShiftF4
""
""
""
""
""
""
""
""

EditCtrlAltShiftF5
""
""
""
""
""
""
""
""

EditCtrlAltShiftF6
""
""
""
""
""
""
""
""

EditCtrlAltShiftF7
""
""
""
""
""
""
""
""

EditCtrlAltShiftF8
""
""
""
""
""
""
""
""

EditCtrlAltShiftF9
""
""
""
""
""
""
""
""

EditCtrlAltShiftF10
""
""
""
""
""
""
""
""

EditCtrlAltShiftF11
""
""
""
""
""
""
""
""

EditCtrlAltShiftF12
le://End of functional keys (Editor)
""
""
""
""
""
""
""
""

SingleEditF1
l:
l://Single Editor: functional keys - 6 characters max, 12 keys, "OEM" is F8 dupe!
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допмга"

SingleEditF2
"Сохран"
"Save"
"Uložit"
"Speich"
"Mentés"
"Zapisz"
"Guarda"
"Збереж"

SingleEditF3
""
""
""
""
""
""
""
""

SingleEditF4
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

SingleEditF5
""
""
""
""
""
""
""
""

SingleEditF6
"Просм"
"View"
"Zobraz"
"Betr."
"Megnéz"
"Zobacz"
"Ver"
"Прогл"

SingleEditF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Szukaj"
"Buscar"
"Пошук"

SingleEditF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Latin 2"
"ANSI"
"ANSI"

SingleEditF9
""
""
""
""
""
""
""
""

SingleEditF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

SingleEditF11
"Модули"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Модулі"

SingleEditF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant. "
"Екрани"

SingleEditF8DOS
le:// don't count this - it's a F8 another text
"OEM"
"OEM"
"OEM"
"OEM"
"OEM"
"CP 1250"
"OEM"
"OEM"

SingleEditShiftF1
l:
l://Single Editor: Shift
""
""
""
""
""
""
""
""

SingleEditShiftF2
"Сохр.в"
"SaveAs"
"UlJako"
"SpeiUn"
"Ment.."
"Zapisz"
"Guarcm"
"Збер.в"

SingleEditShiftF3
""
""
""
""
""
""
""
""

SingleEditShiftF4
""
""
""
""
""
""
""
""

SingleEditShiftF5
""
""
""
""
""
""
""
""

SingleEditShiftF6
""
""
""
""
""
""
""
""

SingleEditShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"TovKer"
"Następ"
"Próxim"
"Далі"

SingleEditShiftF8
"КодСтр"
"CodePg"
upd:"ZnSady"
upd:"Tabell"
"Kódlap"
"Tabela"
"Tabla"
"КдСтор"

SingleEditShiftF9
""
""
""
""
""
""
""
""

SingleEditShiftF10
"СхрВых"
"SaveQ"
"UlKone"
"SaveQ"
"MentKi"
"ZapKon"
"GuaryS"
"СхрВих"

SingleEditShiftF11
""
""
""
""
""
""
""
""

SingleEditShiftF12
""
""
""
""
""
""
""
""

SingleEditAltF1
l:
l://Single Editor: Alt
""
""
""
""
""
""
""
""

SingleEditAltF2
""
""
""
""
""
""
""
""

SingleEditAltF3
""
""
""
""
""
""
""
""

SingleEditAltF4
""
""
""
""
""
""
""
""

SingleEditAltF5
"Печать"
"Print"
"Tisk"
"Druck"
"Nyomt"
"Drukuj"
"Imprime"
"Друк"

SingleEditAltF6
""
""
""
""
""
""
""
""

SingleEditAltF7
""
""
""
""
""
""
""
""

SingleEditAltF8
"Строка"
"Goto"
"Jít na"
"GeheZu"
"Ugrás"
"IdźDo"
"Ir a.."
"Рядок"

SingleEditAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Ekran"
"Video"
"Відео"

SingleEditAltF10
""
""
""
""
""
""
""
""

SingleEditAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"ZobHs"
"VerHis"
"ІстПр"

SingleEditAltF12
""
""
""
""
""
""
""
""

SingleEditCtrlF1
l:
l://Single Editor: Ctrl
""
""
""
""
""
""
""
""

SingleEditCtrlF2
""
""
""
""
""
""
""
""

SingleEditCtrlF3
""
""
""
""
""
""
""
""

SingleEditCtrlF4
""
""
""
""
""
""
""
""

SingleEditCtrlF5
""
""
""
""
""
""
""
""

SingleEditCtrlF6
""
""
""
""
""
""
""
""

SingleEditCtrlF7
"Замена"
"Replac"
"Nahraď"
"Ersetz"
"Csere"
"Zastąp"
"Remplz"
"Заміна"

SingleEditCtrlF8
""
""
""
""
""
""
""
""

SingleEditCtrlF9
""
""
""
""
""
""
""
""

SingleEditCtrlF10
""
""
""
""
""
""
""
""

SingleEditCtrlF11
""
""
""
""
""
""
""
""

SingleEditCtrlF12
""
""
""
""
""
""
""
""

SingleEditAltShiftF1
l:
l://Single Editor: AltShift
""
""
""
""
""
""
""
""

SingleEditAltShiftF2
""
""
""
""
""
""
""
""

SingleEditAltShiftF3
""
""
""
""
""
""
""
""

SingleEditAltShiftF4
""
""
""
""
""
""
""
""

SingleEditAltShiftF5
""
""
""
""
""
""
""
""

SingleEditAltShiftF6
""
""
""
""
""
""
""
""

SingleEditAltShiftF7
""
""
""
""
""
""
""
""

SingleEditAltShiftF8
""
""
""
""
""
""
""
""

SingleEditAltShiftF9
"Конфиг"
"Config"
"Nastav"
"Konfig"
"Beáll."
"Konfig"
"Config"
"Конфіг"

SingleEditAltShiftF10
""
""
""
""
""
""
""
""

SingleEditAltShiftF11
""
""
""
""
""
""
""
""

SingleEditAltShiftF12
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF1
l:
l://Single Editor: CtrlShift
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF2
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF3
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF4
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF5
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF6
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF7
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF8
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF9
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF10
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF11
""
""
""
""
""
""
""
""

SingleEditCtrlShiftF12
""
""
""
""
""
""
""
""

SingleEditCtrlAltF1
l:
l://Single Editor: CtrlAlt
""
""
""
""
""
""
""
""

SingleEditCtrlAltF2
""
""
""
""
""
""
""
""

SingleEditCtrlAltF3
""
""
""
""
""
""
""
""

SingleEditCtrlAltF4
""
""
""
""
""
""
""
""

SingleEditCtrlAltF5
""
""
""
""
""
""
""
""

SingleEditCtrlAltF6
""
""
""
""
""
""
""
""

SingleEditCtrlAltF7
""
""
""
""
""
""
""
""

SingleEditCtrlAltF8
""
""
""
""
""
""
""
""

SingleEditCtrlAltF9
""
""
""
""
""
""
""
""

SingleEditCtrlAltF10
""
""
""
""
""
""
""
""

SingleEditCtrlAltF11
""
""
""
""
""
""
""
""

SingleEditCtrlAltF12
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF1
l:
l://Single Editor: CtrlAltShift
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF2
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF3
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF4
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF5
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF6
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF7
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF8
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF9
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF10
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF11
""
""
""
""
""
""
""
""

SingleEditCtrlAltShiftF12
le://End of functional keys (Single Editor)
""
""
""
""
""
""
""
""

EditSaveAs
l:
"Сохранить &файл как"
"Save file &as"
"Uložit soubor jako"
"Speichern &als"
"Fá&jl mentése, mint:"
"Zapisz plik &jako"
"Guardar archivo &como"
"Зберегти &файл як"

EditCodePage
"&Кодовая страница:"
"&Code page:"
"Kódová stránka:"
"Codepage:"
"Kódlap:"
"&Strona kodowa:"
"&Código caracteres:"
"&Кодова сторінка:"

EditAddSignature
"Добавить &сигнатуру (BOM)"
"Add &signature (BOM)"
"Přidat signaturu (BOM)"
"Sinatur hinzu (BOM)"
"Uni&code bájtsorrend jelzővel (BOM)"
"Dodaj &znacznik BOM"
"Añadir &signatura (BOM)"
"Додати &сигнатуру (BOM)"

EditSaveAsFormatTitle
"Изменить перевод строки:"
"Change line breaks to:"
"Změnit zakončení řádků na:"
"Zeilenumbrüche setzen:"
"Sortörés konverzió:"
"Zamień znaki końca linii na:"
"Cambiar fin de líneas a:"
"Змінити розриви рядків на:"

EditSaveOriginal
"&исходный формат"
"Do n&ot change"
"&Beze změny"
"Nicht verä&ndern"
"Nincs &konverzió"
"&Nie zmieniaj"
"N&o cambiar"
"&Вихідний формат"

EditSaveDOS
"в форма&те DOS/Windows (CR LF)"
"&Dos/Windows format (CR LF)"
"&Dos/Windows formát (CR LF)"
"&Dos/Windows Format (CR LF)"
"&DOS/Windows formátum (CR LF)"
"Format &Dos/Windows (CR LF)"
"Formato &DOS/Windows (CR LF)"
"у форма&ті DOS/Windows (CR LF)"

EditSaveUnix
"в формат&е UNIX (LF)"
"&Unix format (LF)"
"&Unix formát (LF)"
"&Unix Format (LF)"
"&UNIX formátum (LF)"
"Format &Unix (LF)"
"Formato &Unix (LF)"
"у формат&і UNIX (LF)"

EditSaveMac
"в фор&мате MAC (CR)"
"&Mac format (CR)"
"&Mac formát (CR)"
"&Mac Format (CR)"
"&Mac formátum (CR)"
"Format &Mac (CR)"
"Formato &Mac (CR)"
"у фор&маті MAC (CR)"

EditCannotSave
"Ошибка сохранения файла"
"Cannot save the file"
"Nelze uložit soubor"
"Kann die Datei nicht speichern"
"A fájl nem menthető"
"Nie mogę zapisać pliku"
"No se puede guardar archivo"
"Помилка збереження файлу"

EditSavedChangedNonFile
"Файл изменён, но файл или папка, в которой он находился,"
"The file is changed but the file or the folder containing"
"Soubor je změněn, ale soubor, nebo adresář obsahující"
"Inhalt dieser Datei wurde verändert aber die Datei oder der Ordner, welche"
"A fájl megváltozott, de a fájlt vagy a mappáját"
"Plik został zmieniony, ale plik lub folder zawierający"
"El archivo es cambiado pero el archivo o el directorio que contiene"
"Файл змінено, але файл або тека, в якій він знаходився,"

EditSavedChangedNonFile1
"Файл или папка, в которой он находился,"
"The file or the folder containing"
"Soubor nebo adresář obsahující"
"Die Datei oder der Ordner, welche"
"A fájlt vagy a mappáját"
"Plik lub folder zawierający"
"El archivo o el directorio conteniendo"
"Файл або тека, де він знаходився,"

EditSavedChangedNonFile2
"был перемещён или удалён. Сохранить?"
"this file was moved or deleted. Save?"
upd:"tento soubor byl přesunut, nebo smazán. Save?"
upd:"diesen Inhalt enthält wurde verschoben oder gelöscht. Save?"
upd:"időközben áthelyezte/átnevezte vagy törölte. Save?"
upd:"ten plik został przeniesiony lub usunięty. Save?"
"este archivo ha sido movido o borrado. Desea guardarlo?"
"Було переміщено або видалено. Зберегти?"

EditNewPath1
"Путь к редактируемому файлу не существует,"
"The path to the edited file does not exist,"
"Cesta k editovanému souboru neexistuje,"
"Der Pfad zur bearbeiteten Datei existiert nicht,"
"A szerkesztendő fájl célmappája még"
"Ścieżka do edytowanego pliku nie istnieje,"
"La ruta del archivo editado no existe,"
"Шлях до редагованого файлу не існує,"

EditNewPath2
"но будет создан при сохранении файла."
"but will be created when the file is saved."
"ale bude vytvořena při uložení souboru."
"aber wird erstellt sobald die Datei gespeichert wird."
"nem létezik, de mentéskor létrejön."
"ale zostanie utworzona po zapisaniu pliku."
"pero será creada cuando el archivo sea guardado."
"але буде створено при збереженні файлу."

EditNewPath3
"Продолжать?"
"Continue?"
"Pokračovat?"
"Fortsetzen?"
"Folytatja?"
"Kontynuować?"
"Continuar?"
"Продовжувати?"

EditNewPlugin1
"Имя редактируемого файла не может быть пустым"
"The name of the file to edit cannot be empty"
"Název souboru k editaci nesmí být prázdné"
"Der Name der zu editierenden Datei kann nicht leer sein"
"A szerkesztendő fájlnak nevet kell adni"
"Nazwa pliku do edycji nie może być pusta"
"El nombre del archivo a editar no puede estar vacío"
"Ім'я файлу, що редагується, не може бути порожнім"

EditorLoadCPWarn1
"Файл содержит символы, которые невозможно"
"File contains characters, which cannot be"
upd:"File contains characters, which cannot be"
upd:"File contains characters, which cannot be"
upd:"File contains characters, which cannot be"
upd:"File contains characters, which cannot be"
"El archivo contiene caracteres que no pueden ser"
"Файл містить символи, які неможливо"

EditorLoadCPWarn2
"корректно прочитать, используя выбранную кодовую страницу."
"correctly read using selected codepage."
upd:"correctly read using selected codepage."
upd:"correctly read using selected codepage."
upd:"correctly read using selected codepage."
upd:"correctly read using selected codepage."
"correctamente leídos con la tabla (codepage) seleccionada."
"коректно прочитати, використовуючи вибрану кодову сторінку."

EditorSaveCPWarn1
"Редактор содержит символы, которые невозможно"
"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
"El editor contiene caracteres que no pueden ser"
"Редактор містить символи, які неможливо"

EditorSaveCPWarn2
"корректно сохранить, используя выбранную кодовую страницу."
"correctly saved using selected codepage."
upd:"correctly saved using selected codepage."
upd:"correctly saved using selected codepage."
upd:"correctly saved using selected codepage."
upd:"correctly saved using selected codepage."
"correctamente guardados con la tabla (codepage) seleccionada."
"коректно зберегти, використовуючи вибрану кодову сторінку."

EditorSwitchCPWarn1
"Редактор содержит символы, которые невозможно"
"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
"El editor contiene caracteres que no pueden ser"
"Редактор містить символи, які неможливо"

EditorSwitchCPWarn2
"корректно преобразовать, используя выбранную кодовую страницу."
"correctly translated using selected codepage."
upd:"correctly translated using selected codepage."
upd:"correctly translated using selected codepage."
upd:"correctly translated using selected codepage."
upd:"correctly translated using selected codepage."
"correctamente traducidos con la tabla (codepage) seleccionada."
"коректно перетворити, використовуючи вибрану кодову сторінку."

EditDataLostWarn
"Во время редактирования файла некоторые данные были утеряны."
"During file editing some data was lost."
upd:"During file editing some data was lost."
upd:"During file editing some data was lost."
upd:"During file editing some data was lost."
upd:"During file editing some data was lost."
"Durante la edición del archivo algunos datos se perdieron."
"Під час редагування файлу деякі дані були втрачені."

EditorSaveNotRecommended
"Сохранять файл не рекомендуется."
"It is not recommended to save this file."
"Není doporučeno uložit tento soubor."
"Es wird empfohlen, die Datei nicht zu speichern."
"A fájl mentése nem ajánlott."
"Odradzamy zapis pliku."
"No se recomienda guardar este archivo."
"Не рекомендується зберігати файл."

EditorSaveCPWarnShow
"Показать"
"Show"
upd:"Show"
upd:"Show"
upd:"Show"
upd:"Show"
"Mostrar"
"Показати"

ColumnName
l:
"Имя"
"Name"
"Název"
"Name"
"Név"
"Nazwa"
"Nombre"
"Ім'я"

ColumnSize
"Размер"
"Size"
"Velikost"
"Größe"
"Méret"
"Rozmiar"
"Tamaño"
"Розмір"

ColumnPhysical
"ФизРзм"
"PhysSz"
upd:"PhysSz"
upd:"PhysSz"
upd:"PhysSz"
upd:"PhysSz"
upd:"PhysSz"
"ФізРзм"

ColumnDate
"Дата"
"Date"
"Datum"
"Datum"
"Dátum"
"Data"
"Fecha"
"Дата"

ColumnTime
"Время"
"Time"
"Čas"
"Zeit"
"Idő"
"Czas"
"Hora"
"Час"

ColumnWrited
"Запись"
"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
"Запис"

ColumnCreated
"Создание"
"Created"
"Vytvořen"
"Erstellt"
"Létrejött"
"Utworzenie"
"Creado "
"Створення"

ColumnAccessed
"Доступ"
"Accessed"
"Přístup"
"Zugriff"
"Hozzáférés"
"Użycie"
"Acceso  "
"Доступ"

ColumnChanged
"Изменение"
upd:"Change"
upd:"Change"
upd:"Change"
upd:"Change"
upd:"Change"
upd:"Change"
"Змінення"

ColumnAttr
"Атриб"
"Attr"
"Attr"
"Attr"
"Attrib"
"Atrybuty"
"Atrib"
"Атриб"

ColumnDescription
"Описание"
"Description"
"Popis"
"Beschreibung"
"Megjegyzés"
"Opis"
"Descripción"
"Опис"

ColumnOwner
"Владелец"
"Owner"
"Vlastník"
"Besitzer"
"Tulajdonos"
"Właściciel"
"Dueño"
"Власник"

ColumnGroup
"Группа"
"Group"
upd:"Group"
upd:"Group"
upd:"Group"
upd:"Group"
upd:"Group"
"Група"

ColumnMumLinks
"КлС"
"NmL"
"PočLn"
"AnL"
"Lnk"
"NmL"
"NmL"
"КлС"

ListUp
l:
"Вверх"
"  Up  "
"Nahoru"
" Hoch "
"  Fel  "
"W górę"
"UP-DIR"
"Вгору"

ListFolder
"Папка"
"Folder"
"Adresář"
"Ordner"
" Mappa "
"Folder"
" DIR  "
"Тека"

ListSymLink
"Ссылка"
"Symlink"
"Link"
"Symlink"
"SzimLnk"
"LinkSym"
" Enlac"
"Посилання"

ListJunction
"Связь"
"Junction"
"Křížení"
"Knoten"
"Csomópt"
"Dowiązania"
" Junc "
"Зв'язок"

ListBytes
"Б"
"B"
"B"
"B"
"B"
"B"
"B"
"Б"

ListKb
"К"
"K"
"K"
"K"
"k"
"K"
"K"
"К"

ListMb
"М"
"M"
"M"
"M"
"M"
"M"
"M"
"М"

ListGb
"Г"
"G"
"G"
"G"
"G"
"G"
"G"
"Г"

ListTb
"Т"
"T"
"T"
"T"
"T"
"T"
"T"
"Т"

ListPb
"П"
"P"
"P"
"P"
"P"
"P"
"P"
"П"

ListEb
"Э"
"E"
"E"
"E"
"E"
"E"
"E"
"Э"

ListFileSize
" %ls байт в 1 файле "
" %ls bytes in 1 file "
" %ls bytů v 1 souboru "
" %ls Bytes in 1 Datei "
" %ls bájt 1 fájlban "
" %ls bajtów w 1 pliku "
" %ls bytes en 1 archivo "
" %ls байт у 1 файлі "

ListFilesSize1
" %ls байт в %d файле "
" %ls bytes in %d files "
" %ls bytů v %d souborech "
" %ls Bytes in %d Dateien "
" %ls bájt %d fájlban "
" %ls bajtów w %d plikach "
" %ls bytes en %d archivos "
" %ls байт у %d файлі "

ListFilesSize2
" %ls байт в %d файлах "
" %ls bytes in %d files "
" %ls bytů v %d souborech "
" %ls Bytes in %d Dateien "
" %ls bájt %d fájlban "
" %ls bajtów w %d plikach "
" %ls bytes en %d archivos "
" %ls байт у %d файлах "

ListFreeSize
" %ls байт свободно "
" %ls free bytes "
" %ls volných bytů "
" %ls freie Bytes "
" %ls szabad bájt "
" %ls wolnych bajtów "
" %ls bytes libres "
" %ls байт вільно "

DirInfoViewTitle
l:
"Просмотр"
"View"
"Zobraz"
"Betrachten"
"Vizsgálat"
"Podgląd"
"Ver "
"Перегляд"

FileToEdit
"Редактировать файл:"
"File to edit:"
"Soubor k editaci:"
"Datei bearbeiten:"
"Szerkesztendő fájl:"
"Plik do edycji:"
"archivo a editar:"
"Редагувати файл:"

UnselectTitle
l:
"Снять"
"Deselect"
"Odznačit"
"Abwählen"
"Kijelölést levesz"
"Odznacz"
"Deseleccionar"
"Зняти"

SelectTitle
"Пометить"
"Select"
"Označit"
"Auswählen"
"Kijelölés"
"Zaznacz"
"Seleccionar"
"Позначити"

SelectFilter
"&Фильтр"
"&Filter"
"&Filtr"
"&Filter"
"&Szűrő"
"&Filtruj"
"&Filtro"
"&Фільтр"

CompareTitle
l:
"Сравнение"
"Compare"
"Porovnat"
"Vergleichen"
"Összehasonlítás"
"Porównaj"
"Comparar"
"Порівняння"

CompareFilePanelsRequired1
"Для сравнения папок требуются"
"Two file panels are required to perform"
"Pro provedení příkazu Porovnat adresáře"
"Zwei Dateipanels werden benötigt um"
"Mappák összehasonlításához"
"Aby porównać katalogi konieczne są"
"Dos paneles de archivos son necesarios para poder"
"Для порівняння тек потрібні"

CompareFilePanelsRequired2
"две файловые панели"
"the Compare folders command"
"jsou nutné dva souborové panely"
"den Vergleich auszuführen."
"két fájlpanel szükséges"
"dwa zwykłe panele plików"
"utilizar el comando comparar directorios"
"дві файлові панелі"

CompareSameFolders1
"Содержимое папок,"
"The folders contents seems"
"Obsahy adresářů jsou"
"Der Inhalt der beiden Ordner scheint"
"A mappák tartalma"
"Zawartość katalogów"
"El contenido de los directorios parecen"
"Вміст тек,"

CompareSameFolders2
"скорее всего, одинаково"
"to be identical"
"identické"
"identisch zu sein."
"azonosnak tűnik"
"wydaje się być identyczna"
"ser idénticos"
"швидше за все, однаково"

SelectAssocTitle
l:
"Выберите ассоциацию"
"Select association"
"Vyber závislosti"
"Dateiverknüpfung auswählen"
"Válasszon társítást"
"Wybierz przypisanie"
"Seleccionar asociaciones"
"Виберіть асоціацію"

AssocTitle
l:
"Ассоциации для файлов"
"File associations"
"Závislosti souborů"
"Dateiverknüpfungen"
"Fájltársítások"
"Przypisania plików"
"Asociación de archivos"
"Асоціації для файлів"

AssocBottom
"Редактирование: Del,Ins,F4"
"Edit: Del,Ins,F4"
"Edit: Del,Ins,F4"
"Bearb.: Entf,Einf,F4"
"Szerk.: Del,Ins,F4"
"Edycja: Del,Ins,F4"
"Editar: Del,Ins,F4"
"Редагування: Del,Ins,F4"

AskDelAssoc
"Вы хотите удалить ассоциацию для"
"Do you wish to delete association for"
"Přejete si smazat závislost pro"
"Wollen Sie die Verknüpfung löschen für"
"Törölni szeretné a társítást?"
"Czy chcesz usunąć przypisanie dla"
"Desea borrar la asociación para"
"Ви хочете видалити асоціацію для"

FileAssocTitle
l:
"Редактирование ассоциаций файлов"
"Edit file associations"
"Upravit závislosti souborů"
"Dateiverknüpfungen bearbeiten"
"Fájltársítások szerkesztése"
"Edytuj przypisania pliku"
"Editar asociación de archivos"
"Редагування асоціацій файлів"

FileAssocMasks
"Одна или несколько &масок файлов:"
"A file &mask or several file masks:"
"&Maska nebo masky souborů:"
"Datei&maske (mehrere getrennt mit Komma):"
"F&ájlmaszk(ok, vesszővel elválasztva):"
"&Maska pliku lub kilka masek oddzielonych przecinkami:"
"&Máscara de archivo o múltiples máscaras de archivos:"
"Одна або кілька &масок файлів:"

FileAssocDescr
"&Описание ассоциации:"
"&Description of the association:"
"&Popis asociací:"
"&Beschreibung der Verknüpfung:"
"A &társítás leírása:"
"&Opis przypisania:"
"&Descripción de la asociación:"
"&Опис асоціації:"

FileAssocExec
"Команда, &выполняемая по Enter:"
"E&xecute command (used for Enter):"
"&Vykonat příkaz (použito pro Enter):"
"Befehl &ausführen (mit Enter):"
"&Végrehajtandó parancs (Enterre):"
"Polecenie (po naciśnięciu &Enter):"
"E&jecutar comando (usado por Enter):"
"Команда, &яка виконується за Enter:"

FileAssocAltExec
"Коман&да, выполняемая по Ctrl-PgDn:"
"Exec&ute command (used for Ctrl-PgDn):"
"V&ykonat příkaz (použito pro Ctrl-PgDn):"
"Befehl a&usführen (mit Strg-PgDn):"
"Vé&grehajtandó parancs (Ctrl-PgDown-ra):"
"Polecenie (po naciśnięciu &Ctrl-PgDn):"
"Ejecutar comando (usado por Ctrl-PgDn):"
"Коман&да, що виконується за Ctrl-PgDn:"

FileAssocView
"Команда &просмотра, выполняемая по F3:"
"&View command (used for F3):"
"Příkaz &Zobraz (použito pro F3):"
"Be&trachten (mit F3):"
"&Nézőke parancs (F3-ra):"
"&Podgląd (po naciśnięciu F3):"
"Comando de &visor (usado por F3):"
"Команда &перегляду, що виконується за F3:"

FileAssocAltView
"Команда просмотра, в&ыполняемая по Alt-F3:"
"V&iew command (used for Alt-F3):"
"Příkaz Z&obraz (použito pro Alt-F3):"
"Bet&rachten (mit Alt-F3):"
"N&ézőke parancs (Alt-F3-ra):"
"Podg&ląd (po naciśnięciu Alt-F3):"
"Comando de visor (usado por Alt-F3):"
"Команда перегляду, що в&иконується за Alt-F3:"

FileAssocEdit
"Команда &редактирования, выполняемая по F4:"
"&Edit command (used for F4):"
"Příkaz &Edituj (použito pro F4):"
"Bearb&eiten (mit F4):"
"S&zerkesztés parancs (F4-re):"
"&Edycja  (po naciśnięciu F4):"
"Comando de &editor (usado por F4):"
"Команда &редагування, що виконується за F4:"

FileAssocAltEdit
"Команда редактировани&я, выполняемая по Alt-F4:"
"Edit comm&and (used for Alt-F4):"
"Příkaz Editu&j (použito pro Alt-F4):"
"Bearbe&iten (mit Alt-F4):"
"Sze&rkesztés parancs (Alt-F4-re):"
"E&dycja  (po naciśnięciu Alt-F4):"
"Comando de editor (usado por Alt-F4):"
"Команда редагуванн&я, що виконується за Alt-F4:"
ViewF1
l:
l://Viewer: functional keys, 12 keys, except F2 - 2 keys, and F8 - 2 keys
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допмга"

ViewF2
le:// this is another text for F2
"Сверн"
"Wrap"
"Zalom"
"Umbr."
"SorTör"
"Zawiń"
"Divide"
"Згорн"

ViewF3
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

ViewF4
"Код"
"Hex"
"Hex"
"Hex"
"Hexa"
"Hex"
"Hexa"
"Код"

ViewF5
""
""
""
""
""
""
""
""

ViewF6
"Редакт"
"Edit"
"Edit"
"Bearb"
"Szerk."
"Edytuj"
"Editar"
"Редаг"

ViewF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Szukaj"
"Buscar"
"Пошук"

ViewF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Latin 2"
"ANSI"
"ANSI"

ViewF9
""
""
""
""
""
""
""
""

ViewF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

ViewF11
"Модули"
"Plugins"
"Plugin"
"Plugin"
"Plugin"
"Pluginy"
"Plugins"
"Модулі"

ViewF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant. "
"Екрани"

ViewF2Unwrap
"Развер"
"Unwrap"
"Nezal"
"KeinUm"
"NemTör"
"Unwrap"
"Unwrap"
"Розгор"

ViewF4Text
l:// this is another text for F4
"Текст"
"Text"
"Text"
"Text"
"Szöveg"
"Tekst"
"Text"
"Текст"

ViewF8DOS
"OEM"
"OEM"
"OEM"
"OEM"
"OEM"
"CP 1250"
"OEM"
"OEM"

ViewShiftF1
l:
l://Viewer: Shift
""
""
""
""
""
""
""
""

ViewShiftF2
"Слова"
"WWrap"
"ZalSlo"
"WUmbr"
"SzóTör"
"ZawińS"
"ConDiv"
"Слова"

ViewShiftF3
""
""
""
""
""
""
""
""

ViewShiftF4
""
""
""
""
""
""
""
""

ViewShiftF5
""
""
""
""
""
""
""
""

ViewShiftF6
""
""
""
""
""
""
""
""

ViewShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"TovKer"
"Następ"
"Próxim"
upd:"Далі"

ViewShiftF8
"КодСтр"
"CodePg"
upd:"ZnSady"
upd:"Tabell"
"Kódlap"
"Tabela"
"Tabla"
"КодСтор"

ViewShiftF9
""
""
""
""
""
""
""
""

ViewShiftF10
""
""
""
""
""
""
""
""

ViewShiftF11
""
""
""
""
""
""
""
""

ViewShiftF12
""
""
""
""
""
""
""
""

ViewAltF1
l:
l://Viewer: Alt
""
""
""
""
""
""
""
""

ViewAltF2
""
""
""
""
""
""
""
""

ViewAltF3
""
""
""
""
""
""
""
""

ViewAltF4
""
""
""
""
""
""
""
""

ViewAltF5
"Печать"
"Print"
"Tisk"
"Druck"
"Nyomt"
"Drukuj"
"Imprim"
"Друк"

ViewAltF6
""
""
""
""
""
""
""
""

ViewAltF7
"Назад"
"Prev"
"Předch"
"Letzt"
"VisKer"
"Poprz"
"Previo"
"Назад"

ViewAltF8
"Перейт"
"Goto"
"Jít na"
"GeheZu"
"Ugrás"
"IdźDo"
"Ir a.."
"Перейт"

ViewAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

ViewAltF10
"Закр.Far"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"Закр.Far"

ViewAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"Historia"
"HisVer"
"ІстПр"

ViewAltF12
""
""
""
""
""
""
""
""

ViewCtrlF1
l:
l://Viewer: Ctrl
""
""
""
""
""
""
""
""

ViewCtrlF2
""
""
""
""
""
""
""
""

ViewCtrlF3
""
""
""
""
""
""
""
""

ViewCtrlF4
""
""
""
""
""
""
""
""

ViewCtrlF5
""
""
""
""
""
""
""
""

ViewCtrlF6
""
""
""
""
""
""
""
""

ViewCtrlF7
""
""
""
""
""
""
""
""

ViewCtrlF8
""
""
""
""
""
""
""
""

ViewCtrlF9
""
""
""
""
""
""
""
""

ViewCtrlF10
"Позиц"
"GoFile"
"JítSou"
"GehDat"
"FájlPz"
"DoPlik"
"IrArch"
"Позиц"

ViewCtrlF11
""
""
""
""
""
""
""
""

ViewCtrlF12
""
""
""
""
""
""
""
""

ViewAltShiftF1
l:
l://Viewer: AltShift
""
""
""
""
""
""
""
""

ViewAltShiftF2
""
""
""
""
""
""
""
""

ViewAltShiftF3
""
""
""
""
""
""
""
""

ViewAltShiftF4
""
""
""
""
""
""
""
""

ViewAltShiftF5
""
""
""
""
""
""
""
""

ViewAltShiftF6
""
""
""
""
""
""
""
""

ViewAltShiftF7
""
""
""
""
""
""
""
""

ViewAltShiftF8
""
""
""
""
""
""
""
""

ViewAltShiftF9
"Конфиг"
"Config"
"Nastav"
"Konfig"
"Beáll."
"Konfig"
"Config"
"Конфіг"

ViewAltShiftF10
""
""
""
""
""
""
""
""

ViewAltShiftF11
""
""
""
""
""
""
""
""

ViewAltShiftF12
""
""
""
""
""
""
""
""

ViewCtrlShiftF1
l:
l://Viewer: CtrlShift
""
""
""
""
""
""
""
""

ViewCtrlShiftF2
""
""
""
""
""
""
""
""

ViewCtrlShiftF3
""
""
""
""
""
""
""
""

ViewCtrlShiftF4
""
""
""
""
""
""
""
""

ViewCtrlShiftF5
""
""
""
""
""
""
""
""

ViewCtrlShiftF6
""
""
""
""
""
""
""
""

ViewCtrlShiftF7
""
""
""
""
""
""
""
""

ViewCtrlShiftF8
""
""
""
""
""
""
""
""

ViewCtrlShiftF9
""
""
""
""
""
""
""
""

ViewCtrlShiftF10
""
""
""
""
""
""
""
""

ViewCtrlShiftF11
""
""
""
""
""
""
""
""

ViewCtrlShiftF12
""
""
""
""
""
""
""
""

ViewCtrlAltF1
l:
l://Viewer: CtrlAlt
""
""
""
""
""
""
""
""

ViewCtrlAltF2
""
""
""
""
""
""
""
""

ViewCtrlAltF3
""
""
""
""
""
""
""
""

ViewCtrlAltF4
""
""
""
""
""
""
""
""

ViewCtrlAltF5
""
""
""
""
""
""
""
""

ViewCtrlAltF6
""
""
""
""
""
""
""
""

ViewCtrlAltF7
""
""
""
""
""
""
""
""

ViewCtrlAltF8
""
""
""
""
""
""
""
""

ViewCtrlAltF9
""
""
""
""
""
""
""
""

ViewCtrlAltF10
""
""
""
""
""
""
""
""

ViewCtrlAltF11
""
""
""
""
""
""
""
""

ViewCtrlAltF12
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF1
l:
l://Viewer: CtrlAltShift
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF2
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF3
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF4
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF5
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF6
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF7
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF8
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF9
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF10
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF11
""
""
""
""
""
""
""
""

ViewCtrlAltShiftF12
le://end of functional keys (Viewer)
""
""
""
""
""
""
""
""

SingleViewF1
l:
l://Single Viewer: functional keys, 12 keys, except F2 - 2 keys, and F8 - 2 keys
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допмга"

SingleViewF2
"Сверн"
"Wrap"
"Zalom"
"Umbr."
"SorTör"
"Zawiń"
"Divide"
"Згорн"

SingleViewF3
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

SingleViewF4
"Код"
"Hex"
"Hex"
"Hex"
"Hexa"
"Hex"
"Hexa"
"Код"

SingleViewF5
""
""
""
""
""
""
""
""

SingleViewF6
"Редакт"
"Edit"
"Edit"
"Bearb"
"Szerk."
"Edytuj"
"Editar"
"Редаг"

SingleViewF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Szukaj"
"Buscar"
"Пошук"

SingleViewF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Latin 2"
"ANSI"
"ANSI"

SingleViewF9
""
""
""
""
""
""
""
""

SingleViewF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

SingleViewF11
"Модули"
"Plugins"
"Plugin"
"Plugins"
"Plugin"
"Pluginy"
"Plugins"
"Модулі"

SingleViewF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant.  "
"Екрани"

SingleViewF2Unwrap
l:// this is another text for F2
"Развер"
"Unwrap"
"Nezal"
"KeinUm"
"NemTör"
"Rozwij"
"Unwrap"
"Розгор"

SingleViewF4Text
l:// this is another text for F4
"Текст"
"Text"
"Text"
"Text"
"Szöveg"
"Tekst"
"Text"
"Текст"

SingleViewF8DOS
"OEM"
"OEM"
"OEM"
"OEM"
"OEM"
"CP 1250"
"OEM"
"OEM"

SingleViewShiftF1
l:
l://Single Viewer: Shift
""
""
""
""
""
""
""
""

SingleViewShiftF2
"Слова"
"WWrap"
"ZalSlo"
"WUmbr"
"SzóTör"
"ZawińS"
"ConDiv"
"Слова"

SingleViewShiftF3
""
""
""
""
""
""
""
""

SingleViewShiftF4
""
""
""
""
""
""
""
""

SingleViewShiftF5
""
""
""
""
""
""
""
""

SingleViewShiftF6
""
""
""
""
""
""
""
""

SingleViewShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"TovKer"
"Nast."
"Próxim"
"Далі"

SingleViewShiftF8
"КодСтр"
"CodePg"
upd:"ZnSady"
upd:"Tabell"
"Kódlap"
"Tabela"
"Tabla"
"КодСтр"

SingleViewShiftF9
""
""
""
""
""
""
""
""

SingleViewShiftF10
""
""
""
""
""
""
""
""

SingleViewShiftF11
""
""
""
""
""
""
""
""

SingleViewShiftF12
""
""
""
""
""
""
""
""

SingleViewAltF1
l:
l://Single Viewer: Alt
""
""
""
""
""
""
""
""

SingleViewAltF2
""
""
""
""
""
""
""
""

SingleViewAltF3
""
""
""
""
""
""
""
""

SingleViewAltF4
""
""
""
""
""
""
""
""

SingleViewAltF5
"Печать"
"Print"
"Tisk"
"Druck"
"Nyomt"
"Drukuj"
"Imprim"
"Друк"

SingleViewAltF6
""
""
""
""
""
""
""
""

SingleViewAltF7
"Назад"
"Prev"
"Předch"
"Letzt"
"VisKer"
"Poprz"
"Prev"
"Назад"

SingleViewAltF8
"Перейт"
"Goto"
"Jít na"
"GeheZu"
"Ugrás"
"IdźDo"
"Ir a.."
"Перейт"

SingleViewAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

SingleViewAltF10
"Закр.Far"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"Закр.Far"

SingleViewAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"Historia"
"HisVer"
"ІстПр"

SingleViewAltF12
""
""
""
""
""
""
""
""

SingleViewCtrlF1
l:
l://Single Viewer: Ctrl
""
""
""
""
""
""
""
""

SingleViewCtrlF2
""
""
""
""
""
""
""
""

SingleViewCtrlF3
""
""
""
""
""
""
""
""

SingleViewCtrlF4
""
""
""
""
""
""
""
""

SingleViewCtrlF5
""
""
""
""
""
""
""
""

SingleViewCtrlF6
""
""
""
""
""
""
""
""

SingleViewCtrlF7
""
""
""
""
""
""
""
""

SingleViewCtrlF8
""
""
""
""
""
""
""
""

SingleViewCtrlF9
""
""
""
""
""
""
""
""

SingleViewCtrlF10
""
""
""
""
""
""
""
""

SingleViewCtrlF11
""
""
""
""
""
""
""
""

SingleViewCtrlF12
""
""
""
""
""
""
""
""

SingleViewAltShiftF1
l:
l://Single Viewer: AltShift
""
""
""
""
""
""
""
""

SingleViewAltShiftF2
""
""
""
""
""
""
""
""

SingleViewAltShiftF3
""
""
""
""
""
""
""
""

SingleViewAltShiftF4
""
""
""
""
""
""
""
""

SingleViewAltShiftF5
""
""
""
""
""
""
""
""

SingleViewAltShiftF6
""
""
""
""
""
""
""
""

SingleViewAltShiftF7
""
""
""
""
""
""
""
""

SingleViewAltShiftF8
""
""
""
""
""
""
""
""

SingleViewAltShiftF9
"Конфиг"
"Config"
"Nastav"
"Konfig"
"Beáll."
"Konfig"
"Config"
"Конфіг"

SingleViewAltShiftF10
""
""
""
""
""
""
""
""

SingleViewAltShiftF11
""
""
""
""
""
""
""
""

SingleViewAltShiftF12
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF1
l:
l://Single Viewer: CtrlShift
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF2
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF3
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF4
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF5
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF6
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF7
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF8
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF9
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF10
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF11
""
""
""
""
""
""
""
""

SingleViewCtrlShiftF12
""
""
""
""
""
""
""
""

SingleViewCtrlAltF1
l:
l://Single Viewer: CtrlAlt
""
""
""
""
""
""
""
""

SingleViewCtrlAltF2
""
""
""
""
""
""
""
""

SingleViewCtrlAltF3
""
""
""
""
""
""
""
""

SingleViewCtrlAltF4
""
""
""
""
""
""
""
""

SingleViewCtrlAltF5
""
""
""
""
""
""
""
""

SingleViewCtrlAltF6
""
""
""
""
""
""
""
""

SingleViewCtrlAltF7
""
""
""
""
""
""
""
""

SingleViewCtrlAltF8
""
""
""
""
""
""
""
""

SingleViewCtrlAltF9
""
""
""
""
""
""
""
""

SingleViewCtrlAltF10
""
""
""
""
""
""
""
""

SingleViewCtrlAltF11
""
""
""
""
""
""
""
""

SingleViewCtrlAltF12
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF1
l:
l://Single Viewer: CtrlAltShift
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF2
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF3
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF4
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF5
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF6
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF7
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF8
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF9
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF10
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF11
""
""
""
""
""
""
""
""

SingleViewCtrlAltShiftF12
le://end of functional keys (Single Viewer)
""
""
""
""
""
""
""
""

InViewer
"просмотр %ls"
"view %ls"
"prohlížení %ls"
"Betrachte %ls"
"%ls megnézése"
"podgląd %ls"
"ver %ls"
"перегляд %ls"

InEditor
"редактирование %ls"
"edit %ls"
"editace %ls"
"Bearbeite %ls"
"%ls szerkesztése"
"edycja %ls"
"editar %ls"
"редагування %ls"

FilterTitle
l:
"Меню фильтров"
"Filters menu"
"Menu filtrů"
"Filtermenü"
"Szűrők menü"
"Filtry"
"Menú de Filtros"
"Меню фільтрів"

FilterBottom
"+,-,Пробел,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"
"+,-,Space,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"
"+,-,Mezera,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"
"+,-,Leer,I,X,BS,UmschBS,Einf,Entf,F4,F5,StrgUp,StrgDn"
"+,-,Szóköz,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Fel,Ctrl-Le"
"+,-,Spacja,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"
"Seleccione: '+','-',Space. Editor: Ins,Del,F4"
"+,-,Пробіл,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"

PanelFileType
"Файлы панели"
"Panel file type"
"Typ panelu souborů"
"Dateityp in Panel"
"A panel fájltípusa"
"Typ plików w panelu"
"Tipo de panel de archivo"
"Файли панелі"

FolderFileType
"Папки"
"Folders"
"Adresáře"
"Ordner"
"Mappák"
"Foldery"
"Directorios"
"Теки"

CanEditCustomFilterOnly
"Только пользовательский фильтр можно редактировать"
"Only custom filter can be edited"
"Jedině vlastní filtr může být upraven"
"Nur eigene Filter können editiert werden."
"Csak saját szűrő szerkeszthető"
"Tylko filtr użytkownika może być edytowany"
"Sólo filtro personalizado puede ser editado"
"Тільки фільтр користувача можна редагувати"

AskDeleteFilter
"Вы хотите удалить фильтр"
"Do you wish to delete the filter"
"Přejete si smazat filtr"
"Wollen Sie den eigenen Filter löschen"
"Törölni szeretné a szűrőt?"
"Czy chcesz usunąć filtr"
"Desea borrar el filtro"
"Ви хочете видалити фільтр"

CanDeleteCustomFilterOnly
"Только пользовательский фильтр может быть удалён"
"Only custom filter can be deleted"
"Jedině vlastní filtr může být smazán"
"Nur eigene Filter können gelöscht werden."
"Csak saját szűrő törölhető"
"Tylko filtr użytkownika może być usunięty"
"Sólo filtro personalizado puede ser borrado"
"Тільки фільтр користувача може бути видалений"

FindFileTitle
l:
"Поиск файла"
"Find file"
"Hledat soubor"
"Nach Dateien suchen"
"Fájlkeresés"
"Znajdź plik"
"Encontrar archivo"
"Пошук файлу"

FindFileMasks
"Одна или несколько &масок файлов:"
"A file &mask or several file masks:"
"Maska nebo masky souborů:"
"Datei&maske (mehrere getrennt mit Komma):"
"Fájlm&aszk(ok, vesszővel elválasztva):"
"&Maska pliku lub kilka masek oddzielonych przecinkami:"
"&Máscara de archivo o múltiples máscaras de archivos:"
"Одна або кілька &масок файлів:"

FindFileText
"&Содержащих текст:"
"Con&taining text:"
"Obsahující te&xt:"
"Enthält &Text:"
"&Tartalmazza a szöveget:"
"Zawierający &tekst:"
"Conteniendo &texto:"
"&Той, що містить текст:"

FindFileHex
"&Содержащих 16-ричный код:"
"Con&taining hex:"
"Obsahující &hex:"
"En&thält Hex (xx xx ...):"
"Tartalmazza a he&xát:"
"Zawierający wartość &szesnastkową:"
"Conteniendo Hexa:"
"&Вміст, що містить 16-річний код:"

FindFileCodePage
"Используя кодо&вую страницу:"
"Using code pa&ge:"
upd:"Použít &znakovou sadu:"
upd:"Zeichenta&belle verwenden:"
"Kó&dlap:"
"Użyj tablicy znaków:"
"Usando tabla de caracteres:"
"Використовуючи кодо&ву сторінку:"

FindFileCodePageBottom
"Space, Ins"
"Space, Ins"
"Space, Ins"
"Space, Ins"
"Space, Ins"
"Space, Ins"
"Espacio, Ins"
"Space, Ins"

FindFileCase
"&Учитывать регистр"
"&Case sensitive"
"Roz&lišovat velikost písmen"
"Gr&oß-/Kleinschreibung"
"&Nagy/kisbetű érzékeny"
"&Uwzględnij wielkość liter"
"Sensible min/ma&yúsc."
"&Враховувати регістр"

FindFileWholeWords
"Только &целые слова"
"&Whole words"
"&Celá slova"
"Nur &ganze Wörter"
"Csak egés&z szavak"
"Tylko &całe słowa"
"&Palabras completas"
"Тільки &цілі слова"

FindFileAllCodePages
"Все кодовые страницы"
"All code pages"
upd:"Všechny znakové sady"
upd:"Alle Zeichentabellen"
"Minden kódlappal"
"Wszystkie zainstalowane"
"Todas las tablas de caracteres"
"Всі кодові сторінки"

FindArchives
"Искать в а&рхивах"
"Search in arch&ives"
"Hledat v a&rchívech"
"In Arch&iven suchen"
"Keresés t&ömörítettekben"
"Szukaj w arc&hiwach"
"Buscar en archivos compr&imidos"
"Шукати в а&рхівах"

FindFolders
"Искать п&апки"
"Search for f&olders"
"Hledat a&dresáře"
"Nach &Ordnern suchen"
"Keresés mapp&ákra"
"Szukaj &folderów"
"Buscar por direct&orios"
"Шукати т&еки"

FindSymLinks
"Искать в символи&ческих ссылках"
"Search in symbolic lin&ks"
"Hledat v s&ymbolických lincích"
"In symbolischen Lin&ks suchen"
"Keresés sz&imbolikus linkekben"
"Szukaj w &linkach"
"Buscar en enlaces simbólicos"
"Шукати в символі&чних посиланнях"

SearchForHex
"Искать 16-ричн&ый код"
"Search for &hex"
"Hledat &hex"
"Nach &Hex suchen"
"Keresés &hexákra"
"Szukaj wartości &szesnastkowej"
"Buscar por &hexa"
"Шукати 16-річн&ий код"

SearchWhere
"Выберите &область поиска:"
"Select search &area:"
upd:"Zvolte oblast hledání:"
upd:"Suchbereich:"
"Keresés hatós&ugara:"
"Obszar wyszukiwania:"
"Seleccionar área de búsqueda:"
"Виберіть &область пошуку:"

SearchAllDisks
"На всех несъёмных &дисках"
"In &all non-removable drives"
"Ve všech p&evných discích"
"Auf &allen festen Datenträger"
"Minden &fix meghajtón"
"Na dyskach &stałych"
"Buscar en todas las unidades no-removibles"
"На всіх незнімних &дисках"

SearchAllButNetwork
"На всех &локальных дисках"
"In all &local drives"
"Ve všech &lokálních discích"
"Auf allen &lokalen Datenträgern"
"Minden hel&yi meghajtón"
"Na dyskach &lokalnych"
"Buscar en todas las unidades locales"
"На всіх &локальних дисках"

SearchInPATH
"В PATH-катало&гах"
"In &PATH folders"
"V adresářích z &PATH"
"In &PATH-Ordnern"
"A &PATH mappáiban"
"W folderach zmiennej &PATH"
"En directorios de variable &PATH"
"У PATH-катало&гах"

SearchFromRootOfDrive
"С кор&ня диска"
"From the &root of"
"V &kořeni"
"Ab Wu&rzelverz. von"
"Meghajtó &gyökerétől:"
"Od &korzenia"
"Buscar desde directorio &raíz de"
"З кор&іння диска"

SearchFromRootFolder
"С кор&невой папки"
"From the &root folder"
"V kořeno&vém adresáři"
"Ab Wu&rzelverzeichnis"
"A &gyökérmappától"
"Od katalogu &głównego"
"Buscar desde la &raíz del directorio"
"З кор&еневої теки"

SearchFromCurrent
"С &текущей папки"
"From the curre&nt folder"
"V tomto adresář&i"
"Ab dem aktuelle&n Ordner"
"Az akt&uális mappától"
"Od &bieżącego katalogu"
"Buscar desde directorio actual"
"З &поточної теки"

SearchInCurrent
"Только в теку&щей папке"
"The current folder onl&y"
"P&ouze v tomto adresáři"
"Nur im aktue&llen Ordner"
"&Csak az aktuális mappában"
"&Tylko w bieżącym katalogu"
"Buscar en el directorio actua&l solamente"
"Тільки в пото&чній теці"

SearchInSelected
"В &отмеченных папках"
"&Selected folders"
"Ve vy&braných adresářích"
"In au&sgewählten Ordner"
"A ki&jelölt mappákban"
"W &zaznaczonych katalogach"
"Buscar en directorios &seleccionados"
"У &відмічених теках"

FindUseFilter
"Исполь&зовать фильтр"
"&Use filter"
"Použít f&iltr"
"Ben&utze Filter"
"Sz&űrővel"
"&Filtruj"
"&Usar filtro"
"Викорис&товувати фільтр"

FindUsingFilter
"используя фильтр"
"using filter"
"používám filtr"
"mit Filter"
"szűrővel"
"używając filtra"
"usando filtro"
"використовуючи фільтр"

FindFileFind
"&Искать"
"&Find"
"&Hledat"
"&Suchen"
"K&eres"
"Szuka&j"
"&Encontrar"
"&Шукати"

FindFileDrive
"Дис&к"
"Dri&ve"
"D&isk"
"Lauf&werk"
"Meghajt&ó"
"&Dysk"
"Uni&dad"
"Дис&к"

FindFileSetFilter
"&Фильтр"
"Filt&er"
"&Filtr"
"Filt&er"
"Szű&rő"
"&Filtr"
"Filtr&o"
"&Фільтр"

FindFileAdvanced
"До&полнительно"
"Advance&d"
"Pokr&očilé"
"Er&weitert"
"Ha&ladó"
"&Zaawansowane"
"Avanza&da"
"До&датково"

FindSearchingIn
"Поиск%ls в"
"Searching%ls in"
"Hledám%ls v"
"Suche%ls in"
"%ls keresése"
"Szukam%ls w"
"Buscando%ls en:"
"Пошук%ls в"

FindNewSearch
"&Новый поиск"
"&New search"
"&Nové hledání"
"&Neue Suche"
"&Új keresés"
"&Od nowa..."
"&Nueva búsqueda"
"&Новий пошук"

FindGoTo
"Пе&рейти"
"&Go to"
"&Jdi na"
"&Gehe zu"
"U&grás"
"&Idź do"
"&Ir a"
"Пе&рейти"

FindView
"&Просм"
"&View"
"Zo&braz"
"&Betrachten"
"Meg&néz"
"&Podgląd"
"&Ver "
"&Прогл"

FindEdit
"&Редакт"
"&Edit"
upd:"&Edit"
upd:"&Edit"
upd:"&Edit"
upd:"&Edit"
upd:"&Edit"
"&Редаг"

FindPanel
"Пане&ль"
"&Panel"
"&Panel"
"&Panel"
"&Panel"
"&Do panelu"
"&Panel"
"Пане&ль"

FindStop
"С&топ"
"&Stop"
"&Stop"
"&Stoppen"
"&Állj"
"&Stop"
"D&etener"
"С&топ"

FindDone
l:
"Поиск закончен. Найдено файлов: %d, папок: %d"
"Search done. Found files: %d, folders: %d"
"Hledání ukončeno. Nalezeno %d soubor(ů) a %d adresář(ů)"
"Suche beendet. %d Datei(en) und %d Ordner gefunden."
"A keresés kész. %d fájlt és %d mappát találtam."
"Wyszukiwanie zakończone (znalazłem %d plików i %d folderów)"
"Búsqueda finalizada. Encontrados %d archivo(s) y %d directorio(s)"
"Пошук закінчено. Знайдено файли: %d, теки: %d"

FindCancel
"Отм&ена"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"Від&міна"

FindFound
l:
" Файлов: %d, папок: %d "
" Files: %d, folders: %d "
" Souborů: %d, adresářů: %d "
" Dateien: %d, Ordner: %d "
" Fájlt: %d, mappát: %d "
" Plików: %d, folderów: %d "
" Ficheros: %d, carpetas: %d "
" Файлів: %d, тек: %d "

FindFileFolder
l:
"Папка"
"Folder"
"Adresář"
"Ordner"
"Mappa"
"Katalog"
" DIR  "
"Тека"

FindFileSymLink
"Ссылка"
"Symlink"
"Link"
"Symlink"
"SzimLnk"
"LinkSym"
"SimbEnl"
"Посилання"

FindFileJunction
"Связь"
"Junction"
"Křížení"
"Knoten"
"Csomópt"
"Dowiązania"
"Union"
"Зв'язок"

FindFileAdvancedTitle
l:
"Дополнительные параметры поиска"
"Find file advanced options"
"Pokročilé nastavení vyhledávání souborů"
"Erweiterte Optionen"
"Fájlkeresés haladó beállításai"
"Zaawansowane opcje wyszukiwania"
"Opciones avanzada de búsqueda de archivo"
"Додаткові параметри пошуку"

FindFileSearchFirst
"Проводить поиск в &первых:"
"Search only in the &first:"
"Hledat po&uze v prvních:"
"Nur &in den ersten x Bytes:"
"Keresés csak az első &x bájtban:"
"Szukaj wyłącznie w &pierwszych:"
"Buscar solamente en el &primer:"
"Проводити пошук у &перших:"

FindFileSearchOutputFormat
"&Формат вывода:"
"&Output format:"
upd:"&Output format:"
upd:"&Output format:"
upd:"&Output format:"
upd:"&Output format:"
upd:"&Output format:"
"&Формат виведення:"

FindAlternateStreams
"Обрабатывать &альтернативные потоки данных"
"Process &alternate data streams"
upd:"Process &alternate data streams"
upd:"Process &alternate data streams"
"&Alternatív adatsávok (stream) feldolgozása"
upd:"Process &alternate data streams"
"Procesar flujo alternativo de datos"
"Обробляти &альтернативні потоки даних"

FindAlternateModeTypes
"&Типы колонок"
"Column &types"
"&Typ sloupců"
"Spalten&typen"
"Oszlop&típusok"
"&Typy kolumn"
"&Tipos de columna"
"&Типи колонок"

FindAlternateModeWidths
"&Ширина колонок"
"Column &widths"
"Šíř&ka sloupců"
"Spalten&breiten"
"Oszlop&szélességek"
"&Szerokości kolumn"
"Anc&ho de columna"
"&Ширина колонок"

FoldTreeSearch
l:
"Поиск:"
"Search:"
"Hledat:"
"Suchen:"
"Keresés:"
"Wyszukiwanie:"
"Buscar:"
"Пошук:"

GetCodePageTitle
l:
"Кодовые страницы"
"Code pages"
upd:"Znakové sady:"
upd:"Tabellen"
"Kódlapok"
"Strony kodowe"
"Tablas"
"Кодові сторінки"

GetCodePageSystem
"Системные"
"System"
upd:"System"
upd:"System"
"Rendszer"
upd:"System"
"Sistema"
"Системні"

GetCodePageUnicode
"Юникод"
"Unicode"
upd:"Unicode"
upd:"Unicode"
"Unicode"
upd:"Unicode"
"Unicode"
"Юнікод"

GetCodePageFavorites
"Избранные"
"Favorites"
upd:"Favorites"
upd:"Favorites"
"Kedvencek"
upd:"Favorites"
"Favoritos"
"Вибрані"

GetCodePageOther
"Прочие"
"Other"
upd:"Other"
upd:"Other"
"Egyéb"
upd:"Other"
"Otro"
"Інші"

GetCodePageBottomTitle
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"
"Strg-H, Entf, Einf, F4"
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"

GetCodePageBottomShortTitle
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"
"Strg-H, Entf, F4"
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"

GetCodePageEditCodePageName
"Изменить имя кодовой страницы"
"Edit code page name"
upd:"Edit code page name"
upd:"Edit code page name"
upd:"Edit code page name"
upd:"Edit code page name"
"Editar nombre de tabla (codepage)"
"Змінити ім'я кодової сторінки"

GetCodePageResetCodePageName
"&Сбросить"
"&Reset"
upd:"&Reset"
upd:"&Reset"
upd:"&Reset"
upd:"&Reset"
"&Reiniciar"
"&Скинути"

HighlightTitle
l:
"Раскраска файлов"
"Files highlighting"
"Zvýrazňování souborů"
"Farbmarkierungen"
"Fájlkiemelések, rendezési csoportok"
"Wyróżnianie plików"
"Resaltado de archivos"
"Розмальовка файлів"

HighlightBottom
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"
"Ins,Del,F4,F5,Ctrl-Nahoru,Ctrl-Dolů"
"Einf,Entf,F4,F5,StrgUp,StrgDown"
"Ins,Del,F4,F5,Ctrl-Fel,Ctrl-Le"
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"

HighlightUpperSortGroup
"Верхняя группа сортировки"
"Upper sort group"
"Vzesupné řazení"
"Obere Sortiergruppen"
"Felsőbbrendű csoport"
"Górna grupa sortowania"
"Grupo de ordenamiento de arriba"
"Верхня група сортування"

HighlightLowerSortGroup
"Нижняя группа сортировки"
"Lower sort group"
"Sestupné řazení"
"Untere Sortiergruppen"
"Alsóbbrendű csoport"
"Dolna grupa sortowania"
"Grupo de ordenamiento de abajo"
"Нижня група сортування"

HighlightLastGroup
"Наименее приоритетная группа раскраски"
"Lowest priority highlighting group"
"Zvýraznění nejnižší prority"
"Farbmarkierungen mit niedrigster Priorität"
"Legalacsonyabb rendű csoport"
"Grupa wyróżniania o najniższym priorytecie"
"Resaltado de grupo con baja prioridad"
"Найменш пріоритетна група розмальовки"

HighlightAskDel
"Вы хотите удалить раскраску для"
"Do you wish to delete highlighting for"
"Přejete si smazat zvýraznění pro"
"Wollen Sie Farbmarkierungen löschen für"
"Biztosan törli a kiemelést?"
"Czy chcesz usunąć wyróżnianie dla"
"Desea borrar resaltado para"
"Ви хочете видалити розмальовку для"

HighlightWarning
"Будут потеряны все Ваши настройки"
"You will lose all changes"
"Všechny změny budou ztraceny"
"Sie verlieren jegliche Änderungen"
"Minden változtatás elvész"
"Wszystkie zmiany zostaną utracone"
"Usted perderá todos los cambios"
"Втрачені всі Ваші налаштування"

HighlightAskRestore
"Вы хотите восстановить раскраску файлов по умолчанию?"
"Do you wish to restore default highlighting?"
"Přejete si obnovit výchozí nastavení?"
"Wollen Sie Standard-Farbmarkierungen wiederherstellen?"
"Visszaállítja az alapértelmezett kiemeléseket?"
"Czy przywrócić wyróżnianie domyślne?"
"Desea restablecer resaltado por defecto?"
"Ви хочете відновити забарвлення файлів за замовчуванням?"

HighlightEditTitle
l:
"Редактирование раскраски файлов"
"Edit files highlighting"
"Upravit zvýrazňování souborů"
"Farbmarkierungen bearbeiten"
"Fájlkiemelés szerkesztése"
"Edytuj wyróżnianie plików"
"Editar resaltado de archivos"
"Редагування розмальовки файлів"

HighlightMarkChar
"Оп&циональный символ пометки,"
"Optional markin&g character,"
"Volitelný &znak pro označení určených souborů,"
"Optionale Markierun&g mit Zeichen,"
"Megadható &jelölő karakter"
"Opcjonalny znak &wyróżniający zaznaczone pliki,"
"Ca&racter opcional para marcar archivos específicos"
"Оп&ціональний символ позначки,"

HighlightTransparentMarkChar
"прозра&чный"
"tra&nsparent"
"průh&ledný"
"tra&nsparent"
"át&látszó"
"prze&zroczyste"
"tra&nsparente"
"проз&орий"

HighlightColors
" Цвета файлов (\"чёрный на чёрном\" - цвет по умолчанию) "
" File name colors (\"black on black\" - default color) "
" Barva názvu souborů (\"černá na černé\" - výchozí barva) "
" Dateinamenfarben (\"Schwarz auf Schwarz\"=Standard) "
" Fájlnév színek (feketén fekete = alapértelmezett szín) "
" Kolory nazw plików (domyślny - \"czarny na czarnym\") "
" Colores de archivos (\"negro en negro\" - color por defecto) "
" Кольори файлів (\"чорний на чорному\" ​​- колір за замовчуванням) "

HighlightFileName1
"&1. Обычное имя файла                "
"&1. Normal file name               "
"&1. Normální soubor            "
"&1. Normaler Dateiname             "
"&1. Normál fájlnév                  "
"&1. Nazwa pliku bez zaznaczenia    "
"&1. Normal  "
"&1. Звичайне ім'я файлу              "

HighlightFileName2
"&3. Помеченное имя файла             "
"&3. Selected file name             "
"&3. Vybraný soubor             "
"&3. Markierter Dateiame            "
"&3. Kijelölt fájlnév                "
"&3. Zaznaczenie                    "
"&3. Seleccionado"
"&3. Позначене ім'я файлу             "

HighlightFileName3
"&5. Имя файла под курсором           "
"&5. File name under cursor         "
"&5. Soubor pod kurzorem        "
"&5. Dateiname unter Cursor         "
"&5. Kurzor alatti fájlnév           "
"&5. Nazwa pliku pod kursorem       "
"&5. Bajo cursor "
"&5. Ім'я файлу під курсором          "

HighlightFileName4
"&7. Помеченное под курсором имя файла"
"&7. File name selected under cursor"
"&7. Vybraný soubor pod kurzorem"
"&7. Dateiname markiert unter Cursor"
"&7. Kurzor alatti kijelölt fájlnév  "
"&7. Zaznaczony plik pod kursorem   "
"&7. Se&leccionado bajo cursor"
"&7. Позначене під курсором ім'я файлу"

HighlightMarking1
"&2. Пометка"
"&2. Marking"
"&2. Označení"
"&2. Markierung"
"&2. Jelölő kar.:"
"&2. Zaznaczenie"
"&2. Marcado"
"&2. Помітка"

HighlightMarking2
"&4. Пометка"
"&4. Marking"
"&4. Označení"
"&4. Markierung"
"&4. Jelölő kar.:"
"&4. Zaznaczenie"
"&4. Marcado"
"&4. Помітка"

HighlightMarking3
"&6. Пометка"
"&6. Marking"
"&6. Označení"
"&6. Markierung"
"&6. Jelölő kar.:"
"&6. Zaznaczenie"
"&6. Marcado"
"&6. Помітка"

HighlightMarking4
"&8. Пометка"
"&8. Marking"
"&8. Označení"
"&8. Markierung"
"&8. Jelölő kar.:"
"&8. Zaznaczenie"
"&8. Marcado"
"&8. Помітка"

HighlightExample1
"║filename.ext │"
"║filename.ext │"
"║filename.ext │"
"║dateinam.erw │"
"║fájlneve.kit │"
"║nazwa.roz    │"
"║nombre.ext   │"
"║filename.ext │"

HighlightExample2
"║ filename.ext│"
"║ filename.ext│"
"║ filename.ext│"
"║ dateinam.erw│"
"║ fájlneve.kit│"
"║ nazwa.roz   │"
"║ nombre.ext  │"
"║filename.ext │"

HighlightContinueProcessing
"Продолжать &обработку"
"C&ontinue processing"
"Pokračovat ve zpracová&ní"
"Verarbeitung f&ortsetzen"
"Folyamatos f&eldolgozás"
"K&ontynuuj przetwarzanie"
"C&ontinuar procesando"
"Продовжувати &обробку"

InfoTitle
l:
"Информация"
"Information"
"Informace"
"Informationen"
"Információk"
"Informacja"
"Información"
"Інформация"

InfoCompName
"Имя компьютера"
"Computer name"
"Název počítače"
"Computername"
"Számítógép neve"
"Nazwa komputera"
"Nombre computadora"
"Им'я комп'ютера"

InfoUserName
"Имя пользователя"
"User name"
"Jméno uživatele"
"Benutzername"
"Felhasználói név"
"Nazwa użytkownika"
"Nombre usuario"
"Им'я користувача"

InfoRemovable
"Сменный"
"Removable"
"Vyměnitelný"
"Austauschbares"
"Kivehető"
"Wyjmowalny"
"Removible"
"Змінний"

InfoFixed
"Жёсткий"
"Fixed"
"Pevný"
"Lokales"
"Fix"
"Stały"
"Rígido"
"Жорсткий"

InfoNetwork
"Сетевой"
"Network"
"Síťový"
"Netzwerk"
"Hálózati"
"Sieciowy"
"Red"
"Мережевий"

InfoCDROM
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"

InfoCD_RW
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"

InfoCD_RWDVD
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"

InfoDVD_ROM
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"

InfoDVD_RW
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"

InfoDVD_RAM
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"

InfoRAM
"RAM"
"RAM"
"RAM"
"RAM"
"RAM"
"RAM"
"RAM"
"RAM"

InfoSUBST
"SUBST"
"Subst"
"SUBST"
"Subst"
"Virtuális"
"Subst"
"Subst"
"SUBST"

InfoVirtual
"Виртуальный"
"Virtual"
upd:"Virtual"
upd:"Virtual"
upd:"Virtual"
upd:"Virtual"
upd:"Virtual"
"Віртуальний"

InfoDisk
"диск"
"disk"
"disk"
"Laufwerk"
"lemez"
"dysk"
"disco"
"диск"

InfoDiskTotal
"Всего байтов"
"Total bytes"
"Celkem bytů"
"Bytes gesamt"
"Összes bájt"
"Razem bajtów"
"Total de bytes"
"Всього байтів"

InfoDiskFree
"Свободных байтов"
"Free bytes"
"Volných bytů"
"Bytes frei"
"Szabad bájt"
"Wolnych bajtów"
"Bytes libres"
"Вільних байтів"

InfoDiskLabel
"Метка тома"
"Volume label"
"Popisek disku"
"Laufwerksbezeichnung"
"Kötet címke"
"Etykieta woluminu"
"Etiqueta de volumen"
"Мітка тома"

InfoDiskNumber
"Серийный номер"
"Serial number"
"Sériové číslo"
"Seriennummer"
"Sorozatszám"
"Numer seryjny"
"Número de serie"
"Серійний номер"

InfoMemory
" Память "
" Memory "
" Paměť "
" Speicher "
" Memória "
" Pamięć "
" Memoria "
" Пам'ять "

InfoMemoryLoad
"Загрузка памяти"
"Memory load"
"Zatížení paměti"
"Speicherverbrauch"
"Használt memória"
"Użycie pamięci"
"Carga en Memoria"
"Завантаження пам'яті"

InfoMemoryInstalled
"Установлено памяти"
"Installed memory"
upd:"Installed memory"
upd:"Installed memory"
upd:"Installed memory"
upd:"Installed memory"
"Memoria instalada"
"Встановлено пам'яті"

InfoMemoryTotal
"Всего памяти"
"Total memory"
"Celková paměť"
"Speicher gesamt"
"Összes memória"
"Całkowita pamięć"
"Total memoria"
"Всього пам'яті"

InfoMemoryFree
"Свободно памяти"
"Free memory"
"Volná paměť"
"Speicher frei"
"Szabad memória"
"Wolna pamięć"
"Memoria libre"
"Вільно пам'яті"

InfoSharedMemory
"Разделяемая память"
"Shared memory"
upd:"Shared RAM"
upd:"Shared RAM"
upd:"Shared RAM"
upd:"Shared RAM"
upd:"Shared RAM"
"Спільна пам'ять"

InfoBufferMemory
"Буферизованная память"
"Buffer memory"
upd:"Buffer RAM"
upd:"Buffer RAM"
upd:"Buffer RAM"
upd:"Buffer RAM"
upd:"Buffer RAM"
"Буферизована пам'ять"

InfoPageFileTotal
"Всего файла подкачки"
"Total paging file"
upd:"Total paging file"
upd:"Total paging file"
upd:"Total paging file"
upd:"Total paging file"
"Archivo de paginación total"
"Всього файлу підкачки"

InfoPageFileFree
"Свободно файла подкачки"
"Free paging file"
upd:"Free paging file"
upd:"Free paging file"
upd:"Free paging file"
upd:"Free paging file"
"Archivo de paginación libre"
"Вільно файлу підкачки"

InfoDizAbsent
"Файл описания папки отсутствует"
"Folder description file is absent"
"Soubor s popisem adresáře chybí"
"Keine Datei mit Ordnerbeschreibungen vorhanden."
"Mappa megjegyzésfájl nincs"
"Plik opisu katalogu nie istnieje"
"archivo descripción del directorio está ausente"
"Файл опису теки відсутній"

ErrorInvalidFunction
l:
"Некорректная функция"
"Incorrect function"
"Nesprávná funkce"
"Ungültige Funktion"
"Helytelen funkció"
"Niewłaściwa funkcja"
"Función incorrecta"
"Некоректна функція"

ErrorBadCommand
"Команда не распознана"
"Command not recognized"
"Příkaz nebyl rozpoznán"
"Unbekannter Befehl"
"Ismeretlen parancs"
"Nieznane polecenie"
"Comando no reconocido"
"Команда не розпізнана"

ErrorFileNotFound
"Файл не найден"
"File not found"
"Soubor nenalezen"
"Datei nicht gefunden"
"A fájl vagy mappa nem található"
"Nie odnaleziono pliku"
"archivo no encontrado"
"Файл не знайдено"

ErrorPathNotFound
"Путь не найден"
"Path not found"
"Cesta nenalezena"
"Pfad nicht gefunden"
"Az elérési út nem található"
"Nie odnaleziono ścieżki"
"Ruta no encontrada"
"Шлях не знайдено"

ErrorTooManyOpenFiles
"Слишком много открытых файлов"
"Too many open files"
"Příliš mnoho otevřených souborů"
"Zu viele geöffnete Dateien"
"Túl sok nyitott fájl"
"Zbyt wiele otwartych plików"
"Demasiados archivos abiertos"
"Занадто багато відкритих файлів"

ErrorAccessDenied
"Доступ запрещён"
"Access denied"
"Přístup odepřen"
"Zugriff verweigert"
"Hozzáférés megtagadva"
"Dostęp zabroniony"
"Acceso denegado"
"Доступ заборонено"

ErrorNotEnoughMemory
"Недостаточно памяти"
"Not enough memory"
"Nedostatek paměti"
"Nicht genügend Speicher"
"Nincs elég memória"
"Za mało pamięci"
"No hay memoria libre"
"Недостатньо пам'яті"

ErrorDiskRO
"Попытка записи на защищённый от записи диск"
"Cannot write to write protected disk"
"Nelze zapisovat na disk chráněný proti zápisu"
"Der Datenträger ist schreibgeschützt"
"Írásvédett lemezre nem lehet írni"
"Nie mogę zapisać na zabezpieczony dysk"
"No se puede escribir a disco protegido contra escritura"
"Спроба запису на захищений від запису диск"

ErrorDeviceNotReady
"Устройство не готово"
"The device is not ready"
"Zařízení není připraveno"
"Das Gerät ist nicht bereit"
"Az eszköz nem kész"
"Urządzenie nie jest gotowe"
"El dispositivo no está listo"
"Пристрій не готовий"

ErrorCannotAccessDisk
"Доступ к диску невозможен"
"Disk cannot be accessed"
"Na disk nelze přistoupit"
"Auf Datenträger kann nicht zugegriffen werden"
"A lemez nem érhető el"
"Brak dostępu do dysku"
"Disco no puede ser accedido"
"Доступ до диска неможливий"

ErrorSectorNotFound
"Сектор не найден"
"Sector not found"
"Sektor nenalezen"
"Sektor nicht gefunden"
"Szektor nem található"
"Nie odnaleziono sektora"
"Sector no encontrado"
"Сектор не знайдено"

ErrorOutOfPaper
"В принтере нет бумаги"
"The printer is out of paper"
"V tiskárně došel papír"
"Der Drucker hat kein Papier mehr"
"A nyomtatóban nincs papír"
"Brak papieru w drukarce"
"No hay papel en la impresora"
"У принтері немає паперу"

ErrorWrite
"Ошибка записи"
"Write fault error"
"Chyba zápisu"
"Fehler beim Schreibzugriff"
"Írási hiba"
"Błąd zapisu"
"Falla de escritura"
"Помилка запису"

ErrorRead
"Ошибка чтения"
"Read fault error"
"Chyba čtení"
"Fehler beim Lesezugriff"
"Olvasási hiba"
"Błąd odczytu"
"Falla de lectura"
"Помилка читання"

ErrorDeviceGeneral
"Общая ошибка устройства"
"Device general failure"
"Obecná chyba zařízení"
"Ein Gerätefehler ist aufgetreten"
"Eszköz általános hiba"
"Ogólny błąd urządzenia"
"Falla general en dispositivo"
"Загальна помилка пристрою"

ErrorFileSharing
"Нарушение совместного доступа к файлу"
"File sharing violation"
"Narušeno sdílení souborů"
"Zugriffsverletzung"
"Fájlmegosztási hiba"
"Naruszenie zasad współużytkowania pliku"
"Violación de archivo compartido"
"Порушення спільного доступу до файлу"

ErrorNetworkPathNotFound
"Сетевой путь не найден"
"The network path was not found"
"Síťová cesta nebyla nalezena"
"Der Netzwerkpfad wurde nicht gefunden"
"Hálózati útvonal nem található"
"Nie odnaleziono ścieżki sieciowej"
"La ruta de red no ha sido encontrada"
"Мережевий шлях не знайдено"

ErrorNetworkBusy
"Сеть занята"
"The network is busy"
"Síť je zaneprázdněna"
"Das Netzwerk ist beschäftigt"
"A hálózat zsúfolt"
"Sieć jest zajęta"
"La red está ocupada"
"Мережа зайнята"

ErrorNetworkAccessDenied
"Сетевой доступ запрещён"
"Network access is denied"
"Přístup na síť zakázán"
"Netzwerkzugriff wurde verweigert"
"Hálózati hozzáférés megtagadva"
"Dostęp do sieci zabroniony"
"Acceso a red es denegado"
"Мережевий доступ заборонено"

ErrorNetworkWrite
"Ошибка записи в сети"
"A write fault occurred on the network"
"Na síti došlo k chybě v zápisu"
"Fehler beim Schreibzugriff auf das Netzwerk"
"Írási hiba a hálózaton"
"Wystąpił błąd zapisu w sieci"
"Falla de escritura en la red"
"Помилка запису в мережі"

ErrorDiskLocked
"Диск используется или заблокирован другим процессом"
"The disk is in use or locked by another process"
"Disk je používán nebo uzamčen jiným procesem"
"Datenträger wird verwendet oder ist durch einen anderen Prozess gesperrt"
"A lemezt használja vagy zárolja egy folyamat"
"Dysk jest w użyciu lub zablokowany przez inny proces"
"El disco está en uso o bloqueado por otro proceso"
"Диск використовується або заблокований іншим процесом"

ErrorFileExists
"Файл или папка уже существует"
"File or folder already exists"
"Soubor nebo adresář již existuje"
"Die Datei oder der Ordner existiert bereits."
"A fájl vagy mappa már létezik"
"Plik lub katalog już istnieje"
"archivo o directorio ya existe"
"Файл або тека вже існує"

ErrorInvalidName
"Указанное имя неверно"
"The specified name is invalid"
"Zadaný název je neplatný"
"Der angegebene Name ist ungültig"
"A megadott név érvénytelen"
"Podana nazwa jest niewłaściwa"
"El nombre especificado es inválido"
"Вказане ім'я неправильне"

ErrorInsufficientDiskSpace
"Нет места на диске"
"Insufficient disk space"
"Nedostatek místa na disku"
"Unzureichend Speicherplatz am Datenträger"
"Nincs elég hely a lemezen"
"Za mało miejsca na dysku"
"Insuficiente espacio de disco"
"Немає місця на диску"

ErrorFolderNotEmpty
"Папка не пустая"
"The folder is not empty"
"Adresář není prázdný"
"Der Ordner ist nicht leer"
"A mappa nem üres"
"Katalog nie jest pusty"
"El directorio no está vacío"
"Тека не порожня"

ErrorIncorrectUserName
"Неверное имя пользователя"
"Incorrect user name"
"Neplatné jméno uživatele"
"Ungültiger Benutzername"
"Érvénytelen felhasználói név"
"Niewłaściwa nazwa użytkownika"
"Nombre de usuario incorrecto"
"Неправильне ім'я користувача"

ErrorIncorrectPassword
"Неверный пароль"
"Incorrect password"
"Neplatné heslo"
"Ungültiges Passwort"
"Érvénytelen jelszó"
"Niewłaściwe hasło"
"Clave incorrecta"
"Невірний пароль"

ErrorLoginFailure
"Ошибка регистрации"
"Login failure"
"Přihlášení selhalo"
"Login fehlgeschlagen"
"Sikertelen bejelentkezés"
"Logowanie nie powiodło się"
"Falla en conexión"
"Помилка реєстрації"

ErrorConnectionAborted
"Соединение разорвано"
"Connection aborted"
"Spojení přerušeno"
"Verbindung abgebrochen"
"Kapcsolat bontva"
"Połączenie zerwane"
"Conexión abortada"
"З'єднання розірвано"

ErrorCancelled
"Операция отменена"
"Operation cancelled"
"Operace stornována"
"Vorgang abgebrochen"
"A művelet megszakítva"
"Operacja przerwana"
"Operación cancelada"
"Операцію скасовано"

ErrorNetAbsent
"Сеть отсутствует"
"No network present"
"Síť není k dispozici"
"Kein Netzwerk verfügbar"
"Nincs hálózat"
"Brak sieci"
"No hay red presente"
"Мережа відсутня"

ErrorNetDeviceInUse
"Устройство используется и не может быть отсоединено"
"Device is in use and cannot be disconnected"
"Zařízení se používá a nemůže být odpojeno"
"Gerät wird gerade verwendet oder kann nicht getrennt werden"
"Az eszköz használatban van, nem választható le"
"Urządzenie jest w użyciu i nie można go odłączyć"
"Dispositivo está en uso y no puede ser desconectado"
"Пристрій використовується і не може бути від'єднаний"

ErrorNetOpenFiles
"На сетевом диске есть открытые файлы"
"This network connection has open files"
"Přes toto síťové spojení jsou otevřeny soubory"
"Diese Netzwerkverbindung hat geöffnete Dateien"
"A hálózaton nyitott fájlok vannak"
"To połączenie sieciowe posiada otwarte pliki"
"Esta conexión de red tiene archivos abiertos"
"На мережному диску є відкриті файли"

ErrorAlreadyAssigned
"Имя локального устройства уже использовано"
"The local device name is already in use"
"Název lokálního zařízení je již používán"
"Der lokale Gerätename wird bereits verwendet"
"A helyi eszköznév már foglalt"
"Nazwa urządzenia lokalnego jest już używana"
"El nombre del dispositivo local ya está en uso"
"Ім'я локального пристрою вже використано"

ErrorAlreadyRemebered
"Имя локального устройства уже находится в профиле пользователя"
"The local device is already in the user profile"
"Lokální zařízení je již v uživatelově profilu"
"Der lokale Datenträger ist bereits Teil des Benutzerprofils"
"A helyi eszköz már a felhasználói profilban van"
"Lokalne urządzenie znajduje się już w profilu użytkownika"
"El dispositivo local ya está en el perfil de usuario"
"Ім'я локального пристрою вже знаходиться у профілі користувача"

ErrorNotLoggedOn
"Пользователь не зарегистрирован в сети"
"User has not logged on to the network"
"Uživatel nebyl do sítě přihlášen"
"Benutzer hat sich nicht am Netzwerk angemeldet"
"A felhasználó nincs a hálózaton"
"Użytkownik nie jest zalogowany do sieci"
"Usuario no está conectado a la red"
"Користувач не зареєстрований у мережі"

ErrorInvalidPassword
"Неверный пароль пользователя"
"The user password is invalid"
"Uživatelovo heslo není správné"
"Das Benutzerpasswort ist ungültig"
"Érvénytelen felhasználói jelszó"
"Hasło użytkownika jest niewłaściwe"
"La clave de usuario es inválida"
"Невірний пароль користувача"

ErrorNoRecoveryPolicy
"Для этой системы отсутствует политика надёжного восстановления шифрования"
"There is no valid encryption recovery policy configured for this system"
"V tomto systému není nastaveno žádné platné pravidlo pro dešifrování"
"Auf diesem System ist keine gültige Richtlinie zum Wiederherstellen der Verschlüsselung konfiguriert."
"Nincs érvényes titkosítást feloldó szabály a házirendben"
"Polityka odzyskiwania szyfrowania nie jest skonfigurowana"
"No hay política de recuperación de encriptación válida en este sistema"
"Для цієї системи відсутня політика надійного відновлення шифрування"

ErrorEncryptionFailed
"Ошибка при попытке шифрования файла"
"The specified file could not be encrypted"
"Zadaný soubor nemohl být zašifrován"
"Die angegebene Datei konnte nicht verschlüsselt werden"
"A megadott fájl nem titkosítható"
"Nie udało się zaszyfrować pliku"
"El archivo especificado no puede ser encriptado"
"Помилка при спробі шифрування файлу"

ErrorDecryptionFailed
"Ошибка при попытке расшифровки файла"
"The specified file could not be decrypted"
"Zadaný soubor nemohl být dešifrován"
"Die angegebene Datei konnte nicht entschlüsselt werden"
"A megadott fájl titkosítása nem oldható fel"
"Nie udało się odszyfrować pliku"
"El archivo especificado no puede ser desencriptado"
"Помилка при спробі розшифровувати файл"

ErrorFileNotEncrypted
"Указанный файл не зашифрован"
"The specified file is not encrypted"
"Zadaný soubor není zašifrován"
"Die angegebene Datei ist nicht verschlüsselt"
"A megadott fájl nem titkosított"
"Plik nie jest zaszyfrowany"
"El archivo especificado no está encriptado"
"Вказаний файл не зашифрований"

ErrorNoAssociation
"Указанному файлу не сопоставлено ни одно приложение для выполнения данной операции"
"No application is associated with the specified file for this operation"
"K zadanému souboru není asociována žádná aplikace pro tuto operaci"
"Diesem Dateityp und dieser Aktion ist kein Programm zugewiesen."
"A fájlhoz nincs társítva program"
"Z tą operacją dla pliku nie jest skojarzona żadna aplikacja"
"No hay aplicación asociada para esta operación con el archivo especificado"
"Вказаному файлу не зіставлено жодну програму для виконання цієї операції"

CannotExecute
l:
"Ошибка выполнения"
"Cannot execute"
"Nelze provést"
"Fehler beim Ausführen von"
"Nem végrehajtható:"
"Nie mogę wykonać"
"No se puede ejecutar"
"Помилка виконання"

ScanningFolder
"Просмотр папки"
"Scanning the folder"
"Prohledávám adresář"
"Scanne den Ordner"
"Mappák olvasása..."
"Przeszukuję katalog"
"Explorando el directorio"
"Перегляд теки"

MakeFolderTitle
l:
"Создание папки"
"Make folder"
"Vytvoření adresáře"
"Ordner erstellen"
"Új mappa létrehozása"
"Utwórz katalog"
"Crear directorio"
"Створення теки"

CreateFolder
"Создать п&апку"
"Create the &folder"
"Vytvořit &adresář"
"Diesen &Ordner erstellen:"
"Mappa &neve:"
"Nazwa katalogu"
"Nombre del directorio"
"Створити т&еку"

MultiMakeDir
"Обрабатыват&ь несколько имён папок"
"Process &multiple names"
"Zpracovat &více názvů"
"&Mehrere Namen verarbeiten (getrennt durch Semikolon)"
"Töb&b név feldolgozása"
"Przetwarzaj &wiele nazw"
"Procesar &múltiples nombres"
"Оброблят&и кілька імен тек"

IncorrectDirList
"Неправильный список папок"
"Incorrect folders list"
"Neplatný seznam adresářů"
"Fehlerhafte Ordnerliste"
"Hibás mappalista"
"Błędna lista folderów"
"Listado de directorios incorrecto"
"Неправильний список тек"

CannotCreateFolder
"Ошибка создания папки"
"Cannot create the folder"
"Adresář nelze vytvořit"
"Konnte den Ordner nicht erstellen"
"A mappa nem hozható létre"
"Nie mogę utworzyć katalogu"
"No se puede crear el directorio"
"Помилка створення теки"

MenuBriefView
l:
"&Краткий                  LCtrl-1"
"&Brief              LCtrl-1"
"&Stručný                  LCtrl-1"
"&Kurz                 LStrg-1"
"&Rövid              BalCtrl-1"
"&Skrótowy             LCtrl-1"
"&Breve                 LCtrl-1"
"&Короткий                 LCtrl-1"

MenuMediumView
"&Средний                  LCtrl-2"
"&Medium             LCtrl-2"
"S&třední                  LCtrl-2"
"&Mittel               LStrg-2"
"&Közepes            BalCtrl-2"
"Ś&redni               LCtrl-2"
"&Medio                 LCtrl-2"
"&Середній                 LCtrl-2"

MenuFullView
"&Полный                   LCtrl-3"
"&Full               LCtrl-3"
"&Plný                     LCtrl-3"
"&Voll                 LStrg-3"
"&Teljes             BalCtrl-3"
"&Pełny                LCtrl-3"
"&Completo              LCtrl-3"
"&Повний                   LCtrl-3"

MenuWideView
"&Широкий                  LCtrl-4"
"&Wide               LCtrl-4"
"Š&iroký                   LCtrl-4"
"B&reitformat          LStrg-4"
"&Széles             BalCtrl-4"
"S&zeroki              LCtrl-4"
"&Amplio                LCtrl-4"
"&Широкий                  LCtrl-4"

MenuDetailedView
"&Детальный                LCtrl-5"
"Detai&led           LCtrl-5"
"Detai&lní                 LCtrl-5"
"Detai&lliert          LStrg-5"
"Rész&letes          BalCtrl-5"
"Ze sz&czegółami       LCtrl-5"
"De&tallado             LCtrl-5"
"&Детальний                LCtrl-5"

MenuDizView
"&Описания                 LCtrl-6"
"&Descriptions       LCtrl-6"
"P&opisky                  LCtrl-6"
"&Beschreibungen       LStrg-6"
"Fájl&megjegyzések   BalCtrl-6"
"&Opisy                LCtrl-6"
"&Descripción           LCtrl-6"
"&Описи                    LCtrl-6"

MenuLongDizView
"Д&линные описания         LCtrl-7"
"Lon&g descriptions  LCtrl-7"
"&Dlouhé popisky           LCtrl-7"
"Lan&ge Beschreibungen LStrg-7"
"&Hosszú megjegyzés  BalCtrl-7"
"&Długie opisy         LCtrl-7"
"Descripción lar&ga     LCtrl-7"
"Д&овгі описи              LCtrl-7"

MenuOwnersView
"Вл&адельцы файлов         LCtrl-8"
"File own&ers        LCtrl-8"
"Vlastník so&uboru         LCtrl-8"
"B&esitzer             LStrg-8"
"Fájl tula&jdonos    BalCtrl-8"
"&Właściciele          LCtrl-8"
"Du&eños de archivos    LCtrl-8"
"Власники файлів           LCtrl-8"

MenuLinksView
"Свя&зи файлов             LCtrl-9"
"File lin&ks         LCtrl-9"
"Souborové lin&ky          LCtrl-9"
"Dateilin&ks           LStrg-9"
"Fájl li&nkek        BalCtrl-9"
"Dowiąza&nia           LCtrl-9"
"En&laces               LCtrl-9"
"Зв'язки файлів            LCtrl-9"

MenuAlternativeView
"Аль&тернативный полный    LCtrl-0"
"&Alternative full   LCtrl-0"
"&Alternativní plný        LCtrl-0"
"&Alternativ voll      LStrg-0"
"&Alternatív teljes  BalCtrl-0"
"&Alternatywny         LCtrl-0"
"Alternativo com&pleto  LCtrl-0"
"Аль&тернативний повний    LCtrl-0"

MenuInfoPanel
l:
"Панель ин&формации        Ctrl-L"
"&Info panel         Ctrl-L"
"Panel In&fo               Ctrl-L"
"&Infopanel            Strg-L"
"&Info panel         Ctrl-L"
"Panel informacy&jny   Ctrl-L"
"Panel &información     Ctrl-L"
"Панель ін&формациї        Ctrl-L"

MenuTreePanel
"Де&рево папок             Ctrl-T"
"&Tree panel         Ctrl-T"
"Panel St&rom              Ctrl-T"
"Baumansich&t          Strg-T"
"&Fastruktúra        Ctrl-T"
"Drz&ewo               Ctrl-T"
"Panel árbol           Ctrl-T"
"Де&рево тек               Ctrl-T"

MenuQuickView
"Быстры&й просмотр         Ctrl-Q"
"Quick &view         Ctrl-Q"
"Z&běžné zobrazení         Ctrl-Q"
"Sc&hnellansicht       Strg-Q"
"&Gyorsnézet         Ctrl-Q"
"Sz&ybki podgląd       Ctrl-Q"
"&Vista rápida          Ctrl-Q"
"Швидки&й перегляд         Ctrl-Q"

MenuSortModes
"Режим&ы сортировки        Ctrl-F12"
"&Sort modes         Ctrl-F12"
"Módy řaze&ní              Ctrl-F12"
"&Sortiermodi          Strg-F12"
"R&endezési elv      Ctrl-F12"
"Try&by sortowania     Ctrl-F12"
"&Ordenar por...        Ctrl-F12"
"Режим&и сортування        Ctrl-F12"

MenuLongNames
"Показывать длинные &имена Ctrl-N"
"Show long &names    Ctrl-N"
"Zobrazit dlouhé názv&y    Ctrl-N"
"Lange Datei&namen     Strg-N"
"H&osszú fájlnevek   Ctrl-N"
"Po&każ długie nazwy   Ctrl-N"
"Ver &nombres largos    Ctrl-N"
"Показувати довгі &імена   Ctrl-N"

MenuTogglePanel
"Панель &Вкл/Выкл          Ctrl-F1"
"Panel &On/Off       Ctrl-F1"
"Panel &Zap/Vyp            Ctrl-F1"
"&Panel ein/aus        Strg-F1"
"&Panel be/ki        Ctrl-F1"
"Włącz/Wyłącz pane&l   Ctrl-F1"
"Panel &Si/No           Ctrl-F1"
"Панель &Ввмк/Вимк         Ctrl-F1"

MenuReread
"П&еречитать               Ctrl-R"
"&Re-read            Ctrl-R"
"Obno&vit                  Ctrl-R"
"Aktualisie&ren        Strg-R"
"Friss&ítés          Ctrl-R"
"Odśw&ież              Ctrl-R"
"&Releer                Ctrl-R"
"Перечитати                Ctrl-R"

MenuChangeDrive
"С&менить диск             Alt-F1"
"&Change drive       Alt-F1"
"Z&měnit jednotku          Alt-F1"
"Laufwerk we&chseln    Alt-F1"
"Meghajtó&váltás     Alt-F1"
"Z&mień napęd          Alt-F1"
"Cambiar &unidad        Alt-F1"
"З&мінити диск             Alt-F1"

MenuView
l:
"&Просмотр              F3"
"&View               F3"
"&Zobrazit                   F3"
"&Betrachten           F3"
"&Megnéz               F3"
"&Podgląd                   F3"
"&Ver                   F3"
"&Перегляд              F3"

MenuEdit
"&Редактирование        F4"
"&Edit               F4"
"&Editovat                   F4"
"B&earbeiten           F4"
"&Szerkeszt            F4"
"&Edytuj                    F4"
"&Editar                F4"
"&Редагування           F4"

MenuCopy
"&Копирование           F5"
"&Copy               F5"
"&Kopírovat                  F5"
"&Kopieren             F5"
"Más&ol                F5"
"&Kopiuj                    F5"
"&Copiar                F5"
"&Копіювання            F5"

MenuMove
"П&еренос               F6"
"&Rename or move     F6"
"&Přejmenovat/Přesunout      F6"
"Ve&rschieben/Umben.   F6"
"Át&nevez-Mozgat       F6"
"&Zmień nazwę lub przenieś  F6"
"&Renombrar o mover     F6"
"П&еренесення           F6"

MenuCreateFolder
"&Создание папки        F7"
"&Make folder        F7"
"&Vytvořit adresář           F7"
"&Ordner erstellen     F7"
"Ú&j mappa             F7"
"U&twórz katalog            F7"
"Crear &directorio      F7"
"&Створення теки        F7"

MenuDelete
"&Удаление              F8"
"&Delete             F8"
"&Smazat                     F8"
"&Löschen              F8"
"&Töröl                F8"
"&Usuń                      F8"
"&Borrar                F8"
"&Видалення             F8"

MenuWipe
"Уни&чтожение           Alt-Del"
"&Wipe               Alt-Del"
"&Vymazat                    Alt-Del"
"&Sicher löschen       Alt-Entf"
"&Kisöpör              Alt-Del"
"&Wymaż                     Alt-Del"
"&Eliminar              Alt-Del"
"Зни&щення              Alt-Del"

MenuAdd
"&Архивировать          Shift-F1"
"Add &to archive     Shift-F1"
"Přidat do &archívu          Shift-F1"
"Zu Archiv &hinzuf.    Umsch-F1"
"Tömörhöz ho&zzáad     Shift-F1"
"&Dodaj do archiwum         Shift-F1"
"Agregar a arc&hivo     Shift-F1"
"&Архівувати            Shift-F1"

MenuExtract
"Распако&вать           Shift-F2"
"E&xtract files      Shift-F2"
"&Rozbalit soubory           Shift-F2"
"Archiv e&xtrahieren   Umsch-F2"
"Tömörből ki&bont      Shift-F2"
"&Rozpakuj archiwum         Shift-F2"
"E&xtraer archivos      Shift-F2"
"Розпаку&вати           Shift-F2"

MenuArchiveCommands
"Архивн&ые команды      Shift-F3"
"Arc&hive commands   Shift-F3"
"Příkazy arc&hívu            Shift-F3"
"Arc&hivbefehle        Umsch-F3"
"Tömörítő &parancsok   Shift-F3"
"Po&lecenie archiwizera     Shift-F3"
"Co&mandos archivo      Shift-F3"
"Архівн&і команди       Shift-F3"

MenuAttributes
"А&трибуты файлов       Ctrl-A"
"File &attributes    Ctrl-A"
"A&tributy souboru           Ctrl-A"
"Datei&attribute       Strg-A"
"Fájl &attribútumok    Ctrl-A"
"&Atrybuty pliku            Ctrl-A"
"Cambiar &atributos     Ctrl-A"
"А&трибути файлів       Ctrl-A"

MenuApplyCommand
"Применить коман&ду     Ctrl-G"
"A&pply command      Ctrl-G"
"Ap&likovat příkaz           Ctrl-G"
"Befehl an&wenden      Strg-G"
"Parancs &végrehajtása Ctrl-G"
"Zastosuj pole&cenie        Ctrl-G"
"A&plicar comando       Ctrl-G"
"Примінити коман&ду     Ctrl-G"

MenuDescribe
"&Описание файлов       Ctrl-Z"
"Descri&be files     Ctrl-Z"
"Přidat popisek sou&borům    Ctrl-Z"
"Beschrei&bung ändern  Strg-Z"
"Fájlmegje&gyzés       Ctrl-Z"
"&Opisz pliki               Ctrl-Z"
"Describir &archivo     Ctrl-Z"
"&Опис файлів           Ctrl-Z"

MenuSelectGroup
"Пометить &группу       Gray +"
"Select &group       Gray +"
"Oz&načit skupinu            Num +"
"&Gruppe auswählen     Num +"
"Csoport k&ijelölése   Szürke +"
"Zaznacz &grupę             Szary +"
"Seleccionar &grupo     Gray +"
"Позначити &групу       Gray +"

MenuUnselectGroup
"С&нять пометку         Gray -"
"U&nselect group     Gray -"
"O&dznačit skupinu           Num -"
"G&ruppe abwählen      Num -"
"Jelölést l&evesz      Szürke -"
"Odz&nacz grupę             Szary -"
"Deseleccio&nar grupo   Gray -"
"З&няти позначку         Gray -"

MenuInvertSelection
"&Инверсия пометки      Gray *"
"&Invert selection   Gray *"
"&Invertovat výběr           Num *"
"Auswah&l umkehren     Num *"
"Jelölést meg&fordít   Szürke *"
"Od&wróć zaznaczenie        Szary *"
"&Invertir selección    Gray *"
"&Інверсія позначки     Gray *"

MenuRestoreSelection
"Восстановить по&метку  Ctrl-M"
"Re&store selection  Ctrl-M"
"&Obnovit výběr              Ctrl-M"
"Auswahl wiederher&st. Strg-M"
"Jel&ölést visszatesz  Ctrl-M"
"Odtwórz zaznaczen&ie       Ctrl-M"
"Re&staurar selec.      Ctrl-M"
"Відновити по&значку    Ctrl-M"

MenuFindFile
l:
"&Поиск файла              Alt-F7"
"&Find file           Alt-F7"
"H&ledat soubor                  Alt-F7"
"Dateien &finden       Alt-F7"
"Fájl&keresés         Alt-F7"
"&Znajdź plik               Alt-F7"
"Buscar &archivos       Alt-F7"
"&Пошук файла              Alt-F7"

MenuHistory
"&История команд           Alt-F8"
"&History             Alt-F8"
"&Historie                       Alt-F8"
"&Historie             Alt-F8"
"Parancs &előzmények  Alt-F8"
"&Historia                  Alt-F8"
"&Historial             Alt-F8"
"&Історія команд           Alt-F8"

MenuVideoMode
"Видео&режим               Alt-F9"
"&Video mode          Alt-F9"
"&Video mód                      Alt-F9"
"Ansicht<->&Vollbild   Alt-F9"
"&Video mód           Alt-F9"
"&Tryb wyświetlania         Alt-F9"
"Modo de video         Alt-F9"
"Відео&режим               Alt-F9"

MenuFindFolder
"Поис&к папки              Alt-F10"
"Fi&nd folder         Alt-F10"
"Hl&edat adresář                 Alt-F10"
"Ordner fi&nden        Alt-F10"
"&Mappakeresés        Alt-F10"
"Znajdź kata&log            Alt-F10"
"Buscar &directorios    Alt-F10"
"Пошу&к теки               Alt-F10"

MenuViewHistory
"Ис&тория просмотра        Alt-F11"
"File vie&w history   Alt-F11"
"Historie &zobrazení souborů     Alt-F11"
"Be&trachterhistorie   Alt-F11"
"Fáj&l előzmények     Alt-F11"
"Historia &podglądu plików  Alt-F11"
"Historial &visor       Alt-F11"
"Іс&торія перегляду        Alt-F11"

MenuFoldersHistory
"Ист&ория папок            Alt-F12"
"F&olders history     Alt-F12"
"Historie &adresářů              Alt-F12"
"&Ordnerhistorie       Alt-F12"
"Ma&ppa előzmények    Alt-F12"
"Historia &katalogów        Alt-F12"
"Histo&rial dir.        Alt-F12"
"Іст&орія тек              Alt-F12"

MenuSwapPanels
"По&менять панели          Ctrl-U"
"&Swap panels         Ctrl-U"
"Prohodit panel&y                Ctrl-U"
"Panels tau&schen      Strg-U"
"Panel&csere          Ctrl-U"
"Z&amień panele             Ctrl-U"
"I&nvertir paneles      Ctrl-U"
"Зм&інити панелі            Ctrl-U"

MenuTogglePanels
"Панели &Вкл/Выкл          Ctrl-O"
"&Panels On/Off       Ctrl-O"
"&Panely Zap/Vyp                 Ctrl-O"
"&Panels ein/aus       Strg-O"
"Panelek &be/ki       Ctrl-O"
"&Włącz/Wyłącz panele       Ctrl-O"
"&Paneles Si/No         Ctrl-O"
"Панели &Ввмк/Вимк          Ctrl-O"

MenuCompareFolders
"&Сравнение папок"
"&Compare folders"
"Po&rovnat adresáře"
"Ordner verglei&chen"
"Mappák össze&hasonlítása"
"Porówna&j katalogi"
"&Compara directorios"
"&Порівняння тек"

MenuUserMenu
"Меню пользовател&я"
"Edit user &menu"
"Upravit uživatelské &menu"
"Benutzer&menu editieren"
"Felhasználói m&enü szerk."
"Edytuj &menu użytkownika"
"Editar &menú usuario"
"Меню користувач&а"

MenuFileAssociations
"&Ассоциации файлов"
"File &associations"
"Asocia&ce souborů"
"Dat&eiverknüpfungen"
"Fájl&társítások"
"Prz&ypisania plików"
"&Asociar archivos"
"&Ассоциації файлів"

MenuBookmarks
"Зак&ладки на папки"
"Fol&der bookmarks"
"A&dresářové zkratky"
"Or&dnerschnellzugriff"
"Mappa gyorsbillent&yűk"
"&Skróty katalogów"
"Acc&eso a directorio"
"Зак&ладки на теки"

MenuFilter
"&Фильтр панели файлов     Ctrl-I"
"File panel f&ilter   Ctrl-I"
"F&iltr panelu souborů           Ctrl-I"
"Panelf&ilter          Strg-I"
"Fájlpanel &szűrők    Ctrl-I"
"&Filtr panelu plików       Ctrl-I"
"F&iltro de paneles     Ctrl-I"
"&Фільтр панелі файлів      Ctrl-I"

MenuPluginCommands
"Команды внешних мо&дулей  F11"
"Pl&ugin commands     F11"
"Příkazy plu&ginů                F11"
"Pl&uginbefehle        F11"
"Pl&ugin parancsok    F11"
"Pl&uginy                   F11"
"Comandos de pl&ugin    F11"
"Команди зовнішніх мо&дулів F11"

MenuWindowsList
"Список экра&нов           F12"
"Sc&reens list        F12"
"Seznam obrazove&k               F12"
"Seite&nliste          F12"
"Képer&nyők           F12"
"L&ista ekranów             F12"
"&Listado ventanas      F12"
"Список екра&нів           F12"

MenuProcessList
"Список &задач             Ctrl-W"
"Task &list           Ctrl-W"
"Seznam úl&oh                    Ctrl-W"
"Task&liste            Strg-W"
"Futó p&rogramok      Ctrl-W"
"Lista za&dań               Ctrl-W"
"Lista de &tareas       Ctrl-W"
"Список & завдань          Ctrl-W"

MenuHotPlugList
"Список Hotplug-&устройств"
"Ho&tplug devices list"
"Seznam v&yjímatelných zařízení"
"Sicheres En&tfernen"
"H&otplug eszközök"
"Lista urządzeń Ho&tplug"
"Lista de dispositivos ho&tplug"
"Список Hotplug-&пристроїв"

MenuSystemSettings
l:
"Систе&мные параметры"
"S&ystem settings"
"Nastavení S&ystému"
"&Grundeinstellungen"
"&Rendszer beállítások"
"Ustawienia &systemowe"
"&Sistema      "
"Систе&мні параметри"

MenuPanelSettings
"Настройки па&нели"
"&Panel settings"
"Nastavení &Panelů"
"&Panels einrichten"
"&Panel beállítások"
"Ustawienia &panelu"
"&Paneles      "
"Налаштування па&нелі"

MenuInterface
"Настройки &интерфейса"
"&Interface settings"
"Nastavení Ro&zhraní"
"Oberfläche einr&ichten"
"Kezelő&felület beállítások"
"Ustawienia &interfejsu"
"&Interfaz     "
"Налаштування &інтерфейса"

MenuLanguages
"&Языки"
"Lan&guages"
"Nastavení &Jazyka"
"Sprac&hen"
"N&yelvek (Languages)"
"&Język"
"&Idiomas"
"&Мови"

MenuInput
"Параметры &ввода"
"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Параметры &ввода"

MenuPluginsConfig
"Параметры &внешних модулей"
"Pl&ugins configuration"
"Nastavení Plu&ginů"
"Konfiguration von Pl&ugins"
"Pl&ugin beállítások"
"Konfiguracja p&luginów"
"Configuración de pl&ugins"
"Параметри &зовнішніх модулів"

MenuPluginsManagerSettings
"Параметры менеджера внешних модулей"
"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
"Параметри менеджера зовнішніх модулів"

MenuDialogSettings
"Настройки &диалогов"
"Di&alog settings"
"Nastavení Dialo&gů"
"Di&aloge einrichten"
"Pár&beszédablak beállítások"
"Ustawienia okna &dialogowego"
"Opciones de di&álogo"
"Налаштування &діалогів"

MenuVMenuSettings
"Настройки меню"
"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
"Налаштування меню"

MenuCmdlineSettings
"Настройки &командной строки"
"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
"Opciones de línea de comando"
"Налаштування &командного рядка"

MenuAutoCompleteSettings
"На&стройки автозавершения"
"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
"Opciones de autocompletar"
"На&лаштування автозавершення"

MenuInfoPanelSettings
"Нас&тройки информационной панели"
"Inf&oPanel settings"
upd:"Inf&oPanel settings"
upd:"Inf&oPanel settings"
upd:"Inf&oPanel settings"
upd:"Inf&oPanel settings"
"Opciones de panel de inf&ormación"
"Нал&аштування інформаційної панелі"

MenuConfirmation
"&Подтверждения"
"Co&nfirmations"
"P&otvrzení"
"Bestätigu&ngen"
"Meg&erősítések"
"P&otwierdzenia"
"Co&nfirmaciones"
"&Підтвердження"

MenuFilePanelModes
"Режим&ы панели файлов"
"File panel &modes"
"&Módy souborových panelů"
"Anzeige&modi von Dateipanels"
"Fájlpanel mód&ok"
"&Tryby wyświetlania panelu plików"
"&Modo de paneles de archivos"
"Режим&и панелі файлів"

MenuFileDescriptions
"&Описания файлов"
"File &descriptions"
"Popi&sy souborů"
"&Dateibeschreibungen"
"Fájl &megjegyzésfájlok"
"Opis&y plików"
"&Descripción de archivos"
"&Описи файлів"

MenuFolderInfoFiles
"Файлы описания п&апок"
"&Folder description files"
"Soubory popisů &adresářů"
"O&rdnerbeschreibungen"
"M&appa megjegyzésfájlok"
"Pliki opisu &katalogu"
"&Archivo de descripción de directorios"
"Файли описів т&ек"

MenuViewer
"Настройки про&граммы просмотра"
"&Viewer settings"
"Nastavení P&rohlížeče"
"Be&trachter einrichten"
"&Nézőke beállítások"
"Ustawienia pod&glądu"
"&Visor "
"Налаштування про&грами перегляду"

MenuEditor
"Настройки &редактора"
"&Editor settings"
"Nastavení &Editoru"
"&Editor einrichten"
"&Szerkesztő beállítások"
"Ustawienia &edytora"
"&Editor "
"Налаштування &редактора"

MenuNotifications
"Настройки &уведомлений"
"No&tifications settings"
upd:"No&tifications settings"
upd:"No&tifications settings"
upd:"No&tifications settings"
upd:"No&tifications settings"
upd:"No&tifications settings"
"Налаштування &повідомлень"

MenuCodePages
"Кодов&ые страницы"
upd:"&Code pages"
upd:"Znakové sady:"
upd:"Tabellen"
upd:"Kódlapok"
upd:"Strony kodowe"
"Tablas (code pages)"
"Кодов&і сторінки"

MenuColors
"&Цвета"
"Co&lors"
"&Barvy"
"&Farben"
"S&zínek"
"Kolo&ry"
"&Colores"
"&Кольори"

MenuFilesHighlighting
"Раскраска &файлов и группы сортировки"
"Files &highlighting and sort groups"
"Z&výrazňování souborů a skupiny řazení"
"Farbmar&kierungen und Sortiergruppen"
"Fájlkiemelések, rendezési &csoportok"
"&Wyróżnianie plików"
"&Resaltar archivos y ordenar grupos"
"Розмальовка &файлів та групи сортування"

MenuSaveSetup
"&Сохранить параметры                  Shift-F9"
"&Save setup                         Shift-F9"
"&Uložit nastavení                      Shift-F9"
"Setup &speichern                     Umsch-F9"
"Beállítások men&tése                 Shift-F9"
"&Zapisz ustawienia       Shift-F9"
"&Guardar configuración     Shift-F9"
"&Зберегти параметри                   Shift-F9"

MenuTogglePanelRight
"Панель &Вкл/Выкл          Ctrl-F2"
"Panel &On/Off       Ctrl-F2"
"Panel &Zap/Vyp            Ctrl-F2"
"Panel &ein/aus        Strg-F2"
"Panel be/&ki        Ctrl-F2"
"Włącz/wyłącz pane&l   Ctrl-F2"
"Panel &Si/No           Ctrl-F2"
"Панель &Ввмк/Вимк         Ctrl-F2"

MenuChangeDriveRight
"С&менить диск             Alt-F2"
"&Change drive       Alt-F2"
"Z&měnit jednotku          Alt-F2"
"Laufwerk &wechseln    Alt-F2"
"Meghajtó&váltás     Alt-F2"
"Z&mień napęd          Alt-F2"
"Cambiar &unidad        Alt-F2"
"З&мінити диск             Alt-F2"

MenuLeftTitle
l:
"&Левая"
"&Left"
"&Levý"
"&Links"
"&Bal"
"&Lewy"
"&Izquierdo"
"&Ліва"

MenuFilesTitle
"&Файлы"
"&Files"
"&Soubory"
"&Dateien"
"&Fájlok"
"Pl&iki"
"&Archivo"
"&Файли"

MenuCommandsTitle
"&Команды"
"&Commands"
"Pří&kazy"
"&Befehle"
"&Parancsok"
"Pol&ecenia"
"&Comandos"
"&Команди"

MenuOptionsTitle
"Па&раметры"
"&Options"
"&Nastavení"
"&Optionen"
"B&eállítások"
"&Opcje"
"&Opciones"
"Па&раметри"

MenuRightTitle
"&Правая"
"&Right"
"&Pravý"
"&Rechts"
"&Jobb"
"&Prawy"
"&Derecho"
"&Права"

MenuSortTitle
l:
"Критерий сортировки"
"Sort by"
"Seřadit podle"
"Sortieren nach"
"Rendezési elv"
"Sortuj według..."
"Ordenar por"
"Критерій сортування"

MenuSortByName
"&Имя                              Ctrl-F3"
"&Name                   Ctrl-F3"
"&Názvu                     Ctrl-F3"
"&Name                   Strg-F3"
"&Név                  Ctrl-F3"
"&nazwy                       Ctrl-F3"
"&Nombre               Ctrl-F3"
"&ІИм'я                            Ctrl-F3"

MenuSortByExt
"&Расширение                       Ctrl-F4"
"E&xtension              Ctrl-F4"
"&Přípony                   Ctrl-F4"
"&Erweiterung            Strg-F4"
"Ki&terjesztés         Ctrl-F4"
"ro&zszerzenia                Ctrl-F4"
"E&xtensión            Ctrl-F4"
"&Розширення                       Ctrl-F4"

MenuSortByWrite
"Время &записи                     Ctrl-F5"
"&Write time             Ctrl-F5"
upd:"&Write time             Ctrl-F5"
upd:"&Write time             Ctrl-F5"
upd:"&Write time             Ctrl-F5"
upd:"&Write time             Ctrl-F5"
"Fecha &modificación   Ctrl-F5"
"Час &запису                       Ctrl-F5"

MenuSortBySize
"Р&азмер                           Ctrl-F6"
"&Size                   Ctrl-F6"
"&Velikosti                 Ctrl-F6"
"&Größe                  Strg-F6"
"&Méret                Ctrl-F6"
"&rozmiaru                    Ctrl-F6"
"&Tamaño               Ctrl-F6"
"Р&озмір                           Ctrl-F6"

MenuUnsorted
"&Не сортировать                   Ctrl-F7"
"&Unsorted               Ctrl-F7"
"N&eřadit                   Ctrl-F7"
"&Unsortiert             Strg-F7"
"&Rendezetlen          Ctrl-F7"
"&bez sortowania              Ctrl-F7"
"&Sin ordenar          Ctrl-F7"
"&Не сортувати                     Ctrl-F7"

MenuSortByCreation
"Время &создания                   Ctrl-F8"
"&Creation time          Ctrl-F8"
"&Data vytvoření            Ctrl-F8"
"E&rstelldatum           Strg-F8"
"Ke&letkezés ideje     Ctrl-F8"
"czasu u&tworzenia            Ctrl-F8"
"Fecha de &creación    Ctrl-F8"
"Час &створення                    Ctrl-F8"

MenuSortByAccess
"Время &доступа                    Ctrl-F9"
"&Access time            Ctrl-F9"
"Ča&su přístupu             Ctrl-F9"
"&Zugriffsdatum          Strg-F9"
"&Hozzáférés ideje     Ctrl-F9"
"czasu &użycia                Ctrl-F9"
"Fecha de &acceso      Ctrl-F9"
"Час &доступу                      Ctrl-F9"

MenuSortByChange
"Время из&менения"
"Chan&ge time"
upd:"Change time"
upd:"Change time"
upd:"Change time"
upd:"Change time"
upd:"Change time"
"Час з&міни"

MenuSortByDiz
"&Описания                         Ctrl-F10"
"&Descriptions           Ctrl-F10"
"P&opisků                   Ctrl-F10"
"&Beschreibungen         Strg-F10"
"Megjegyzé&sek         Ctrl-F10"
"&opisu                       Ctrl-F10"
"&Descripciones        Ctrl-F10"
"&Опис                             Ctrl-F10"

MenuSortByOwner
"&Владельцы файлов                 Ctrl-F11"
"&Owner                  Ctrl-F11"
"V&lastníka                 Ctrl-F11"
"Bes&itzer               Strg-F11"
"Tula&jdonos           Ctrl-F11"
"&właściciela                 Ctrl-F11"
"Dueñ&o                Ctrl-F11"
"&Власники файлів                  Ctrl-F11"

MenuSortByPhysicalSize
"&Физический размер"
"&Physical size"
upd:"&Komprimované velikosti"
upd:"Kom&primierte Größe"
upd:"Tömörített mér&et"
upd:"rozmiaru po &kompresji"
upd:"Tamaño de com&presin"
"&Фізичний розмір"

MenuSortByNumLinks
"Ко&личество ссылок"
"Number of &hard links"
"Poč&tu pevných linků"
"Anzahl an &Links"
"Hardlinkek s&záma"
"&liczby dowiązań"
"Número de enlaces &rígidos"
"Кі&лькість посилань"

MenuSortByFullName
"&Полное имя"
"&Full name"
upd:"&Full name"
upd:"&Full name"
upd:"&Full name"
upd:"&Full name"
"Nombre completo"
"&Повне ім'я"

MenuSortByCustomData
upd:"Cus&tom data"
"Cus&tom data"
upd:"Cus&tom data"
upd:"Cus&tom data"
upd:"Cus&tom data"
upd:"Cus&tom data"
"Datos opcionales"
upd:"Cus&tom data"

MenuSortUseGroups
"Использовать &группы сортировки   Shift-F11"
"Use sort &groups        Shift-F11"
"Řazení podle skup&in       Shift-F11"
"Sortier&gruppen verw.   Umsch-F11"
"Rend. cs&oport haszn. Shift-F11"
"użyj &grup sortowania        Shift-F11"
"Usar orden/&grupo      Shift-F11"
"Використовувати &групи сортування   Shift-F11"

MenuSortSelectedFirst
"Помеченные &файлы вперёд          Shift-F12"
"Show selected f&irst    Shift-F12"
"Nejdřív zobrazit vy&brané  Shift-F12"
"&Ausgewählte zuerst     Umsch-F12"
"Kijel&ölteket előre   Shift-F12"
"zazna&czone najpierw         Shift-F12"
"Mostrar seleccionados primero Shift-F12"
"Позначені &файли вперед Shift-F12"

MenuSortDirectoriesFirst
"&Каталоги вперёд"
"Sho&w directories first"
upd:"Sho&w directories first"
upd:"Sho&w directories first"
upd:"Sho&w directories first"
upd:"Sho&w directories first"
"Mostrar directorios primero"
"&Каталоги вперед"

MenuSortUseNumeric
"&Числовая сортировка"
"Num&eric sort"
"Použít čí&selné řazení"
"Nu&merische Sortierung"
"N&umerikus rendezés"
"Sortuj num&erycznie"
"Usar orden num&érico"
"&Числове сортування"

MenuSortUseCaseSensitive
"Сортировка с учётом регистра"
"Use case sensitive sort"
"Použít řazení citlivé na velikost písmen"
"Sortierung abhängig von Groß-/Kleinschreibung"
"Nagy/kisbetű érzékeny rendezés"
"Sortuj uwzględniając wielkość liter"
"Usar orden sensible a min/mayúsc."
"Сортування з урахуванням регістру"

ChangeDriveTitle
l:
"Перейти"
"Location"
"Jednotka"
"Laufwerke"
"Meghajtók"
"Napęd"
"Unidad"
"Перейти"

ChangeDriveRemovable
"сменный"
"removable"
"vyměnitelná"
"wechsel."
"kivehető"
"wyjmowalny"
"removible"
"змінний"

ChangeDriveFixed
"жёсткий"
"fixed"
"pevná"
"fest"
"fix"
"stały"
"rígido   "
"жорсткий"

ChangeDriveNetwork
"сетевой"
"network"
"síťová"
"Netzwerk"
"hálózati"
"sieciowy"
"red      "
"мережевий"

ChangeDriveDisconnectedNetwork
"отключенный"
"disconnected"
upd:"disconnected"
upd:"disconnected"
"leválasztva"
upd:"disconnected"
"desconectado"
"відключений"

ChangeDriveCDROM
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM   "
"CD-ROM"

ChangeDriveCD_RW
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"

ChangeDriveCD_RWDVD
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"

ChangeDriveDVD_ROM
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"

ChangeDriveDVD_RW
"DVD-RW"
"DVD-RW"
"DWD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"

ChangeDriveDVD_RAM
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"

ChangeDriveBD_ROM
"BD-ROM"
"BD-ROM"
"BD-ROM"
"BD-ROM"
"BD-ROM"
"BD-ROM"
"BD-ROM"
"BD-ROM"

ChangeDriveBD_RW
"BD-RW"
"BD-RW"
"BD-RW"
"BD-RW"
"BD-RW"
"BD-RW"
"BD-RW"
"BD-RW"

ChangeDriveHDDVD_ROM
"HDDVD-ROM"
"HDDVD-ROM"
"HDDVD-ROM"
"HDDVD-ROM"
"HDDVD-ROM"
"HDDVD-ROM"
"HDDVD-ROM"
"HDDVD-ROM"

ChangeDriveHDDVD_RW
"HDDVD-RW"
"HDDVD-RW"
"HDDVD-RW"
"HDDVD-RW"
"HDDVD-RW"
"HDDVD-RW"
"HDDVD-RW"
"HDDVD-RW"


ChangeDriveRAM
"RAM диск"
"RAM disk"
"RAM disk"
"RAM-DISK"
"RAM lemez"
"RAM-dysk"
"Disco RAM"
"RAM диск"

ChangeDriveSUBST
"SUBST"
"subst"
"SUBST"
"Subst"
"virtuális"
"subst"
"subst    "
"SUBST"

ChangeDriveVirtual
"виртуальный"
"virtual"
upd:"virtual"
upd:"virtual"
upd:"virtual"
upd:"virtual"
upd:"virtual"
"віртуальний"

ChangeDriveLabelAbsent
"недоступен"
"not available"
"není k dispozici"
"nicht vorh."
"nem elérhető"
"niedostępny"
"no disponible"
"недоступний"

ChangeDriveCannotReadDisk
"Ошибка чтения диска в дисководе"
"Cannot read the disk in drive"
"Nelze přečíst disk v jednotce"
"Kann nicht gelesen werden datenträge in Laufwerk"
"Meghajtó lemeze nem olvasható"
"Nie mogę odczytać dysku w napędzie"
"No se puede leer el disco en unidad"
"Помилка читання диска в дисководі"

ChangeDriveCannotDisconnect
"Не удаётся отсоединиться от %ls"
"Cannot disconnect from %ls"
"Nelze se odpojit od %ls"
"Verbindung zu %ls konnte nicht getrennt werden."
"Nem lehet leválni innen: %ls"
"Nie mogę odłączyć się od %ls"
"No se puede desconectar desde %ls"
"Неможливо від'єднатися від %ls"

ChangeDriveCannotDelSubst
"Не удаётся удалить виртуальный драйвер %ls"
"Cannot delete a substituted drive %ls"
"Nelze smazat substnutá jednotka %ls"
"Substlaufwerk %ls konnte nicht gelöscht werden."
"%ls virtuális meghajtó nem törölhető"
"Nie można usunąć dysku SUBST %ls"
"No se puede borrar una unidad sustituida %ls"
"Не вдалося видалити віртуальний драйвер %ls"

ChangeDriveOpenFiles
"Если вы не закроете открытые файлы, данные могут быть утеряны"
"If you do not close the open files, data may be lost"
"Pokud neuzavřete otevřené soubory, mohou být tato data ztracena"
"Wenn Sie offene Dateien nicht schließen könnten Daten verloren gehen"
"Ha a nyitott fájlokat nem zárja be, az adatok elveszhetnek"
"Jeśli nie zamkniesz otwartych plików, możesz utracić dane"
"Si no cierra los archivos abiertos, los datos se pueden perder."
"Якщо ви не закриєте відкриті файли, дані можуть бути втрачені"

ChangeSUBSTDisconnectDriveTitle
l:
"Отключение виртуального устройства"
"Virtual device disconnection"
"Odpojování virtuálního zařízení"
"Virtuelles Gerät trennen"
"Virtuális meghajtó törlése"
"Odłączanie napędu wirtualnego"
"Desconexion de dispositivo virtual"
"Вимкнення віртуального пристрою"

ChangeSUBSTDisconnectDriveQuestion
"Отключить SUBST-диск %c:?"
"Disconnect SUBST-disk %c:?"
"Odpojit SUBST-disk %c:?"
"Substlaufwerk %c: trennen?"
"Törli %c: virtuális meghajtót?"
"Odłączyć dysk SUBST %c:?"
"Desconectarse de disco sustituido %c:?"
"Вимкнути SUBST-диск %c:?"

ChangeVHDDisconnectDriveTitle
"Отсоединение виртуального диска"
"Virtual disk detaching"
upd:"Virtual disk detaching"
upd:"Virtual disk detaching"
upd:"Virtual disk detaching"
upd:"Virtual disk detaching"
upd:"Virtual disk detaching"
"Від'єднання віртуального диска"

ChangeVHDDisconnectDriveQuestion
"Отсоединить виртуальный диск %c:?"
"Detach virtual disk %c:?"
upd:"Detach virtual disk %c:?"
upd:"Detach virtual disk %c:?"
upd:"Detach virtual disk %c:?"
upd:"Detach virtual disk %c:?"
upd:"Detach virtual disk %c:?"
"Від'єднати віртуальний диск %c:?"

ChangeHotPlugDisconnectDriveTitle
l:
"Удаление устройства"
"Device Removal"
"Odpojování zařízení"
"Sicheres Entfernen"
"Eszköz biztonságos eltávolítása"
"Odłączanie urządzenia"
"Remover dispositivo"
"Видалення пристрою"

ChangeHotPlugDisconnectDriveQuestion
"Вы хотите удалить устройство"
"Do you want to remove the device"
"Opravdu si přejete odpojit zařízení"
"Wollen Sie folgendes Gerät sicher entfernen? "
"Eltávolítja az eszközt?"
"Czy odłączyć urządzenie"
"Desea remover el dispositivo"
"Ви хочете видалити пристрій"

HotPlugDisks
"(диск(и): %ls)"
"(disk(s): %ls)"
"(disk(y): %ls)"
"(Laufwerk(e): %ls)"
"(%ls meghajtó)"
"(dysk(i): %ls)"
"(disco(s): %ls)"
"(диск(и): %ls)"

ChangeCouldNotEjectHotPlugMedia
"Невозможно удалить устройство для диска %c:"
"Cannot remove a device for drive %c:"
"Zařízení %c: nemůže být odpojeno."
"Ein Gerät für Laufwerk %c: konnte nicht entfernt werden"
"%c: eszköz nem távolítható el"
"Nie udało się odłączyć dysku %c:"
"No se puede remover dispositivo para unidad %c:"
"Не можна видалити пристрій для диска %c:"

ChangeCouldNotEjectHotPlugMedia2
"Невозможно удалить устройство:"
"Cannot remove a device:"
"Zařízení nemůže být odpojeno."
"Kann folgendes Geräte nicht entfernen:"
"Az eszköz nem távolítható el:"
"Nie udało się odłączyć urządzenia:"
"No se puede remover el dispositivo:"
"Не можна видалити пристрій:"

ChangeHotPlugNotify1
"Теперь устройство" 
"The device" 
"Zařízení"
"Das Gerät"
"Az eszköz:"
"Urządzenie"
"El dispositivo"
"Тепер пристрій"

ChangeHotPlugNotify2
"может быть безопасно извлечено из компьютера"
"can now be safely removed"
"může být nyní bezpečně odebráno"
"kann nun vom Computer getrennt werden"
"már biztonságosan eltávolítható"
"można teraz bezpiecznie odłączyć"
"ahora puede ser removido de forma segura"
"може бути безпечно вилучено з комп'ютера"

HotPlugListTitle
"Hotplug-устройства"
"Hotplug devices list"
"Seznam vyjímatelných zařízení"
"Hardware sicher entfernen"
"Hotplug eszközök"
"Lista urządzeń Hotplug"
"Lista de conexión de dispositivos"
"Hotplug-пристрої"

HotPlugListBottom
"Редактирование: Del,Ctrl-R"
"Edit: Del,Ctrl-R"
"Edit: Del,Ctrl-R"
"Tasten: Entf,StrgR,F1"
"Szerkesztés: Del,Ctrl-R"
"Edycja: Del,Ctrl-R"
"Editar: Del,Ctrl-R"
"Редагування: Del,Ctrl-R"

ChangeDriveDisconnectTitle
l:
"Отключение сетевого устройства"
"Disconnect network drive"
"Odpojit síťovou jednotku"
"Netzwerklaufwerk trennen"
"Hálózati meghajtó leválasztása"
"Odłączanie dysku sieciowego"
"Desconectar unidad de red"
"Вимкнення мережного пристрою"

ChangeDriveDisconnectQuestion
"Вы хотите удалить соединение с устройством %c:?"
"Do you want to disconnect from the drive %c:?"
"Opravdu si přejete odpojit od jednotky %c:?"
"Wollen Sie die Verbindung zu Laufwerk %c: trennen?"
"Le akar válni %c: meghajtóról?"
"Czy odłączyć dysk %c:?"
"Quiere desconectarse desde la unidad %c:?"
"Ви хочете видалити з'єднання з пристроєм %c:?"

ChangeDriveDisconnectMapped
"На устройство %c: отображена папка"
"The drive %c: is mapped to..."
"Jednotka %c: je namapována na..."
"Laufwerk %c: ist verknüpft zu..."
"%c: meghajtó hozzárendelve:"
"Dysk %c: jest skojarzony z..."
"La unidad %c: es mapeada hacia..."
"На пристрій %c: відображено папку"

ChangeDriveDisconnectReconnect
"&Восстанавливать при входе в систему"
"&Reconnect at Logon"
"&Znovu připojit při přihlášení"
"Bei Anmeldung &verbinden"
"&Bejelentkezésnél újracsatlakoztat"
"&Podłącz ponownie przy logowaniu"
"&Reconectar al desconectar"
"&Відновити при вході в систему"

ChangeDriveAskDisconnect
l:
"Вы хотите в любом случае отключиться от устройства?"
"Do you want to disconnect the device anyway?"
"Přejete si přesto zařízení odpojit?"
"Wollen Sie die Verbindung trotzdem trennen?"
"Mindenképpen leválasztja az eszközt?"
"Czy chcesz mimo to odłączyć urządzenie?"
"Quiere desconectar el dispositivo de cualquier forma?"
"Ви хочете в будь-якому випадку відключитися від пристрою?"

ChangeVolumeInUse
"Не удаётся извлечь диск из привода %c:"
"Volume %c: cannot be ejected."
"Jednotka %c: nemůže být vysunuta."
"Datenträger %c: kann nicht ausgeworfen werden."
"%c: kötet nem oldható ki"
"Nie można wysunąć dysku %c."
"Volumen %c: no puede ser expulsado."
"Не вдається вийняти диск із приводу %c:"

ChangeVolumeInUse2
"Используется другим приложением"
"It is used by another application"
"Je používaná jinou aplikací"
"Andere Programme greifen momentan darauf zu"
"Másik program használja"
"Jest używany przez inną aplikację"
"Es usada por otra aplicación"
"Використовується іншим додатком"

ChangeWaitingLoadDisk
"Ожидание чтения диска..."
"Waiting for disk to mount..."
"Čekám na disk k připojení..."
"Warte auf Datenträger..."
"Lemez betöltése..."
"Trwa montowanie dysku..."
"Esperando para montar el disco..."
"Чекання читання диска..."

ChangeCouldNotEjectMedia
"Невозможно извлечь диск из привода %c:"
"Could not eject media from drive %c:"
"Nelze vysunout médium v jednotce %c:"
"Konnte Medium in Laufwerk %c: nicht auswerfen"
"%c: meghajtó lemeze nem oldható ki"
"Nie można wysunąć dysku z napędu %c:"
"No se puede expulsar medio de la unidad %c:"
"Неможливо вилучити диск із приводу %c:"

ChangeDriveConfigure
"Настройка меню выбора диска"
"Change Drive Menu Options"
upd:"Change Drive Menu Options"
upd:"Change Drive Menu Options"
upd:"Change Drive Menu Options"
upd:"Change Drive Menu Options"
"Cambiar opciones de menú de unidades"
"Налаштування меню вибору диска"

ChangeDriveShowDiskType
"Показывать &тип диска"
"Show disk &type"
upd:"Show disk type"
upd:"Show disk type"
upd:"Show disk type"
upd:"Show disk type"
"Mostrar &tipo de disco"
"Показувати &тип диска"

ChangeDriveShowNetworkName
"Показывать &сетевое имя/путь SUBST/имя VHD"
"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
"Показувати мережеве ім'я/шлях SUBST/ім'я VHD"

ChangeDriveShowLabel
"Показывать &метку диска"
"Show disk &label"
upd:"Show disk &label"
upd:"Show disk &label"
upd:"Show disk &label"
upd:"Show disk &label"
"Mostrar etiqueta"
"Показувати &мітку диска"

ChangeDriveShowFileSystem
"Показывать тип &файловой системы"
"Show &file system type"
upd:"Show &file system type"
upd:"Show &file system type"
upd:"Show &file system type"
upd:"Show &file system type"
"Mostrar sistema de archivos"
"Показувати тип &файлової системи"

ChangeDriveShowSize
"Показывать &размер"
"Show &size"
upd:"Show &size"
upd:"Show &size"
upd:"Show &size"
upd:"Show &size"
"Mostrar tamaño"
"Показувати &розмір"

ChangeDriveShowSizeFloat
"Показывать ра&змер в стиле Windows Explorer"
"Show size in &Windows Explorer style"
upd:"Show size in &Windows Explorer style"
upd:"Show size in &Windows Explorer style"
upd:"Show size in &Windows Explorer style"
upd:"Show size in &Windows Explorer style"
"Mostrar tamaño estilo &Windows Explorer"
"Показувати ро&змір у стилі Windows Explorer"

ChangeDriveShowRemovableDrive
"Показывать параметры см&енных дисков"
"Show &removable drive parameters"
upd:"Show &removable drive parameters"
upd:"Show &removable drive parameters"
upd:"Show &removable drive parameters"
upd:"Show &removable drive parameters"
"Mostrar parámetros de unidad removible"
"Показувати параметри зм&інних дисків"

ChangeDriveShowPlugins
"Показывать &плагины"
"Show &plugins"
upd:"Show &plugins"
upd:"Show &plugins"
upd:"Show &plugins"
upd:"Show &plugins"
"Mostrar &plugins"
"Показувати &Плагіни"

ChangeDriveShowShortcuts
"Показывать &закладки"
"Show &bookmarks"
upd:"Show &bookmarks"
upd:"Show &bookmarks"
upd:"Show &bookmarks"
upd:"Show &bookmarks"
upd:"Mostrar &bookmarks"
"Показувати &закладки"

ChangeDriveShowCD
"Показывать параметры &компакт-дисков"
"Show &CD drive parameters"
upd:"Show &CD drive parameters"
upd:"Show &CD drive parameters"
upd:"Show &CD drive parameters"
upd:"Show &CD drive parameters"
"Mostrar parámetros unidad de &CD"
"Показувати параметри &компакт-дисків"

ChangeDriveShowNetworkDrive
"Показывать параметры се&тевых дисков"
"Show n&etwork drive parameters"
upd:"Show ne&twork drive parameters"
upd:"Show ne&twork drive parameters"
upd:"Show ne&twork drive parameters"
upd:"Show ne&twork drive parameters"
"Mostrar parámetros unidades de red"
"Показувати параметри ме&режевих дисків"

ChangeDriveMenuFooter
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"

EditControlHistoryFooter
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"

EditControlHistoryFooterNoDel
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"

HistoryFooter
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"

SearchFileTitle
l:
" Поиск "
" Search "
" Hledat "
" Suchen "
" Keresés "
" Szukaj "
" Buscar "
" Пошук "

CannotCreateListFile
"Ошибка создания списка файлов"
"Cannot create list file"
"Nelze vytvořit soubor se seznamem"
"Dateiliste konnte nicht erstellt werden"
"A listafájl nem hozható létre"
"Nie mogę utworzyć listy plików"
"No se puede crear archivo de lista"
"Помилка створення списку файлів"

CannotCreateListTemp
"(невозможно создать временный файл для списка)"
"(cannot create temporary file for list)"
"(nemohu vytvořit dočasný soubor pro seznam)"
"(Fehler beim Anlegen einer temporären Datei für Liste)"
"(a lista átmeneti fájl nem hozható létre)"
"(nie można utworzyć pliku tymczasowego dla listy)"
"(no se puede crear archivo temporal para lista)"
"(неможливо створити тимчасовий файл для списку)"

CannotCreateListWrite
"(невозможно записать данные в файл)"
"(cannot write data in file)"
"(nemohu zapsat data do souboru)"
"(Fehler beim Schreiben der Daten)"
"(a fájlba nem írható adat)"
"(nie można zapisać danych do pliku)"
"(no se puede escribir datos en el archivo)"
"(неможливо записати дані у файл)"

DragFiles
l:
"%d файлов"
"%d files"
"%d souborů"
"%d Dateien"
"%d fájl"
"%d plików"
"%d archivos"
"%d файлів"

DragMove
"Перенос %ls"
"Move %ls"
"Přesunout %ls"
"Verschiebe %ls"
"%ls mozgatása"
"Przenieś %ls"
"Mover %ls"
"Перенесення %ls"

DragCopy
"Копирование %ls"
"Copy %ls"
"Kopírovat %ls"
"Kopiere %ls"
"%ls másolása"
"Kopiuj %ls"
"Copiar %ls"
"Копіювання %ls"

ProcessListTitle
l:
"Список задач"
"Task list"
"Seznam úloh"
"Taskliste"
"Futó programok"
"Lista zadań"
"Lista de tareas"
"Список завдань"

ProcessListBottom
"Редактирование: Del,Ctrl-R"
"Edit: Del,Ctrl-R"
"Edit: Del,Ctrl-R"
"Tasten: Entf,StrgR"
"Szerk.: Del,Ctrl-R"
"Edycja: Del,Ctrl-R"
"Editar: Del,Ctrl-R"
"Редагування: Del,Ctrl-R"

KillProcessTitle
"Удаление задачи"
"Kill task"
"Zabít úlohu"
"Task beenden"
"Programkilövés"
"Zakończ zadanie"
"Terminar tarea"
"Видалення завдання"

AskKillProcess
"Вы хотите удалить выбранную задачу?"
"Do you wish to kill selected task?"
"Přejete si zabít vybranou úlohu?"
"Wollen Sie den ausgewählten Task beenden?"
"Ki akarja lőni a kijelölt programot?"
"Czy chcesz zakończyć wybrane zadanie?"
"Desea terminar la tarea seleccionada?"
"Ви хочете видалити вибране завдання?"

KillProcessWarning
"Вы потеряете всю несохраненную информацию этой программы"
"You will lose any unsaved information in this program"
"V tomto programu budou ztraceny neuložené informace"
"Alle ungespeicherten Daten dieses Programmes gehen verloren"
"A program minden mentetlen adata elvész"
"Utracisz wszystkie niezapisane dane w tym programie"
"Usted perder cualquier información no grabada de este programa"
"Ви втратите всю незбережену інформацію цієї програми"

KillProcessKill
"Удалить"
"Kill"
"Zabít"
"Beenden"
"Kilő"
"Zakończ"
"Terminar"
"Вилучити"

CannotKillProcess
"Указанную задачу удалить не удалось"
"Cannot kill the specified task"
"Nemohu ukončit zvolenou úlohu"
"Task konnte nicht beendet werden"
"A programot nem lehet kilőni"
"Nie mogę zakończyć wybranego zadania"
"No se puede terminar la tarea seleccionada"
"Вказане завдання видалити не вдалося"

CannotKillProcessPerm
"Вы не имеет права удалить этот процесс."
"You have no permission to kill this process."
"Nemáte oprávnění zabít tento proces."
"Sie haben keine Rechte um diesen Prozess zu beenden."
"Nincs joga a program kilövésére"
"Nie masz wystarczających uprawnień do zakończenia procesu."
"Usted no tiene permiso para terminar este proceso."
"Ви не маєте права видалити цей процес."

QuickViewTitle
l:
"Быстрый просмотр"
"Quick view"
"Zběžné zobrazení"
"Schnellansicht"
"Gyorsnézet"
"Szybki podgląd"
"Vista rápida"
"Швидкий перегляд"

QuickViewFolder
"Папка"
"Folder"
"Adresář"
"Verzeichnis"
"Mappa"
"Folder"
"Directorio"
"Тека"

QuickViewJunction
"Связь"
"Junction"
"Křížení"
"Knotenpunkt"
"Csomópont"
"Powiązanie"
"Juntar"
"Зв'язок"

QuickViewSymlink
"Ссылка"
"Symlink"
"Symbolický link"
"Symlink"
"Szimlink"
"Link"
"Enlace"
"Посилання"

QuickViewVolMount
"Том"
"Volume"
"Svazek"
"Datenträger"
"Kötet"
"Napęd"
"Volumen"
"Том"

QuickViewContains
"Содержит:"
"Contains:"
"Obsah:"
"Enthält:"
"Tartalma:"
"Zawiera:"
"Contiene:"
"Утримує:"

QuickViewFolders
"Папок               "
"Folders          "
"Adresáře           "
"Ordner           "
"Mappák száma     "
"Katalogi            "
"Directorios      "
"Тек                 "

QuickViewFiles
"Файлов              "
"Files            "
"Soubory            "
"Dateien          "
"Fájlok száma     "
"Pliki               "
"archivos         "
"Файлів              "

QuickViewBytes
"Размер файлов       "
"Files size       "
"Velikost souborů   "
"Gesamtgröße      "
"Fájlok mérete    "
"Rozmiar plików      "
"Tamaño archivos  "
"Розмір файлів       "

QuickViewPhysical
"Физичеcкий размер  "
"Physical size    "
upd:"Komprim. velikost  "
upd:"Komprimiert      "
upd:"Tömörített méret "
upd:"Po kompresji        "
upd:"Tamaño comprimido"
"Фізичний розмір  "

QuickViewRatio
"Степень сжатия      "
"Ratio            "
"Poměr              "
"Rate             "
"Tömörítés aránya "
"Procent             "
"Promedio"
"Ступінь стиснення   "

QuickViewCluster
"Размер кластера     "
"Cluster size     "
"Velikost clusteru  "
"Clustergröße     "
"Klaszterméret    "
"Rozmiar klastra     "
"Tamaño cluster   "
"Розмір кластера     "

QuickViewRealSize
"Реальный размер     "
"Real files size  "
"Opravdová velikost "
"Tatsächlich      "
"Valódi fájlméret "
"Właściwy rozmiar    "
"Tamaño real      "
"Реальний розмір     "

QuickViewSlack
"Остатки кластеров   "
"Files slack      "
"Mrtvé místo        "
"Verlust          "
"Meddő terület    "
"Przestrzeń stracona "
"Desperdiciado    "
"Залишки кластерів   "

SetAttrTitle
l:
"Атрибуты"
"Attributes"
"Atributy"
"Attribute"
"Attribútumok"
"Atrybuty"
"Atributos"
"Атрибути"

SetAttrFor
"Изменить файловые атрибуты"
"Change file attributes for"
"Změna atributů souboru pro"
"Ändere Dateiattribute für"
"Attribútumok megváltoztatása"
"Zmień atrybuty dla"
"Cambiar atributos del archivo"
"Змінити файлові атрибути"

SetAttrSelectedObjects
"выбранных объектов"
"selected objects"
"vybrané objekty"
"markierte Objekte"
"a kijelölt objektumokon"
"wybranych obiektów"
"objetos seleccionados"
"вибраних об'єктів"

SetAttrHardLinks
"жёстких ссылок"
"hard links"
"pevné linky"
"Hardlinks"
"hardlink"
"linków trwałych"
"Enlace rígido"
"жорстких посилань"

SetAttrJunction
"Связь:"
"Junction:"
"Křížení:"
"Knotenpunkte:"
"Сsomópont:"
"Powiązanie:"
"Juntar:"
"Зв'язок:"

SetAttrSymlink
"Ссылка:"
"Symlink:"
"Link:"
"Symlink:"
"Szimlink:"
"Link:"
"Enlace:"
"Посилання:"

SetAttrVolMount
"Том:"
"Volume:"
"Svazek:"
"Datenträger:"
"Kötet:"
"Punkt zamontowania:"
"Volumen:"
"Том:"

SetAttrUnknownJunction
"(нет данных)"
"(data not available)"
"(data nejsou k dispozici)"
"(nicht verfügbar)"
"(adat nem elérhető)"
"(dane niedostępne)"
"(dato no disponible)"
"(немає даних)"

SetAttrSubfolders
"Обрабатывать &вложенные папки"
"Process sub&folders"
"Zpracovat i po&dadresáře"
"Unterordner miteinbe&ziehen"
"Az almappákon is"
"Przetwarzaj &podkatalogi"
"Procesar sub&directorios"
"Обробляти &вкладені теки"

SetAttrOwner
"Владелец:"
"Owner:"
"Vlastník:"
"Besitzer:"
"Tulajdonos:"
"Właściciel:"
"Dueño:"
"Власник:"

SetAttrOwnerMultiple
"(несколько значений)"
"(multiple values)"
upd:"(multiple values)"
upd:"(multiple values)"
upd:"(multiple values)"
upd:"(multiple values)"
"(valores múltiples)"
"(кілька значень)"

SetAttrModification
"Время последней &записи:"
"Last &write time:"
upd:"Last &write time:"
upd:"Last &write time:"
upd:"Last &write time:"
upd:"Last &write time:"
upd:"Last &write time:"
"Час останнього &запису:"

SetAttrCreation
"Время со&здания:"
"Crea&tion time:"
"Čas v&ytvoření:"
"Datei erstell&t:"
"&Létrehozás dátuma/ideje:"
"Czas u&tworzenia:"
"Hora de creación:"
"Час ст&ворення:"

SetAttrLastAccess
"Время последнего &доступа:"
"&Last access time:"
"Čas posledního pří&stupu:"
"&Letzter Zugriff:"
"&Utolsó hozzáférés dátuma/ideje:"
"Czas ostatniego &dostępu:"
"Hora de &último acceso:"
"Час останнього &доступу:"

SetAttrChange
"Время из&менения:"
"Chan&ge time:"
upd:"Change time:"
upd:"Change time:"
upd:"Change time:"
upd:"Change time:"
upd:"Change time:"
"Час зм&іни:"

SetAttrOriginal
"Исход&ное"
"&Original"
"&Originál"
"&Original"
"&Eredeti"
"Wstaw &oryginalny"
"Ori&ginal"
"Вихід&не"

SetAttrCurrent
"Те&кущее"
"Curre&nt"
"So&učasný"
"Akt&uell"
"Aktuál&is"
"Wstaw &bieżący"
"Ac&tual"
"По&точне"

SetAttrBlank
"Сбр&ос"
"&Blank"
"P&rázdný"
"L&eer"
"&Üres"
"&Wyczyść"
"&Vaciar"
"Ски&дання"

SetAttrSet
"Установить"
"Set"
"Nastavit"
"Setzen"
"Alkalmaz"
"Usta&w"
"Poner"
"Встановити"

SetAttrTimeTitle1
l:
"ММ%cДД%cГГГГГ чч%cмм%cсс%cмс"
"MM%cDD%cYYYYY hh%cmm%css%cms"
upd:"MM%cDD%cRRRRR hh%cmm%css%cms"
upd:"MM%cTT%cJJJJJ hh%cmm%css%cms"
upd:"HH%cNN%cÉÉÉÉÉ óó%cpp%cmm%cms"
upd:"MM%cDD%cRRRRR gg%cmm%css%cms"
"MM%cDD%cAAAAA hh%cmm%css"
"ММ%cДД%cРРРРР гг%cхх%cсс%cмс"

SetAttrTimeTitle2
"ДД%cММ%cГГГГГ чч%cмм%cсс%cмс"
"DD%cMM%cYYYYY hh%cmm%css%cms"
upd:"DD%cMM%cRRRRR hh%cmm%css%cms"
upd:"TT%cMM%cJJJJJ hh%cmm%css%cms"
upd:"NN%cHH%cÉÉÉÉÉ óó%cpp%cmm%cms"
upd:"DD%cMM%cRRRRR gg%cmm%css%cms"
"DD%cMM%cAAAAA hh%cmm%css"
"ДД%cММ%cРРРРР гг%cхх%cсс%cмс"

SetAttrTimeTitle3
"ГГГГГ%cММ%cДД чч%cмм%cсс%cмс"
"YYYYY%cMM%cDD hh%cmm%css%cms"
upd:"RRRRR%cMM%cDD hh%cmm%css%cms"
upd:"JJJJJ%cMM%cTT hh%cmm%css%cms"
upd:"ÉÉÉÉÉ%cHH%cNN óó%cpp%cmm%cms"
upd:"RRRRR%cMM%cDD gg%cmm%css%cms"
"AAAAA%cMM%cDD hh%cmm%css"
"ГГГГГ%cММ%cДД гг%cхх%cсс%cмс"

SetAttrSystemDialog
"Системные &свойства"
"System &properties"
upd:"System &properties"
upd:"System &properties"
upd:"System &properties"
upd:"System &properties"
"&Propiedades del sistema"
"Системні &властивості"

SetAttrSetting
l:
"Установка файловых атрибутов для"
"Setting file attributes for"
"Nastavení atributů souboru pro"
"Setze Dateiattribute für"
"Attribútumok beállítása"
"Ustawiam atrybuty"
"Poniendo atributos de archivo para"
"Встановлення файлових атрибутів для"

SetAttrCannotFor
"Ошибка установки атрибутов для"
"Cannot set attributes for"
"Nelze nastavit atributy pro"
"Konnte Dateiattribute nicht setzen für"
"Az attribútumok nem állíthatók be:"
"Nie mogę ustawić atrybutów dla"
"No se pueden poner atributos para"
"Помилка установки атрибутів для"

SetAttrCompressedCannotFor
"Не удалось установить атрибут СЖАТЫЙ для"
"Cannot set attribute COMPRESSED for"
"Nelze nastavit atribut KOMPRIMOVANÝ pro"
"Konnte Komprimierung nicht setzen für"
"A TÖMÖRÍTETT attribútum nem állítható be:"
"Nie mogę ustawić atrybutu SKOMPRESOWANY dla"
"No se puede poner atributo COMPRIMIDO a"
"Не вдалося встановити атрибут СТИСНУТИЙ для"

SetAttrEncryptedCannotFor
"Не удалось установить атрибут ЗАШИФРОВАННЫЙ для"
"Cannot set attribute ENCRYPTED for"
"Nelze nastavit atribut ŠIFROVANÝ pro"
"Konnte Verschlüsselung nicht setzen für"
"A TITKOSÍTOTT attribútum nem állítható be:"
"Nie mogę ustawić atrybutu ZASZYFROWANY dla"
"No se puede poner atributo CIFRADO a"
"Не вдалося встановити атрибут ЗАШИФОВАНИЙ для"

SetAttrSparseCannotFor
"Не удалось установить атрибут РАЗРЕЖЁННЫЙ для"
"Cannot set attribute SPARSE for"
upd:"Cannot set attribute SPARSE for"
upd:"Cannot set attribute SPARSE for"
"A RITKÍTOTT attribútum nem állítható be:"
upd:"Cannot set attribute SPARSE for"
"No se puede poner atributo SPARSE para"
"Не вдалося встановити атрибут РОЗРІЖЕНИЙ для"

SetAttrTimeCannotFor
"Не удалось установить время файла для"
"Cannot set file time for"
"Nelze nastavit čas souboru pro"
"Konnte Dateidatum nicht setzen für"
"A dátum nem állítható be:"
"Nie mogę ustawić czasu pliku dla"
"No se puede poner hora de archivo para"
"Не вдалося встановити час для файлу"

SetAttrOwnerCannotFor
"Не удалось установить владельца для"
"Cannot set owner for"
upd:"Cannot set owner for"
upd:"Cannot set owner for"
upd:"Cannot set owner for"
upd:"Cannot set owner for"
"No se puede poner como dueño para"
"Не вдалося встановити власника для"

SetAttrGroupCannotFor
"Не удалось установить группу для"
"Cannot set group for"
upd:"Cannot set group for"
upd:"Cannot set group for"
upd:"Cannot set group for"
upd:"Cannot set group for"
upd:"Cannot set group for"
"Не вдалося встановити групу для"

SetAttrGroup
"Группа:"
"Group:"
upd:"Group:"
upd:"Group:"
upd:"Group:"
upd:"Group:"
upd:"Group:"
"Група:"

SetAttrAccessUser
"Права пользователя"
"User's access"
upd:"User's access"
upd:"User's access"
upd:"User's access"
upd:"User's access"
upd:"User's access"
"Права користувача"

SetAttrAccessGroup
"Права группы"
"Group's access"
upd:"Group's access"
upd:"Group's access"
upd:"Group's access"
upd:"Group's access"
upd:"Group's access"
"Права групи"

SetAttrAccessOther
"Права остальных"
"Other's access"
upd:"Other's access"
upd:"Other's access"
upd:"Other's access"
upd:"Other's access"
upd:"Other's access"
"Права інших"

SetAttrAccessUserRead
"&Чтение"
"&Read"
upd:"&Read"
upd:"&Read"
upd:"&Read"
upd:"&Read"
upd:"&Read"
"&Читання"

SetAttrAccessUserWrite
"&Запись"
"&Write"
upd:"&Write"
upd:"&Write"
upd:"&Write"
upd:"&Write"
upd:"&Write"
"&Запис"

SetAttrAccessUserExecute
"&Исполнение"
"E&xecute"
upd:"E&xecute"
upd:"E&xecute"
upd:"E&xecute"
upd:"E&xecute"
upd:"E&xecute"
"&Виконання"

SetAttrAccessGroupRead
"Чтение"
"Read"
upd:"Read"
upd:"Read"
upd:"Read"
upd:"Read"
upd:"Read"
"Читання"

SetAttrAccessGroupWrite
"Запись"
"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
"Запис"

SetAttrAccessGroupExecute
"Исполнение"
"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
"Виконання"

SetAttrAccessOtherRead
"Чтение"
"Read"
upd:"Read"
upd:"Read"
upd:"Read"
upd:"Read"
upd:"Read"
"Читання"

SetAttrAccessOtherWrite
"Запись"
"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
"Запис"

SetAttrAccessOtherExecute
"Исполнение"
"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
"Виконання"

SetAttrAccessTime
"Время последнего доступа"
"Last access time"
upd:"Last access time"
upd:"Last access time"
upd:"Last access time"
upd:"Last access time"
upd:"Last access time"
"Час останнього доступу"

SetAttrModificationTime
"Время последней модификации"
"Last modification time"
upd:"Last modification time"
upd:"Last modification time"
upd:"Last modification time"
upd:"Last modification time"
upd:"Last modification time"
"Час останньої модифікації"

SetAttrStatusChangeTime
"Время изменения статуса"
"Last status change time"
upd:"Last status change time"
upd:"Last status change time"
upd:"Last status change time"
upd:"Last status change time"
upd:"Last status change time"
"Час зміни статусу"

SetColorPanel
l:
"&Панель"
"&Panel"
"&Panel"
"&Panel"
"&Panel"
"&Panel"
"&Panel"
"&Панель"

SetColorDialog
"&Диалог"
"&Dialog"
"&Dialog"
"&Dialog"
"Pár&beszédablak"
"Okno &dialogowe"
"&Diálogo"
"&Діалог"

SetColorWarning
"Пр&едупреждение"
"&Warning message"
"&Varovná zpráva"
"&Warnmeldung"
"&Figyelmeztetés"
"&Ostrzeżenie"
"Me&nsaje de advertencia"
"Поп&ередження"

SetColorMenu
"&Меню"
"&Menu"
"&Menu"
"&Menü"
"&Menü"
"&Menu"
"&Menú"
"&Меню"

SetColorHMenu
"&Горизонтальное меню"
"Hori&zontal menu"
"Hori&zontální menu"
"Hori&zontales Menü"
"&Vízszintes menü"
"Pa&sek menu"
"Menú hori&zontal"
"&Горизонтальне меню"

SetColorKeyBar
"&Линейка клавиш"
"&Key bar"
"&Řádek kláves"
upd:"&Key bar"
"F&unkcióbill.sor"
"Pasek &klawiszy"
"Barra de me&nú"
"&Лінійка клавиш"

SetColorCommandLine
"&Командная строка"
"&Command line"
"Pří&kazový řádek"
"&Kommandozeile"
"P&arancssor"
"&Linia poleceń"
"Línea de &comando"
"&Командна строка"

SetColorClock
"&Часы"
"C&lock"
"&Hodiny"
"U&hr"
"Ó&ra"
"&Zegar"
"Re&loj"
"&Годинник"

SetColorViewer
"Про&смотрщик"
"&Viewer"
"P&rohlížeč"
"&Betrachter"
"&Nézőke"
"Pod&gląd"
"&Visor"
"Пере&глядач"

SetColorEditor
"&Редактор"
"&Editor"
"&Editor"
"&Editor"
"&Szerkesztő"
"&Edytor"
"&Editor"
"&Редактор"

SetColorHelp
"П&омощь"
"&Help"
"&Nápověda"
"&Hilfe"
"Sú&gó"
"P&omoc"
"&Ayuda"
"Д&опомога"

SetDefaultColors
"&Установить стандартные цвета"
"Set de&fault colors"
"N&astavit výchozí barvy"
"Setze Standard&farben"
"Alapért. s&zínek"
"&Ustaw kolory domyślne"
"Poner colores prede&terminados"
"&Встановити стандартні кольори"

SetBW
"Чёрно-бел&ый режим"
"&Black and white mode"
"Černo&bílý mód"
"Schwarz && &Weiß"
"Fekete-fe&hér mód"
"&Tryb czarno-biały"
"Modo &blanco y negro"
"Чорно-біл&ий режим"

SetColorPanelNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorPanelSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Wybrany tekst"
"Texto seleccionado"
"Вибраний текст"

SetColorPanelHighlightedInfo
"Выделенная информация"
"Highlighted info"
"Info zvýrazněné"
"Markierung"
"Kiemelt info"
"Podświetlone info"
"Info resaltados"
"Виділена інформація"

SetColorPanelDragging
"Перетаскиваемый текст"
"Dragging text"
"Tažený text"
"Drag && Drop Text"
"Vonszolt szöveg"
"Przeciągany tekst"
"Texto arrastrado"
"Перетягуваний текст"

SetColorPanelBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

SetColorPanelNormalCursor
"Обычный курсор"
"Normal cursor"
"Normální kurzor"
"Normale Auswahl"
"Normál kurzor"
"Normalny kursor"
"Cursor normal"
"Звичайний курсор"

SetColorPanelSelectedCursor
"Выделенный курсор"
"Selected cursor"
"Vybraný kurzor"
"Markierte Auswahl"
"Kijelölt kurzor"
"Wybrany kursor"
"Cursor seleccionado"
"Виділений курсор"

SetColorPanelNormalTitle
"Обычный заголовок"
"Normal title"
"Normální nadpis"
"Normaler Titel"
"Normál név"
"Normalny tytuł"
"Título normal"
"Звичайний заголовок"

SetColorPanelSelectedTitle
"Выделенный заголовок"
"Selected title"
"Vybraný nadpis"
"Markierter Titel"
"Kijelölt név"
"Wybrany tytuł"
"Título seleccionado"
"Виділелений заголовок"

SetColorPanelColumnTitle
"Заголовок колонки"
"Column title"
"Nadpis sloupce"
"Spaltentitel"
"Oszlopnév"
"Tytuł kolumny"
"Título de columna"
"Заголовок колонки"

SetColorPanelTotalInfo
"Количество файлов"
"Total info"
"Info celkové"
"Gesamtinfo"
"Összes info"
"Całkowite info"
"Info total"
"Кількість файлів"

SetColorPanelSelectedInfo
"Количество выбранных файлов"
"Selected info"
"Info výběr"
"Markierungsinfo"
"Kijelölt info"
"Wybrane info"
"Info seleccionados"
"Кількість вибраних файлів"

SetColorPanelScrollbar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra desplazamiento"
"Полоса прокрутки"

SetColorPanelScreensNumber
"Количество фоновых экранов"
"Number of background screens"
"Počet obrazovek na pozadí"
"Anzahl an Hintergrundseiten"
"Háttérképernyők száma"
"Ilość ekranów w tle "
"Número de pantallas de fondo"
"Кількість фонових екранів"

SetColorDialogNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Tekst zwykły"
"Texto normal"
"Звичайний текст"

SetColorDialogHighlighted
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierter Text"
"Kiemelt szöveg"
"Tekst podświetlony"
"Texto resaltado"
"Виділений текст"

SetColorDialogDisabled
"Блокированный текст"
"Disabled text"
"Zakázaný text"
"Deaktivierter Text"
"Inaktív szöveg"
"Tekst nieaktywny"
"Deshabilitar texto"
"Блокований текст"

SetColorDialogBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

SetColorDialogBoxTitle
"Заголовок рамки"
"Title"
"Nadpis"
"Titel"
"Keret neve"
"Tytuł"
"Título"
"Заголовок рамки"

SetColorDialogHighlightedBoxTitle
"Выделенный заголовок рамки"
"Highlighted title"
"Zvýrazněný nadpis"
"Markierter Titel"
"Kiemelt keretnév"
"Podświetlony tytuł"
"Título resaltado"
"Виділений заголовок рамки"

SetColorDialogTextInput
"Ввод текста"
"Text input"
"Textový vstup"
"Texteingabe"
"Beírt szöveg"
"Wpisywany tekst"
"Entrada de texto"
"Введення тексту"

SetColorDialogUnchangedTextInput
"Неизмененный текст"
"Unchanged text input"
"Nezměněný textový vstup"
"Unveränderte Texteingabe"
"Változatlan beírt szöveg"
"Niezmieniony wpisywany tekst "
"Entrada de texto sin cambiar"
"Незмінений текст"

SetColorDialogSelectedTextInput
"Ввод выделенного текста"
"Selected text input"
"Vybraný textový vstup"
"Markierte Texteingabe"
"Beírt szöveg kijelölve"
"Zaznaczony wpisywany tekst"
"Entrada de texto seleccionada"
"Введення виділеного тексту"

SetColorDialogEditDisabled
"Блокированное поле ввода"
"Disabled input line"
"Zakázaný vstupní řádek"
"Deaktivierte Eingabezeile"
"Inaktív beviteli sor"
"Nieaktywna linia wprowadzania danych"
"Deshabilitar línea de entrada"
"Блоковане поле вводу"

SetColorDialogButtons
"Кнопки"
"Buttons"
"Tlačítka"
"Schaltflächen"
"Gombok"
"Przyciski"
"Botones"
"Кнопки"

SetColorDialogSelectedButtons
"Выбранные кнопки"
"Selected buttons"
"Vybraná tlačítka"
"Aktive Schaltflächen"
"Kijelölt gombok"
"Wybrane przyciski"
"Botones seleccionados"
"Вибрані кнопки"

SetColorDialogHighlightedButtons
"Выделенные кнопки"
"Highlighted buttons"
"Zvýrazněná tlačítka"
"Markierte Schaltflächen"
"Kiemelt gombok"
"Podświetlone przyciski"
"Botones resaltados"
"Виділені кнопки"

SetColorDialogSelectedHighlightedButtons
"Выбранные выделенные кнопки"
"Selected highlighted buttons"
"Vybraná zvýrazněná tlačítka"
"Aktive markierte Schaltflächen"
"Kijelölt kiemelt gombok"
"Wybrane podświetlone przyciski "
"Botones resaltados seleccionados"
"Вибрані виділені кнопки"

SetColorDialogDefaultButton
"Кнопка по умолчанию"
"Default button"
upd:"Default button"
upd:"Default button"
upd:"Default button"
upd:"Default button"
"Botón por defecto"
"Кнопка за замовчуванням"

SetColorDialogSelectedDefaultButton
"Выбранная кнопка по умолчанию"
"Selected default button"
upd:"Selected default button"
upd:"Selected default button"
upd:"Selected default button"
upd:"Selected default button"
"Botón por defecto seleccionado"
"Вибрана кнопка за замовчуванням"

SetColorDialogHighlightedDefaultButton
"Выделенная кнопка по умолчанию"
"Highlighted default button"
upd:"Highlighted default button"
upd:"Highlighted default button"
upd:"Highlighted default button"
upd:"Highlighted default button"
"Botón por defecto resaltado"
"Виділена кнопка за замовчуванням"

SetColorDialogSelectedHighlightedDefaultButton
"Выбранная выделенная кнопка по умолчанию"
"Selected highlighted default button"
upd:"Selected highlighted default button"
upd:"Selected highlighted default button"
upd:"Selected highlighted default button"
upd:"Selected highlighted default button"
"Botón por defecto resaltado seleccionado"
"Вибрана кнопка за замовчуванням"

SetColorDialogListBoxControl
"Список"
"List box"
"Seznam položek"
"Listenfelder"
"Listaablak"
"Lista"
"Cuadro de lista"
"Список"

SetColorDialogComboBoxControl
"Комбинированный список"
"Combobox"
"Výběr položek"
"Kombinatiosfelder"
"Lenyíló szövegablak"
"Pole combo"
"Cuadro combo"
"Комбінований список"

SetColorDialogListText
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Tekst zwykły"
"Texto normal"
"Звичайний текст"

SetColorDialogListSelectedText
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Tekst wybrany"
"Texto seleccionado"
"Вибраний текст"

SetColorDialogListHighLight
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierung"
"Kiemelt szöveg"
"Tekst podświetlony"
"Texto resaltado"
"Виділений текст"

SetColorDialogListSelectedHighLight
"Выбранный выделенный текст"
"Selected highlighted text"
"Vybraný zvýrazněný text"
"Aktive Markierung"
"Kijelölt kiemelt szöveg"
"Tekst wybrany i podświetlony"
"Texto resaltado seleccionado"
"Вибраний виділений текст"

SetColorDialogListDisabled
"Блокированный пункт"
"Disabled item"
"Naktivní položka"
"Deaktiviertes Element"
"Inaktív elem"
"Pole nieaktywne"
"Deshabilitar ítem"
"Блокований пункт"

SetColorDialogListBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

SetColorDialogListTitle
"Заголовок"
"Title"
"Nadpis"
"Titel"
"Keret neve"
"Tytuł"
"Título"
"Заголовок"

SetColorDialogListGrayed
"Серый текст списка"
"Grayed list text"
upd:"Grayed list text"
upd:"Grayed list text"
"Szürke listaszöveg"
upd:"Grayed list text"
"Texto de listado en gris"
"Сірий текст списку"

SetColorDialogSelectedListGrayed
"Выбранный серый текст списка"
"Selected grayed list text"
upd:"Selected grayed list text"
upd:"Selected grayed list text"
"Kijelölt szürke listaszöveg"
upd:"Selected grayed list text"
"Texto de listado en gris seleccionado"
"Вибраний сірий текст списку"

SetColorDialogListScrollBar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra desplazamiento"
"Полоса прокрутки"

SetColorDialogListArrows
"Индикаторы длинных строк"
"Long string indicators"
"Značka dlouhého řetězce"
"Indikator für lange Zeichenketten"
"Hosszú sztring jelzők"
"Znacznik długiego napisu"
"Indicadores de cadena larga"
"Індикатори довгих рядків"

SetColorDialogListArrowsSelected
"Выбранные индикаторы длинных строк"
"Selected long string indicators"
"Vybraná značka dlouhého řetězce"
"Aktiver Indikator"
"Kijelölt hosszú sztring jelzők"
"Zaznaczone znacznik długiego napisu"
"Indicadores de cadena larga seleccionados"
"Вибрані індикатори довгих рядків"

SetColorDialogListArrowsDisabled
"Блокированные индикаторы длинных строк"
"Disabled long string indicators"
"Zakázaná značka dlouhého řetězce"
"Deaktivierter Indikator"
"Inaktív hosszú sztring jelzők"
"Nieaktywny znacznik długiego napisu"
"Deshabilitar indicadores de cadena largos"
"Блоковані індикатори довгих рядків"

SetColorMenuNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorMenuSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Wybrany tekst"
"Texto seleccionado"
"Вибраний текст"

SetColorMenuHighlighted
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierung"
"Kiemelt szöveg"
"Podświetlony tekst"
"Texto resaltado"
"Виділений текст"

SetColorMenuSelectedHighlighted
"Выбранный выделенный текст"
"Selected highlighted text"
"Vybraný zvýrazněný text"
"Aktive Markierung"
"Kijelölt kiemelt szöveg"
"Wybrany podświetlony tekst "
"Texto resaltado seleccionado"
"Вибраний виділений текст"

SetColorMenuDisabled
"Недоступный пункт"
"Disabled text"
"Neaktivní text"
"Disabled text"
"Inaktív szöveg"
"Tekst nieaktywny"
"Deshabilitar texto"
"Недоступний пункт"

SetColorMenuGrayed
"Серый текст"
"Grayed text"
upd:"Grayed text"
upd:"Grayed text"
"Szürke szöveg"
upd:"Grayed text"
"Texto en gris"
"Сірий текст"

SetColorMenuSelectedGrayed
"Выбранный серый текст"
"Selected grayed text"
upd:"Selected grayed text"
upd:"Selected grayed text"
"Kijelölt szürke szöveg"
upd:"Selected grayed text"
"Texto en gris seleccionado"
"Вибраний сірий текст"

SetColorMenuBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

SetColorMenuTitle
"Заголовок"
"Title"
"Nadpis"
"Titel"
"Keret neve"
"Tytuł"
"Título"
"Заголовок"

SetColorMenuScrollBar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra desplazamiento"
"Полоса прокрутки"

SetColorMenuArrows
"Индикаторы длинных строк"
"Long string indicators"
"Značka dlouhého řetězce"
"Long string indicators"
"Hosszú sztring jelzők"
"Znacznik długiego napisu"
"Indicadores de cadena larga"
"Індикатори довгих рядків"

SetColorMenuArrowsSelected
"Выбранные индикаторы длинных строк"
"Selected long string indicators"
"Vybraná značka dlouhého řetězce"
"Selected long string indicators"
"Kijelölt hosszú sztring jelzők"
"Zaznaczone znacznik długiego napisu"
"Indicadores de cadena larga seleccionados"
"Вибрані індикатори довгих рядків"

SetColorMenuArrowsDisabled
"Блокированные индикаторы длинных строк"
"Disabled long string indicators"
"Zakázaná značka dlouhého řetězce"
"Disabled long string indicators"
"Inaktív hosszú sztring jelzők"
"Nieaktywny znacznik długiego napisu"
"Deshabilitar indicadores de cadena largos"
"Блоковані індикатори довгих рядків"

SetColorHMenuNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorHMenuSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Wybrany tekst"
"Texto seleccionado"
"Вибраний текст"

SetColorHMenuHighlighted
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierung"
"Kiemelt szöveg"
"Podświetlony tekst"
"Texto resaltado"
"Виділений текст"

SetColorHMenuSelectedHighlighted
"Выбранный выделенный текст"
"Selected highlighted text"
"Vybraný zvýrazněný text"
"Aktive Markierung"
"Kijelölt kiemelt szöveg"
"Wybrany podświetlony tekst "
"Texto resaltado seleccionado"
"Вибраний виділений текст"

SetColorKeyBarNumbers
l:
"Номера клавиш"
"Key numbers"
"Čísla kláves"
"Tastenziffern"
"Funkció száma"
"Numery klawiszy"
"Números teclas"
"Номери клавіш"

SetColorKeyBarNames
"Названия клавиш"
"Key names"
"Názvy kláves"
"Tastennamen"
"Funkció neve"
"Nazwy klawiszy"
"Nombres teclas"
"Назви клавіш"

SetColorKeyBarBackground
"Фон"
"Background"
"Pozadí"
"Hintergrund"
"Háttere"
"Tło"
"Fondo"
"Фон"

SetColorCommandLineNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorCommandLineSelected
"Выделенный текст"
"Selected text input"
"Vybraný textový vstup"
"Markierte Texteingabe"
"Beírt szöveg kijelölve"
"Zaznaczony wpisany tekst"
"Entrada de texto seleccionada"
"Виділелений текст"

SetColorCommandLinePrefix
"Текст префикса"
"Prefix text"
"Text předpony"
"Prefix Text"
"Előtag szövege"
"Tekst prefiksu"
"Texto prefijado"
"Текст префіксу"

SetColorCommandLineUserScreen
"Пользовательский экран"
"User screen"
"Obrazovka uživatele"
"Benutzerseite"
"Konzol háttere"
"Ekran użytkownika"
"Pantalla de usuario"
"Користувацький екран"

SetColorClockNormal
l:
"Обычный текст (панели)"
"Normal text (Panel)"
"Normální text (Panel)"
"Normaler Text (Panel)"
"Normál szöveg (panelek)"
"Normalny tekst (Panel)"
"Texto normal (Panel)"
"Звичайний текст (панелі)"

SetColorClockNormalEditor
"Обычный текст (редактор)"
"Normal text (Editor)"
"Normální text (Editor)"
"Normaler Text (Editor)"
"Normál szöveg (szerkesztő)"
"Normalny tekst (Edytor)"
"Texto normal (Editor)"
"Звичайний текст (редактор)"

SetColorClockNormalViewer
"Обычный текст (вьювер)"
"Normal text (Viewer)"
"Normální text (Prohlížeč)"
"Normaler Text (Betrachter)"
"Normál szöveg (nézőke)"
"Normalny tekst (Podgląd)"
"Texto normal (Visor)"
"Звичайний текст (в'ювер)"

SetColorViewerNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorViewerSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Zaznaczony tekst"
"Texto seleccionado"
"Вибраний текст"

SetColorViewerStatus
"Статус"
"Status line"
"Stavový řádek"
"Statuszeile"
"Állapotsor"
"Linia statusu"
"Línea de estado"
"Статус"

SetColorViewerArrows
"Стрелки сдвига экрана"
"Screen scrolling arrows"
"Skrolovací šipky"
"Pfeile auf Scrollbalken"
"Képernyőgördítő nyilak"
"Strzałki przesuwające ekran"
"Flechas desplazamiento de pantalla"
"Стрілки зсуву екрана"

SetColorViewerScrollbar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barras desplazamiento"
"Полоса прокрутки"

SetColorEditorNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorEditorSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Zaznaczony tekst"
"Texto seleccionado"
"Вибраний текст"

SetColorEditorStatus
"Статус"
"Status line"
"Stavový řádek"
"Statuszeile"
"Állapotsor"
"Linia statusu"
"Línea de estado"
"Статус"

SetColorEditorScrollbar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra de desplazamiento"
"Полоса прокрутки"

SetColorHelpNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

SetColorHelpHighlighted
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierung"
"Kiemelt szöveg"
"Podświetlony tekst"
"Texto resaltado"
"Виділений текст"

SetColorHelpReference
"Ссылка"
"Reference"
"Odkaz"
"Referenz"
"Hivatkozás"
"Odniesienie"
"Referencia"
"Посилання"

SetColorHelpSelectedReference
"Выбранная ссылка"
"Selected reference"
"Vybraný odkaz"
"Ausgewählte Referenz"
"Kijelölt hivatkozás"
"Wybrane odniesienie "
"Referencia seleccionada"
"Вибране посилання"

SetColorHelpBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

SetColorHelpBoxTitle
"Заголовок рамки"
"Title"
"Nadpis"
"Titel"
"Keret neve"
"Tytuł"
"Título"
"Заголовок рамки"

SetColorHelpScrollbar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra desplazamiento"
"Полоса прокрутки"

SetColorGroupsTitle
l:
"Цветовые группы"
"Color groups"
"Skupiny barev"
"Farbgruppen"
"Színcsoportok"
"Grupy kolorów"
"Grupos de colores"
"Кольорові групи"

SetColorItemsTitle
"Элементы группы"
"Group items"
"Položky skupin"
"Gruppeneinträge"
"A színcsoport elemei"
"Elementy grupy"
"Grupos de ítems"
"Елементи групи"

SetColorTitle
l:
"Цвет"
"Color"
"Barva"
"Farbe"
"Színek"
"Kolor"
"Color"
"Колір"

SetColorForeground
"&Текст"
"&Foreground"
"&Popředí"
"&Vordergrund"
"&Előtér"
"&Pierwszy plan"
"&Caracteres"
"&Текст"

SetColorBackground
"&Фон"
"&Background"
"Po&zadí"
"&Hintergrund"
"&Háttér"
"&Tło"
"&Fondo     "
"&Фон"

SetColorForeTransparent
"&Прозрачный"
"&Transparent"
"Průhlednos&t"
"&Transparent"
"Átlá&tszó"
"P&rzezroczyste"
"&Transparente"
"&Прозорий"

SetColorBackTransparent
"П&розрачный"
"T&ransparent"
"Průhledno&st"
"T&ransparent"
"Átlát&szó"
"Pr&zezroczyste"
"T&ransparente"
"П&розорий"

SetColorSample
"Текст Текст Текст Текст Текст Текст"
"Text Text Text Text Text Text Text"
"Text Text Text Text Text Text Text"
"Text Text Text Text Text Text Text"
"Text Text Text Text Text Text Text"
"Tekst Tekst Tekst Tekst Tekst Tekst"
"Texto Texto Texto Texto Texto"
"Текст Текст Текст Текст Текст Текст"

SetColorSet
"Установить"
"Set"
"Nastavit"
"Setzen"
"A&lkalmaz"
"Ustaw"
"Poner"
"Встановити"

SetColorCancel
"Отменить"
"Cancel"
"Storno"
"Abbruch"
"&Mégsem"
"Anuluj"
"Cancelar"
"Скасувати"

SetConfirmTitle
l:
"Подтверждения"
"Confirmations"
"Potvrzení"
"Bestätigungen"
"Megerősítések"
"Potwierdzenia"
"Confirmaciones"
"Підтвердження"

SetConfirmCopy
"Перезапись файлов при &копировании"
"&Copy"
"&Kopírování"
"&Kopieren"
"&Másolás"
"&Kopiowanie"
"&Copiar"
"Перезаписування файлів під час &копіювання"

SetConfirmMove
"Перезапись файлов при &переносе"
"&Move"
"&Přesouvání"
"&Verschieben"
"Moz&gatás"
"&Przenoszenie"
"&Mover"
"Перезаписування файлів під час &перенесення"

SetConfirmRO
"Перезапись и удаление R/O &файлов"
"&Overwrite and delete R/O files"
upd:"&Overwrite and delete R/O files"
upd:"&Overwrite and delete R/O files"
"&Csak olv. fájlok felülírása/törlése"
upd:"&Overwrite and delete R/O files"
"S&obrescribir y eliminar ficheros Sólo/Lectura"
"Перезапис та видалення R/O файлів"

SetConfirmDrag
"Пере&таскивание"
"&Drag and drop"
"&Drag and drop"
"&Ziehen und Ablegen"
"&Húzd és ejtsd"
"P&rzeciąganie i upuszczanie"
"&Arrastrar y soltar"
"Пере&тягання"

SetConfirmDelete
"&Удаление"
"De&lete"
"&Mazání"
"&Löschen"
"&Törlés"
"&Usuwanie"
"&Borrar"
"&Видалення"

SetConfirmDeleteFolders
"У&даление непустых папок"
"Delete non-empty &folders"
"Mazat &neprázdné adresáře"
"Löschen von Ordnern mit &Inhalt"
"Nem &üres mappák törlése"
"Usuwanie &niepustych katalogów"
"Borrar &directorios no-vacíos"
"Ви&далення непорожніх тек"

SetConfirmEsc
"Прерыва&ние операций"
"&Interrupt operation"
"Pře&rušit operaci"
"&Unterbrechen von Vorgängen"
"Mű&velet megszakítása"
"&Przerwanie operacji"
"&Interrumpir operación"
"Перерива&ння операцій"

SetConfirmRemoveConnection
"&Отключение сетевого устройства"
"Disconnect &network drive"
"Odpojení &síťové jednotky"
"Trennen von &Netzwerklaufwerken"
"Háló&zati meghajtó leválasztása"
"Odłączenie dysku &sieciowego"
"Desconectar u&nidad de red"
"&Відключення мережного пристрою"

SetConfirmRemoveSUBST
"Отключение SUBST-диска"
"Disconnect &SUBST-disk"
"Odpojení SUBST-d&isku"
"Trennen von &Substlaufwerken"
"Virt&uális meghajtó törlése"
"Odłączenie dysku &SUBST"
"Desconectar disco &sustituido"
"Відключення SUBST-диска"

SetConfirmDetachVHD
"Отсоедиение виртуального диска"
"Detach virtual disk"
upd:"Detach virtual disk"
upd:"Detach virtual disk"
upd:"Detach virtual disk"
upd:"Detach virtual disk"
upd:"Detach virtual disk"
"Від'єднання віртуального диска"

SetConfirmRemoveHotPlug
"Отключение HotPlug-у&стройства"
"Hot&Plug-device removal"
"Odpojení vyjímatelného zařízení"
"Sicheres Entfernen von Hardware"
"H&otPlug eszköz eltávolítása"
"Odłączanie urządzenia HotPlug"
"Remover dispositivo de conexión"
"Вимкнення HotPlug-п&ристроя"

SetConfirmAllowReedit
"Повто&рное открытие файла в редакторе"
"&Reload edited file"
"&Obnovit upravovaný soubor"
"Bea&rbeitete Datei neu laden"
"&Szerkesztett fájl újratöltése"
"&Załaduj edytowany plik"
"&Recargar archivo editado"
"Повто&рне відкриття файлу в редакторі"

SetConfirmHistoryClear
"Очистка списка &истории"
"Clear &history list"
"Vymazat seznam &historie"
"&Historielisten löschen"
"&Előzménylista törlése"
"Czyszczenie &historii"
"Limpiar listado de &historial"
"Очищення списку &історії"

SetConfirmExit
"&Выход"
"E&xit"
"U&končení"
"Be&enden"
"K&ilépés a FAR-ból"
"&Wyjście"
"&Salir"
"&Виход"

PluginsManagerSettingsTitle
l:
"Параметры менеджера внешних модулей"
"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
"Параметри менеджера зовнішніх модулів"

PluginsManagerOEMPluginsSupport
"Поддержка OEM-плагинов"
"OEM plugins support"
upd:"OEM plugins support"
upd:"OEM plugins support"
upd:"OEM plugins support"
upd:"OEM plugins support"
upd:"OEM plugins support"
"Підтримка OEM-плагінів"

PluginsManagerScanSymlinks
"Ск&анировать символические ссылки"
"Scan s&ymbolic links"
"Prohledávat s&ymbolické linky"
"S&ymbolische Links scannen"
"Szimbolikus linkek &vizsgálata"
"Skanuj linki s&ymboliczne"
"Explorar enlaces simbólicos"
"Ск?анувати символічні посилання"

PluginsManagerPersonalPath
"Путь к персональным п&лагинам:"
"&Path for personal plugins:"
"&Cesta k vlastním pluginům:"
"&Pfad für eigene Plugins:"
"Saját plu&ginek útvonala:"
"Ś&cieżka do własnych pluginów:"
"Ruta para pl&ugins personales:
"Шлях до персональних п&лагінів:"

PluginsManagerOFP
"Обработка &файла (OpenFilePlugin)"
"&File processing (OpenFilePlugin)"
upd:"&File processing (OpenFilePlugin)"
upd:"&File processing (OpenFilePlugin)"
"&Fájl feldolgozása (OpenFilePlugin)"
upd:"&File processing (OpenFilePlugin)"
"Proceso de archivo (OpenFilePlugin)"
"Обробка &файлу (OpenFilePlugin)"

PluginsManagerStdAssoc
"Пункт вызова стандартной &ассоциации"
"Show standard &association item"
upd:"Show standard &association item"
upd:"Show standard &association item"
"Szabvány társítás megjelenítése"
upd:"Show standard &association item"
"Mostrar asociaciones normales de ítems"
"Пункт виклику стандартної &асоціації"

PluginsManagerEvenOne
"Даже если найден всего &один плагин"
"Even if only &one plugin found"
upd:"Even if only &one plugin found"
upd:"Even if only &one plugin found"
"Akkor is, ha csak egy plugin van"
upd:"Even if only &one plugin found"
"Aún si solo se encontr un plugin"
"Навіть якщо знайдено всього &один плагін"

PluginsManagerSFL
"&Результаты поиска (SetFindList)"
"Search &results (SetFindList)"
upd:"Search &results (SetFindList)"
upd:"Search &results (SetFindList)"
"Keresés eredménye (SetFindList)"
upd:"Search &results (SetFindList)"
"Resultados de búsqueda (SetFindList)"
"&Результати пошуку (SetFindList)"

PluginsManagerPF
"Обработка &префикса"
"&Prefix processing"
upd:"&Prefix processing"
upd:"&Prefix processing"
"Előtag feldolgozása"
upd:"&Prefix processing"
"Proceso de prefijo"
"Обробка &префіксу"

PluginConfirmationTitle
"Выбор плагина"
"Plugin selection"
upd:"Plugin selection"
upd:"Plugin selection"
"Plugin választás"
upd:"Plugin selection"
"Selección de plugin"
"Вибір плагіна"

MenuPluginStdAssociation
"Стандартная ассоциация"
"Standard association"
upd:"Standard association"
upd:"Standard association"
"Szabvány társítás"
upd:"Standard association"
"Asociación normal"
"Стандартна асоціація"

FindFolderTitle
l:
"Поиск папки"
"Find folder"
"Najít adresář"
"Ordner finden"
"Mappakeresés"
"Znajdź folder"
"Encontrar directorio"
"Пошук теки"

KBFolderTreeF1
l:
l:// Find folder Tree KeyBar
"Помощь"
"Help"
"Nápověda"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

KBFolderTreeF2
"Обновить"
"Rescan"
"Obnovit"
"Aktual"
"FaFris"
"Czytaj ponownie"
"ReExpl"
"Оновити"

KBFolderTreeF5
"Размер"
"Zoom"
"Zoom"
"Vergr."
"Nagyít"
"Powiększ"
"Zoom"
"Розмір"

KBFolderTreeF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

KBFolderTreeAltF9
"Видео"
"Video"
"Video"
"Vollb"
"Video"
"Video"
"Video"
"Відео"

TreeTitle
"Дерево"
"Tree"
"Stromové zobrazení"
"Baum"
"Fa"
"Drzewo"
"Arbol"
"Дерево"

CannotSaveTree
"Ошибка записи дерева папок в файл"
"Cannot save folders tree to file"
"Adresářový strom nelze uložit do souboru"
"Konnte Ordnerliste nicht in Datei speichern."
"A mappák fastruktúrája nem menthető fájlba"
"Nie mogę zapisać drzewa katalogów do pliku"
"No se puede guardar árbol de directorios al archivo"
"Помилка запису дерева папок у файл"

ReadingTree
"Чтение дерева папок"
"Reading the folders tree"
"Načítám adresářový strom"
"Lese Ordnerliste"
"Mappaszerkezet újraolvasása..."
"Odczytuję drzewo katalogów"
"Leyendo árbol de directorios"
"Читання дерева папок"

UserMenuTitle
l:
"Пользовательское меню"
"User menu"
"Menu uživatele"
"Benutzermenü"
"Felhasználói menü szerkesztése"
"Menu użytkownika"
"Menú de usuario"
"Користувачське меню"

ChooseMenuType
"Выберите тип пользовательского меню для редактирования"
"Choose user menu type to edit"
"Zvol typ menu uživatele pro úpravu"
"Wählen Sie den Typ des zu editierenden Benutzermenüs"
"Felhasználói menü típusa:"
"Wybierz typ menu do edycji"
"Elija tipo de menú usuario a editar"
"Виберіть тип меню користувача для редагування"

ChooseMenuMain
"&Главное"
"&Main"
"&Hlavní"
"&Hauptmenü"
"&Főmenü"
"Główne"
"&Principal"
"&Головне"

ChooseMenuLocal
"&Местное"
"&Local"
"&Lokální"
"&Lokales Menü"
"&Helyi menü"
"Lokalne"
"&Local"
"&Місцеве"

MainMenuTitle
"Главное меню"
"Main menu"
"Hlavní menu"
"Hauptmenü"
"Főmenü"
"Menu główne"
"Menú principal"
"Головне меню"

MainMenuFAR
"Папка FAR"
"FAR folder"
"Složka FARu"
"FAR Ordner"
"FAR mappa"
"Folder FAR-a"
"Directorio FAR"
"Тека FAR"

MainMenuREG
l:
l:// <...menu (Registry)>
"Реестр"
"Registry"
"Registry"
"Reg."
"Registry"
"Rejestr"
"Registro"
"Реєстр"

LocalMenuTitle
"Местное меню"
"Local menu"
"Lokalní menu"
"Lokales Menü"
"Helyi menü"
"Menu lokalne"
"Menú local"
"Місцеве меню"

MainMenuBottomTitle
"Редактирование: Del,Ins,F4,Ctrl-F4"
"Edit: Del,Ins,F4,Ctrl-F4"
"Edit: Del,Ins,F4,Ctrl-F4"
"Bearb.: Entf,Einf,F4,Ctrl-F4"
"Szerk.: Del,Ins,F4,Ctrl-F4"
"Edycja: Del,Ins,F4,Ctrl-F4"
"Editar: Del,Ins,F4"
"Редагування: Del,Ins,F4,Ctrl-F4"

AskDeleteMenuItem
"Вы хотите удалить пункт меню"
"Do you wish to delete the menu item"
"Přejete si smazat položku v menu"
"Do you wish to delete the menu item"
"Biztosan törli a menüelemet?"
"Czy usunąć pozycję menu"
"Desea borrar el ítem del menú"
"Ви хочете видалити пункт меню"

AskDeleteSubMenuItem
"Вы хотите удалить вложенное меню"
"Do you wish to delete the submenu"
"Přejete si smazat podmenu"
"Do you wish to delete the submenu"
"Biztosan törli az almenüt?"
"Czy usunąć podmenu"
"Desea borrar el submenú"
"Ви хочете видалити вкладене меню"

UserMenuInvalidInputLabel
"Неправильный формат метки меню"
"Invalid format for UserMenu Label"
"Neplatný formát pro název Uživatelského menu"
"Invalid format for UserMenu Label"
"A felhasználói menü névformátuma érvénytelen"
"Błędny format etykiety menu użytkownika"
"Formato inválido para etiqueta de menú usuario"
"Неправильний формат мітки меню"

UserMenuInvalidInputHotKey
"Неправильный формат горячей клавиши"
"Invalid format for Hot Key"
"Neplatný formát pro klávesovou zkratku"
"Invalid format for Hot Key"
"A gyorsbillentyű formátuma érvénytelen"
"Błędny format klawisza skrótu"
"Formato inválido para tecla rápida"
"Неправильний формат гарячої клавіші"

EditMenuTitle
l:
"Редактирование пользовательского меню"
"Edit user menu"
"Editace uživatelského menu"
"Menübefehl bearbeiten"
"Parancs szerkesztése"
"Edytuj menu użytkownika"
"Editar menú de usuario"
"Редагування меню користувача"

EditMenuHotKey
"&Горячая клавиша:"
"&Hot key:"
"K&lávesová zkratka:"
"&Kurztaste:"
"&Gyorsbillentyű:"
"&Klawisz skrótu:"
"&Tecla rápida:"
"&Гаряча клавіша:"

EditMenuLabel
"&Метка:"
"&Label:"
"&Popisek:"
"&Bezeichnung:"
"&Név:"
"&Etykieta:"
"&Etiqueta:"
"&Мітка:"

EditMenuCommands
"&Команды:"
"&Commands:"
"Pří&kazy:"
"&Befehle:"
"&Parancsok:"
"&Polecenia:"
"&Comandos:"
"&Команди:"

AskInsertMenuOrCommand
l:
"Вы хотите вставить новую команду или новое меню?"
"Do you wish to insert a new command or a new menu?"
"Přejete si vložit nový příkaz nebo nové menu?"
"Wollen Sie einen neuen Menübefehl oder ein neues Menu erstellen?"
"Új parancs vagy új menü?"
"Czy chcesz wstawić nowe polecenie lub nowe menu?"
"Desea insertar un nuevo comando o un nuevo menú?"
"Ви хочете вставити нову команду або нове меню?"

MenuInsertCommand
"Вставить команду"
"Insert command"
"Vložit příkaz"
"Neuer Befehl"
"Parancs"
"Wstaw polecenie"
"Insertar comando"
"Вставити команду"

MenuInsertMenu
"Вставить меню"
"Insert menu"
"Vložit menu"
"Neues Menü"
"Menü"
"Wstaw menu"
"Insertar menú"
"Вставити меню"

EditSubmenuTitle
l:
"Редактирование метки вложенного меню"
"Edit submenu label"
"Úprava popisku podmenu"
"Untermenü bearbeiten"
"Almenü szerkesztése"
"Edytuj etykietę podmenu"
"Editar etiqueta de submenú"
"Редагування позначки вкладеного меню"

EditSubmenuHotKey
"&Горячая клавиша:"
"&Hot key:"
"Klávesová &zkratka:"
"&Kurztaste:"
"&Gyorsbillentyű:"
"&Klawisz skrótu:"
"&Tecla rápida:"
"&Гаряча клавіша:"

EditSubmenuLabel
"&Метка:"
"&Label:"
"&Popisek:"
"&Bezeichnung:"
"&Név:"
"&Etykieta:"
"&Etiqueta:"
"&Мітка:"

ViewerTitle
l:
"Просмотр"
"Viewer"
"Prohlížeč"
"Betrachter"
"Nézőke"
"Podgląd"
"Visor"
"Перегляд"

ViewerCannotOpenFile
"Ошибка открытия файла"
"Cannot open the file"
"Nelze otevřít soubor"
"Kann Datei nicht öffnen"
"A fájl nem nyitható meg"
"Nie mogę otworzyć pliku"
"No se puede abrir el archivo"
"Помилка відкриття файлу"

ViewerStatusCol
"Кол"
"Col"
"Sloupec"
"Spalte"
"Oszlop"
"Kolumna"
"Col"
"Кол"

ViewSearchTitle
l:
"Поиск"
"Search"
"Hledat"
"Durchsuchen"
"Keresés"
"Szukaj"
"Buscar"
"Пошук"

ViewSearchFor
"&Искать"
"&Search for"
"H&ledat"
"&Suchen nach"
"&Keresés:"
"&Znajdź"
"&Buscar por"
"&Шукати"

ViewSearchForText
"Искать &текст"
"Search for &text"
"Hledat &text"
"Suchen nach &Text"
"&Szöveg keresése"
"Szukaj &tekstu"
"Buscar cadena de &texto"
"Шукати &текст"

ViewSearchForHex
"Искать 16-ричный &код"
"Search for &hex"
"Hledat he&x"
"Suchen nach &Hex (xx xx ...)"
"&Hexa keresése"
"Szukaj &wartości szesnastkowych"
"Buscar cadena &hexadecimal"
"Шукати 16-річний &код"

ViewSearchCase
"&Учитывать регистр"
"&Case sensitive"
"&Rozlišovat velikost písmen"
"Gr&oß-/Kleinschreibung"
"&Nagy/kisbetű érzékeny"
"&Uwzględnij wielkość liter"
"Sensible min/ma&yúsculas"
"&Враховувати регістр"

ViewSearchWholeWords
"Только &целые слова"
"&Whole words"
"Celá &slova"
"Ganze &Wörter"
"Csak e&gész szavak"
"Tylko całe słowa"
"&Palabras completas"
"Тільки &цілі слова"

ViewSearchReverse
"Обратн&ый поиск"
"Re&verse search"
"&Zpětné hledání"
"Richtung um&kehren"
"&Visszafelé keres"
"Szukaj w &odwrotnym kierunku"
"Buscar al in&verso"
"Зворотн&ий пошук"

ViewSearchRegexp
"&Регулярные выражения"
"&Regular expressions"
upd:"&Regular expressions"
upd:"&Regular expressions"
upd:"&Regular expressions"
upd:"&Regular expressions"
"Expresiones &regulares"
"&Регулярні вирази"

ViewSearchSearch
"Искать"
"Search"
"Hledat"
"Suchen"
"Keres"
"&Szukaj"
"Buscar"
"Шукать"

ViewSearchCancel
"Отменить"
"Cancel"
"Storno"
"Abbrechen"
"Mégsem"
"&Anuluj"
"Cancelar"
"Скасувати"

ViewSearchingFor
l:
"Поиск"
"Searching for"
"Vyhledávám"
"Suche nach"
"Keresés:"
"Szukam"
"Buscando por"
"Пошук"

ViewSearchingHex
"Поиск байтов"
"Searching for bytes"
"Vyhledávám sekvenci bytů"
"Suche nach Bytes"
"Bájtok keresése:"
"Szukam bajtów"
"Buscando por bytes"
"Пошук байтів"

ViewSearchCannotFind
"Строка не найдена"
"Could not find the string"
"Nelze najít řetězec"
"Konnte Zeichenkette nicht finden"
"Nem találtam a szöveget:"
"Nie mogę odnaleźć ciągu znaków"
"No se puede encontrar la cadena"
"Строка не знайдена"

ViewSearchCannotFindHex
"Байты не найдены"
"Could not find the bytes"
"Nelze najít sekvenci bytů"
"Konnte Bytefolge nicht finden"
"Nem találtam a bájtokat:"
"Nie mogę odnaleźć bajtów"
"No se puede encontrar los bytes"
"Байти не знайдені"

ViewSearchFromBegin
"Продолжить поиск с начала документа?"
"Continue the search from the beginning of the document?"
"Pokračovat s hledáním od začátku dokumentu?"
"Mit Suche am Anfang des Dokuments fortfahren?"
"Folytassam a keresést a dokumentum elejétől?"
"Kontynuować wyszukiwanie od początku dokumentu?"
"Continuar búsqueda desde el comienzo del documento"
"Продовжити пошук з початку документа?"

ViewSearchFromEnd
"Продолжить поиск с конца документа?"
"Continue the search from the end of the document?"
"Pokračovat s hledáním od konce dokumentu?"
"Mit Suche am Ende des Dokuments fortfahren?"
"Folytassam a keresést a dokumentum végétől?"
"Kontynuować wyszukiwanie od końca dokumentu?"
"Continuar búsqueda desde el final del documento"
"Продовжити пошук з кінця документа?"

PrintTitle
l:
"Печать"
"Print"
"Tisk"
"Drucken"
"Nyomtatás"
"Drukuj"
"Imprimir"
"Друк"

PrintTo
"Печатать %ls на"
"Print %ls to"
"Vytisknout %ls na"
"Drucke %ls nach"
"%ls nyomtatása:"
"Drukuj %ls do"
"Imprimir %ls a"
"Друкувати %ls на"

PrintFilesTo
"Печатать %d файлов на"
"Print %d files to"
"Vytisknout %d souborů na"
"Drucke %d Dateien mit"
"%d fájl nyomtatása:"
"Drukuj %d pliki(ów) do"
"Imprimir %d archivos a"
"Друкувати %d файлів на"

PreparingForPrinting
"Подготовка файлов к печати"
"Preparing files for printing"
"Připravuji soubory pro tisk"
"Vorbereiten der Druckaufträge"
"Fájlok előkészítése nyomtatáshoz"
"Przygotowuję plik(i) do drukowania"
"Preparando archivos para imprimir"
"Підготовка файлів до друку"

CannotEnumeratePrinters
"Не удалось получить список доступных принтеров"
"Cannot enumerate available printers list"
upd:"Cannot enumerate available printers list"
upd:"Cannot enumerate available printers list"
"Az elérhető nyomtatók listája nem állítható össze"
upd:"Cannot enumerate available printers list"
"trabajos"
"Не вдалося отримати список доступних принтерів"

CannotOpenPrinter
"Не удалось открыть принтер"
"Cannot open printer"
"Nelze otevřít tiskárnu"
"Fehler beim öffnen des Druckers"
"Nyomtató nem elérhető"
"Nie mogę połączyć się z drukarką"
"No se puede abrir impresora"
"Не вдалося відкрити принтер"

CannotPrint
"Не удалось распечатать"
"Cannot print"
"Nelze tisknout"
"Fehler beim Drucken"
"Nem nyomtatható"
"Nie mogę drukować"
"No se puede imprimir"
"Не вдалося роздрукувати"

DescribeFiles
l:
"Описание файла"
"Describe file"
"Popiskový soubor"
"Beschreibung ändern"
"Fájlmegjegyzés"
"Opisz plik"
"Describir archivos"
"Опис файлу"

EnterDescription
"Введите описание для"
"Enter description for"
"Zadejte popisek"
"Beschreibung für"
upd:"Írja be megjegyzését:"
"Wprowadź opis"
"Entrar descripción de %ls"
"Введіть опис для"

ReadingDiz
l:
"Чтение описаний файлов"
"Reading file descriptions"
"Načítám popisky souboru"
"Lese Dateibeschreibungen"
"Fájlmegjegyzések olvasása"
"Odczytuję opisy plików"
"Leyendo descripción de archivos"
"Читання описів файлів"

CannotUpdateDiz
"Не удалось обновить описания файлов"
"Cannot update file descriptions"
"Nelze aktualizovat popisky souboru"
"Dateibeschreibungen konnten nicht aktualisiert werden."
"A fájlmegjegyzések nem frissíthetők"
"Nie moge aktualizować opisów plików"
"No se puede actualizar descripción de archivos"
"Не вдалося оновити опис файлів"

CannotUpdateRODiz
"Файл описаний защищён от записи"
"The description file is read only"
"Popiskový soubor má atribut Jen pro čtení"
"Die Beschreibungsdatei ist schreibgeschützt."
"A megjegyzésfájl csak olvasható"
"Opis jest plikiem tylko do odczytu"
"El archivo descripción es de sólo lectura"
"Файл опису захищений від запису"

CfgDizTitle
l:
"Описания файлов"
"File descriptions"
"Popisky souboru"
"Dateibeschreibungen"
"Fájl megjegyzésfájlok"
"Opisy plików"
"Descripción de archivos"
"Опис файлів"

CfgDizListNames
"Имена &списков описаний, разделённые запятыми:"
"Description &list names delimited with commas:"
"Seznam pop&isových souborů oddělených čárkami:"
"Beschreibungs&dateien, getrennt durch Komma:"
"Megjegyzés&fájlok nevei, vesszővel elválasztva:"
"Nazwy &plików z opisami oddzielone przecinkami:"
"Nombres de &listas de descripción delimitado con comas:"
"Імена &списків описів, розділені комами:"

CfgDizSetHidden
"Устанавливать &атрибут ""Скрытый"" на новые списки описаний"
"Set ""&Hidden"" attribute to new description lists"
"Novým souborům s popisy nastavit atribut ""&Skrytý"""
"Setze das '&Versteckt'-Attribut für neu angelegte Dateien"
"Az új megjegyzésfájl ""&rejtett"" attribútumú legyen"
"Ustaw atrybut ""&Ukryty"" dla nowych plików z opisami"
"Poner atributo ""&Oculto"" a las nuevas listas de descripción"
"Встановлювати &атрибут ""Прихований"" на нові списки описів"

CfgDizROUpdate
"Обновлять файл описаний с атрибутом ""Толь&ко для чтения"""
"Update &read only description file"
"Aktualizovat popisové soubory s atributem Jen pro čtení"
"Schreibgeschützte Dateien aktualisie&ren"
"&Csak olvasható megjegyzésfájlok frissítése"
"Aktualizuj plik opisu tylko do odczytu"
"Actualizar archivo descripción de sólo lectura"
"Оновлювати файл опису з атрибутом ""Тіль&ки для читання"""

CfgDizStartPos
"&Позиция новых описаний в строке"
"&Position of new descriptions in the string"
"&Pozice nových popisů v řetězci"
"&Position neuer Beschreibungen in der Zeichenkette"
"Új megjegyzéseknél a szöveg &kezdete"
"Pozy&cja nowych opisów w linii"
"&Posición de nueva descripciones en la cadena"
"&Позиція нових описів у рядку"

CfgDizNotUpdate
"&Не обновлять описания"
"Do &not update descriptions"
"&Neaktualizovat popisy"
"Beschreibungen &nie aktualisieren"
"N&e frissítse a megjegyzéseket"
"&Nie aktualizuj opisów"
"&No actualizar descripciones"
"&Не оновлювати описи"

CfgDizUpdateIfDisplayed
"&Обновлять, если они выводятся на экран"
"Update if &displayed"
"Aktualizovat, jestliže je &zobrazen"
"Aktualisieren &wenn angezeigt"
"Frissítsen, ha meg&jelenik"
"Aktualizuj jeśli &widoczne"
"Actualizar si es visualiza&do"
"&Оновлювати, якщо вони відображаються на екрані"

CfgDizAlwaysUpdate
"&Всегда обновлять"
"&Always update"
"&Vždy aktualizovat"
"Im&mer aktualisieren"
"&Mindig frissítsen"
"&Zawsze aktualizuj"
"&Actualizar siempre"
"&Завжди оновлювати"

CfgDizAnsiByDefault
"&Использовать кодовую страницу ANSI по умолчанию"
"Use ANS&I code page by default"
upd:"Automaticky otevírat soubory ve &WIN kódování"
upd:"Dateien standardmäßig mit Windows-Kod&ierung öffnen"
"Fájlok eredeti megnyitása ANS&I kódlappal"
"&Otwieraj pliki w kodowaniu Windows"
"Usar código ANS&I por defecto"
"&Використовувати кодову сторінку ANSI за замовчуванням"

CfgDizSaveInUTF
"Сохранять в UTF8"
"Save in UTF8"
upd:"Save in UTF8"
upd:"Save in UTF8"
upd:"Save in UTF8"
upd:"Save in UTF8"
"Guardar en UTF8"
"Зберігати в UTF8"

ReadingTitleFiles
l:
"Обновление панелей"
"Update of panels"
"Aktualizace panelů"
"Aktualisiere Panels"
"Panelek frissítése"
"Aktualizacja panelu"
"Actualizar paneles"
"Оновлення панелей"

ReadingFiles
"Чтение: %d файлов"
"Reading: %d files"
"Načítám: %d souborů"
"Lese: %d Dateien"
" %d fájl olvasása"
"Czytam: %d plików"
"Leyendo: %d archivos"
"Читання: %d файлів"

OperationNotCompleted
"Операция не завершена"
"Operation not completed"
"Operace není dokončena"
"Vorgang nicht abgeschlossen"
"A művelet félbeszakadt"
"Operacja nie doprowadzona do końca"
"Operación no completada"
"Операція не завершена"

EditPanelModes
l:
"Режимы панели"
"Edit panel modes"
"Editovat módy panelu"
"Anzeigemodi von Panels bearbeiten"
"Panel módok szerkesztése"
"Edytuj tryby wyświetlania paneli"
"Editar modo de paneles"
"Режими панелі"

EditPanelModesBrief
l:
"&Краткий режим"
"&Brief mode"
"&Stručný mód"
"&Kurz"
"&Rövid mód"
"&Skrótowy"
"&Breve     "
"&Короткий режим"

EditPanelModesMedium
"&Средний режим"
"&Medium mode"
"S&třední mód"
"&Mittel"
"&Közepes mód"
"Ś&redni"
"&Medio      "
"&Середний режим"

EditPanelModesFull
"&Полный режим"
"&Full mode"
"&Plný mód"
"&Voll"
"&Teljes mód"
"&Pełny"
"&Completo "
"&Повний режим"

EditPanelModesWide
"&Широкий режим"
"&Wide mode"
"Š&iroký mód"
"B&reitformat"
"&Széles mód"
"S&zeroki"
"&Amplio   "
"&Широкий режим"

EditPanelModesDetailed
"&Детальный режим"
"Detai&led mode"
"Detai&lní mód"
"Detai&lliert"
"Rés&zletes mód"
"Ze sz&czegółami"
"Detal&lado    "
"&Детальний режим"

EditPanelModesDiz
"&Описания"
"&Descriptions mode"
"P&opiskový mód"
"&Beschreibungen"
"&Fájlmegjegyzés mód"
"&Opisy"
"&Descripción      "
"&Описи"

EditPanelModesLongDiz
"Д&линные описания"
"Lon&g descriptions mode"
"&Mód dlouhých popisků"
"Lan&ge Beschreibungen"
"&Hosszú megjegyzés mód"
"&Długie opisy"
"Descripción lar&ga"
"Д&овгі описи"

EditPanelModesOwners
"Вл&адельцы файлов"
"File own&ers mode"
"Mód vlastníka so&uborů"
"B&esitzer"
"T&ulajdonos mód"
"&Właściciele"
"Du&eños de archivos"
"Вл&асники файлів"

EditPanelModesLinks
"Свя&зи файлов"
"Lin&ks mode"
"Lin&kový mód"
"Dateilin&ks"
"Li&nkek mód"
"Dowiąza&nia"
"En&laces    "
"Зв'я&зки файлів"

EditPanelModesAlternative
"Аль&тернативный полный режим"
"&Alternative full mode"
"&Alternativní plný mód"
"&Alternative Vollansicht"
"&Alternatív teljes mód"
"&Alternatywny"
"Alternativo com&pleto "
"Аль&тернативний повний режим"

EditPanelModeTypes
l:
"&Типы колонок"
"Column &types"
"&Typ sloupců"
"Spalten&typen"
"Oszlop&típusok"
"&Typy kolumn"
"&Tipos de columna"
"&Типи колонок"

EditPanelModeWidths
"&Ширина колонок"
"Column &widths"
"Šíř&ka sloupců"
"Spalten&breiten"
"Oszlop&szélességek"
"&Szerokości kolumn"
"Anc&ho de columna"
"&Ширина колонок"

EditPanelModeStatusTypes
"Типы колонок строки ст&атуса"
"St&atus line column types"
"T&yp sloupců stavového řádku"
"St&atuszeile Spaltentypen"
"Állapotsor oszloptíp&usok"
"Typy kolumn &linii statusu"
"Tipos de columnas líne&a de estado"
"Типи колонок рядка ст&атусу"

EditPanelModeStatusWidths
"Ширина колонок строки стат&уса"
"Status l&ine column widths"
"Šířka slo&upců stavového řádku"
"Statusze&ile Spaltenbreiten"
"Állapotsor &oszlopszélességek"
"Szerokości kolumn l&inii statusu"
"Ancho de columnas lí&nea de estado"
"Ширина колонок рядка стат&усу"

EditPanelModeFullscreen
"&Полноэкранный режим"
"&Fullscreen view"
"&Celoobrazovkový režim"
"&Vollbild"
"Tel&jes képernyős nézet"
"Widok &pełnoekranowy"
"&Vista pantalla completa"
"&Повноекранний режим"

EditPanelModeAlignExtensions
"&Выравнивать расширения файлов"
"Align file &extensions"
"Zarovnat příp&ony souborů"
"Datei&erweiterungen ausrichten"
"Fájlkiterjesztések &igazítása"
"W&yrównaj rozszerzenia plików"
"Alinear &extensiones de archivos"
"&Вирівнювати розширення файлів"

EditPanelModeAlignFolderExtensions
"Выравнивать расширения пап&ок"
"Align folder e&xtensions"
"Zarovnat přípony adre&sářů"
"Ordnerer&weiterungen ausrichten"
"Mappakiterjesztések i&gazítása"
"Wyrównaj rozszerzenia &folderów"
"Alinear e&xtensiones de directorios"
"Вирівнювати розширення те&к"

EditPanelModeFoldersUpperCase
"Показывать папки &заглавными буквами"
"Show folders in &uppercase"
"Zobrazit adresáře &velkými písmeny"
"Ordner in Großb&uchstaben zeigen"
"Mappák NAG&YBETŰVEL mutatva"
"Nazwy katalogów &WIELKIMI LITERAMI"
"Directorios en mayú&sculas"
"Показувати теки &великими літерами"

EditPanelModeFilesLowerCase
"Показывать файлы ст&рочными буквами"
"Show files in &lowercase"
"Zobrazit soubory ma&lými písmeny"
"Dateien in K&leinbuchstaben zeigen"
"Fájlok kis&betűvel mutatva"
"&Nazwy plików małymi literami"
"archivos en minúscu&las"
"Показувати файли ма&лими літерами"

EditPanelModeUpperToLowerCase
"Показывать имена файлов из заглавных букв &строчными буквами"
"Show uppercase file names in lower&case"
"Zobrazit velké znaky ve jménech souborů jako &malá písmena"
"G&roßgeschriebene Dateinamen in Kleinbuchstaben zeigen"
"NAGYBETŰS fájl&nevek kisbetűvel"
"Wyświetl NAZWY_PLIKÓW &jako nazwy_plików"
"archivos en mayúsculas mostrarlos con minús&culas"
"Показувати імена файлів із великих літер &маленькими літерами"

EditPanelReadHelp
" Нажмите F1, чтобы получить информацию по настройке "
" Read online help for instructions "
" Pro instrukce si přečtěte online nápovědu "
" Siehe Hilfe für Anweisungen "
" Tanácsokat a súgóban talál (F1) "
" Instrukcje zawarte są w pomocy podręcznej "
" Para instrucciones leer ayuda en línea "
" Натисніть F1, щоб отримати інформацію про налаштування "

SetFolderInfoTitle
l:
"Файлы информации о папках"
"Folder description files"
"Soubory s popiskem adresáře"
"Ordnerbeschreibungen"
"Mappa megjegyzésfájlok"
"Pliki opisu katalogu"
"Descripciones de directorio"
"Файли інформації про теки"

SetFolderInfoNames
"Введите имена файлов, разделённые запятыми (допускаются маски)"
"Enter file names delimited with commas (wildcards are allowed)"
"Zadejte jména souborů oddělených čárkami (značky jsou povoleny)"
"Dateiliste, getrennt mit Komma (Jokerzeichen möglich):"
"Fájlnevek, vesszővel elválasztva (joker is használható)"
"Nazwy plików oddzielone przecinkami (znaki ? i * dopuszczalne)"
"Ingrese nombre de archivo delimitado con comas (comodines permitidos)"
"Введіть імена файлів, розділені комами (допускаються маски)"

ScreensTitle
l:
"Экраны"
"Screens"
"Obrazovky"
"Seiten"
"Képernyők"
"Ekrany"
"Pant.  "
"Екрани"

ScreensPanels
"Панели"
"Panels"
"Panely"
"Panels"
"Panelek"
"Panele"
"Paneles"
"Панелі"

ScreensView
"Просмотр"
"View"
"Zobrazit"
"Betr."
"Nézőke"
"Podgląd"
"Ver"
"Перегляд"

ScreensEdit
"Редактор"
"Edit"
"Editovat"
"Bearb"
"Szerkesztő"
"Edycja"
"Editar"
"Редактор"

AskApplyCommandTitle
l:
"Применить команду"
"Apply command"
"Aplikovat příkaz"
"Befehl anwenden"
"Parancs végrehajtása"
"Zastosuj polecenie"
"Aplicar comando"
"Примінити команду"

AskApplyCommand
"Введите команду для обработки выбранных файлов"
"Enter command to process selected files"
"Zadejte příkaz pro zpracování vybraných souborů"
"Befehlszeile auf ausgewählte Dateien anwenden:"
"Írja be a kijelölt fájlok parancsát:"
"Wprowadź polecenie do przetworzenia wybranych plików"
"Ingrese comando para procesar archivos seleccionados"
"Введіть команду для обробки вибраних файлів"

PluginConfigTitle
l:
"Конфигурация модулей"
"Plugins configuration"
"Nastavení Pluginů"
"Konfiguration von Plugins"
"Plugin beállítások"
"Konfiguracja pluginów"
"Configuración de plugins"
"Конфігурація модулів"

PluginCommandsMenuTitle
"Команды внешних модулей"
"Plugin commands"
"Příkazy pluginů"
"Pluginbefehle"
"Plugin parancsok"
"Dostępne pluginy"
"Comandos de plugins"
"Команди зовнішніх модулів"

PreparingList
l:
"Создание списка файлов"
"Preparing files list"
"Připravuji seznam souborů"
"Dateiliste wird vorbereitet"
"Fájllista elkészítése"
"Przygotowuję listę plików"
"Preparando lista de archivos"
"Створення списку файлів"

LangTitle
l:
"Основной язык"
"Main language"
"Hlavní jazyk"
"Hauptsprache"
"A program nyelve"
"Język programu"
"Idioma principal"
"Основна мова"

HelpLangTitle
"Язык помощи"
"Help language"
"Jazyk nápovědy"
"Sprache der Hilfedatei"
"A súgó nyelve"
"Język pomocy"
"Idioma de ayuda"
"Мова допомоги"

DefineMacroTitle
l:
"Задание макрокоманды"
"Define macro"
"Definovat makro"
"Definiere Makro"
"Makró gyorsbillentyű"
"Zdefiniuj makro"
"Definir macro"
"Завдання макрокоманди"

DefineMacro
"Нажмите желаемую клавишу"
"Press the desired key"
"Stiskněte požadovanou klávesu"
"Tastenkombination:"
"Nyomja le a billentyűt"
"Naciśnij żądany klawisz"
"Pulse la tecla deseada"
"Натисніть бажану клавішу"

MacroReDefinedKey
"Макроклавиша '%ls' уже определена."
"Macro key '%ls' already defined."
"Klávesa makra '%ls' již je definována."
"Makro '%ls' bereits definiert."
""%ls" makróbillentyű foglalt"
"Skrót '%ls' jest już zdefiniowany."
"Macro '%ls' ya está definido. Secuencia:"
"Макроклавіша '%ls' вже визначена."

MacroDeleteAssign
"Макроклавиша '%ls' не активна."
"Macro key '%ls' is not active."
"Klávesa makra '%ls' není aktivní."
"Makro '%ls' nicht aktiv."
""%ls" makróbillentyű nem él"
"Skrót '%ls' jest nieaktywny."
"Macro '%ls' no está activo. Secuencia:"
"Макроклавіша '%ls' не активна."

MacroDeleteKey
"Макроклавиша '%ls' будет удалена."
"Macro key '%ls' will be removed."
"Klávesa makra '%ls' bude odstraněna."
"Makro '%ls' wird entfernt und ersetzt:"
""%ls" makróbillentyű törlődik"
"Skrót '%ls' zostanie usunięty."
"Macro '%ls' será removido. Secuencia:"
"Макроклавіша '%ls' буде видалена."

MacroCommonReDefinedKey
"Общая макроклавиша '%ls' уже определена."
"Common macro key '%ls' already defined."
"Klávesa pro běžné makro '%ls' již je definována."
"Gemeinsames Makro '%ls' bereits definiert."
""%ls" közös makróbill. foglalt"
"Skrót '%ls' jest już zdefiniowany."
"Tecla de macro '%ls' ya ha sido definida."
"Спільна макроклавіша '%ls' вже визначена."

MacroCommonDeleteAssign
"Общая макроклавиша '%ls' не активна."
"Common macro key '%ls' is not active."
"Klávesa pro běžné makro '%ls' není aktivní."
"Gemeinsames Makro '%ls' nicht aktiv."
""%ls" közös makróbill. nem él"
"Skrót '%ls' jest nieaktywny."
"Tecla de macro '%ls' no está activada."
"Спільна макроклавіша '%ls' не активна."

MacroCommonDeleteKey
"Общая макроклавиша '%ls' будет удалена."
"Common macro key '%ls' will be removed."
"Klávesa pro běžné makro '%ls' bude odstraněna."
"Gemeinsames Makro '%ls' wird entfernt und ersetzt:"
""%ls" közös makróbill. törlődik"
"Skrót '%ls' zostanie usunięty."
"Tecla de macro '%ls' será removida."
"Спільна макроклавіша '%ls' буде видалена."

MacroSequence
"Последовательность:"
"Sequence:"
"Posloupnost:"
"Sequenz:"
"Szekvencia:"
"Sekwencja:"
"Secuencia:"
"Послідовність:"

MacroReDefinedKey2
"Переопределить?"
"Redefine?"
"Předefinovat?"
"Neu definieren?"
"Újradefiniálja?"
"Zdefiniować powtórnie?"
"Redefinir?"
"Перевизначити?"

MacroDeleteKey2
"Удалить?"
"Delete?"
"Odstranit?"
"Löschen?"
"Törli?"
"Usunąć?"
"Borrar?"
"Видалити?"

MacroDisDisabledKey
"(макроклавиша не активна)"
"(macro key is not active)"
"(klávesa makra není aktivní)"
"(Makro inaktiv)"
"(makróbill. nem él)"
"(skrót jest nieaktywny)"
"(macro no está activo)"
"(макроклавиша не активна)"

MacroDisOverwrite
"Переопределить"
"Overwrite"
"Přepsat"
"Überschreiben"
"Felülírás"
"Zastąpić"
"Sobrescribir"
"Перевизначити"

MacroDisAnotherKey
"Изменить клавишу"
"Try another key"
"Zkusit jinou klávesu"
"Neue Kombination"
"Adjon meg másik billentyűt"
"Spróbuj inny klawisz"
"Intente otra tecla"
"Змінити клавішу"

MacroSettingsTitle
l:
"Параметры макрокоманды для '%ls'"
"Macro settings for '%ls'"
"Nastavení makra pro '%ls'"
"Einstellungen für Makro '%ls'"
""%ls" makró beállításai"
"Ustawienia makra dla '%ls'"
"Configurar macro para '%ls'"
"Параметри макрокоманди для '%ls'"

MacroSettingsEnableOutput
"Разрешить во время &выполнения вывод на экран"
"Allo&w screen output while executing macro"
"Povolit &výstup na obrazovku dokud se provádí makro"
"Bildschirmausgabe &während Makro abläuft"
"Képernyő&kimenet a makró futása közben"
"&Wyłącz zapis na ekran podczas wykonywania makra"
"Permitir salida pantalla mientras se ejecut&an los macros"
"Дозволити під час виконання &виведення на екран"

MacroSettingsRunAfterStart
"В&ыполнять после запуска FAR"
"Execute after FAR &start"
"&Spustit po spuštění FARu"
"Ausführen beim &Starten von FAR"
"Végrehajtás a FAR &indítása után"
"Wykonaj po &starcie FAR-a"
"Ejecutar luego de &iniciar FAR"
"В&иконати після запуску FAR"

MacroSettingsActivePanel
"&Активная панель"
"&Active panel"
"&Aktivní panel"
"&Aktives Panel"
"&Aktív panel"
"Panel &aktywny"
"Panel &activo"
"&Активна панель"

MacroSettingsPassivePanel
"&Пассивная панель"
"&Passive panel"
"Pa&sivní panel"
"&Passives Panel"
"Passzí&v panel"
"Panel &pasywny"
"Panel &pasivo"
"&Пассивна панель"

MacroSettingsPluginPanel
"На панели пла&гина"
"P&lugin panel"
"Panel p&luginů"
"P&lugin Panel"
"Ha &plugin panel"
"Panel p&luginów"
"Panel de p&lugins"
"На панелі пла&гіна"

MacroSettingsFolders
"Выполнять для папо&к"
"Execute for &folders"
"Spustit pro ad&resáře"
"Auf Ordnern aus&führen"
"Ha &mappa"
"Wykonaj dla &folderów"
"Ejecutar para &directorios"
"Виконувати для те&к"

MacroSettingsSelectionPresent
"&Отмечены файлы"
"Se&lection present"
"E&xistující výběr"
"Auswah&l vorhanden"
"Ha van ki&jelölés"
"Zaznaczenie &obecne"
"Selección presente"
"&Визначено файли"

MacroSettingsCommandLine
"Пустая командная &строка"
"Empty &command line"
"Prázdný pří&kazový řádek"
"Leere Befehls&zeile"
"Ha &üres a parancssor"
"Pusta &linia poleceń"
"Vaciar línea de &comandos"
"Порожній командний &рядок"

MacroSettingsSelectionBlockPresent
"Отмечен б&лок"
"Selection &block present"
"Existující blok výběr&u"
"Mar&kierter Text vorhanden"
"Ha van kijelölt &blokk"
"Obecny &blok zaznaczenia"
"Selección de bloque presente"
"Відзначений б&лок"

MacroOutputFormatForHelpSz
l:
l:// for <!Macro:Vars!> and <!Macro:Consts!>, count formats = 1
"„%ls”"
"„%ls”"
"„%ls”"
"„%ls”"
"„%ls”"
"„%ls”"
"%ls"
"„%ls”"

MacroOutputFormatForHelpDWord
l:// for <!Macro:Vars!> and <!Macro:Consts!>, count formats = 2
"%d / 0x%X"
"%d / 0x%X"
"%d / 0x%X"
"%d / 0x%X"
"%d / 0x%X"
"%d / 0x%X"
"%d / 0x%X"
"%d / 0x%X"

MacroOutputFormatForHelpQWord
l:// for <!Macro:Vars!> and <!Macro:Consts!>, count formats = 2
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"

MacroOutputFormatForHelpDouble
l:// for <!Macro:Vars!> and <!Macro:Consts!>, count formats = 2
"%g"
"%g"
"%g"
"%g"
"%g"
"%g"
"%g"
"%g"

MacroPErrorTitle
"Ошибка при разборе макроса"
"Error parsing macro"
upd:"Error parsing macro"
upd:"Error parsing macro"
upd:"Error parsing macro"
upd:"Error parsing macro"
"Error parsing macro"
"Помилка при розборі макросу"

MacroPErrorPosition
"Строка %d, позиция %d"
"Line %d, Pos %d"
upd:"Line %d, Pos %d"
upd:"Line %d, Pos %d"
upd:"Line %d, Pos %d"
upd:"Line %d, Pos %d"
"Línea %d, Pos %d"
"Рядок %d, позиція %d"

MacroPErrUnrecognized_keyword
l:
"Неизвестное ключевое слово '%ls'"
"Unrecognized keyword '%ls'"
"Neznámé klíčové slovo '%ls'"
"Unbekanntes Schlüsselwort '%ls'"
"Ismeretlen kulcsszó "%ls""
"Nie rozpoznano słowa kluczowego '%ls'"
"Unrecognized keyword '%ls'"
"Невідоме ключове слово '%ls'"

MacroPErrUnrecognized_function
"Неизвестная функция '%ls'"
"Unrecognized function '%ls'"
"Neznámá funkce '%ls'"
"Unbekannte Funktion '%ls'"
"Ismeretlen funkció "%ls""
"Nie rozpoznano funkcji'%ls'"
"Unrecognized function '%ls'"
"Невідома функція '%ls'"

MacroPErrFuncParam
"Неверное количество параметров у функции '%ls'"
"Incorrect number of arguments for function '%ls'"
upd:"Incorrect number of arguments for function '%ls'"
upd:"Incorrect number of arguments for function '%ls'"
"'%ls' funkció paramétereinek száma helytelen"
upd:"Incorrect number of arguments for function '%ls'"
"Incorrect number of arguments for function '%ls'"
"Неправильна кількість параметрів у функції '%ls'"

MacroPErrNot_expected_ELSE
"Неожиданное появление $Else"
"Unexpected $Else"
"Neočekávané $Else"
"Unerwartetes $Else"
"Váratlan $Else"
"$Else w niewłaściwym miejscu"
"Unexpected $Else"
"Несподівана поява $Else"

MacroPErrNot_expected_END
"Неожиданное появление $End"
"Unexpected $End"
"Neočekávané $End"
"Unerwartetes $End"
"Váratlan $End"
"$End w niewłaściwym miejscu"
"Unexpected $End"
"Несподівана поява $End"

MacroPErrUnexpected_EOS
"Неожиданный конец строки"
"Unexpected end of source string"
"Neočekávaný konec zdrojového řetězce"
"Unerwartetes Ende der Zeichenkette"
"Váratlanul vége a forrássztringnek"
"Nie spodziewano się końca ciągu"
"Unexpected end of source string"
"Несподіваний кінець рядка"

MacroPErrExpected
"Ожидается '%ls'"
"Expected '%ls'"
"Očekávané '%ls'"
"Erwartet '%ls'"
"Várható "%ls""
"Oczekiwano '%ls'"
"Expected '%ls'"
"Очікується '%ls'"

MacroPErrBad_Hex_Control_Char
"Неизвестный шестнадцатеричный управляющий символ"
"Bad Hex Control Char"
"Chybný kontrolní znak Hex"
"Fehlerhaftes Hexzeichen"
"Rossz hexa vezérlőkarakter"
"Błędny szesnastkowy znak sterujący"
"Bad Hex Control Char"
"Невідомий шістнадцятковий керуючий символ"

MacroPErrBad_Control_Char
"Неправильный управляющий символ"
"Bad Control Char"
"Špatný kontrolní znak"
"Fehlerhaftes Kontrollzeichen"
"Rossz vezérlőkarakter"
"Błędny znak sterujący"
"Bad Control Char"
"Неправильний керуючий символ"

MacroPErrVar_Expected
"Переменная '%ls' не найдена"
"Variable Expected '%ls'"
"Očekávaná proměnná '%ls'"
"Variable erwartet '%ls'"
""%ls" várható változó"
"Oczekiwano zmiennej '%ls'"
"Variable Expected '%ls'"
"Змінна '%ls' не знайдена"

MacroPErrExpr_Expected
"Ошибка синтаксиса"
"Expression Expected"
"Očekávaný výraz"
"Ausdruck erwartet"
"Szintaktikai hiba"
"Oczekiwano wyrażenia"
"Expression Expected"
"Помилка синтаксису"

MacroPErr_ZeroLengthMacro
"Пустая макропоследовательность"
"Zero-length macro"
upd:"Zero-length macro"
upd:"Zero-length macro"
"Nulla hosszúságú makró"
upd:"Zero-length macro"
"macro de longitud 0"
"Порожня макропослідовність"

MacroPErrIntParserError
"Внутренняя ошибка парсера"
"Internal parser error"
upd:"Internal parser error"
upd:"Internal parser error"
upd:"Internal parser error"
upd:"Internal parser error"
"Macro parsing error"
"Внутрішня помилка парсера"

MacroPErrContinueOutsideTheLoop
"Оператор $Continue вне цикла"
upd:"$Continue outside the loop"
upd:"$Continue outside the loop"
upd:"$Continue outside the loop"
upd:"$Continue outside the loop"
upd:"$Continue outside the loop"
"$Continuar por fuera del loop"
"Оператор $Continue поза циклом"

CannotSaveFile
l:
"Ошибка сохранения файла"
"Cannot save file"
"Nelze uložit soubor"
"Kann Datei nicht speichern"
"A fájl nem menthető"
"Nie mogę zapisać pliku"
"No se puede guardar archivo"
"Помилка збереження файлу"

TextSavedToTemp
"Отредактированный текст записан в"
"Edited text is stored in"
"Editovaný text je uložen v"
"Editierter Text ist gespeichert in"
"A szerkesztett szöveg elmentve:"
"Edytowany tekst został zachowany w"
"Texto editado es almacenado en"
"Відредагований текст записано в"

MonthJan
l:
"Янв"
"Jan"
"Led"
"Jan"
"Jan"
"Sty"
"Ene"
"Січ"

MonthFeb
"Фев"
"Feb"
"Úno"
"Feb"
"Feb"
"Lut"
"Feb"
"Лют"

MonthMar
"Мар"
"Mar"
"Bře"
"Mär"
"Már"
"Mar"
"Mar"
"Бер"

MonthApr
"Апр"
"Apr"
"Dub"
"Apr"
"Ápr"
"Kwi"
"Abr"
"Кві"

MonthMay
"Май"
"May"
"Kvě"
"Mai"
"Máj"
"Maj"
"May"
"Тра"

MonthJun
"Июн"
"Jun"
"Čer"
"Jun"
"Jún"
"Cze"
"Jun"
"Чрв"

MonthJul
"Июл"
"Jul"
"Čec"
"Jul"
"Júl"
"Lip"
"Jul"
"Лип"

MonthAug
"Авг"
"Aug"
"Srp"
"Aug"
"Aug"
"Sie"
"Ago"
"Срп"

MonthSep
"Сен"
"Sep"
"Zář"
"Sep"
"Sze"
"Wrz"
"Sep"
"Вер"

MonthOct
"Окт"
"Oct"
"Říj"
"Okt"
"Okt"
"Paź"
"Oct"
"Жов"

MonthNov
"Ноя"
"Nov"
"Lis"
"Nov"
"Nov"
"Lis"
"Nov"
"Лст"

MonthDec
"Дек"
"Dec"
"Pro"
"Dez"
"Dec"
"Gru"
"Dic"
"Гру"

HelpHotKey
"Введите горячую клавишу (букву или цифру)"
"Enter hot key (letter or digit)"
"Zadejte horkou klávesu (písmeno nebo číslici)"
"Buchstabe oder Ziffer:"
"Nyomja le a billentyűt (betű vagy szám)"
"Podaj klawisz skrótu (litera lub cyfra)"
"Entrar tecla rápida (letra o dígito)"
"Введіть гарячу клавішу (літеру або цифру)"

PluginHotKeyBottom
"F4 - задать горячую клавишу"
"F4 - set hot key"
"F4 - nastavení horké klávesy"
"Kurztaste setzen: F4"
"F4 - gyorsbillentyű hozzárendelés"
"F4 - ustaw klawisz skrótu"
"F4 - asignar tecla rápida"
"F4 - встановити гарячу клавішу"

PluginHotKeyTitle
l:
"Назначение горячей клавиши"
"Assign plugin hot key"
"Přidělit horkou klávesu pluginu"
"Dem Plugin eine Kurztaste zuweisen"
"Plugin gyorsbillentyű hozzárendelés"
"Przypisz klawisz skrótu do pluginu"
"Asignar tecla rápida a plugin"
"Призначення гарячої клавіші"

LocationHotKeyTitle
l:
"Назначение горячей клавиши"
"Assign location hot key"
upd:"Přidělit horkou klávesu ???"
upd:"Dem ??? eine Kurztaste zuweisen"
upd:"??? gyorsbillentyű hozzárendelés"
upd:"Przypisz klawisz skrótu do ???"
upd:"Asignar tecla rápida a ???"
"Призначення гарячої клавіші"

LocationHotKey
"Введите горячую клавишу (букву или цифру)"
"Enter hot key (letter or digit)"
"Zadejte horkou klávesu (písmeno nebo číslici)"
"Buchstabe oder Ziffer:"
"Nyomja le a billentyűt (betű vagy szám)"
"Podaj klawisz skrótu (litera lub cyfra)"
"Entrar tecla rápida (letra o dígito)"
"Введіть гарячу клавішу (літеру або цифру)"

RightCtrl
l:
"ПравыйCtrl"
"RightCtrl"
"PravýCtrl"
"StrgRechts"
"JobbCtrl"
"PrawyCtrl"
"CtrlDrcho"
"ПравиййCtrl"

ViewerGoTo
l:
"Перейти"
"Go to"
"Jdi na"
"Gehe zu"
"Ugrás"
"Idź do"
"Ir a:"
"Перейти"

GoToPercent
"&Процент"
"&Percent"
"&Procent"
"&Prozent"
"&Százalékban"
"&Procent"
"&Porcentaje"
"&Процент"

GoToHex
"16-ричное &смещение"
"&Hex offset"
"&Hex offset"
"Position (&Hex)"
"&Hexában"
"Pozycja (&szesnastkowo)"
"Dirección &Hexa"
"16-річне &зміщення"

GoToDecimal
"10-ичное с&мещение"
"&Decimal offset"
"&Desítkový offset"
"Position (&dezimal)"
"&Decimálisan"
"Pozycja (&dziesiętnie)"
"Dirección &Decimal"
"10-ічне з&міщення"

ExcTrappedException
"Исключительная ситуация"
"Exception occurred"
"Vyskytla se výjimka"
"Ausnahmefehler aufgetreten"
"Kivétel történt"
"Wystąpił wyjątek"
"Error de excepción"
"Виняткова ситуація"

ExcRAccess
"Нарушение доступа (чтение из 0x%p)"
"Access violation (read from 0x%p)"
"Neplatná adresa (čtení z 0x%p)"
"Zugriffsverletzung (Lesen von 0x%p)"
"Hozzáférési jogsértés (olvasás 0x%p címről)"
"Błąd dostępu (odczyt z 0x%p)"
"Violación de acceso (leído desde 0x%p)"
"Порушення доступу (читання 0x%p)"

ExcWAccess
"Нарушение доступа (запись в 0x%p)"
"Access violation (write to 0x%p)"
"Neplatná adresa (zápis na 0x%p)"
"Zugriffsverletzung (Schreiben nach 0x%p)"
"Hozzáférési jogsértés (írás 0x%p címre)"
"Błąd dostępu (zapis do 0x%p)"
"Violación de acceso (escrito a 0x%p)"
"Порушення доступу (запис 0x%p)"

ExcEAccess
"Нарушение доступа (исполнение кода из 0x%p)"
"Access violation (execute at 0x%p)"
"Neplatná adresa (spuštění na 0x%p)"
"Zugriffsverletzung (Ausführen bei 0x%p)"
"Hozzáférési jogsértés (végrehajtás 0x%p címen)"
"Błąd dostępu (wykonanie w 0x%p)"
"Violación de acceso (ejecutado en 0x%p)"
"Порушення доступу (виконання коду з 0x%p)"

ExcOutOfBounds
"Попытка доступа к элементу за границами массива"
"Array out of bounds"
"Pole mimo hranice"
"Arrayüberlauf"
"A tömb határait meghaladta"
"Przekroczenie granic tabeli"
"Array out of bounds"
"Спроба доступу до елемента за межами масиву"

ExcDivideByZero
"Деление на нуль"
"Divide by zero"
"Dělení nulou"
"Division durch Null"
"Nullával osztás"
"Dzielenie przez zero"
"División por cero"
"Поділ на нуль"

ExcStackOverflow
"Переполнение стека"
"Stack Overflow"
"Přetečení zásobníku"
"Stacküberlauf"
"Verem túlcsordulás"
"Przepełnienie stosu"
"Stack overflow"
"Переповнення стека"

ExcBreakPoint
"Точка останова"
"Breakpoint exception"
"Výjimka přerušení"
"Breakpoint exception"
"Törésponti kivétel"
"Wyjątek punktu przerwania"
"Excepción de punto de quiebre"
"Точка зупинки"

ExcFloatDivideByZero
"Деление на нуль при операции с плавающей точкой"
"Floating-point divide by zero"
"Dělení nulou v pohyblivé čárce"
"Fließkomma-Division durch Null"
"Lebegőpontos szám osztása nullával"
"Błąd zmiennoprzecinkowego dzielenia przez zero"
"Punto flotante dividido por cero"
"Поділ на нуль при операції з плаваючою точкою"

ExcFloatOverflow
"Переполнение при операции с плавающей точкой"
"Floating point operation overflow"
"Přetečení při operaci v pohyblivé čárce"
"Fließkomma-Operation verursachte Überlauf"
"Lebegőpontos művelet túlcsordulás"
"Przepełnienie przy operacji zmiennnoprzecinkowej"
"Operación de punto flotante desbordada"
"Переповнення при операції з плаваючою точкою"

ExcFloatStackOverflow
"Стек регистров сопроцессора полон или пуст"
"Floating point stack empty or full"
"Prázdný nebo plný zásobník v pohyblivé čárce"
"Fließkomma-Stack leer bzw. voll"
"Lebegőpont verem üres vagy megtelt"
"Stos operacji zmiennoprzecinkowych pusty lub pełny"
"Pila de punto flotante vacía o llena"
"Стек регістрів співпроцесора повний або порожній"

ExcFloatUnderflow
"Потеря точности при операции с плавающей точкой"
"Floating point operation underflow"
"Podtečení při operaci v pohyblivé čárce"
"Fließkomma-Operation verursachte Underflow"
"Lebegőpontos művelet alulcsordulás"
"Błąd niedomiaru przy operacji zmiennoprzecinkowej"
"Operación de punto flotante underflow"
"Втрата точності при операції з плаваючою точкою"

ExcBadInstruction
"Недопустимая инструкция"
"Illegal instruction"
"Neplatná instrukce"
"Ungültige Anweisung"
"Érvénytelen utasítás"
"Błędna instrukcja"
"Instrucción ilegal"
"Неприпустима інструкція"

ExcDatatypeMisalignment
"Попытка доступа к невыравненным данным"
"Alignment fault"
"Chyba zarovnání"
"Fehler bei Datenausrichtung"
"Adattípus illesztési hiba"
"Błąd ustawienia"
"Falta de alineamiento"
"Спроба доступу до невирівняних даних"

ExcUnknown
"Неизвестное исключение"
"Unknown exception"
"Neznámá výjimka"
"Unbekannte Ausnahme"
"Ismeretlen kivétel"
"Nieznany wyjątek"
"Excepción desconocida"
"Невідомий виняток"

ExcException
"Исключение:"
"Exception:"
upd:"Exception:"
upd:"Exception:"
upd:"Exception:"
upd:"Exception:"
"Excepción:"
"Виняток:"

ExcAddress
"Адрес:"
"Address:"
upd:"Address:"
upd:"Address:"
upd:"Address:"
upd:"Address:"
"Dirección:"
"Адреса:"

ExcFunction
"Функция:"
"Function:"
upd:"Function:"
upd:"Function:"
upd:"Function:"
upd:"Function:"
"Función:"
"Функція:"

ExcModule
"Модуль:"
"Module:"
upd:"Module:"
upd:"Module:"
upd:"Module:"
upd:"Module:"
"Módulo:"
"Модуль:"

ExcTerminate
"Завершить FAR"
"Terminate FAR"
upd:"Terminate FAR"
upd:"Terminate FAR"
upd:"Terminate FAR"
upd:"Terminate FAR"
"FAR se dará por terminado"
"Завершити FAR"

ExcUnload
"Выгрузить плагин"
"Unload plugin"
upd:"Unload plugin"
upd:"Unload plugin"
upd:"Unload plugin "
upd:"Unload plugin"
"El plugin será descargado"
"Вивантажити плагін"

ExcDebugger
"Отладка"
"Debug"
upd:"Debug"
upd:"Debug"
upd:"Debug"
upd:"Debug"
"Depurador"
"Налагодження"

NetUserName
l:
"Имя пользователя"
"User name"
"Jméno uživatele"
"Benutzername"
"Felhasználói név"
"Nazwa użytkownika"
"Nombre de usuario"
"Ім'я користувача"

NetUserPassword
"Пароль пользователя"
"User password"
"Heslo uživatele"
"Benutzerpasswort"
"Felhasználói jelszó"
"Hasło użytkownika"
"Clave de usuario"
"Пароль користувача"

ReadFolderError
l:
"Не удаётся прочесть содержимое папки"
"Cannot read folder contents"
"Nelze načíst obsah adresáře"
"Kann Ordnerinhalt nicht lesen"
"A mappa tartalma nem olvasható"
"Nie udało się odczytać zawartości folderu"
"No se puede leer contenidos de directorios"
"Не вдається прочитати вміст теки"

PlgBadVers
l:
"Этот модуль требует FAR более высокой версии"
"This plugin requires higher FAR version"
"Tento plugin vyžaduje vyšší verzi FARu"
"Das Plugin benötigt eine aktuellere Version von FAR"
"A pluginhez újabb FAR verzió kell"
"Do uruchomienia pluginu wymagana jest wyższa wersja FAR-a"
"Este plugin requiere versión más actual de FAR"
"Цей модуль потребує FAR більш високої версії"

PlgRequired
"Требуется версия FAR - %d.%d.%d."
"Required FAR version is %d.%d.%d."
"Požadovaná verze FARu je %d.%d.%d."
"Benötigte FAR-Version ist %d.%d.%d."
"A szükséges FAR verzió: %d.%d.%d."
"Wymagana wersja FAR-a to %d.%d.%d."
"Requiere la versión FAR %d.%d.%d."
"Потрібна версія FAR - %d.%d.%d."

PlgRequired2
"Текущая версия FAR - %d.%d.%d."
"Current FAR version is %d.%d.%d."
"Nynější verze FARu je %d.%d.%d."
"Aktuelle FAR-Version ist %d.%d.%d."
"A jelenlegi FAR verzió: %d.%d.%d."
"Bieżąca wersja FAR-a: %d.%d.%d."
"Versión actual de FAR es %d.%d.%d"
"Поточна версія FAR - %d.%d.%d."

PlgLoadPluginError
"Ошибка при загрузке плагина"
"Error loading plugin module"
"Chyba při nahrávání zásuvného modulu"
"Fehler beim Laden des Pluginmoduls"
"Plugin betöltési hiba"
"Błąd ładowania modułu plugina"
"Error cargando módulo plugin"
"Помилка під час завантаження плагіна"

CheckBox2State
l:
"?"
"?"
"?"
"?"
"?"
"?"
"?"
"?"

HelpTitle
l:
"Помощь"
"Help"
"Nápověda"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

HelpActivatorURL
"Эта ссылка запускает внешнее приложение:"
"This reference starts the external application:"
"Tento odkaz spouští externí aplikaci:"
"Diese Referenz startet folgendes externes Programm:"
"A hivatkozás által indított program:"
"To wywołanie uruchomi aplikację zewnętrzną:"
"Esta referencia inicia la aplicación externa:"
"Це посилання запускає зовнішню програму:"

HelpActivatorFormat
"с параметром:"
"with parameter:"
"s parametrem:"
"mit Parameter:"
"Paraméterei:"
"z parametrem:"
"con parámetro:"
"з параметром:"

HelpActivatorQ
"Желаете запустить?"
"Do you wish to start it?"
"Přejete si ji spustit?"
"Wollen Sie jetzt starten?"
"El akarja indítani?"
"Czy chcesz ją uruchomić?"
"Desea comenzar la aplicación?"
"Хочете запустити?"

CannotOpenHelp
"Ошибка открытия файла"
"Cannot open the file"
"Nelze otevřít soubor"
"Kann Datei nicht öffnen"
"A fájl nem nyitható meg"
"Nie można otworzyć pliku"
"No se puede abrir el archivo"
"Помилка відкриття файлу"

HelpTopicNotFound
"Не найден запрошенный раздел помощи:"
"Requested help topic not found:"
"požadované téma nápovědy nebylo nalezeno"
"Angefordertes Hilfethema wurde nicht gefunden:"
"A kívánt súgó témakör nem található:"
"Nie znaleziono tematu pomocy:"
"Tema de ayuda requerido no encontrado"
"Не знайдено запитаний розділ допомоги:"

PluginsHelpTitle
l:
"Внешние модули"
"Plugins help"
"Nápověda Pluginů"
"Pluginhilfe"
"Pluginek súgói"
"Pomoc dla pluginów"
"Ayuda plugins"
"Зовнішні модулі"

DocumentsHelpTitle
"Документы"
"Documents help"
"Nápověda Dokumentů"
"Dokumentenhilfe"
"Dokumentumok súgói"
"Pomoc dla dokumentów"
"Ayuda documentos"
"Документи"

HelpSearchTitle
l:
"Поиск"
"Search"
"Hledání"
"Suchen"
"Keresés"
"Szukaj"
"Buscar"
"Пошук"

HelpSearchingFor
"Поиск для"
"Searching for"
"Hledání"
"Suche nach"
"Keresés:"
"Znajdź"
"Buscando por"
"Пошук для"

HelpSearchCannotFind
"Строка не найдена"
"Could not find the string"
"Nelze najít řetězec"
"Konnte Zeichenkette nicht finden"
"A szöveg nem található:"
"Nie mogę odnaleźć ciągu znaków"
"No se encontró la cadena"
"Рядок не знайдено"

HelpF1
l:
l:// Help KeyBar F1-12
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

HelpF2
""
""
""
""
""
""
""
""

HelpF3
""
""
""
""
""
""
""
""

HelpF4
""
""
""
""
""
""
""
""

HelpF5
"Размер"
"Zoom"
"Zoom"
"Vergr."
"Nagyít"
"Powiększ"
"Zoom"
"Розмір"

HelpF6
""
""
""
""
""
""
""
""

HelpF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Szukaj"
"Buscar"
"Пошук"

HelpF8
""
""
""
""
""
""
""
""

HelpF9
""
""
""
""
""
""
""
""

HelpF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

HelpF11
""
""
""
""
""
""
""
""

HelpF12
""
""
""
""
""
""
""
""

HelpShiftF1
l:
l:// Help KeyBar Shift-F1-12
"Содерж"
"Index"
"Index"
"Index"
"Tartlm"
"Indeks"
"Indice"
"Зміст"

HelpShiftF2
"Плагин"
"Plugin"
"Plugin"
"Plugin"
"PlgSúg"
"Plugin"
"Plugin"
"Плагін"

HelpShiftF3
"Докум"
"Docums"
"Dokume"
"Dokume"
"DokSúg"
"Dokumenty"
"Docums"
"Докум"

HelpShiftF4
""
""
""
""
""
""
""
""

HelpShiftF5
""
""
""
""
""
""
""
""

HelpShiftF6
""
""
""
""
""
""
""
""

HelpShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"Tovább"
"Nast."
"Próxim"
"Далі"

HelpShiftF8
""
""
""
""
""
""
""
""

HelpShiftF9
""
""
""
""
""
""
""
""

HelpShiftF10
""
""
""
""
""
""
""
""

HelpShiftF11
""
""
""
""
""
""
""
""

HelpShiftF12
""
""
""
""
""
""
""
""

HelpAltF1
l:
l:// Help KeyBar Alt-F1-12
"Пред."
"Prev"
"Předch"
"Letzt"
"Vissza"
"Poprz."
"Previo"
"Попрд."

HelpAltF2
""
""
""
""
""
""
""
""

HelpAltF3
""
""
""
""
""
""
""
""

HelpAltF4
""
""
""
""
""
""
""
""

HelpAltF5
""
""
""
""
""
""
""
""

HelpAltF6
""
""
""
""
""
""
""
""

HelpAltF7
""
""
""
""
""
""
""
""

HelpAltF8
""
""
""
""
""
""
""
""

HelpAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

HelpAltF10
""
""
""
""
""
""
""
""

HelpAltF11
""
""
""
""
""
""
""
""

HelpAltF12
""
""
""
""
""
""
""
""

HelpCtrlF1
l:
l:// Help KeyBar Ctrl-F1-12
""
""
""
""
""
""
""
""

HelpCtrlF2
""
""
""
""
""
""
""
""

HelpCtrlF3
""
""
""
""
""
""
""
""

HelpCtrlF4
""
""
""
""
""
""
""
""

HelpCtrlF5
""
""
""
""
""
""
""
""

HelpCtrlF6
""
""
""
""
""
""
""
""

HelpCtrlF7
""
""
""
""
""
""
""
""

HelpCtrlF8
""
""
""
""
""
""
""
""

HelpCtrlF9
""
""
""
""
""
""
""
""

HelpCtrlF10
""
""
""
""
""
""
""
""

HelpCtrlF11
""
""
""
""
""
""
""
""

HelpCtrlF12
""
""
""
""
""
""
""
""

HelpCtrlShiftF1
l:
l:// Help KeyBar CtrlShiftF1-12
""
""
""
""
""
""
""
""

HelpCtrlShiftF2
""
""
""
""
""
""
""
""

HelpCtrlShiftF3
""
""
""
""
""
""
""
""

HelpCtrlShiftF4
""
""
""
""
""
""
""
""

HelpCtrlShiftF5
""
""
""
""
""
""
""
""

HelpCtrlShiftF6
""
""
""
""
""
""
""
""

HelpCtrlShiftF7
""
""
""
""
""
""
""
""

HelpCtrlShiftF8
""
""
""
""
""
""
""
""

HelpCtrlShiftF9
""
""
""
""
""
""
""
""

HelpCtrlShiftF10
""
""
""
""
""
""
""
""

HelpCtrlShiftF11
""
""
""
""
""
""
""
""

HelpCtrlShiftF12
""
""
""
""
""
""
""
""

HelpCtrlAltF1
l:
l:// Help KeyBar CtrlAltF1-12
""
""
""
""
""
""
""
""

HelpCtrlAltF2
""
""
""
""
""
""
""
""

HelpCtrlAltF3
""
""
""
""
""
""
""
""

HelpCtrlAltF4
""
""
""
""
""
""
""
""

HelpCtrlAltF5
""
""
""
""
""
""
""
""

HelpCtrlAltF6
""
""
""
""
""
""
""
""

HelpCtrlAltF7
""
""
""
""
""
""
""
""

HelpCtrlAltF8
""
""
""
""
""
""
""
""

HelpCtrlAltF9
""
""
""
""
""
""
""
""

HelpCtrlAltF10
""
""
""
""
""
""
""
""

HelpCtrlAltF11
""
""
""
""
""
""
""
""

HelpCtrlAltF12
""
""
""
""
""
""
""
""

HelpAltShiftF1
l:
l:// Help KeyBar AltShiftF1-12
""
""
""
""
""
""
""
""

HelpAltShiftF2
""
""
""
""
""
""
""
""

HelpAltShiftF3
""
""
""
""
""
""
""
""

HelpAltShiftF4
""
""
""
""
""
""
""
""

HelpAltShiftF5
""
""
""
""
""
""
""
""

HelpAltShiftF6
""
""
""
""
""
""
""
""

HelpAltShiftF7
""
""
""
""
""
""
""
""

HelpAltShiftF8
""
""
""
""
""
""
""
""

HelpAltShiftF9
""
""
""
""
""
""
""
""

HelpAltShiftF10
""
""
""
""
""
""
""
""

HelpAltShiftF11
""
""
""
""
""
""
""
""

HelpAltShiftF12
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF1
l:
l:// Help KeyBar CtrlAltShiftF1-12
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF2
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF3
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF4
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF5
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF6
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF7
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF8
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF9
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF10
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF11
""
""
""
""
""
""
""
""

HelpCtrlAltShiftF12
""
""
""
""
""
""
""
""

InfoF1
l:
l:// InfoPanel KeyBar F1-F12
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

InfoF2
"Сверн"
"Wrap"
"Zalam"
"Umbr."
"SorTör"
"Zawiń"
"Divide"
"Згорн"

InfoF3
"СмОпис"
"VieDiz"
"Zobraz"
"BetDiz"
"MjMnéz"
"VieDiz"
"VerDiz"
"ДвОпис"

InfoF4
"РедОпи"
"EdtDiz"
"Edit"
"BeaDiz"
"MjSzrk"
"EdtDiz"
"EdtDiz"
"РедОпи"

InfoF5
""
""
""
""
""
""
""
""

InfoF6
""
""
""
""
""
""
""
""

InfoF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Search"
"Buscar"
"Пошук"

InfoF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Win"
"ANSI"


InfoF9
"КонфМн"
"ConfMn"
"KonfMn"
"KonfMn"
"KonfMn"
"ConfMn"
"BarMnu"
"КонфМн"

InfoF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

InfoF11
"Модули"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Модулі"

InfoF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant. "
"Екрани"

InfoShiftF1
l:
l:// InfoPanel KeyBar Shift-F1-F12
""
""
""
""
""
""
""
""

InfoShiftF2
"Слова"
"WWrap"
"ZalSlo"
"WUmbr"
"SzóTör"
"ZawijS"
"ConDiv"
"Слова"

InfoShiftF3
""
""
""
""
""
""
""
""

InfoShiftF4
""
""
""
""
""
""
""
""

InfoShiftF5
""
""
""
""
""
""
""
""

InfoShiftF6
""
""
""
""
""
""
""
""

InfoShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"TovKer"
"Nast."
"Próxim"
"Далі"

InfoShiftF8
"КодСтр"
"CodePg"
upd:"ZnSady"
upd:"Tabell"
"Kódlap"
"StrKod"
"Tabla"
"КодСтор"

InfoShiftF9
"Сохран"
"Save"
"Uložit"
"Speich"
"Mentés"
"Zapisz"
"Guarda"
"Зберти"

InfoShiftF10
"Послдн"
"Last"
"Posled"
"Letzt"
"UtsMnü"
"Ostat."
"Ultimo"
"Останн"

InfoShiftF11
""
""
""
""
""
""
""
""

InfoShiftF12
""
""
""
""
""
""
""
""

InfoAltF1
l:
l:// InfoPanel KeyBar Alt-F1-F12
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda"
"Ліва"

InfoAltF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha"
"Права"

InfoAltF3
""
""
""
""
""
""
""
""

InfoAltF4
""
""
""
""
""
""
""
""

InfoAltF5
""
""
""
""
""
""
""
""

InfoAltF6
""
""
""
""
""
""
""
""

InfoAltF7
"Искать"
"Find"
"Hledat"
"Suchen"
"Keres"
"Znajdź"
"Encont"
"Шукати"

InfoAltF8
"Строка"
"Goto"
"Jít na"
"GeheZu"
"Ugrás"
"IdźDo"
"Ir a.."
"Рядок"

InfoAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

InfoAltF10
"Дерево"
"Tree"
"Strom"
"Baum"
"MapKer"
"Drzewo"
"Arbol"
"Дерево"

InfoAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"Historia"
"HisVer"
"ІстПр"

InfoAltF12
"ИстПап"
"FoldHs"
"AdrsHs"
"OrdnHs"
"MapElő"
"FoldHs"
"HisDir"
"ІстТек"

InfoCtrlF1
l:
l:// InfoPanel KeyBar Ctrl-F1-F12
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda"
"Ліва"

InfoCtrlF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha"
"Права"

InfoCtrlF3
""
""
""
""
""
""
""
""

InfoCtrlF4
""
""
""
""
""
""
""
""

InfoCtrlF5
""
""
""
""
""
""
""
""

InfoCtrlF6
""
""
""
""
""
""
""
""

InfoCtrlF7
""
""
""
""
""
""
""
""

InfoCtrlF8
""
""
""
""
""
""
""
""

InfoCtrlF9
""
""
""
""
""
""
""
""

InfoCtrlF10
""
""
""
""
""
""
""
""

InfoCtrlF11
""
""
""
""
""
""
""
""

InfoCtrlF12
""
""
""
""
""
""
""
""

InfoCtrlShiftF1
l:
l:// InfoPanel KeyBar CtrlShiftF1-12
""
""
""
""
""
""
""
""

InfoCtrlShiftF2
""
""
""
""
""
""
""
""

InfoCtrlShiftF3
""
""
""
""
""
""
""
""

InfoCtrlShiftF4
""
""
""
""
""
""
""
""

InfoCtrlShiftF5
""
""
""
""
""
""
""
""

InfoCtrlShiftF6
""
""
""
""
""
""
""
""

InfoCtrlShiftF7
""
""
""
""
""
""
""
""

InfoCtrlShiftF8
""
""
""
""
""
""
""
""

InfoCtrlShiftF9
""
""
""
""
""
""
""
""

InfoCtrlShiftF10
""
""
""
""
""
""
""
""

InfoCtrlShiftF11
""
""
""
""
""
""
""
""

InfoCtrlShiftF12
""
""
""
""
""
""
""
""

InfoCtrlAltF1
l:
l:// InfoPanel KeyBar CtrlAltF1-12
""
""
""
""
""
""
""
""

InfoCtrlAltF2
""
""
""
""
""
""
""
""

InfoCtrlAltF3
""
""
""
""
""
""
""
""

InfoCtrlAltF4
""
""
""
""
""
""
""
""

InfoCtrlAltF5
""
""
""
""
""
""
""
""

InfoCtrlAltF6
""
""
""
""
""
""
""
""

InfoCtrlAltF7
""
""
""
""
""
""
""
""

InfoCtrlAltF8
""
""
""
""
""
""
""
""

InfoCtrlAltF9
""
""
""
""
""
""
""
""

InfoCtrlAltF10
""
""
""
""
""
""
""
""

InfoCtrlAltF11
""
""
""
""
""
""
""
""

InfoCtrlAltF12
""
""
""
""
""
""
""
""

InfoAltShiftF1
l:
l:// InfoPanel KeyBar AltShiftF1-12
""
""
""
""
""
""
""
""

InfoAltShiftF2
""
""
""
""
""
""
""
""

InfoAltShiftF3
""
""
""
""
""
""
""
""

InfoAltShiftF4
""
""
""
""
""
""
""
""

InfoAltShiftF5
""
""
""
""
""
""
""
""

InfoAltShiftF6
""
""
""
""
""
""
""
""

InfoAltShiftF7
""
""
""
""
""
""
""
""

InfoAltShiftF8
""
""
""
""
""
""
""
""

InfoAltShiftF9
""
""
""
""
""
""
""
""

InfoAltShiftF10
""
""
""
""
""
""
""
""

InfoAltShiftF11
""
""
""
""
""
""
""
""

InfoAltShiftF12
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF1
l:
l:// InfoPanel KeyBar CtrlAltShiftF1-12
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF2
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF3
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF4
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF5
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF6
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF7
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF8
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF9
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF10
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF11
""
""
""
""
""
""
""
""

InfoCtrlAltShiftF12
""
""
""
""
""
""
""
""

QViewF1
l:
l:// QView KeyBar F1-F12
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

QViewF2
"Сверн"
"Wrap"
"Zalam"
"Umbr."
"SorTör"
"Zawiń"
"Divide"
"Згорн"

QViewF3
"Просм"
"View"
"Zobraz"
"Betr."
"Megnéz"
"Zobacz"
"Ver"
"Прогл"

QViewF4
"Код"
"Hex"
"Hex"
"Hex"
"Hexa"
"Hex"
"Hexa"
"Код"

QViewF5
""
""
""
""
""
""
""
""

QViewF6
""
""
""
""
""
""
""
""

QViewF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Szukaj"
"Buscar"
"Пошук"

QViewF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Win"
"ANSI"

QViewF9
"КонфМн"
"ConfMn"
"KonfMn"
"KonfMn"
"KonfMn"
"ConfMn"
"BarMnu"
"КонфМн"

QViewF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

QViewF11
"Модули"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Модулі"

QViewF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant. "
"Екрани"

QViewShiftF1
l:
l:// QView KeyBar Shift-F1-F12
""
""
""
""
""
""
""
""

QViewShiftF2
"Слова"
"WWrap"
"ZalSlo"
"WUmbr"
"SzóTör"
"WWrap"
"ConDiv"
"Слова"

QViewShiftF3
""
""
""
""
""
""
""
""

QViewShiftF4
""
""
""
""
""
""
""
""

QViewShiftF5
""
""
""
""
""
""
""
""

QViewShiftF6
""
""
""
""
""
""
""
""

QViewShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"TovKer"
"Nast."
"Próxim"
"Далі"

QViewShiftF8
"КодСтр"
"CodePg"
upd:"ZnSady"
upd:"Tabell"
"Kódlap"
"StrKod"
"Tabla"
"КодСтор"

QViewShiftF9
"Сохран"
"Save"
"Uložit"
"Speich"
"Mentés"
"Zapisz"
"Guarda"
"Збргти"

QViewShiftF10
"Послдн"
"Last"
"Posled"
"Letzt"
"UtsMnü"
"Ostat."
"Ultimo"
"Останн"

QViewShiftF11
""
""
""
""
""
""
""
""

QViewShiftF12
""
""
""
""
""
""
""
""

QViewAltF1
l:
l:// QView KeyBar Alt-F1-F12
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda"
"Ліва"

QViewAltF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha"
"Права"

QViewAltF3
""
""
""
""
""
""
""
""

QViewAltF4
""
""
""
""
""
""
""
""

QViewAltF5
""
""
""
""
""
""
""
""

QViewAltF6
""
""
""
""
""
""
""
""

QViewAltF7
"Искать"
"Find"
"Hledat"
"Suchen"
"Keres"
"Znajdź"
"Encont"
"Шукати"

QViewAltF8
"Строка"
"Goto"
"Jít na"
"GeheZu"
"Ugrás"
"IdźDo"
"Ir a."
"Рядок"

QViewAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

QViewAltF10
"Дерево"
"Tree"
"Strom"
"Baum"
"MapKer"
"Drzewo"
"Arbol"
"Дерево"

QViewAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"Historia"
"HisVer"
"ІстПр"

QViewAltF12
"ИстПап"
"FoldHs"
"AdrsHs"
"OrdnHs"
"MapElő"
"FoldHs"
"HisDir"
"ІстТек"

QViewCtrlF1
l:
l:// QView KeyBar Ctrl-F1-F12
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda"
"Ліва"

QViewCtrlF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha"
"Права"

QViewCtrlF3
""
""
""
""
""
""
""
""

QViewCtrlF4
""
""
""
""
""
""
""
""

QViewCtrlF5
""
""
""
""
""
""
""
""

QViewCtrlF6
""
""
""
""
""
""
""
""

QViewCtrlF7
""
""
""
""
""
""
""
""

QViewCtrlF8
""
""
""
""
""
""
""
""

QViewCtrlF9
""
""
""
""
""
""
""
""

QViewCtrlF10
""
""
""
""
""
""
""
""

QViewCtrlF11
""
""
""
""
""
""
""
""

QViewCtrlF12
""
""
""
""
""
""
""
""

QViewCtrlShiftF1
l:
l:// QView KeyBar CtrlShiftF1-12
""
""
""
""
""
""
""
""

QViewCtrlShiftF2
""
""
""
""
""
""
""
""

QViewCtrlShiftF3
""
""
""
""
""
""
""
""

QViewCtrlShiftF4
""
""
""
""
""
""
""
""

QViewCtrlShiftF5
""
""
""
""
""
""
""
""

QViewCtrlShiftF6
""
""
""
""
""
""
""
""

QViewCtrlShiftF7
""
""
""
""
""
""
""
""

QViewCtrlShiftF8
""
""
""
""
""
""
""
""

QViewCtrlShiftF9
""
""
""
""
""
""
""
""

QViewCtrlShiftF10
""
""
""
""
""
""
""
""

QViewCtrlShiftF11
""
""
""
""
""
""
""
""

QViewCtrlShiftF12
""
""
""
""
""
""
""
""

QViewCtrlAltF1
l:
l:// QView KeyBar CtrlAltF1-12
""
""
""
""
""
""
""
""

QViewCtrlAltF2
""
""
""
""
""
""
""
""

QViewCtrlAltF3
""
""
""
""
""
""
""
""

QViewCtrlAltF4
""
""
""
""
""
""
""
""

QViewCtrlAltF5
""
""
""
""
""
""
""
""

QViewCtrlAltF6
""
""
""
""
""
""
""
""

QViewCtrlAltF7
""
""
""
""
""
""
""
""

QViewCtrlAltF8
""
""
""
""
""
""
""
""

QViewCtrlAltF9
""
""
""
""
""
""
""
""

QViewCtrlAltF10
""
""
""
""
""
""
""
""

QViewCtrlAltF11
""
""
""
""
""
""
""
""

QViewCtrlAltF12
""
""
""
""
""
""
""
""

QViewAltShiftF1
l:
l:// QView KeyBar AltShiftF1-12
""
""
""
""
""
""
""
""

QViewAltShiftF2
""
""
""
""
""
""
""
""

QViewAltShiftF3
""
""
""
""
""
""
""
""

QViewAltShiftF4
""
""
""
""
""
""
""
""

QViewAltShiftF5
""
""
""
""
""
""
""
""

QViewAltShiftF6
""
""
""
""
""
""
""
""

QViewAltShiftF7
""
""
""
""
""
""
""
""

QViewAltShiftF8
""
""
""
""
""
""
""
""

QViewAltShiftF9
""
""
""
""
""
""
""
""

QViewAltShiftF10
""
""
""
""
""
""
""
""

QViewAltShiftF11
""
""
""
""
""
""
""
""

QViewAltShiftF12
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF1
l:
l:// QView KeyBar CtrlAltShiftF1-12
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF2
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF3
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF4
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF5
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF6
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF7
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF8
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF9
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF10
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF11
""
""
""
""
""
""
""
""

QViewCtrlAltShiftF12
""
""
""
""
""
""
""
""

KBTreeF1
l:
l:// Tree KeyBar F1-F12
"Помощь"
"Help"
"Pomoc"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

KBTreeF2
"ПользМ"
"UserMn"
"UživMn"
"BenuMn"
"FelhMn"
"UserMn"
"Menú"
"КорстМ"

KBTreeF3
""
""
""
""
""
""
""
""

KBTreeF4
"Атриб"
"Attr"
"Attr"
"Attr"
"Attrib"
"Atryb."
"Atrib"
"Атриб"

KBTreeF5
"Копир"
"Copy"
"Kopír."
"Kopier"
"Másol"
"Kopiuj"
"Copiar"
"Копию"

KBTreeF6
"Перен"
"RenMov"
"PřjPřs"
"RenMov"
"ÁtnMoz"
"Zamień"
"RenMov"
"Перен"

KBTreeF7
"Папка"
"MkFold"
"VytAdr"
"VerzEr"
"ÚjMapp"
"NowyFldr"
"CrDIR "
"Тека"

KBTreeF8
"Удален"
"Delete"
"Smazat"
"Lösch"
"Törlés"
"Usuń"
"Borrar"
"Видалн"

KBTreeF9
"КонфМн"
"ConfMn"
"KonfMn"
"KonfMn"
"KonfMn"
"KonfMenu"
"BarMnu"
"КонфМн"

KBTreeF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

KBTreeF11
"Модули"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Модулі"

KBTreeF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant."
"Екрани"

KBTreeShiftF1
l:
l:// Tree KeyBar Shift-F1-F12
""
""
""
""
""
""
""
""

KBTreeShiftF2
""
""
""
""
""
""
""
""

KBTreeShiftF3
""
""
""
""
""
""
""
""

KBTreeShiftF4
""
""
""
""
""
""
""
""

KBTreeShiftF5
"Копир"
"Copy"
"Kopír."
"Kopier"
"Másol"
"Kopiuj"
"Copiar"
"Копию"

KBTreeShiftF6
"Перен"
"Rename"
"Přejm."
"Umben"
"ÁtnMoz"
"Zamień"
"RenMov"
"Перен"

KBTreeShiftF7
""
""
""
""
""
""
""
""

KBTreeShiftF8
""
""
""
""
""
""
""
""

KBTreeShiftF9
"Сохран"
"Save"
"Uložit"
"Speich"
"Mentés"
"Zapisz"
"Guarda"
"Збргти"

KBTreeShiftF10
"Послдн"
"Last"
"Posled"
"Letzt"
"UtsMnü"
"Ostat."
"Ultimo"
"Останн"

KBTreeShiftF11
"Группы"
"Group"
"Skupin"
"Gruppe"
"Csoprt"
"Grupa"
"Grupo"
"Групи"

KBTreeShiftF12
"Выбран"
"SelUp"
"VybPrv"
"AuswOb"
"KijFel"
"SelUp"
"SelUp"
"Вибран"

KBTreeAltF1
l:
l:// Tree KeyBar Alt-F1-F12
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda"
"Ліва"

KBTreeAltF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha"
"Права"

KBTreeAltF3
""
""
""
""
""
""
""
""

KBTreeAltF4
""
""
""
""
""
""
""
""

KBTreeAltF5
""
""
""
""
""
""
""
""

KBTreeAltF6
""
""
""
""
""
""
""
""

KBTreeAltF7
"Искать"
"Find"
"Hledat"
"Suchen"
"Keres"
"Znajdź"
"Encont"
"Шукати"

KBTreeAltF8
"Истор"
"Histry"
"Histor"
"Histor"
"ParElő"
"Historia"
"Histor"
"Істор"

KBTreeAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

KBTreeAltF10
"Дерево"
"Tree"
"Strom"
"Baum"
"MapKer"
"Drzewo"
"Arbol"
"Дерево"

KBTreeAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"Historia"
"HisVer"
"ІстПр"

KBTreeAltF12
"ИстПап"
"FoldHs"
"AdrsHs"
"OrdnHs"
"MapElő"
"FoldHs"
"HisDir"
"ІстТек"

KBTreeCtrlF1
l:
l:// Tree KeyBar Ctrl-F1-F12
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda"
"Ліва"

KBTreeCtrlF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha"
"Права"

KBTreeCtrlF3
""
""
""
""
""
""
""
""

KBTreeCtrlF4
""
""
""
""
""
""
""
""

KBTreeCtrlF5
""
""
""
""
""
""
""
""

KBTreeCtrlF6
""
""
""
""
""
""
""
""

KBTreeCtrlF7
""
""
""
""
""
""
""
""

KBTreeCtrlF8
""
""
""
""
""
""
""
""

KBTreeCtrlF9
""
""
""
""
""
""
""
""

KBTreeCtrlF10
""
""
""
""
""
""
""
""

KBTreeCtrlF11
""
""
""
""
""
""
""
""

KBTreeCtrlF12
""
""
""
""
""
""
""
""

KBTreeCtrlShiftF1
l:
l:// Tree KeyBar CtrlShiftF1-12
""
""
""
""
""
""
""
""

KBTreeCtrlShiftF2
""
""
""
""
""
""
""
""

KBTreeCtrlShiftF3
""
""
""
""
""
""
""
""

KBTreeCtrlShiftF4
""
""
""
""
""
""
""
""

KBTreeCtrlShiftF5
""
""
""
""
""
""
""
""

KBTreeCtrlShiftF6
""
""
""
""
""
""
""
""

KBTreeCtrlShiftF7
""
""
""
""
""
""
""
""

KBTreeCtrlShiftF8
""
""
""
""
""
""
""
""

KBTreeCtrlShiftF9
""
""
""
""
""
""
""
""

KBTreeCtrlShiftF10
""
""
""
""
""
""
""
""

KBTreeCtrlShiftF11
""
""
""
""
""
""
""
""

KBTreeCtrlShiftF12
""
""
""
""
""
""
""
""

KBTreeCtrlAltF1
l:
l:// Tree KeyBar CtrlAltF1-12
""
""
""
""
""
""
""
""

KBTreeCtrlAltF2
""
""
""
""
""
""
""
""

KBTreeCtrlAltF3
""
""
""
""
""
""
""
""

KBTreeCtrlAltF4
""
""
""
""
""
""
""
""

KBTreeCtrlAltF5
""
""
""
""
""
""
""
""

KBTreeCtrlAltF6
""
""
""
""
""
""
""
""

KBTreeCtrlAltF7
""
""
""
""
""
""
""
""

KBTreeCtrlAltF8
""
""
""
""
""
""
""
""

KBTreeCtrlAltF9
""
""
""
""
""
""
""
""

KBTreeCtrlAltF10
""
""
""
""
""
""
""
""

KBTreeCtrlAltF11
""
""
""
""
""
""
""
""

KBTreeCtrlAltF12
""
""
""
""
""
""
""
""

KBTreeAltShiftF1
l:
l:// Tree KeyBar AltShiftF1-12
""
""
""
""
""
""
""
""

KBTreeAltShiftF2
""
""
""
""
""
""
""
""

KBTreeAltShiftF3
""
""
""
""
""
""
""
""

KBTreeAltShiftF4
""
""
""
""
""
""
""
""

KBTreeAltShiftF5
""
""
""
""
""
""
""
""

KBTreeAltShiftF6
""
""
""
""
""
""
""
""

KBTreeAltShiftF7
""
""
""
""
""
""
""
""

KBTreeAltShiftF8
""
""
""
""
""
""
""
""

KBTreeAltShiftF9
""
""
""
""
""
""
""
""

KBTreeAltShiftF10
""
""
""
""
""
""
""
""

KBTreeAltShiftF11
""
""
""
""
""
""
""
""

KBTreeAltShiftF12
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF1
l:
l:// Tree KeyBar CtrlAltShiftF1-12
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF2
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF3
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF4
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF5
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF6
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF7
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF8
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF9
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF10
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF11
""
""
""
""
""
""
""
""

KBTreeCtrlAltShiftF12
""
""
""
""
""
""
""
""

CopyTimeInfo
l:
"Время: %8.8ls    Осталось: %8.8ls    %8.8lsб/с"
"Time: %8.8ls    Remaining: %8.8ls    %8.8lsb/s"
"Čas: %8.8ls      Zbývá: %8.8ls      %8.8lsb/s"
"Zeit: %8.8ls   Verbleibend: %8.8ls   %8.8lsb/s"
"Eltelt: %8.8ls    Maradt: %8.8ls    %8.8lsb/s"
"Czas: %8.8ls    Pozostało: %8.8ls    %8.8lsb/s"
"Tiempo: %8.8ls    Restante: %8.8ls    %8.8lsb/s"
"Час: %8.8ls    Залишилось: %8.8ls    %8.8lsб/с"

KeyESCWasPressed
l:
"Действие было прервано"
"Operation has been interrupted"
"Operace byla přerušena"
"Vorgang wurde unterbrochen"
"A műveletet megszakította"
"Operacja została przerwana"
"Operación ha sido interrumpida"
"Дія була перервана"

DoYouWantToStopWork
"Вы действительно хотите отменить действие?"
"Do you really want to cancel it?"
"Opravdu chcete operaci stornovat?"
"Wollen Sie den Vorgang wirklich abbrechen?"
"Valóban le akarja állítani?"
"Czy naprawdę chcesz ją anulować?"
"Desea realmente cancelar la operación?"
"Ви дійсно хочете скасувати дію?"

DoYouWantToStopWork2
"Продолжить выполнение?"
"Continue work? "
"Pokračovat v práci?"
"Vorgang fortsetzen? "
"Folytatja?"
"Kontynuować? "
"Continuar trabajo? "
"Продовжити виконання?"

CheckingFileInPlugin
l:
"Файл проверяется в плагине"
"The file is being checked by the plugin"
"Soubor je právě kontrolován pluginem"
"Datei wird von Plugin überprüft"
"A fájlt ez a plugin használja:"
"Plugin sprawdza plik"
"El archivo está siendo chequeado por el plugin"
"Файл перевіряється у плагіні"

DialogType
l:
"Диалог"
"Dialog"
"Dialog"
"Dialog"
"Párbeszéd"
"Dialog"
"Diálogo"
"Діалог"

HelpType
"Помощь"
"Help"
"Nápověda"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

FolderTreeType
"ПоискКаталогов"
"FolderTree"
"StromAdresáře"
"Ordnerbaum"
"MappaFa"
"Drzewo folderów"
"ArbolDirectorio"
"ПошукКаталогів"

VMenuType
"Меню"
"Menu"
"Menu"
"Menü"
"Menü"
"Menu"
"Menú"
"Меню"

IncorrectMask
l:
"Некорректная маска файлов"
"File-mask string contains errors"
"Řetězec masky souboru obsahuje chyby"
"Zeichenkette mit Dateimaske enthält Fehler"
"A fájlmaszk hibás"
"Maska pliku zawiera błędy"
"Cadena de máscara de archivos contiene errores"
"Неправильна маска файлів"

PanelBracketsForLongName
l:
"{}"
"{}"
"{}"
"{}"
"{}"
"{}"
"{}"
"{}"

ComspecNotFound
l:
"Переменная окружения %COMSPEC% не определена"
"Environment variable %COMSPEC% not defined"
"Proměnná prostředí %COMSPEC% není definována"
"Umgebungsvariable %COMSPEC% nicht definiert"
"A %COMSPEC% környezeti változó nincs definiálva"
"Nie zdefiniowano zmiennej środowiskowej %COMSPEC%"
"Variable de entorno %COMSPEC% no definida"
"Змінне оточення %COMSPEC% не визначено"

ExecuteErrorMessage
"'%ls' не является внутренней или внешней командой, исполняемой программой или пакетным файлом.\n"
"'%ls' is not recognized as an internal or external command, operable program or batch file.\n"
"'%ls' nebylo nalezeno jako vniřní nebo externí příkaz, spustitelná aplikace nebo dávkový soubor.\n"
"'%ls' nicht erkannt als interner oder externer Befehl, Programm oder Stapeldatei.\n"
""%ls" nem azonítható külső vagy belső parancsként, futtatható programként vagy batch fájlként.\n"
"Nie rozpoznano '%ls' jako polecenia, programu ani skryptu.\n"
"'%ls' no es reconocida como un comando interno o externo, programa operable o archivo de lotes.\n"
"'%ls' не є внутрішньою або зовнішньою командою, яку виконує програма або пакетний файл.\n"

OpenPluginCannotOpenFile
l:
"Ошибка открытия файла"
"Cannot open the file"
"Nelze otevřít soubor"
"Kann Datei nicht öffnen"
"A fájl nem nyitható meg"
"Nie można otworzyć pliku"
"No se puede abrir el archivo"
"Помилка відкриття файлу"

FileFilterTitle
l:
"Фильтр"
"Filter"
"Filtr"
"Filter"
"Felhasználói szűrő"
"Filtr wyszukiwania"
"Filtro"
"Фільтр"

FileHilightTitle
"Раскраска файлов"
"Files highlighting"
"Zvýrazňování souborů"
"Farbmarkierungen"
"Fájlkiemelés"
"Zaznaczanie plików"
"Resaltado de archivos"
"Розмальовка файлів"

FileFilterName
"Имя &фильтра:"
"Filter &name:"
"Jmé&no filtru:"
"Filter&name:"
"Szűrő &neve:"
"Nazwa &filtra:"
"&Nombre filtro:"
"Ім'я &фільтра:"

FileFilterMatchMask
"&Маска:"
"&Mask:"
"&Maska"
"&Maske:"
"&Maszk:"
"&Maska:"
"&Máscara:"
"&Маска:"

FileFilterSize
"Разм&ер:"
"Si&ze:"
"Vel&ikost"
"G&röße:"
"M&éret:"
"Ro&zmiar:"
"&Tamaño:"
"Розм&ір:"

FileFilterSizeFromSign
">="
">="
">="
">="
">="
">="
">="
">="

FileFilterSizeToSign
"<="
"<="
"<="
"<="
"<="
"<="
"<="
"<="

FileFilterDate
"&Дата/Время:"
"Da&te/Time:"
"Dat&um/Čas:"
"Da&tum/Zeit:"
"&Dátum/Idő:"
"Da&ta/Czas:"
"&Fecha/Hora:"
"&Дата/Час:"

FileFilterWrited
"&записи"
upd:"&write"
upd:"&write"
upd:"&write"
upd:"&write"
upd:"&write"
"&modificación"
"&записи"

FileFilterCreated
"&создания"
"&creation"
"&vytvoření"
"E&rstellung"
"&Létrehozás"
"&utworzenia"
"&creación"
"&створення"

FileFilterOpened
"&доступа"
"&access"
"&přístupu"
"Z&ugriff"
"&Hozzáférés"
"&dostępu"
"&acceso"
"&доступу"

FileFilterChanged
"&изменения"
"c&hange"
upd:"c&hange"
upd:"c&hange"
upd:"c&hange"
upd:"c&hange"
upd:"c&hange"
"&зміни"

FileFilterDateRelative
"Относительна&я"
"Relat&ive"
"Relati&vní"
"Relat&iv"
"Relat&ív"
"Relat&ive"
"Relat&ivo"
"Відносн&а"

FileFilterDateAfterSign
">="
">="
">="
">="
">="
">="
">="
">="

FileFilterDateBeforeSign
"<="
"<="
"<="
"<="
"<="
"<="
"<="
"<="

FileFilterCurrent
"Теку&щая"
"C&urrent"
"Aktuá&lní"
"Akt&uell"
"&Jelenlegi"
"&Bieżący"
"Act&ual"
"Пото&чна"

FileFilterBlank
"С&брос"
"B&lank"
"Práz&dný"
"&Leer"
"&Üres"
"&Wyczyść"
"En b&lanco"
"С&кидання"

FileFilterAttr
"Атрибут&ы"
"Attri&butes"
"Attri&buty"
"Attri&bute"
"Attri&bútumok"
"&Atrybuty"
"Atri&butos"
"Атрибут&и"

FileFilterAttrR
"&Только для чтения"
"&Read only"
"Jen pro čt&ení"
"Sch&reibschutz"
"&Csak olvasható"
"&Do odczytu"
"Sólo Lectu&ra"
"&Тільки для читання"

FileFilterAttrA
"&Архивный"
"&Archive"
"Arc&hivovat"
"&Archiv"
"&Archív"
"&Archiwalny"
"&Archivo"
"&Архівний"

FileFilterAttrH
"&Скрытый"
"&Hidden"
"Skry&tý"
"&Versteckt"
"&Rejtett"
"&Ukryty"
"&Oculto"
"&Скритий"

FileFilterAttrS
"С&истемный"
"&System"
"Systémo&vý"
"&System"
"Re&ndszer"
"&Systemowy"
"&Sistema"
"С&истемний"

FileFilterAttrC
"С&жатый"
"&Compressed"
"Kompri&movaný"
"&Komprimiert"
"&Tömörített"
"S&kompresowany"
"&Comprimido"
"С&тиснутий"

FileFilterAttrE
"&Зашифрованный"
"&Encrypted"
"Ši&frovaný"
"V&erschlüsselt"
"T&itkosított"
"&Zaszyfrowany"
"Ci&frado"
"&Зашифрований"

FileFilterAttrD
"&Каталог"
"&Directory"
"Adr&esář"
"Ver&zeichnis"
"Map&pa"
"&Katalog"
"&Directorio"
"&Каталог"

FileFilterAttrNI
"&Неиндексируемый"
"Not inde&xed"
"Neinde&xovaný"
"Nicht in&diziert"
"Nem inde&xelt"
"Nie z&indeksowany"
"No inde&xado"
"&Неіндексований"

FileFilterAttrSparse
"&Разрежённый"
"S&parse"
"Říd&ký"
"Reserve"
"Ritk&ított"
"S&parse"
"Escaso"
"&Розріджений"

FileFilterAttrT
"&Временный"
"Temporar&y"
"Doča&sný"
"Temporär"
"Átm&eneti"
"&Tymczasowy"
"Tempora&l"
"&Тимчасовий"

FileFilterAttrReparse
"Симво&л. ссылка"
"Symbolic lin&k"
"Sybolický li&nk"
"Symbolischer Lin&k"
"S&zimbolikus link"
"Link &symboliczny"
"Enlace simbólic&o"
"Симво&л. посилання"

FileFilterAttrOffline
"Автономны&й"
"O&ffline"
"O&ffline"
"O&ffline"
"O&ffline"
"O&ffline"
"O&ffline"
"Автономни&й"

FileFilterAttrVirtual
"Вирт&уальный"
"&Virtual"
"Virtuální"
"&Virtuell"
"&Virtuális"
"&Wirtualny"
"&Virtual"
"Вірт&уальний"

FileFilterAttrExecutable
"Исполняемый"
"E&xecutable"
upd:"E&xecutable"
upd:"E&xecutable"
upd:"E&xecutable"
upd:"E&xecutable"
upd:"E&xecutable"
"Виконуваний"

FileFilterAttrBroken
"Неисправный"
"&Broken"
upd:"&Broken"
upd:"&Broken"
upd:"&Broken"
upd:"&Broken"
upd:"&Broken"
"Несправний"

FileFilterReset
"Очистит&ь"
"Reset"
"Reset"
"Rücksetzen"
"Reset"
"Wy&czyść"
"Reinicio"
"Очистит&и"

FileFilterCancel
"Отмена"
"Cancel"
"Storno"
"Abbruch"
"Mégsem"
"&Anuluj"
"Cancelar"
"Відміна"

FileFilterMakeTransparent
"Выставить прозрачность"
"Make transparent"
"Zprůhlednit"
"Transparent"
"Legyen átlátszó"
"Ustaw jako przezroczysty"
"Hacer transparente"
"Виставити прозорість"

BadFileSizeFormat
"Неправильно заполнено поле размера"
"File size field is incorrectly filled"
"Velikost souboru neobsahuje správnou hodnotu"
"Angabe der Dateigröße ist fehlerhaft"
"A fájlméret mező rosszul van kitöltve"
"Rozmiar pliku jest niepoprawny"
"Campo de tamaño de archivo no está correctamente llenado"
"Неправильно заповнено поле розміру"

FarTitleAddonsAdmin
l:
"root"
"root"
upd:"root"
upd:"root"
upd:"root"
upd:"root"
"root"
"root"

AdminRequired
"Нужно обладать правами администратора"
"You need to provide administrator permission"
upd:"You need to provide administrator permission"
upd:"You need to provide administrator permission"
upd:"You need to provide administrator permission"
upd:"You need to provide administrator permission"
"Usted necesita permisos de administrador"
"Потрібно мати права адміністратора"

AdminRequiredPrivileges
"Требуются дополнительные привилегии"
"Additional privileges required"
upd:"Additional privileges required"
upd:"Additional privileges required"
upd:"Additional privileges required"
upd:"Additional privileges required"
"Privilegios adicionales requeridos"
"Потрібні додаткові привілеї"

AdminRequiredProcess
"для обработки этого объекта:"
"to process this object:"
upd:"to process this object:"
upd:"to process this object:"
upd:"to process this object:"
upd:"to process this object:"
"para procesar este objeto:"
"для обробки цього об'єкта:"

AdminRequiredCreate
"для создания этого объекта:"
"to create this object:"
upd:"to create this object:"
upd:"to create this object:"
upd:"to create this object:"
upd:"to create this object:"
"para crear este objeto:"
"для створення цього об'єкта:"

AdminRequiredDelete
"для удаления этого объекта:"
"to delete this object:"
upd:"to delete this object:"
upd:"to delete this object:"
upd:"to delete this object:"
upd:"to delete this object:"
"para eliminar este objeto:"
"для видалення цього об'єкта:"

AdminRequiredCopy
"для копирования этого объекта:"
"to copy this object:"
upd:"to copy this object:"
upd:"to copy this object:"
upd:"to copy this object:"
upd:"to copy this object:"
"para copiar este objeto:"
"для копіювання цього об'єкта:"

AdminRequiredMove
"для перемещения этого объекта:"
"to move this object:"
upd:"to move this object:"
upd:"to move this object:"
upd:"to move this object:"
upd:"to move this object:"
"para mover este objeto:"
"для переміщення цього об'єкта:"

AdminRequiredGetAttributes
"для получения атрибутов этого объекта:"
"to get attributes of this object:"
upd:"to get attributes of this object:"
upd:"to get attributes of this object:"
upd:"to get attributes of this object:"
upd:"to get attributes of this object:"
"para obtener atributos de este objeto:"
"для отримання атрибутів цього об'єкта:"

AdminRequiredSetAttributes
"для установки атрибутов этого объекта:"
"to set attributes of this object:"
upd:"to set attributes of this object:"
upd:"to set attributes of this object:"
upd:"to set attributes of this object:"
upd:"to set attributes of this object:"
"para poner atributos a este objeto:"
"для встановлення атрибутів цього об'єкта:"

AdminRequiredHardLink
"для создания этой жёсткой ссылки:"
"to create this hard link:"
upd:"to create this hard link:"
upd:"to create this hard link:"
upd:"to create this hard link:"
upd:"to create this hard link:"
"para crear este enlace rígido:"
"для створення цього жорсткого посилання:"

AdminRequiredSymLink
"для создания этой символической ссылки:"
"to create this symbolic link:"
upd:"to create this symbolic link:"
upd:"to create this symbolic link:"
upd:"to create this symbolic link:"
upd:"to create this symbolic link:"
"para crear este enlace simbólico:"
"для створення цього символічного посилання:"

AdminRequiredRecycle
"для перемещения этого объекта в корзину:"
"to move this object to recycle bin:"
upd:"to move this object to recycle bin:"
upd:"to move this object to recycle bin:"
upd:"to move this object to recycle bin:"
upd:"to move this object to recycle bin:"
"para mover este objeto a la papelera:"
"для переміщення цього об'єкта в кошик:"

AdminRequiredList
"для просмотра этого объекта:"
"to list this object:"
upd:"to list this object:"
upd:"to list this object:"
upd:"to list this object:"
upd:"to list this object:"
"para listar este objeto:"
"для перегляду цього об'єкта:"

AdminRequiredSetOwner
"для установки владельца этого объекта:"
"to set owner of this object:"
upd:"to set owner of this object:"
upd:"to set owner of this object:"
upd:"to set owner of this object:"
upd:"to set owner of this object:"
"para poner como dueño de este objeto:"
"для встановлення власника цього об'єкта:"

AdminRequiredOpen
"для открытия этого объекта:"
"to open this object:"
upd:"to open this object:"
upd:"to open this object:"
upd:"to open this object:"
upd:"to open this object:"
"para abrir este objeto:"
"для відкриття цього об'єкта:"

AdminDoForAll
"Выполнить это действие для &всех текущих объектов"
"Do this for &all current objects"
upd:"Do this for &all current objects"
upd:"Do this for &all current objects"
upd:"Do this for &all current objects"
upd:"Do this for &all current objects"
"Hacer esto para todos los objetos actuales"
"Виконати цю дію для &всіх поточних об'єктів"

AdminDoNotAskAgainInTheCurrentSession
"Больше не спрашивать в текущей сессии"
"Do not ask again in the current session"
upd:"Do not ask again in the current session"
upd:"Do not ask again in the current session"
upd:"Do not ask again in the current session"
upd:"Do not ask again in the current session"
"No preguntar nuevamente en la sesión actual"
"Більше не питати у поточній сесії"

TerminalClipboardAccessTitle
"Доступ к буферу обмена"
"Clipboard access"
upd:"Clipboard access"
upd:"Clipboard access"
upd:"Clipboard access"
upd:"Clipboard access"
upd:"Clipboard access"
"Доступ до буфера обміну"

TerminalClipboardAccessText
"Укажите как это приложение может пользоваться буфером обмена."
"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
"Вкажіть, як ця програма може користуватися буфером обміну."

TerminalClipboardAccessBlock
"&Заблокировать"
"&Block attempt"
upd:"&Block attempt"
upd:"&Block attempt"
upd:"&Block attempt"
upd:"&Block attempt"
upd:"&Block attempt"
"&Заблокувати"

TerminalClipboardAccessTemporaryRemote
"&Удаленный буфер"
"&Remote clipboard"
upd:"&Remote clipboard"
upd:"&Remote clipboard"
upd:"&Remote clipboard"
upd:"&Remote clipboard"
upd:"&Remote clipboard"
"&Віддалений буфер"

TerminalClipboardAccessTemporaryLocal
"&Общий буфер"
"&Share clipboard"
upd:"&Share clipboard"
upd:"&Share clipboard"
upd:"&Share clipboard"
upd:"&Share clipboard"
upd:"&Share clipboard"
"&Спільний буфер"

TerminalClipboardAccessAlwaysLocal
"Общий буфер всег&да"
"Share clipboard &always"
upd:"Share clipboard &always"
upd:"Share clipboard &always"
upd:"Share clipboard &always"
upd:"Share clipboard &always"
upd:"Share clipboard &always"
"Загальний буфер завж&ди"

MountsRoot
"Корень"
"Root"
upd:"Root"
upd:"Root"
upd:"Root"
upd:"Root"
upd:"Root"
"Корінь"

MountsHome
"Дом"
"Home"
upd:"Home"
upd:"Home"
upd:"Home"
upd:"Home"
upd:"Home"
"Дом"

MountsOther
"Др. панель"
"Other panel"
upd:"Other panel"
upd:"Other panel"
upd:"Other panel"
upd:"Other panel"
upd:"Other panel"
"Інш. панель"

#Must be the last
NewFileName
l:
"?Новый файл?"
"?New File?"
"?Nový soubor?"
"?Neue Datei?"
"?Új fájl?"
"?Nowy plik?"
"?Nuevoi Archivo?"
"?Новий файл?"
