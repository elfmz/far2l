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

#--------------------------------------------------------------------
#now come the lng feeds
#--------------------------------------------------------------------
#first comes the text name from the enum which can be preceded with
#comments that will go to the hpp file
#h://This comment will appear before MYes
#he://This comment will appear after MYes
#MYes
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


MYes=0
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

MNo
"Нет"
"No"
"Ne"
"Nein"
"Nem"
"Nie"
"No"
"Ні"

MOk
"OK"
"OK"
"Ok"
"OK"
"OK"
"OK"
"Aceptar"
"OK"

MHYes
l:
"&Да"
"&Yes"
"&Ano"
"&Ja"
"I&gen"
"&Tak"
"&Si"
"&Так"

MHNo
"&Нет"
"&No"
"&Ne"
"&Nein"
"Ne&m"
"&Nie"
"&No"
"&Ні"

MHOk
"&OK"
"&OK"
"&Ok"
"&OK"
"&OK"
"&OK"
"&Aceptar"
"&OK"

MCancel
l:
"Отмена"
"Cancel"
"Storno"
"Abbrechen"
"Mégsem"
"Anuluj"
"Cancelar"
"Відміна"

MRetry
"Повторить"
"Retry"
"Znovu"
"Wiederholen"
"Újra"
"Ponów"
"Reiterar"
"Повторити"

MSkip
"Пропустить"
"Skip"
"Přeskočit"
"Überspringen"
"Kihagy"
"Omiń"
"Omitir"
"Пропустити"

MAbort
"Прервать"
"Abort"
"Zrušit"
"Abbrechen"
"Megszakít"
"Zaniechaj"
"Abortar"
"Перервати"

MIgnore
"Игнорировать"
"Ignore"
"Ignorovat"
"Ignorieren"
"Mégis"
"Zignoruj"
"Ignorar"
"Ігнорувати"

MDelete
"Удалить"
"Delete"
"Smazat"
"Löschen"
"Töröl"
"Usuń"
"Borrar"
"Вилучити"

MSplit
"Разделить"
"Split"
"Rozdělit"
"Zerteilen"
"Feloszt"
"Podziel"
"Dividir"
"Розділити"

MRemove
"Удалить"
"Remove"
"Odstranit"
"Entfernen"
"Eltávolít"
"Usuń"
"Remover"
"Видалити"

MHCancel
l:
"&Отмена"
"&Cancel"
"&Storno"
"&Abbrechen"
"Még&sem"
"&Anuluj"
"&Cancelar"
"&Відміна"

MHRetry
"&Повторить"
"&Retry"
"&Znovu"
"&Wiederholen"
"Ú&jra"
"&Ponów"
"&Reiterar"
"&Повторити"

MHSkip
"П&ропустить"
"&Skip"
"&Přeskočit"
"Über&springen"
"Ki&hagy"
"&Omiń"
"&Omitir"
"П&ропустити"

MHSkipAll
"Пропустить &все"
"S&kip all"
"Přeskočit &vše"
"Alle übersprin&gen"
"Kihagy &mind"
"Omiń &wszystkie"
"Omitir &Todo"
"Пропустити &усе"

MHAbort
"Прер&вать"
"&Abort"
"Zr&ušit"
"&Abbrechen"
"Megsza&kít"
"&Zaniechaj"
"Ab&ortar"
"Перер&вати"

MHIgnore
"&Игнорировать"
"&Ignore"
"&Ignorovat"
"&Ignorieren"
"Mé&gis"
"Z&ignoruj"
"&Ignorar"
"&Ігнорувати"

MHDelete
"&Удалить"
"&Delete"
"S&mazat"
"&Löschen"
"&Töröl"
"&Usuń"
"&Borrar"
"&Вилучити"

MHRemove
"&Удалить"
"R&emove"
"&Odstranit"
"Ent&fernen"
"Eltá&volít"
"U&suń"
"R&emover"
"&Видалити"

MHSplit
"Раз&делить"
"Sp&lit"
"&Rozdělit"
"&Zerteilen"
"Fel&oszt"
"Po&dziel"
"Dividir"
"Роз&ділити"


MWarning
l:
"Предупреждение"
"Warning"
"Varování"
"Warnung"
"Figyelem"
"Ostrzeżenie"
"Advertencia"
"Попередження"

MError
"Ошибка"
"Error"
"Chyba"
"Fehler"
"Hiba"
"Błąd"
"Error"
"Помилка"

MQuit
l:
"Выход"
"Quit"
"Konec"
"Beenden"
"Kilépés"
"Zakończ"
"Salir"
"Вихід"

MAskQuit
"Вы хотите завершить работу в FAR?"
"Do you want to quit FAR?"
"Opravdu chcete ukončit FAR?"
"Wollen Sie FAR beenden?"
"Biztosan kilép a FAR-ból?"
"Czy chcesz zakończyć pracę z FARem?"
"Desea salir de FAR?"
"Ви хочете завершити роботу в FAR?"

MBackground
"&В фон"
"&Background"
"&Background"
"&Background"
"&Background"
"&Background"
"&Background"
"&У тлі"

MGetOut
"&Выбраться"
"&Get out"
"&Get out"
"&Get out"
"&Get out"
"&Get out"
"&Get out"
"&Вибратися"

MF1
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

MF2
"ПользМ"
"UserMn"
"UživMn"
"BenuMn"
"FhMenü"
"Menu"
"Menú "
"КористМ"

MF3
"Просм"
"View"
"Zobraz"
"Betr."
"Megnéz"
"Zobacz"
"Ver "
"Перегл"

MF4
"Редакт"
"Edit"
"Edit"
"Bearb"
"Szerk."
"Edytuj"
"Editar"
"Редакт"

MF5
"Копир"
"Copy"
"Kopír."
"Kopier"
"Másol"
"Kopiuj"
"Copiar"
"Копію"

MF6
"Перен"
"RenMov"
"PřjPřs"
"Versch"
"AtnMoz"
"ZmNazw"
"RenMov"
"Перен"

MF7
"Папка"
"MkFold"
"VytAdr"
"VerzEr"
"ÚjMapp"
"UtwKat"
"CrDIR "
"Тека"

MF8
"Удален"
"Delete"
"Smazat"
"Lösch."
"Töröl"
"Usuń"
"Borrar"
"Видалн"

MF9
"КонфМн"
"ConfMn"
"KonfMn"
"KonfMn"
"KonfMn"
"Konfig"
"BarMnu"
"КонфМн"

MF10
"Выход"
"Quit"
"Konec"
"Beend."
"Kilép"
"Koniec"
"Salir"
"Вихід"

MF11
"Модули"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Модулі"

MF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant. "
"Екрани"

MAltF1
l:
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda "
"Ліва"

MAltF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha "
"Права"

MAltF3
"Смотр."
"View.."
"Zobr.."
"Betr.."
"Néző.."
"Zobacz"
"Ver..."
"Див."

MAltF4
"Редак."
"Edit.."
"Edit.."
"Bear.."
"Szrk.."
"Edytuj"
"Edita."
"Редаг."

MAltF5
"Печать"
"Print"
"Tisk"
"Druck"
"Nyomt"
"Drukuj"
"Imprim"
"Друк"

MAltF6
"Ссылка"
"MkLink"
"VytLnk"
"LinkEr"
"ÚjLink"
"Dowiąż"
"CrVinc"
"Посилн"

MAltF7
"Искать"
"Find"
"Hledat"
"Suchen"
"Keres"
"Znajdź"
"Buscar"
"Шукати"

MAltF8
"Истор"
"Histry"
"Histor"
"Histor"
"ParElő"
"Histor"
"Histor"
"Істор"

MAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Tryb"
"Video"
"Відео""

MAltF10
"Дерево"
"Tree"
"Strom"
"Baum"
"MapKer"
"Drzewo"
"Arbol"
"Дерево"

MAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"HsPodg"
"HisVer"
"ІстПр"

MAltF12
"ИстПап"
"FoldHs"
"AdrsHs"
"BearHs"
"MapElő"
"HsKat"
"HisDir"
"ІстТек"

MCtrlF1
l:
"Левая"
"Left"
"Levý"
"Links"
"Bal"
"Lewy"
"Izqda "
"Ліва"

MCtrlF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha "
"Права"

MCtrlF3
"Имя   "
"Name  "
"Název "
"Name  "
"Név"
"Nazwa"
"Nombre"
"Ім'я"

MCtrlF4
"Расшир"
"Extens"
"Přípon"
"Erweit"
"Kiterj"
"Rozsz"
"Extens"
"Розшир"

MCtrlF5
"Запись"
"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
"Запис"

MCtrlF6
"Размер"
"Size"
"Veliko"
"Größe"
"Méret"
"Rozm"
"Tamaño"
"Розмір"

MCtrlF7
"Несорт"
"Unsort"
"Neřadi"
"Unsort"
"NincsR"
"BezSor"
"SinOrd"
"Несорт"

MCtrlF8
"Создан"
"Creatn"
"Vytvoř"
"Erstel"
"Keletk"
"Utworz"
"Creado"
"Стврно"

MCtrlF9
"Доступ"
"Access"
"Přístu"
"Zugrif"
"Hozzáf"
"Użycie"
"Acceso"
"Доступ"

MCtrlF10
"Описан"
"Descr"
"Popis"
"Beschr"
"Megjgy"
"Opis"
"Descr"
"Опсний"

MCtrlF11
"Владел"
"Owner"
"Vlastn"
"Besitz"
"Tulajd"
"Właśc"
"Dueño"
"Володр"

MCtrlF12
"Сорт"
"Sort"
"Třídit"
"Sort."
"RendMd"
"Sortuj"
"Orden"
"Сорт"

MShiftF1
l:
"Добавл"
"Add"
"Přidat"
"Hinzu"
"Tömört"
"Dodaj"
"Añadir"
"Добавл"

MShiftF2
"Распак"
"Extrct"
"Rozbal"
"Extrah"
"Kibont"
"Rozpak"
"Extrae"
"Розпак"

MShiftF3
"АрхКом"
"ArcCmd"
"ArcPří"
"ArcBef"
"TömPar"
"Polec"
"ArcCmd"
"АрхКом"

MShiftF4
"Редак."
"Edit.."
"Edit.."
"Erst.."
"ÚjFájl"
"Edytuj"
"Editar"
"Редаг."

MShiftF5
"Копир"
"Copy"
"Kopír."
"Kopier"
"Másol"
"Kopiuj"
"Copiar"
"Копію."

MShiftF6
"Переим"
"Rename"
"Přejme"
"Umbene"
"ÁtnMoz"
"ZmNazw"
"RenMov"
"Перейм"

MShiftF7
""
""
""
""
""
""
""
""

MShiftF8
"Удален"
"Delete"
"Smazat"
"Lösch."
"Töröl"
"Usuń"
"Borrar"
"Видалн"

MShiftF9
"Сохран"
"Save"
"Uložit"
"Speich"
"Mentés"
"Zapisz"
"Guarda"
"Збереж"

MShiftF10
"Послдн"
"Last"
"Posled"
"Letzte"
"UtsMnü"
"Ostatn"
"Ultimo"
"Останн"

MShiftF11
"Группы"
"Group"
"Skupin"
"Gruppe"
"Csoprt"
"Grupa"
"Grupo"
"Групи"

MShiftF12
"Выбран"
"SelUp"
"VybPrv"
"AuswOb"
"KijFel"
"SelUp"
"SelUp"
"Обранй"

MAltShiftF1
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

MAltShiftF2
""
""
""
""
""
""
""
""

MAltShiftF3
""
""
""
""
""
""
""
""

MAltShiftF4
""
""
""
""
""
""
""
""

MAltShiftF5
""
""
""
""
""
""
""
""

MAltShiftF6
""
""
""
""
""
""
""
""

MAltShiftF7
""
""
""
""
""
""
""
""

MAltShiftF8
""
""
""
""
""
""
""
""

MAltShiftF9
"КонфПл"
"ConfPl"
"KonfPl"
"KonfPn"
"PluKnf"
"KonfPl"
"ConfPl"
"КонфПл"

MAltShiftF10
""
""
""
""
""
""
""
""

MAltShiftF11
""
""
""
""
""
""
""
""

MAltShiftF12
""
""
""
""
""
""
""
""

MCtrlShiftF1
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

MCtrlShiftF2
""
""
""
""
""
""
""
""

MCtrlShiftF3
"Просм"
"View"
"Zobraz"
"Betr"
"Megnéz"
"Podgląd"
"Ver "
"Пергл"

MCtrlShiftF4
"Редакт"
"Edit"
"Edit"
"Bearb"
"Szerk."
"Edycja"
"Editar"
"Редаг"

MCtrlShiftF5
""
""
""
""
""
""
""
""

MCtrlShiftF6
""
""
""
""
""
""
""
""

MCtrlShiftF7
""
""
""
""
""
""
""
""

MCtrlShiftF8
""
""
""
""
""
""
""
""

MCtrlShiftF9
""
""
""
""
""
""
""
""

MCtrlShiftF10
""
""
""
""
""
""
""
""

MCtrlShiftF11
""
""
""
""
""
""
""
""

MCtrlShiftF12
""
""
""
""
""
""
""
""

MCtrlAltF1
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

MCtrlAltF2
""
""
""
""
""
""
""
""

MCtrlAltF3
""
""
""
""
""
""
""
""

MCtrlAltF4
""
""
""
""
""
""
""
""

MCtrlAltF5
""
""
""
""
""
""
""
""

MCtrlAltF6
""
""
""
""
""
""
""
""

MCtrlAltF7
""
""
""
""
""
""
""
""

MCtrlAltF8
""
""
""
""
""
""
""
""

MCtrlAltF9
""
""
""
""
""
""
""
""

MCtrlAltF10
""
""
""
""
""
""
""
""

MCtrlAltF11
""
""
""
""
""
""
""
""

MCtrlAltF12
""
""
""
""
""
""
""
""

MCtrlAltShiftF1
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

MCtrlAltShiftF2
""
""
""
""
""
""
""
""

MCtrlAltShiftF3
""
""
""
""
""
""
""
""

MCtrlAltShiftF4
""
""
""
""
""
""
""
""

MCtrlAltShiftF5
""
""
""
""
""
""
""
""

MCtrlAltShiftF6
""
""
""
""
""
""
""
""

MCtrlAltShiftF7
""
""
""
""
""
""
""
""

MCtrlAltShiftF8
""
""
""
""
""
""
""
""

MCtrlAltShiftF9
""
""
""
""
""
""
""
""

MCtrlAltShiftF10
""
""
""
""
""
""
""
""

MCtrlAltShiftF11
""
""
""
""
""
""
""
""

MCtrlAltShiftF12
le://End of functional keys
""
""
""
""
""
""
""
""

MHistoryTitle
l:
"История команд"
"History"
"Historie"
"Historie der letzten Befehle"
"Parancs előzmények"
"Historia"
"Historial"
"Історія команд"


MFolderHistoryTitle
"История папок"
"Folders history"
"Historie adresářů"
"Zuletzt besuchte Ordner"
"Mappa előzmények"
"Historia katalogów"
"Historial directorios"
"Історія тек"

MViewHistoryTitle
"История просмотра"
"File view history"
"Historie prohlížení souborů"
"Zuletzt betrachtete Dateien"
"Fájl előzmények"
"Historia podglądu plików"
"Historial visor"
"Історія перегляду"

MViewHistoryIsCreate
"Создать файл?"
"Create file?"
"Vytvořit soubor?"
"Datei erstellen?"
"Fájl létrehozása?"
"Utworzyć plik?"
"Crear archivo?"
"Створити файл?"

MHistoryView
"Просмотр"
"View"
"Zobrazit"
"Betr"
"Nézett"
"Zobacz"
"Ver   "
"Перегляд"

MHistoryEdit
"Редактор"
"Edit"
"Editovat"
"Bearb"
"Szerk."
"Edytuj"
"Editar"
"Редактор"

MHistoryExt
"Внешний"
"Ext."
"Rozšíření"
"Ext."
"Kit."
"Ext."
"Ext."
"Зовнішній"

MHistoryClear
l:
"История будет полностью очищена. Продолжить?"
"All records in the history will be deleted. Continue?"
"Všechny záznamy v historii budou smazány. Pokračovat?"
"Die gesamte Historie wird gelöscht. Fortfahren?"
"Az előzmények minden eleme törlődik. Folytatja?"
"Wszystkie wpisy historii będą usunięte. Kontynuować?"
"Los datos en el historial serán borrados. Continuar?"
"Історія буде повністю очищена. Продовжити?"


MClear
"&Очистить"
"&Clear history"
"&Vymazat historii"
"Historie &löschen"
"Elő&zmények törlése"
"&Czyść historię"
"&Limpiar historial"
"&Очистити"

MConfigSystemTitle
l:
"Системные параметры"
"System settings"
"Nastavení systému"
"Grundeinstellungen"
"Rendszer beállítások"
"Ustawienia systemowe"
"Opciones de sistema"
"Системні параметри"

MConfigRO
"&Снимать атрибут R/O c CD файлов"
"&Clear R/O attribute from CD files"
"Z&rušit atribut R/O u souborů na CD"
"Schreibschutz von CD-Dateien ent&fernen"
"&Csak olvasható attr. törlése CD fájlokról"
"Wyczyść atrybut &R/O przy kopiowaniu z CD"
"&Borrar atributos R/O de archivos de CD"
"&Зняти атрибут R/O з CD файлів"

MConfigSudoEnabled
"Разрешить повышение привилегий"
"Enable s&udo privileges elevation"
upd:"Enable sudo privileges elevation"
upd:"Enable sudo privileges elevation"
upd:"Enable sudo privileges elevation"
upd:"Enable sudo privileges elevation"
upd:"Enable sudo privileges elevation"
"Дозволити підвишення привілеїв"

MConfigSudoConfirmModify
"Подтверждать все операции записи"
"Always confirm modify operations"
upd:"Always confirm modify operations"
upd:"Always confirm modify operations"
upd:"Always confirm modify operations"
upd:"Always confirm modify operations"
upd:"Always confirm modify operations"
"Підтверджувати усі операції запису"

MConfigSudoPasswordExpiration
"Время действия пароля (сек):"
"Password expiration (sec):"
upd:"Password expiration (sec):"
upd:"Password expiration (sec):"
upd:"Password expiration (sec):"
upd:"Password expiration (sec):"
upd:"Password expiration (sec):"
"Час дії пароля (сек):"

MSudoTitle
"Операция требует повышения привилегий"
"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
upd:"Operation requires priviledges elevation"
"Операція вимагає підвищення привілеїв"

MSudoPrompt
"Введите пароль для sudo"
"Enter sudo password"
upd:"Enter sudo password"
upd:"Enter sudo password"
upd:"Enter sudo password"
upd:"Enter sudo password"
upd:"Enter sudo password"
"Введіть пароль для sudo"

MSudoConfirm
"Подтвердите использование привилегий"
"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
upd:"Confirm elevated priviledges use"
"Підтвердіть використання привілеїв"

MConfigRecycleBin
"Удалять в &Корзину"
"&Delete to Recycle Bin"
"&Mazat do Koše"
"In Papierkorb &löschen"
"&Törlés a Lomtárba"
"&Usuwaj do Kosza"
"Borrar hacia &papelera de reciclaje"
"Видаляти у &Кошик"

MConfigRecycleBinLink
"У&далять символические ссылки"
"Delete symbolic &links"
"Mazat symbolické &linky"
"Symbolische L&inks löschen"
"Szimbolikus l&inkek törlése"
"Usuń &linki symboliczne"
"Borrar en&laces simbólicos"
"В&идаляти символічні посилання"

MCopyWriteThrough
"Выключить кэ&ширование записи"
"Disable &write cache"
upd:"Disable &write cache"
upd:"Disable &write cache"
upd:"Disable &write cache"
upd:"Disable &write cache"
upd:"Disable &write cache"
"Вимкнути ке&шування запису"

MCopyXAttr
"Копировать расширенные а&ттрибуты"
"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
upd:"Copy extended a&ttributes"
"Копіювати розширені а&трибути"

MConfigOnlyFilesSize
"Учитывать только размер файлов"
"Use only files size in estimation"
upd:"Use only files size in estimation"
upd:"Use only files size in estimation"
upd:"Use only files size in estimation"
upd:"Use only files size in estimation"
upd:"Use only files size in estimation"
"Враховувати лише розмір файлів"

MConfigScanJunction
"Ск&анировать символические ссылки"
"Scan s&ymbolic links"
"Prohledávat s&ymbolické linky"
"S&ymbolische Links scannen"
"Szimbolikus linkek &vizsgálata"
"Skanuj linki s&ymboliczne"
"Explorar enlaces simbólicos"
"Ск&анувати символічні посилання"

MConfigInactivity
"&Время бездействия"
"&Inactivity time"
"&Doba nečinnosti"
"Inaktivitäts&zeit"
"A FAR kilé&p"
"Czas &bezczynności"
"Desact&ivar FAR en..."
"&Час бездіяльності"

MConfigInactivityMinutes
"минут"
"minutes"
"minut"
"Minuten"
"perc tétlenség után"
"&minut"
"minutos"
"хвилин"

MConfigSaveHistory
"Сохранять &историю команд"
"Save commands &history"
"Ukládat historii &příkazů"
"&Befehlshistorie speichern"
"Parancs elő&zmények mentése"
"Zapisz historię &poleceń"
"Guardar &historial de comandos"
"Зберігати &історію команд"

MConfigSaveFoldersHistory
"Сохранять историю п&апок"
"Save &folders history"
"Ukládat historii &adresářů"
"&Ordnerhistorie speichern"
"M&appa előzmények mentése"
"Zapisz historię &katalogów"
"Guardar historial de directorios"
"Зберігати історію т&ек"

MConfigSaveViewHistory
"Сохранять историю п&росмотра и редактора"
"Save &view and edit history"
"Ukládat historii Zobraz a Editu&j"
"Betrachter/&Editor-Historie speichern"
"Nézőke és &szerkesztő előzmények mentése"
"Zapisz historię podglądu i &edycji"
"Guardar historial de &visor y editor"
"Зберігати історію п&ерегляду та редагування"

MConfigRegisteredTypes
"Использовать стандартные &типы файлов"
"Use Windows &registered types"
"Používat regi&strované typy Windows"
"&Registrierte Windows-Dateitypen verwenden"
"&Windows reg. fájltípusok használata"
"Użyj zare&jestrowanych typów Windows"
"Usar extensiones &registradas de Windows"
"Використовувати стандартні &типи файлів"

MConfigCloseCDGate
"Автоматически монтироват&ь CDROM"
"CD drive auto &mount"
"Automatické př&ipojení CD disků"
"CD-Laufwerk auto&matisch schließen"
"CD tálca a&utomatikus behúzása"
"&Montuj CD automatycznie"
"CD-ROM: automontar unidad"
"Автоматично монтуват&и CDROM"

MConfigUpdateEnvironment
"Автообновление переменных окружения"
"Automatic update of environment variables"
upd:"Automatic update of environment variables"
upd:"Automatic update of environment variables"
upd:"Automatic update of environment variables"
upd:"Automatic update of environment variables"
upd:"Automatic update of environment variables"
"Автооновленння змінних оточення"

MConfigAutoSave
"Автозапись кон&фигурации"
"Auto &save setup"
"Automatické ukládaní &nastavení"
"Setup automatisch &"speichern"
"B&eállítások automatikus mentése"
"Automatycznie &zapisuj ustawienia"
"Auto&guardar configuración"
"Автозапис кон&фігурації"

MConfigPanelTitle
l:
"Настройки панели"
"Panel settings"
"Nastavení panelů"
"Panels einrichten"
"Panel beállítások"
"Ustawienia panelu"
"Configuración de paneles"
"Налаштування панели"

MConfigHidden
"Показывать скр&ытые и системные файлы"
"Show &hidden and system files"
"Ukázat &skryté a systémové soubory"
"&Versteckte und Systemdateien anzeigen"
"&Rejtett és rendszerfájlok mutatva"
"Pokazuj pliki &ukryte i systemowe"
"Mostrar archivos ocultos y de sistema"
"Показувати при&ховані та системні файли"

MConfigHighlight
"&Раскраска файлов"
"Hi&ghlight files"
"Zvý&razňovat soubory"
"Dateien mark&ieren"
"Fá&jlok kiemelése"
"W&yróżniaj pliki"
"Resaltar archivos"
"&Розфарбовка файлів"

MConfigAutoChange
"&Автосмена папки"
"&Auto change folder"
"&Automaticky měnit adresář"
"Ordner &automatisch wechseln (Baumansicht)"
"&Automatikus mappaváltás"
"&Automatycznie zmieniaj katalog"
"&Auto cambiar directorio"
"&Автозміна теки"

MConfigSelectFolders
"Пометка &папок"
"Select &folders"
"Vybírat a&dresáře"
"&Ordner auswählen"
"A ma&ppák is kijelölhetők"
"Zaznaczaj katalo&gi"
"Seleccionar &directorios"
"Позначка &тек"

MConfigSortFolderExt
"Сортировать имена папок по рас&ширению"
"Sort folder names by e&xtension"
"Řadit adresáře podle přípony"
"Ordner nach Er&weiterung sortieren"
"Mappák is rendezhetők &kiterjesztés szerint"
"Sortuj nazwy katalogów wg r&ozszerzeń"
"Ordenar directorios por extensión"
"Сортувати імена папок по роз&ширенню"

MConfigReverseSort
"Разрешить &обратную сортировку"
"Allow re&verse sort modes"
"Do&volit změnu směru řazení"
"&Umgekehrte Sortiermodi zulassen"
"Fordí&tott rendezés engedélyezése"
"Włącz &możliwość odwrotnego sortowania"
"Permitir modo de orden in&verso"
"Дозволити &зворотне сортування"

MConfigAutoUpdateLimit
"Отключать автооб&новление панелей,"
"&Disable automatic update of panels"
"Vypnout a&utomatickou aktualizaci panelů"
"Automatisches Panelupdate &deaktivieren"
"Pan&el automatikus frissítése kikapcsolva,"
"&Wyłącz automatyczną aktualizację paneli"
"Desactiva actualización automát. de &paneles"
"Відключати автоо&новлення панелей,"

MConfigAutoUpdateLimit2
"если объектов больше"
"if object count exceeds"
"jestliže počet objektů překročí"
"wenn mehr Objekte als"
"ha több elem van, mint:"
"jeśli zawierają więcej obiektów niż"
"si conteo de objetos es excedido"
"якщо об'ектів більше"

MConfigAutoUpdateRemoteDrive
"Автообновление с&етевых дисков"
"Network drives autor&efresh"
"Automatická obnova síťových disků"
"Netzw&erklauferke autom. aktualisieren"
"Hálózati meghajtók autom. &frissítése"
"Auto&odświeżanie dysków sieciowych"
"Autor&efrescar unidades de Red"
"Автооновлення м&ережевих дисків"

MConfigShowColumns
"Показывать &заголовки колонок"
"Show &column titles"
"Zobrazovat &nadpisy sloupců"
"S&paltentitel anzeigen"
"Oszlop&nevek mutatva"
"Wyświetl tytuły &kolumn"
"Mostrar títulos de &columnas"
"Показувати &заголовки колонок"

MConfigShowStatus
"Показывать &строку статуса"
"Show &status line"
"Zobrazovat sta&vový řádek"
"&Statuszeile anzeigen"
"Á&llapotsor mutatva"
"Wyświetl &linię statusu"
"Mostrar línea de e&stado"
"Показувати &рядок статусу"

MConfigShowTotal
"Показывать су&ммарную информацию"
"Show files &total information"
"Zobrazovat &informace o velikosti souborů"
"&Gesamtzahl für Dateien anzeigen"
"Fájl össze&s információja mutatva"
"Wyświetl &całkowitą informację o plikach"
"Mostrar información comple&ta de archivos"
"Показувати су&марну інформацію"

MConfigShowFree
"Показывать с&вободное место"
"Show f&ree size"
"Zobrazovat vo&lné místo"
"&Freien Speicher anzeigen"
"Sza&bad lemezterület mutatva"
"Wyświetl ilość &wolnego miejsca"
"Mostrar espacio lib&re"
"Показувати в&ільне місце"

MConfigShowScrollbar
"Показывать по&лосу прокрутки"
"Show scroll&bar"
"Zobrazovat &posuvník"
"Scroll&balken anzeigen"
"Gördítősá&v mutatva"
"Wyświetl &suwak"
"Mostrar &barra de desplazamiento"
"Показувати см&угу прокручування"

MConfigShowScreensNumber
"Показывать количество &фоновых экранов"
"Show background screens &number"
"Zobrazovat počet &obrazovek na pozadí"
"&Nummer von Hintergrundseiten anzeigen"
"&Háttérképernyők száma mutatva"
"Wyświetl ilość &ekranów w tle"
"Mostrar &número de pantallas de fondo"
"Показувати кількість &фонових екранів"

MConfigShowSortMode
"Показывать букву режима сор&тировки"
"Show sort &mode letter"
"Zobrazovat písmeno &módu řazení"
"Buchstaben der Sortier&modi anzeigen"
"Rendezési mó&d betűjele mutatva"
"Wyświetl l&iterę trybu sortowania"
"Mostrar letra para &modo de orden"
"Показувати літеру режиму сор&тування"

MConfigInterfaceTitle
l:
"Настройки интерфейса"
"Interface settings"
"Nastavení rozhraní"
"Oberfläche einrichten"
"Kezelőfelület beállítások"
"Ustawienia interfejsu"
"Opciones de interfaz"
"Налаштування інтерфейсу"

MConfigInputTitle
l:
"Настройки ввода"
"Input settings"
upd:"Input settings"
upd:"Input settings"
upd:"Input settings"
upd:"Input settings"
upd:"Input settings"
upd:"Настройки ввода"

MConfigClock
"&Часы в панелях"
"&Clock in panels"
"&Hodiny v panelech"
"&Uhr in Panels anzeigen"
"Ór&a a paneleken"
"&Zegar"
"&Reloj en paneles"
"&Годинник у панелях"

MConfigViewerEditorClock
"Ч&асы при редактировании и просмотре"
"C&lock in viewer and editor"
"H&odiny v prohlížeči a editoru"
"U&hr in Betrachter und Editor anzeigen"
"Ó&ra a nézőkében és szerkesztőben"
"Zegar w &podglądzie i edytorze"
"Re&loj en visor y editor"
"Г&одинник під час редагування та перегляду"

MConfigMouse
"Мы&шь"
"M&ouse"
"M&yš"
"M&aus aktivieren"
"&Egér kezelése"
"M&ysz"
"Rat&ón"
"Ми&ша"

MConfigXLats
"Правила &транслитерации:"
"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"&Transliteration ruleset:"
upd:"Правила &транслитерации:"

MConfigXLatDialogs
"Транслитерация &диалоговой навигации"
"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Enable &dialog navigation transliteration"
upd:"Транслитерация &диалоговой навигации"

MConfigXLatFastFileFind
"Транслитерация быстрого поиска &файла"
"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Enable fast &file find transliteration"
upd:"Транслитерация быстрого поиска &файла"

MConfigKeyBar
"Показывать &линейку клавиш"
"Show &key bar"
"Zobrazovat &zkratkové klávesy"
"Tast&enleiste anzeigen"
"&Funkcióbillentyűk sora mutatva"
"Wyświetl pasek &klawiszy"
"Mostrar barra de &funciones"
"Показувати &лінійку клавіш"

MConfigMenuBar
"Всегда показывать &меню"
"Always show &menu bar"
"Vždy zobrazovat hlavní &menu"
"&Menüleiste immer anzeigen"
"A &menüsor mindig látszik"
"Zawsze pokazuj pasek &menu"
"Mostrar siempre barra de &menú"
"Завжди показувати &меню"

MConfigSaver
"&Сохранение экрана"
"&Screen saver"
"Sp&ořič obrazovky"
"Bildschirm&schoner"
"&Képernyőpihentető"
"&Wygaszacz ekranu"
"&Salvapantallas"
"&Збереження екрана"

MConfigSaverMinutes
"минут"
"minutes"
"minut"
"Minuten"
"perc tétlenség után"
"m&inut"
"minutos"
"хвилин"

MConfigConsoleChangeFont
"Выбрать шри&фт"
"Change &font"
upd:"Change &font"
upd:"Change &font"
upd:"Change &font"
upd:"Change &font"
upd:"Change &font"
"Вибрати шри&фт"

MConfigConsolePaintSharp
"Отключить сглаживание"
"Disable antialiasing"
upd:"Disable antialiasing"
upd:"Disable antialiasing"
upd:"Disable antialiasing"
upd:"Disable antialiasing"
upd:"Disable antialiasing"
"Відключити згладжування"

MConfigExclusiveKeys
"&Экслюзивная обработка нажатий, включающих:"
"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
upd:"&Exclusively handle hotkeys that include:"
"&Ексклюзивне оброблення натискань, що включають:"

MConfigExclusiveCtrlLeft
"Левый Ctrl"
"Left Ctrl"
upd:"Left Ctrl"
upd:"Left Ctrl"
upd:"Left Ctrl"
upd:"Left Ctrl"
upd:"Left Ctrl"
"Лівий Ctrl"

MConfigExclusiveCtrlRight
"Правый Ctrl"
"Right Ctrl"
upd:"Right Ctrl"
upd:"Right Ctrl"
upd:"Right Ctrl"
upd:"Right Ctrl"
upd:"Right Ctrl"
"Правий Ctrl"

MConfigExclusiveAltLeft
"Левый Alt "
"Left Alt "
upd:"Left Alt "
upd:"Left Alt "
upd:"Left Alt "
upd:"Left Alt "
upd:"Left Alt "
"Лівий Alt "

MConfigExclusiveAltRight
"Правый Alt "
"Right Alt "
upd:"Right Alt "
upd:"Right Alt "
upd:"Right Alt "
upd:"Right Alt "
upd:"Right Alt "
"Правий Alt "

MConfigExclusiveWinLeft
"Левый Win "
"Left Win "
upd:"Left Win "
upd:"Left Win "
upd:"Left Win "
upd:"Left Win "
upd:"Left Win "
"Лівий Win "

MConfigExclusiveWinRight
"Правый Win "
"Right Win "
upd:"Right Win "
upd:"Right Win "
upd:"Right Win "
upd:"Right Win "
upd:"Right Win "
"Правий Win "

MConfigCopyTotal
"Показывать &общий индикатор копирования"
"Show &total copy progress indicator"
"Zobraz. ukazatel celkového stavu &kopírování"
"Zeige Gesamtfor&tschritt beim Kopieren"
"Másolás összesen folyamat&jelző"
"Pokaż &całkowity postęp kopiowania"
"Mostrar indicador de progreso de copia &total"
"Показувати &загальний індикатор копіювання"

MConfigCopyTimeRule
"Показывать информацию о времени &копирования"
"Show cop&ying time information"
"Zobrazovat informace o čase kopírování"
"Zeige Rest&zeit beim Kopieren"
"Má&solási idő mutatva"
"Pokaż informację o c&zasie kopiowania"
"Mostrar información de tiempo de copiado"
"Показувати інформацію про час &копіювання"

MConfigDeleteTotal
"Показывать общий индикатор удаления"
"Show total delete progress indicator"
upd:"Show total delete progress indicator"
upd:"Show total delete progress indicator"
upd:"Show total delete progress indicator"
upd:"Show total delete progress indicator"
"Mostrar indicador de progreso de borrado total"
"Показувати загальний індикатор видалення"

MConfigPgUpChangeDisk
"Использовать Ctrl-PgUp для в&ыбора диска"
"Use Ctrl-Pg&Up to change drive"
"Použít Ctrl-Pg&Up pro změnu disku"
"Strg-Pg&Up wechselt das Laufwerk"
"A Ctrl-Pg&Up meghajtót vált"
"Użyj Ctrl-Pg&Up do zmiany napędu"
"Usar Ctrl-Pg&Up para cambiar unidad"
"Використовувати Ctrl-PgUp для в&ибору диска"

MConfigTitleAddons
upd:"Far window title addons:"
"Far window title addons:"
upd:"Far window title addons:"
upd:"Far window title addons:"
upd:"Far window title addons:"
upd:"Far window title addons:"
"Título de ventana de FAR:"
"доповнення заголовків вікна FAR:"

MConfigDlgSetsTitle
l:
"Настройки диалогов"
"Dialog settings"
"Nastavení dialogů"
"Dialoge einrichten"
"Párbeszédablak beállítások"
"Ustawienia okien dialogowych"
"Opciones de diálogo"
"Налаштування діалогів"

MConfigDialogsEditHistory
"&История в строках ввода диалогов"
"&History in dialog edit controls"
"H&istorie v dialozích"
"&Historie in Eingabefelder von Dialogen"
"&Beviteli sor előzmények mentése"
"&Historia w polach edycyjnych"
"&Historial en controles de diálogo de edición"
"&Історія у рядках введення діалогів"

MConfigMaxHistoryCount
"&Макс количество записей:"
"&Max history items:"
upd:"&Max history items:"
upd:"&Max history items:"
upd:"&Max history items:"
upd:"&Max history items:"
upd:"&Max history items:"
"&Макс кількість записів:"

MConfigDialogsEditBlock
"&Постоянные блоки в строках ввода"
"&Persistent blocks in edit controls"
"&Trvalé bloky v editačních polích"
"Dauer&hafte Markierungen in Eingabefelder"
"Maradó b&lokkok a beviteli sorokban"
"&Trwałe bloki podczas edycji"
"&Bloques persistentes en controles de edición"
"&Постійні блоки в рядках введення"

MConfigDialogsDelRemovesBlocks
"Del удаляет б&локи в строках ввода"
"&Del removes blocks in edit controls"
"&Del maže položky v editačních polích"
"&Entf löscht Markierungen"
"A &Del törli a beviteli sorok blokkjait"
"&Del usuwa blok podczas edycji"
"&Del remueve bloques en controles de edición"
"Del видаляє б&локи в рядках введення"

MConfigDialogsAutoComplete
"&Автозавершение в строках ввода"
"&AutoComplete in edit controls"
"Automatické dokončování v editač&ních polích"
"&Automatisches Vervollständigen"
"Beviteli sor a&utomatikus kiegészítése"
"&Autouzupełnianie podczas edycji"
"Autocompl&etar en controles de edición"
"&Автозавершення в рядках введення"

MConfigDialogsEULBsClear
"Backspace &удаляет неизмененный текст"
"&Backspace deletes unchanged text"
"&Backspace maže nezměněný text"
"&Rücktaste (BS) löscht unveränderten Text"
"A Ba&ckspace törli a változatlan szöveget"
"&Backspace usuwa nie zmieniony tekst"
"&Backspace elimina texto no cambiado"
"Backspace &видаляє незмінений текст"

MConfigDialogsMouseButton
"Клик мыши &вне диалога закрывает диалог"
"Mouse click &outside a dialog closes it"
"Kl&iknutí myší mimo dialog ho zavře"
"Dial&og schließen wenn Mausklick ausserhalb"
"&Egérkattintás a párb.ablakon kívül: bezárja"
"&Kliknięcie myszy poza oknem zamyka je"
"Click en ratón afuera del diálogo lo cierra"
"Клік миши &поза діалогом закриває діалог"

MConfigVMenuTitle
l:
"Настройки меню"
"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
"Налаштування меню"

MConfigVMenuLBtnClick
"Клик левой кнопки мыши вне меню"
"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
upd:"Left mouse click outside a menu"
"Клік лівої кнопки миши поза меню"

MConfigVMenuRBtnClick
"Клик правой кнопки мыши вне меню"
"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
upd:"Right mouse click outside a menu"
"Клік правої кнопки миши поза меню"

MConfigVMenuMBtnClick
"Клик средней кнопки мыши вне меню"
"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
upd:"Middle mouse click outside a menu"
"Клік середньої кнопки миши поза меню"

MConfigVMenuClickCancel
"Закрыть с отменой"
"Cancel menu"
upd:"Cancel menu"
upd:"Cancel menu"
upd:"Cancel menu"
upd:"Cancel menu"
upd:"Cancel menu"
"Закрити зі скасуванням"

MConfigVMenuClickApply
"Выполнить текущий пункт"
"Execute selected item"
upd:"Execute selected item"
upd:"Execute selected item"
upd:"Execute selected item"
upd:"Execute selected item"
upd:"Execute selected item"
"Виконати поточний пункт"

MConfigVMenuClickIgnore
"Ничего не делать"
"Do nothing"
upd:"Do nothing"
upd:"Do nothing"
upd:"Do nothing"
upd:"Do nothing"
upd:"Do nothing"
"Нічого не робити"

MConfigCmdlineTitle
l:
"Настройки командной строки"
"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
"Opciones de línea de comando"
"Налаштування командного рядка"

MConfigCmdlineEditBlock
"&Постоянные блоки"
"&Persistent blocks"
upd:"Persistent blocks"
upd:"Persistent blocks"
upd:"Persistent blocks"
upd:"Persistent blocks"
"Bloques &persistentes"
"&Постійні блоки"

MConfigCmdlineDelRemovesBlocks
"Del удаляет б&локи"
"&Del removes blocks"
upd:"Del removes blocks"
upd:"Del removes blocks"
upd:"Del removes blocks"
upd:"Del removes blocks"
"&Del remueve bloques"
"Del видаляє б&локи"

MConfigCmdlineAutoComplete
"&Автозавершение"
"&AutoComplete"
upd:"AutoComplete"
upd:"AutoComplete"
upd:"AutoComplete"
upd:"AutoComplete"
"&AutoCompletar"
"&Автозавершення"

MConfigCmdlineWaitKeypress
"&Ожидать нажатие перед закрытием"
"&Wait keypress before close"
upd:"&Wait keypress before close"
upd:"&Wait keypress before close"
upd:"&Wait keypress before close"
upd:"&Wait keypress before close"
upd:"&Wait keypress before close"
"&Очікувать натискання перед закриттям"

MConfigCmdlineWaitKeypress_Never
"Никогда"
"Never"
upd:"Never"
upd:"Never"
upd:"Never"
upd:"Never"
"Never"
"Ніколи"

MConfigCmdlineWaitKeypress_OnError
"При ошибке"
"On error"
upd:"On error"
upd:"On error"
upd:"On error"
upd:"On error"
"On error"
"При помилці"

MConfigCmdlineWaitKeypress_Always
"Всегда"
"Always"
upd:"Always"
upd:"Always"
upd:"Always"
upd:"Always"
"Always"
"Завжди"

MConfigCmdlineUseShell
"Использовать &шелл"
"Use &shell"
upd:"Use shell"
upd:"Use shell"
upd:"Use shell"
upd:"Use shell"
upd:"Use shell"
"Використовувати &шелл"

MConfigCmdlineUsePromptFormat
"Установить &формат командной строки"
"Set command line &prompt format"
"Nastavit formát &příkazového řádku"
"&Promptformat der Kommandozeile"
"Parancssori &prompt formátuma"
"Wy&gląd znaku zachęty linii poleceń"
"Formato para línea de comando (&prompt)"
"Встановити &формат командного рядка"

MConfigCmdlinePromptFormatAdmin
"Администратор"
"Administrator"
upd:"Administrator"
upd:"Administrator"
upd:"Administrator"
upd:"Administrator"
"Administrador"
"Адміністратор"

MConfigAutoCompleteTitle
l:
"Настройка автозавершения"
"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
"Opciones de autocompletar"
"Налаштування автозавершення"

MConfigAutoCompleteExceptions
l:
"Шаблоны &исключений"
"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
upd:"&Exceptions wildcards:"
"Шаблони &винятковий"

MConfigAutoCompleteShowList
l:
"Показывать &список"
"&Show list"
upd:"&Show list"
upd:"&Show list"
upd:"&Show list"
upd:"&Show list"
"Mo&strar lista"
"Показувати &список"

MConfigAutoCompleteModalList
l:
"&Модальный режим"
"&Modal mode"
upd:"&Modal mode"
upd:"&Modal mode"
upd:"&Modal mode"
upd:"&Modal mode"
"Clase de &Modo"
"&Модальний режим"

MConfigAutoCompleteAutoAppend
l:
"&Подставлять первый подходящий вариант"
"&Append first matched item"
upd:"&Append first matched item"
upd:"&Append first matched item"
upd:"&Append first matched item"
upd:"&Append first matched item"
"&Agregar primer ítem coincidente"
"&підставляти перший відповідний варіант"

MConfigInfoPanelTitle
l:
"Настройка информационной панели"
"InfoPanel settings"
upd:"InfoPanel settings"
upd:"InfoPanel settings"
upd:"InfoPanel settings"
upd:"InfoPanel settings"
"Opciones de panel de información"
"налаштування інформаційної панелі"

MConfigInfoPanelCNTitle
"Формат вывода имени &компьютера"
upd:"ComputerName &format"
upd:"ComputerName &format"
upd:"ComputerName &format"
upd:"ComputerName &format"
upd:"ComputerName &format"
"&Formato NombreComputadora"
"формат виведення імені &комп'ютера"

MConfigInfoPanelCNPhysicalNetBIOS
upd:"Physical NetBIOS"
upd:"Physical NetBIOS"
upd:"Physical NetBIOS"
upd:"Physical NetBIOS"
upd:"Physical NetBIOS"
upd:"Physical NetBIOS"
"NetBios físico"
"Фізичний NetBIOS"

MConfigInfoPanelCNPhysicalDnsHostname
upd:"Physical DNS hostname"
upd:"Physical DNS hostname"
upd:"Physical DNS hostname"
upd:"Physical DNS hostname"
upd:"Physical DNS hostname"
upd:"Physical DNS hostname"
"DNS hostname físico"
"фізичне ім'я хосту DNS"

MConfigInfoPanelCNPhysicalDnsDomain
upd:"Physical DNS domain"
upd:"Physical DNS domain"
upd:"Physical DNS domain"
upd:"Physical DNS domain"
upd:"Physical DNS domain"
upd:"Physical DNS domain"
"Dominio DNS físico"
"фізичний домен DNS"

MConfigInfoPanelCNPhysicalDnsFullyQualified
upd:"Physical DNS fully-qualified"
upd:"Physical DNS fully-qualified"
upd:"Physical DNS fully-qualified"
upd:"Physical DNS fully-qualified"
upd:"Physical DNS fully-qualified"
upd:"Physical DNS fully-qualified"
"DNS calificado físico"
"фізичний DNS повністю кваліфікований"

MConfigInfoPanelCNNetBIOS
upd:"NetBIOS"
upd:"NetBIOS"
upd:"NetBIOS"
upd:"NetBIOS"
upd:"NetBIOS"
upd:"NetBIOS"
"NetBios"
"NetBios"

MConfigInfoPanelCNDnsHostname
upd:"DNS hostname"
upd:"DNS hostname"
upd:"DNS hostname"
upd:"DNS hostname"
upd:"DNS hostname"
upd:"DNS hostname"
"DNS hostname"
"DNS ім'я хоста"

MConfigInfoPanelCNDnsDomain
upd:"DNS domain"
upd:"DNS domain"
upd:"DNS domain"
upd:"DNS domain"
upd:"DNS domain"
upd:"DNS domain"
"Dominio DNS"
"DNS домен"

MConfigInfoPanelCNDnsFullyQualified
upd:"DNS fully-qualified"
upd:"DNS fully-qualified"
upd:"DNS fully-qualified"
upd:"DNS fully-qualified"
upd:"DNS fully-qualified"
upd:"DNS fully-qualified"
"DNS Calificado"
upd:"DNS повністю кваліфікований"

MConfigInfoPanelUNTitle
"Формат вывода имени &пользователя"
upd:"UserName &format"
upd:"UserName &format"
upd:"UserName &format"
upd:"UserName &format"
upd:"UserName &format"
"&Formato nombre de usuario"
"Формат виводу імені &користувача"

MConfigInfoPanelUNUnknown
"По умолчанию"
"Default"
upd:"Default"
upd:"Default"
upd:"Default"
upd:"Default"
"Por defecto"
"За замовчуванням"

MConfigInfoPanelUNFullyQualifiedDN
"Полностью определённое имя домена"
"Fully Qualified Domain Name"
upd:"Fully Qualified Domain Name"
upd:"Fully Qualified Domain Name"
upd:"Fully Qualified Domain Name"
upd:"Fully Qualified Domain Name"
"Nombre dominio completamente calificado"
"повністю визначене ім'я домену"

MConfigInfoPanelUNSamCompatible
upd:"Sam Compatible"
upd:"Sam Compatible"
upd:"Sam Compatible"
upd:"Sam Compatible"
upd:"Sam Compatible"
upd:"Sam Compatible"
"Compatible con Sam"
upd:"Sam Compatible"

MConfigInfoPanelUNDisplay
upd:"Display Name"
upd:"Display Name"
upd:"Display Name"
upd:"Display Name"
upd:"Display Name"
upd:"Display Name"
"Mostrar Nombre"
upd:"Display Name"

MConfigInfoPanelUNUniqueId
"Уникальный идентификатор"
upd:"Unique Id"
upd:"Unique Id"
upd:"Unique Id"
upd:"Unique Id"
upd:"Unique Id"
"ID nico"
"Унікальний ідентифікатор"

MConfigInfoPanelUNCanonical
"Канонический вид"
"Canonical Name"
upd:"Canonical Name"
upd:"Canonical Name"
upd:"Canonical Name"
upd:"Canonical Name"
"Nombre Cannico"
"Каноничний вид"

MConfigInfoPanelUNUserPrincipal
"Основное имя пользователя"
upd:"User Principial Name"
upd:"User Principial Name"
upd:"User Principial Name"
upd:"User Principial Name"
upd:"User Principial Name"
"Nombre principal usuario"
"Основне ім'я користувача"

MConfigInfoPanelUNServicePrincipal
upd:"Service Principal"
upd:"Service Principal"
upd:"Service Principal"
upd:"Service Principal"
upd:"Service Principal"
upd:"Service Principal"
"Servicio principal"
upd:"Service Principal"

MConfigInfoPanelUNDnsDomain
upd:"Dns Domain"
upd:"Dns Domain"
upd:"Dns Domain"
upd:"Dns Domain"
upd:"Dns Domain"
upd:"Dns Domain"
"Dominio DNS"
upd:"Dns Domain"

MViewConfigTitle
l:
"Программа просмотра"
"Viewer"
"Prohlížeč"
"Betrachter"
"Nézőke"
"Podgląd"
"Visor"
"Програма перегляду"

MViewConfigExternalF3
"Запускать внешнюю программу просмотра по F3 вместо Alt-F3"
"Use external viewer for F3 instead of Alt-F3"
upd:"Use external viewer for F3 instead of Alt-F3"
upd:"Use external viewer for F3 instead of Alt-F3"
"Alt-F3 helyett F3 indítja a külső nézőkét"
upd:"Use external viewer for F3 instead of Alt-F3"
"Usar visor externo con F3 en lugar de Alt-F3"
"запускати зовнішню програму перегляду F3 замість Alt-F3"

MViewConfigExternalCommand
"&Команда просмотра:"
"&Viewer command:"
"&Příkaz prohlížeče:"
"Befehl für e&xternen Betracher:"
"Nézőke &parancs:"
"&Polecenie:"
"Comando &visor:"
"&Команда перегляду:"

MViewConfigInternal
" Встроенная программа просмотра "
" Internal viewer "
" Interní prohlížeč "
" Interner Betracher "
" Belső nézőke "
" Podgląd wbudowany "
"Visor interno"
" Вбудована программа перегляду "

MViewConfigSavePos
"&Сохранять позицию файла"
"&Save file position"
"&Ukládat pozici v souboru"
"Dateipositionen &speichern"
"&Fájlpozíció mentése"
"&Zapamiętaj pozycję w pliku"
"&Guardar posición de archivo"
"&Зберігати позицію файлу"

MViewConfigSaveShortPos
"Сохранять &закладки"
"Save &bookmarks"
"Ukládat &záložky"
"&Lesezeichen speichern"
"Könyv&jelzők mentése"
"Zapisz z&akładki"
"Guardar &marcadores"
"Зберігати &закладки"

MViewAutoDetectCodePage
"&Автоопределение кодовой страницы"
"&Autodetect code page"
upd:"&Autodetekovat znakovou sadu"
upd:"Zeichentabelle &automatisch erkennen"
"&Kódlap automatikus felismerése"
"Rozpozn&aj tablicę znaków"
"&Autodetectar tabla de caracteres"
"&Автовизначення кодової сторінки"

MViewConfigTabSize
"Раз&мер табуляции"
"Tab si&ze"
"Velikost &Tabulátoru"
"Ta&bulatorgröße"
"Ta&bulátor mérete"
"Rozmiar &tabulatora"
"Tamaño de &tabulación"
"Роз&мір табуляції"

MViewConfigScrollbar
"Показывать &полосу прокрутки"
"Show scro&llbar"
"Zobrazovat posu&vník"
"Scro&llbalken anzeigen"
"Gör&dítősáv mutatva"
"Pokaż &pasek przewijania"
"Mostrar barra de desp&lazamiento"
"Показувати &полосу прокрутки"

MViewConfigArrows
"Показывать стрелки с&двига"
"Show scrolling arro&ws"
"Zobrazovat &skrolovací šipky"
"P&feile bei Scrollbalken zeigen"
"Gördítőn&yilak mutatva"
"Pokaż strzał&ki przewijania"
"Mostrar flechas de despla&zamiento"
"Показувати стрілки з&суву"


MViewShowKeyBar
"Показывать &линейку клавиш"
"Show &key bar"
"Zobrazovat &zkratkové klávesy"
"Tast&enleiste anzeigen"
"&Funkcióbillentyűk sora mutatva"
"Wyświetl pasek &klawiszy"
"Mostrar barra de &funciones"
"Показувати &лінійку клавиш"

MViewShowTitleBar
"Показывать &заголовок"
"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
"Показувати &заголовок"

MViewConfigPersistentSelection
"Постоянное &выделение"
"&Persistent selection"
"Trvalé &výběry"
"Dauerhafte Text&markierungen"
"&Maradó blokkok"
"T&rwałe zaznaczenie"
"Selección &persistente"
"Постійне &виділення"

MViewConfigDefaultCodePage
"Выберите &кодовую страницу по умолчанию:"
"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
"Виберіть &кодову сторінку за замовчуванням:"

MEditConfigTitle
l:
"Редактор"
"Editor"
"Editor"
"Editor"
"Szerkesztő"
"Edytor"
"Editor"
"Редактор"

MEditConfigEditorF4
"Запускать внешний редактор по F4 вместо Alt-F4"
"Use external editor for F4 instead of Alt-F4"
upd:"Use external editor for F4 instead of Alt-F4"
upd:"Use external editor for F4 instead of Alt-F4"
"Alt-F4 helyett F4 indítja a külső szerkesztőt"
upd:"Use external editor for F4 instead of Alt-F4"
"Usar editor externo con F4 en lugar de Alt-F4"
"Запускати зовнішній редактор F4 замість Alt-F4"

MEditConfigEditorCommand
"&Команда редактирования:"
"&Editor command:"
"&Příkaz editoru:"
"Befehl für e&xternen Editor:"
"&Szerkesztő parancs:"
"&Polecenie:"
"Comando &editor:"
"&Команда редагування:"

MEditConfigInternal
" Встроенный редактор "
" Internal editor "
" Interní editor "
" Interner Editor "
" Belső szerkesztő "
" Edytor wbudowany "
"Editor interno"
" Вбудований редактор "

MEditConfigExpandTabsTitle
"Преобразовывать &табуляцию:"
"Expand &tabs:"
"Rozšířit Ta&bulátory mezerami"
"&Tabs expandieren:"
"&Tabulátorból szóközök:"
"Zamiana znaków &tabulacji:"
"Expandir &tabulación a espacios"
"Перетворити &табуляцію:"

MEditConfigDoNotExpandTabs
l:
"Не преобразовывать табуляцию"
"Do not expand tabs"
"Nerozšiřovat tabulátory mezerami"
"Tabs nicht expandieren"
"Ne helyettesítse a tabulátorokat"
"Nie zamieniaj znaków tabulacji"
"No expandir tabulacines"
"Не перетворювати табуляцію"

MEditConfigExpandTabs
"Преобразовывать новые символы табуляции в пробелы"
"Expand newly entered tabs to spaces"
"Rozšířit nově zadané tabulátory mezerami"
"Neue Tabs zu Leerzeichen expandieren"
"Újonnan beírt tabulátorból szóközök"
"Zamień nowo dodane znaki tabulacji na spacje"
"Expandir nuevas tabulaciones ingresadas a espacios"
"Перетворити нові символи табуляції на пробіли"

MEditConfigConvertAllTabsToSpaces
"Преобразовывать все символы табуляции в пробелы"
"Expand all tabs to spaces"
"Rozšířit všechny tabulátory mezerami"
"Alle Tabs zu Leerzeichen expandieren"
"Minden tabulátorból szóközök"
"Zastąp wszystkie tabulatory spacjami"
"Expandir todas las tabulaciones a espacios"
"Перетворити всі символи табуляції на пробіли"

MEditConfigPersistentBlocks
"&Постоянные блоки"
"&Persistent blocks"
"&Trvalé bloky"
"Dauerhafte Text&markierungen"
"&Maradó blokkok"
"T&rwałe bloki"
"Bloques &persistente"
"&Постійні блоки"

MEditConfigDelRemovesBlocks
l:
"Del удаляет б&локи"
"&Del removes blocks"
"&Del maže bloky"
"&Entf löscht Textmark."
"A &Del törli a blokkokat"
"&Del usuwa bloki"
"Del &remueve bloques"
"Del видаляє б&локи"

MEditConfigAutoIndent
"Авто&отступ"
"Auto &indent"
"Auto &Odsazování"
"Automatischer E&inzug"
"Automatikus &behúzás"
"Automatyczne &wcięcia"
"Auto &dentar"
"Авто&відступ"

MEditConfigSavePos
"&Сохранять позицию файла"
"&Save file position"
"&Ukládat pozici v souboru"
"Dateipositionen &speichern"
"Fájl&pozíció mentése"
"&Zapamiętaj pozycję kursora w pliku"
"&Guardar posición de archivo"
"&Зберігати позицію файлу"

MEditConfigSaveShortPos
"Сохранять &закладки"
"Save &bookmarks"
"Ukládat zá&ložky"
"&Lesezeichen speichern"
"Könyv&jelzők mentése"
"Zapisz &zakładki"
"Guardar &marcadores"
"Зберігати &закладки"

MEditCursorBeyondEnd
"Ку&рсор за пределами строки"
"&Cursor beyond end of line"
"&Kurzor za koncem řádku"
upd:"&Cursor hinter dem Ende"
"Kurzor a sor&végjel után is"
"&Kursor za końcem linii"
"&Cursor después de fin de línea"
"Ку&рсор за межами рядка"

MEditAutoDetectCodePage
"&Автоопределение кодовой страницы"
"&Autodetect code page"
upd:"&Autodetekovat znakovou sadu"
upd:"Zeichentabelle &automatisch erkennen"
"&Kódlap automatikus felismerése"
"Rozpozn&aj tablicę znaków"
"&Autodetectar tabla de caracteres"
"&Автовизначення кодової сторінки"

MEditShareWrite
"Разрешить редактирование открытых для записи &файлов"
"Allow editing files ope&ned for writing"
upd:"Allow editing files opened for &writing"
upd:"Allow editing files opened for &writing"
"Írásra m&egnyitott fájlok szerkeszthetők"
upd:"Allow editing files opened for &writing"
"Permitir escritura de archivos abiertos para edición"
"Дозволити редагування відкритих для запису &файлів"

MEditLockROFileModification
"Блокировать р&едактирование файлов с атрибутом R/O"
"Lock editing of read-only &files"
"&Zamknout editaci souborů určených jen pro čtení"
"Bearbeiten von &Dateien mit Schreibschutz verhindern"
"Csak olvasható fájlok s&zerkesztése tiltva"
"Nie edytuj plików tylko do odczytu"
"Bloquear edición de &archivos de sólo lectura"
"Блокувати р&едагування файлів з атрибутом R/O"

MEditWarningBeforeOpenROFile
"Пре&дупреждать при открытии файла с атрибутом R/O"
"&Warn when opening read-only files"
"&Varovat při otevření souborů určených jen pro čtení"
"Beim Öffnen von Dateien mit Schreibschutz &warnen"
"Figyelmeztet &csak olvasható fájl megnyitásakor"
"&Ostrzeż przed otwieraniem plików tylko do odczytu"
"Advertencia al abrir archivos de sólo lectura"
"Зап&обігати відкриванню файлу з атрибутом R/O"

MEditConfigTabSize
"Раз&мер табуляции"
"Tab si&ze"
"Velikost &Tabulátoru"
"Ta&bulatorgröße"
"Tab&ulátor mérete"
"Rozmiar ta&bulatora"
"Tamaño de tabulación"
"Роз&мір табуляції"

MEditConfigScrollbar
"Показывать &полосу прокрутки"
"Show scro&llbar"
"Zobr&azovat posuvník"
"Scro&llbalken anzeigen"
"&Gördítősáv mutatva"
"Pokaż %pasek przewijania"
"Mostrar barra de desp&lazamiento"
"Показувати &смугу прокручування"

MEditShowWhiteSpace
"Пробельные символы"
"Show white space"
upd:"Show white space"
upd:"Show white space"
upd:"Show white space"
upd:"Show white space"
"Mostrar espacios en blanco"
"Пробільні символи"

MEditShowKeyBar
"Показывать &линейку клавиш"
"Show &key bar"
"Zobrazovat &zkratkové klávesy"
"Tast&enleiste anzeigen"
"&Funkcióbillentyűk sora mutatva"
"Wyświetl pasek &klawiszy"
"Mostrar barra de &funciones"
"Показувати &лінійку клавіш"

MEditShowTitleBar
"Показывать &заголовок"
"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
upd:"S&how title bar"
"Показувати &заголовок"

MEditConfigPickUpWord
"Cлово под к&урсором"
"Pick &up the word"
upd:"Pick &up the word"
upd:"Pick &up the word"
upd:"Pick &up the word"
upd:"Pick &up the word"
"Pick &up the word"
"Слово під к&урсором"

MEditConfigDefaultCodePage
"Выберите &кодовую страницу по умолчанию:"
"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
upd:"Choose default code pa&ge:"
"Виберіть &кодову сторінку за промовчанням:"

MNotifConfigTitle
l:
"Уведомления"
"Notifications"
"Notifications"
"Notifications"
"Notifications"
"Notifications"
"Notifications"
"Повідомлення"

MNotifConfigOnFileOperation
"Уведомлять о завершении &файловой операции"
"Notify on &file operation completion"
upd:"Notify on &file operation completion"
upd:"Notify on &file operation completion"
upd:"Notify on &file operation completion"
upd:"Notify on &file operation completion"
upd:"Notify on &file operation completion"
"Повідомляти про завершення &файлової операції"

MNotifConfigOnConsole
"Уведомлять о завершении &консольной команды"
"Notify on &console command completion"
upd:"Notify on &console command completion"
upd:"Notify on &console command completion"
upd:"Notify on &console command completion"
upd:"Notify on &console command completion"
upd:"Notify on &console command completion"
"Повідомляти про завершення &консольної команди"

MNotifConfigOnlyIfBackground
"Уведомлять только когда в &фоне"
"Notify only if in &background"
upd:"Notify only if in &background"
upd:"Notify only if in &background"
upd:"Notify only if in &background"
upd:"Notify only if in &background"
upd:"Notify only if in &background"
"Повідомляти лише коли у &фоні"

MConsoleCommandComplete
"Консольная команда выполнена"
"Console command complete"
upd:"Console command complete"
upd:"Console command complete"
upd:"Console command complete"
upd:"Console command complete"
upd:"Console command complete"
"Консольна команда виконана"

MConsoleCommandFailed
"Консольная команда завершена с ошибкой"
"Console command failed"
upd:"Console command failed"
upd:"Console command failed"
upd:"Console command failed"
upd:"Console command failed"
upd:"Console command failed"
"Консольна команда завершена з помилкою"

MFileOperationComplete
"Файловая операция выполнена"
"File operation complete"
upd:"File operation complete"
upd:"File operation complete"
upd:"File operation complete"
upd:"File operation complete"
upd:"File operation complete"
"Файлова операція виконана"

MSaveSetupTitle
l:
"Конфигурация"
"Save setup"
"Uložit nastavení"
"Einstellungen speichern"
"Beállítások mentése"
"Zapisz ustawienia"
"Guardar configuración"
"Конфігурація"

MSaveSetupAsk1
"Вы хотите сохранить"
"Do you wish to save"
"Přejete si uložit"
"Wollen Sie die aktuellen Einstellungen"
"Elmenti a jelenlegi"
"Czy chcesz zapisać"
"Desea guardar la configuración"
"Ви хочете зберегти"

MSaveSetupAsk2
"текущую конфигурацию?"
"current setup?"
"aktuální nastavení?"
"speichern?"
"beállításokat?"
"bieżące ustawienia?"
"actual de FAR?"
"Поточну конфігурацію?"

MSaveSetup
"Сохранить"
"Save"
"Uložit"
"Speichern"
"Mentés"
"Zapisz"
"Guardar"
"Зберегти"

MCopyDlgTitle
l:
"Копирование"
"Copy"
"Kopírovat"
"Kopieren"
"Másolás"
"Kopiuj"
"Copiar"
"Копіювання"

MMoveDlgTitle
"Переименование/Перенос"
"Rename/Move"
"Přejmenovat/Přesunout"
"Verschieben/Umbenennen"
"Átnevezés-Mozgatás"
"Zmień nazwę/przenieś"
"Renombrar/Mover"
"Перейменування/Перенесення"

MLinkDlgTitle
"Ссылка"
"Link"
"Link"
"Link erstellen"
"Link létrehozása"
"Dowiąż"
"Enlace"
"Посилання"

MCopyAccessMode
"Копировать &режим доступа к файлам"
"Copy files &access mode"
upd:"Copy files &access mode"
upd:"Copy files &access mode"
upd:"Copy files &access mode"
upd:"Copy files &access mode"
upd:"Copy files &access mode"
"Копіювати &режим доступу до файлів"

MCopyIfFileExist
"Уже су&ществующие файлы:"
"Already e&xisting files:"
"Již e&xistující soubory:"
"&Dateien überschreiben:"
"Már &létező fájloknál:"
"Dla już &istniejących:"
"Archivos ya e&xistentes:"
"Вже іс&нуючі файли:"

MCopyAsk
"&Запрос действия"
"&Ask"
"Ptát s&e"
"Fr&agen"
"Kér&dez"
"&Zapytaj"
"Pregunt&ar"
"&Запит дії"

MCopyAskRO
"Запрос подтверждения для &R/O файлов"
"Also ask on &R/O files"
"Ptát se také na &R/O soubory"
"Bei Dateien mit Sch&reibschutz fragen"
"&Csak olvasható fájloknál is kérdez"
"&Pytaj także o pliki tylko do odczytu"
"Preguntar también en archivos de Sólo Lectu&ra"
"Запит підтвердження для &R/O файлів"

MCopyOnlyNewerFiles
"Только &новые/обновлённые файлы"
"Only ne&wer file(s)"
"Pouze &novější soubory"
"Nur &neuere Dateien"
"Cs&ak az újabb fájlokat"
"Tylko &nowsze pliki"
"Sólo archivo(s) más nuev&os"
"Тільки &нові/оновлені файли"

MLinkType
"&Тип ссылки:"
"Link t&ype:"
"&Typ linku:"
"Linkt&yp:"
"Link &típusa:"
"&Typ linku:"
"Tipo de &enlace"
"&Тип посилання:"

MLinkTypeJunction
"&связь каталогов"
"directory &junction"
"křížení a&dresářů"
"Ordner&knotenpunkt"
"Mappa &csomópont"
"directory &junction"
"unión de directorio"
"&зв'язок каталогів"

MLinkTypeHardlink
"&жёсткая ссылка"
"&hard link"
"&pevný link"
"&Hardlink"
"&Hardlink"
"link &trwały"
"enlace duro"
"&жорстке посилання"

MLinkTypeSymlink
"си&мволическая ссылка"
"&symbolic link"
"symbolický link"
"Symbolischer Link"
"Szimbolikus link"
"link symboliczny"
"enlace simbólico"
"си&мволічне посилання"

MLinkTypeSymlinkFile
"символическая ссылка (&файл)"
"symbolic link (&file)"
"symbolický link (&soubor)"
"Symbolischer Link (&Datei)"
"Szimbolikus link (&fájl)"
"link symboliczny (do &pliku)"
"enlace simbólico (&archivo)"
"символічне посилання (&файл)"

MLinkTypeSymlinkDirectory
"символическая ссылка (&папка)"
"symbolic link (fol&der)"
"symbolický link (&adresář)"
"Symbolischer Link (Or&dner)"
"Szimbolikus link (&mappa)"
"link symboliczny (do &folderu)"
"enlace simbólico (&directorios)"
"символічна посилання (&тека)"

MCopySymLinkText
"Символические сс&ылки:"
"With s&ymlinks:"
upd:"With s&ymlinks:"
upd:"With s&ymlinks:"
upd:"With s&ymlinks:"
upd:"With s&ymlinks:"
upd:"With s&ymlinks:"
"Символічні по&силання:"

MLinkCopyAsIs
"Всегда копировать &ссылку"
"Always copy &link"
upd:"Always copy &link"
upd:"Always copy &link"
upd:"Always copy &link"
upd:"Always copy &link"
upd:"Always copy &link"
"Завжди копіювати &посилання"

MLinkCopySmart
"&Умно копировать ссылку или файл"
"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
upd:"&Smartly copy link or target file"
"&Розумно копіювати посилання або файл"

MLinkCopyContent
"Копировать как &файл"
"Always copy target &file"
upd:"Always copy target &file"
upd:"Always copy target &file"
upd:"Always copy target &file"
upd:"Always copy target &file"
upd:"Always copy target &file"
"Копіювати як &файл"

MCopySparseFiles
"Создавать &разреженные файлы"
"Produce &sparse files"
upd:"Produce &sparse files"
upd:"Produce &sparse files"
upd:"Produce &sparse files"
upd:"Produce &sparse files"
upd:"Produce &sparse files"
"Створювати &розріджені файли"

MCopyUseCOW
"Использовать копирование-&при-записи если возможно"
"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
upd:"Use copy-o&n-write if possible"
"Використовувати копіювання-&та-записи якщо це можливо"

MCopyMultiActions
"Обр&абатывать несколько имён файлов"
"Process &multiple destinations"
"&Zpracovat více míst určení"
"&Mehrere Ziele verarbeiten"
"Tö&bbszörös cél létrehozása"
"Przetwarzaj &wszystkie cele"
"Procesar &múltiples destinos"
"Виб&рати кілька імен файлів"

MCopyDlgCopy
"&Копировать"
"&Copy"
"&Kopírovat"
"&Kopieren"
"&Másolás"
"&Kopiuj"
"&Copiar"
"&Копіювати"

MCopyDlgTree
"F10-&Дерево"
"F10-&Tree"
"F10-&Strom"
"F10-&Baum"
"F10-&Fa"
"F10-&Drzewo"
"F10-&Arbol"
"F10-&Дерево"

MCopyDlgCancel
"&Отменить"
"Ca&ncel"
"&Storno"
"Ab&bruch"
"Még&sem"
"&Anuluj"
"Ca&ncelar"
"&Скасувати"

MCopyDlgRename
"&Переименовать"
"&Rename"
"Přej&menovat"
"&Umbenennen"
"Át&nevez-Mozgat"
"&Zmień nazwę"
"&Renombrar"
"&Перейменувати"

MCopyDlgLink
"&Создать ссылку"
"Create &link"
upd:"Create &link"
upd:"Create &link"
upd:"Create &link"
upd:"Create &link"
upd:"Create &link"
"&Створити посилання"

MCopyDlgTotal
"Всего"
"Total"
"Celkem"
"Gesamt"
"Összesen"
"Razem"
"Total"
"Всього"

MCopyScanning
"Сканирование папок..."
"Scanning folders..."
"Načítání adresářů..."
"Scanne Ordner..."
"Mappák olvasása..."
"Przeszukuję katalogi..."
"Explorando directorios..."
"Сканування тек..."

MCopyPrepareSecury
"Применение прав доступа..."
"Applying access rights..."
"Nastavuji přístupová práva..."
"Anwenden der Zugriffsrechte..."
"Hozzáférési jogok alkalmazása..."
"Ustawianie praw dostępu..."
"Aplicando derechos de acceso..."
"Застосування прав доступу..."

MCopyUseFilter
"Исполь&зовать фильтр"
"&Use filter"
"P&oužít filtr"
"Ben&utze Filter"
"Szűrő&vel"
"&Użyj filtra"
"&Usar filtros"
"Викори&стовувати фільтр"

MCopySetFilter
"&Фильтр"
"Filt&er"
"Filt&r"
"Filt&er"
"S&zűrő"
"Filt&r"
"Fi&ltro"
"&Фільтр"

MCopyFile
l:
"Копировать"
"Copy"
"Kopírovat"
"Kopiere"
upd:"másolása"
"Skopiuj"
"Copiar"
"Копіювати"

MMoveFile
"Переименовать или перенести"
"Rename or move"
"Přejmenovat nebo přesunout"
"Verschiebe"
upd:"átnevezése-mozgatása"
"Zmień nazwę lub przenieś"
"Renombrar o mover"
"Перейменувати або перенести"

MLinkFile
"Создать ссылку на"
"Create link to"
upd:"Create link to"
upd:"Create link to"
upd:"Create link to"
upd:"Create link to"
upd:"Create link to"
"Створити посилання на"

MCopyFiles
"Копировать %d элемент%ls"
"Copy %d item%ls"
"Kopírovat %d polož%ls"
"Kopiere %d Objekt%ls"
" %d elem másolása"
"Skopiuj %d plików"
"Copiar %d ítem%ls"
"Копіювати %d елемент%ls"

MMoveFiles
"Переименовать или перенести %d элемент%ls"
"Rename or move %d item%ls"
"Přejmenovat nebo přesunout %d polož%ls"
"Verschiebe %d Objekt%ls"
" %d elem átnevezése-mozgatása"
"Zmień nazwę lub przenieś %d plików"
"Renombrar o mover %d ítem%ls"
"Перейменувати або перенести %d елемент %ls"

MLinkFiles
"Создать ссылки на %d элемент%ls"
"Create links to %d item%ls"
upd:"Create links to %d item%ls"
upd:"Create links to %d item%ls"
upd:"Create links to %d item%ls"
upd:"Create links to %d item%ls"
upd:"Create links to %d item%ls"
"Створити посилання на %d елемент %ls"

MCMLTargetTO
" &в:"
" t&o:"
" d&o:"
" na&ch:"
" ide:"
" d&o:"
" &hacia:"
" &в:"

MCMLTargetIN
" &в:"
" in:"
upd:" &in:"
upd:" &in:"
upd:" &in:"
upd:" &in:"
upd:" &in:"
" &в:"

MCMLItems0
""
""
"u"
""
""
""
""
""

MCMLItemsA
"а"
"s"
"ek"
"e"
""
"s"
"s"
"а"


MCMLItemsS
"ов"
"s"
"ky"
"e"
""
"s"
"s"
"ів"

MCopyIncorrectTargetList
l:
"Указан некорректный список целей"
"Incorrect target list"
"Nesprávný seznam cílů"
"Ungültige Liste von Zielen"
"Érvénytelen céllista"
"Błędna lista wynikowa"
"Lista destino incorrecta"
"Вказано некоректний список цілей"

MCopyCopyingTitle
l:
"Копирование"
"Copying"
"Kopíruji"
"Kopieren"
"Másolás"
"Kopiowanie"
"Copiando"
"Копіювання"

MCopyMovingTitle
"Перенос"
"Moving"
"Přesouvám"
"Verschieben"
"Mozgatás"
"Przenoszenie"
"Moviendo"
"Перенесення"

MCopyCannotFind
l:
"Файл не найден"
"Cannot find the file"
"Nelze nalézt soubor"
"Folgende Datei kann nicht gefunden werden:"
"A fájl nem található:"
"Nie mogę odnaleźć pliku"
"No se puede encontrar el archivo"
"Файл не знайдено"

MCannotCopyFolderToItself1
l:
"Нельзя копировать папку"
"Cannot copy the folder"
"Nelze kopírovat adresář"
"Folgender Ordner kann nicht kopiert werden:"
"A mappa:"
"Nie można skopiować katalogu"
"No se puede copiar el directorio"
"Не можна копіювати папку"

MCannotCopyFolderToItself2
"в саму себя"
"onto itself"
"sám na sebe"
"Ziel und Quelle identisch."
"nem másolható önmagába/önmagára"
"do niego samego"
"en sí mismo"
"у саму себе"

MCannotCopyToTwoDot
l:
"Нельзя копировать файл или папку"
"You may not copy files or folders"
"Nelze kopírovat soubory nebo adresáře"
"Kopieren von Dateien oder Ordnern ist maximal"
"Nem másolhatja a fájlt vagy mappát"
"Nie można skopiować plików"
"Usted no puede copiar archivos o directorios"
"Не можна копіювати файл або папку"

MCannotMoveToTwoDot
"Нельзя перемещать файл или папку"
"You may not move files or folders"
"Nelze přesunout soubory nebo adresáře"
"Verschieben von Dateien oder Ordnern ist maximal"
"Nem mozgathatja a fájlt vagy mappát"
"Nie można przenieść plików"
"Usted no puede mover archivos o directorios"
"Не можна переміщувати файл або папку"

MCannotCopyMoveToTwoDot
"выше корневого каталога"
"higher than the root folder"
"na vyšší úroveň než kořenový adresář"
"bis zum Wurzelverzeichnis möglich."
"a gyökér fölé"
"na poziom wyższy niż do korzenia"
"más alto que el directorio raíz"
"вище кореневого каталогу"

MCopyCannotCreateFolder
l:
"Ошибка создания папки"
"Cannot create the folder"
"Nelze vytvořit adresář"
"Folgender Ordner kann nicht erstellt werden:"
"A mappa nem hozható létre:"
"Nie udało się utworzyć katalogu"
"No se puede crear el directorio"
"Помилка створення папки"

MCopyCannotChangeFolderAttr
"Невозможно установить атрибуты для папки"
"Failed to set folder attributes"
"Nastavení atributů adresáře selhalo"
"Fehler beim Setzen der Ordnerattribute"
"A mappa attribútumok beállítása sikertelen"
"Nie udało się ustawić atrybutów folderu"
"Error al poner atributos en directorio"
"Неможливо встановити атрибути для папки"

MCopyCannotRenameFolder
"Невозможно переименовать папку"
"Cannot rename the folder"
"Nelze přejmenovat adresář"
"Folgender Ordner kann nicht umbenannt werden:"
"A mappa nem nevezhető át:"
"Nie udało się zmienić nazwy katalogu"
"No se puede renombrar el directorio"
"Неможливо перейменувати папку"

MCopyIgnore
"&Игнорировать"
"&Ignore"
"&Ignorovat"
"&Ignorieren"
"Mé&gis"
"&Ignoruj"
"&Ignorar"
"&Ігнорувати"

MCopyIgnoreAll
"Игнорировать &все"
"Ignore &All"
"Ignorovat &vše"
"&Alle ignorieren"
"Min&d"
"Ignoruj &wszystko"
"Ignorar &Todo"
"Ігнорувати &все"

MCopyRetry
"&Повторить"
"&Retry"
"&Opakovat"
"Wiede&rholen"
"Ú&jra"
"&Ponów"
"&Reiterar"
"&Повторити"

MCopySkip
"П&ропустить"
"&Skip"
"&Přeskočit"
"Ausla&ssen"
"&Kihagy"
"&Omiń"
"&Omitir"
"П&ропустити"

MCopySkipAll
"&Пропустить все"
"S&kip all"
"Př&eskočit vše"
"Alle aus&lassen"
"Mi&nd"
"Omiń w&szystkie"
"O&mitir todos"
"&Пропустити все"

MCopyCancel
"&Отменить"
"&Cancel"
"&Storno"
"Abb&rechen"
"Még&sem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

MCopyCannotCreateLink
l:
"Ошибка создания ссылки"
"Cannot create the link"
"Nelze vytvořit symbolický link"
"Folgender Link kann nicht erstellt werden:"
"A link nem hozható létre:"
"Nie udało się utworzyć linku"
"No se puede crear el enlace simbólico"
"Помилка створення посилання"

MCopyFolderNotEmpty
"Папка назначения должна быть пустой"
"Target folder must be empty"
"Cílový adresář musí být prázdný"
"Zielordner muss leer sein."
"A célmappának üresnek kell lennie"
"Folder wynikowy musi być pusty"
"Directorio destino debe estar vacío"
"Папка призначення має бути порожньою"

MCopyCannotCreateJunctionToFile
"Невозможно создать связь. Файл уже существует:"
"Cannot create junction. The file already exists:"
"Nelze vytvořit křížový odkaz. Soubor již existuje:"
"Knotenpunkt wurde nicht erstellt. Datei existiert bereits:"
"A csomópont nem hozható létre. A fájl már létezik:"
"Nie można utworzyć połączenia - plik już istnieje:"
"No se puede unir. El archivo ya existe:"
"Неможливо створити зв'язок. Файл уже існує:"

MCopyCannotCreateSymlinkAskCopyContents
"Невозможно создать связь. Копировать данные вместо связей?"
"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
upd:"Cannot create symlink. Copy contents instead?"
"Неможливо створити зв'язок. Копіювати дані замість зв'язків?"

MCopyCannotCreateVolMount
l:
"Ошибка монтирования диска"
"Volume mount points error"
"Chyba připojovacích svazků"
"Fehler im Mountpoint des Datenträgers"
"Kötet mountpont hiba"
"Błąd montowania napędu"
"Error en puntos de montaje de volumen"
"Помилка монтування диска"

MCopyMountVolFailed
"Ошибка при монтировании диска '%ls'"
"Attempt to volume mount '%ls'"
"Pokus o připojení svazku '%ls'"
"Versuch Datenträger '%ls' zu aktivieren"
""%ls" kötet mountolása"
"Nie udało się zamontować woluminu '%ls'"
"Intento de montaje de volumen '%ls'"
"Помилка при монтуванні диска '%ls'"

MCopyMountVolFailed2
"на '%ls'"
"at '%ls' failed"
"na '%ls' selhal"
"fehlgeschlagen bei '%ls'"
"nem sikerült: "%ls""
"w '%ls' nie udało się"
"a '%ls' ha fallado"
"на '%ls'"

MCopyMountName
"disk_%c"
"Disk_%c"
"Disk_%c"
"Disk_%c"
"Disk_%c"
"Disk_%c"
"Disco_%c"
"disk_%c"

MCannotCopyFileToItself1
l:
"Нельзя копировать файл"
"Cannot copy the file"
"Nelze kopírovat soubor"
"Folgende Datei kann nicht kopiert werden:"
"A fájl"
"Nie można skopiować pliku"
"Imposible copiar el archivo"
"Не можна копіювати файл"

MCannotCopyFileToItself2
"в самого себя"
"onto itself"
"sám na sebe"
"Ziel und Quelle identisch."
"nem másolható önmagára"
"do niego samego"
"en sí mismo"
"у самого себе"

MCopyStream1
l:
"Исходный файл содержит более одного потока данных,"
"The source file contains more than one data stream."
"Zdrojový soubor obsahuje více než jeden datový proud."
"Die Quelldatei enthält mehr als einen Datenstream"
"A forrásfájl több stream-et tartalmaz,"
"Plik źródłowy zawiera więcej niż jeden strumień danych."
"El archivo origen contiene más de un flujo de datos."
"Вихідний файл містить більше одного потоку даних,"

MCopyStream2
"но вы не используете системную функцию копирования."
"but since you do not use a system copy routine."
"protože nepoužíváte systémovou kopírovací rutinu."
"aber Sie verwenden derzeit nicht die systemeigene Kopierroutine."
"de nem a rendszer másolórutinját használja."
"ale ze względu na rezygnację z systemowej procedury kopiowania."
"pero desde que usted no usa la rutina de copia del sistema."
"але ви не використовуєте системну функцію копіювання."

MCopyStream3
"но том назначения не поддерживает этой возможности."
"but the destination volume does not support this feature."
"protože cílový svazek nepodporuje tuto vlastnost."
"aber der Zieldatenträger unterstützt diese Fähigkeit nicht."
"de a célkötet nem támogatja ezt a lehetőséget."
"ale napęd docelowy nie obsługuje tej funkcji."
"pero el volumen de destino no soporta esta opción."
"але тому призначення не підтримує цієї можливості."

MCopyStream4
"Часть сведений не будет сохранена."
"Some data will not be preserved as a result."
"To bude mít za následek, že některá data nebudou uchována."
"Ein Teil der Daten bleiben daher nicht erhalten."
"Az adatok egy része el fog veszni."
"Nie wszystkie dane zostaną zachowane."
"Algunos datos no serán preservados como un resultado."
"Частина відомостей не буде збережена."

MCopyDirectoryOrFile
l:
"Подразумевается имя папки или файла?"
"Does it specify a folder name or file name?"
upd:"Does it specify a folder name or file name?"
upd:"Does it specify a folder name or file name?"
upd:"Does it specify a folder name or file name?"
upd:"Does it specify a folder name or file name?"
"Si especifica nombre de carpeta o nombre de archivo?"
"Має на увазі ім'я теки або файлу?"

MCopyDirectoryOrFileDirectory
"Папка"
"Folder"
upd:"Folder"
upd:"Folder"
upd:"Folder"
upd:"Folder"
"Carpeta"
"Тека"

MCopyDirectoryOrFileFile
"Файл"
"File"
upd:"File"
upd:"File"
upd:"File"
upd:"File"
"Archivo"
"Файл"

MCopyFileExist
l:
"Файл уже существует"
"File already exists"
"Soubor již existuje"
"Datei existiert bereits"
"A fájl már létezik:"
"Plik już istnieje"
"El archivo ya existe"
"Файл вже існує"

MCopySource
"&Новый"
"&New"
"&Nový"
"&Neue Datei"
"Ú&j verzió:"
"&Nowy"
"Nuevo"
"&Новий"

MCopyDest
"Су&ществующий"
"E&xisting"
"E&xistující"
"Be&stehende Datei"
"Létező &verzió:"
"&Istniejący"
"Existente"
"Іс&нуючий"

MCopyOverwrite
"В&место"
"&Overwrite"
"&Přepsat"
"Über&schr."
"&Felülír"
"N&adpisz"
"&Sobrescribir"
"З&амість"

MCopySkipOvr
"&Пропустить"
"&Skip"
"Přes&kočit"
"Über&spr."
"&Kihagy"
"&Omiń"
"&Omitir"
"&Пропустити"

MCopyAppend
"&Дописать"
"A&ppend"
"Př&ipojit"
"&Anhängen"
"Hoz&záfűz"
"&Dołącz"
"A&gregar"
"&Дописати"

MCopyResume
"Возоб&новить"
"&Resume"
"Po&kračovat"
"&Weiter"
"Fol&ytat"
"Ponó&w"
"&Resumir"
"Від&новити"

MCopyRename
"&Имя"
"R&ename"
upd:"R&ename"
upd:"R&ename"
"Á&tnevez"
upd:"R&ename"
"Renombrar"
"&Ім'я"

MCopyCancelOvr
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"&Відмінити"

MCopyRememberChoice
"&Запомнить выбор"
"&Remember choice"
"Zapama&tovat volbu"
"Auswahl me&rken"
"Mind&ent a kiválasztott módon"
"&Zapamiętaj ustawienia"
"&Recordar elección"
"&Запам'ятати вибір"

MCopyRenameTitle
"Переименование"
"Rename"
upd:"Rename"
upd:"Rename"
"Átnevezés"
upd:"Rename"
"Renombrar"
"Перейменування"

MCopyRenameText
"&Новое имя:"
"&New name:"
upd:"&New name:"
upd:"&New name:"
"Ú&j név:"
upd:"&New name:"
"&Nuevo nombre:"
"&Нове ім'я:"

MCopyFileRO
l:
"Файл имеет атрибут \"Только для чтения\""
"The file is read only"
"Soubor je určen pouze pro čtení"
"Folgende Datei ist schreibgeschützt:"
"A fájl csak olvasható:"
"Ten plik jest tylko-do-odczytu"
"El archivo es de sólo lectura"
"Файл має атрибут \"Тільки для читання\""

MCopyAskDelete
"Вы хотите удалить его?"
"Do you wish to delete it?"
"Opravdu si ho přejete smazat?"
"Wollen Sie sie dennoch löschen?"
"Biztosan törölni akarja?"
"Czy chcesz go usunąć?"
"Desea borrarlo igual?"
"Ви хочете видалити його?"

MCopyDeleteRO
"&Удалить"
"&Delete"
"S&mazat"
"&Löschen"
"&Törli"
"&Usuń"
"&Borrar"
"&Вилучити"

MCopyDeleteAllRO
"&Все"
"&All"
"&Vše"
"&Alle Löschen"
"Min&det"
"&Wszystkie"
"&Todos"
"&Усе"

MCopySkipRO
"&Пропустить"
"&Skip"
"Přes&kočit"
"Über&springen"
"&Kihagyja"
"&Omiń"
"&Omitir"
"&Пропустити"

MCopySkipAllRO
"П&ропустить все"
"S&kip all"
"Př&eskočit vše"
"A&lle überspringen"
"Mind&et"
"O&miń wszystkie"
"O&mitir todos"
"П&ропустити все"

MCopyCancelRO
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

MCannotCopy
l:
"Ошибка копирования"
"Cannot copy"
"Nelze kopírovat"
"Konnte nicht kopieren"
"Nem másolható"
"Nie mogę skopiować"
"No se puede copiar %ls"
"Помилка копіювання"

MCannotMove
"Ошибка переноса"
"Cannot move"
"Nelze přesunout"
"Konnte nicht verschieben"
"Nem mozgatható"
"Nie mogę przenieść"
"No se puede mover %ls"
"Помилка перенесення"

MCannotLink
"Ошибка создания ссылки"
"Cannot link"
"Nelze linkovat"
"Konnte nicht verlinken"
"Nem linkelhető"
"Nie mogę dowiązać"
"No se puede enlazar %ls"
"Помилка створення посилання"

MCannotCopyTo
"в"
"to"
"do"
"nach"
"ide:"
"do"
"hacia %ls"
"в"

MCopyEncryptWarn1
"Файл"
"The file"
"Soubor"
"Die Datei"
"A fájl"
"Plik"
"El archivo"
"Файл"

MCopyEncryptWarn2
"нельзя скопировать или переместить, не потеряв его шифрование."
"cannot be copied or moved without losing its encryption."
"nemůže být zkopírován nebo přesunut bez ztráty jeho šifrování."
"kann nicht bewegt werden ohne ihre Verschlüsselung zu verlieren."
"csak titkosítása elvesztésével másolható vagy mozgatható."
"nie może zostać skopiowany/przeniesiony bez utraty szyfrowania"
"no puede copiarse o moverse sin perder el cifrado."
"Не можна скопіювати або перемістити, не втративши його шифрування."

MCopyEncryptWarn3
"Можно пропустить эту ошибку или отменить операцию."
"You can choose to ignore this error and continue, or cancel."
"Můžete tuto chybu ignorovat a pokračovat, nebo operaci ukončit."
"Sie können dies ignorieren und fortfahren oder abbrechen."
"Ennek ellenére folytathatja vagy felfüggesztheti."
"Możesz zignorować błąd i kontynuować lub anulować operację."
"Usted puede ignorar este error y continuar, o cancelar."
"Можна пропустити цю помилку або скасувати операцію."

MCopyReadError
l:
"Ошибка чтения данных из"
"Cannot read data from"
"Nelze číst data z"
"Kann Daten nicht lesen von"
"Nem olvasható adat innen:"
"Nie mogę odczytać danych z"
"No se puede leer datos desde"
"Помилка читання даних з"

MCopyWriteError
"Ошибка записи данных в"
"Cannot write data to"
"Nelze zapsat data do"
"Dann Daten nicht schreiben in"
"Nem írható adat ide:"
"Nie mogę zapisać danych do"
"No se puede escribir datos hacia"
"Помилка запису даних"

MCopyProcessed
l:
"Обработано файлов: %d"
"Files processed: %d"
"Zpracováno souborů: %d"
"Dateien verarbeitet: %d"
" %d fájl kész"
"Przetworzonych plików: %d"
"Archivos procesados: %d"
"Оброблено файли: %d"

MCopyProcessedTotal
"Обработано файлов: %d из %d"
"Files processed: %d of %d"
"Zpracováno souborů: %d z %d"
"Dateien verarbeitet: %d von %d"
" %d fájl kész %d fájlból"
"Przetworzonych plików: %d z %d"
"Archivos procesados: %d de %d"
"Оброблено файли: %d з %d"

MCopyMoving
"Перенос файла"
"Moving the file"
"Přesunuji soubor"
"Verschiebe die Datei"
"Fájl mozgatása"
"Przenoszę plik"
"Moviendo el archivo"
"Перенесення файлу"

MCopyCopying
"Копирование файла"
"Copying the file"
"Kopíruji soubor"
"Kopiere die Datei"
"Fájl másolása"
"Kopiuję plik"
"Copiando el archivo"
"Копіювання файлу"

MCopyTo
"в"
"to"
"do"
"nach"
"ide:"
"do"
"Hacia"
"в"

MCopyErrorDiskFull
l:
"Диск заполнен. Вставьте следующий"
"Disk full. Insert next"
"Disk je plný. Vložte dalšíí"
"Datenträger voll. Bitte nächsten einlegen"
"A lemez megtelt, kérem a következőt"
"Dysk pełny. Włóż następny"
"Disco lleno. Inserte el próximo"
"Диск заповнений. Вставте наступний"

MDeleteTitle
l:
"Удаление"
"Delete"
"Smazat"
"Löschen"
"Törlés"
"Usuń"
"Borrar"
"Видалення"

MAskDeleteFolder
"Вы хотите удалить папку"
"Do you wish to delete the folder"
"Přejete si smazat adresář"
"Wollen Sie den Ordner löschen"
"Törölni akarja a mappát?"
"Czy chcesz wymazać katalog"
"Desea borrar el directorio"
"Ви хочете видалити теку"

MAskDeleteFile
"Вы хотите удалить файл"
"Do you wish to delete the file"
"Přejete si smazat soubor"
"Wollen Sie die Datei löschen"
"Törölni akarja a fájlt?"
"Czy chcesz usunąć plik"
"Desea borrar el archivo"
"Ви хочете видалити файл"

MAskDelete
"Вы хотите удалить"
"Do you wish to delete"
"Přejete si smazat"
"Wollen Sie folgendes Objekt löschen"
"Törölni akar"
"Czy chcesz usunąć"
"Desea borrar"
"Ви хочете видалити"

MAskDeleteRecycleFolder
"Вы хотите переместить в Корзину папку"
"Do you wish to move to the Recycle Bin the folder"
"Přejete si přesunout do Koše adresář"
"Wollen Sie den Ordner in den Papierkorb verschieben"
"A Lomtárba akarja dobni a mappát?"
"Czy chcesz przenieść katalog do Kosza"
"Desea mover hacia la papelera de reciclaje el directorio"
"Ви хочете перемістити в Кошик теку"

MAskDeleteRecycleFile
"Вы хотите переместить в Корзину файл"
"Do you wish to move to the Recycle Bin the file"
"Přejete si přesunout do Koše soubor"
"Wollen Sie die Datei in den Papierkorb verschieben"
"A Lomtárba akarja dobni a fájlt?"
"Czy chcesz przenieść plik do Kosza"
"Desea mover hacia la papelera de reciclaje el archivo"
"Ви хочете перемістити в Кошик файл"

MAskDeleteRecycle
"Вы хотите переместить в Корзину"
"Do you wish to move to the Recycle Bin"
"Přejete si přesunout do Koše"
"Wollen Sie das Objekt in den Papierkorb verschieben"
"A Lomtárba akar dobni"
"Czy chcesz przenieść do Kosza"
"Desea mover hacia la papelera de reciclaje"
"Ви хочете перемістити в Кошик"

MDeleteWipeTitle
"Уничтожение"
"Wipe"
"Vymazat"
"Sicheres Löschen"
"Kisöprés"
"Wymaż"
"Limpiar"
"Знищення"

MAskWipeFolder
"Вы хотите уничтожить папку"
"Do you wish to wipe the folder"
"Přejete si vymazat adresář"
"Wollen Sie den Ordner sicher löschen"
"Ki akarja söpörni a mappát?"
"Czy chcesz wymazać katalog"
"Desea limpiar el directorio"
"Ви хочете знищити теку"

MAskWipeFile
"Вы хотите уничтожить файл"
"Do you wish to wipe the file"
"Přejete si vymazat soubor"
"Wollen Sie die Datei sicher löschen"
"Ki akarja söpörni a fájlt?"
"Czy chcesz wymazać plik"
"Desea limpiar el archivo"
"Ви хочете знищити файл"

MAskWipe
"Вы хотите уничтожить"
"Do you wish to wipe"
"Přejete si vymazat"
"Wollen Sie das Objekt sicher löschen"
"Ki akar söpörni"
"Czy chcesz wymazać"
"Desea limpiar"
"Ви хочете знищити"

MDeleteLinkTitle
"Удаление ссылки"
"Delete link"
"Smazat link"
"Link löschen"
"Link törlése"
"Usuń link"
"Borrar enlace"
"Видалення посилання"

MAskDeleteLink
"является ссылкой на"
"is a symbolic link to"
"je symbolicky link na"
"ist ein symbolischer Link auf"
"szimlinkelve ide:"
"jest linkiem symbolicznym do"
"es un enlace simbólico al"
"є посиланням на"

MAskDeleteLinkFolder
"папку"
"folder"
"adresář"
"Ordner"
"mappa"
"folder"
"directorio"
"теку"

MAskDeleteLinkFile
"файл"
"file"
"soubor"
"Date"
"fájl"
"plik"
"archivo"
"файл"

MAskDeleteItems
"%d элемент%ls"
"%d item%ls"
"%d polož%ls"
"%d Objekt%ls"
"%d elemet%ls"
"%d plik%ls"
"%d ítem%ls"
"%d елемент%ls"

MAskDeleteItems0
""
""
"ku"
""
""
""
""
""

MAskDeleteItemsA
"а"
"s"
"ky"
"e"
""
"i"
"s"
"а"

MAskDeleteItemsS
"ов"
"s"
"ek"
"e"
""
"ów"
"s"
"ів"

MDeleteFolderTitle
l:
"Удаление папки "
"Delete folder"
"Smazat adresář"
"Ordner löschen"
"Mappa törlése"
"Usuń folder"
"Borrar directorio"
"Видалення теки "

MWipeFolderTitle
"Уничтожение папки "
"Wipe folder"
"Vymazat adresář"
"Ordner sicher löschen"
"Mappa kisöprése"
"Wymaż folder"
"Limpiar directorio"
"Знищення теки "

MDeleteFilesTitle
"Удаление файлов"
"Delete files"
"Smazat soubory"
"Dateien löschen"
"Fájlok törlése"
"Usuń pliki"
"Borrar archivos"
"Видалення файлів"

MWipeFilesTitle
"Уничтожение файлов"
"Wipe files"
"Vymazat soubory"
"Dateien sicher löschen"
"Fájlok kisöprése"
"Wymaż pliki"
"Limpiar archivos"
"Знищення файлів"

MDeleteFolderConfirm
"Данная папка будет удалена:"
"The following folder will be deleted:"
"Následující adresář bude smazán:"
"Folgender Ordner wird gelöscht:"
"A mappa törlődik:"
"Następujący folder zostanie usunięty:"
"El siguiente directorio será borrado:"
"Ця тека буде видалена:"

MWipeFolderConfirm
"Данная папка будет уничтожена:"
"The following folder will be wiped:"
"Následující adresář bude vymazán:"
"Folgender Ordner wird sicher gelöscht:"
"A mappa kisöprődik:"
"Następujący folder zostanie wymazany:"
"El siguiente directorio será limpiado:"
"Ця тека буде знищена:"

MDeleteWipe
"Уничтожить"
"Wipe"
"Vymazat"
"Sicheres Löschen"
"Kisöpör"
"Wymaż"
"Limpiar"
"Знищити"

MDeleteRecycle
"Переместить"
"Move"
upd:"Move"
upd:"Move"
upd:"Move"
upd:"Move"
upd:"Move"
"Перемістити"

MDeleteFileDelete
"&Удалить"
"&Delete"
"S&mazat"
"&Löschen"
"&Töröl"
"&Usuń"
"&Borrar"
"&Вилучити"

MDeleteFileWipe
"&Уничтожить"
"&Wipe"
"V&ymazat"
"&Sicher löschen"
"Kisö&pör"
"&Wymaż"
"&Limpiar"
"&Знищити"

MDeleteFileAll
"&Все"
"&All"
"&Vše"
"&Alle"
"Min&det"
"&wszystkie"
"&Todos"
"&Усе"

MDeleteFileSkip
"&Пропустить"
"&Skip"
"Přes&kočit"
"Über&springen"
"&Kihagy"
"&Omiń"
"&Omitir"
"&Пропустити"

MDeleteFileSkipAll
"П&ропустить все"
"S&kip all"
"Př&eskočit vše"
"A&lle überspr."
"Mind&et"
"O&miń wszystkie"
"O&mitir todos"
"П&ропустити все"

MDeleteFileCancel
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

MDeleteLinkDelete
l:
"Удалить ссылку"
"Delete link"
"Smazat link"
"Link löschen"
"Link törlése"
"Usuń link"
"Borrar enlace"
"Видалити посилання"

MDeleteLinkUnlink
"Разорвать ссылку"
"Break link"
"Poškozený link"
"Link auflösen"
"Link megszakítása"
"Przerwij link"
"Romper enlace"
"Розірвати посилання"

MDeletingTitle
l:
"Удаление"
"Deleting"
"Mazání"
"Lösche"
"Törlés"
"Usuwam"
"Borrando"
"Видалення"

MDeleting
l:
"Удаление файла или папки"
"Deleting the file or folder"
"Mazání souboru nebo adresáře"
"Löschen von Datei oder Ordner"
"Fájl vagy mappa törlése"
"Usuwam plik/katalog"
"Borrando el archivo o directorio"
"Видалення файлу або теки"

MDeletingWiping
"Уничтожение файла или папки"
"Wiping the file or folder"
"Vymazávání souboru nebo adresáře"
"Sicheres löschen von Datei oder Ordner"
"Fájl vagy mappa kisöprése"
"Wymazuję plik/katalog"
"Limpiando el archivo o directorio"
"Знищення файлу або теки"

MDeleteRO
l:
"Файл имеет атрибут \"Только для чтения\""
"The file is read only"
"Soubor je určen pouze pro čtení"
"Folgende Datei ist schreibgeschützt:"
"A fájl csak olvasható:"
"Ten plik jest tylko do odczytu"
"El archivo es de sólo lectura"
"Файл має атрибут \"Тільки для читання\""

MAskDeleteRO
"Вы хотите удалить его?"
"Do you wish to delete it?"
"Opravdu si ho přejete smazat?"
"Wollen Sie sie dennoch löschen?"
"Mégis törölni akarja?"
"Czy chcesz go usunąć?"
"Desea borrarlo?"
"Ви хочете видалити його?"

MAskWipeRO
"Вы хотите уничтожить его?"
"Do you wish to wipe it?"
"Opravdu si ho přejete vymazat?"
"Wollen Sie sie dennoch sicher löschen?"
"Mégis ki akarja söpörni?"
"Czy chcesz go wymazać?"
"Desea limpiarlo?"
"Ви хочете знищити його?"

MDeleteHardLink1
l:
"Файл имеет несколько жёстких ссылок"
"Several hard links link to this file."
"Více pevných linků ukazuje na tento soubor."
"Mehrere Hardlinks zeigen auf diese Datei."
"Több hardlink kapcsolódik a fájlhoz, a fájl"
"Do tego pliku prowadzi wiele linków trwałych."
"Demasiados enlaces rígidos a este archivo."
"Файл має кілька жорстких посилань"

MDeleteHardLink2
"Уничтожение файла приведёт к обнулению всех ссылающихся на него файлов."
"Wiping this file will void all files linking to it."
"Vymazání tohoto souboru zneplatní všechny soubory, které na něj linkují."
"Sicheres Löschen dieser Datei entfernt ebenfalls alle Links."
"kisöprése a linkelt fájlokat is megsemmisíti."
"Wymazanie tego pliku wymaże wszystkie pliki dolinkowane."
"Limpiando este archivo invalidará todos los archivos enlazados."
"Знищення файлу призведе до обнулення всіх файлів, що посилаються."

MDeleteHardLink3
"Уничтожать файл?"
"Do you wish to wipe this file?"
"Opravdu chcete vymazat tento soubor?"
"Wollen Sie diese Datei sicher löschen?"
"Biztosan kisöpri a fájlt?"
"Czy wymazać plik?"
"Desea limpiar este archivo"
"Знищувати файл?"

MCannotDeleteFile
l:
"Ошибка удаления файла"
"Cannot delete the file"
"Nelze smazat soubor"
"Datei konnte nicht gelöscht werden"
"A fájl nem törölhető"
"Nie mogę usunąć pliku"
"No se puede borrar el archivo"
"Помилка видалення файлу"

MCannotDeleteFolder
"Ошибка удаления папки"
"Cannot delete the folder"
"Nelze smazat adresář"
"Ordner konnte nicht gelöscht werden"
"A mappa nem törölhető"
"Nie mogę usunąć katalogu"
"No se puede borrar el directorio"
"Помилка видалення теки"

MDeleteRetry
"&Повторить"
"&Retry"
"&Znovu"
"Wiede&rholen"
"Ú&jra"
"&Ponów"
"&Reiterar"
"&Повторити"

MDeleteSkip
"П&ропустить"
"&Skip"
"Přes&kočit"
"Über&springen"
"&Kihagy"
"Po&miń"
"&Omitir"
"П&ропустити"

MDeleteSkipAll
"Пропустить &все"
"S&kip all"
"Přeskočit &vše"
"A&lle überspr."
"Min&d"
"Pomiń &wszystkie"
"Omitir &Todo"
"Пропустити &все"

MDeleteCancel
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

MCannotGetSecurity
l:
"Ошибка получения прав доступа к файлу"
"Cannot get file access rights for"
"Nemohu získat přístupová práva pro"
"Kann Zugriffsrechte nicht lesen für"
"A fájlhoz nincs hozzáférési joga:"
"Nie mogę pobrać praw dostępu dla"
"No se puede tener permisos de acceso a archivo"
"Помилка отримання доступу до файлу"

MCannotSetSecurity
"Ошибка установки прав доступа к файлу"
"Cannot set file access rights for"
"Nemohu nastavit přístupová práva pro"
"Kann Zugriffsrechte nicht setzen für"
"A fájl hozzáférési jogát nem állíthatja:"
"Nie mogę ustawić praw dostępu dla"
"No se puede poner permisos de acceso a archivo"
"Помилка встановлення прав доступу до файлу"

MEditTitle
l:
"Редактор"
"Editor"
"Editor"
"Editor"
"Szerkesztő"
"Edytor"
"Editor"
"Редактор"

MAskReload
"уже загружен. Как открыть этот файл?"
"already loaded. How to open this file?"
"již otevřen. Jak otevřít tento soubor?"
"bereits geladen. Wie wollen Sie die Datei öffnen?"
"fájl már be van töltve. Hogyan szerkeszti?"
"został już załadowany. Załadować ponownie?"
"ya está cargado. Como abrir este archivo?"
"вже завантажено. Як відкрити цей файл?"

MCurrent
"&Текущий"
"&Current"
"&Stávající"
"A&ktuell"
"A mostanit &folytatja"
"&Bieżący"
"A&ctual"
"&Поточний"

MReload
"Пере&грузить"
"R&eload"
"&Znovu načíst"
"Aktualisie&ren"
"Újra&tölti"
"&Załaduj"
"R&ecargar"
"Пере&вантажити"

MNewOpen
"&Новая копия"
"&New instance"
"&Nová instance"
"&Neue Instanz"
"Ú&j példányban"
"&Nowa instancja"
"&Nueva instancia"
"&Нова копія"

MEditCannotOpen
"Ошибка открытия файла"
"Cannot open the file"
"Nelze otevřít soubor"
"Kann Datei nicht öffnen"
"A fájl nem nyitható meg"
"Nie mogę otworzyć pliku"
"No se puede abrir el archivo"
"Помилка відкриття файлу"

MEditReading
"Чтение файла"
"Reading the file"
"Načítám soubor"
"Lesen der Datei"
"Fájl olvasása"
"Czytam plik"
"Leyendo el archivo"
"Читання файлу"

MEditAskSave
"Файл был изменён. Сохранить?"
"File has been modified. Save?"
upd:"Soubor byl modifikován. Save?"
upd:"Datei wurde verändert. Save?"
upd:"A fájl megváltozott. Save?"
upd:"Plik został zmodyfikowany. Save?"
"El archivo ha sido modificado. Desea guardarlo?"
"Файл було змінено. Зберегти?"

MEditAskSaveExt
"Файл был изменён внешней программой. Сохранить?"
"The file was changed by an external program. Save?"
upd:"Soubor byl změněný externím programem. Save?"
upd:"Die Datei wurde durch ein externes Programm verändert. Save?"
upd:"A fájlt egy külső program megváltoztatta. Save?"
upd:"Plik został zmieniony przez inny program. Save?"
"El archivo ha sido cambiado por un programa externo. Desea guardarlo?"
"Файл було змінено зовнішньою програмою. Зберегти?"

MEditBtnSaveAs
"Сохр&анить как..."
"Save &as..."
"Ulož&it jako..."
"Speichern &als..."
"Mentés más&ként..."
"Zapisz &jako..."
"Guardar como..."
"Збе&регти як..."

MEditRO
l:
"имеет атрибут \"Только для чтения\""
"is a read-only file"
"je určen pouze pro čtení"
"ist eine schreibgeschützte Datei"
"csak olvasható fájl"
"jest plikiem tylko do odczytu"
"es un archivo de sólo lectura"
"має атрибут \"Тільки для читання\""

MEditExists
"уже существует"
"already exists"
"již existuje"
"ist bereits vorhanden"
"már létezik"
"już istnieje"
"ya existe"
"вже існує"

MEditOvr
"Вы хотите перезаписать его?"
"Do you wish to overwrite it?"
"Přejete si ho přepsat?"
"Wollen Sie die Datei überschreiben?"
"Felül akarja írni?"
"Czy chcesz go nadpisać?"
"Desea sobrescribirlo?"
"Ви хочете перезаписати його?"

MEditSaving
"Сохранение файла"
"Saving the file"
"Ukládám soubor"
"Speichere die Datei"
"Fájl mentése"
"Zapisuję plik"
"Guardando el archivo"
"Збереження файлу"

MEditStatusLine
"Строка"
"Line"
"Řádek"
"Zeile"
"Sor"
"linia"
"Línea"
"Рядок"

MEditStatusCol
"Кол"
"Col"
"Sloupec"
"Spal"
"Oszlop"
"kolumna"
"Col"
"Кол"

MEditRSH
l:
"предназначен только для чтения"
"is a read-only file"
"je určen pouze pro čtení"
"ist eine schreibgeschützte Datei"
"csak olvasható fájl"
"jest plikiem tylko do odczytu"
"es un archivo de sólo lectura"
"призначений лише для читання"

MEditFileGetSizeError
"Не удалось определить размер."
"File size could not be determined."
upd:"File size could not be determined."
upd:"File size could not be determined."
"A fájlméret megállapíthatatlan."
upd:"File size could not be determined."
"Tamaño de archivo no puede ser determinado"
"Не вдалося визначити розмір."

MEditFileLong
"имеет размер %ls,"
"has the size of %ls,"
"má velikost %ls,"
"hat eine Größe von %ls,"
"mérete %ls,"
"ma wielkość %ls,"
"tiene el tamaño de %ls,"
"має розмір %ls,"

MEditFileLong2
"что превышает заданное ограничение в %ls."
"which exceeds the configured maximum size of %ls."
"která překračuje nastavenou maximální velikost %ls."
"die die konfiguierte Maximalgröße von %ls überschreitet."
"meghaladja %ls beállított maximumát."
"przekraczającą ustalone maksimum %ls."
"cual excede el tamaño máximo configurado de %ls."
"що перевищує задане обмеження у %ls."

MEditROOpen
"Вы хотите редактировать его?"
"Do you wish to edit it?"
"Opravdu si ho přejete upravit?"
"Wollen Sie sie dennoch bearbeiten?"
"Mégis szerkeszti?"
"Czy chcesz go edytować?"
"Desea editarlo?"
"Ви хочете редагувати його?"

MEditCanNotEditDirectory
l:
"Невозможно редактировать папку"
"It is impossible to edit the folder"
"Nelze editovat adresář"
"Es ist nicht möglich den Ordner zu bearbeiten"
"A mappa nem szerkeszthető"
"Nie można edytować folderu"
"Es imposible editar el directorio"
"Неможливо редагувати теку"

MEditSearchTitle
l:
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keresés"
"Szukaj"
"Buscar"
"Пошук"

MEditSearchFor
"&Искать"
"&Search for"
"&Hledat"
"&Suchen nach"
"&Keresés:"
"&Znajdź"
"&Buscar por"
"&Шукати"

MEditSearchCase
"&Учитывать регистр"
"&Case sensitive"
"&Rozlišovat velikost písmen"
"G&roß-/Kleinschrb."
"&Nagy/kisbetű érz."
"&Uwzględnij wielkość liter"
"Sensible min/ma&y"
"&Враховувати регістр"

MEditSearchWholeWords
"Только &целые слова"
"&Whole words"
"&Celá slova"
"&Ganze Wörter"
"Csak e&gész szavak"
"Tylko całe słowa"
"&Palabras completas"
"Тільки &цілі слова"

MEditSearchReverse
"Обратн&ый поиск"
"Re&verse search"
"&Zpětné hledání"
"Richtung um&kehren"
"&Visszafelé keres"
"Szukaj w &odwrotnym kierunku"
"Búsqueda in&versa"
"Зворотн&ий пошук"

MEditSearchSelFound
"&Выделять найденное"
"Se&lect found"
"Vy&ber nalezené"
"Treffer &markieren"
"&Találat kijelölése"
"W&ybierz znalezione"
"Se&leccionado encontrado"
"&Виділяти знайдене"

MEditSearchRegexp
"&Регулярные выражения"
"Re&gular expressions"
upd:"Re&gular expressions"
upd:"Re&gular expressions"
upd:"Re&gular expressions"
upd:"Re&gular expressions"
"Expresiones re&gulares"
"&Регулярні вирази"

MEditSearchSearch
"Искать"
"Search"
"Hledat"
"Suchen"
"Kere&sés"
"&Szukaj"
"Buscar"
"Шукати"

MEditSearchCancel
"Отменить"
"Cancel"
"Storno"
"Abbruch"
"&Mégsem"
"&Anuluj"
"Cancelar"
"Скасувати"

MEditReplaceTitle
l:
"Замена"
"Replace"
"Nahradit"
"Ersetzen"
"Keresés és csere"
"Zamień"
"Reemplazar"
"Заміна"

MEditReplaceWith
"Заменить &на"
"R&eplace with"
"Nahradit &s"
"&Ersetzen mit"
"&Erre cseréli:"
"Zamień &na"
"R&eemplazar con"
"Замінити &на"

MEditReplaceReplace
"&Замена"
"&Replace"
"&Nahradit"
"E&rsetzen"
"&Csere"
"Za&mień"
"&Reemplazar"
"&Заміна"

MEditSearchingFor
l:
"Искать"
"Searching for"
"Vyhledávám"
"Suche nach"
"Keresett szöveg:"
"Szukam"
"Buscando por"
"Шукати"

MEditNotFound
"Строка не найдена"
"Could not find the string"
"Nemůžu najít řetězec"
"Konnte Zeichenkette nicht finden"
"A szöveg nem található:"
"Nie mogę odnaleźć ciągu"
"No se puede encontrar la cadena"
"Рядок не знайдено"

MEditAskReplace
l:
"Заменить"
"Replace"
"Nahradit"
"Ersetze"
"Ezt cseréli:"
"Zamienić"
"Reemplazar"
"Замінити"

MEditAskReplaceWith
"на"
"with"
"s"
"mit"
"erre a szövegre:"
"na"
"con"
"на"

MEditReplace
"&Заменить"
"&Replace"
"&Nahradit"
"E&rsetzen"
"&Csere"
"&Zamień"
"&Reemplazar"
"&Замінити"

MEditReplaceAll
"&Все"
"&All"
"&Vše"
"&Alle"
"&Mindet"
"&Wszystkie"
"&Todos"
"&Усе"

MEditSkip
"&Пропустить"
"&Skip"
"Přes&kočit"
"Über&springen"
"&Kihagy"
"&Omiń"
"&Omitir"
"&Пропустити"

MEditCancel
"&Отменить"
"&Cancel"
"&Storno"
"Ab&bruch"
"Mé&gsem"
"&Anuluj"
"&Cancelar"
"&Скасувати"

MEditOpenCreateLabel
"&Открыть/создать файл:"
"&Open/create file:"
"Otevřít/vytvořit soubor:"
"Öffnen/datei erstellen:"
"Fájl megnyitása/&létrehozása:"
"&Otwórz/utwórz plik:"
"&Abrir/crear archivo:"
"&Відкрити/створити файл:"

MEditOpenAutoDetect
"&Автоматическое определение"
"&Automatic detection"
upd:"Automatic detection"
upd:"Automatic detection"
"&Automatikus felismerés"
"&Wykryj automatycznie"
"Deteccion &automática"
"&Автоматичне визначення"

MEditGoToLine
l:
"Перейти"
"Go to position"
"Jít na pozici"
"Gehe zu Zeile"
"Sorra ugrás"
"Idź do linii"
"Ir a posición"
"Перейти"

MBookmarksTitle
l:
"Закладки"
"Bookmarks"
"Adresářové zkratky"
"Ordnerschnellzugriff"
"Mappa gyorsbillentyűk"
"Skróty katalogów"
"Accesos a directorio"
"Закладки"

MPluginsTitle
l:
"Плагины"
"Plugins"
upd:"Plugins"
upd:"Plugins"
upd:"Plugins"
upd:"Plugins"
upd:"Plugins"
"Плагіни"

MVTStop
l:
"Завершение фоновой оболочки."
"Closing back shell."
upd:"Closing back shell."
upd:"Closing back shell."
upd:"Closing back shell."
upd:"Closing back shell."
upd:"Closing back shell."
"Завершення фонової оболонки."

MVTStopTip
l:
"Подсказка: чтобы закрыть far2l - введите 'exit far'."
"TIP: To close far2l - type 'exit far'."
upd:"TIP: To close far2l - type 'exit far'."
upd:"TIP: To close far2l - type 'exit far'."
upd:"TIP: To close far2l - type 'exit far'."
upd:"TIP: To close far2l - type 'exit far'."
upd:"TIP: To close far2l - type 'exit far'."
"Підказка: щоб закрити far2l - введіть 'exit far'."

MVTStartTipNoCmdTitle
l:
"При наборе команды:                                                       "
"While typing command:                                                     "
upd:"While typing command:                                                     "
upd:"While typing command:                                                     "
upd:"While typing command:                                                     "
upd:"While typing command:                                                     "
upd:"While typing command:                                                     "
"При наборі команди:                                                       "

MVTStartTipNoCmdCtrlO
l:
" Ctrl+O - переключения панель/терминал.                                   "
" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
upd:" Ctrl+O - switch between panel/terminal.                                  "
" Ctrl+O - перемикання панель/термінал.                                    "

MVTStartTipNoCmdCtrlArrow
l:
" Ctrl+Вверх/+Вниз/+Влево/+Вправо - изменение размера панелей.             "
" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
upd:" Ctrl+Up/+Down/+Left/+Right - adjust panels dimensions.                   "
" Ctrl+Вгору/+Вниз/+Вліво/+Вправо - зміна розміру панелей.                 "

MVTStartTipNoCmdShiftTAB
l:
" Двойной Shift+TAB - автодополнение от bash.                              "
" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
upd:" Double Shift+TAB - bash-guided autocomplete.                             "
"Подвійний Shift+TAB - автодоповнення від bash.                            "

MVTStartTipNoCmdFn
l:
" F3, F4, F8 - просмотр/редактор/очистка лога терминала при выкл. панелях. "
" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
upd:" F3, F4, F8 - viewer/editor/clear terminal log (if panels are off).       "
" F3, F4, F8 - перегляд/редактор/очищення лога терміналу при вимкн. панелях."

MVTStartTipNoCmdMouse
l:
" Ctrl+Shift+MouseScrollUp - автозавершающийся просмотр лога терминала.    "
" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
upd:" Ctrl+Shift+MouseScrollUp - open autoclosing viewer with terminal log.    "
" Ctrl+Shift+MouseScrollUp - автозавершення перегляду лога терміналу.      "

MVTStartTipPendCmdTitle
l:
"В процессе исполнения команды:                                            "
"While executing command:                                                  "
upd:"While executing command:                                                  "
upd:"While executing command:                                                  "
upd:"While executing command:                                                  "
upd:"While executing command:                                                  "
upd:"While executing command:                                                  "
"У процесі виконання команди:                                              "

MVTStartTipPendCmdFn
l:
" Ctrl+Shift+F3/+F4 - пауза и открытие просмотра/редактора лога терминала. "
" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
upd:" Ctrl+Shift+F3/+F4 - pause and open viewer/editor with console log.       "
" Ctrl+Shift+F3/+F4 - пауза та відкриття перегляду/редактора лога терміналу."

MVTStartTipPendCmdCtrlAltC
l:
" Ctrl+Alt+C - завершить все процессы в этой оболочке.                     "
" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
upd:" Ctrl+Alt+C - terminate everything in this shell.                         "
" Ctrl+Alt+C - завершити всі процеси в цій оболонці.                       "

MVTStartTipPendCmdCtrlAltZ
l:
" Ctrl+Alt+Z - отправить процесс far2l в фон, освободив терминал.          "
" Ctrl+Alt+Z - detach far2l application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2l application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2l application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2l application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2l application to background releasing terminal.  "
upd:" Ctrl+Alt+Z - detach far2l application to background releasing terminal.  "
" Ctrl+Alt+Z - надіслати процес far2l у фон, звільнивши термінал.          "

MVTStartTipPendCmdMouse
l:
" MouseScrollUp - автозавершающийся просмотр лога терминала.               "
" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
upd:" MouseScrollUp - pause and open autoclosing viewer with console log.      "
" MouseScrollUp - перегляд лога терміналу, що завершується автоматично.    "

MBookmarkBottom
"Редактирование: Del,Ins,F4,Shift+Вверх,Shift+Вниз"
"Edit: Del,Ins,F4,Shift+Up,Shift+Down"
"Edit: Del,Ins,F4,Shift+Up,Shift+Down"
"Bearb.: Entf,Einf,F4,Shift+Up,Shift+Down"
"Szerk.: Del,Ins,F4,Shift+Up,Shift+Down"
"Edycja: Del,Ins,F4,Shift+Up,Shift+Down"
"Editar: Del,Ins,F4,Shift+Up,Shift+Down"
"Редагування: Del,Ins,F4,Shift+Вгору,Shift+Вниз"

MShortcutNone
"<отсутствует>"
"<none>"
"<není>"
"<keiner>"
"<nincs>"
"<brak>"
"<nada>"
"<відсутня>"

MShortcutPlugin
"<плагин>"
"<plugin>"
"<plugin>"
"<Plugin>"
"<plugin>"
"<plugin>"
"<plugin>"
"<плагін>"

MFSShortcut
"Введите новую закладку:"
"Enter bookmark path:"
"Zadejte novou zkratku:"
"Neue Verknüpfung:"
"A gyorsbillentyűhöz rendelt mappa:"
"Wprowadź nowy skrót:"
"Ingrése nuevo acceso:"
"Введіть нову закладку:"

MNeedNearPath
"Перейти в ближайшую доступную папку?"
"Jump to the nearest existing folder?"
"Skočit na nejbližší existující adresář?"
"Zum nahesten existierenden Ordner springen?"
"Ugrás a legközelebbi létező mappára?"
"Przejść do najbliższego istniejącego folderu?"
"Saltar al próximo directorio existente"
"Перейти до доступної теки?"

MSaveThisShortcut
"Запомнить эту закладку?"
"Save this bookmark?"
"Uložit tyto zkratky?"
"Verknüpfung speichern?"
"Mentsem a gyorsbillentyűket?"
"Zapisać skróty?"
"Guardar estos accesos"
"Запам'ятати цю закладку?"

MEditF1
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

MEditF2
"Сохран"
"Save"
"Uložit"
"Speich"
"Mentés"
"Zapisz"
"Guarda"
"Збргти"

MEditF3
""
""
""
""
""
""
""
""

MEditF4
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

MEditF5
""
""
""
""
""
""
""
""

MEditF6
"Просм"
"View"
"Zobraz"
"Betr."
"Megnéz"
"Zobacz"
"Ver "
"Прогл"

MEditF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Szukaj"
"Buscar"
"Пошук"

MEditF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Latin 2"
"ANSI"
"ANSI"

MEditF9
""
""
""
""
""
""
""
""

MEditF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

MEditF11
"Модули"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Модулі"

MEditF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant. "
"Екрани"

MEditF8DOS
le:// don't count this - it's a F8 another text
"OEM"
"OEM"
"OEM"
"OEM"
"OEM"
"CP-1250"
"OEM"
"OEM"

MViewF5Processed
le:// don't count this - it's a F5 another text
"Обработ"
"Proc-d"
"Proc-d"
"Proc-d"
"Proc-d"
"Proc-d"
"Proc-d"
"Обробно"

MViewF5Raw
le:// don't count this - it's a F5 another text
"Сырой"
"Raw"
"Raw"
"Raw"
"Raw"
"Raw"
"Raw"
"Сирий"

MEditShiftF1
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

MEditShiftF2
"Сохр.в"
"SaveAs"
"UlJako"
"SpeiUn"
"Ment.."
"Zapisz"
"Grbcom"
"Збер.в"

MEditShiftF3
""
""
""
""
""
""
""
""

MEditShiftF4
"Редак."
"Edit.."
"Edit.."
"Bear.."
"Szrk.."
"Edytuj"
"Editar."
"Редаг."

MEditShiftF5
""
""
""
""
""
""
""
""

MEditShiftF6
""
""
""
""
""
""
""
""

MEditShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"TovKer"
"Następ"
"Próximo"
"Далі"

MEditShiftF8
"КодСтр"
"CodePg"
upd:"ZnSady"
upd:"Tabell"
"Kódlap"
"Tabela"
"CodePag"
"КодСтор"

MEditShiftF9
""
""
""
""
""
""
""
""

MEditShiftF10
"СхрВых"
"SaveQ"
"UlKone"
"SaveQ"
"MentKi"
"ZapKon"
"GrdySal"
"ЗбрВих"

MEditShiftF11
""
""
""
""
""
""
""
""

MEditShiftF12
""
""
""
""
""
""
""
""

MEditAltF1
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

MEditAltF2
""
""
""
""
""
""
""
""

MEditAltF3
""
""
""
""
""
""
""
""

MEditAltF4
""
""
""
""
""
""
""
""

MEditAltF5
"Печать"
"Print"
"Tisk"
"Druck"
"Nyomt"
"Drukuj"
"Imprim"
"Друк"

MEditAltF6
""
""
""
""
""
""
""
""

MEditAltF7
"Назад"
"Prev"
"Předch"
"Letzt"
"VisKer"
"Poprz"
"Previo"
"Назад"

MEditAltF8
"Строка"
"Goto"
"Jít na"
"GeheZu"
"Ugrás"
"IdźDo"
"Ir a.."
"Рядок"

MEditAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

MEditAltF10
"Закр.Far"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"Закр.Far"

MEditAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"Historia"
"HisVer"
"ІстПр"

MEditAltF12
""
""
""
""
""
""
""
""

MEditCtrlF1
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

MEditCtrlF2
""
""
""
""
""
""
""
""

MEditCtrlF3
""
""
""
""
""
""
""
""

MEditCtrlF4
""
""
""
""
""
""
""
""

MEditCtrlF5
""
""
""
""
""
""
""
""

MEditCtrlF6
""
""
""
""
""
""
""
""

MEditCtrlF7
"Замена"
"Replac"
"Nahraď"
"Ersetz"
"Csere"
"Zamień"
"Remplz"
"Заміна"

MEditCtrlF8
""
""
""
""
""
""
""
""

MEditCtrlF9
""
""
""
""
""
""
""
""

MEditCtrlF10
"Позиц"
"GoFile"
"JdiSou"
"GehDat"
"FájlPz"
"GoFile"
"IrArch"
"Позиц"

MEditCtrlF11
""
""
""
""
""
""
""
""

MEditCtrlF12
""
""
""
""
""
""
""
""

MEditAltShiftF1
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

MEditAltShiftF2
""
""
""
""
""
""
""
""

MEditAltShiftF3
""
""
""
""
""
""
""
""

MEditAltShiftF4
""
""
""
""
""
""
""
""

MEditAltShiftF5
""
""
""
""
""
""
""
""

MEditAltShiftF6
""
""
""
""
""
""
""
""

MEditAltShiftF7
""
""
""
""
""
""
""
""

MEditAltShiftF8
""
""
""
""
""
""
""
""

MEditAltShiftF9
"Конфиг"
"Config"
"Nastav"
"Konfig"
"Beáll."
"Konfig"
"Config"
"Конфіг"

MEditAltShiftF10
""
""
""
""
""
""
""
""

MEditAltShiftF11
""
""
""
""
""
""
""
""

MEditAltShiftF12
""
""
""
""
""
""
""
""

MEditCtrlShiftF1
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

MEditCtrlShiftF2
""
""
""
""
""
""
""
""

MEditCtrlShiftF3
""
""
""
""
""
""
""
""

MEditCtrlShiftF4
""
""
""
""
""
""
""
""

MEditCtrlShiftF5
""
""
""
""
""
""
""
""

MEditCtrlShiftF6
""
""
""
""
""
""
""
""

MEditCtrlShiftF7
""
""
""
""
""
""
""
""

MEditCtrlShiftF8
""
""
""
""
""
""
""
""

MEditCtrlShiftF9
""
""
""
""
""
""
""
""

MEditCtrlShiftF10
""
""
""
""
""
""
""
""

MEditCtrlShiftF11
""
""
""
""
""
""
""
""

MEditCtrlShiftF12
""
""
""
""
""
""
""
""

MEditCtrlAltF1
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

MEditCtrlAltF2
""
""
""
""
""
""
""
""

MEditCtrlAltF3
""
""
""
""
""
""
""
""

MEditCtrlAltF4
""
""
""
""
""
""
""
""

MEditCtrlAltF5
""
""
""
""
""
""
""
""

MEditCtrlAltF6
""
""
""
""
""
""
""
""

MEditCtrlAltF7
""
""
""
""
""
""
""
""

MEditCtrlAltF8
""
""
""
""
""
""
""
""

MEditCtrlAltF9
""
""
""
""
""
""
""
""

MEditCtrlAltF10
""
""
""
""
""
""
""
""

MEditCtrlAltF11
""
""
""
""
""
""
""
""

MEditCtrlAltF12
""
""
""
""
""
""
""
""

MEditCtrlAltShiftF1
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

MEditCtrlAltShiftF2
""
""
""
""
""
""
""
""

MEditCtrlAltShiftF3
""
""
""
""
""
""
""
""

MEditCtrlAltShiftF4
""
""
""
""
""
""
""
""

MEditCtrlAltShiftF5
""
""
""
""
""
""
""
""

MEditCtrlAltShiftF6
""
""
""
""
""
""
""
""

MEditCtrlAltShiftF7
""
""
""
""
""
""
""
""

MEditCtrlAltShiftF8
""
""
""
""
""
""
""
""

MEditCtrlAltShiftF9
""
""
""
""
""
""
""
""

MEditCtrlAltShiftF10
""
""
""
""
""
""
""
""

MEditCtrlAltShiftF11
""
""
""
""
""
""
""
""

MEditCtrlAltShiftF12
le://End of functional keys (Editor)
""
""
""
""
""
""
""
""

MSingleEditF1
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

MSingleEditF2
"Сохран"
"Save"
"Uložit"
"Speich"
"Mentés"
"Zapisz"
"Guarda"
"Збереж"

MSingleEditF3
""
""
""
""
""
""
""
""

MSingleEditF4
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

MSingleEditF5
""
""
""
""
""
""
""
""

MSingleEditF6
"Просм"
"View"
"Zobraz"
"Betr."
"Megnéz"
"Zobacz"
"Ver"
"Прогл"

MSingleEditF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Szukaj"
"Buscar"
"Пошук"

MSingleEditF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Latin 2"
"ANSI"
"ANSI"

MSingleEditF9
""
""
""
""
""
""
""
""

MSingleEditF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

MSingleEditF11
"Модули"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Модулі"

MSingleEditF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant. "
"Екрани"

MSingleEditF8DOS
le:// don't count this - it's a F8 another text
"OEM"
"OEM"
"OEM"
"OEM"
"OEM"
"CP 1250"
"OEM"
"OEM"

MSingleEditShiftF1
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

MSingleEditShiftF2
"Сохр.в"
"SaveAs"
"UlJako"
"SpeiUn"
"Ment.."
"Zapisz"
"Guarcm"
"Збер.в"

MSingleEditShiftF3
""
""
""
""
""
""
""
""

MSingleEditShiftF4
""
""
""
""
""
""
""
""

MSingleEditShiftF5
""
""
""
""
""
""
""
""

MSingleEditShiftF6
""
""
""
""
""
""
""
""

MSingleEditShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"TovKer"
"Następ"
"Próxim"
"Далі"

MSingleEditShiftF8
"КодСтр"
"CodePg"
upd:"ZnSady"
upd:"Tabell"
"Kódlap"
"Tabela"
"Tabla"
"КдСтор"

MSingleEditShiftF9
""
""
""
""
""
""
""
""

MSingleEditShiftF10
"СхрВых"
"SaveQ"
"UlKone"
"SaveQ"
"MentKi"
"ZapKon"
"GuaryS"
"СхрВих"

MSingleEditShiftF11
""
""
""
""
""
""
""
""

MSingleEditShiftF12
""
""
""
""
""
""
""
""

MSingleEditAltF1
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

MSingleEditAltF2
""
""
""
""
""
""
""
""

MSingleEditAltF3
""
""
""
""
""
""
""
""

MSingleEditAltF4
""
""
""
""
""
""
""
""

MSingleEditAltF5
"Печать"
"Print"
"Tisk"
"Druck"
"Nyomt"
"Drukuj"
"Imprime"
"Друк"

MSingleEditAltF6
""
""
""
""
""
""
""
""

MSingleEditAltF7
""
""
""
""
""
""
""
""

MSingleEditAltF8
"Строка"
"Goto"
"Jít na"
"GeheZu"
"Ugrás"
"IdźDo"
"Ir a.."
"Рядок"

MSingleEditAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Ekran"
"Video"
"Відео"

MSingleEditAltF10
""
""
""
""
""
""
""
""

MSingleEditAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"ZobHs"
"VerHis"
"ІстПр"

MSingleEditAltF12
""
""
""
""
""
""
""
""

MSingleEditCtrlF1
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

MSingleEditCtrlF2
""
""
""
""
""
""
""
""

MSingleEditCtrlF3
""
""
""
""
""
""
""
""

MSingleEditCtrlF4
""
""
""
""
""
""
""
""

MSingleEditCtrlF5
""
""
""
""
""
""
""
""

MSingleEditCtrlF6
""
""
""
""
""
""
""
""

MSingleEditCtrlF7
"Замена"
"Replac"
"Nahraď"
"Ersetz"
"Csere"
"Zastąp"
"Remplz"
"Заміна"

MSingleEditCtrlF8
""
""
""
""
""
""
""
""

MSingleEditCtrlF9
""
""
""
""
""
""
""
""

MSingleEditCtrlF10
""
""
""
""
""
""
""
""

MSingleEditCtrlF11
""
""
""
""
""
""
""
""

MSingleEditCtrlF12
""
""
""
""
""
""
""
""

MSingleEditAltShiftF1
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

MSingleEditAltShiftF2
""
""
""
""
""
""
""
""

MSingleEditAltShiftF3
""
""
""
""
""
""
""
""

MSingleEditAltShiftF4
""
""
""
""
""
""
""
""

MSingleEditAltShiftF5
""
""
""
""
""
""
""
""

MSingleEditAltShiftF6
""
""
""
""
""
""
""
""

MSingleEditAltShiftF7
""
""
""
""
""
""
""
""

MSingleEditAltShiftF8
""
""
""
""
""
""
""
""

MSingleEditAltShiftF9
"Конфиг"
"Config"
"Nastav"
"Konfig"
"Beáll."
"Konfig"
"Config"
"Конфіг"

MSingleEditAltShiftF10
""
""
""
""
""
""
""
""

MSingleEditAltShiftF11
""
""
""
""
""
""
""
""

MSingleEditAltShiftF12
""
""
""
""
""
""
""
""

MSingleEditCtrlShiftF1
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

MSingleEditCtrlShiftF2
""
""
""
""
""
""
""
""

MSingleEditCtrlShiftF3
""
""
""
""
""
""
""
""

MSingleEditCtrlShiftF4
""
""
""
""
""
""
""
""

MSingleEditCtrlShiftF5
""
""
""
""
""
""
""
""

MSingleEditCtrlShiftF6
""
""
""
""
""
""
""
""

MSingleEditCtrlShiftF7
""
""
""
""
""
""
""
""

MSingleEditCtrlShiftF8
""
""
""
""
""
""
""
""

MSingleEditCtrlShiftF9
""
""
""
""
""
""
""
""

MSingleEditCtrlShiftF10
""
""
""
""
""
""
""
""

MSingleEditCtrlShiftF11
""
""
""
""
""
""
""
""

MSingleEditCtrlShiftF12
""
""
""
""
""
""
""
""

MSingleEditCtrlAltF1
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

MSingleEditCtrlAltF2
""
""
""
""
""
""
""
""

MSingleEditCtrlAltF3
""
""
""
""
""
""
""
""

MSingleEditCtrlAltF4
""
""
""
""
""
""
""
""

MSingleEditCtrlAltF5
""
""
""
""
""
""
""
""

MSingleEditCtrlAltF6
""
""
""
""
""
""
""
""

MSingleEditCtrlAltF7
""
""
""
""
""
""
""
""

MSingleEditCtrlAltF8
""
""
""
""
""
""
""
""

MSingleEditCtrlAltF9
""
""
""
""
""
""
""
""

MSingleEditCtrlAltF10
""
""
""
""
""
""
""
""

MSingleEditCtrlAltF11
""
""
""
""
""
""
""
""

MSingleEditCtrlAltF12
""
""
""
""
""
""
""
""

MSingleEditCtrlAltShiftF1
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

MSingleEditCtrlAltShiftF2
""
""
""
""
""
""
""
""

MSingleEditCtrlAltShiftF3
""
""
""
""
""
""
""
""

MSingleEditCtrlAltShiftF4
""
""
""
""
""
""
""
""

MSingleEditCtrlAltShiftF5
""
""
""
""
""
""
""
""

MSingleEditCtrlAltShiftF6
""
""
""
""
""
""
""
""

MSingleEditCtrlAltShiftF7
""
""
""
""
""
""
""
""

MSingleEditCtrlAltShiftF8
""
""
""
""
""
""
""
""

MSingleEditCtrlAltShiftF9
""
""
""
""
""
""
""
""

MSingleEditCtrlAltShiftF10
""
""
""
""
""
""
""
""

MSingleEditCtrlAltShiftF11
""
""
""
""
""
""
""
""

MSingleEditCtrlAltShiftF12
le://End of functional keys (Single Editor)
""
""
""
""
""
""
""
""

MEditSaveAs
l:
"Сохранить &файл как"
"Save file &as"
"Uložit soubor jako"
"Speichern &als"
"Fá&jl mentése, mint:"
"Zapisz plik &jako"
"Guardar archivo &como"
"Зберегти &файл як"

MEditCodePage
"&Кодовая страница:"
"&Code page:"
"Kódová stránka:"
"Codepage:"
"Kódlap:"
"&Strona kodowa:"
"&Código caracteres:"
"&Кодова сторінка:"

MEditAddSignature
"Добавить &сигнатуру (BOM)"
"Add &signature (BOM)"
"Přidat signaturu (BOM)"
"Sinatur hinzu (BOM)"
"Uni&code bájtsorrend jelzővel (BOM)"
"Dodaj &znacznik BOM"
"Añadir &signatura (BOM)"
"Додати &сигнатуру (BOM)"

MEditSaveAsFormatTitle
"Изменить перевод строки:"
"Change line breaks to:"
"Změnit zakončení řádků na:"
"Zeilenumbrüche setzen:"
"Sortörés konverzió:"
"Zamień znaki końca linii na:"
"Cambiar fin de líneas a:"
"Змінити розриви рядків на:"

MEditSaveOriginal
"&исходный формат"
"Do n&ot change"
"&Beze změny"
"Nicht verä&ndern"
"Nincs &konverzió"
"&Nie zmieniaj"
"N&o cambiar"
"&Вихідний формат"

MEditSaveDOS
"в форма&те DOS/Windows (CR LF)"
"&Dos/Windows format (CR LF)"
"&Dos/Windows formát (CR LF)"
"&Dos/Windows Format (CR LF)"
"&DOS/Windows formátum (CR LF)"
"Format &Dos/Windows (CR LF)"
"Formato &DOS/Windows (CR LF)"
"у форма&ті DOS/Windows (CR LF)"

MEditSaveUnix
"в формат&е UNIX (LF)"
"&Unix format (LF)"
"&Unix formát (LF)"
"&Unix Format (LF)"
"&UNIX formátum (LF)"
"Format &Unix (LF)"
"Formato &Unix (LF)"
"у формат&і UNIX (LF)"

MEditSaveMac
"в фор&мате MAC (CR)"
"&Mac format (CR)"
"&Mac formát (CR)"
"&Mac Format (CR)"
"&Mac formátum (CR)"
"Format &Mac (CR)"
"Formato &Mac (CR)"
"у фор&маті MAC (CR)"

MEditCannotSave
"Ошибка сохранения файла"
"Cannot save the file"
"Nelze uložit soubor"
"Kann die Datei nicht speichern"
"A fájl nem menthető"
"Nie mogę zapisać pliku"
"No se puede guardar archivo"
"Помилка збереження файлу"

MEditSavedChangedNonFile
"Файл изменён, но файл или папка, в которой он находился,"
"The file is changed but the file or the folder containing"
"Soubor je změněn, ale soubor, nebo adresář obsahující"
"Inhalt dieser Datei wurde verändert aber die Datei oder der Ordner, welche"
"A fájl megváltozott, de a fájlt vagy a mappáját"
"Plik został zmieniony, ale plik lub folder zawierający"
"El archivo es cambiado pero el archivo o el directorio que contiene"
"Файл змінено, але файл або тека, в якій він знаходився,"

MEditSavedChangedNonFile1
"Файл или папка, в которой он находился,"
"The file or the folder containing"
"Soubor nebo adresář obsahující"
"Die Datei oder der Ordner, welche"
"A fájlt vagy a mappáját"
"Plik lub folder zawierający"
"El archivo o el directorio conteniendo"
"Файл або тека, де він знаходився,"

MEditSavedChangedNonFile2
"был перемещён или удалён. Сохранить?"
"this file was moved or deleted. Save?"
upd:"tento soubor byl přesunut, nebo smazán. Save?"
upd:"diesen Inhalt enthält wurde verschoben oder gelöscht. Save?"
upd:"időközben áthelyezte/átnevezte vagy törölte. Save?"
upd:"ten plik został przeniesiony lub usunięty. Save?"
"este archivo ha sido movido o borrado. Desea guardarlo?"
"Було переміщено або видалено. Зберегти?"

MEditNewPath1
"Путь к редактируемому файлу не существует,"
"The path to the edited file does not exist,"
"Cesta k editovanému souboru neexistuje,"
"Der Pfad zur bearbeiteten Datei existiert nicht,"
"A szerkesztendő fájl célmappája még"
"Ścieżka do edytowanego pliku nie istnieje,"
"La ruta del archivo editado no existe,"
"Шлях до редагованого файлу не існує,"

MEditNewPath2
"но будет создан при сохранении файла."
"but will be created when the file is saved."
"ale bude vytvořena při uložení souboru."
"aber wird erstellt sobald die Datei gespeichert wird."
"nem létezik, de mentéskor létrejön."
"ale zostanie utworzona po zapisaniu pliku."
"pero será creada cuando el archivo sea guardado."
"але буде створено при збереженні файлу."

MEditNewPath3
"Продолжать?"
"Continue?"
"Pokračovat?"
"Fortsetzen?"
"Folytatja?"
"Kontynuować?"
"Continuar?"
"Продовжувати?"

MEditNewPlugin1
"Имя редактируемого файла не может быть пустым"
"The name of the file to edit cannot be empty"
"Název souboru k editaci nesmí být prázdné"
"Der Name der zu editierenden Datei kann nicht leer sein"
"A szerkesztendő fájlnak nevet kell adni"
"Nazwa pliku do edycji nie może być pusta"
"El nombre del archivo a editar no puede estar vacío"
"Ім'я файлу, що редагується, не може бути порожнім"

MEditorLoadCPWarn1
"Файл содержит символы, которые невозможно"
"File contains characters, which cannot be"
upd:"File contains characters, which cannot be"
upd:"File contains characters, which cannot be"
upd:"File contains characters, which cannot be"
upd:"File contains characters, which cannot be"
"El archivo contiene caracteres que no pueden ser"
"Файл містить символи, які неможливо"

MEditorLoadCPWarn2
"корректно прочитать, используя выбранную кодовую страницу."
"correctly read using selected codepage."
upd:"correctly read using selected codepage."
upd:"correctly read using selected codepage."
upd:"correctly read using selected codepage."
upd:"correctly read using selected codepage."
"correctamente leídos con la tabla (codepage) seleccionada."
"коректно прочитати, використовуючи вибрану кодову сторінку."

MEditorSaveCPWarn1
"Редактор содержит символы, которые невозможно"
"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
"El editor contiene caracteres que no pueden ser"
"Редактор містить символи, які неможливо"

MEditorSaveCPWarn2
"корректно сохранить, используя выбранную кодовую страницу."
"correctly saved using selected codepage."
upd:"correctly saved using selected codepage."
upd:"correctly saved using selected codepage."
upd:"correctly saved using selected codepage."
upd:"correctly saved using selected codepage."
"correctamente guardados con la tabla (codepage) seleccionada."
"коректно зберегти, використовуючи вибрану кодову сторінку."

MEditorSwitchCPWarn1
"Редактор содержит символы, которые невозможно"
"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
upd:"Editor contains characters, which cannot be"
"El editor contiene caracteres que no pueden ser"
"Редактор містить символи, які неможливо"

MEditorSwitchCPWarn2
"корректно преобразовать, используя выбранную кодовую страницу."
"correctly translated using selected codepage."
upd:"correctly translated using selected codepage."
upd:"correctly translated using selected codepage."
upd:"correctly translated using selected codepage."
upd:"correctly translated using selected codepage."
"correctamente traducidos con la tabla (codepage) seleccionada."
"коректно перетворити, використовуючи вибрану кодову сторінку."

MEditDataLostWarn
"Во время редактирования файла некоторые данные были утеряны."
"During file editing some data was lost."
upd:"During file editing some data was lost."
upd:"During file editing some data was lost."
upd:"During file editing some data was lost."
upd:"During file editing some data was lost."
"Durante la edición del archivo algunos datos se perdieron."
"Під час редагування файлу деякі дані були втрачені."

MEditorSaveNotRecommended
"Сохранять файл не рекомендуется."
"It is not recommended to save this file."
"Není doporučeno uložit tento soubor."
"Es wird empfohlen, die Datei nicht zu speichern."
"A fájl mentése nem ajánlott."
"Odradzamy zapis pliku."
"No se recomienda guardar este archivo."
"Не рекомендується зберігати файл."

MEditorSaveCPWarnShow
"Показать"
"Show"
upd:"Show"
upd:"Show"
upd:"Show"
upd:"Show"
"Mostrar"
"Показати"

MColumnName
l:
"Имя"
"Name"
"Název"
"Name"
"Név"
"Nazwa"
"Nombre"
"Ім'я"

MColumnSize
"Размер"
"Size"
"Velikost"
"Größe"
"Méret"
"Rozmiar"
"Tamaño"
"Розмір"

MColumnPhysical
"ФизРзм"
"PhysSz"
upd:"PhysSz"
upd:"PhysSz"
upd:"PhysSz"
upd:"PhysSz"
upd:"PhysSz"
"ФізРзм"

MColumnDate
"Дата"
"Date"
"Datum"
"Datum"
"Dátum"
"Data"
"Fecha"
"Дата"

MColumnTime
"Время"
"Time"
"Čas"
"Zeit"
"Idő"
"Czas"
"Hora"
"Час"

MColumnWrited
"Запись"
"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
"Запис"

MColumnCreated
"Создание"
"Created"
"Vytvořen"
"Erstellt"
"Létrejött"
"Utworzenie"
"Creado "
"Створення"

MColumnAccessed
"Доступ"
"Accessed"
"Přístup"
"Zugriff"
"Hozzáférés"
"Użycie"
"Acceso  "
"Доступ"

MColumnChanged
"Изменение"
upd:"Change"
upd:"Change"
upd:"Change"
upd:"Change"
upd:"Change"
upd:"Change"
"Змінення"

MColumnAttr
"Атриб"
"Attr"
"Attr"
"Attr"
"Attrib"
"Atrybuty"
"Atrib"
"Атриб"

MColumnDescription
"Описание"
"Description"
"Popis"
"Beschreibung"
"Megjegyzés"
"Opis"
"Descripción"
"Опис"

MColumnOwner
"Владелец"
"Owner"
"Vlastník"
"Besitzer"
"Tulajdonos"
"Właściciel"
"Dueño"
"Власник"

MColumnGroup
"Группа"
"Group"
upd:"Group"
upd:"Group"
upd:"Group"
upd:"Group"
upd:"Group"
"Група"

MColumnMumLinks
"КлС"
"NmL"
"PočLn"
"AnL"
"Lnk"
"NmL"
"NmL"
"КлС"

MListUp
l:
"Вверх"
"  Up  "
"Nahoru"
" Hoch "
"  Fel  "
"W górę"
"UP-DIR"
"Вгору"

MListFolder
"Папка"
"Folder"
"Adresář"
"Ordner"
" Mappa "
"Folder"
" DIR  "
"Тека"

MListSymLink
"Ссылка"
"Symlink"
"Link"
"Symlink"
"SzimLnk"
"LinkSym"
" Enlac"
"Посилання"

MListJunction
"Связь"
"Junction"
"Křížení"
"Knoten"
"Csomópt"
"Dowiązania"
" Junc "
"Зв'язок"

MListBytes
"Б"
"B"
"B"
"B"
"B"
"B"
"B"
"Б"

MListKb
"К"
"K"
"K"
"K"
"k"
"K"
"K"
"К"

MListMb
"М"
"M"
"M"
"M"
"M"
"M"
"M"
"М"

MListGb
"Г"
"G"
"G"
"G"
"G"
"G"
"G"
"Г"

MListTb
"Т"
"T"
"T"
"T"
"T"
"T"
"T"
"Т"

MListPb
"П"
"P"
"P"
"P"
"P"
"P"
"P"
"П"

MListEb
"Э"
"E"
"E"
"E"
"E"
"E"
"E"
"Э"

MListFileSize
" %ls байт в 1 файле "
" %ls bytes in 1 file "
" %ls bytů v 1 souboru "
" %ls Bytes in 1 Datei "
" %ls bájt 1 fájlban "
" %ls bajtów w 1 pliku "
" %ls bytes en 1 archivo "
" %ls байт у 1 файлі "

MListFilesSize1
" %ls байт в %d файле "
" %ls bytes in %d files "
" %ls bytů v %d souborech "
" %ls Bytes in %d Dateien "
" %ls bájt %d fájlban "
" %ls bajtów w %d plikach "
" %ls bytes en %d archivos "
" %ls байт у %d файлі "

MListFilesSize2
" %ls байт в %d файлах "
" %ls bytes in %d files "
" %ls bytů v %d souborech "
" %ls Bytes in %d Dateien "
" %ls bájt %d fájlban "
" %ls bajtów w %d plikach "
" %ls bytes en %d archivos "
" %ls байт у %d файлах "

MListFreeSize
" %ls байт свободно "
" %ls free bytes "
" %ls volných bytů "
" %ls freie Bytes "
" %ls szabad bájt "
" %ls wolnych bajtów "
" %ls bytes libres "
" %ls байт вільно "

MDirInfoViewTitle
l:
"Просмотр"
"View"
"Zobraz"
"Betrachten"
"Vizsgálat"
"Podgląd"
"Ver "
"Перегляд"

MFileToEdit
"Редактировать файл:"
"File to edit:"
"Soubor k editaci:"
"Datei bearbeiten:"
"Szerkesztendő fájl:"
"Plik do edycji:"
"archivo a editar:"
"Редагувати файл:"

MUnselectTitle
l:
"Снять"
"Deselect"
"Odznačit"
"Abwählen"
"Kijelölést levesz"
"Odznacz"
"Deseleccionar"
"Зняти"

MSelectTitle
"Пометить"
"Select"
"Označit"
"Auswählen"
"Kijelölés"
"Zaznacz"
"Seleccionar"
"Позначити"

MSelectFilter
"&Фильтр"
"&Filter"
"&Filtr"
"&Filter"
"&Szűrő"
"&Filtruj"
"&Filtro"
"&Фільтр"

MCompareTitle
l:
"Сравнение"
"Compare"
"Porovnat"
"Vergleichen"
"Összehasonlítás"
"Porównaj"
"Comparar"
"Порівняння"

MCompareFilePanelsRequired1
"Для сравнения папок требуются"
"Two file panels are required to perform"
"Pro provedení příkazu Porovnat adresáře"
"Zwei Dateipanels werden benötigt um"
"Mappák összehasonlításához"
"Aby porównać katalogi konieczne są"
"Dos paneles de archivos son necesarios para poder"
"Для порівняння тек потрібні"

MCompareFilePanelsRequired2
"две файловые панели"
"the Compare folders command"
"jsou nutné dva souborové panely"
"den Vergleich auszuführen."
"két fájlpanel szükséges"
"dwa zwykłe panele plików"
"utilizar el comando comparar directorios"
"дві файлові панелі"

MCompareSameFolders1
"Содержимое папок,"
"The folders contents seems"
"Obsahy adresářů jsou"
"Der Inhalt der beiden Ordner scheint"
"A mappák tartalma"
"Zawartość katalogów"
"El contenido de los directorios parecen"
"Вміст тек,"

MCompareSameFolders2
"скорее всего, одинаково"
"to be identical"
"identické"
"identisch zu sein."
"azonosnak tűnik"
"wydaje się być identyczna"
"ser idénticos"
"швидше за все, однаково"

MSelectAssocTitle
l:
"Выберите ассоциацию"
"Select association"
"Vyber závislosti"
"Dateiverknüpfung auswählen"
"Válasszon társítást"
"Wybierz przypisanie"
"Seleccionar asociaciones"
"Виберіть асоціацію"

MAssocTitle
l:
"Ассоциации для файлов"
"File associations"
"Závislosti souborů"
"Dateiverknüpfungen"
"Fájltársítások"
"Przypisania plików"
"Asociación de archivos"
"Асоціації для файлів"

MAssocBottom
"Редактирование: Del,Ins,F4"
"Edit: Del,Ins,F4"
"Edit: Del,Ins,F4"
"Bearb.: Entf,Einf,F4"
"Szerk.: Del,Ins,F4"
"Edycja: Del,Ins,F4"
"Editar: Del,Ins,F4"
"Редагування: Del,Ins,F4"

MAskDelAssoc
"Вы хотите удалить ассоциацию для"
"Do you wish to delete association for"
"Přejete si smazat závislost pro"
"Wollen Sie die Verknüpfung löschen für"
"Törölni szeretné a társítást?"
"Czy chcesz usunąć przypisanie dla"
"Desea borrar la asociación para"
"Ви хочете видалити асоціацію для"

MFileAssocTitle
l:
"Редактирование ассоциаций файлов"
"Edit file associations"
"Upravit závislosti souborů"
"Dateiverknüpfungen bearbeiten"
"Fájltársítások szerkesztése"
"Edytuj przypisania pliku"
"Editar asociación de archivos"
"Редагування асоціацій файлів"

MFileAssocMasks
"Одна или несколько &масок файлов:"
"A file &mask or several file masks:"
"&Maska nebo masky souborů:"
"Datei&maske (mehrere getrennt mit Komma):"
"F&ájlmaszk(ok, vesszővel elválasztva):"
"&Maska pliku lub kilka masek oddzielonych przecinkami:"
"&Máscara de archivo o múltiples máscaras de archivos:"
"Одна або кілька &масок файлів:"

MFileAssocDescr
"&Описание ассоциации:"
"&Description of the association:"
"&Popis asociací:"
"&Beschreibung der Verknüpfung:"
"A &társítás leírása:"
"&Opis przypisania:"
"&Descripción de la asociación:"
"&Опис асоціації:"

MFileAssocExec
"Команда, &выполняемая по Enter:"
"E&xecute command (used for Enter):"
"&Vykonat příkaz (použito pro Enter):"
"Befehl &ausführen (mit Enter):"
"&Végrehajtandó parancs (Enterre):"
"Polecenie (po naciśnięciu &Enter):"
"E&jecutar comando (usado por Enter):"
"Команда, &яка виконується за Enter:"

MFileAssocAltExec
"Коман&да, выполняемая по Ctrl-PgDn:"
"Exec&ute command (used for Ctrl-PgDn):"
"V&ykonat příkaz (použito pro Ctrl-PgDn):"
"Befehl a&usführen (mit Strg-PgDn):"
"Vé&grehajtandó parancs (Ctrl-PgDown-ra):"
"Polecenie (po naciśnięciu &Ctrl-PgDn):"
"Ejecutar comando (usado por Ctrl-PgDn):"
"Коман&да, що виконується за Ctrl-PgDn:"

MFileAssocView
"Команда &просмотра, выполняемая по F3:"
"&View command (used for F3):"
"Příkaz &Zobraz (použito pro F3):"
"Be&trachten (mit F3):"
"&Nézőke parancs (F3-ra):"
"&Podgląd (po naciśnięciu F3):"
"Comando de &visor (usado por F3):"
"Команда &перегляду, що виконується за F3:"

MFileAssocAltView
"Команда просмотра, в&ыполняемая по Alt-F3:"
"V&iew command (used for Alt-F3):"
"Příkaz Z&obraz (použito pro Alt-F3):"
"Bet&rachten (mit Alt-F3):"
"N&ézőke parancs (Alt-F3-ra):"
"Podg&ląd (po naciśnięciu Alt-F3):"
"Comando de visor (usado por Alt-F3):"
"Команда перегляду, що в&иконується за Alt-F3:"

MFileAssocEdit
"Команда &редактирования, выполняемая по F4:"
"&Edit command (used for F4):"
"Příkaz &Edituj (použito pro F4):"
"Bearb&eiten (mit F4):"
"S&zerkesztés parancs (F4-re):"
"&Edycja  (po naciśnięciu F4):"
"Comando de &editor (usado por F4):"
"Команда &редагування, що виконується за F4:"

MFileAssocAltEdit
"Команда редактировани&я, выполняемая по Alt-F4:"
"Edit comm&and (used for Alt-F4):"
"Příkaz Editu&j (použito pro Alt-F4):"
"Bearbe&iten (mit Alt-F4):"
"Sze&rkesztés parancs (Alt-F4-re):"
"E&dycja  (po naciśnięciu Alt-F4):"
"Comando de editor (usado por Alt-F4):"
"Команда редагуванн&я, що виконується за Alt-F4:"
MViewF1
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

MViewF2
le:// this is another text for F2
"Сверн"
"Wrap"
"Zalom"
"Umbr."
"SorTör"
"Zawiń"
"Divide"
"Згорн"

MViewF3
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

MViewF4
"Код"
"Hex"
"Hex"
"Hex"
"Hexa"
"Hex"
"Hexa"
"Код"

MViewF5
""
""
""
""
""
""
""
""

MViewF6
"Редакт"
"Edit"
"Edit"
"Bearb"
"Szerk."
"Edytuj"
"Editar"
"Редаг"

MViewF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Szukaj"
"Buscar"
"Пошук"

MViewF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Latin 2"
"ANSI"
"ANSI"

MViewF9
""
""
""
""
""
""
""
""

MViewF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

MViewF11
"Модули"
"Plugins"
"Plugin"
"Plugin"
"Plugin"
"Pluginy"
"Plugins"
"Модулі"

MViewF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant. "
"Екрани"

MViewF2Unwrap
"Развер"
"Unwrap"
"Nezal"
"KeinUm"
"NemTör"
"Unwrap"
"Unwrap"
"Розгор"

MViewF4Text
l:// this is another text for F4
"Текст"
"Text"
"Text"
"Text"
"Szöveg"
"Tekst"
"Text"
"Текст"

MViewF8DOS
"OEM"
"OEM"
"OEM"
"OEM"
"OEM"
"CP 1250"
"OEM"
"OEM"

MViewShiftF1
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

MViewShiftF2
"Слова"
"WWrap"
"ZalSlo"
"WUmbr"
"SzóTör"
"ZawińS"
"ConDiv"
"Слова"

MViewShiftF3
""
""
""
""
""
""
""
""

MViewShiftF4
""
""
""
""
""
""
""
""

MViewShiftF5
""
""
""
""
""
""
""
""

MViewShiftF6
""
""
""
""
""
""
""
""

MViewShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"TovKer"
"Następ"
"Próxim"
upd:"Далі"

MViewShiftF8
"КодСтр"
"CodePg"
upd:"ZnSady"
upd:"Tabell"
"Kódlap"
"Tabela"
"Tabla"
"КодСтор"

MViewShiftF9
""
""
""
""
""
""
""
""

MViewShiftF10
""
""
""
""
""
""
""
""

MViewShiftF11
""
""
""
""
""
""
""
""

MViewShiftF12
""
""
""
""
""
""
""
""

MViewAltF1
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

MViewAltF2
""
""
""
""
""
""
""
""

MViewAltF3
""
""
""
""
""
""
""
""

MViewAltF4
""
""
""
""
""
""
""
""

MViewAltF5
"Печать"
"Print"
"Tisk"
"Druck"
"Nyomt"
"Drukuj"
"Imprim"
"Друк"

MViewAltF6
""
""
""
""
""
""
""
""

MViewAltF7
"Назад"
"Prev"
"Předch"
"Letzt"
"VisKer"
"Poprz"
"Previo"
"Назад"

MViewAltF8
"Перейт"
"Goto"
"Jít na"
"GeheZu"
"Ugrás"
"IdźDo"
"Ir a.."
"Перейт"

MViewAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

MViewAltF10
"Закр.Far"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"Закр.Far"

MViewAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"Historia"
"HisVer"
"ІстПр"

MViewAltF12
""
""
""
""
""
""
""
""

MViewCtrlF1
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

MViewCtrlF2
""
""
""
""
""
""
""
""

MViewCtrlF3
""
""
""
""
""
""
""
""

MViewCtrlF4
""
""
""
""
""
""
""
""

MViewCtrlF5
""
""
""
""
""
""
""
""

MViewCtrlF6
""
""
""
""
""
""
""
""

MViewCtrlF7
""
""
""
""
""
""
""
""

MViewCtrlF8
""
""
""
""
""
""
""
""

MViewCtrlF9
""
""
""
""
""
""
""
""

MViewCtrlF10
"Позиц"
"GoFile"
"JítSou"
"GehDat"
"FájlPz"
"DoPlik"
"IrArch"
"Позиц"

MViewCtrlF11
""
""
""
""
""
""
""
""

MViewCtrlF12
""
""
""
""
""
""
""
""

MViewAltShiftF1
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

MViewAltShiftF2
""
""
""
""
""
""
""
""

MViewAltShiftF3
""
""
""
""
""
""
""
""

MViewAltShiftF4
""
""
""
""
""
""
""
""

MViewAltShiftF5
""
""
""
""
""
""
""
""

MViewAltShiftF6
""
""
""
""
""
""
""
""

MViewAltShiftF7
""
""
""
""
""
""
""
""

MViewAltShiftF8
""
""
""
""
""
""
""
""

MViewAltShiftF9
"Конфиг"
"Config"
"Nastav"
"Konfig"
"Beáll."
"Konfig"
"Config"
"Конфіг"

MViewAltShiftF10
""
""
""
""
""
""
""
""

MViewAltShiftF11
""
""
""
""
""
""
""
""

MViewAltShiftF12
""
""
""
""
""
""
""
""

MViewCtrlShiftF1
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

MViewCtrlShiftF2
""
""
""
""
""
""
""
""

MViewCtrlShiftF3
""
""
""
""
""
""
""
""

MViewCtrlShiftF4
""
""
""
""
""
""
""
""

MViewCtrlShiftF5
""
""
""
""
""
""
""
""

MViewCtrlShiftF6
""
""
""
""
""
""
""
""

MViewCtrlShiftF7
""
""
""
""
""
""
""
""

MViewCtrlShiftF8
""
""
""
""
""
""
""
""

MViewCtrlShiftF9
""
""
""
""
""
""
""
""

MViewCtrlShiftF10
""
""
""
""
""
""
""
""

MViewCtrlShiftF11
""
""
""
""
""
""
""
""

MViewCtrlShiftF12
""
""
""
""
""
""
""
""

MViewCtrlAltF1
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

MViewCtrlAltF2
""
""
""
""
""
""
""
""

MViewCtrlAltF3
""
""
""
""
""
""
""
""

MViewCtrlAltF4
""
""
""
""
""
""
""
""

MViewCtrlAltF5
""
""
""
""
""
""
""
""

MViewCtrlAltF6
""
""
""
""
""
""
""
""

MViewCtrlAltF7
""
""
""
""
""
""
""
""

MViewCtrlAltF8
""
""
""
""
""
""
""
""

MViewCtrlAltF9
""
""
""
""
""
""
""
""

MViewCtrlAltF10
""
""
""
""
""
""
""
""

MViewCtrlAltF11
""
""
""
""
""
""
""
""

MViewCtrlAltF12
""
""
""
""
""
""
""
""

MViewCtrlAltShiftF1
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

MViewCtrlAltShiftF2
""
""
""
""
""
""
""
""

MViewCtrlAltShiftF3
""
""
""
""
""
""
""
""

MViewCtrlAltShiftF4
""
""
""
""
""
""
""
""

MViewCtrlAltShiftF5
""
""
""
""
""
""
""
""

MViewCtrlAltShiftF6
""
""
""
""
""
""
""
""

MViewCtrlAltShiftF7
""
""
""
""
""
""
""
""

MViewCtrlAltShiftF8
""
""
""
""
""
""
""
""

MViewCtrlAltShiftF9
""
""
""
""
""
""
""
""

MViewCtrlAltShiftF10
""
""
""
""
""
""
""
""

MViewCtrlAltShiftF11
""
""
""
""
""
""
""
""

MViewCtrlAltShiftF12
le://end of functional keys (Viewer)
""
""
""
""
""
""
""
""

MSingleViewF1
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

MSingleViewF2
"Сверн"
"Wrap"
"Zalom"
"Umbr."
"SorTör"
"Zawiń"
"Divide"
"Згорн"

MSingleViewF3
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

MSingleViewF4
"Код"
"Hex"
"Hex"
"Hex"
"Hexa"
"Hex"
"Hexa"
"Код"

MSingleViewF5
""
""
""
""
""
""
""
""

MSingleViewF6
"Редакт"
"Edit"
"Edit"
"Bearb"
"Szerk."
"Edytuj"
"Editar"
"Редаг"

MSingleViewF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Szukaj"
"Buscar"
"Пошук"

MSingleViewF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Latin 2"
"ANSI"
"ANSI"

MSingleViewF9
""
""
""
""
""
""
""
""

MSingleViewF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

MSingleViewF11
"Модули"
"Plugins"
"Plugin"
"Plugins"
"Plugin"
"Pluginy"
"Plugins"
"Модулі"

MSingleViewF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant.  "
"Екрани"

MSingleViewF2Unwrap
l:// this is another text for F2
"Развер"
"Unwrap"
"Nezal"
"KeinUm"
"NemTör"
"Rozwij"
"Unwrap"
"Розгор"

MSingleViewF4Text
l:// this is another text for F4
"Текст"
"Text"
"Text"
"Text"
"Szöveg"
"Tekst"
"Text"
"Текст"

MSingleViewF8DOS
"OEM"
"OEM"
"OEM"
"OEM"
"OEM"
"CP 1250"
"OEM"
"OEM"

MSingleViewShiftF1
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

MSingleViewShiftF2
"Слова"
"WWrap"
"ZalSlo"
"WUmbr"
"SzóTör"
"ZawińS"
"ConDiv"
"Слова"

MSingleViewShiftF3
""
""
""
""
""
""
""
""

MSingleViewShiftF4
""
""
""
""
""
""
""
""

MSingleViewShiftF5
""
""
""
""
""
""
""
""

MSingleViewShiftF6
""
""
""
""
""
""
""
""

MSingleViewShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"TovKer"
"Nast."
"Próxim"
"Далі"

MSingleViewShiftF8
"КодСтр"
"CodePg"
upd:"ZnSady"
upd:"Tabell"
"Kódlap"
"Tabela"
"Tabla"
"КодСтр"

MSingleViewShiftF9
""
""
""
""
""
""
""
""

MSingleViewShiftF10
""
""
""
""
""
""
""
""

MSingleViewShiftF11
""
""
""
""
""
""
""
""

MSingleViewShiftF12
""
""
""
""
""
""
""
""

MSingleViewAltF1
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

MSingleViewAltF2
""
""
""
""
""
""
""
""

MSingleViewAltF3
""
""
""
""
""
""
""
""

MSingleViewAltF4
""
""
""
""
""
""
""
""

MSingleViewAltF5
"Печать"
"Print"
"Tisk"
"Druck"
"Nyomt"
"Drukuj"
"Imprim"
"Друк"

MSingleViewAltF6
""
""
""
""
""
""
""
""

MSingleViewAltF7
"Назад"
"Prev"
"Předch"
"Letzt"
"VisKer"
"Poprz"
"Prev"
"Назад"

MSingleViewAltF8
"Перейт"
"Goto"
"Jít na"
"GeheZu"
"Ugrás"
"IdźDo"
"Ir a.."
"Перейт"

MSingleViewAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

MSingleViewAltF10
"Закр.Far"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"ExitFar"
"Закр.Far"

MSingleViewAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"Historia"
"HisVer"
"ІстПр"

MSingleViewAltF12
""
""
""
""
""
""
""
""

MSingleViewCtrlF1
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

MSingleViewCtrlF2
""
""
""
""
""
""
""
""

MSingleViewCtrlF3
""
""
""
""
""
""
""
""

MSingleViewCtrlF4
""
""
""
""
""
""
""
""

MSingleViewCtrlF5
""
""
""
""
""
""
""
""

MSingleViewCtrlF6
""
""
""
""
""
""
""
""

MSingleViewCtrlF7
""
""
""
""
""
""
""
""

MSingleViewCtrlF8
""
""
""
""
""
""
""
""

MSingleViewCtrlF9
""
""
""
""
""
""
""
""

MSingleViewCtrlF10
""
""
""
""
""
""
""
""

MSingleViewCtrlF11
""
""
""
""
""
""
""
""

MSingleViewCtrlF12
""
""
""
""
""
""
""
""

MSingleViewAltShiftF1
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

MSingleViewAltShiftF2
""
""
""
""
""
""
""
""

MSingleViewAltShiftF3
""
""
""
""
""
""
""
""

MSingleViewAltShiftF4
""
""
""
""
""
""
""
""

MSingleViewAltShiftF5
""
""
""
""
""
""
""
""

MSingleViewAltShiftF6
""
""
""
""
""
""
""
""

MSingleViewAltShiftF7
""
""
""
""
""
""
""
""

MSingleViewAltShiftF8
""
""
""
""
""
""
""
""

MSingleViewAltShiftF9
"Конфиг"
"Config"
"Nastav"
"Konfig"
"Beáll."
"Konfig"
"Config"
"Конфіг"

MSingleViewAltShiftF10
""
""
""
""
""
""
""
""

MSingleViewAltShiftF11
""
""
""
""
""
""
""
""

MSingleViewAltShiftF12
""
""
""
""
""
""
""
""

MSingleViewCtrlShiftF1
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

MSingleViewCtrlShiftF2
""
""
""
""
""
""
""
""

MSingleViewCtrlShiftF3
""
""
""
""
""
""
""
""

MSingleViewCtrlShiftF4
""
""
""
""
""
""
""
""

MSingleViewCtrlShiftF5
""
""
""
""
""
""
""
""

MSingleViewCtrlShiftF6
""
""
""
""
""
""
""
""

MSingleViewCtrlShiftF7
""
""
""
""
""
""
""
""

MSingleViewCtrlShiftF8
""
""
""
""
""
""
""
""

MSingleViewCtrlShiftF9
""
""
""
""
""
""
""
""

MSingleViewCtrlShiftF10
""
""
""
""
""
""
""
""

MSingleViewCtrlShiftF11
""
""
""
""
""
""
""
""

MSingleViewCtrlShiftF12
""
""
""
""
""
""
""
""

MSingleViewCtrlAltF1
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

MSingleViewCtrlAltF2
""
""
""
""
""
""
""
""

MSingleViewCtrlAltF3
""
""
""
""
""
""
""
""

MSingleViewCtrlAltF4
""
""
""
""
""
""
""
""

MSingleViewCtrlAltF5
""
""
""
""
""
""
""
""

MSingleViewCtrlAltF6
""
""
""
""
""
""
""
""

MSingleViewCtrlAltF7
""
""
""
""
""
""
""
""

MSingleViewCtrlAltF8
""
""
""
""
""
""
""
""

MSingleViewCtrlAltF9
""
""
""
""
""
""
""
""

MSingleViewCtrlAltF10
""
""
""
""
""
""
""
""

MSingleViewCtrlAltF11
""
""
""
""
""
""
""
""

MSingleViewCtrlAltF12
""
""
""
""
""
""
""
""

MSingleViewCtrlAltShiftF1
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

MSingleViewCtrlAltShiftF2
""
""
""
""
""
""
""
""

MSingleViewCtrlAltShiftF3
""
""
""
""
""
""
""
""

MSingleViewCtrlAltShiftF4
""
""
""
""
""
""
""
""

MSingleViewCtrlAltShiftF5
""
""
""
""
""
""
""
""

MSingleViewCtrlAltShiftF6
""
""
""
""
""
""
""
""

MSingleViewCtrlAltShiftF7
""
""
""
""
""
""
""
""

MSingleViewCtrlAltShiftF8
""
""
""
""
""
""
""
""

MSingleViewCtrlAltShiftF9
""
""
""
""
""
""
""
""

MSingleViewCtrlAltShiftF10
""
""
""
""
""
""
""
""

MSingleViewCtrlAltShiftF11
""
""
""
""
""
""
""
""

MSingleViewCtrlAltShiftF12
le://end of functional keys (Single Viewer)
""
""
""
""
""
""
""
""

MInViewer
"просмотр %ls"
"view %ls"
"prohlížení %ls"
"Betrachte %ls"
"%ls megnézése"
"podgląd %ls"
"ver %ls"
"перегляд %ls"

MInEditor
"редактирование %ls"
"edit %ls"
"editace %ls"
"Bearbeite %ls"
"%ls szerkesztése"
"edycja %ls"
"editar %ls"
"редагування %ls"

MFilterTitle
l:
"Меню фильтров"
"Filters menu"
"Menu filtrů"
"Filtermenü"
"Szűrők menü"
"Filtry"
"Menú de Filtros"
"Меню фільтрів"

MFilterBottom
"+,-,Пробел,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"
"+,-,Space,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"
"+,-,Mezera,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"
"+,-,Leer,I,X,BS,UmschBS,Einf,Entf,F4,F5,StrgUp,StrgDn"
"+,-,Szóköz,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Fel,Ctrl-Le"
"+,-,Spacja,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"
"Seleccione: '+','-',Space. Editor: Ins,Del,F4"
"+,-,Пробіл,I,X,BS,Shift-BS,Ins,Del,F4,F5,Ctrl-Up,Ctrl-Dn"

MPanelFileType
"Файлы панели"
"Panel file type"
"Typ panelu souborů"
"Dateityp in Panel"
"A panel fájltípusa"
"Typ plików w panelu"
"Tipo de panel de archivo"
"Файли панелі"

MFolderFileType
"Папки"
"Folders"
"Adresáře"
"Ordner"
"Mappák"
"Foldery"
"Directorios"
"Теки"

MCanEditCustomFilterOnly
"Только пользовательский фильтр можно редактировать"
"Only custom filter can be edited"
"Jedině vlastní filtr může být upraven"
"Nur eigene Filter können editiert werden."
"Csak saját szűrő szerkeszthető"
"Tylko filtr użytkownika może być edytowany"
"Sólo filtro personalizado puede ser editado"
"Тільки фільтр користувача можна редагувати"

MAskDeleteFilter
"Вы хотите удалить фильтр"
"Do you wish to delete the filter"
"Přejete si smazat filtr"
"Wollen Sie den eigenen Filter löschen"
"Törölni szeretné a szűrőt?"
"Czy chcesz usunąć filtr"
"Desea borrar el filtro"
"Ви хочете видалити фільтр"

MCanDeleteCustomFilterOnly
"Только пользовательский фильтр может быть удалён"
"Only custom filter can be deleted"
"Jedině vlastní filtr může být smazán"
"Nur eigene Filter können gelöscht werden."
"Csak saját szűrő törölhető"
"Tylko filtr użytkownika może być usunięty"
"Sólo filtro personalizado puede ser borrado"
"Тільки фільтр користувача може бути видалений"

MFindFileTitle
l:
"Поиск файла"
"Find file"
"Hledat soubor"
"Nach Dateien suchen"
"Fájlkeresés"
"Znajdź plik"
"Encontrar archivo"
"Пошук файлу"

MFindFileMasks
"Одна или несколько &масок файлов:"
"A file &mask or several file masks:"
"Maska nebo masky souborů:"
"Datei&maske (mehrere getrennt mit Komma):"
"Fájlm&aszk(ok, vesszővel elválasztva):"
"&Maska pliku lub kilka masek oddzielonych przecinkami:"
"&Máscara de archivo o múltiples máscaras de archivos:"
"Одна або кілька &масок файлів:"

MFindFileText
"&Содержащих текст:"
"Con&taining text:"
"Obsahující te&xt:"
"Enthält &Text:"
"&Tartalmazza a szöveget:"
"Zawierający &tekst:"
"Conteniendo &texto:"
"&Той, що містить текст:"

MFindFileHex
"&Содержащих 16-ричный код:"
"Con&taining hex:"
"Obsahující &hex:"
"En&thält Hex (xx xx ...):"
"Tartalmazza a he&xát:"
"Zawierający wartość &szesnastkową:"
"Conteniendo Hexa:"
"&Вміст, що містить 16-річний код:"

MFindFileCodePage
"Используя кодо&вую страницу:"
"Using code pa&ge:"
upd:"Použít &znakovou sadu:"
upd:"Zeichenta&belle verwenden:"
"Kó&dlap:"
"Użyj tablicy znaków:"
"Usando tabla de caracteres:"
"Використовуючи кодо&ву сторінку:"

MFindFileCodePageBottom
"Space, Ins"
"Space, Ins"
"Space, Ins"
"Space, Ins"
"Space, Ins"
"Space, Ins"
"Espacio, Ins"
"Space, Ins"

MFindFileCase
"&Учитывать регистр"
"&Case sensitive"
"Roz&lišovat velikost písmen"
"Gr&oß-/Kleinschreibung"
"&Nagy/kisbetű érzékeny"
"&Uwzględnij wielkość liter"
"Sensible min/ma&yúsc."
"&Враховувати регістр"

MFindFileWholeWords
"Только &целые слова"
"&Whole words"
"&Celá slova"
"Nur &ganze Wörter"
"Csak egés&z szavak"
"Tylko &całe słowa"
"&Palabras completas"
"Тільки &цілі слова"

MFindFileAllCodePages
"Все кодовые страницы"
"All code pages"
upd:"Všechny znakové sady"
upd:"Alle Zeichentabellen"
"Minden kódlappal"
"Wszystkie zainstalowane"
"Todas las tablas de caracteres"
"Всі кодові сторінки"

MFindArchives
"Искать в а&рхивах"
"Search in arch&ives"
"Hledat v a&rchívech"
"In Arch&iven suchen"
"Keresés t&ömörítettekben"
"Szukaj w arc&hiwach"
"Buscar en archivos compr&imidos"
"Шукати в а&рхівах"

MFindFolders
"Искать п&апки"
"Search for f&olders"
"Hledat a&dresáře"
"Nach &Ordnern suchen"
"Keresés mapp&ákra"
"Szukaj &folderów"
"Buscar por direct&orios"
"Шукати т&еки"

MFindSymLinks
"Искать в символи&ческих ссылках"
"Search in symbolic lin&ks"
"Hledat v s&ymbolických lincích"
"In symbolischen Lin&ks suchen"
"Keresés sz&imbolikus linkekben"
"Szukaj w &linkach"
"Buscar en enlaces simbólicos"
"Шукати в символі&чних посиланнях"

MSearchForHex
"Искать 16-ричн&ый код"
"Search for &hex"
"Hledat &hex"
"Nach &Hex suchen"
"Keresés &hexákra"
"Szukaj wartości &szesnastkowej"
"Buscar por &hexa"
"Шукати 16-річн&ий код"

MSearchWhere
"Выберите &область поиска:"
"Select search &area:"
upd:"Zvolte oblast hledání:"
upd:"Suchbereich:"
"Keresés hatós&ugara:"
"Obszar wyszukiwania:"
"Seleccionar área de búsqueda:"
"Виберіть &область пошуку:"

MSearchAllDisks
"На всех несъёмных &дисках"
"In &all non-removable drives"
"Ve všech p&evných discích"
"Auf &allen festen Datenträger"
"Minden &fix meghajtón"
"Na dyskach &stałych"
"Buscar en todas las unidades no-removibles"
"На всіх незнімних &дисках"

MSearchAllButNetwork
"На всех &локальных дисках"
"In all &local drives"
"Ve všech &lokálních discích"
"Auf allen &lokalen Datenträgern"
"Minden hel&yi meghajtón"
"Na dyskach &lokalnych"
"Buscar en todas las unidades locales"
"На всіх &локальних дисках"

MSearchInPATH
"В PATH-катало&гах"
"In &PATH folders"
"V adresářích z &PATH"
"In &PATH-Ordnern"
"A &PATH mappáiban"
"W folderach zmiennej &PATH"
"En directorios de variable &PATH"
"У PATH-катало&гах"

MSearchFromRootOfDrive
"С кор&ня диска"
"From the &root of"
"V &kořeni"
"Ab Wu&rzelverz. von"
"Meghajtó &gyökerétől:"
"Od &korzenia"
"Buscar desde directorio &raíz de"
"З кор&іння диска"

MSearchFromRootFolder
"С кор&невой папки"
"From the &root folder"
"V kořeno&vém adresáři"
"Ab Wu&rzelverzeichnis"
"A &gyökérmappától"
"Od katalogu &głównego"
"Buscar desde la &raíz del directorio"
"З кор&еневої теки"

MSearchFromCurrent
"С &текущей папки"
"From the curre&nt folder"
"V tomto adresář&i"
"Ab dem aktuelle&n Ordner"
"Az akt&uális mappától"
"Od &bieżącego katalogu"
"Buscar desde directorio actual"
"З &поточної теки"

MSearchInCurrent
"Только в теку&щей папке"
"The current folder onl&y"
"P&ouze v tomto adresáři"
"Nur im aktue&llen Ordner"
"&Csak az aktuális mappában"
"&Tylko w bieżącym katalogu"
"Buscar en el directorio actua&l solamente"
"Тільки в пото&чній теці"

MSearchInSelected
"В &отмеченных папках"
"&Selected folders"
"Ve vy&braných adresářích"
"In au&sgewählten Ordner"
"A ki&jelölt mappákban"
"W &zaznaczonych katalogach"
"Buscar en directorios &seleccionados"
"У &відмічених теках"

MFindUseFilter
"Исполь&зовать фильтр"
"&Use filter"
"Použít f&iltr"
"Ben&utze Filter"
"Sz&űrővel"
"&Filtruj"
"&Usar filtro"
"Викорис&товувати фільтр"

MFindUsingFilter
"используя фильтр"
"using filter"
"používám filtr"
"mit Filter"
"szűrővel"
"używając filtra"
"usando filtro"
"використовуючи фільтр"

MFindFileFind
"&Искать"
"&Find"
"&Hledat"
"&Suchen"
"K&eres"
"Szuka&j"
"&Encontrar"
"&Шукати"

MFindFileDrive
"Дис&к"
"Dri&ve"
"D&isk"
"Lauf&werk"
"Meghajt&ó"
"&Dysk"
"Uni&dad"
"Дис&к"

MFindFileSetFilter
"&Фильтр"
"Filt&er"
"&Filtr"
"Filt&er"
"Szű&rő"
"&Filtr"
"Filtr&o"
"&Фільтр"

MFindFileAdvanced
"До&полнительно"
"Advance&d"
"Pokr&očilé"
"Er&weitert"
"Ha&ladó"
"&Zaawansowane"
"Avanza&da"
"До&датково"

MFindSearchingIn
"Поиск%ls в"
"Searching%ls in"
"Hledám%ls v"
"Suche%ls in"
"%ls keresése"
"Szukam%ls w"
"Buscando%ls en:"
"Пошук%ls в"

MFindNewSearch
"&Новый поиск"
"&New search"
"&Nové hledání"
"&Neue Suche"
"&Új keresés"
"&Od nowa..."
"&Nueva búsqueda"
"&Новий пошук"

MFindGoTo
"Пе&рейти"
"&Go to"
"&Jdi na"
"&Gehe zu"
"U&grás"
"&Idź do"
"&Ir a"
"Пе&рейти"

MFindView
"&Просм"
"&View"
"Zo&braz"
"&Betrachten"
"Meg&néz"
"&Podgląd"
"&Ver "
"&Прогл"

MFindEdit
"&Редакт"
"&Edit"
upd:"&Edit"
upd:"&Edit"
upd:"&Edit"
upd:"&Edit"
upd:"&Edit"
"&Редаг"

MFindPanel
"Пане&ль"
"&Panel"
"&Panel"
"&Panel"
"&Panel"
"&Do panelu"
"&Panel"
"Пане&ль"

MFindStop
"С&топ"
"&Stop"
"&Stop"
"&Stoppen"
"&Állj"
"&Stop"
"D&etener"
"С&топ"

MFindDone
l:
"Поиск закончен. Найдено файлов: %d, папок: %d"
"Search done. Found files: %d, folders: %d"
"Hledání ukončeno. Nalezeno %d soubor(ů) a %d adresář(ů)"
"Suche beendet. %d Datei(en) und %d Ordner gefunden."
"A keresés kész. %d fájlt és %d mappát találtam."
"Wyszukiwanie zakończone (znalazłem %d plików i %d folderów)"
"Búsqueda finalizada. Encontrados %d archivo(s) y %d directorio(s)"
"Пошук закінчено. Знайдено файли: %d, теки: %d"

MFindCancel
"Отм&ена"
"&Cancel"
"&Storno"
"Ab&bruch"
"&Mégsem"
"&Anuluj"
"&Cancelar"
"Від&міна"

MFindFound
l:
" Файлов: %d, папок: %d "
" Files: %d, folders: %d "
" Souborů: %d, adresářů: %d "
" Dateien: %d, Ordner: %d "
" Fájlt: %d, mappát: %d "
" Plików: %d, folderów: %d "
" Ficheros: %d, carpetas: %d "
" Файлів: %d, тек: %d "

MFindFileFolder
l:
"Папка"
"Folder"
"Adresář"
"Ordner"
"Mappa"
"Katalog"
" DIR  "
"Тека"

MFindFileSymLink
"Ссылка"
"Symlink"
"Link"
"Symlink"
"SzimLnk"
"LinkSym"
"SimbEnl"
"Посилання"

MFindFileJunction
"Связь"
"Junction"
"Křížení"
"Knoten"
"Csomópt"
"Dowiązania"
"Union"
"Зв'язок"

MFindFileAdvancedTitle
l:
"Дополнительные параметры поиска"
"Find file advanced options"
"Pokročilé nastavení vyhledávání souborů"
"Erweiterte Optionen"
"Fájlkeresés haladó beállításai"
"Zaawansowane opcje wyszukiwania"
"Opciones avanzada de búsqueda de archivo"
"Додаткові параметри пошуку"

MFindFileSearchFirst
"Проводить поиск в &первых:"
"Search only in the &first:"
"Hledat po&uze v prvních:"
"Nur &in den ersten x Bytes:"
"Keresés csak az első &x bájtban:"
"Szukaj wyłącznie w &pierwszych:"
"Buscar solamente en el &primer:"
"Проводити пошук у &перших:"

MFindFileSearchOutputFormat
"&Формат вывода:"
"&Output format:"
upd:"&Output format:"
upd:"&Output format:"
upd:"&Output format:"
upd:"&Output format:"
upd:"&Output format:"
"&Формат виведення:"

MFindAlternateStreams
"Обрабатывать &альтернативные потоки данных"
"Process &alternate data streams"
upd:"Process &alternate data streams"
upd:"Process &alternate data streams"
"&Alternatív adatsávok (stream) feldolgozása"
upd:"Process &alternate data streams"
"Procesar flujo alternativo de datos"
"Обробляти &альтернативні потоки даних"

MFindAlternateModeTypes
"&Типы колонок"
"Column &types"
"&Typ sloupců"
"Spalten&typen"
"Oszlop&típusok"
"&Typy kolumn"
"&Tipos de columna"
"&Типи колонок"

MFindAlternateModeWidths
"&Ширина колонок"
"Column &widths"
"Šíř&ka sloupců"
"Spalten&breiten"
"Oszlop&szélességek"
"&Szerokości kolumn"
"Anc&ho de columna"
"&Ширина колонок"

MFoldTreeSearch
l:
"Поиск:"
"Search:"
"Hledat:"
"Suchen:"
"Keresés:"
"Wyszukiwanie:"
"Buscar:"
"Пошук:"

MGetCodePageTitle
l:
"Кодовые страницы"
"Code pages"
upd:"Znakové sady:"
upd:"Tabellen"
"Kódlapok"
"Strony kodowe"
"Tablas"
"Кодові сторінки"

MGetCodePageSystem
"Системные"
"System"
upd:"System"
upd:"System"
"Rendszer"
upd:"System"
"Sistema"
"Системні"

MGetCodePageUnicode
"Юникод"
"Unicode"
upd:"Unicode"
upd:"Unicode"
"Unicode"
upd:"Unicode"
"Unicode"
"Юнікод"

MGetCodePageFavorites
"Избранные"
"Favorites"
upd:"Favorites"
upd:"Favorites"
"Kedvencek"
upd:"Favorites"
"Favoritos"
"Вибрані"

MGetCodePageOther
"Прочие"
"Other"
upd:"Other"
upd:"Other"
"Egyéb"
upd:"Other"
"Otro"
"Інші"

MGetCodePageBottomTitle
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"
"Strg-H, Entf, Einf, F4"
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"
"Ctrl-H, Del, Ins, F4"

MGetCodePageBottomShortTitle
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"
"Strg-H, Entf, F4"
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"
"Ctrl-H, Del, F4"

MGetCodePageEditCodePageName
"Изменить имя кодовой страницы"
"Edit code page name"
upd:"Edit code page name"
upd:"Edit code page name"
upd:"Edit code page name"
upd:"Edit code page name"
"Editar nombre de tabla (codepage)"
"Змінити ім'я кодової сторінки"

MGetCodePageResetCodePageName
"&Сбросить"
"&Reset"
upd:"&Reset"
upd:"&Reset"
upd:"&Reset"
upd:"&Reset"
"&Reiniciar"
"&Скинути"

MHighlightTitle
l:
"Раскраска файлов"
"Files highlighting"
"Zvýrazňování souborů"
"Farbmarkierungen"
"Fájlkiemelések, rendezési csoportok"
"Wyróżnianie plików"
"Resaltado de archivos"
"Розмальовка файлів"

MHighlightBottom
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"
"Ins,Del,F4,F5,Ctrl-Nahoru,Ctrl-Dolů"
"Einf,Entf,F4,F5,StrgUp,StrgDown"
"Ins,Del,F4,F5,Ctrl-Fel,Ctrl-Le"
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"
"Ins,Del,F4,F5,Ctrl-Up,Ctrl-Down"

MHighlightUpperSortGroup
"Верхняя группа сортировки"
"Upper sort group"
"Vzesupné řazení"
"Obere Sortiergruppen"
"Felsőbbrendű csoport"
"Górna grupa sortowania"
"Grupo de ordenamiento de arriba"
"Верхня група сортування"

MHighlightLowerSortGroup
"Нижняя группа сортировки"
"Lower sort group"
"Sestupné řazení"
"Untere Sortiergruppen"
"Alsóbbrendű csoport"
"Dolna grupa sortowania"
"Grupo de ordenamiento de abajo"
"Нижня група сортування"

MHighlightLastGroup
"Наименее приоритетная группа раскраски"
"Lowest priority highlighting group"
"Zvýraznění nejnižší prority"
"Farbmarkierungen mit niedrigster Priorität"
"Legalacsonyabb rendű csoport"
"Grupa wyróżniania o najniższym priorytecie"
"Resaltado de grupo con baja prioridad"
"Найменш пріоритетна група розмальовки"

MHighlightAskDel
"Вы хотите удалить раскраску для"
"Do you wish to delete highlighting for"
"Přejete si smazat zvýraznění pro"
"Wollen Sie Farbmarkierungen löschen für"
"Biztosan törli a kiemelést?"
"Czy chcesz usunąć wyróżnianie dla"
"Desea borrar resaltado para"
"Ви хочете видалити розмальовку для"

MHighlightWarning
"Будут потеряны все Ваши настройки"
"You will lose all changes"
"Všechny změny budou ztraceny"
"Sie verlieren jegliche Änderungen"
"Minden változtatás elvész"
"Wszystkie zmiany zostaną utracone"
"Usted perderá todos los cambios"
"Втрачені всі Ваші налаштування"

MHighlightAskRestore
"Вы хотите восстановить раскраску файлов по умолчанию?"
"Do you wish to restore default highlighting?"
"Přejete si obnovit výchozí nastavení?"
"Wollen Sie Standard-Farbmarkierungen wiederherstellen?"
"Visszaállítja az alapértelmezett kiemeléseket?"
"Czy przywrócić wyróżnianie domyślne?"
"Desea restablecer resaltado por defecto?"
"Ви хочете відновити забарвлення файлів за замовчуванням?"

MHighlightEditTitle
l:
"Редактирование раскраски файлов"
"Edit files highlighting"
"Upravit zvýrazňování souborů"
"Farbmarkierungen bearbeiten"
"Fájlkiemelés szerkesztése"
"Edytuj wyróżnianie plików"
"Editar resaltado de archivos"
"Редагування розмальовки файлів"

MHighlightMarkChar
"Оп&циональный символ пометки,"
"Optional markin&g character,"
"Volitelný &znak pro označení určených souborů,"
"Optionale Markierun&g mit Zeichen,"
"Megadható &jelölő karakter"
"Opcjonalny znak &wyróżniający zaznaczone pliki,"
"Ca&racter opcional para marcar archivos específicos"
"Оп&ціональний символ позначки,"

MHighlightTransparentMarkChar
"прозра&чный"
"tra&nsparent"
"průh&ledný"
"tra&nsparent"
"át&látszó"
"prze&zroczyste"
"tra&nsparente"
"проз&орий"

MHighlightColors
" Цвета файлов (\"чёрный на чёрном\" - цвет по умолчанию) "
" File name colors (\"black on black\" - default color) "
" Barva názvu souborů (\"černá na černé\" - výchozí barva) "
" Dateinamenfarben (\"Schwarz auf Schwarz\"=Standard) "
" Fájlnév színek (feketén fekete = alapértelmezett szín) "
" Kolory nazw plików (domyślny - \"czarny na czarnym\") "
" Colores de archivos (\"negro en negro\" - color por defecto) "
" Кольори файлів (\"чорний на чорному\" ​​- колір за замовчуванням) "

MHighlightFileName1
"&1. Обычное имя файла                "
"&1. Normal file name               "
"&1. Normální soubor            "
"&1. Normaler Dateiname             "
"&1. Normál fájlnév                  "
"&1. Nazwa pliku bez zaznaczenia    "
"&1. Normal  "
"&1. Звичайне ім'я файлу              "

MHighlightFileName2
"&3. Помеченное имя файла             "
"&3. Selected file name             "
"&3. Vybraný soubor             "
"&3. Markierter Dateiame            "
"&3. Kijelölt fájlnév                "
"&3. Zaznaczenie                    "
"&3. Seleccionado"
"&3. Позначене ім'я файлу             "

MHighlightFileName3
"&5. Имя файла под курсором           "
"&5. File name under cursor         "
"&5. Soubor pod kurzorem        "
"&5. Dateiname unter Cursor         "
"&5. Kurzor alatti fájlnév           "
"&5. Nazwa pliku pod kursorem       "
"&5. Bajo cursor "
"&5. Ім'я файлу під курсором          "

MHighlightFileName4
"&7. Помеченное под курсором имя файла"
"&7. File name selected under cursor"
"&7. Vybraný soubor pod kurzorem"
"&7. Dateiname markiert unter Cursor"
"&7. Kurzor alatti kijelölt fájlnév  "
"&7. Zaznaczony plik pod kursorem   "
"&7. Se&leccionado bajo cursor"
"&7. Позначене під курсором ім'я файлу"

MHighlightMarking1
"&2. Пометка"
"&2. Marking"
"&2. Označení"
"&2. Markierung"
"&2. Jelölő kar.:"
"&2. Zaznaczenie"
"&2. Marcado"
"&2. Помітка"

MHighlightMarking2
"&4. Пометка"
"&4. Marking"
"&4. Označení"
"&4. Markierung"
"&4. Jelölő kar.:"
"&4. Zaznaczenie"
"&4. Marcado"
"&4. Помітка"

MHighlightMarking3
"&6. Пометка"
"&6. Marking"
"&6. Označení"
"&6. Markierung"
"&6. Jelölő kar.:"
"&6. Zaznaczenie"
"&6. Marcado"
"&6. Помітка"

MHighlightMarking4
"&8. Пометка"
"&8. Marking"
"&8. Označení"
"&8. Markierung"
"&8. Jelölő kar.:"
"&8. Zaznaczenie"
"&8. Marcado"
"&8. Помітка"

MHighlightExample1
"║filename.ext │"
"║filename.ext │"
"║filename.ext │"
"║dateinam.erw │"
"║fájlneve.kit │"
"║nazwa.roz    │"
"║nombre.ext   │"
"║filename.ext │"

MHighlightExample2
"║ filename.ext│"
"║ filename.ext│"
"║ filename.ext│"
"║ dateinam.erw│"
"║ fájlneve.kit│"
"║ nazwa.roz   │"
"║ nombre.ext  │"
"║filename.ext │"

MHighlightContinueProcessing
"Продолжать &обработку"
"C&ontinue processing"
"Pokračovat ve zpracová&ní"
"Verarbeitung f&ortsetzen"
"Folyamatos f&eldolgozás"
"K&ontynuuj przetwarzanie"
"C&ontinuar procesando"
"Продовжувати &обробку"

MInfoTitle
l:
"Информация"
"Information"
"Informace"
"Informationen"
"Információk"
"Informacja"
"Información"
"Інформация"

MInfoCompName
"Имя компьютера"
"Computer name"
"Název počítače"
"Computername"
"Számítógép neve"
"Nazwa komputera"
"Nombre computadora"
"Им'я комп'ютера"

MInfoUserName
"Имя пользователя"
"User name"
"Jméno uživatele"
"Benutzername"
"Felhasználói név"
"Nazwa użytkownika"
"Nombre usuario"
"Им'я користувача"

MInfoRemovable
"Сменный"
"Removable"
"Vyměnitelný"
"Austauschbares"
"Kivehető"
"Wyjmowalny"
"Removible"
"Змінний"

MInfoFixed
"Жёсткий"
"Fixed"
"Pevný"
"Lokales"
"Fix"
"Stały"
"Rígido"
"Жорсткий"

MInfoNetwork
"Сетевой"
"Network"
"Síťový"
"Netzwerk"
"Hálózati"
"Sieciowy"
"Red"
"Мережевий"

MInfoCDROM
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"

MInfoCD_RW
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"

MInfoCD_RWDVD
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"

MInfoDVD_ROM
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"

MInfoDVD_RW
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"

MInfoDVD_RAM
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"

MInfoRAM
"RAM"
"RAM"
"RAM"
"RAM"
"RAM"
"RAM"
"RAM"
"RAM"

MInfoSUBST
"SUBST"
"Subst"
"SUBST"
"Subst"
"Virtuális"
"Subst"
"Subst"
"SUBST"

MInfoVirtual
"Виртуальный"
"Virtual"
upd:"Virtual"
upd:"Virtual"
upd:"Virtual"
upd:"Virtual"
upd:"Virtual"
"Віртуальний"

MInfoDisk
"диск"
"disk"
"disk"
"Laufwerk"
"lemez"
"dysk"
"disco"
"диск"

MInfoDiskTotal
"Всего байтов"
"Total bytes"
"Celkem bytů"
"Bytes gesamt"
"Összes bájt"
"Razem bajtów"
"Total de bytes"
"Всього байтів"

MInfoDiskFree
"Свободных байтов"
"Free bytes"
"Volných bytů"
"Bytes frei"
"Szabad bájt"
"Wolnych bajtów"
"Bytes libres"
"Вільних байтів"

MInfoDiskLabel
"Метка тома"
"Volume label"
"Popisek disku"
"Laufwerksbezeichnung"
"Kötet címke"
"Etykieta woluminu"
"Etiqueta de volumen"
"Мітка тома"

MInfoDiskNumber
"Серийный номер"
"Serial number"
"Sériové číslo"
"Seriennummer"
"Sorozatszám"
"Numer seryjny"
"Número de serie"
"Серійний номер"

MInfoMemory
" Память "
" Memory "
" Paměť "
" Speicher "
" Memória "
" Pamięć "
" Memoria "
" Пам'ять "

MInfoMemoryLoad
"Загрузка памяти"
"Memory load"
"Zatížení paměti"
"Speicherverbrauch"
"Használt memória"
"Użycie pamięci"
"Carga en Memoria"
"Завантаження пам'яті"

MInfoMemoryInstalled
"Установлено памяти"
"Installed memory"
upd:"Installed memory"
upd:"Installed memory"
upd:"Installed memory"
upd:"Installed memory"
"Memoria instalada"
"Встановлено пам'яті"

MInfoMemoryTotal
"Всего памяти"
"Total memory"
"Celková paměť"
"Speicher gesamt"
"Összes memória"
"Całkowita pamięć"
"Total memoria"
"Всього пам'яті"

MInfoMemoryFree
"Свободно памяти"
"Free memory"
"Volná paměť"
"Speicher frei"
"Szabad memória"
"Wolna pamięć"
"Memoria libre"
"Вільно пам'яті"

MInfoSharedMemory
"Разделяемая память"
"Shared memory"
upd:"Shared RAM"
upd:"Shared RAM"
upd:"Shared RAM"
upd:"Shared RAM"
upd:"Shared RAM"
"Спільна пам'ять"

MInfoBufferMemory
"Буферизованная память"
"Buffer memory"
upd:"Buffer RAM"
upd:"Buffer RAM"
upd:"Buffer RAM"
upd:"Buffer RAM"
upd:"Buffer RAM"
"Буферизована пам'ять"

MInfoPageFileTotal
"Всего файла подкачки"
"Total paging file"
upd:"Total paging file"
upd:"Total paging file"
upd:"Total paging file"
upd:"Total paging file"
"Archivo de paginación total"
"Всього файлу підкачки"

MInfoPageFileFree
"Свободно файла подкачки"
"Free paging file"
upd:"Free paging file"
upd:"Free paging file"
upd:"Free paging file"
upd:"Free paging file"
"Archivo de paginación libre"
"Вільно файлу підкачки"

MInfoDizAbsent
"Файл описания папки отсутствует"
"Folder description file is absent"
"Soubor s popisem adresáře chybí"
"Keine Datei mit Ordnerbeschreibungen vorhanden."
"Mappa megjegyzésfájl nincs"
"Plik opisu katalogu nie istnieje"
"archivo descripción del directorio está ausente"
"Файл опису теки відсутній"

MErrorInvalidFunction
l:
"Некорректная функция"
"Incorrect function"
"Nesprávná funkce"
"Ungültige Funktion"
"Helytelen funkció"
"Niewłaściwa funkcja"
"Función incorrecta"
"Некоректна функція"

MErrorBadCommand
"Команда не распознана"
"Command not recognized"
"Příkaz nebyl rozpoznán"
"Unbekannter Befehl"
"Ismeretlen parancs"
"Nieznane polecenie"
"Comando no reconocido"
"Команда не розпізнана"

MErrorFileNotFound
"Файл не найден"
"File not found"
"Soubor nenalezen"
"Datei nicht gefunden"
"A fájl vagy mappa nem található"
"Nie odnaleziono pliku"
"archivo no encontrado"
"Файл не знайдено"

MErrorPathNotFound
"Путь не найден"
"Path not found"
"Cesta nenalezena"
"Pfad nicht gefunden"
"Az elérési út nem található"
"Nie odnaleziono ścieżki"
"Ruta no encontrada"
"Шлях не знайдено"

MErrorTooManyOpenFiles
"Слишком много открытых файлов"
"Too many open files"
"Příliš mnoho otevřených souborů"
"Zu viele geöffnete Dateien"
"Túl sok nyitott fájl"
"Zbyt wiele otwartych plików"
"Demasiados archivos abiertos"
"Занадто багато відкритих файлів"

MErrorAccessDenied
"Доступ запрещён"
"Access denied"
"Přístup odepřen"
"Zugriff verweigert"
"Hozzáférés megtagadva"
"Dostęp zabroniony"
"Acceso denegado"
"Доступ заборонено"

MErrorNotEnoughMemory
"Недостаточно памяти"
"Not enough memory"
"Nedostatek paměti"
"Nicht genügend Speicher"
"Nincs elég memória"
"Za mało pamięci"
"No hay memoria libre"
"Недостатньо пам'яті"

MErrorDiskRO
"Попытка записи на защищённый от записи диск"
"Cannot write to write protected disk"
"Nelze zapisovat na disk chráněný proti zápisu"
"Der Datenträger ist schreibgeschützt"
"Írásvédett lemezre nem lehet írni"
"Nie mogę zapisać na zabezpieczony dysk"
"No se puede escribir a disco protegido contra escritura"
"Спроба запису на захищений від запису диск"

MErrorDeviceNotReady
"Устройство не готово"
"The device is not ready"
"Zařízení není připraveno"
"Das Gerät ist nicht bereit"
"Az eszköz nem kész"
"Urządzenie nie jest gotowe"
"El dispositivo no está listo"
"Пристрій не готовий"

MErrorCannotAccessDisk
"Доступ к диску невозможен"
"Disk cannot be accessed"
"Na disk nelze přistoupit"
"Auf Datenträger kann nicht zugegriffen werden"
"A lemez nem érhető el"
"Brak dostępu do dysku"
"Disco no puede ser accedido"
"Доступ до диска неможливий"

MErrorSectorNotFound
"Сектор не найден"
"Sector not found"
"Sektor nenalezen"
"Sektor nicht gefunden"
"Szektor nem található"
"Nie odnaleziono sektora"
"Sector no encontrado"
"Сектор не знайдено"

MErrorOutOfPaper
"В принтере нет бумаги"
"The printer is out of paper"
"V tiskárně došel papír"
"Der Drucker hat kein Papier mehr"
"A nyomtatóban nincs papír"
"Brak papieru w drukarce"
"No hay papel en la impresora"
"У принтері немає паперу"

MErrorWrite
"Ошибка записи"
"Write fault error"
"Chyba zápisu"
"Fehler beim Schreibzugriff"
"Írási hiba"
"Błąd zapisu"
"Falla de escritura"
"Помилка запису"

MErrorRead
"Ошибка чтения"
"Read fault error"
"Chyba čtení"
"Fehler beim Lesezugriff"
"Olvasási hiba"
"Błąd odczytu"
"Falla de lectura"
"Помилка читання"

MErrorDeviceGeneral
"Общая ошибка устройства"
"Device general failure"
"Obecná chyba zařízení"
"Ein Gerätefehler ist aufgetreten"
"Eszköz általános hiba"
"Ogólny błąd urządzenia"
"Falla general en dispositivo"
"Загальна помилка пристрою"

MErrorFileSharing
"Нарушение совместного доступа к файлу"
"File sharing violation"
"Narušeno sdílení souborů"
"Zugriffsverletzung"
"Fájlmegosztási hiba"
"Naruszenie zasad współużytkowania pliku"
"Violación de archivo compartido"
"Порушення спільного доступу до файлу"

MErrorNetworkPathNotFound
"Сетевой путь не найден"
"The network path was not found"
"Síťová cesta nebyla nalezena"
"Der Netzwerkpfad wurde nicht gefunden"
"Hálózati útvonal nem található"
"Nie odnaleziono ścieżki sieciowej"
"La ruta de red no ha sido encontrada"
"Мережевий шлях не знайдено"

MErrorNetworkBusy
"Сеть занята"
"The network is busy"
"Síť je zaneprázdněna"
"Das Netzwerk ist beschäftigt"
"A hálózat zsúfolt"
"Sieć jest zajęta"
"La red está ocupada"
"Мережа зайнята"

MErrorNetworkAccessDenied
"Сетевой доступ запрещён"
"Network access is denied"
"Přístup na síť zakázán"
"Netzwerkzugriff wurde verweigert"
"Hálózati hozzáférés megtagadva"
"Dostęp do sieci zabroniony"
"Acceso a red es denegado"
"Мережевий доступ заборонено"

MErrorNetworkWrite
"Ошибка записи в сети"
"A write fault occurred on the network"
"Na síti došlo k chybě v zápisu"
"Fehler beim Schreibzugriff auf das Netzwerk"
"Írási hiba a hálózaton"
"Wystąpił błąd zapisu w sieci"
"Falla de escritura en la red"
"Помилка запису в мережі"

MErrorDiskLocked
"Диск используется или заблокирован другим процессом"
"The disk is in use or locked by another process"
"Disk je používán nebo uzamčen jiným procesem"
"Datenträger wird verwendet oder ist durch einen anderen Prozess gesperrt"
"A lemezt használja vagy zárolja egy folyamat"
"Dysk jest w użyciu lub zablokowany przez inny proces"
"El disco está en uso o bloqueado por otro proceso"
"Диск використовується або заблокований іншим процесом"

MErrorFileExists
"Файл или папка уже существует"
"File or folder already exists"
"Soubor nebo adresář již existuje"
"Die Datei oder der Ordner existiert bereits."
"A fájl vagy mappa már létezik"
"Plik lub katalog już istnieje"
"archivo o directorio ya existe"
"Файл або тека вже існує"

MErrorInvalidName
"Указанное имя неверно"
"The specified name is invalid"
"Zadaný název je neplatný"
"Der angegebene Name ist ungültig"
"A megadott név érvénytelen"
"Podana nazwa jest niewłaściwa"
"El nombre especificado es inválido"
"Вказане ім'я неправильне"

MErrorInsufficientDiskSpace
"Нет места на диске"
"Insufficient disk space"
"Nedostatek místa na disku"
"Unzureichend Speicherplatz am Datenträger"
"Nincs elég hely a lemezen"
"Za mało miejsca na dysku"
"Insuficiente espacio de disco"
"Немає місця на диску"

MErrorFolderNotEmpty
"Папка не пустая"
"The folder is not empty"
"Adresář není prázdný"
"Der Ordner ist nicht leer"
"A mappa nem üres"
"Katalog nie jest pusty"
"El directorio no está vacío"
"Тека не порожня"

MErrorIncorrectUserName
"Неверное имя пользователя"
"Incorrect user name"
"Neplatné jméno uživatele"
"Ungültiger Benutzername"
"Érvénytelen felhasználói név"
"Niewłaściwa nazwa użytkownika"
"Nombre de usuario incorrecto"
"Неправильне ім'я користувача"

MErrorIncorrectPassword
"Неверный пароль"
"Incorrect password"
"Neplatné heslo"
"Ungültiges Passwort"
"Érvénytelen jelszó"
"Niewłaściwe hasło"
"Clave incorrecta"
"Невірний пароль"

MErrorLoginFailure
"Ошибка регистрации"
"Login failure"
"Přihlášení selhalo"
"Login fehlgeschlagen"
"Sikertelen bejelentkezés"
"Logowanie nie powiodło się"
"Falla en conexión"
"Помилка реєстрації"

MErrorConnectionAborted
"Соединение разорвано"
"Connection aborted"
"Spojení přerušeno"
"Verbindung abgebrochen"
"Kapcsolat bontva"
"Połączenie zerwane"
"Conexión abortada"
"З'єднання розірвано"

MErrorCancelled
"Операция отменена"
"Operation cancelled"
"Operace stornována"
"Vorgang abgebrochen"
"A művelet megszakítva"
"Operacja przerwana"
"Operación cancelada"
"Операцію скасовано"

MErrorNetAbsent
"Сеть отсутствует"
"No network present"
"Síť není k dispozici"
"Kein Netzwerk verfügbar"
"Nincs hálózat"
"Brak sieci"
"No hay red presente"
"Мережа відсутня"

MErrorNetDeviceInUse
"Устройство используется и не может быть отсоединено"
"Device is in use and cannot be disconnected"
"Zařízení se používá a nemůže být odpojeno"
"Gerät wird gerade verwendet oder kann nicht getrennt werden"
"Az eszköz használatban van, nem választható le"
"Urządzenie jest w użyciu i nie można go odłączyć"
"Dispositivo está en uso y no puede ser desconectado"
"Пристрій використовується і не може бути від'єднаний"

MErrorNetOpenFiles
"На сетевом диске есть открытые файлы"
"This network connection has open files"
"Přes toto síťové spojení jsou otevřeny soubory"
"Diese Netzwerkverbindung hat geöffnete Dateien"
"A hálózaton nyitott fájlok vannak"
"To połączenie sieciowe posiada otwarte pliki"
"Esta conexión de red tiene archivos abiertos"
"На мережному диску є відкриті файли"

MErrorAlreadyAssigned
"Имя локального устройства уже использовано"
"The local device name is already in use"
"Název lokálního zařízení je již používán"
"Der lokale Gerätename wird bereits verwendet"
"A helyi eszköznév már foglalt"
"Nazwa urządzenia lokalnego jest już używana"
"El nombre del dispositivo local ya está en uso"
"Ім'я локального пристрою вже використано"

MErrorAlreadyRemebered
"Имя локального устройства уже находится в профиле пользователя"
"The local device is already in the user profile"
"Lokální zařízení je již v uživatelově profilu"
"Der lokale Datenträger ist bereits Teil des Benutzerprofils"
"A helyi eszköz már a felhasználói profilban van"
"Lokalne urządzenie znajduje się już w profilu użytkownika"
"El dispositivo local ya está en el perfil de usuario"
"Ім'я локального пристрою вже знаходиться у профілі користувача"

MErrorNotLoggedOn
"Пользователь не зарегистрирован в сети"
"User has not logged on to the network"
"Uživatel nebyl do sítě přihlášen"
"Benutzer hat sich nicht am Netzwerk angemeldet"
"A felhasználó nincs a hálózaton"
"Użytkownik nie jest zalogowany do sieci"
"Usuario no está conectado a la red"
"Користувач не зареєстрований у мережі"

MErrorInvalidPassword
"Неверный пароль пользователя"
"The user password is invalid"
"Uživatelovo heslo není správné"
"Das Benutzerpasswort ist ungültig"
"Érvénytelen felhasználói jelszó"
"Hasło użytkownika jest niewłaściwe"
"La clave de usuario es inválida"
"Невірний пароль користувача"

MErrorNoRecoveryPolicy
"Для этой системы отсутствует политика надёжного восстановления шифрования"
"There is no valid encryption recovery policy configured for this system"
"V tomto systému není nastaveno žádné platné pravidlo pro dešifrování"
"Auf diesem System ist keine gültige Richtlinie zum Wiederherstellen der Verschlüsselung konfiguriert."
"Nincs érvényes titkosítást feloldó szabály a házirendben"
"Polityka odzyskiwania szyfrowania nie jest skonfigurowana"
"No hay política de recuperación de encriptación válida en este sistema"
"Для цієї системи відсутня політика надійного відновлення шифрування"

MErrorEncryptionFailed
"Ошибка при попытке шифрования файла"
"The specified file could not be encrypted"
"Zadaný soubor nemohl být zašifrován"
"Die angegebene Datei konnte nicht verschlüsselt werden"
"A megadott fájl nem titkosítható"
"Nie udało się zaszyfrować pliku"
"El archivo especificado no puede ser encriptado"
"Помилка при спробі шифрування файлу"

MErrorDecryptionFailed
"Ошибка при попытке расшифровки файла"
"The specified file could not be decrypted"
"Zadaný soubor nemohl být dešifrován"
"Die angegebene Datei konnte nicht entschlüsselt werden"
"A megadott fájl titkosítása nem oldható fel"
"Nie udało się odszyfrować pliku"
"El archivo especificado no puede ser desencriptado"
"Помилка при спробі розшифровувати файл"

MErrorFileNotEncrypted
"Указанный файл не зашифрован"
"The specified file is not encrypted"
"Zadaný soubor není zašifrován"
"Die angegebene Datei ist nicht verschlüsselt"
"A megadott fájl nem titkosított"
"Plik nie jest zaszyfrowany"
"El archivo especificado no está encriptado"
"Вказаний файл не зашифрований"

MErrorNoAssociation
"Указанному файлу не сопоставлено ни одно приложение для выполнения данной операции"
"No application is associated with the specified file for this operation"
"K zadanému souboru není asociována žádná aplikace pro tuto operaci"
"Diesem Dateityp und dieser Aktion ist kein Programm zugewiesen."
"A fájlhoz nincs társítva program"
"Z tą operacją dla pliku nie jest skojarzona żadna aplikacja"
"No hay aplicación asociada para esta operación con el archivo especificado"
"Вказаному файлу не зіставлено жодну програму для виконання цієї операції"

MCannotExecute
l:
"Ошибка выполнения"
"Cannot execute"
"Nelze provést"
"Fehler beim Ausführen von"
"Nem végrehajtható:"
"Nie mogę wykonać"
"No se puede ejecutar"
"Помилка виконання"

MScanningFolder
"Просмотр папки"
"Scanning the folder"
"Prohledávám adresář"
"Scanne den Ordner"
"Mappák olvasása..."
"Przeszukuję katalog"
"Explorando el directorio"
"Перегляд теки"

MMakeFolderTitle
l:
"Создание папки"
"Make folder"
"Vytvoření adresáře"
"Ordner erstellen"
"Új mappa létrehozása"
"Utwórz katalog"
"Crear directorio"
"Створення теки"

MCreateFolder
"Создать п&апку"
"Create the &folder"
"Vytvořit &adresář"
"Diesen &Ordner erstellen:"
"Mappa &neve:"
"Nazwa katalogu"
"Nombre del directorio"
"Створити т&еку"

MMultiMakeDir
"Обрабатыват&ь несколько имён папок"
"Process &multiple names"
"Zpracovat &více názvů"
"&Mehrere Namen verarbeiten (getrennt durch Semikolon)"
"Töb&b név feldolgozása"
"Przetwarzaj &wiele nazw"
"Procesar &múltiples nombres"
"Оброблят&и кілька імен тек"

MIncorrectDirList
"Неправильный список папок"
"Incorrect folders list"
"Neplatný seznam adresářů"
"Fehlerhafte Ordnerliste"
"Hibás mappalista"
"Błędna lista folderów"
"Listado de directorios incorrecto"
"Неправильний список тек"

MCannotCreateFolder
"Ошибка создания папки"
"Cannot create the folder"
"Adresář nelze vytvořit"
"Konnte den Ordner nicht erstellen"
"A mappa nem hozható létre"
"Nie mogę utworzyć katalogu"
"No se puede crear el directorio"
"Помилка створення теки"

MMenuBriefView
l:
"&Краткий                  LCtrl-1"
"&Brief              LCtrl-1"
"&Stručný                  LCtrl-1"
"&Kurz                 LStrg-1"
"&Rövid              BalCtrl-1"
"&Skrótowy             LCtrl-1"
"&Breve                 LCtrl-1"
"&Короткий                 LCtrl-1"

MMenuMediumView
"&Средний                  LCtrl-2"
"&Medium             LCtrl-2"
"S&třední                  LCtrl-2"
"&Mittel               LStrg-2"
"&Közepes            BalCtrl-2"
"Ś&redni               LCtrl-2"
"&Medio                 LCtrl-2"
"&Середній                 LCtrl-2"

MMenuFullView
"&Полный                   LCtrl-3"
"&Full               LCtrl-3"
"&Plný                     LCtrl-3"
"&Voll                 LStrg-3"
"&Teljes             BalCtrl-3"
"&Pełny                LCtrl-3"
"&Completo              LCtrl-3"
"&Повний                   LCtrl-3"

MMenuWideView
"&Широкий                  LCtrl-4"
"&Wide               LCtrl-4"
"Š&iroký                   LCtrl-4"
"B&reitformat          LStrg-4"
"&Széles             BalCtrl-4"
"S&zeroki              LCtrl-4"
"&Amplio                LCtrl-4"
"&Широкий                  LCtrl-4"

MMenuDetailedView
"&Детальный                LCtrl-5"
"Detai&led           LCtrl-5"
"Detai&lní                 LCtrl-5"
"Detai&lliert          LStrg-5"
"Rész&letes          BalCtrl-5"
"Ze sz&czegółami       LCtrl-5"
"De&tallado             LCtrl-5"
"&Детальний                LCtrl-5"

MMenuDizView
"&Описания                 LCtrl-6"
"&Descriptions       LCtrl-6"
"P&opisky                  LCtrl-6"
"&Beschreibungen       LStrg-6"
"Fájl&megjegyzések   BalCtrl-6"
"&Opisy                LCtrl-6"
"&Descripción           LCtrl-6"
"&Описи                    LCtrl-6"

MMenuLongDizView
"Д&линные описания         LCtrl-7"
"Lon&g descriptions  LCtrl-7"
"&Dlouhé popisky           LCtrl-7"
"Lan&ge Beschreibungen LStrg-7"
"&Hosszú megjegyzés  BalCtrl-7"
"&Długie opisy         LCtrl-7"
"Descripción lar&ga     LCtrl-7"
"Д&овгі описи              LCtrl-7"

MMenuOwnersView
"Вл&адельцы файлов         LCtrl-8"
"File own&ers        LCtrl-8"
"Vlastník so&uboru         LCtrl-8"
"B&esitzer             LStrg-8"
"Fájl tula&jdonos    BalCtrl-8"
"&Właściciele          LCtrl-8"
"Du&eños de archivos    LCtrl-8"
"Власники файлів           LCtrl-8"

MMenuLinksView
"Свя&зи файлов             LCtrl-9"
"File lin&ks         LCtrl-9"
"Souborové lin&ky          LCtrl-9"
"Dateilin&ks           LStrg-9"
"Fájl li&nkek        BalCtrl-9"
"Dowiąza&nia           LCtrl-9"
"En&laces               LCtrl-9"
"Зв'язки файлів            LCtrl-9"

MMenuAlternativeView
"Аль&тернативный полный    LCtrl-0"
"&Alternative full   LCtrl-0"
"&Alternativní plný        LCtrl-0"
"&Alternativ voll      LStrg-0"
"&Alternatív teljes  BalCtrl-0"
"&Alternatywny         LCtrl-0"
"Alternativo com&pleto  LCtrl-0"
"Аль&тернативний повний    LCtrl-0"

MMenuInfoPanel
l:
"Панель ин&формации        Ctrl-L"
"&Info panel         Ctrl-L"
"Panel In&fo               Ctrl-L"
"&Infopanel            Strg-L"
"&Info panel         Ctrl-L"
"Panel informacy&jny   Ctrl-L"
"Panel &información     Ctrl-L"
"Панель ін&формациї        Ctrl-L"

MMenuTreePanel
"Де&рево папок             Ctrl-T"
"&Tree panel         Ctrl-T"
"Panel St&rom              Ctrl-T"
"Baumansich&t          Strg-T"
"&Fastruktúra        Ctrl-T"
"Drz&ewo               Ctrl-T"
"Panel árbol           Ctrl-T"
"Де&рево тек               Ctrl-T"

MMenuQuickView
"Быстры&й просмотр         Ctrl-Q"
"Quick &view         Ctrl-Q"
"Z&běžné zobrazení         Ctrl-Q"
"Sc&hnellansicht       Strg-Q"
"&Gyorsnézet         Ctrl-Q"
"Sz&ybki podgląd       Ctrl-Q"
"&Vista rápida          Ctrl-Q"
"Швидки&й перегляд         Ctrl-Q"

MMenuSortModes
"Режим&ы сортировки        Ctrl-F12"
"&Sort modes         Ctrl-F12"
"Módy řaze&ní              Ctrl-F12"
"&Sortiermodi          Strg-F12"
"R&endezési elv      Ctrl-F12"
"Try&by sortowania     Ctrl-F12"
"&Ordenar por...        Ctrl-F12"
"Режим&и сортування        Ctrl-F12"

MMenuLongNames
"Показывать длинные &имена Ctrl-N"
"Show long &names    Ctrl-N"
"Zobrazit dlouhé názv&y    Ctrl-N"
"Lange Datei&namen     Strg-N"
"H&osszú fájlnevek   Ctrl-N"
"Po&każ długie nazwy   Ctrl-N"
"Ver &nombres largos    Ctrl-N"
"Показувати довгі &імена   Ctrl-N"

MMenuTogglePanel
"Панель &Вкл/Выкл          Ctrl-F1"
"Panel &On/Off       Ctrl-F1"
"Panel &Zap/Vyp            Ctrl-F1"
"&Panel ein/aus        Strg-F1"
"&Panel be/ki        Ctrl-F1"
"Włącz/Wyłącz pane&l   Ctrl-F1"
"Panel &Si/No           Ctrl-F1"
"Панель &Ввмк/Вимк         Ctrl-F1"

MMenuReread
"П&еречитать               Ctrl-R"
"&Re-read            Ctrl-R"
"Obno&vit                  Ctrl-R"
"Aktualisie&ren        Strg-R"
"Friss&ítés          Ctrl-R"
"Odśw&ież              Ctrl-R"
"&Releer                Ctrl-R"
"Перечитати                Ctrl-R"

MMenuChangeDrive
"С&менить диск             Alt-F1"
"&Change drive       Alt-F1"
"Z&měnit jednotku          Alt-F1"
"Laufwerk we&chseln    Alt-F1"
"Meghajtó&váltás     Alt-F1"
"Z&mień napęd          Alt-F1"
"Cambiar &unidad        Alt-F1"
"З&мінити диск             Alt-F1"

MMenuView
l:
"&Просмотр              F3"
"&View               F3"
"&Zobrazit                   F3"
"&Betrachten           F3"
"&Megnéz               F3"
"&Podgląd                   F3"
"&Ver                   F3"
"&Перегляд              F3"

MMenuEdit
"&Редактирование        F4"
"&Edit               F4"
"&Editovat                   F4"
"B&earbeiten           F4"
"&Szerkeszt            F4"
"&Edytuj                    F4"
"&Editar                F4"
"&Редагування           F4"

MMenuCopy
"&Копирование           F5"
"&Copy               F5"
"&Kopírovat                  F5"
"&Kopieren             F5"
"Más&ol                F5"
"&Kopiuj                    F5"
"&Copiar                F5"
"&Копіювання            F5"

MMenuMove
"П&еренос               F6"
"&Rename or move     F6"
"&Přejmenovat/Přesunout      F6"
"Ve&rschieben/Umben.   F6"
"Át&nevez-Mozgat       F6"
"&Zmień nazwę lub przenieś  F6"
"&Renombrar o mover     F6"
"П&еренесення           F6"

MMenuCreateFolder
"&Создание папки        F7"
"&Make folder        F7"
"&Vytvořit adresář           F7"
"&Ordner erstellen     F7"
"Ú&j mappa             F7"
"U&twórz katalog            F7"
"Crear &directorio      F7"
"&Створення теки        F7"

MMenuDelete
"&Удаление              F8"
"&Delete             F8"
"&Smazat                     F8"
"&Löschen              F8"
"&Töröl                F8"
"&Usuń                      F8"
"&Borrar                F8"
"&Видалення             F8"

MMenuWipe
"Уни&чтожение           Alt-Del"
"&Wipe               Alt-Del"
"&Vymazat                    Alt-Del"
"&Sicher löschen       Alt-Entf"
"&Kisöpör              Alt-Del"
"&Wymaż                     Alt-Del"
"&Eliminar              Alt-Del"
"Зни&щення              Alt-Del"

MMenuAdd
"&Архивировать          Shift-F1"
"Add &to archive     Shift-F1"
"Přidat do &archívu          Shift-F1"
"Zu Archiv &hinzuf.    Umsch-F1"
"Tömörhöz ho&zzáad     Shift-F1"
"&Dodaj do archiwum         Shift-F1"
"Agregar a arc&hivo     Shift-F1"
"&Архівувати            Shift-F1"

MMenuExtract
"Распако&вать           Shift-F2"
"E&xtract files      Shift-F2"
"&Rozbalit soubory           Shift-F2"
"Archiv e&xtrahieren   Umsch-F2"
"Tömörből ki&bont      Shift-F2"
"&Rozpakuj archiwum         Shift-F2"
"E&xtraer archivos      Shift-F2"
"Розпаку&вати           Shift-F2"

MMenuArchiveCommands
"Архивн&ые команды      Shift-F3"
"Arc&hive commands   Shift-F3"
"Příkazy arc&hívu            Shift-F3"
"Arc&hivbefehle        Umsch-F3"
"Tömörítő &parancsok   Shift-F3"
"Po&lecenie archiwizera     Shift-F3"
"Co&mandos archivo      Shift-F3"
"Архівн&і команди       Shift-F3"

MMenuAttributes
"А&трибуты файлов       Ctrl-A"
"File &attributes    Ctrl-A"
"A&tributy souboru           Ctrl-A"
"Datei&attribute       Strg-A"
"Fájl &attribútumok    Ctrl-A"
"&Atrybuty pliku            Ctrl-A"
"Cambiar &atributos     Ctrl-A"
"А&трибути файлів       Ctrl-A"

MMenuApplyCommand
"Применить коман&ду     Ctrl-G"
"A&pply command      Ctrl-G"
"Ap&likovat příkaz           Ctrl-G"
"Befehl an&wenden      Strg-G"
"Parancs &végrehajtása Ctrl-G"
"Zastosuj pole&cenie        Ctrl-G"
"A&plicar comando       Ctrl-G"
"Примінити коман&ду     Ctrl-G"

MMenuDescribe
"&Описание файлов       Ctrl-Z"
"Descri&be files     Ctrl-Z"
"Přidat popisek sou&borům    Ctrl-Z"
"Beschrei&bung ändern  Strg-Z"
"Fájlmegje&gyzés       Ctrl-Z"
"&Opisz pliki               Ctrl-Z"
"Describir &archivo     Ctrl-Z"
"&Опис файлів           Ctrl-Z"

MMenuSelectGroup
"Пометить &группу       Gray +"
"Select &group       Gray +"
"Oz&načit skupinu            Num +"
"&Gruppe auswählen     Num +"
"Csoport k&ijelölése   Szürke +"
"Zaznacz &grupę             Szary +"
"Seleccionar &grupo     Gray +"
"Позначити &групу       Gray +"

MMenuUnselectGroup
"С&нять пометку         Gray -"
"U&nselect group     Gray -"
"O&dznačit skupinu           Num -"
"G&ruppe abwählen      Num -"
"Jelölést l&evesz      Szürke -"
"Odz&nacz grupę             Szary -"
"Deseleccio&nar grupo   Gray -"
"З&няти позначку         Gray -"

MMenuInvertSelection
"&Инверсия пометки      Gray *"
"&Invert selection   Gray *"
"&Invertovat výběr           Num *"
"Auswah&l umkehren     Num *"
"Jelölést meg&fordít   Szürke *"
"Od&wróć zaznaczenie        Szary *"
"&Invertir selección    Gray *"
"&Інверсія позначки     Gray *"

MMenuRestoreSelection
"Восстановить по&метку  Ctrl-M"
"Re&store selection  Ctrl-M"
"&Obnovit výběr              Ctrl-M"
"Auswahl wiederher&st. Strg-M"
"Jel&ölést visszatesz  Ctrl-M"
"Odtwórz zaznaczen&ie       Ctrl-M"
"Re&staurar selec.      Ctrl-M"
"Відновити по&значку    Ctrl-M"

MMenuFindFile
l:
"&Поиск файла              Alt-F7"
"&Find file           Alt-F7"
"H&ledat soubor                  Alt-F7"
"Dateien &finden       Alt-F7"
"Fájl&keresés         Alt-F7"
"&Znajdź plik               Alt-F7"
"Buscar &archivos       Alt-F7"
"&Пошук файла              Alt-F7"

MMenuHistory
"&История команд           Alt-F8"
"&History             Alt-F8"
"&Historie                       Alt-F8"
"&Historie             Alt-F8"
"Parancs &előzmények  Alt-F8"
"&Historia                  Alt-F8"
"&Historial             Alt-F8"
"&Історія команд           Alt-F8"

MMenuVideoMode
"Видео&режим               Alt-F9"
"&Video mode          Alt-F9"
"&Video mód                      Alt-F9"
"Ansicht<->&Vollbild   Alt-F9"
"&Video mód           Alt-F9"
"&Tryb wyświetlania         Alt-F9"
"Modo de video         Alt-F9"
"Відео&режим               Alt-F9"

MMenuFindFolder
"Поис&к папки              Alt-F10"
"Fi&nd folder         Alt-F10"
"Hl&edat adresář                 Alt-F10"
"Ordner fi&nden        Alt-F10"
"&Mappakeresés        Alt-F10"
"Znajdź kata&log            Alt-F10"
"Buscar &directorios    Alt-F10"
"Пошу&к теки               Alt-F10"

MMenuViewHistory
"Ис&тория просмотра        Alt-F11"
"File vie&w history   Alt-F11"
"Historie &zobrazení souborů     Alt-F11"
"Be&trachterhistorie   Alt-F11"
"Fáj&l előzmények     Alt-F11"
"Historia &podglądu plików  Alt-F11"
"Historial &visor       Alt-F11"
"Іс&торія перегляду        Alt-F11"

MMenuFoldersHistory
"Ист&ория папок            Alt-F12"
"F&olders history     Alt-F12"
"Historie &adresářů              Alt-F12"
"&Ordnerhistorie       Alt-F12"
"Ma&ppa előzmények    Alt-F12"
"Historia &katalogów        Alt-F12"
"Histo&rial dir.        Alt-F12"
"Іст&орія тек              Alt-F12"

MMenuSwapPanels
"По&менять панели          Ctrl-U"
"&Swap panels         Ctrl-U"
"Prohodit panel&y                Ctrl-U"
"Panels tau&schen      Strg-U"
"Panel&csere          Ctrl-U"
"Z&amień panele             Ctrl-U"
"I&nvertir paneles      Ctrl-U"
"Зм&інити панелі            Ctrl-U"

MMenuTogglePanels
"Панели &Вкл/Выкл          Ctrl-O"
"&Panels On/Off       Ctrl-O"
"&Panely Zap/Vyp                 Ctrl-O"
"&Panels ein/aus       Strg-O"
"Panelek &be/ki       Ctrl-O"
"&Włącz/Wyłącz panele       Ctrl-O"
"&Paneles Si/No         Ctrl-O"
"Панели &Ввмк/Вимк          Ctrl-O"

MMenuCompareFolders
"&Сравнение папок"
"&Compare folders"
"Po&rovnat adresáře"
"Ordner verglei&chen"
"Mappák össze&hasonlítása"
"Porówna&j katalogi"
"&Compara directorios"
"&Порівняння тек"

MMenuUserMenu
"Меню пользовател&я"
"Edit user &menu"
"Upravit uživatelské &menu"
"Benutzer&menu editieren"
"Felhasználói m&enü szerk."
"Edytuj &menu użytkownika"
"Editar &menú usuario"
"Меню користувач&а"

MMenuFileAssociations
"&Ассоциации файлов"
"File &associations"
"Asocia&ce souborů"
"Dat&eiverknüpfungen"
"Fájl&társítások"
"Prz&ypisania plików"
"&Asociar archivos"
"&Ассоциації файлів"

MMenuBookmarks
"Зак&ладки на папки"
"Fol&der bookmarks"
"A&dresářové zkratky"
"Or&dnerschnellzugriff"
"Mappa gyorsbillent&yűk"
"&Skróty katalogów"
"Acc&eso a directorio"
"Зак&ладки на теки"

MMenuFilter
"&Фильтр панели файлов     Ctrl-I"
"File panel f&ilter   Ctrl-I"
"F&iltr panelu souborů           Ctrl-I"
"Panelf&ilter          Strg-I"
"Fájlpanel &szűrők    Ctrl-I"
"&Filtr panelu plików       Ctrl-I"
"F&iltro de paneles     Ctrl-I"
"&Фільтр панелі файлів      Ctrl-I"

MMenuPluginCommands
"Команды внешних мо&дулей  F11"
"Pl&ugin commands     F11"
"Příkazy plu&ginů                F11"
"Pl&uginbefehle        F11"
"Pl&ugin parancsok    F11"
"Pl&uginy                   F11"
"Comandos de pl&ugin    F11"
"Команди зовнішніх мо&дулів F11"

MMenuWindowsList
"Список экра&нов           F12"
"Sc&reens list        F12"
"Seznam obrazove&k               F12"
"Seite&nliste          F12"
"Képer&nyők           F12"
"L&ista ekranów             F12"
"&Listado ventanas      F12"
"Список екра&нів           F12"

MMenuProcessList
"Список &задач             Ctrl-W"
"Task &list           Ctrl-W"
"Seznam úl&oh                    Ctrl-W"
"Task&liste            Strg-W"
"Futó p&rogramok      Ctrl-W"
"Lista za&dań               Ctrl-W"
"Lista de &tareas       Ctrl-W"
"Список & завдань          Ctrl-W"

MMenuHotPlugList
"Список Hotplug-&устройств"
"Ho&tplug devices list"
"Seznam v&yjímatelných zařízení"
"Sicheres En&tfernen"
"H&otplug eszközök"
"Lista urządzeń Ho&tplug"
"Lista de dispositivos ho&tplug"
"Список Hotplug-&пристроїв"

MMenuSystemSettings
l:
"Систе&мные параметры"
"S&ystem settings"
"Nastavení S&ystému"
"&Grundeinstellungen"
"&Rendszer beállítások"
"Ustawienia &systemowe"
"&Sistema      "
"Систе&мні параметри"

MMenuPanelSettings
"Настройки па&нели"
"&Panel settings"
"Nastavení &Panelů"
"&Panels einrichten"
"&Panel beállítások"
"Ustawienia &panelu"
"&Paneles      "
"Налаштування па&нелі"

MMenuInterface
"Настройки &интерфейса"
"&Interface settings"
"Nastavení Ro&zhraní"
"Oberfläche einr&ichten"
"Kezelő&felület beállítások"
"Ustawienia &interfejsu"
"&Interfaz     "
"Налаштування &інтерфейса"

MMenuLanguages
"&Языки"
"Lan&guages"
"Nastavení &Jazyka"
"Sprac&hen"
"N&yelvek (Languages)"
"&Język"
"&Idiomas"
"&Мови"

MMenuInput
"Параметры &ввода"
"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Inpu&t settings"
upd:"Параметры &ввода"

MMenuPluginsConfig
"Параметры &внешних модулей"
"Pl&ugins configuration"
"Nastavení Plu&ginů"
"Konfiguration von Pl&ugins"
"Pl&ugin beállítások"
"Konfiguracja p&luginów"
"Configuración de pl&ugins"
"Параметри &зовнішніх модулів"

MMenuPluginsManagerSettings
"Параметры менеджера внешних модулей"
"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
"Параметри менеджера зовнішніх модулів"

MMenuDialogSettings
"Настройки &диалогов"
"Di&alog settings"
"Nastavení Dialo&gů"
"Di&aloge einrichten"
"Pár&beszédablak beállítások"
"Ustawienia okna &dialogowego"
"Opciones de di&álogo"
"Налаштування &діалогів"

MMenuVMenuSettings
"Настройки меню"
"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
upd:"Menu settings"
"Налаштування меню"

MMenuCmdlineSettings
"Настройки &командной строки"
"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
upd:"Command line settings"
"Opciones de línea de comando"
"Налаштування &командного рядка"

MMenuAutoCompleteSettings
"На&стройки автозавершения"
"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
upd:"AutoComplete settings"
"Opciones de autocompletar"
"На&лаштування автозавершення"

MMenuInfoPanelSettings
"Нас&тройки информационной панели"
"Inf&oPanel settings"
upd:"Inf&oPanel settings"
upd:"Inf&oPanel settings"
upd:"Inf&oPanel settings"
upd:"Inf&oPanel settings"
"Opciones de panel de inf&ormación"
"Нал&аштування інформаційної панелі"

MMenuConfirmation
"&Подтверждения"
"Co&nfirmations"
"P&otvrzení"
"Bestätigu&ngen"
"Meg&erősítések"
"P&otwierdzenia"
"Co&nfirmaciones"
"&Підтвердження"

MMenuFilePanelModes
"Режим&ы панели файлов"
"File panel &modes"
"&Módy souborových panelů"
"Anzeige&modi von Dateipanels"
"Fájlpanel mód&ok"
"&Tryby wyświetlania panelu plików"
"&Modo de paneles de archivos"
"Режим&и панелі файлів"

MMenuFileDescriptions
"&Описания файлов"
"File &descriptions"
"Popi&sy souborů"
"&Dateibeschreibungen"
"Fájl &megjegyzésfájlok"
"Opis&y plików"
"&Descripción de archivos"
"&Описи файлів"

MMenuFolderInfoFiles
"Файлы описания п&апок"
"&Folder description files"
"Soubory popisů &adresářů"
"O&rdnerbeschreibungen"
"M&appa megjegyzésfájlok"
"Pliki opisu &katalogu"
"&Archivo de descripción de directorios"
"Файли описів т&ек"

MMenuViewer
"Настройки про&граммы просмотра"
"&Viewer settings"
"Nastavení P&rohlížeče"
"Be&trachter einrichten"
"&Nézőke beállítások"
"Ustawienia pod&glądu"
"&Visor "
"Налаштування про&грами перегляду"

MMenuEditor
"Настройки &редактора"
"&Editor settings"
"Nastavení &Editoru"
"&Editor einrichten"
"&Szerkesztő beállítások"
"Ustawienia &edytora"
"&Editor "
"Налаштування &редактора"

MMenuNotifications
"Настройки &уведомлений"
"No&tifications settings"
upd:"No&tifications settings"
upd:"No&tifications settings"
upd:"No&tifications settings"
upd:"No&tifications settings"
upd:"No&tifications settings"
"Налаштування &повідомлень"

MMenuCodePages
"Кодов&ые страницы"
upd:"&Code pages"
upd:"Znakové sady:"
upd:"Tabellen"
upd:"Kódlapok"
upd:"Strony kodowe"
"Tablas (code pages)"
"Кодов&і сторінки"

MMenuColors
"&Цвета"
"Co&lors"
"&Barvy"
"&Farben"
"S&zínek"
"Kolo&ry"
"&Colores"
"&Кольори"

MMenuFilesHighlighting
"Раскраска &файлов и группы сортировки"
"Files &highlighting and sort groups"
"Z&výrazňování souborů a skupiny řazení"
"Farbmar&kierungen und Sortiergruppen"
"Fájlkiemelések, rendezési &csoportok"
"&Wyróżnianie plików"
"&Resaltar archivos y ordenar grupos"
"Розмальовка &файлів та групи сортування"

MMenuSaveSetup
"&Сохранить параметры                  Shift-F9"
"&Save setup                         Shift-F9"
"&Uložit nastavení                      Shift-F9"
"Setup &speichern                     Umsch-F9"
"Beállítások men&tése                 Shift-F9"
"&Zapisz ustawienia       Shift-F9"
"&Guardar configuración     Shift-F9"
"&Зберегти параметри                   Shift-F9"

MMenuTogglePanelRight
"Панель &Вкл/Выкл          Ctrl-F2"
"Panel &On/Off       Ctrl-F2"
"Panel &Zap/Vyp            Ctrl-F2"
"Panel &ein/aus        Strg-F2"
"Panel be/&ki        Ctrl-F2"
"Włącz/wyłącz pane&l   Ctrl-F2"
"Panel &Si/No           Ctrl-F2"
"Панель &Ввмк/Вимк         Ctrl-F2"

MMenuChangeDriveRight
"С&менить диск             Alt-F2"
"&Change drive       Alt-F2"
"Z&měnit jednotku          Alt-F2"
"Laufwerk &wechseln    Alt-F2"
"Meghajtó&váltás     Alt-F2"
"Z&mień napęd          Alt-F2"
"Cambiar &unidad        Alt-F2"
"З&мінити диск             Alt-F2"

MMenuLeftTitle
l:
"&Левая"
"&Left"
"&Levý"
"&Links"
"&Bal"
"&Lewy"
"&Izquierdo"
"&Ліва"

MMenuFilesTitle
"&Файлы"
"&Files"
"&Soubory"
"&Dateien"
"&Fájlok"
"Pl&iki"
"&Archivo"
"&Файли"

MMenuCommandsTitle
"&Команды"
"&Commands"
"Pří&kazy"
"&Befehle"
"&Parancsok"
"Pol&ecenia"
"&Comandos"
"&Команди"

MMenuOptionsTitle
"Па&раметры"
"&Options"
"&Nastavení"
"&Optionen"
"B&eállítások"
"&Opcje"
"&Opciones"
"Па&раметри"

MMenuRightTitle
"&Правая"
"&Right"
"&Pravý"
"&Rechts"
"&Jobb"
"&Prawy"
"&Derecho"
"&Права"

MMenuSortTitle
l:
"Критерий сортировки"
"Sort by"
"Seřadit podle"
"Sortieren nach"
"Rendezési elv"
"Sortuj według..."
"Ordenar por"
"Критерій сортування"

MMenuSortByName
"&Имя                              Ctrl-F3"
"&Name                   Ctrl-F3"
"&Názvu                     Ctrl-F3"
"&Name                   Strg-F3"
"&Név                  Ctrl-F3"
"&nazwy                       Ctrl-F3"
"&Nombre               Ctrl-F3"
"&ІИм'я                            Ctrl-F3"

MMenuSortByExt
"&Расширение                       Ctrl-F4"
"E&xtension              Ctrl-F4"
"&Přípony                   Ctrl-F4"
"&Erweiterung            Strg-F4"
"Ki&terjesztés         Ctrl-F4"
"ro&zszerzenia                Ctrl-F4"
"E&xtensión            Ctrl-F4"
"&Розширення                       Ctrl-F4"

MMenuSortByWrite
"Время &записи                     Ctrl-F5"
"&Write time             Ctrl-F5"
upd:"&Write time             Ctrl-F5"
upd:"&Write time             Ctrl-F5"
upd:"&Write time             Ctrl-F5"
upd:"&Write time             Ctrl-F5"
"Fecha &modificación   Ctrl-F5"
"Час &запису                       Ctrl-F5"

MMenuSortBySize
"Р&азмер                           Ctrl-F6"
"&Size                   Ctrl-F6"
"&Velikosti                 Ctrl-F6"
"&Größe                  Strg-F6"
"&Méret                Ctrl-F6"
"&rozmiaru                    Ctrl-F6"
"&Tamaño               Ctrl-F6"
"Р&озмір                           Ctrl-F6"

MMenuUnsorted
"&Не сортировать                   Ctrl-F7"
"&Unsorted               Ctrl-F7"
"N&eřadit                   Ctrl-F7"
"&Unsortiert             Strg-F7"
"&Rendezetlen          Ctrl-F7"
"&bez sortowania              Ctrl-F7"
"&Sin ordenar          Ctrl-F7"
"&Не сортувати                     Ctrl-F7"

MMenuSortByCreation
"Время &создания                   Ctrl-F8"
"&Creation time          Ctrl-F8"
"&Data vytvoření            Ctrl-F8"
"E&rstelldatum           Strg-F8"
"Ke&letkezés ideje     Ctrl-F8"
"czasu u&tworzenia            Ctrl-F8"
"Fecha de &creación    Ctrl-F8"
"Час &створення                    Ctrl-F8"

MMenuSortByAccess
"Время &доступа                    Ctrl-F9"
"&Access time            Ctrl-F9"
"Ča&su přístupu             Ctrl-F9"
"&Zugriffsdatum          Strg-F9"
"&Hozzáférés ideje     Ctrl-F9"
"czasu &użycia                Ctrl-F9"
"Fecha de &acceso      Ctrl-F9"
"Час &доступу                      Ctrl-F9"

MMenuSortByChange
"Время из&менения"
"Chan&ge time"
upd:"Change time"
upd:"Change time"
upd:"Change time"
upd:"Change time"
upd:"Change time"
"Час з&міни"

MMenuSortByDiz
"&Описания                         Ctrl-F10"
"&Descriptions           Ctrl-F10"
"P&opisků                   Ctrl-F10"
"&Beschreibungen         Strg-F10"
"Megjegyzé&sek         Ctrl-F10"
"&opisu                       Ctrl-F10"
"&Descripciones        Ctrl-F10"
"&Опис                             Ctrl-F10"

MMenuSortByOwner
"&Владельцы файлов                 Ctrl-F11"
"&Owner                  Ctrl-F11"
"V&lastníka                 Ctrl-F11"
"Bes&itzer               Strg-F11"
"Tula&jdonos           Ctrl-F11"
"&właściciela                 Ctrl-F11"
"Dueñ&o                Ctrl-F11"
"&Власники файлів                  Ctrl-F11"

MMenuSortByPhysicalSize
"&Физический размер"
"&Physical size"
upd:"&Komprimované velikosti"
upd:"Kom&primierte Größe"
upd:"Tömörített mér&et"
upd:"rozmiaru po &kompresji"
upd:"Tamaño de com&presin"
"&Фізичний розмір"

MMenuSortByNumLinks
"Ко&личество ссылок"
"Number of &hard links"
"Poč&tu pevných linků"
"Anzahl an &Links"
"Hardlinkek s&záma"
"&liczby dowiązań"
"Número de enlaces &rígidos"
"Кі&лькість посилань"

MMenuSortByFullName
"&Полное имя"
"&Full name"
upd:"&Full name"
upd:"&Full name"
upd:"&Full name"
upd:"&Full name"
"Nombre completo"
"&Повне ім'я"

MMenuSortByCustomData
upd:"Cus&tom data"
"Cus&tom data"
upd:"Cus&tom data"
upd:"Cus&tom data"
upd:"Cus&tom data"
upd:"Cus&tom data"
"Datos opcionales"
upd:"Cus&tom data"

MMenuSortUseGroups
"Использовать &группы сортировки   Shift-F11"
"Use sort &groups        Shift-F11"
"Řazení podle skup&in       Shift-F11"
"Sortier&gruppen verw.   Umsch-F11"
"Rend. cs&oport haszn. Shift-F11"
"użyj &grup sortowania        Shift-F11"
"Usar orden/&grupo      Shift-F11"
"Використовувати &групи сортування   Shift-F11"

MMenuSortSelectedFirst
"Помеченные &файлы вперёд          Shift-F12"
"Show selected f&irst    Shift-F12"
"Nejdřív zobrazit vy&brané  Shift-F12"
"&Ausgewählte zuerst     Umsch-F12"
"Kijel&ölteket előre   Shift-F12"
"zazna&czone najpierw         Shift-F12"
"Mostrar seleccionados primero Shift-F12"
"Позначені &файли вперед Shift-F12"

MMenuSortDirectoriesFirst
"&Каталоги вперёд"
"Sho&w directories first"
upd:"Sho&w directories first"
upd:"Sho&w directories first"
upd:"Sho&w directories first"
upd:"Sho&w directories first"
"Mostrar directorios primero"
"&Каталоги вперед"

MMenuSortUseNumeric
"&Числовая сортировка"
"Num&eric sort"
"Použít čí&selné řazení"
"Nu&merische Sortierung"
"N&umerikus rendezés"
"Sortuj num&erycznie"
"Usar orden num&érico"
"&Числове сортування"

MMenuSortUseCaseSensitive
"Сортировка с учётом регистра"
"Use case sensitive sort"
"Použít řazení citlivé na velikost písmen"
"Sortierung abhängig von Groß-/Kleinschreibung"
"Nagy/kisbetű érzékeny rendezés"
"Sortuj uwzględniając wielkość liter"
"Usar orden sensible a min/mayúsc."
"Сортування з урахуванням регістру"

MChangeDriveTitle
l:
"Перейти"
"Location"
"Jednotka"
"Laufwerke"
"Meghajtók"
"Napęd"
"Unidad"
"Перейти"

MChangeDriveRemovable
"сменный"
"removable"
"vyměnitelná"
"wechsel."
"kivehető"
"wyjmowalny"
"removible"
"змінний"

MChangeDriveFixed
"жёсткий"
"fixed"
"pevná"
"fest"
"fix"
"stały"
"rígido   "
"жорсткий"

MChangeDriveNetwork
"сетевой"
"network"
"síťová"
"Netzwerk"
"hálózati"
"sieciowy"
"red      "
"мережевий"

MChangeDriveDisconnectedNetwork
"отключенный"
"disconnected"
upd:"disconnected"
upd:"disconnected"
"leválasztva"
upd:"disconnected"
"desconectado"
"відключений"

MChangeDriveCDROM
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM"
"CD-ROM   "
"CD-ROM"

MChangeDriveCD_RW
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"
"CD-RW"

MChangeDriveCD_RWDVD
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"
"CD-RW/DVD"

MChangeDriveDVD_ROM
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"
"DVD-ROM"

MChangeDriveDVD_RW
"DVD-RW"
"DVD-RW"
"DWD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"
"DVD-RW"

MChangeDriveDVD_RAM
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"
"DVD-RAM"

MChangeDriveBD_ROM
"BD-ROM"
"BD-ROM"
"BD-ROM"
"BD-ROM"
"BD-ROM"
"BD-ROM"
"BD-ROM"
"BD-ROM"

MChangeDriveBD_RW
"BD-RW"
"BD-RW"
"BD-RW"
"BD-RW"
"BD-RW"
"BD-RW"
"BD-RW"
"BD-RW"

MChangeDriveHDDVD_ROM
"HDDVD-ROM"
"HDDVD-ROM"
"HDDVD-ROM"
"HDDVD-ROM"
"HDDVD-ROM"
"HDDVD-ROM"
"HDDVD-ROM"
"HDDVD-ROM"

MChangeDriveHDDVD_RW
"HDDVD-RW"
"HDDVD-RW"
"HDDVD-RW"
"HDDVD-RW"
"HDDVD-RW"
"HDDVD-RW"
"HDDVD-RW"
"HDDVD-RW"


MChangeDriveRAM
"RAM диск"
"RAM disk"
"RAM disk"
"RAM-DISK"
"RAM lemez"
"RAM-dysk"
"Disco RAM"
"RAM диск"

MChangeDriveSUBST
"SUBST"
"subst"
"SUBST"
"Subst"
"virtuális"
"subst"
"subst    "
"SUBST"

MChangeDriveVirtual
"виртуальный"
"virtual"
upd:"virtual"
upd:"virtual"
upd:"virtual"
upd:"virtual"
upd:"virtual"
"віртуальний"

MChangeDriveLabelAbsent
"недоступен"
"not available"
"není k dispozici"
"nicht vorh."
"nem elérhető"
"niedostępny"
"no disponible"
"недоступний"

MChangeDriveCannotReadDisk
"Ошибка чтения диска в дисководе"
"Cannot read the disk in drive"
"Nelze přečíst disk v jednotce"
"Kann nicht gelesen werden datenträge in Laufwerk"
"Meghajtó lemeze nem olvasható"
"Nie mogę odczytać dysku w napędzie"
"No se puede leer el disco en unidad"
"Помилка читання диска в дисководі"

MChangeDriveCannotDisconnect
"Не удаётся отсоединиться от %ls"
"Cannot disconnect from %ls"
"Nelze se odpojit od %ls"
"Verbindung zu %ls konnte nicht getrennt werden."
"Nem lehet leválni innen: %ls"
"Nie mogę odłączyć się od %ls"
"No se puede desconectar desde %ls"
"Неможливо від'єднатися від %ls"

MChangeDriveCannotDelSubst
"Не удаётся удалить виртуальный драйвер %ls"
"Cannot delete a substituted drive %ls"
"Nelze smazat substnutá jednotka %ls"
"Substlaufwerk %ls konnte nicht gelöscht werden."
"%ls virtuális meghajtó nem törölhető"
"Nie można usunąć dysku SUBST %ls"
"No se puede borrar una unidad sustituida %ls"
"Не вдалося видалити віртуальний драйвер %ls"

MChangeDriveOpenFiles
"Если вы не закроете открытые файлы, данные могут быть утеряны"
"If you do not close the open files, data may be lost"
"Pokud neuzavřete otevřené soubory, mohou být tato data ztracena"
"Wenn Sie offene Dateien nicht schließen könnten Daten verloren gehen"
"Ha a nyitott fájlokat nem zárja be, az adatok elveszhetnek"
"Jeśli nie zamkniesz otwartych plików, możesz utracić dane"
"Si no cierra los archivos abiertos, los datos se pueden perder."
"Якщо ви не закриєте відкриті файли, дані можуть бути втрачені"

MChangeSUBSTDisconnectDriveTitle
l:
"Отключение виртуального устройства"
"Virtual device disconnection"
"Odpojování virtuálního zařízení"
"Virtuelles Gerät trennen"
"Virtuális meghajtó törlése"
"Odłączanie napędu wirtualnego"
"Desconexion de dispositivo virtual"
"Вимкнення віртуального пристрою"

MChangeSUBSTDisconnectDriveQuestion
"Отключить SUBST-диск %c:?"
"Disconnect SUBST-disk %c:?"
"Odpojit SUBST-disk %c:?"
"Substlaufwerk %c: trennen?"
"Törli %c: virtuális meghajtót?"
"Odłączyć dysk SUBST %c:?"
"Desconectarse de disco sustituido %c:?"
"Вимкнути SUBST-диск %c:?"

MChangeVHDDisconnectDriveTitle
"Отсоединение виртуального диска"
"Virtual disk detaching"
upd:"Virtual disk detaching"
upd:"Virtual disk detaching"
upd:"Virtual disk detaching"
upd:"Virtual disk detaching"
upd:"Virtual disk detaching"
"Від'єднання віртуального диска"

MChangeVHDDisconnectDriveQuestion
"Отсоединить виртуальный диск %c:?"
"Detach virtual disk %c:?"
upd:"Detach virtual disk %c:?"
upd:"Detach virtual disk %c:?"
upd:"Detach virtual disk %c:?"
upd:"Detach virtual disk %c:?"
upd:"Detach virtual disk %c:?"
"Від'єднати віртуальний диск %c:?"

MChangeHotPlugDisconnectDriveTitle
l:
"Удаление устройства"
"Device Removal"
"Odpojování zařízení"
"Sicheres Entfernen"
"Eszköz biztonságos eltávolítása"
"Odłączanie urządzenia"
"Remover dispositivo"
"Видалення пристрою"

MChangeHotPlugDisconnectDriveQuestion
"Вы хотите удалить устройство"
"Do you want to remove the device"
"Opravdu si přejete odpojit zařízení"
"Wollen Sie folgendes Gerät sicher entfernen? "
"Eltávolítja az eszközt?"
"Czy odłączyć urządzenie"
"Desea remover el dispositivo"
"Ви хочете видалити пристрій"

MHotPlugDisks
"(диск(и): %ls)"
"(disk(s): %ls)"
"(disk(y): %ls)"
"(Laufwerk(e): %ls)"
"(%ls meghajtó)"
"(dysk(i): %ls)"
"(disco(s): %ls)"
"(диск(и): %ls)"

MChangeCouldNotEjectHotPlugMedia
"Невозможно удалить устройство для диска %c:"
"Cannot remove a device for drive %c:"
"Zařízení %c: nemůže být odpojeno."
"Ein Gerät für Laufwerk %c: konnte nicht entfernt werden"
"%c: eszköz nem távolítható el"
"Nie udało się odłączyć dysku %c:"
"No se puede remover dispositivo para unidad %c:"
"Не можна видалити пристрій для диска %c:"

MChangeCouldNotEjectHotPlugMedia2
"Невозможно удалить устройство:"
"Cannot remove a device:"
"Zařízení nemůže být odpojeno."
"Kann folgendes Geräte nicht entfernen:"
"Az eszköz nem távolítható el:"
"Nie udało się odłączyć urządzenia:"
"No se puede remover el dispositivo:"
"Не можна видалити пристрій:"

MChangeHotPlugNotify1
"Теперь устройство" 
"The device" 
"Zařízení"
"Das Gerät"
"Az eszköz:"
"Urządzenie"
"El dispositivo"
"Тепер пристрій"

MChangeHotPlugNotify2
"может быть безопасно извлечено из компьютера"
"can now be safely removed"
"může být nyní bezpečně odebráno"
"kann nun vom Computer getrennt werden"
"már biztonságosan eltávolítható"
"można teraz bezpiecznie odłączyć"
"ahora puede ser removido de forma segura"
"може бути безпечно вилучено з комп'ютера"

MHotPlugListTitle
"Hotplug-устройства"
"Hotplug devices list"
"Seznam vyjímatelných zařízení"
"Hardware sicher entfernen"
"Hotplug eszközök"
"Lista urządzeń Hotplug"
"Lista de conexión de dispositivos"
"Hotplug-пристрої"

MHotPlugListBottom
"Редактирование: Del,Ctrl-R"
"Edit: Del,Ctrl-R"
"Edit: Del,Ctrl-R"
"Tasten: Entf,StrgR,F1"
"Szerkesztés: Del,Ctrl-R"
"Edycja: Del,Ctrl-R"
"Editar: Del,Ctrl-R"
"Редагування: Del,Ctrl-R"

MChangeDriveDisconnectTitle
l:
"Отключение сетевого устройства"
"Disconnect network drive"
"Odpojit síťovou jednotku"
"Netzwerklaufwerk trennen"
"Hálózati meghajtó leválasztása"
"Odłączanie dysku sieciowego"
"Desconectar unidad de red"
"Вимкнення мережного пристрою"

MChangeDriveDisconnectQuestion
"Вы хотите удалить соединение с устройством %c:?"
"Do you want to disconnect from the drive %c:?"
"Opravdu si přejete odpojit od jednotky %c:?"
"Wollen Sie die Verbindung zu Laufwerk %c: trennen?"
"Le akar válni %c: meghajtóról?"
"Czy odłączyć dysk %c:?"
"Quiere desconectarse desde la unidad %c:?"
"Ви хочете видалити з'єднання з пристроєм %c:?"

MChangeDriveDisconnectMapped
"На устройство %c: отображена папка"
"The drive %c: is mapped to..."
"Jednotka %c: je namapována na..."
"Laufwerk %c: ist verknüpft zu..."
"%c: meghajtó hozzárendelve:"
"Dysk %c: jest skojarzony z..."
"La unidad %c: es mapeada hacia..."
"На пристрій %c: відображено папку"

MChangeDriveDisconnectReconnect
"&Восстанавливать при входе в систему"
"&Reconnect at Logon"
"&Znovu připojit při přihlášení"
"Bei Anmeldung &verbinden"
"&Bejelentkezésnél újracsatlakoztat"
"&Podłącz ponownie przy logowaniu"
"&Reconectar al desconectar"
"&Відновити при вході в систему"

MChangeDriveAskDisconnect
l:
"Вы хотите в любом случае отключиться от устройства?"
"Do you want to disconnect the device anyway?"
"Přejete si přesto zařízení odpojit?"
"Wollen Sie die Verbindung trotzdem trennen?"
"Mindenképpen leválasztja az eszközt?"
"Czy chcesz mimo to odłączyć urządzenie?"
"Quiere desconectar el dispositivo de cualquier forma?"
"Ви хочете в будь-якому випадку відключитися від пристрою?"

MChangeVolumeInUse
"Не удаётся извлечь диск из привода %c:"
"Volume %c: cannot be ejected."
"Jednotka %c: nemůže být vysunuta."
"Datenträger %c: kann nicht ausgeworfen werden."
"%c: kötet nem oldható ki"
"Nie można wysunąć dysku %c."
"Volumen %c: no puede ser expulsado."
"Не вдається вийняти диск із приводу %c:"

MChangeVolumeInUse2
"Используется другим приложением"
"It is used by another application"
"Je používaná jinou aplikací"
"Andere Programme greifen momentan darauf zu"
"Másik program használja"
"Jest używany przez inną aplikację"
"Es usada por otra aplicación"
"Використовується іншим додатком"

MChangeWaitingLoadDisk
"Ожидание чтения диска..."
"Waiting for disk to mount..."
"Čekám na disk k připojení..."
"Warte auf Datenträger..."
"Lemez betöltése..."
"Trwa montowanie dysku..."
"Esperando para montar el disco..."
"Чекання читання диска..."

MChangeCouldNotEjectMedia
"Невозможно извлечь диск из привода %c:"
"Could not eject media from drive %c:"
"Nelze vysunout médium v jednotce %c:"
"Konnte Medium in Laufwerk %c: nicht auswerfen"
"%c: meghajtó lemeze nem oldható ki"
"Nie można wysunąć dysku z napędu %c:"
"No se puede expulsar medio de la unidad %c:"
"Неможливо вилучити диск із приводу %c:"

MChangeDriveConfigure
"Настройка меню выбора диска"
"Change Drive Menu Options"
upd:"Change Drive Menu Options"
upd:"Change Drive Menu Options"
upd:"Change Drive Menu Options"
upd:"Change Drive Menu Options"
"Cambiar opciones de menú de unidades"
"Налаштування меню вибору диска"

MChangeDriveShowDiskType
"Показывать &тип диска"
"Show disk &type"
upd:"Show disk type"
upd:"Show disk type"
upd:"Show disk type"
upd:"Show disk type"
"Mostrar &tipo de disco"
"Показувати &тип диска"

MChangeDriveShowNetworkName
"Показывать &сетевое имя/путь SUBST/имя VHD"
"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
upd:"Show &network name/SUBST path/VHD name"
"Показувати мережеве ім'я/шлях SUBST/ім'я VHD"

MChangeDriveShowLabel
"Показывать &метку диска"
"Show disk &label"
upd:"Show disk &label"
upd:"Show disk &label"
upd:"Show disk &label"
upd:"Show disk &label"
"Mostrar etiqueta"
"Показувати &мітку диска"

MChangeDriveShowFileSystem
"Показывать тип &файловой системы"
"Show &file system type"
upd:"Show &file system type"
upd:"Show &file system type"
upd:"Show &file system type"
upd:"Show &file system type"
"Mostrar sistema de archivos"
"Показувати тип &файлової системи"

MChangeDriveShowSize
"Показывать &размер"
"Show &size"
upd:"Show &size"
upd:"Show &size"
upd:"Show &size"
upd:"Show &size"
"Mostrar tamaño"
"Показувати &розмір"

MChangeDriveShowSizeFloat
"Показывать ра&змер в стиле Windows Explorer"
"Show size in &Windows Explorer style"
upd:"Show size in &Windows Explorer style"
upd:"Show size in &Windows Explorer style"
upd:"Show size in &Windows Explorer style"
upd:"Show size in &Windows Explorer style"
"Mostrar tamaño estilo &Windows Explorer"
"Показувати ро&змір у стилі Windows Explorer"

MChangeDriveShowRemovableDrive
"Показывать параметры см&енных дисков"
"Show &removable drive parameters"
upd:"Show &removable drive parameters"
upd:"Show &removable drive parameters"
upd:"Show &removable drive parameters"
upd:"Show &removable drive parameters"
"Mostrar parámetros de unidad removible"
"Показувати параметри зм&інних дисків"

MChangeDriveShowPlugins
"Показывать &плагины"
"Show &plugins"
upd:"Show &plugins"
upd:"Show &plugins"
upd:"Show &plugins"
upd:"Show &plugins"
"Mostrar &plugins"
"Показувати &Плагіни"

MChangeDriveShowShortcuts
"Показывать &закладки"
"Show &bookmarks"
upd:"Show &bookmarks"
upd:"Show &bookmarks"
upd:"Show &bookmarks"
upd:"Show &bookmarks"
upd:"Mostrar &bookmarks"
"Показувати &закладки"

MChangeDriveShowCD
"Показывать параметры &компакт-дисков"
"Show &CD drive parameters"
upd:"Show &CD drive parameters"
upd:"Show &CD drive parameters"
upd:"Show &CD drive parameters"
upd:"Show &CD drive parameters"
"Mostrar parámetros unidad de &CD"
"Показувати параметри &компакт-дисків"

MChangeDriveShowNetworkDrive
"Показывать параметры се&тевых дисков"
"Show n&etwork drive parameters"
upd:"Show ne&twork drive parameters"
upd:"Show ne&twork drive parameters"
upd:"Show ne&twork drive parameters"
upd:"Show ne&twork drive parameters"
"Mostrar parámetros unidades de red"
"Показувати параметри ме&режевих дисків"

MChangeDriveMenuFooter
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"
"Ins,Del,Shift-Del,F4,F9"

MEditControlHistoryFooter
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"
"Up/Down,Enter,Esc,Shift-Del"

MEditControlHistoryFooterNoDel
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"
"Up/Down,Enter,Esc"

MHistoryFooter
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"
"Up/Down,Enter,Esc,Shift-Del,Del,Ins,Ctrl-C"

MSearchFileTitle
l:
" Поиск "
" Search "
" Hledat "
" Suchen "
" Keresés "
" Szukaj "
" Buscar "
" Пошук "

MCannotCreateListFile
"Ошибка создания списка файлов"
"Cannot create list file"
"Nelze vytvořit soubor se seznamem"
"Dateiliste konnte nicht erstellt werden"
"A listafájl nem hozható létre"
"Nie mogę utworzyć listy plików"
"No se puede crear archivo de lista"
"Помилка створення списку файлів"

MCannotCreateListTemp
"(невозможно создать временный файл для списка)"
"(cannot create temporary file for list)"
"(nemohu vytvořit dočasný soubor pro seznam)"
"(Fehler beim Anlegen einer temporären Datei für Liste)"
"(a lista átmeneti fájl nem hozható létre)"
"(nie można utworzyć pliku tymczasowego dla listy)"
"(no se puede crear archivo temporal para lista)"
"(неможливо створити тимчасовий файл для списку)"

MCannotCreateListWrite
"(невозможно записать данные в файл)"
"(cannot write data in file)"
"(nemohu zapsat data do souboru)"
"(Fehler beim Schreiben der Daten)"
"(a fájlba nem írható adat)"
"(nie można zapisać danych do pliku)"
"(no se puede escribir datos en el archivo)"
"(неможливо записати дані у файл)"

MDragFiles
l:
"%d файлов"
"%d files"
"%d souborů"
"%d Dateien"
"%d fájl"
"%d plików"
"%d archivos"
"%d файлів"

MDragMove
"Перенос %ls"
"Move %ls"
"Přesunout %ls"
"Verschiebe %ls"
"%ls mozgatása"
"Przenieś %ls"
"Mover %ls"
"Перенесення %ls"

MDragCopy
"Копирование %ls"
"Copy %ls"
"Kopírovat %ls"
"Kopiere %ls"
"%ls másolása"
"Kopiuj %ls"
"Copiar %ls"
"Копіювання %ls"

MProcessListTitle
l:
"Список задач"
"Task list"
"Seznam úloh"
"Taskliste"
"Futó programok"
"Lista zadań"
"Lista de tareas"
"Список завдань"

MProcessListBottom
"Редактирование: Del,Ctrl-R"
"Edit: Del,Ctrl-R"
"Edit: Del,Ctrl-R"
"Tasten: Entf,StrgR"
"Szerk.: Del,Ctrl-R"
"Edycja: Del,Ctrl-R"
"Editar: Del,Ctrl-R"
"Редагування: Del,Ctrl-R"

MKillProcessTitle
"Удаление задачи"
"Kill task"
"Zabít úlohu"
"Task beenden"
"Programkilövés"
"Zakończ zadanie"
"Terminar tarea"
"Видалення завдання"

MAskKillProcess
"Вы хотите удалить выбранную задачу?"
"Do you wish to kill selected task?"
"Přejete si zabít vybranou úlohu?"
"Wollen Sie den ausgewählten Task beenden?"
"Ki akarja lőni a kijelölt programot?"
"Czy chcesz zakończyć wybrane zadanie?"
"Desea terminar la tarea seleccionada?"
"Ви хочете видалити вибране завдання?"

MKillProcessWarning
"Вы потеряете всю несохраненную информацию этой программы"
"You will lose any unsaved information in this program"
"V tomto programu budou ztraceny neuložené informace"
"Alle ungespeicherten Daten dieses Programmes gehen verloren"
"A program minden mentetlen adata elvész"
"Utracisz wszystkie niezapisane dane w tym programie"
"Usted perder cualquier información no grabada de este programa"
"Ви втратите всю незбережену інформацію цієї програми"

MKillProcessKill
"Удалить"
"Kill"
"Zabít"
"Beenden"
"Kilő"
"Zakończ"
"Terminar"
"Вилучити"

MCannotKillProcess
"Указанную задачу удалить не удалось"
"Cannot kill the specified task"
"Nemohu ukončit zvolenou úlohu"
"Task konnte nicht beendet werden"
"A programot nem lehet kilőni"
"Nie mogę zakończyć wybranego zadania"
"No se puede terminar la tarea seleccionada"
"Вказане завдання видалити не вдалося"

MCannotKillProcessPerm
"Вы не имеет права удалить этот процесс."
"You have no permission to kill this process."
"Nemáte oprávnění zabít tento proces."
"Sie haben keine Rechte um diesen Prozess zu beenden."
"Nincs joga a program kilövésére"
"Nie masz wystarczających uprawnień do zakończenia procesu."
"Usted no tiene permiso para terminar este proceso."
"Ви не маєте права видалити цей процес."

MQuickViewTitle
l:
"Быстрый просмотр"
"Quick view"
"Zběžné zobrazení"
"Schnellansicht"
"Gyorsnézet"
"Szybki podgląd"
"Vista rápida"
"Швидкий перегляд"

MQuickViewFolder
"Папка"
"Folder"
"Adresář"
"Verzeichnis"
"Mappa"
"Folder"
"Directorio"
"Тека"

MQuickViewJunction
"Связь"
"Junction"
"Křížení"
"Knotenpunkt"
"Csomópont"
"Powiązanie"
"Juntar"
"Зв'язок"

MQuickViewSymlink
"Ссылка"
"Symlink"
"Symbolický link"
"Symlink"
"Szimlink"
"Link"
"Enlace"
"Посилання"

MQuickViewVolMount
"Том"
"Volume"
"Svazek"
"Datenträger"
"Kötet"
"Napęd"
"Volumen"
"Том"

MQuickViewContains
"Содержит:"
"Contains:"
"Obsah:"
"Enthält:"
"Tartalma:"
"Zawiera:"
"Contiene:"
"Утримує:"

MQuickViewFolders
"Папок               "
"Folders          "
"Adresáře           "
"Ordner           "
"Mappák száma     "
"Katalogi            "
"Directorios      "
"Тек                 "

MQuickViewFiles
"Файлов              "
"Files            "
"Soubory            "
"Dateien          "
"Fájlok száma     "
"Pliki               "
"archivos         "
"Файлів              "

MQuickViewBytes
"Размер файлов       "
"Files size       "
"Velikost souborů   "
"Gesamtgröße      "
"Fájlok mérete    "
"Rozmiar plików      "
"Tamaño archivos  "
"Розмір файлів       "

MQuickViewPhysical
"Физичеcкий размер  "
"Physical size    "
upd:"Komprim. velikost  "
upd:"Komprimiert      "
upd:"Tömörített méret "
upd:"Po kompresji        "
upd:"Tamaño comprimido"
"Фізичний розмір  "

MQuickViewRatio
"Степень сжатия      "
"Ratio            "
"Poměr              "
"Rate             "
"Tömörítés aránya "
"Procent             "
"Promedio"
"Ступінь стиснення   "

MQuickViewCluster
"Размер кластера     "
"Cluster size     "
"Velikost clusteru  "
"Clustergröße     "
"Klaszterméret    "
"Rozmiar klastra     "
"Tamaño cluster   "
"Розмір кластера     "

MQuickViewRealSize
"Реальный размер     "
"Real files size  "
"Opravdová velikost "
"Tatsächlich      "
"Valódi fájlméret "
"Właściwy rozmiar    "
"Tamaño real      "
"Реальний розмір     "

MQuickViewSlack
"Остатки кластеров   "
"Files slack      "
"Mrtvé místo        "
"Verlust          "
"Meddő terület    "
"Przestrzeń stracona "
"Desperdiciado    "
"Залишки кластерів   "

MSetAttrTitle
l:
"Атрибуты"
"Attributes"
"Atributy"
"Attribute"
"Attribútumok"
"Atrybuty"
"Atributos"
"Атрибути"

MSetAttrFor
"Изменить файловые атрибуты"
"Change file attributes for"
"Změna atributů souboru pro"
"Ändere Dateiattribute für"
"Attribútumok megváltoztatása"
"Zmień atrybuty dla"
"Cambiar atributos del archivo"
"Змінити файлові атрибути"

MSetAttrSelectedObjects
"выбранных объектов"
"selected objects"
"vybrané objekty"
"markierte Objekte"
"a kijelölt objektumokon"
"wybranych obiektów"
"objetos seleccionados"
"вибраних об'єктів"

MSetAttrHardLinks
"жёстких ссылок"
"hard links"
"pevné linky"
"Hardlinks"
"hardlink"
"linków trwałych"
"Enlace rígido"
"жорстких посилань"

MSetAttrJunction
"Связь:"
"Junction:"
"Křížení:"
"Knotenpunkte:"
"Сsomópont:"
"Powiązanie:"
"Juntar:"
"Зв'язок:"

MSetAttrSymlink
"Ссылка:"
"Symlink:"
"Link:"
"Symlink:"
"Szimlink:"
"Link:"
"Enlace:"
"Посилання:"

MSetAttrVolMount
"Том:"
"Volume:"
"Svazek:"
"Datenträger:"
"Kötet:"
"Punkt zamontowania:"
"Volumen:"
"Том:"

MSetAttrUnknownJunction
"(нет данных)"
"(data not available)"
"(data nejsou k dispozici)"
"(nicht verfügbar)"
"(adat nem elérhető)"
"(dane niedostępne)"
"(dato no disponible)"
"(немає даних)"

MSetAttrSubfolders
"Обрабатывать &вложенные папки"
"Process sub&folders"
"Zpracovat i po&dadresáře"
"Unterordner miteinbe&ziehen"
"Az almappákon is"
"Przetwarzaj &podkatalogi"
"Procesar sub&directorios"
"Обробляти &вкладені теки"

MSetAttrOwner
"Владелец:"
"Owner:"
"Vlastník:"
"Besitzer:"
"Tulajdonos:"
"Właściciel:"
"Dueño:"
"Власник:"

MSetAttrOwnerMultiple
"(несколько значений)"
"(multiple values)"
upd:"(multiple values)"
upd:"(multiple values)"
upd:"(multiple values)"
upd:"(multiple values)"
"(valores múltiples)"
"(кілька значень)"

MSetAttrModification
"Время последней &записи:"
"Last &write time:"
upd:"Last &write time:"
upd:"Last &write time:"
upd:"Last &write time:"
upd:"Last &write time:"
upd:"Last &write time:"
"Час останнього &запису:"

MSetAttrCreation
"Время со&здания:"
"Crea&tion time:"
"Čas v&ytvoření:"
"Datei erstell&t:"
"&Létrehozás dátuma/ideje:"
"Czas u&tworzenia:"
"Hora de creación:"
"Час ст&ворення:"

MSetAttrLastAccess
"Время последнего &доступа:"
"&Last access time:"
"Čas posledního pří&stupu:"
"&Letzter Zugriff:"
"&Utolsó hozzáférés dátuma/ideje:"
"Czas ostatniego &dostępu:"
"Hora de &último acceso:"
"Час останнього &доступу:"

MSetAttrChange
"Время из&менения:"
"Chan&ge time:"
upd:"Change time:"
upd:"Change time:"
upd:"Change time:"
upd:"Change time:"
upd:"Change time:"
"Час зм&іни:"

MSetAttrOriginal
"Исход&ное"
"&Original"
"&Originál"
"&Original"
"&Eredeti"
"Wstaw &oryginalny"
"Ori&ginal"
"Вихід&не"

MSetAttrCurrent
"Те&кущее"
"Curre&nt"
"So&učasný"
"Akt&uell"
"Aktuál&is"
"Wstaw &bieżący"
"Ac&tual"
"По&точне"

MSetAttrBlank
"Сбр&ос"
"&Blank"
"P&rázdný"
"L&eer"
"&Üres"
"&Wyczyść"
"&Vaciar"
"Ски&дання"

MSetAttrSet
"Установить"
"Set"
"Nastavit"
"Setzen"
"Alkalmaz"
"Usta&w"
"Poner"
"Встановити"

MSetAttrTimeTitle1
l:
"ММ%cДД%cГГГГГ чч%cмм%cсс%cмс"
"MM%cDD%cYYYYY hh%cmm%css%cms"
upd:"MM%cDD%cRRRRR hh%cmm%css%cms"
upd:"MM%cTT%cJJJJJ hh%cmm%css%cms"
upd:"HH%cNN%cÉÉÉÉÉ óó%cpp%cmm%cms"
upd:"MM%cDD%cRRRRR gg%cmm%css%cms"
"MM%cDD%cAAAAA hh%cmm%css"
"ММ%cДД%cРРРРР гг%cхх%cсс%cмс"

MSetAttrTimeTitle2
"ДД%cММ%cГГГГГ чч%cмм%cсс%cмс"
"DD%cMM%cYYYYY hh%cmm%css%cms"
upd:"DD%cMM%cRRRRR hh%cmm%css%cms"
upd:"TT%cMM%cJJJJJ hh%cmm%css%cms"
upd:"NN%cHH%cÉÉÉÉÉ óó%cpp%cmm%cms"
upd:"DD%cMM%cRRRRR gg%cmm%css%cms"
"DD%cMM%cAAAAA hh%cmm%css"
"ДД%cММ%cРРРРР гг%cхх%cсс%cмс"

MSetAttrTimeTitle3
"ГГГГГ%cММ%cДД чч%cмм%cсс%cмс"
"YYYYY%cMM%cDD hh%cmm%css%cms"
upd:"RRRRR%cMM%cDD hh%cmm%css%cms"
upd:"JJJJJ%cMM%cTT hh%cmm%css%cms"
upd:"ÉÉÉÉÉ%cHH%cNN óó%cpp%cmm%cms"
upd:"RRRRR%cMM%cDD gg%cmm%css%cms"
"AAAAA%cMM%cDD hh%cmm%css"
"ГГГГГ%cММ%cДД гг%cхх%cсс%cмс"

MSetAttrSystemDialog
"Системные &свойства"
"System &properties"
upd:"System &properties"
upd:"System &properties"
upd:"System &properties"
upd:"System &properties"
"&Propiedades del sistema"
"Системні &властивості"

MSetAttrSetting
l:
"Установка файловых атрибутов для"
"Setting file attributes for"
"Nastavení atributů souboru pro"
"Setze Dateiattribute für"
"Attribútumok beállítása"
"Ustawiam atrybuty"
"Poniendo atributos de archivo para"
"Встановлення файлових атрибутів для"

MSetAttrCannotFor
"Ошибка установки атрибутов для"
"Cannot set attributes for"
"Nelze nastavit atributy pro"
"Konnte Dateiattribute nicht setzen für"
"Az attribútumok nem állíthatók be:"
"Nie mogę ustawić atrybutów dla"
"No se pueden poner atributos para"
"Помилка установки атрибутів для"

MSetAttrCompressedCannotFor
"Не удалось установить атрибут СЖАТЫЙ для"
"Cannot set attribute COMPRESSED for"
"Nelze nastavit atribut KOMPRIMOVANÝ pro"
"Konnte Komprimierung nicht setzen für"
"A TÖMÖRÍTETT attribútum nem állítható be:"
"Nie mogę ustawić atrybutu SKOMPRESOWANY dla"
"No se puede poner atributo COMPRIMIDO a"
"Не вдалося встановити атрибут СТИСНУТИЙ для"

MSetAttrEncryptedCannotFor
"Не удалось установить атрибут ЗАШИФРОВАННЫЙ для"
"Cannot set attribute ENCRYPTED for"
"Nelze nastavit atribut ŠIFROVANÝ pro"
"Konnte Verschlüsselung nicht setzen für"
"A TITKOSÍTOTT attribútum nem állítható be:"
"Nie mogę ustawić atrybutu ZASZYFROWANY dla"
"No se puede poner atributo CIFRADO a"
"Не вдалося встановити атрибут ЗАШИФОВАНИЙ для"

MSetAttrSparseCannotFor
"Не удалось установить атрибут РАЗРЕЖЁННЫЙ для"
"Cannot set attribute SPARSE for"
upd:"Cannot set attribute SPARSE for"
upd:"Cannot set attribute SPARSE for"
"A RITKÍTOTT attribútum nem állítható be:"
upd:"Cannot set attribute SPARSE for"
"No se puede poner atributo SPARSE para"
"Не вдалося встановити атрибут РОЗРІЖЕНИЙ для"

MSetAttrTimeCannotFor
"Не удалось установить время файла для"
"Cannot set file time for"
"Nelze nastavit čas souboru pro"
"Konnte Dateidatum nicht setzen für"
"A dátum nem állítható be:"
"Nie mogę ustawić czasu pliku dla"
"No se puede poner hora de archivo para"
"Не вдалося встановити час для файлу"

MSetAttrOwnerCannotFor
"Не удалось установить владельца для"
"Cannot set owner for"
upd:"Cannot set owner for"
upd:"Cannot set owner for"
upd:"Cannot set owner for"
upd:"Cannot set owner for"
"No se puede poner como dueño para"
"Не вдалося встановити власника для"

MSetAttrGroupCannotFor
"Не удалось установить группу для"
"Cannot set group for"
upd:"Cannot set group for"
upd:"Cannot set group for"
upd:"Cannot set group for"
upd:"Cannot set group for"
upd:"Cannot set group for"
"Не вдалося встановити групу для"

MSetAttrGroup
"Группа:"
"Group:"
upd:"Group:"
upd:"Group:"
upd:"Group:"
upd:"Group:"
upd:"Group:"
"Група:"

MSetAttrAccessUser
"Права пользователя"
"User's access"
upd:"User's access"
upd:"User's access"
upd:"User's access"
upd:"User's access"
upd:"User's access"
"Права користувача"

MSetAttrAccessGroup
"Права группы"
"Group's access"
upd:"Group's access"
upd:"Group's access"
upd:"Group's access"
upd:"Group's access"
upd:"Group's access"
"Права групи"

MSetAttrAccessOther
"Права остальных"
"Other's access"
upd:"Other's access"
upd:"Other's access"
upd:"Other's access"
upd:"Other's access"
upd:"Other's access"
"Права інших"

MSetAttrAccessUserRead
"&Чтение"
"&Read"
upd:"&Read"
upd:"&Read"
upd:"&Read"
upd:"&Read"
upd:"&Read"
"&Читання"

MSetAttrAccessUserWrite
"&Запись"
"&Write"
upd:"&Write"
upd:"&Write"
upd:"&Write"
upd:"&Write"
upd:"&Write"
"&Запис"

MSetAttrAccessUserExecute
"&Исполнение"
"E&xecute"
upd:"E&xecute"
upd:"E&xecute"
upd:"E&xecute"
upd:"E&xecute"
upd:"E&xecute"
"&Виконання"

MSetAttrAccessGroupRead
"Чтение"
"Read"
upd:"Read"
upd:"Read"
upd:"Read"
upd:"Read"
upd:"Read"
"Читання"

MSetAttrAccessGroupWrite
"Запись"
"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
"Запис"

MSetAttrAccessGroupExecute
"Исполнение"
"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
"Виконання"

MSetAttrAccessOtherRead
"Чтение"
"Read"
upd:"Read"
upd:"Read"
upd:"Read"
upd:"Read"
upd:"Read"
"Читання"

MSetAttrAccessOtherWrite
"Запись"
"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
upd:"Write"
"Запис"

MSetAttrAccessOtherExecute
"Исполнение"
"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
upd:"Execute"
"Виконання"

MSetAttrAccessTime
"Время последнего доступа"
"Last access time"
upd:"Last access time"
upd:"Last access time"
upd:"Last access time"
upd:"Last access time"
upd:"Last access time"
"Час останнього доступу"

MSetAttrModificationTime
"Время последней модификации"
"Last modification time"
upd:"Last modification time"
upd:"Last modification time"
upd:"Last modification time"
upd:"Last modification time"
upd:"Last modification time"
"Час останньої модифікації"

MSetAttrStatusChangeTime
"Время изменения статуса"
"Last status change time"
upd:"Last status change time"
upd:"Last status change time"
upd:"Last status change time"
upd:"Last status change time"
upd:"Last status change time"
"Час зміни статусу"

MSetColorPanel
l:
"&Панель"
"&Panel"
"&Panel"
"&Panel"
"&Panel"
"&Panel"
"&Panel"
"&Панель"

MSetColorDialog
"&Диалог"
"&Dialog"
"&Dialog"
"&Dialog"
"Pár&beszédablak"
"Okno &dialogowe"
"&Diálogo"
"&Діалог"

MSetColorWarning
"Пр&едупреждение"
"&Warning message"
"&Varovná zpráva"
"&Warnmeldung"
"&Figyelmeztetés"
"&Ostrzeżenie"
"Me&nsaje de advertencia"
"Поп&ередження"

MSetColorMenu
"&Меню"
"&Menu"
"&Menu"
"&Menü"
"&Menü"
"&Menu"
"&Menú"
"&Меню"

MSetColorHMenu
"&Горизонтальное меню"
"Hori&zontal menu"
"Hori&zontální menu"
"Hori&zontales Menü"
"&Vízszintes menü"
"Pa&sek menu"
"Menú hori&zontal"
"&Горизонтальне меню"

MSetColorKeyBar
"&Линейка клавиш"
"&Key bar"
"&Řádek kláves"
upd:"&Key bar"
"F&unkcióbill.sor"
"Pasek &klawiszy"
"Barra de me&nú"
"&Лінійка клавиш"

MSetColorCommandLine
"&Командная строка"
"&Command line"
"Pří&kazový řádek"
"&Kommandozeile"
"P&arancssor"
"&Linia poleceń"
"Línea de &comando"
"&Командна строка"

MSetColorClock
"&Часы"
"C&lock"
"&Hodiny"
"U&hr"
"Ó&ra"
"&Zegar"
"Re&loj"
"&Годинник"

MSetColorViewer
"Про&смотрщик"
"&Viewer"
"P&rohlížeč"
"&Betrachter"
"&Nézőke"
"Pod&gląd"
"&Visor"
"Пере&глядач"

MSetColorEditor
"&Редактор"
"&Editor"
"&Editor"
"&Editor"
"&Szerkesztő"
"&Edytor"
"&Editor"
"&Редактор"

MSetColorHelp
"П&омощь"
"&Help"
"&Nápověda"
"&Hilfe"
"Sú&gó"
"P&omoc"
"&Ayuda"
"Д&опомога"

MSetDefaultColors
"&Установить стандартные цвета"
"Set de&fault colors"
"N&astavit výchozí barvy"
"Setze Standard&farben"
"Alapért. s&zínek"
"&Ustaw kolory domyślne"
"Poner colores prede&terminados"
"&Встановити стандартні кольори"

MSetBW
"Чёрно-бел&ый режим"
"&Black and white mode"
"Černo&bílý mód"
"Schwarz && &Weiß"
"Fekete-fe&hér mód"
"&Tryb czarno-biały"
"Modo &blanco y negro"
"Чорно-біл&ий режим"

MSetColorPanelNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

MSetColorPanelSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Wybrany tekst"
"Texto seleccionado"
"Вибраний текст"

MSetColorPanelHighlightedInfo
"Выделенная информация"
"Highlighted info"
"Info zvýrazněné"
"Markierung"
"Kiemelt info"
"Podświetlone info"
"Info resaltados"
"Виділена інформація"

MSetColorPanelDragging
"Перетаскиваемый текст"
"Dragging text"
"Tažený text"
"Drag && Drop Text"
"Vonszolt szöveg"
"Przeciągany tekst"
"Texto arrastrado"
"Перетягуваний текст"

MSetColorPanelBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

MSetColorPanelNormalCursor
"Обычный курсор"
"Normal cursor"
"Normální kurzor"
"Normale Auswahl"
"Normál kurzor"
"Normalny kursor"
"Cursor normal"
"Звичайний курсор"

MSetColorPanelSelectedCursor
"Выделенный курсор"
"Selected cursor"
"Vybraný kurzor"
"Markierte Auswahl"
"Kijelölt kurzor"
"Wybrany kursor"
"Cursor seleccionado"
"Виділений курсор"

MSetColorPanelNormalTitle
"Обычный заголовок"
"Normal title"
"Normální nadpis"
"Normaler Titel"
"Normál név"
"Normalny tytuł"
"Título normal"
"Звичайний заголовок"

MSetColorPanelSelectedTitle
"Выделенный заголовок"
"Selected title"
"Vybraný nadpis"
"Markierter Titel"
"Kijelölt név"
"Wybrany tytuł"
"Título seleccionado"
"Виділелений заголовок"

MSetColorPanelColumnTitle
"Заголовок колонки"
"Column title"
"Nadpis sloupce"
"Spaltentitel"
"Oszlopnév"
"Tytuł kolumny"
"Título de columna"
"Заголовок колонки"

MSetColorPanelTotalInfo
"Количество файлов"
"Total info"
"Info celkové"
"Gesamtinfo"
"Összes info"
"Całkowite info"
"Info total"
"Кількість файлів"

MSetColorPanelSelectedInfo
"Количество выбранных файлов"
"Selected info"
"Info výběr"
"Markierungsinfo"
"Kijelölt info"
"Wybrane info"
"Info seleccionados"
"Кількість вибраних файлів"

MSetColorPanelScrollbar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra desplazamiento"
"Полоса прокрутки"

MSetColorPanelScreensNumber
"Количество фоновых экранов"
"Number of background screens"
"Počet obrazovek na pozadí"
"Anzahl an Hintergrundseiten"
"Háttérképernyők száma"
"Ilość ekranów w tle "
"Número de pantallas de fondo"
"Кількість фонових екранів"

MSetColorDialogNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Tekst zwykły"
"Texto normal"
"Звичайний текст"

MSetColorDialogHighlighted
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierter Text"
"Kiemelt szöveg"
"Tekst podświetlony"
"Texto resaltado"
"Виділений текст"

MSetColorDialogDisabled
"Блокированный текст"
"Disabled text"
"Zakázaný text"
"Deaktivierter Text"
"Inaktív szöveg"
"Tekst nieaktywny"
"Deshabilitar texto"
"Блокований текст"

MSetColorDialogBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

MSetColorDialogBoxTitle
"Заголовок рамки"
"Title"
"Nadpis"
"Titel"
"Keret neve"
"Tytuł"
"Título"
"Заголовок рамки"

MSetColorDialogHighlightedBoxTitle
"Выделенный заголовок рамки"
"Highlighted title"
"Zvýrazněný nadpis"
"Markierter Titel"
"Kiemelt keretnév"
"Podświetlony tytuł"
"Título resaltado"
"Виділений заголовок рамки"

MSetColorDialogTextInput
"Ввод текста"
"Text input"
"Textový vstup"
"Texteingabe"
"Beírt szöveg"
"Wpisywany tekst"
"Entrada de texto"
"Введення тексту"

MSetColorDialogUnchangedTextInput
"Неизмененный текст"
"Unchanged text input"
"Nezměněný textový vstup"
"Unveränderte Texteingabe"
"Változatlan beírt szöveg"
"Niezmieniony wpisywany tekst "
"Entrada de texto sin cambiar"
"Незмінений текст"

MSetColorDialogSelectedTextInput
"Ввод выделенного текста"
"Selected text input"
"Vybraný textový vstup"
"Markierte Texteingabe"
"Beírt szöveg kijelölve"
"Zaznaczony wpisywany tekst"
"Entrada de texto seleccionada"
"Введення виділеного тексту"

MSetColorDialogEditDisabled
"Блокированное поле ввода"
"Disabled input line"
"Zakázaný vstupní řádek"
"Deaktivierte Eingabezeile"
"Inaktív beviteli sor"
"Nieaktywna linia wprowadzania danych"
"Deshabilitar línea de entrada"
"Блоковане поле вводу"

MSetColorDialogButtons
"Кнопки"
"Buttons"
"Tlačítka"
"Schaltflächen"
"Gombok"
"Przyciski"
"Botones"
"Кнопки"

MSetColorDialogSelectedButtons
"Выбранные кнопки"
"Selected buttons"
"Vybraná tlačítka"
"Aktive Schaltflächen"
"Kijelölt gombok"
"Wybrane przyciski"
"Botones seleccionados"
"Вибрані кнопки"

MSetColorDialogHighlightedButtons
"Выделенные кнопки"
"Highlighted buttons"
"Zvýrazněná tlačítka"
"Markierte Schaltflächen"
"Kiemelt gombok"
"Podświetlone przyciski"
"Botones resaltados"
"Виділені кнопки"

MSetColorDialogSelectedHighlightedButtons
"Выбранные выделенные кнопки"
"Selected highlighted buttons"
"Vybraná zvýrazněná tlačítka"
"Aktive markierte Schaltflächen"
"Kijelölt kiemelt gombok"
"Wybrane podświetlone przyciski "
"Botones resaltados seleccionados"
"Вибрані виділені кнопки"

MSetColorDialogDefaultButton
"Кнопка по умолчанию"
"Default button"
upd:"Default button"
upd:"Default button"
upd:"Default button"
upd:"Default button"
"Botón por defecto"
"Кнопка за замовчуванням"

MSetColorDialogSelectedDefaultButton
"Выбранная кнопка по умолчанию"
"Selected default button"
upd:"Selected default button"
upd:"Selected default button"
upd:"Selected default button"
upd:"Selected default button"
"Botón por defecto seleccionado"
"Вибрана кнопка за замовчуванням"

MSetColorDialogHighlightedDefaultButton
"Выделенная кнопка по умолчанию"
"Highlighted default button"
upd:"Highlighted default button"
upd:"Highlighted default button"
upd:"Highlighted default button"
upd:"Highlighted default button"
"Botón por defecto resaltado"
"Виділена кнопка за замовчуванням"

MSetColorDialogSelectedHighlightedDefaultButton
"Выбранная выделенная кнопка по умолчанию"
"Selected highlighted default button"
upd:"Selected highlighted default button"
upd:"Selected highlighted default button"
upd:"Selected highlighted default button"
upd:"Selected highlighted default button"
"Botón por defecto resaltado seleccionado"
"Вибрана кнопка за замовчуванням"

MSetColorDialogListBoxControl
"Список"
"List box"
"Seznam položek"
"Listenfelder"
"Listaablak"
"Lista"
"Cuadro de lista"
"Список"

MSetColorDialogComboBoxControl
"Комбинированный список"
"Combobox"
"Výběr položek"
"Kombinatiosfelder"
"Lenyíló szövegablak"
"Pole combo"
"Cuadro combo"
"Комбінований список"

MSetColorDialogListText
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Tekst zwykły"
"Texto normal"
"Звичайний текст"

MSetColorDialogListSelectedText
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Tekst wybrany"
"Texto seleccionado"
"Вибраний текст"

MSetColorDialogListHighLight
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierung"
"Kiemelt szöveg"
"Tekst podświetlony"
"Texto resaltado"
"Виділений текст"

MSetColorDialogListSelectedHighLight
"Выбранный выделенный текст"
"Selected highlighted text"
"Vybraný zvýrazněný text"
"Aktive Markierung"
"Kijelölt kiemelt szöveg"
"Tekst wybrany i podświetlony"
"Texto resaltado seleccionado"
"Вибраний виділений текст"

MSetColorDialogListDisabled
"Блокированный пункт"
"Disabled item"
"Naktivní položka"
"Deaktiviertes Element"
"Inaktív elem"
"Pole nieaktywne"
"Deshabilitar ítem"
"Блокований пункт"

MSetColorDialogListBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

MSetColorDialogListTitle
"Заголовок"
"Title"
"Nadpis"
"Titel"
"Keret neve"
"Tytuł"
"Título"
"Заголовок"

MSetColorDialogListGrayed
"Серый текст списка"
"Grayed list text"
upd:"Grayed list text"
upd:"Grayed list text"
"Szürke listaszöveg"
upd:"Grayed list text"
"Texto de listado en gris"
"Сірий текст списку"

MSetColorDialogSelectedListGrayed
"Выбранный серый текст списка"
"Selected grayed list text"
upd:"Selected grayed list text"
upd:"Selected grayed list text"
"Kijelölt szürke listaszöveg"
upd:"Selected grayed list text"
"Texto de listado en gris seleccionado"
"Вибраний сірий текст списку"

MSetColorDialogListScrollBar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra desplazamiento"
"Полоса прокрутки"

MSetColorDialogListArrows
"Индикаторы длинных строк"
"Long string indicators"
"Značka dlouhého řetězce"
"Indikator für lange Zeichenketten"
"Hosszú sztring jelzők"
"Znacznik długiego napisu"
"Indicadores de cadena larga"
"Індикатори довгих рядків"

MSetColorDialogListArrowsSelected
"Выбранные индикаторы длинных строк"
"Selected long string indicators"
"Vybraná značka dlouhého řetězce"
"Aktiver Indikator"
"Kijelölt hosszú sztring jelzők"
"Zaznaczone znacznik długiego napisu"
"Indicadores de cadena larga seleccionados"
"Вибрані індикатори довгих рядків"

MSetColorDialogListArrowsDisabled
"Блокированные индикаторы длинных строк"
"Disabled long string indicators"
"Zakázaná značka dlouhého řetězce"
"Deaktivierter Indikator"
"Inaktív hosszú sztring jelzők"
"Nieaktywny znacznik długiego napisu"
"Deshabilitar indicadores de cadena largos"
"Блоковані індикатори довгих рядків"

MSetColorMenuNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

MSetColorMenuSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Wybrany tekst"
"Texto seleccionado"
"Вибраний текст"

MSetColorMenuHighlighted
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierung"
"Kiemelt szöveg"
"Podświetlony tekst"
"Texto resaltado"
"Виділений текст"

MSetColorMenuSelectedHighlighted
"Выбранный выделенный текст"
"Selected highlighted text"
"Vybraný zvýrazněný text"
"Aktive Markierung"
"Kijelölt kiemelt szöveg"
"Wybrany podświetlony tekst "
"Texto resaltado seleccionado"
"Вибраний виділений текст"

MSetColorMenuDisabled
"Недоступный пункт"
"Disabled text"
"Neaktivní text"
"Disabled text"
"Inaktív szöveg"
"Tekst nieaktywny"
"Deshabilitar texto"
"Недоступний пункт"

MSetColorMenuGrayed
"Серый текст"
"Grayed text"
upd:"Grayed text"
upd:"Grayed text"
"Szürke szöveg"
upd:"Grayed text"
"Texto en gris"
"Сірий текст"

MSetColorMenuSelectedGrayed
"Выбранный серый текст"
"Selected grayed text"
upd:"Selected grayed text"
upd:"Selected grayed text"
"Kijelölt szürke szöveg"
upd:"Selected grayed text"
"Texto en gris seleccionado"
"Вибраний сірий текст"

MSetColorMenuBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

MSetColorMenuTitle
"Заголовок"
"Title"
"Nadpis"
"Titel"
"Keret neve"
"Tytuł"
"Título"
"Заголовок"

MSetColorMenuScrollBar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra desplazamiento"
"Полоса прокрутки"

MSetColorMenuArrows
"Индикаторы длинных строк"
"Long string indicators"
"Značka dlouhého řetězce"
"Long string indicators"
"Hosszú sztring jelzők"
"Znacznik długiego napisu"
"Indicadores de cadena larga"
"Індикатори довгих рядків"

MSetColorMenuArrowsSelected
"Выбранные индикаторы длинных строк"
"Selected long string indicators"
"Vybraná značka dlouhého řetězce"
"Selected long string indicators"
"Kijelölt hosszú sztring jelzők"
"Zaznaczone znacznik długiego napisu"
"Indicadores de cadena larga seleccionados"
"Вибрані індикатори довгих рядків"

MSetColorMenuArrowsDisabled
"Блокированные индикаторы длинных строк"
"Disabled long string indicators"
"Zakázaná značka dlouhého řetězce"
"Disabled long string indicators"
"Inaktív hosszú sztring jelzők"
"Nieaktywny znacznik długiego napisu"
"Deshabilitar indicadores de cadena largos"
"Блоковані індикатори довгих рядків"

MSetColorHMenuNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

MSetColorHMenuSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Wybrany tekst"
"Texto seleccionado"
"Вибраний текст"

MSetColorHMenuHighlighted
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierung"
"Kiemelt szöveg"
"Podświetlony tekst"
"Texto resaltado"
"Виділений текст"

MSetColorHMenuSelectedHighlighted
"Выбранный выделенный текст"
"Selected highlighted text"
"Vybraný zvýrazněný text"
"Aktive Markierung"
"Kijelölt kiemelt szöveg"
"Wybrany podświetlony tekst "
"Texto resaltado seleccionado"
"Вибраний виділений текст"

MSetColorKeyBarNumbers
l:
"Номера клавиш"
"Key numbers"
"Čísla kláves"
"Tastenziffern"
"Funkció száma"
"Numery klawiszy"
"Números teclas"
"Номери клавіш"

MSetColorKeyBarNames
"Названия клавиш"
"Key names"
"Názvy kláves"
"Tastennamen"
"Funkció neve"
"Nazwy klawiszy"
"Nombres teclas"
"Назви клавіш"

MSetColorKeyBarBackground
"Фон"
"Background"
"Pozadí"
"Hintergrund"
"Háttere"
"Tło"
"Fondo"
"Фон"

MSetColorCommandLineNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

MSetColorCommandLineSelected
"Выделенный текст"
"Selected text input"
"Vybraný textový vstup"
"Markierte Texteingabe"
"Beírt szöveg kijelölve"
"Zaznaczony wpisany tekst"
"Entrada de texto seleccionada"
"Виділелений текст"

MSetColorCommandLinePrefix
"Текст префикса"
"Prefix text"
"Text předpony"
"Prefix Text"
"Előtag szövege"
"Tekst prefiksu"
"Texto prefijado"
"Текст префіксу"

MSetColorCommandLineUserScreen
"Пользовательский экран"
"User screen"
"Obrazovka uživatele"
"Benutzerseite"
"Konzol háttere"
"Ekran użytkownika"
"Pantalla de usuario"
"Користувацький екран"

MSetColorClockNormal
l:
"Обычный текст (панели)"
"Normal text (Panel)"
"Normální text (Panel)"
"Normaler Text (Panel)"
"Normál szöveg (panelek)"
"Normalny tekst (Panel)"
"Texto normal (Panel)"
"Звичайний текст (панелі)"

MSetColorClockNormalEditor
"Обычный текст (редактор)"
"Normal text (Editor)"
"Normální text (Editor)"
"Normaler Text (Editor)"
"Normál szöveg (szerkesztő)"
"Normalny tekst (Edytor)"
"Texto normal (Editor)"
"Звичайний текст (редактор)"

MSetColorClockNormalViewer
"Обычный текст (вьювер)"
"Normal text (Viewer)"
"Normální text (Prohlížeč)"
"Normaler Text (Betrachter)"
"Normál szöveg (nézőke)"
"Normalny tekst (Podgląd)"
"Texto normal (Visor)"
"Звичайний текст (в'ювер)"

MSetColorViewerNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

MSetColorViewerSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Zaznaczony tekst"
"Texto seleccionado"
"Вибраний текст"

MSetColorViewerStatus
"Статус"
"Status line"
"Stavový řádek"
"Statuszeile"
"Állapotsor"
"Linia statusu"
"Línea de estado"
"Статус"

MSetColorViewerArrows
"Стрелки сдвига экрана"
"Screen scrolling arrows"
"Skrolovací šipky"
"Pfeile auf Scrollbalken"
"Képernyőgördítő nyilak"
"Strzałki przesuwające ekran"
"Flechas desplazamiento de pantalla"
"Стрілки зсуву екрана"

MSetColorViewerScrollbar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barras desplazamiento"
"Полоса прокрутки"

MSetColorEditorNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

MSetColorEditorSelected
"Выбранный текст"
"Selected text"
"Vybraný text"
"Markierter Text"
"Kijelölt szöveg"
"Zaznaczony tekst"
"Texto seleccionado"
"Вибраний текст"

MSetColorEditorStatus
"Статус"
"Status line"
"Stavový řádek"
"Statuszeile"
"Állapotsor"
"Linia statusu"
"Línea de estado"
"Статус"

MSetColorEditorScrollbar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra de desplazamiento"
"Полоса прокрутки"

MSetColorHelpNormal
l:
"Обычный текст"
"Normal text"
"Normální text"
"Normaler Text"
"Normál szöveg"
"Normalny tekst"
"Texto normal"
"Звичайний текст"

MSetColorHelpHighlighted
"Выделенный текст"
"Highlighted text"
"Zvýrazněný text"
"Markierung"
"Kiemelt szöveg"
"Podświetlony tekst"
"Texto resaltado"
"Виділений текст"

MSetColorHelpReference
"Ссылка"
"Reference"
"Odkaz"
"Referenz"
"Hivatkozás"
"Odniesienie"
"Referencia"
"Посилання"

MSetColorHelpSelectedReference
"Выбранная ссылка"
"Selected reference"
"Vybraný odkaz"
"Ausgewählte Referenz"
"Kijelölt hivatkozás"
"Wybrane odniesienie "
"Referencia seleccionada"
"Вибране посилання"

MSetColorHelpBox
"Рамка"
"Border"
"Okraj"
"Rahmen"
"Keret"
"Ramka"
"Borde"
"Рамка"

MSetColorHelpBoxTitle
"Заголовок рамки"
"Title"
"Nadpis"
"Titel"
"Keret neve"
"Tytuł"
"Título"
"Заголовок рамки"

MSetColorHelpScrollbar
"Полоса прокрутки"
"Scrollbar"
"Posuvník"
"Scrollbalken"
"Gördítősáv"
"Suwak"
"Barra desplazamiento"
"Полоса прокрутки"

MSetColorGroupsTitle
l:
"Цветовые группы"
"Color groups"
"Skupiny barev"
"Farbgruppen"
"Színcsoportok"
"Grupy kolorów"
"Grupos de colores"
"Кольорові групи"

MSetColorItemsTitle
"Элементы группы"
"Group items"
"Položky skupin"
"Gruppeneinträge"
"A színcsoport elemei"
"Elementy grupy"
"Grupos de ítems"
"Елементи групи"

MSetColorTitle
l:
"Цвет"
"Color"
"Barva"
"Farbe"
"Színek"
"Kolor"
"Color"
"Колір"

MSetColorForeground
"&Текст"
"&Foreground"
"&Popředí"
"&Vordergrund"
"&Előtér"
"&Pierwszy plan"
"&Caracteres"
"&Текст"

MSetColorBackground
"&Фон"
"&Background"
"Po&zadí"
"&Hintergrund"
"&Háttér"
"&Tło"
"&Fondo     "
"&Фон"

MSetColorForeTransparent
"&Прозрачный"
"&Transparent"
"Průhlednos&t"
"&Transparent"
"Átlá&tszó"
"P&rzezroczyste"
"&Transparente"
"&Прозорий"

MSetColorBackTransparent
"П&розрачный"
"T&ransparent"
"Průhledno&st"
"T&ransparent"
"Átlát&szó"
"Pr&zezroczyste"
"T&ransparente"
"П&розорий"

MSetColorSample
"Текст Текст Текст Текст Текст Текст"
"Text Text Text Text Text Text Text"
"Text Text Text Text Text Text Text"
"Text Text Text Text Text Text Text"
"Text Text Text Text Text Text Text"
"Tekst Tekst Tekst Tekst Tekst Tekst"
"Texto Texto Texto Texto Texto"
"Текст Текст Текст Текст Текст Текст"

MSetColorSet
"Установить"
"Set"
"Nastavit"
"Setzen"
"A&lkalmaz"
"Ustaw"
"Poner"
"Встановити"

MSetColorCancel
"Отменить"
"Cancel"
"Storno"
"Abbruch"
"&Mégsem"
"Anuluj"
"Cancelar"
"Скасувати"

MSetConfirmTitle
l:
"Подтверждения"
"Confirmations"
"Potvrzení"
"Bestätigungen"
"Megerősítések"
"Potwierdzenia"
"Confirmaciones"
"Підтвердження"

MSetConfirmCopy
"Перезапись файлов при &копировании"
"&Copy"
"&Kopírování"
"&Kopieren"
"&Másolás"
"&Kopiowanie"
"&Copiar"
"Перезаписування файлів під час &копіювання"

MSetConfirmMove
"Перезапись файлов при &переносе"
"&Move"
"&Přesouvání"
"&Verschieben"
"Moz&gatás"
"&Przenoszenie"
"&Mover"
"Перезаписування файлів під час &перенесення"

MSetConfirmRO
"Перезапись и удаление R/O &файлов"
"&Overwrite and delete R/O files"
upd:"&Overwrite and delete R/O files"
upd:"&Overwrite and delete R/O files"
"&Csak olv. fájlok felülírása/törlése"
upd:"&Overwrite and delete R/O files"
"S&obrescribir y eliminar ficheros Sólo/Lectura"
"Перезапис та видалення R/O файлів"

MSetConfirmDrag
"Пере&таскивание"
"&Drag and drop"
"&Drag and drop"
"&Ziehen und Ablegen"
"&Húzd és ejtsd"
"P&rzeciąganie i upuszczanie"
"&Arrastrar y soltar"
"Пере&тягання"

MSetConfirmDelete
"&Удаление"
"De&lete"
"&Mazání"
"&Löschen"
"&Törlés"
"&Usuwanie"
"&Borrar"
"&Видалення"

MSetConfirmDeleteFolders
"У&даление непустых папок"
"Delete non-empty &folders"
"Mazat &neprázdné adresáře"
"Löschen von Ordnern mit &Inhalt"
"Nem &üres mappák törlése"
"Usuwanie &niepustych katalogów"
"Borrar &directorios no-vacíos"
"Ви&далення непорожніх тек"

MSetConfirmEsc
"Прерыва&ние операций"
"&Interrupt operation"
"Pře&rušit operaci"
"&Unterbrechen von Vorgängen"
"Mű&velet megszakítása"
"&Przerwanie operacji"
"&Interrumpir operación"
"Перерива&ння операцій"

MSetConfirmRemoveConnection
"&Отключение сетевого устройства"
"Disconnect &network drive"
"Odpojení &síťové jednotky"
"Trennen von &Netzwerklaufwerken"
"Háló&zati meghajtó leválasztása"
"Odłączenie dysku &sieciowego"
"Desconectar u&nidad de red"
"&Відключення мережного пристрою"

MSetConfirmRemoveSUBST
"Отключение SUBST-диска"
"Disconnect &SUBST-disk"
"Odpojení SUBST-d&isku"
"Trennen von &Substlaufwerken"
"Virt&uális meghajtó törlése"
"Odłączenie dysku &SUBST"
"Desconectar disco &sustituido"
"Відключення SUBST-диска"

MSetConfirmDetachVHD
"Отсоедиение виртуального диска"
"Detach virtual disk"
upd:"Detach virtual disk"
upd:"Detach virtual disk"
upd:"Detach virtual disk"
upd:"Detach virtual disk"
upd:"Detach virtual disk"
"Від'єднання віртуального диска"

MSetConfirmRemoveHotPlug
"Отключение HotPlug-у&стройства"
"Hot&Plug-device removal"
"Odpojení vyjímatelného zařízení"
"Sicheres Entfernen von Hardware"
"H&otPlug eszköz eltávolítása"
"Odłączanie urządzenia HotPlug"
"Remover dispositivo de conexión"
"Вимкнення HotPlug-п&ристроя"

MSetConfirmAllowReedit
"Повто&рное открытие файла в редакторе"
"&Reload edited file"
"&Obnovit upravovaný soubor"
"Bea&rbeitete Datei neu laden"
"&Szerkesztett fájl újratöltése"
"&Załaduj edytowany plik"
"&Recargar archivo editado"
"Повто&рне відкриття файлу в редакторі"

MSetConfirmHistoryClear
"Очистка списка &истории"
"Clear &history list"
"Vymazat seznam &historie"
"&Historielisten löschen"
"&Előzménylista törlése"
"Czyszczenie &historii"
"Limpiar listado de &historial"
"Очищення списку &історії"

MSetConfirmExit
"&Выход"
"E&xit"
"U&končení"
"Be&enden"
"K&ilépés a FAR-ból"
"&Wyjście"
"&Salir"
"&Виход"

MPluginsManagerSettingsTitle
l:
"Параметры менеджера внешних модулей"
"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
upd:"Plugins manager settings"
"Параметри менеджера зовнішніх модулів"

MPluginsManagerOEMPluginsSupport
"Поддержка OEM-плагинов"
"OEM plugins support"
upd:"OEM plugins support"
upd:"OEM plugins support"
upd:"OEM plugins support"
upd:"OEM plugins support"
upd:"OEM plugins support"
"Підтримка OEM-плагінів"

MPluginsManagerScanSymlinks
"Ск&анировать символические ссылки"
"Scan s&ymbolic links"
"Prohledávat s&ymbolické linky"
"S&ymbolische Links scannen"
"Szimbolikus linkek &vizsgálata"
"Skanuj linki s&ymboliczne"
"Explorar enlaces simbólicos"
"Ск?анувати символічні посилання"

MPluginsManagerPersonalPath
"Путь к персональным п&лагинам:"
"&Path for personal plugins:"
"&Cesta k vlastním pluginům:"
"&Pfad für eigene Plugins:"
"Saját plu&ginek útvonala:"
"Ś&cieżka do własnych pluginów:"
"Ruta para pl&ugins personales:
"Шлях до персональних п&лагінів:"

MPluginsManagerOFP
"Обработка &файла (OpenFilePlugin)"
"&File processing (OpenFilePlugin)"
upd:"&File processing (OpenFilePlugin)"
upd:"&File processing (OpenFilePlugin)"
"&Fájl feldolgozása (OpenFilePlugin)"
upd:"&File processing (OpenFilePlugin)"
"Proceso de archivo (OpenFilePlugin)"
"Обробка &файлу (OpenFilePlugin)"

MPluginsManagerStdAssoc
"Пункт вызова стандартной &ассоциации"
"Show standard &association item"
upd:"Show standard &association item"
upd:"Show standard &association item"
"Szabvány társítás megjelenítése"
upd:"Show standard &association item"
"Mostrar asociaciones normales de ítems"
"Пункт виклику стандартної &асоціації"

MPluginsManagerEvenOne
"Даже если найден всего &один плагин"
"Even if only &one plugin found"
upd:"Even if only &one plugin found"
upd:"Even if only &one plugin found"
"Akkor is, ha csak egy plugin van"
upd:"Even if only &one plugin found"
"Aún si solo se encontr un plugin"
"Навіть якщо знайдено всього &один плагін"

MPluginsManagerSFL
"&Результаты поиска (SetFindList)"
"Search &results (SetFindList)"
upd:"Search &results (SetFindList)"
upd:"Search &results (SetFindList)"
"Keresés eredménye (SetFindList)"
upd:"Search &results (SetFindList)"
"Resultados de búsqueda (SetFindList)"
"&Результати пошуку (SetFindList)"

MPluginsManagerPF
"Обработка &префикса"
"&Prefix processing"
upd:"&Prefix processing"
upd:"&Prefix processing"
"Előtag feldolgozása"
upd:"&Prefix processing"
"Proceso de prefijo"
"Обробка &префіксу"

MPluginConfirmationTitle
"Выбор плагина"
"Plugin selection"
upd:"Plugin selection"
upd:"Plugin selection"
"Plugin választás"
upd:"Plugin selection"
"Selección de plugin"
"Вибір плагіна"

MMenuPluginStdAssociation
"Стандартная ассоциация"
"Standard association"
upd:"Standard association"
upd:"Standard association"
"Szabvány társítás"
upd:"Standard association"
"Asociación normal"
"Стандартна асоціація"

MFindFolderTitle
l:
"Поиск папки"
"Find folder"
"Najít adresář"
"Ordner finden"
"Mappakeresés"
"Znajdź folder"
"Encontrar directorio"
"Пошук теки"

MKBFolderTreeF1
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

MKBFolderTreeF2
"Обновить"
"Rescan"
"Obnovit"
"Aktual"
"FaFris"
"Czytaj ponownie"
"ReExpl"
"Оновити"

MKBFolderTreeF5
"Размер"
"Zoom"
"Zoom"
"Vergr."
"Nagyít"
"Powiększ"
"Zoom"
"Розмір"

MKBFolderTreeF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

MKBFolderTreeAltF9
"Видео"
"Video"
"Video"
"Vollb"
"Video"
"Video"
"Video"
"Відео"

MTreeTitle
"Дерево"
"Tree"
"Stromové zobrazení"
"Baum"
"Fa"
"Drzewo"
"Arbol"
"Дерево"

MCannotSaveTree
"Ошибка записи дерева папок в файл"
"Cannot save folders tree to file"
"Adresářový strom nelze uložit do souboru"
"Konnte Ordnerliste nicht in Datei speichern."
"A mappák fastruktúrája nem menthető fájlba"
"Nie mogę zapisać drzewa katalogów do pliku"
"No se puede guardar árbol de directorios al archivo"
"Помилка запису дерева папок у файл"

MReadingTree
"Чтение дерева папок"
"Reading the folders tree"
"Načítám adresářový strom"
"Lese Ordnerliste"
"Mappaszerkezet újraolvasása..."
"Odczytuję drzewo katalogów"
"Leyendo árbol de directorios"
"Читання дерева папок"

MUserMenuTitle
l:
"Пользовательское меню"
"User menu"
"Menu uživatele"
"Benutzermenü"
"Felhasználói menü szerkesztése"
"Menu użytkownika"
"Menú de usuario"
"Користувачське меню"

MChooseMenuType
"Выберите тип пользовательского меню для редактирования"
"Choose user menu type to edit"
"Zvol typ menu uživatele pro úpravu"
"Wählen Sie den Typ des zu editierenden Benutzermenüs"
"Felhasználói menü típusa:"
"Wybierz typ menu do edycji"
"Elija tipo de menú usuario a editar"
"Виберіть тип меню користувача для редагування"

MChooseMenuMain
"&Главное"
"&Main"
"&Hlavní"
"&Hauptmenü"
"&Főmenü"
"Główne"
"&Principal"
"&Головне"

MChooseMenuLocal
"&Местное"
"&Local"
"&Lokální"
"&Lokales Menü"
"&Helyi menü"
"Lokalne"
"&Local"
"&Місцеве"

MMainMenuTitle
"Главное меню"
"Main menu"
"Hlavní menu"
"Hauptmenü"
"Főmenü"
"Menu główne"
"Menú principal"
"Головне меню"

MMainMenuFAR
"Папка FAR"
"FAR folder"
"Složka FARu"
"FAR Ordner"
"FAR mappa"
"Folder FAR-a"
"Directorio FAR"
"Тека FAR"

MMainMenuREG
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

MLocalMenuTitle
"Местное меню"
"Local menu"
"Lokalní menu"
"Lokales Menü"
"Helyi menü"
"Menu lokalne"
"Menú local"
"Місцеве меню"

MMainMenuBottomTitle
"Редактирование: Del,Ins,F4,Ctrl-F4"
"Edit: Del,Ins,F4,Ctrl-F4"
"Edit: Del,Ins,F4,Ctrl-F4"
"Bearb.: Entf,Einf,F4,Ctrl-F4"
"Szerk.: Del,Ins,F4,Ctrl-F4"
"Edycja: Del,Ins,F4,Ctrl-F4"
"Editar: Del,Ins,F4"
"Редагування: Del,Ins,F4,Ctrl-F4"

MAskDeleteMenuItem
"Вы хотите удалить пункт меню"
"Do you wish to delete the menu item"
"Přejete si smazat položku v menu"
"Do you wish to delete the menu item"
"Biztosan törli a menüelemet?"
"Czy usunąć pozycję menu"
"Desea borrar el ítem del menú"
"Ви хочете видалити пункт меню"

MAskDeleteSubMenuItem
"Вы хотите удалить вложенное меню"
"Do you wish to delete the submenu"
"Přejete si smazat podmenu"
"Do you wish to delete the submenu"
"Biztosan törli az almenüt?"
"Czy usunąć podmenu"
"Desea borrar el submenú"
"Ви хочете видалити вкладене меню"

MUserMenuInvalidInputLabel
"Неправильный формат метки меню"
"Invalid format for UserMenu Label"
"Neplatný formát pro název Uživatelského menu"
"Invalid format for UserMenu Label"
"A felhasználói menü névformátuma érvénytelen"
"Błędny format etykiety menu użytkownika"
"Formato inválido para etiqueta de menú usuario"
"Неправильний формат мітки меню"

MUserMenuInvalidInputHotKey
"Неправильный формат горячей клавиши"
"Invalid format for Hot Key"
"Neplatný formát pro klávesovou zkratku"
"Invalid format for Hot Key"
"A gyorsbillentyű formátuma érvénytelen"
"Błędny format klawisza skrótu"
"Formato inválido para tecla rápida"
"Неправильний формат гарячої клавіші"

MEditMenuTitle
l:
"Редактирование пользовательского меню"
"Edit user menu"
"Editace uživatelského menu"
"Menübefehl bearbeiten"
"Parancs szerkesztése"
"Edytuj menu użytkownika"
"Editar menú de usuario"
"Редагування меню користувача"

MEditMenuHotKey
"&Горячая клавиша:"
"&Hot key:"
"K&lávesová zkratka:"
"&Kurztaste:"
"&Gyorsbillentyű:"
"&Klawisz skrótu:"
"&Tecla rápida:"
"&Гаряча клавіша:"

MEditMenuLabel
"&Метка:"
"&Label:"
"&Popisek:"
"&Bezeichnung:"
"&Név:"
"&Etykieta:"
"&Etiqueta:"
"&Мітка:"

MEditMenuCommands
"&Команды:"
"&Commands:"
"Pří&kazy:"
"&Befehle:"
"&Parancsok:"
"&Polecenia:"
"&Comandos:"
"&Команди:"

MAskInsertMenuOrCommand
l:
"Вы хотите вставить новую команду или новое меню?"
"Do you wish to insert a new command or a new menu?"
"Přejete si vložit nový příkaz nebo nové menu?"
"Wollen Sie einen neuen Menübefehl oder ein neues Menu erstellen?"
"Új parancs vagy új menü?"
"Czy chcesz wstawić nowe polecenie lub nowe menu?"
"Desea insertar un nuevo comando o un nuevo menú?"
"Ви хочете вставити нову команду або нове меню?"

MMenuInsertCommand
"Вставить команду"
"Insert command"
"Vložit příkaz"
"Neuer Befehl"
"Parancs"
"Wstaw polecenie"
"Insertar comando"
"Вставити команду"

MMenuInsertMenu
"Вставить меню"
"Insert menu"
"Vložit menu"
"Neues Menü"
"Menü"
"Wstaw menu"
"Insertar menú"
"Вставити меню"

MEditSubmenuTitle
l:
"Редактирование метки вложенного меню"
"Edit submenu label"
"Úprava popisku podmenu"
"Untermenü bearbeiten"
"Almenü szerkesztése"
"Edytuj etykietę podmenu"
"Editar etiqueta de submenú"
"Редагування позначки вкладеного меню"

MEditSubmenuHotKey
"&Горячая клавиша:"
"&Hot key:"
"Klávesová &zkratka:"
"&Kurztaste:"
"&Gyorsbillentyű:"
"&Klawisz skrótu:"
"&Tecla rápida:"
"&Гаряча клавіша:"

MEditSubmenuLabel
"&Метка:"
"&Label:"
"&Popisek:"
"&Bezeichnung:"
"&Név:"
"&Etykieta:"
"&Etiqueta:"
"&Мітка:"

MViewerTitle
l:
"Просмотр"
"Viewer"
"Prohlížeč"
"Betrachter"
"Nézőke"
"Podgląd"
"Visor"
"Перегляд"

MViewerCannotOpenFile
"Ошибка открытия файла"
"Cannot open the file"
"Nelze otevřít soubor"
"Kann Datei nicht öffnen"
"A fájl nem nyitható meg"
"Nie mogę otworzyć pliku"
"No se puede abrir el archivo"
"Помилка відкриття файлу"

MViewerStatusCol
"Кол"
"Col"
"Sloupec"
"Spalte"
"Oszlop"
"Kolumna"
"Col"
"Кол"

MViewSearchTitle
l:
"Поиск"
"Search"
"Hledat"
"Durchsuchen"
"Keresés"
"Szukaj"
"Buscar"
"Пошук"

MViewSearchFor
"&Искать"
"&Search for"
"H&ledat"
"&Suchen nach"
"&Keresés:"
"&Znajdź"
"&Buscar por"
"&Шукати"

MViewSearchForText
"Искать &текст"
"Search for &text"
"Hledat &text"
"Suchen nach &Text"
"&Szöveg keresése"
"Szukaj &tekstu"
"Buscar cadena de &texto"
"Шукати &текст"

MViewSearchForHex
"Искать 16-ричный &код"
"Search for &hex"
"Hledat he&x"
"Suchen nach &Hex (xx xx ...)"
"&Hexa keresése"
"Szukaj &wartości szesnastkowych"
"Buscar cadena &hexadecimal"
"Шукати 16-річний &код"

MViewSearchCase
"&Учитывать регистр"
"&Case sensitive"
"&Rozlišovat velikost písmen"
"Gr&oß-/Kleinschreibung"
"&Nagy/kisbetű érzékeny"
"&Uwzględnij wielkość liter"
"Sensible min/ma&yúsculas"
"&Враховувати регістр"

MViewSearchWholeWords
"Только &целые слова"
"&Whole words"
"Celá &slova"
"Ganze &Wörter"
"Csak e&gész szavak"
"Tylko całe słowa"
"&Palabras completas"
"Тільки &цілі слова"

MViewSearchReverse
"Обратн&ый поиск"
"Re&verse search"
"&Zpětné hledání"
"Richtung um&kehren"
"&Visszafelé keres"
"Szukaj w &odwrotnym kierunku"
"Buscar al in&verso"
"Зворотн&ий пошук"

MViewSearchRegexp
"&Регулярные выражения"
"&Regular expressions"
upd:"&Regular expressions"
upd:"&Regular expressions"
upd:"&Regular expressions"
upd:"&Regular expressions"
"Expresiones &regulares"
"&Регулярні вирази"

MViewSearchSearch
"Искать"
"Search"
"Hledat"
"Suchen"
"Keres"
"&Szukaj"
"Buscar"
"Шукать"

MViewSearchCancel
"Отменить"
"Cancel"
"Storno"
"Abbrechen"
"Mégsem"
"&Anuluj"
"Cancelar"
"Скасувати"

MViewSearchingFor
l:
"Поиск"
"Searching for"
"Vyhledávám"
"Suche nach"
"Keresés:"
"Szukam"
"Buscando por"
"Пошук"

MViewSearchingHex
"Поиск байтов"
"Searching for bytes"
"Vyhledávám sekvenci bytů"
"Suche nach Bytes"
"Bájtok keresése:"
"Szukam bajtów"
"Buscando por bytes"
"Пошук байтів"

MViewSearchCannotFind
"Строка не найдена"
"Could not find the string"
"Nelze najít řetězec"
"Konnte Zeichenkette nicht finden"
"Nem találtam a szöveget:"
"Nie mogę odnaleźć ciągu znaków"
"No se puede encontrar la cadena"
"Строка не знайдена"

MViewSearchCannotFindHex
"Байты не найдены"
"Could not find the bytes"
"Nelze najít sekvenci bytů"
"Konnte Bytefolge nicht finden"
"Nem találtam a bájtokat:"
"Nie mogę odnaleźć bajtów"
"No se puede encontrar los bytes"
"Байти не знайдені"

MViewSearchFromBegin
"Продолжить поиск с начала документа?"
"Continue the search from the beginning of the document?"
"Pokračovat s hledáním od začátku dokumentu?"
"Mit Suche am Anfang des Dokuments fortfahren?"
"Folytassam a keresést a dokumentum elejétől?"
"Kontynuować wyszukiwanie od początku dokumentu?"
"Continuar búsqueda desde el comienzo del documento"
"Продовжити пошук з початку документа?"

MViewSearchFromEnd
"Продолжить поиск с конца документа?"
"Continue the search from the end of the document?"
"Pokračovat s hledáním od konce dokumentu?"
"Mit Suche am Ende des Dokuments fortfahren?"
"Folytassam a keresést a dokumentum végétől?"
"Kontynuować wyszukiwanie od końca dokumentu?"
"Continuar búsqueda desde el final del documento"
"Продовжити пошук з кінця документа?"

MPrintTitle
l:
"Печать"
"Print"
"Tisk"
"Drucken"
"Nyomtatás"
"Drukuj"
"Imprimir"
"Друк"

MPrintTo
"Печатать %ls на"
"Print %ls to"
"Vytisknout %ls na"
"Drucke %ls nach"
"%ls nyomtatása:"
"Drukuj %ls do"
"Imprimir %ls a"
"Друкувати %ls на"

MPrintFilesTo
"Печатать %d файлов на"
"Print %d files to"
"Vytisknout %d souborů na"
"Drucke %d Dateien mit"
"%d fájl nyomtatása:"
"Drukuj %d pliki(ów) do"
"Imprimir %d archivos a"
"Друкувати %d файлів на"

MPreparingForPrinting
"Подготовка файлов к печати"
"Preparing files for printing"
"Připravuji soubory pro tisk"
"Vorbereiten der Druckaufträge"
"Fájlok előkészítése nyomtatáshoz"
"Przygotowuję plik(i) do drukowania"
"Preparando archivos para imprimir"
"Підготовка файлів до друку"

MCannotEnumeratePrinters
"Не удалось получить список доступных принтеров"
"Cannot enumerate available printers list"
upd:"Cannot enumerate available printers list"
upd:"Cannot enumerate available printers list"
"Az elérhető nyomtatók listája nem állítható össze"
upd:"Cannot enumerate available printers list"
"trabajos"
"Не вдалося отримати список доступних принтерів"

MCannotOpenPrinter
"Не удалось открыть принтер"
"Cannot open printer"
"Nelze otevřít tiskárnu"
"Fehler beim öffnen des Druckers"
"Nyomtató nem elérhető"
"Nie mogę połączyć się z drukarką"
"No se puede abrir impresora"
"Не вдалося відкрити принтер"

MCannotPrint
"Не удалось распечатать"
"Cannot print"
"Nelze tisknout"
"Fehler beim Drucken"
"Nem nyomtatható"
"Nie mogę drukować"
"No se puede imprimir"
"Не вдалося роздрукувати"

MDescribeFiles
l:
"Описание файла"
"Describe file"
"Popiskový soubor"
"Beschreibung ändern"
"Fájlmegjegyzés"
"Opisz plik"
"Describir archivos"
"Опис файлу"

MEnterDescription
"Введите описание для"
"Enter description for"
"Zadejte popisek"
"Beschreibung für"
upd:"Írja be megjegyzését:"
"Wprowadź opis"
"Entrar descripción de %ls"
"Введіть опис для"

MReadingDiz
l:
"Чтение описаний файлов"
"Reading file descriptions"
"Načítám popisky souboru"
"Lese Dateibeschreibungen"
"Fájlmegjegyzések olvasása"
"Odczytuję opisy plików"
"Leyendo descripción de archivos"
"Читання описів файлів"

MCannotUpdateDiz
"Не удалось обновить описания файлов"
"Cannot update file descriptions"
"Nelze aktualizovat popisky souboru"
"Dateibeschreibungen konnten nicht aktualisiert werden."
"A fájlmegjegyzések nem frissíthetők"
"Nie moge aktualizować opisów plików"
"No se puede actualizar descripción de archivos"
"Не вдалося оновити опис файлів"

MCannotUpdateRODiz
"Файл описаний защищён от записи"
"The description file is read only"
"Popiskový soubor má atribut Jen pro čtení"
"Die Beschreibungsdatei ist schreibgeschützt."
"A megjegyzésfájl csak olvasható"
"Opis jest plikiem tylko do odczytu"
"El archivo descripción es de sólo lectura"
"Файл опису захищений від запису"

MCfgDizTitle
l:
"Описания файлов"
"File descriptions"
"Popisky souboru"
"Dateibeschreibungen"
"Fájl megjegyzésfájlok"
"Opisy plików"
"Descripción de archivos"
"Опис файлів"

MCfgDizListNames
"Имена &списков описаний, разделённые запятыми:"
"Description &list names delimited with commas:"
"Seznam pop&isových souborů oddělených čárkami:"
"Beschreibungs&dateien, getrennt durch Komma:"
"Megjegyzés&fájlok nevei, vesszővel elválasztva:"
"Nazwy &plików z opisami oddzielone przecinkami:"
"Nombres de &listas de descripción delimitado con comas:"
"Імена &списків описів, розділені комами:"

MCfgDizSetHidden
"Устанавливать &атрибут ""Скрытый"" на новые списки описаний"
"Set ""&Hidden"" attribute to new description lists"
"Novým souborům s popisy nastavit atribut ""&Skrytý"""
"Setze das '&Versteckt'-Attribut für neu angelegte Dateien"
"Az új megjegyzésfájl ""&rejtett"" attribútumú legyen"
"Ustaw atrybut ""&Ukryty"" dla nowych plików z opisami"
"Poner atributo ""&Oculto"" a las nuevas listas de descripción"
"Встановлювати &атрибут ""Прихований"" на нові списки описів"

MCfgDizROUpdate
"Обновлять файл описаний с атрибутом ""Толь&ко для чтения"""
"Update &read only description file"
"Aktualizovat popisové soubory s atributem Jen pro čtení"
"Schreibgeschützte Dateien aktualisie&ren"
"&Csak olvasható megjegyzésfájlok frissítése"
"Aktualizuj plik opisu tylko do odczytu"
"Actualizar archivo descripción de sólo lectura"
"Оновлювати файл опису з атрибутом ""Тіль&ки для читання"""

MCfgDizStartPos
"&Позиция новых описаний в строке"
"&Position of new descriptions in the string"
"&Pozice nových popisů v řetězci"
"&Position neuer Beschreibungen in der Zeichenkette"
"Új megjegyzéseknél a szöveg &kezdete"
"Pozy&cja nowych opisów w linii"
"&Posición de nueva descripciones en la cadena"
"&Позиція нових описів у рядку"

MCfgDizNotUpdate
"&Не обновлять описания"
"Do &not update descriptions"
"&Neaktualizovat popisy"
"Beschreibungen &nie aktualisieren"
"N&e frissítse a megjegyzéseket"
"&Nie aktualizuj opisów"
"&No actualizar descripciones"
"&Не оновлювати описи"

MCfgDizUpdateIfDisplayed
"&Обновлять, если они выводятся на экран"
"Update if &displayed"
"Aktualizovat, jestliže je &zobrazen"
"Aktualisieren &wenn angezeigt"
"Frissítsen, ha meg&jelenik"
"Aktualizuj jeśli &widoczne"
"Actualizar si es visualiza&do"
"&Оновлювати, якщо вони відображаються на екрані"

MCfgDizAlwaysUpdate
"&Всегда обновлять"
"&Always update"
"&Vždy aktualizovat"
"Im&mer aktualisieren"
"&Mindig frissítsen"
"&Zawsze aktualizuj"
"&Actualizar siempre"
"&Завжди оновлювати"

MCfgDizAnsiByDefault
"&Использовать кодовую страницу ANSI по умолчанию"
"Use ANS&I code page by default"
upd:"Automaticky otevírat soubory ve &WIN kódování"
upd:"Dateien standardmäßig mit Windows-Kod&ierung öffnen"
"Fájlok eredeti megnyitása ANS&I kódlappal"
"&Otwieraj pliki w kodowaniu Windows"
"Usar código ANS&I por defecto"
"&Використовувати кодову сторінку ANSI за замовчуванням"

MCfgDizSaveInUTF
"Сохранять в UTF8"
"Save in UTF8"
upd:"Save in UTF8"
upd:"Save in UTF8"
upd:"Save in UTF8"
upd:"Save in UTF8"
"Guardar en UTF8"
"Зберігати в UTF8"

MReadingTitleFiles
l:
"Обновление панелей"
"Update of panels"
"Aktualizace panelů"
"Aktualisiere Panels"
"Panelek frissítése"
"Aktualizacja panelu"
"Actualizar paneles"
"Оновлення панелей"

MReadingFiles
"Чтение: %d файлов"
"Reading: %d files"
"Načítám: %d souborů"
"Lese: %d Dateien"
" %d fájl olvasása"
"Czytam: %d plików"
"Leyendo: %d archivos"
"Читання: %d файлів"

MOperationNotCompleted
"Операция не завершена"
"Operation not completed"
"Operace není dokončena"
"Vorgang nicht abgeschlossen"
"A művelet félbeszakadt"
"Operacja nie doprowadzona do końca"
"Operación no completada"
"Операція не завершена"

MEditPanelModes
l:
"Режимы панели"
"Edit panel modes"
"Editovat módy panelu"
"Anzeigemodi von Panels bearbeiten"
"Panel módok szerkesztése"
"Edytuj tryby wyświetlania paneli"
"Editar modo de paneles"
"Режими панелі"

MEditPanelModesBrief
l:
"&Краткий режим"
"&Brief mode"
"&Stručný mód"
"&Kurz"
"&Rövid mód"
"&Skrótowy"
"&Breve     "
"&Короткий режим"

MEditPanelModesMedium
"&Средний режим"
"&Medium mode"
"S&třední mód"
"&Mittel"
"&Közepes mód"
"Ś&redni"
"&Medio      "
"&Середний режим"

MEditPanelModesFull
"&Полный режим"
"&Full mode"
"&Plný mód"
"&Voll"
"&Teljes mód"
"&Pełny"
"&Completo "
"&Повний режим"

MEditPanelModesWide
"&Широкий режим"
"&Wide mode"
"Š&iroký mód"
"B&reitformat"
"&Széles mód"
"S&zeroki"
"&Amplio   "
"&Широкий режим"

MEditPanelModesDetailed
"&Детальный режим"
"Detai&led mode"
"Detai&lní mód"
"Detai&lliert"
"Rés&zletes mód"
"Ze sz&czegółami"
"Detal&lado    "
"&Детальний режим"

MEditPanelModesDiz
"&Описания"
"&Descriptions mode"
"P&opiskový mód"
"&Beschreibungen"
"&Fájlmegjegyzés mód"
"&Opisy"
"&Descripción      "
"&Описи"

MEditPanelModesLongDiz
"Д&линные описания"
"Lon&g descriptions mode"
"&Mód dlouhých popisků"
"Lan&ge Beschreibungen"
"&Hosszú megjegyzés mód"
"&Długie opisy"
"Descripción lar&ga"
"Д&овгі описи"

MEditPanelModesOwners
"Вл&адельцы файлов"
"File own&ers mode"
"Mód vlastníka so&uborů"
"B&esitzer"
"T&ulajdonos mód"
"&Właściciele"
"Du&eños de archivos"
"Вл&асники файлів"

MEditPanelModesLinks
"Свя&зи файлов"
"Lin&ks mode"
"Lin&kový mód"
"Dateilin&ks"
"Li&nkek mód"
"Dowiąza&nia"
"En&laces    "
"Зв'я&зки файлів"

MEditPanelModesAlternative
"Аль&тернативный полный режим"
"&Alternative full mode"
"&Alternativní plný mód"
"&Alternative Vollansicht"
"&Alternatív teljes mód"
"&Alternatywny"
"Alternativo com&pleto "
"Аль&тернативний повний режим"

MEditPanelModeTypes
l:
"&Типы колонок"
"Column &types"
"&Typ sloupců"
"Spalten&typen"
"Oszlop&típusok"
"&Typy kolumn"
"&Tipos de columna"
"&Типи колонок"

MEditPanelModeWidths
"&Ширина колонок"
"Column &widths"
"Šíř&ka sloupců"
"Spalten&breiten"
"Oszlop&szélességek"
"&Szerokości kolumn"
"Anc&ho de columna"
"&Ширина колонок"

MEditPanelModeStatusTypes
"Типы колонок строки ст&атуса"
"St&atus line column types"
"T&yp sloupců stavového řádku"
"St&atuszeile Spaltentypen"
"Állapotsor oszloptíp&usok"
"Typy kolumn &linii statusu"
"Tipos de columnas líne&a de estado"
"Типи колонок рядка ст&атусу"

MEditPanelModeStatusWidths
"Ширина колонок строки стат&уса"
"Status l&ine column widths"
"Šířka slo&upců stavového řádku"
"Statusze&ile Spaltenbreiten"
"Állapotsor &oszlopszélességek"
"Szerokości kolumn l&inii statusu"
"Ancho de columnas lí&nea de estado"
"Ширина колонок рядка стат&усу"

MEditPanelModeFullscreen
"&Полноэкранный режим"
"&Fullscreen view"
"&Celoobrazovkový režim"
"&Vollbild"
"Tel&jes képernyős nézet"
"Widok &pełnoekranowy"
"&Vista pantalla completa"
"&Повноекранний режим"

MEditPanelModeAlignExtensions
"&Выравнивать расширения файлов"
"Align file &extensions"
"Zarovnat příp&ony souborů"
"Datei&erweiterungen ausrichten"
"Fájlkiterjesztések &igazítása"
"W&yrównaj rozszerzenia plików"
"Alinear &extensiones de archivos"
"&Вирівнювати розширення файлів"

MEditPanelModeAlignFolderExtensions
"Выравнивать расширения пап&ок"
"Align folder e&xtensions"
"Zarovnat přípony adre&sářů"
"Ordnerer&weiterungen ausrichten"
"Mappakiterjesztések i&gazítása"
"Wyrównaj rozszerzenia &folderów"
"Alinear e&xtensiones de directorios"
"Вирівнювати розширення те&к"

MEditPanelModeFoldersUpperCase
"Показывать папки &заглавными буквами"
"Show folders in &uppercase"
"Zobrazit adresáře &velkými písmeny"
"Ordner in Großb&uchstaben zeigen"
"Mappák NAG&YBETŰVEL mutatva"
"Nazwy katalogów &WIELKIMI LITERAMI"
"Directorios en mayú&sculas"
"Показувати теки &великими літерами"

MEditPanelModeFilesLowerCase
"Показывать файлы ст&рочными буквами"
"Show files in &lowercase"
"Zobrazit soubory ma&lými písmeny"
"Dateien in K&leinbuchstaben zeigen"
"Fájlok kis&betűvel mutatva"
"&Nazwy plików małymi literami"
"archivos en minúscu&las"
"Показувати файли ма&лими літерами"

MEditPanelModeUpperToLowerCase
"Показывать имена файлов из заглавных букв &строчными буквами"
"Show uppercase file names in lower&case"
"Zobrazit velké znaky ve jménech souborů jako &malá písmena"
"G&roßgeschriebene Dateinamen in Kleinbuchstaben zeigen"
"NAGYBETŰS fájl&nevek kisbetűvel"
"Wyświetl NAZWY_PLIKÓW &jako nazwy_plików"
"archivos en mayúsculas mostrarlos con minús&culas"
"Показувати імена файлів із великих літер &маленькими літерами"

MEditPanelReadHelp
" Нажмите F1, чтобы получить информацию по настройке "
" Read online help for instructions "
" Pro instrukce si přečtěte online nápovědu "
" Siehe Hilfe für Anweisungen "
" Tanácsokat a súgóban talál (F1) "
" Instrukcje zawarte są w pomocy podręcznej "
" Para instrucciones leer ayuda en línea "
" Натисніть F1, щоб отримати інформацію про налаштування "

MSetFolderInfoTitle
l:
"Файлы информации о папках"
"Folder description files"
"Soubory s popiskem adresáře"
"Ordnerbeschreibungen"
"Mappa megjegyzésfájlok"
"Pliki opisu katalogu"
"Descripciones de directorio"
"Файли інформації про теки"

MSetFolderInfoNames
"Введите имена файлов, разделённые запятыми (допускаются маски)"
"Enter file names delimited with commas (wildcards are allowed)"
"Zadejte jména souborů oddělených čárkami (značky jsou povoleny)"
"Dateiliste, getrennt mit Komma (Jokerzeichen möglich):"
"Fájlnevek, vesszővel elválasztva (joker is használható)"
"Nazwy plików oddzielone przecinkami (znaki ? i * dopuszczalne)"
"Ingrese nombre de archivo delimitado con comas (comodines permitidos)"
"Введіть імена файлів, розділені комами (допускаються маски)"

MScreensTitle
l:
"Экраны"
"Screens"
"Obrazovky"
"Seiten"
"Képernyők"
"Ekrany"
"Pant.  "
"Екрани"

MScreensPanels
"Панели"
"Panels"
"Panely"
"Panels"
"Panelek"
"Panele"
"Paneles"
"Панелі"

MScreensView
"Просмотр"
"View"
"Zobrazit"
"Betr."
"Nézőke"
"Podgląd"
"Ver"
"Перегляд"

MScreensEdit
"Редактор"
"Edit"
"Editovat"
"Bearb"
"Szerkesztő"
"Edycja"
"Editar"
"Редактор"

MAskApplyCommandTitle
l:
"Применить команду"
"Apply command"
"Aplikovat příkaz"
"Befehl anwenden"
"Parancs végrehajtása"
"Zastosuj polecenie"
"Aplicar comando"
"Примінити команду"

MAskApplyCommand
"Введите команду для обработки выбранных файлов"
"Enter command to process selected files"
"Zadejte příkaz pro zpracování vybraných souborů"
"Befehlszeile auf ausgewählte Dateien anwenden:"
"Írja be a kijelölt fájlok parancsát:"
"Wprowadź polecenie do przetworzenia wybranych plików"
"Ingrese comando para procesar archivos seleccionados"
"Введіть команду для обробки вибраних файлів"

MPluginConfigTitle
l:
"Конфигурация модулей"
"Plugins configuration"
"Nastavení Pluginů"
"Konfiguration von Plugins"
"Plugin beállítások"
"Konfiguracja pluginów"
"Configuración de plugins"
"Конфігурація модулів"

MPluginCommandsMenuTitle
"Команды внешних модулей"
"Plugin commands"
"Příkazy pluginů"
"Pluginbefehle"
"Plugin parancsok"
"Dostępne pluginy"
"Comandos de plugins"
"Команди зовнішніх модулів"

MPreparingList
l:
"Создание списка файлов"
"Preparing files list"
"Připravuji seznam souborů"
"Dateiliste wird vorbereitet"
"Fájllista elkészítése"
"Przygotowuję listę plików"
"Preparando lista de archivos"
"Створення списку файлів"

MLangTitle
l:
"Основной язык"
"Main language"
"Hlavní jazyk"
"Hauptsprache"
"A program nyelve"
"Język programu"
"Idioma principal"
"Основна мова"

MHelpLangTitle
"Язык помощи"
"Help language"
"Jazyk nápovědy"
"Sprache der Hilfedatei"
"A súgó nyelve"
"Język pomocy"
"Idioma de ayuda"
"Мова допомоги"

MDefineMacroTitle
l:
"Задание макрокоманды"
"Define macro"
"Definovat makro"
"Definiere Makro"
"Makró gyorsbillentyű"
"Zdefiniuj makro"
"Definir macro"
"Завдання макрокоманди"

MDefineMacro
"Нажмите желаемую клавишу"
"Press the desired key"
"Stiskněte požadovanou klávesu"
"Tastenkombination:"
"Nyomja le a billentyűt"
"Naciśnij żądany klawisz"
"Pulse la tecla deseada"
"Натисніть бажану клавішу"

MMacroReDefinedKey
"Макроклавиша '%ls' уже определена."
"Macro key '%ls' already defined."
"Klávesa makra '%ls' již je definována."
"Makro '%ls' bereits definiert."
""%ls" makróbillentyű foglalt"
"Skrót '%ls' jest już zdefiniowany."
"Macro '%ls' ya está definido. Secuencia:"
"Макроклавіша '%ls' вже визначена."

MMacroDeleteAssign
"Макроклавиша '%ls' не активна."
"Macro key '%ls' is not active."
"Klávesa makra '%ls' není aktivní."
"Makro '%ls' nicht aktiv."
""%ls" makróbillentyű nem él"
"Skrót '%ls' jest nieaktywny."
"Macro '%ls' no está activo. Secuencia:"
"Макроклавіша '%ls' не активна."

MMacroDeleteKey
"Макроклавиша '%ls' будет удалена."
"Macro key '%ls' will be removed."
"Klávesa makra '%ls' bude odstraněna."
"Makro '%ls' wird entfernt und ersetzt:"
""%ls" makróbillentyű törlődik"
"Skrót '%ls' zostanie usunięty."
"Macro '%ls' será removido. Secuencia:"
"Макроклавіша '%ls' буде видалена."

MMacroCommonReDefinedKey
"Общая макроклавиша '%ls' уже определена."
"Common macro key '%ls' already defined."
"Klávesa pro běžné makro '%ls' již je definována."
"Gemeinsames Makro '%ls' bereits definiert."
""%ls" közös makróbill. foglalt"
"Skrót '%ls' jest już zdefiniowany."
"Tecla de macro '%ls' ya ha sido definida."
"Спільна макроклавіша '%ls' вже визначена."

MMacroCommonDeleteAssign
"Общая макроклавиша '%ls' не активна."
"Common macro key '%ls' is not active."
"Klávesa pro běžné makro '%ls' není aktivní."
"Gemeinsames Makro '%ls' nicht aktiv."
""%ls" közös makróbill. nem él"
"Skrót '%ls' jest nieaktywny."
"Tecla de macro '%ls' no está activada."
"Спільна макроклавіша '%ls' не активна."

MMacroCommonDeleteKey
"Общая макроклавиша '%ls' будет удалена."
"Common macro key '%ls' will be removed."
"Klávesa pro běžné makro '%ls' bude odstraněna."
"Gemeinsames Makro '%ls' wird entfernt und ersetzt:"
""%ls" közös makróbill. törlődik"
"Skrót '%ls' zostanie usunięty."
"Tecla de macro '%ls' será removida."
"Спільна макроклавіша '%ls' буде видалена."

MMacroSequence
"Последовательность:"
"Sequence:"
"Posloupnost:"
"Sequenz:"
"Szekvencia:"
"Sekwencja:"
"Secuencia:"
"Послідовність:"

MMacroReDefinedKey2
"Переопределить?"
"Redefine?"
"Předefinovat?"
"Neu definieren?"
"Újradefiniálja?"
"Zdefiniować powtórnie?"
"Redefinir?"
"Перевизначити?"

MMacroDeleteKey2
"Удалить?"
"Delete?"
"Odstranit?"
"Löschen?"
"Törli?"
"Usunąć?"
"Borrar?"
"Видалити?"

MMacroDisDisabledKey
"(макроклавиша не активна)"
"(macro key is not active)"
"(klávesa makra není aktivní)"
"(Makro inaktiv)"
"(makróbill. nem él)"
"(skrót jest nieaktywny)"
"(macro no está activo)"
"(макроклавиша не активна)"

MMacroDisOverwrite
"Переопределить"
"Overwrite"
"Přepsat"
"Überschreiben"
"Felülírás"
"Zastąpić"
"Sobrescribir"
"Перевизначити"

MMacroDisAnotherKey
"Изменить клавишу"
"Try another key"
"Zkusit jinou klávesu"
"Neue Kombination"
"Adjon meg másik billentyűt"
"Spróbuj inny klawisz"
"Intente otra tecla"
"Змінити клавішу"

MMacroSettingsTitle
l:
"Параметры макрокоманды для '%ls'"
"Macro settings for '%ls'"
"Nastavení makra pro '%ls'"
"Einstellungen für Makro '%ls'"
""%ls" makró beállításai"
"Ustawienia makra dla '%ls'"
"Configurar macro para '%ls'"
"Параметри макрокоманди для '%ls'"

MMacroSettingsEnableOutput
"Разрешить во время &выполнения вывод на экран"
"Allo&w screen output while executing macro"
"Povolit &výstup na obrazovku dokud se provádí makro"
"Bildschirmausgabe &während Makro abläuft"
"Képernyő&kimenet a makró futása közben"
"&Wyłącz zapis na ekran podczas wykonywania makra"
"Permitir salida pantalla mientras se ejecut&an los macros"
"Дозволити під час виконання &виведення на екран"

MMacroSettingsRunAfterStart
"В&ыполнять после запуска FAR"
"Execute after FAR &start"
"&Spustit po spuštění FARu"
"Ausführen beim &Starten von FAR"
"Végrehajtás a FAR &indítása után"
"Wykonaj po &starcie FAR-a"
"Ejecutar luego de &iniciar FAR"
"В&иконати після запуску FAR"

MMacroSettingsActivePanel
"&Активная панель"
"&Active panel"
"&Aktivní panel"
"&Aktives Panel"
"&Aktív panel"
"Panel &aktywny"
"Panel &activo"
"&Активна панель"

MMacroSettingsPassivePanel
"&Пассивная панель"
"&Passive panel"
"Pa&sivní panel"
"&Passives Panel"
"Passzí&v panel"
"Panel &pasywny"
"Panel &pasivo"
"&Пассивна панель"

MMacroSettingsPluginPanel
"На панели пла&гина"
"P&lugin panel"
"Panel p&luginů"
"P&lugin Panel"
"Ha &plugin panel"
"Panel p&luginów"
"Panel de p&lugins"
"На панелі пла&гіна"

MMacroSettingsFolders
"Выполнять для папо&к"
"Execute for &folders"
"Spustit pro ad&resáře"
"Auf Ordnern aus&führen"
"Ha &mappa"
"Wykonaj dla &folderów"
"Ejecutar para &directorios"
"Виконувати для те&к"

MMacroSettingsSelectionPresent
"&Отмечены файлы"
"Se&lection present"
"E&xistující výběr"
"Auswah&l vorhanden"
"Ha van ki&jelölés"
"Zaznaczenie &obecne"
"Selección presente"
"&Визначено файли"

MMacroSettingsCommandLine
"Пустая командная &строка"
"Empty &command line"
"Prázdný pří&kazový řádek"
"Leere Befehls&zeile"
"Ha &üres a parancssor"
"Pusta &linia poleceń"
"Vaciar línea de &comandos"
"Порожній командний &рядок"

MMacroSettingsSelectionBlockPresent
"Отмечен б&лок"
"Selection &block present"
"Existující blok výběr&u"
"Mar&kierter Text vorhanden"
"Ha van kijelölt &blokk"
"Obecny &blok zaznaczenia"
"Selección de bloque presente"
"Відзначений б&лок"

MMacroOutputFormatForHelpSz
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

MMacroOutputFormatForHelpDWord
l:// for <!Macro:Vars!> and <!Macro:Consts!>, count formats = 2
"%d / 0x%X"
"%d / 0x%X"
"%d / 0x%X"
"%d / 0x%X"
"%d / 0x%X"
"%d / 0x%X"
"%d / 0x%X"
"%d / 0x%X"

MMacroOutputFormatForHelpQWord
l:// for <!Macro:Vars!> and <!Macro:Consts!>, count formats = 2
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"
"%I64d / 0x%I64X"

MMacroOutputFormatForHelpDouble
l:// for <!Macro:Vars!> and <!Macro:Consts!>, count formats = 2
"%g"
"%g"
"%g"
"%g"
"%g"
"%g"
"%g"
"%g"

MMacroPErrorTitle
"Ошибка при разборе макроса"
"Error parsing macro"
upd:"Error parsing macro"
upd:"Error parsing macro"
upd:"Error parsing macro"
upd:"Error parsing macro"
"Error parsing macro"
"Помилка при розборі макросу"

MMacroPErrorPosition
"Строка %d, позиция %d"
"Line %d, Pos %d"
upd:"Line %d, Pos %d"
upd:"Line %d, Pos %d"
upd:"Line %d, Pos %d"
upd:"Line %d, Pos %d"
"Línea %d, Pos %d"
"Рядок %d, позиція %d"

MMacroPErrUnrecognized_keyword
l:
"Неизвестное ключевое слово '%ls'"
"Unrecognized keyword '%ls'"
"Neznámé klíčové slovo '%ls'"
"Unbekanntes Schlüsselwort '%ls'"
"Ismeretlen kulcsszó "%ls""
"Nie rozpoznano słowa kluczowego '%ls'"
"Unrecognized keyword '%ls'"
"Невідоме ключове слово '%ls'"

MMacroPErrUnrecognized_function
"Неизвестная функция '%ls'"
"Unrecognized function '%ls'"
"Neznámá funkce '%ls'"
"Unbekannte Funktion '%ls'"
"Ismeretlen funkció "%ls""
"Nie rozpoznano funkcji'%ls'"
"Unrecognized function '%ls'"
"Невідома функція '%ls'"

MMacroPErrFuncParam
"Неверное количество параметров у функции '%ls'"
"Incorrect number of arguments for function '%ls'"
upd:"Incorrect number of arguments for function '%ls'"
upd:"Incorrect number of arguments for function '%ls'"
"'%ls' funkció paramétereinek száma helytelen"
upd:"Incorrect number of arguments for function '%ls'"
"Incorrect number of arguments for function '%ls'"
"Неправильна кількість параметрів у функції '%ls'"

MMacroPErrNot_expected_ELSE
"Неожиданное появление $Else"
"Unexpected $Else"
"Neočekávané $Else"
"Unerwartetes $Else"
"Váratlan $Else"
"$Else w niewłaściwym miejscu"
"Unexpected $Else"
"Несподівана поява $Else"

MMacroPErrNot_expected_END
"Неожиданное появление $End"
"Unexpected $End"
"Neočekávané $End"
"Unerwartetes $End"
"Váratlan $End"
"$End w niewłaściwym miejscu"
"Unexpected $End"
"Несподівана поява $End"

MMacroPErrUnexpected_EOS
"Неожиданный конец строки"
"Unexpected end of source string"
"Neočekávaný konec zdrojového řetězce"
"Unerwartetes Ende der Zeichenkette"
"Váratlanul vége a forrássztringnek"
"Nie spodziewano się końca ciągu"
"Unexpected end of source string"
"Несподіваний кінець рядка"

MMacroPErrExpected
"Ожидается '%ls'"
"Expected '%ls'"
"Očekávané '%ls'"
"Erwartet '%ls'"
"Várható "%ls""
"Oczekiwano '%ls'"
"Expected '%ls'"
"Очікується '%ls'"

MMacroPErrBad_Hex_Control_Char
"Неизвестный шестнадцатеричный управляющий символ"
"Bad Hex Control Char"
"Chybný kontrolní znak Hex"
"Fehlerhaftes Hexzeichen"
"Rossz hexa vezérlőkarakter"
"Błędny szesnastkowy znak sterujący"
"Bad Hex Control Char"
"Невідомий шістнадцятковий керуючий символ"

MMacroPErrBad_Control_Char
"Неправильный управляющий символ"
"Bad Control Char"
"Špatný kontrolní znak"
"Fehlerhaftes Kontrollzeichen"
"Rossz vezérlőkarakter"
"Błędny znak sterujący"
"Bad Control Char"
"Неправильний керуючий символ"

MMacroPErrVar_Expected
"Переменная '%ls' не найдена"
"Variable Expected '%ls'"
"Očekávaná proměnná '%ls'"
"Variable erwartet '%ls'"
""%ls" várható változó"
"Oczekiwano zmiennej '%ls'"
"Variable Expected '%ls'"
"Змінна '%ls' не знайдена"

MMacroPErrExpr_Expected
"Ошибка синтаксиса"
"Expression Expected"
"Očekávaný výraz"
"Ausdruck erwartet"
"Szintaktikai hiba"
"Oczekiwano wyrażenia"
"Expression Expected"
"Помилка синтаксису"

MMacroPErr_ZeroLengthMacro
"Пустая макропоследовательность"
"Zero-length macro"
upd:"Zero-length macro"
upd:"Zero-length macro"
"Nulla hosszúságú makró"
upd:"Zero-length macro"
"macro de longitud 0"
"Порожня макропослідовність"

MMacroPErrIntParserError
"Внутренняя ошибка парсера"
"Internal parser error"
upd:"Internal parser error"
upd:"Internal parser error"
upd:"Internal parser error"
upd:"Internal parser error"
"Macro parsing error"
"Внутрішня помилка парсера"

MMacroPErrContinueOutsideTheLoop
"Оператор $Continue вне цикла"
upd:"$Continue outside the loop"
upd:"$Continue outside the loop"
upd:"$Continue outside the loop"
upd:"$Continue outside the loop"
upd:"$Continue outside the loop"
"$Continuar por fuera del loop"
"Оператор $Continue поза циклом"

MCannotSaveFile
l:
"Ошибка сохранения файла"
"Cannot save file"
"Nelze uložit soubor"
"Kann Datei nicht speichern"
"A fájl nem menthető"
"Nie mogę zapisać pliku"
"No se puede guardar archivo"
"Помилка збереження файлу"

MTextSavedToTemp
"Отредактированный текст записан в"
"Edited text is stored in"
"Editovaný text je uložen v"
"Editierter Text ist gespeichert in"
"A szerkesztett szöveg elmentve:"
"Edytowany tekst został zachowany w"
"Texto editado es almacenado en"
"Відредагований текст записано в"

MMonthJan
l:
"Янв"
"Jan"
"Led"
"Jan"
"Jan"
"Sty"
"Ene"
"Січ"

MMonthFeb
"Фев"
"Feb"
"Úno"
"Feb"
"Feb"
"Lut"
"Feb"
"Лют"

MMonthMar
"Мар"
"Mar"
"Bře"
"Mär"
"Már"
"Mar"
"Mar"
"Бер"

MMonthApr
"Апр"
"Apr"
"Dub"
"Apr"
"Ápr"
"Kwi"
"Abr"
"Кві"

MMonthMay
"Май"
"May"
"Kvě"
"Mai"
"Máj"
"Maj"
"May"
"Тра"

MMonthJun
"Июн"
"Jun"
"Čer"
"Jun"
"Jún"
"Cze"
"Jun"
"Чрв"

MMonthJul
"Июл"
"Jul"
"Čec"
"Jul"
"Júl"
"Lip"
"Jul"
"Лип"

MMonthAug
"Авг"
"Aug"
"Srp"
"Aug"
"Aug"
"Sie"
"Ago"
"Срп"

MMonthSep
"Сен"
"Sep"
"Zář"
"Sep"
"Sze"
"Wrz"
"Sep"
"Вер"

MMonthOct
"Окт"
"Oct"
"Říj"
"Okt"
"Okt"
"Paź"
"Oct"
"Жов"

MMonthNov
"Ноя"
"Nov"
"Lis"
"Nov"
"Nov"
"Lis"
"Nov"
"Лст"

MMonthDec
"Дек"
"Dec"
"Pro"
"Dez"
"Dec"
"Gru"
"Dic"
"Гру"

MHelpHotKey
"Введите горячую клавишу (букву или цифру)"
"Enter hot key (letter or digit)"
"Zadejte horkou klávesu (písmeno nebo číslici)"
"Buchstabe oder Ziffer:"
"Nyomja le a billentyűt (betű vagy szám)"
"Podaj klawisz skrótu (litera lub cyfra)"
"Entrar tecla rápida (letra o dígito)"
"Введіть гарячу клавішу (літеру або цифру)"

MPluginHotKeyBottom
"F4 - задать горячую клавишу"
"F4 - set hot key"
"F4 - nastavení horké klávesy"
"Kurztaste setzen: F4"
"F4 - gyorsbillentyű hozzárendelés"
"F4 - ustaw klawisz skrótu"
"F4 - asignar tecla rápida"
"F4 - встановити гарячу клавішу"

MPluginHotKeyTitle
l:
"Назначение горячей клавиши"
"Assign plugin hot key"
"Přidělit horkou klávesu pluginu"
"Dem Plugin eine Kurztaste zuweisen"
"Plugin gyorsbillentyű hozzárendelés"
"Przypisz klawisz skrótu do pluginu"
"Asignar tecla rápida a plugin"
"Призначення гарячої клавіші"

MLocationHotKeyTitle
l:
"Назначение горячей клавиши"
"Assign location hot key"
upd:"Přidělit horkou klávesu ???"
upd:"Dem ??? eine Kurztaste zuweisen"
upd:"??? gyorsbillentyű hozzárendelés"
upd:"Przypisz klawisz skrótu do ???"
upd:"Asignar tecla rápida a ???"
"Призначення гарячої клавіші"

MLocationHotKey
"Введите горячую клавишу (букву или цифру)"
"Enter hot key (letter or digit)"
"Zadejte horkou klávesu (písmeno nebo číslici)"
"Buchstabe oder Ziffer:"
"Nyomja le a billentyűt (betű vagy szám)"
"Podaj klawisz skrótu (litera lub cyfra)"
"Entrar tecla rápida (letra o dígito)"
"Введіть гарячу клавішу (літеру або цифру)"

MRightCtrl
l:
"ПравыйCtrl"
"RightCtrl"
"PravýCtrl"
"StrgRechts"
"JobbCtrl"
"PrawyCtrl"
"CtrlDrcho"
"ПравиййCtrl"

MViewerGoTo
l:
"Перейти"
"Go to"
"Jdi na"
"Gehe zu"
"Ugrás"
"Idź do"
"Ir a:"
"Перейти"

MGoToPercent
"&Процент"
"&Percent"
"&Procent"
"&Prozent"
"&Százalékban"
"&Procent"
"&Porcentaje"
"&Процент"

MGoToHex
"16-ричное &смещение"
"&Hex offset"
"&Hex offset"
"Position (&Hex)"
"&Hexában"
"Pozycja (&szesnastkowo)"
"Dirección &Hexa"
"16-річне &зміщення"

MGoToDecimal
"10-ичное с&мещение"
"&Decimal offset"
"&Desítkový offset"
"Position (&dezimal)"
"&Decimálisan"
"Pozycja (&dziesiętnie)"
"Dirección &Decimal"
"10-ічне з&міщення"

MExcTrappedException
"Исключительная ситуация"
"Exception occurred"
"Vyskytla se výjimka"
"Ausnahmefehler aufgetreten"
"Kivétel történt"
"Wystąpił wyjątek"
"Error de excepción"
"Виняткова ситуація"

MExcRAccess
"Нарушение доступа (чтение из 0x%p)"
"Access violation (read from 0x%p)"
"Neplatná adresa (čtení z 0x%p)"
"Zugriffsverletzung (Lesen von 0x%p)"
"Hozzáférési jogsértés (olvasás 0x%p címről)"
"Błąd dostępu (odczyt z 0x%p)"
"Violación de acceso (leído desde 0x%p)"
"Порушення доступу (читання 0x%p)"

MExcWAccess
"Нарушение доступа (запись в 0x%p)"
"Access violation (write to 0x%p)"
"Neplatná adresa (zápis na 0x%p)"
"Zugriffsverletzung (Schreiben nach 0x%p)"
"Hozzáférési jogsértés (írás 0x%p címre)"
"Błąd dostępu (zapis do 0x%p)"
"Violación de acceso (escrito a 0x%p)"
"Порушення доступу (запис 0x%p)"

MExcEAccess
"Нарушение доступа (исполнение кода из 0x%p)"
"Access violation (execute at 0x%p)"
"Neplatná adresa (spuštění na 0x%p)"
"Zugriffsverletzung (Ausführen bei 0x%p)"
"Hozzáférési jogsértés (végrehajtás 0x%p címen)"
"Błąd dostępu (wykonanie w 0x%p)"
"Violación de acceso (ejecutado en 0x%p)"
"Порушення доступу (виконання коду з 0x%p)"

MExcOutOfBounds
"Попытка доступа к элементу за границами массива"
"Array out of bounds"
"Pole mimo hranice"
"Arrayüberlauf"
"A tömb határait meghaladta"
"Przekroczenie granic tabeli"
"Array out of bounds"
"Спроба доступу до елемента за межами масиву"

MExcDivideByZero
"Деление на нуль"
"Divide by zero"
"Dělení nulou"
"Division durch Null"
"Nullával osztás"
"Dzielenie przez zero"
"División por cero"
"Поділ на нуль"

MExcStackOverflow
"Переполнение стека"
"Stack Overflow"
"Přetečení zásobníku"
"Stacküberlauf"
"Verem túlcsordulás"
"Przepełnienie stosu"
"Stack overflow"
"Переповнення стека"

MExcBreakPoint
"Точка останова"
"Breakpoint exception"
"Výjimka přerušení"
"Breakpoint exception"
"Törésponti kivétel"
"Wyjątek punktu przerwania"
"Excepción de punto de quiebre"
"Точка зупинки"

MExcFloatDivideByZero
"Деление на нуль при операции с плавающей точкой"
"Floating-point divide by zero"
"Dělení nulou v pohyblivé čárce"
"Fließkomma-Division durch Null"
"Lebegőpontos szám osztása nullával"
"Błąd zmiennoprzecinkowego dzielenia przez zero"
"Punto flotante dividido por cero"
"Поділ на нуль при операції з плаваючою точкою"

MExcFloatOverflow
"Переполнение при операции с плавающей точкой"
"Floating point operation overflow"
"Přetečení při operaci v pohyblivé čárce"
"Fließkomma-Operation verursachte Überlauf"
"Lebegőpontos művelet túlcsordulás"
"Przepełnienie przy operacji zmiennnoprzecinkowej"
"Operación de punto flotante desbordada"
"Переповнення при операції з плаваючою точкою"

MExcFloatStackOverflow
"Стек регистров сопроцессора полон или пуст"
"Floating point stack empty or full"
"Prázdný nebo plný zásobník v pohyblivé čárce"
"Fließkomma-Stack leer bzw. voll"
"Lebegőpont verem üres vagy megtelt"
"Stos operacji zmiennoprzecinkowych pusty lub pełny"
"Pila de punto flotante vacía o llena"
"Стек регістрів співпроцесора повний або порожній"

MExcFloatUnderflow
"Потеря точности при операции с плавающей точкой"
"Floating point operation underflow"
"Podtečení při operaci v pohyblivé čárce"
"Fließkomma-Operation verursachte Underflow"
"Lebegőpontos művelet alulcsordulás"
"Błąd niedomiaru przy operacji zmiennoprzecinkowej"
"Operación de punto flotante underflow"
"Втрата точності при операції з плаваючою точкою"

MExcBadInstruction
"Недопустимая инструкция"
"Illegal instruction"
"Neplatná instrukce"
"Ungültige Anweisung"
"Érvénytelen utasítás"
"Błędna instrukcja"
"Instrucción ilegal"
"Неприпустима інструкція"

MExcDatatypeMisalignment
"Попытка доступа к невыравненным данным"
"Alignment fault"
"Chyba zarovnání"
"Fehler bei Datenausrichtung"
"Adattípus illesztési hiba"
"Błąd ustawienia"
"Falta de alineamiento"
"Спроба доступу до невирівняних даних"

MExcUnknown
"Неизвестное исключение"
"Unknown exception"
"Neznámá výjimka"
"Unbekannte Ausnahme"
"Ismeretlen kivétel"
"Nieznany wyjątek"
"Excepción desconocida"
"Невідомий виняток"

MExcException
"Исключение:"
"Exception:"
upd:"Exception:"
upd:"Exception:"
upd:"Exception:"
upd:"Exception:"
"Excepción:"
"Виняток:"

MExcAddress
"Адрес:"
"Address:"
upd:"Address:"
upd:"Address:"
upd:"Address:"
upd:"Address:"
"Dirección:"
"Адреса:"

MExcFunction
"Функция:"
"Function:"
upd:"Function:"
upd:"Function:"
upd:"Function:"
upd:"Function:"
"Función:"
"Функція:"

MExcModule
"Модуль:"
"Module:"
upd:"Module:"
upd:"Module:"
upd:"Module:"
upd:"Module:"
"Módulo:"
"Модуль:"

MExcTerminate
"Завершить FAR"
"Terminate FAR"
upd:"Terminate FAR"
upd:"Terminate FAR"
upd:"Terminate FAR"
upd:"Terminate FAR"
"FAR se dará por terminado"
"Завершити FAR"

MExcUnload
"Выгрузить плагин"
"Unload plugin"
upd:"Unload plugin"
upd:"Unload plugin"
upd:"Unload plugin "
upd:"Unload plugin"
"El plugin será descargado"
"Вивантажити плагін"

MExcDebugger
"Отладка"
"Debug"
upd:"Debug"
upd:"Debug"
upd:"Debug"
upd:"Debug"
"Depurador"
"Налагодження"

MNetUserName
l:
"Имя пользователя"
"User name"
"Jméno uživatele"
"Benutzername"
"Felhasználói név"
"Nazwa użytkownika"
"Nombre de usuario"
"Ім'я користувача"

MNetUserPassword
"Пароль пользователя"
"User password"
"Heslo uživatele"
"Benutzerpasswort"
"Felhasználói jelszó"
"Hasło użytkownika"
"Clave de usuario"
"Пароль користувача"

MReadFolderError
l:
"Не удаётся прочесть содержимое папки"
"Cannot read folder contents"
"Nelze načíst obsah adresáře"
"Kann Ordnerinhalt nicht lesen"
"A mappa tartalma nem olvasható"
"Nie udało się odczytać zawartości folderu"
"No se puede leer contenidos de directorios"
"Не вдається прочитати вміст теки"

MPlgBadVers
l:
"Этот модуль требует FAR более высокой версии"
"This plugin requires higher FAR version"
"Tento plugin vyžaduje vyšší verzi FARu"
"Das Plugin benötigt eine aktuellere Version von FAR"
"A pluginhez újabb FAR verzió kell"
"Do uruchomienia pluginu wymagana jest wyższa wersja FAR-a"
"Este plugin requiere versión más actual de FAR"
"Цей модуль потребує FAR більш високої версії"

MPlgRequired
"Требуется версия FAR - %d.%d.%d."
"Required FAR version is %d.%d.%d."
"Požadovaná verze FARu je %d.%d.%d."
"Benötigte FAR-Version ist %d.%d.%d."
"A szükséges FAR verzió: %d.%d.%d."
"Wymagana wersja FAR-a to %d.%d.%d."
"Requiere la versión FAR %d.%d.%d."
"Потрібна версія FAR - %d.%d.%d."

MPlgRequired2
"Текущая версия FAR - %d.%d.%d."
"Current FAR version is %d.%d.%d."
"Nynější verze FARu je %d.%d.%d."
"Aktuelle FAR-Version ist %d.%d.%d."
"A jelenlegi FAR verzió: %d.%d.%d."
"Bieżąca wersja FAR-a: %d.%d.%d."
"Versión actual de FAR es %d.%d.%d"
"Поточна версія FAR - %d.%d.%d."

MPlgLoadPluginError
"Ошибка при загрузке плагина"
"Error loading plugin module"
"Chyba při nahrávání zásuvného modulu"
"Fehler beim Laden des Pluginmoduls"
"Plugin betöltési hiba"
"Błąd ładowania modułu plugina"
"Error cargando módulo plugin"
"Помилка під час завантаження плагіна"

MCheckBox2State
l:
"?"
"?"
"?"
"?"
"?"
"?"
"?"
"?"

MHelpTitle
l:
"Помощь"
"Help"
"Nápověda"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

MHelpActivatorURL
"Эта ссылка запускает внешнее приложение:"
"This reference starts the external application:"
"Tento odkaz spouští externí aplikaci:"
"Diese Referenz startet folgendes externes Programm:"
"A hivatkozás által indított program:"
"To wywołanie uruchomi aplikację zewnętrzną:"
"Esta referencia inicia la aplicación externa:"
"Це посилання запускає зовнішню програму:"

MHelpActivatorFormat
"с параметром:"
"with parameter:"
"s parametrem:"
"mit Parameter:"
"Paraméterei:"
"z parametrem:"
"con parámetro:"
"з параметром:"

MHelpActivatorQ
"Желаете запустить?"
"Do you wish to start it?"
"Přejete si ji spustit?"
"Wollen Sie jetzt starten?"
"El akarja indítani?"
"Czy chcesz ją uruchomić?"
"Desea comenzar la aplicación?"
"Хочете запустити?"

MCannotOpenHelp
"Ошибка открытия файла"
"Cannot open the file"
"Nelze otevřít soubor"
"Kann Datei nicht öffnen"
"A fájl nem nyitható meg"
"Nie można otworzyć pliku"
"No se puede abrir el archivo"
"Помилка відкриття файлу"

MHelpTopicNotFound
"Не найден запрошенный раздел помощи:"
"Requested help topic not found:"
"požadované téma nápovědy nebylo nalezeno"
"Angefordertes Hilfethema wurde nicht gefunden:"
"A kívánt súgó témakör nem található:"
"Nie znaleziono tematu pomocy:"
"Tema de ayuda requerido no encontrado"
"Не знайдено запитаний розділ допомоги:"

MPluginsHelpTitle
l:
"Внешние модули"
"Plugins help"
"Nápověda Pluginů"
"Pluginhilfe"
"Pluginek súgói"
"Pomoc dla pluginów"
"Ayuda plugins"
"Зовнішні модулі"

MDocumentsHelpTitle
"Документы"
"Documents help"
"Nápověda Dokumentů"
"Dokumentenhilfe"
"Dokumentumok súgói"
"Pomoc dla dokumentów"
"Ayuda documentos"
"Документи"

MHelpSearchTitle
l:
"Поиск"
"Search"
"Hledání"
"Suchen"
"Keresés"
"Szukaj"
"Buscar"
"Пошук"

MHelpSearchingFor
"Поиск для"
"Searching for"
"Hledání"
"Suche nach"
"Keresés:"
"Znajdź"
"Buscando por"
"Пошук для"

MHelpSearchCannotFind
"Строка не найдена"
"Could not find the string"
"Nelze najít řetězec"
"Konnte Zeichenkette nicht finden"
"A szöveg nem található:"
"Nie mogę odnaleźć ciągu znaków"
"No se encontró la cadena"
"Рядок не знайдено"

MHelpF1
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

MHelpF2
""
""
""
""
""
""
""
""

MHelpF3
""
""
""
""
""
""
""
""

MHelpF4
""
""
""
""
""
""
""
""

MHelpF5
"Размер"
"Zoom"
"Zoom"
"Vergr."
"Nagyít"
"Powiększ"
"Zoom"
"Розмір"

MHelpF6
""
""
""
""
""
""
""
""

MHelpF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Szukaj"
"Buscar"
"Пошук"

MHelpF8
""
""
""
""
""
""
""
""

MHelpF9
""
""
""
""
""
""
""
""

MHelpF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Salir"
"Вихід"

MHelpF11
""
""
""
""
""
""
""
""

MHelpF12
""
""
""
""
""
""
""
""

MHelpShiftF1
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

MHelpShiftF2
"Плагин"
"Plugin"
"Plugin"
"Plugin"
"PlgSúg"
"Plugin"
"Plugin"
"Плагін"

MHelpShiftF3
"Докум"
"Docums"
"Dokume"
"Dokume"
"DokSúg"
"Dokumenty"
"Docums"
"Докум"

MHelpShiftF4
""
""
""
""
""
""
""
""

MHelpShiftF5
""
""
""
""
""
""
""
""

MHelpShiftF6
""
""
""
""
""
""
""
""

MHelpShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"Tovább"
"Nast."
"Próxim"
"Далі"

MHelpShiftF8
""
""
""
""
""
""
""
""

MHelpShiftF9
""
""
""
""
""
""
""
""

MHelpShiftF10
""
""
""
""
""
""
""
""

MHelpShiftF11
""
""
""
""
""
""
""
""

MHelpShiftF12
""
""
""
""
""
""
""
""

MHelpAltF1
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

MHelpAltF2
""
""
""
""
""
""
""
""

MHelpAltF3
""
""
""
""
""
""
""
""

MHelpAltF4
""
""
""
""
""
""
""
""

MHelpAltF5
""
""
""
""
""
""
""
""

MHelpAltF6
""
""
""
""
""
""
""
""

MHelpAltF7
""
""
""
""
""
""
""
""

MHelpAltF8
""
""
""
""
""
""
""
""

MHelpAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

MHelpAltF10
""
""
""
""
""
""
""
""

MHelpAltF11
""
""
""
""
""
""
""
""

MHelpAltF12
""
""
""
""
""
""
""
""

MHelpCtrlF1
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

MHelpCtrlF2
""
""
""
""
""
""
""
""

MHelpCtrlF3
""
""
""
""
""
""
""
""

MHelpCtrlF4
""
""
""
""
""
""
""
""

MHelpCtrlF5
""
""
""
""
""
""
""
""

MHelpCtrlF6
""
""
""
""
""
""
""
""

MHelpCtrlF7
""
""
""
""
""
""
""
""

MHelpCtrlF8
""
""
""
""
""
""
""
""

MHelpCtrlF9
""
""
""
""
""
""
""
""

MHelpCtrlF10
""
""
""
""
""
""
""
""

MHelpCtrlF11
""
""
""
""
""
""
""
""

MHelpCtrlF12
""
""
""
""
""
""
""
""

MHelpCtrlShiftF1
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

MHelpCtrlShiftF2
""
""
""
""
""
""
""
""

MHelpCtrlShiftF3
""
""
""
""
""
""
""
""

MHelpCtrlShiftF4
""
""
""
""
""
""
""
""

MHelpCtrlShiftF5
""
""
""
""
""
""
""
""

MHelpCtrlShiftF6
""
""
""
""
""
""
""
""

MHelpCtrlShiftF7
""
""
""
""
""
""
""
""

MHelpCtrlShiftF8
""
""
""
""
""
""
""
""

MHelpCtrlShiftF9
""
""
""
""
""
""
""
""

MHelpCtrlShiftF10
""
""
""
""
""
""
""
""

MHelpCtrlShiftF11
""
""
""
""
""
""
""
""

MHelpCtrlShiftF12
""
""
""
""
""
""
""
""

MHelpCtrlAltF1
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

MHelpCtrlAltF2
""
""
""
""
""
""
""
""

MHelpCtrlAltF3
""
""
""
""
""
""
""
""

MHelpCtrlAltF4
""
""
""
""
""
""
""
""

MHelpCtrlAltF5
""
""
""
""
""
""
""
""

MHelpCtrlAltF6
""
""
""
""
""
""
""
""

MHelpCtrlAltF7
""
""
""
""
""
""
""
""

MHelpCtrlAltF8
""
""
""
""
""
""
""
""

MHelpCtrlAltF9
""
""
""
""
""
""
""
""

MHelpCtrlAltF10
""
""
""
""
""
""
""
""

MHelpCtrlAltF11
""
""
""
""
""
""
""
""

MHelpCtrlAltF12
""
""
""
""
""
""
""
""

MHelpAltShiftF1
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

MHelpAltShiftF2
""
""
""
""
""
""
""
""

MHelpAltShiftF3
""
""
""
""
""
""
""
""

MHelpAltShiftF4
""
""
""
""
""
""
""
""

MHelpAltShiftF5
""
""
""
""
""
""
""
""

MHelpAltShiftF6
""
""
""
""
""
""
""
""

MHelpAltShiftF7
""
""
""
""
""
""
""
""

MHelpAltShiftF8
""
""
""
""
""
""
""
""

MHelpAltShiftF9
""
""
""
""
""
""
""
""

MHelpAltShiftF10
""
""
""
""
""
""
""
""

MHelpAltShiftF11
""
""
""
""
""
""
""
""

MHelpAltShiftF12
""
""
""
""
""
""
""
""

MHelpCtrlAltShiftF1
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

MHelpCtrlAltShiftF2
""
""
""
""
""
""
""
""

MHelpCtrlAltShiftF3
""
""
""
""
""
""
""
""

MHelpCtrlAltShiftF4
""
""
""
""
""
""
""
""

MHelpCtrlAltShiftF5
""
""
""
""
""
""
""
""

MHelpCtrlAltShiftF6
""
""
""
""
""
""
""
""

MHelpCtrlAltShiftF7
""
""
""
""
""
""
""
""

MHelpCtrlAltShiftF8
""
""
""
""
""
""
""
""

MHelpCtrlAltShiftF9
""
""
""
""
""
""
""
""

MHelpCtrlAltShiftF10
""
""
""
""
""
""
""
""

MHelpCtrlAltShiftF11
""
""
""
""
""
""
""
""

MHelpCtrlAltShiftF12
""
""
""
""
""
""
""
""

MInfoF1
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

MInfoF2
"Сверн"
"Wrap"
"Zalam"
"Umbr."
"SorTör"
"Zawiń"
"Divide"
"Згорн"

MInfoF3
"СмОпис"
"VieDiz"
"Zobraz"
"BetDiz"
"MjMnéz"
"VieDiz"
"VerDiz"
"ДвОпис"

MInfoF4
"РедОпи"
"EdtDiz"
"Edit"
"BeaDiz"
"MjSzrk"
"EdtDiz"
"EdtDiz"
"РедОпи"

MInfoF5
""
""
""
""
""
""
""
""

MInfoF6
""
""
""
""
""
""
""
""

MInfoF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Search"
"Buscar"
"Пошук"

MInfoF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Win"
"ANSI"


MInfoF9
"КонфМн"
"ConfMn"
"KonfMn"
"KonfMn"
"KonfMn"
"ConfMn"
"BarMnu"
"КонфМн"

MInfoF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

MInfoF11
"Модули"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Модулі"

MInfoF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant. "
"Екрани"

MInfoShiftF1
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

MInfoShiftF2
"Слова"
"WWrap"
"ZalSlo"
"WUmbr"
"SzóTör"
"ZawijS"
"ConDiv"
"Слова"

MInfoShiftF3
""
""
""
""
""
""
""
""

MInfoShiftF4
""
""
""
""
""
""
""
""

MInfoShiftF5
""
""
""
""
""
""
""
""

MInfoShiftF6
""
""
""
""
""
""
""
""

MInfoShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"TovKer"
"Nast."
"Próxim"
"Далі"

MInfoShiftF8
"КодСтр"
"CodePg"
upd:"ZnSady"
upd:"Tabell"
"Kódlap"
"StrKod"
"Tabla"
"КодСтор"

MInfoShiftF9
"Сохран"
"Save"
"Uložit"
"Speich"
"Mentés"
"Zapisz"
"Guarda"
"Зберти"

MInfoShiftF10
"Послдн"
"Last"
"Posled"
"Letzt"
"UtsMnü"
"Ostat."
"Ultimo"
"Останн"

MInfoShiftF11
""
""
""
""
""
""
""
""

MInfoShiftF12
""
""
""
""
""
""
""
""

MInfoAltF1
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

MInfoAltF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha"
"Права"

MInfoAltF3
""
""
""
""
""
""
""
""

MInfoAltF4
""
""
""
""
""
""
""
""

MInfoAltF5
""
""
""
""
""
""
""
""

MInfoAltF6
""
""
""
""
""
""
""
""

MInfoAltF7
"Искать"
"Find"
"Hledat"
"Suchen"
"Keres"
"Znajdź"
"Encont"
"Шукати"

MInfoAltF8
"Строка"
"Goto"
"Jít na"
"GeheZu"
"Ugrás"
"IdźDo"
"Ir a.."
"Рядок"

MInfoAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

MInfoAltF10
"Дерево"
"Tree"
"Strom"
"Baum"
"MapKer"
"Drzewo"
"Arbol"
"Дерево"

MInfoAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"Historia"
"HisVer"
"ІстПр"

MInfoAltF12
"ИстПап"
"FoldHs"
"AdrsHs"
"OrdnHs"
"MapElő"
"FoldHs"
"HisDir"
"ІстТек"

MInfoCtrlF1
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

MInfoCtrlF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha"
"Права"

MInfoCtrlF3
""
""
""
""
""
""
""
""

MInfoCtrlF4
""
""
""
""
""
""
""
""

MInfoCtrlF5
""
""
""
""
""
""
""
""

MInfoCtrlF6
""
""
""
""
""
""
""
""

MInfoCtrlF7
""
""
""
""
""
""
""
""

MInfoCtrlF8
""
""
""
""
""
""
""
""

MInfoCtrlF9
""
""
""
""
""
""
""
""

MInfoCtrlF10
""
""
""
""
""
""
""
""

MInfoCtrlF11
""
""
""
""
""
""
""
""

MInfoCtrlF12
""
""
""
""
""
""
""
""

MInfoCtrlShiftF1
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

MInfoCtrlShiftF2
""
""
""
""
""
""
""
""

MInfoCtrlShiftF3
""
""
""
""
""
""
""
""

MInfoCtrlShiftF4
""
""
""
""
""
""
""
""

MInfoCtrlShiftF5
""
""
""
""
""
""
""
""

MInfoCtrlShiftF6
""
""
""
""
""
""
""
""

MInfoCtrlShiftF7
""
""
""
""
""
""
""
""

MInfoCtrlShiftF8
""
""
""
""
""
""
""
""

MInfoCtrlShiftF9
""
""
""
""
""
""
""
""

MInfoCtrlShiftF10
""
""
""
""
""
""
""
""

MInfoCtrlShiftF11
""
""
""
""
""
""
""
""

MInfoCtrlShiftF12
""
""
""
""
""
""
""
""

MInfoCtrlAltF1
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

MInfoCtrlAltF2
""
""
""
""
""
""
""
""

MInfoCtrlAltF3
""
""
""
""
""
""
""
""

MInfoCtrlAltF4
""
""
""
""
""
""
""
""

MInfoCtrlAltF5
""
""
""
""
""
""
""
""

MInfoCtrlAltF6
""
""
""
""
""
""
""
""

MInfoCtrlAltF7
""
""
""
""
""
""
""
""

MInfoCtrlAltF8
""
""
""
""
""
""
""
""

MInfoCtrlAltF9
""
""
""
""
""
""
""
""

MInfoCtrlAltF10
""
""
""
""
""
""
""
""

MInfoCtrlAltF11
""
""
""
""
""
""
""
""

MInfoCtrlAltF12
""
""
""
""
""
""
""
""

MInfoAltShiftF1
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

MInfoAltShiftF2
""
""
""
""
""
""
""
""

MInfoAltShiftF3
""
""
""
""
""
""
""
""

MInfoAltShiftF4
""
""
""
""
""
""
""
""

MInfoAltShiftF5
""
""
""
""
""
""
""
""

MInfoAltShiftF6
""
""
""
""
""
""
""
""

MInfoAltShiftF7
""
""
""
""
""
""
""
""

MInfoAltShiftF8
""
""
""
""
""
""
""
""

MInfoAltShiftF9
""
""
""
""
""
""
""
""

MInfoAltShiftF10
""
""
""
""
""
""
""
""

MInfoAltShiftF11
""
""
""
""
""
""
""
""

MInfoAltShiftF12
""
""
""
""
""
""
""
""

MInfoCtrlAltShiftF1
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

MInfoCtrlAltShiftF2
""
""
""
""
""
""
""
""

MInfoCtrlAltShiftF3
""
""
""
""
""
""
""
""

MInfoCtrlAltShiftF4
""
""
""
""
""
""
""
""

MInfoCtrlAltShiftF5
""
""
""
""
""
""
""
""

MInfoCtrlAltShiftF6
""
""
""
""
""
""
""
""

MInfoCtrlAltShiftF7
""
""
""
""
""
""
""
""

MInfoCtrlAltShiftF8
""
""
""
""
""
""
""
""

MInfoCtrlAltShiftF9
""
""
""
""
""
""
""
""

MInfoCtrlAltShiftF10
""
""
""
""
""
""
""
""

MInfoCtrlAltShiftF11
""
""
""
""
""
""
""
""

MInfoCtrlAltShiftF12
""
""
""
""
""
""
""
""

MQViewF1
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

MQViewF2
"Сверн"
"Wrap"
"Zalam"
"Umbr."
"SorTör"
"Zawiń"
"Divide"
"Згорн"

MQViewF3
"Просм"
"View"
"Zobraz"
"Betr."
"Megnéz"
"Zobacz"
"Ver"
"Прогл"

MQViewF4
"Код"
"Hex"
"Hex"
"Hex"
"Hexa"
"Hex"
"Hexa"
"Код"

MQViewF5
""
""
""
""
""
""
""
""

MQViewF6
""
""
""
""
""
""
""
""

MQViewF7
"Поиск"
"Search"
"Hledat"
"Suchen"
"Keres"
"Szukaj"
"Buscar"
"Пошук"

MQViewF8
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"ANSI"
"Win"
"ANSI"

MQViewF9
"КонфМн"
"ConfMn"
"KonfMn"
"KonfMn"
"KonfMn"
"ConfMn"
"BarMnu"
"КонфМн"

MQViewF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

MQViewF11
"Модули"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Модулі"

MQViewF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant. "
"Екрани"

MQViewShiftF1
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

MQViewShiftF2
"Слова"
"WWrap"
"ZalSlo"
"WUmbr"
"SzóTör"
"WWrap"
"ConDiv"
"Слова"

MQViewShiftF3
""
""
""
""
""
""
""
""

MQViewShiftF4
""
""
""
""
""
""
""
""

MQViewShiftF5
""
""
""
""
""
""
""
""

MQViewShiftF6
""
""
""
""
""
""
""
""

MQViewShiftF7
"Дальше"
"Next"
"Další"
"Nächst"
"TovKer"
"Nast."
"Próxim"
"Далі"

MQViewShiftF8
"КодСтр"
"CodePg"
upd:"ZnSady"
upd:"Tabell"
"Kódlap"
"StrKod"
"Tabla"
"КодСтор"

MQViewShiftF9
"Сохран"
"Save"
"Uložit"
"Speich"
"Mentés"
"Zapisz"
"Guarda"
"Збргти"

MQViewShiftF10
"Послдн"
"Last"
"Posled"
"Letzt"
"UtsMnü"
"Ostat."
"Ultimo"
"Останн"

MQViewShiftF11
""
""
""
""
""
""
""
""

MQViewShiftF12
""
""
""
""
""
""
""
""

MQViewAltF1
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

MQViewAltF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha"
"Права"

MQViewAltF3
""
""
""
""
""
""
""
""

MQViewAltF4
""
""
""
""
""
""
""
""

MQViewAltF5
""
""
""
""
""
""
""
""

MQViewAltF6
""
""
""
""
""
""
""
""

MQViewAltF7
"Искать"
"Find"
"Hledat"
"Suchen"
"Keres"
"Znajdź"
"Encont"
"Шукати"

MQViewAltF8
"Строка"
"Goto"
"Jít na"
"GeheZu"
"Ugrás"
"IdźDo"
"Ir a."
"Рядок"

MQViewAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

MQViewAltF10
"Дерево"
"Tree"
"Strom"
"Baum"
"MapKer"
"Drzewo"
"Arbol"
"Дерево"

MQViewAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"Historia"
"HisVer"
"ІстПр"

MQViewAltF12
"ИстПап"
"FoldHs"
"AdrsHs"
"OrdnHs"
"MapElő"
"FoldHs"
"HisDir"
"ІстТек"

MQViewCtrlF1
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

MQViewCtrlF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha"
"Права"

MQViewCtrlF3
""
""
""
""
""
""
""
""

MQViewCtrlF4
""
""
""
""
""
""
""
""

MQViewCtrlF5
""
""
""
""
""
""
""
""

MQViewCtrlF6
""
""
""
""
""
""
""
""

MQViewCtrlF7
""
""
""
""
""
""
""
""

MQViewCtrlF8
""
""
""
""
""
""
""
""

MQViewCtrlF9
""
""
""
""
""
""
""
""

MQViewCtrlF10
""
""
""
""
""
""
""
""

MQViewCtrlF11
""
""
""
""
""
""
""
""

MQViewCtrlF12
""
""
""
""
""
""
""
""

MQViewCtrlShiftF1
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

MQViewCtrlShiftF2
""
""
""
""
""
""
""
""

MQViewCtrlShiftF3
""
""
""
""
""
""
""
""

MQViewCtrlShiftF4
""
""
""
""
""
""
""
""

MQViewCtrlShiftF5
""
""
""
""
""
""
""
""

MQViewCtrlShiftF6
""
""
""
""
""
""
""
""

MQViewCtrlShiftF7
""
""
""
""
""
""
""
""

MQViewCtrlShiftF8
""
""
""
""
""
""
""
""

MQViewCtrlShiftF9
""
""
""
""
""
""
""
""

MQViewCtrlShiftF10
""
""
""
""
""
""
""
""

MQViewCtrlShiftF11
""
""
""
""
""
""
""
""

MQViewCtrlShiftF12
""
""
""
""
""
""
""
""

MQViewCtrlAltF1
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

MQViewCtrlAltF2
""
""
""
""
""
""
""
""

MQViewCtrlAltF3
""
""
""
""
""
""
""
""

MQViewCtrlAltF4
""
""
""
""
""
""
""
""

MQViewCtrlAltF5
""
""
""
""
""
""
""
""

MQViewCtrlAltF6
""
""
""
""
""
""
""
""

MQViewCtrlAltF7
""
""
""
""
""
""
""
""

MQViewCtrlAltF8
""
""
""
""
""
""
""
""

MQViewCtrlAltF9
""
""
""
""
""
""
""
""

MQViewCtrlAltF10
""
""
""
""
""
""
""
""

MQViewCtrlAltF11
""
""
""
""
""
""
""
""

MQViewCtrlAltF12
""
""
""
""
""
""
""
""

MQViewAltShiftF1
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

MQViewAltShiftF2
""
""
""
""
""
""
""
""

MQViewAltShiftF3
""
""
""
""
""
""
""
""

MQViewAltShiftF4
""
""
""
""
""
""
""
""

MQViewAltShiftF5
""
""
""
""
""
""
""
""

MQViewAltShiftF6
""
""
""
""
""
""
""
""

MQViewAltShiftF7
""
""
""
""
""
""
""
""

MQViewAltShiftF8
""
""
""
""
""
""
""
""

MQViewAltShiftF9
""
""
""
""
""
""
""
""

MQViewAltShiftF10
""
""
""
""
""
""
""
""

MQViewAltShiftF11
""
""
""
""
""
""
""
""

MQViewAltShiftF12
""
""
""
""
""
""
""
""

MQViewCtrlAltShiftF1
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

MQViewCtrlAltShiftF2
""
""
""
""
""
""
""
""

MQViewCtrlAltShiftF3
""
""
""
""
""
""
""
""

MQViewCtrlAltShiftF4
""
""
""
""
""
""
""
""

MQViewCtrlAltShiftF5
""
""
""
""
""
""
""
""

MQViewCtrlAltShiftF6
""
""
""
""
""
""
""
""

MQViewCtrlAltShiftF7
""
""
""
""
""
""
""
""

MQViewCtrlAltShiftF8
""
""
""
""
""
""
""
""

MQViewCtrlAltShiftF9
""
""
""
""
""
""
""
""

MQViewCtrlAltShiftF10
""
""
""
""
""
""
""
""

MQViewCtrlAltShiftF11
""
""
""
""
""
""
""
""

MQViewCtrlAltShiftF12
""
""
""
""
""
""
""
""

MKBTreeF1
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

MKBTreeF2
"ПользМ"
"UserMn"
"UživMn"
"BenuMn"
"FelhMn"
"UserMn"
"Menú"
"КорстМ"

MKBTreeF3
""
""
""
""
""
""
""
""

MKBTreeF4
"Атриб"
"Attr"
"Attr"
"Attr"
"Attrib"
"Atryb."
"Atrib"
"Атриб"

MKBTreeF5
"Копир"
"Copy"
"Kopír."
"Kopier"
"Másol"
"Kopiuj"
"Copiar"
"Копию"

MKBTreeF6
"Перен"
"RenMov"
"PřjPřs"
"RenMov"
"ÁtnMoz"
"Zamień"
"RenMov"
"Перен"

MKBTreeF7
"Папка"
"MkFold"
"VytAdr"
"VerzEr"
"ÚjMapp"
"NowyFldr"
"CrDIR "
"Тека"

MKBTreeF8
"Удален"
"Delete"
"Smazat"
"Lösch"
"Törlés"
"Usuń"
"Borrar"
"Видалн"

MKBTreeF9
"КонфМн"
"ConfMn"
"KonfMn"
"KonfMn"
"KonfMn"
"KonfMenu"
"BarMnu"
"КонфМн"

MKBTreeF10
"Выход"
"Quit"
"Konec"
"Ende"
"Kilép"
"Koniec"
"Quitar"
"Вихід"

MKBTreeF11
"Модули"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Plugin"
"Модулі"

MKBTreeF12
"Экраны"
"Screen"
"Obraz."
"Seiten"
"Képrny"
"Ekran"
"Pant."
"Екрани"

MKBTreeShiftF1
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

MKBTreeShiftF2
""
""
""
""
""
""
""
""

MKBTreeShiftF3
""
""
""
""
""
""
""
""

MKBTreeShiftF4
""
""
""
""
""
""
""
""

MKBTreeShiftF5
"Копир"
"Copy"
"Kopír."
"Kopier"
"Másol"
"Kopiuj"
"Copiar"
"Копию"

MKBTreeShiftF6
"Перен"
"Rename"
"Přejm."
"Umben"
"ÁtnMoz"
"Zamień"
"RenMov"
"Перен"

MKBTreeShiftF7
""
""
""
""
""
""
""
""

MKBTreeShiftF8
""
""
""
""
""
""
""
""

MKBTreeShiftF9
"Сохран"
"Save"
"Uložit"
"Speich"
"Mentés"
"Zapisz"
"Guarda"
"Збргти"

MKBTreeShiftF10
"Послдн"
"Last"
"Posled"
"Letzt"
"UtsMnü"
"Ostat."
"Ultimo"
"Останн"

MKBTreeShiftF11
"Группы"
"Group"
"Skupin"
"Gruppe"
"Csoprt"
"Grupa"
"Grupo"
"Групи"

MKBTreeShiftF12
"Выбран"
"SelUp"
"VybPrv"
"AuswOb"
"KijFel"
"SelUp"
"SelUp"
"Вибран"

MKBTreeAltF1
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

MKBTreeAltF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha"
"Права"

MKBTreeAltF3
""
""
""
""
""
""
""
""

MKBTreeAltF4
""
""
""
""
""
""
""
""

MKBTreeAltF5
""
""
""
""
""
""
""
""

MKBTreeAltF6
""
""
""
""
""
""
""
""

MKBTreeAltF7
"Искать"
"Find"
"Hledat"
"Suchen"
"Keres"
"Znajdź"
"Encont"
"Шукати"

MKBTreeAltF8
"Истор"
"Histry"
"Histor"
"Histor"
"ParElő"
"Historia"
"Histor"
"Істор"

MKBTreeAltF9
"Видео"
"Video"
"Video"
"Ansich"
"Video"
"Video"
"Video"
"Відео"

MKBTreeAltF10
"Дерево"
"Tree"
"Strom"
"Baum"
"MapKer"
"Drzewo"
"Arbol"
"Дерево"

MKBTreeAltF11
"ИстПр"
"ViewHs"
"ProhHs"
"BetrHs"
"NézElő"
"Historia"
"HisVer"
"ІстПр"

MKBTreeAltF12
"ИстПап"
"FoldHs"
"AdrsHs"
"OrdnHs"
"MapElő"
"FoldHs"
"HisDir"
"ІстТек"

MKBTreeCtrlF1
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

MKBTreeCtrlF2
"Правая"
"Right"
"Pravý"
"Rechts"
"Jobb"
"Prawy"
"Drcha"
"Права"

MKBTreeCtrlF3
""
""
""
""
""
""
""
""

MKBTreeCtrlF4
""
""
""
""
""
""
""
""

MKBTreeCtrlF5
""
""
""
""
""
""
""
""

MKBTreeCtrlF6
""
""
""
""
""
""
""
""

MKBTreeCtrlF7
""
""
""
""
""
""
""
""

MKBTreeCtrlF8
""
""
""
""
""
""
""
""

MKBTreeCtrlF9
""
""
""
""
""
""
""
""

MKBTreeCtrlF10
""
""
""
""
""
""
""
""

MKBTreeCtrlF11
""
""
""
""
""
""
""
""

MKBTreeCtrlF12
""
""
""
""
""
""
""
""

MKBTreeCtrlShiftF1
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

MKBTreeCtrlShiftF2
""
""
""
""
""
""
""
""

MKBTreeCtrlShiftF3
""
""
""
""
""
""
""
""

MKBTreeCtrlShiftF4
""
""
""
""
""
""
""
""

MKBTreeCtrlShiftF5
""
""
""
""
""
""
""
""

MKBTreeCtrlShiftF6
""
""
""
""
""
""
""
""

MKBTreeCtrlShiftF7
""
""
""
""
""
""
""
""

MKBTreeCtrlShiftF8
""
""
""
""
""
""
""
""

MKBTreeCtrlShiftF9
""
""
""
""
""
""
""
""

MKBTreeCtrlShiftF10
""
""
""
""
""
""
""
""

MKBTreeCtrlShiftF11
""
""
""
""
""
""
""
""

MKBTreeCtrlShiftF12
""
""
""
""
""
""
""
""

MKBTreeCtrlAltF1
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

MKBTreeCtrlAltF2
""
""
""
""
""
""
""
""

MKBTreeCtrlAltF3
""
""
""
""
""
""
""
""

MKBTreeCtrlAltF4
""
""
""
""
""
""
""
""

MKBTreeCtrlAltF5
""
""
""
""
""
""
""
""

MKBTreeCtrlAltF6
""
""
""
""
""
""
""
""

MKBTreeCtrlAltF7
""
""
""
""
""
""
""
""

MKBTreeCtrlAltF8
""
""
""
""
""
""
""
""

MKBTreeCtrlAltF9
""
""
""
""
""
""
""
""

MKBTreeCtrlAltF10
""
""
""
""
""
""
""
""

MKBTreeCtrlAltF11
""
""
""
""
""
""
""
""

MKBTreeCtrlAltF12
""
""
""
""
""
""
""
""

MKBTreeAltShiftF1
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

MKBTreeAltShiftF2
""
""
""
""
""
""
""
""

MKBTreeAltShiftF3
""
""
""
""
""
""
""
""

MKBTreeAltShiftF4
""
""
""
""
""
""
""
""

MKBTreeAltShiftF5
""
""
""
""
""
""
""
""

MKBTreeAltShiftF6
""
""
""
""
""
""
""
""

MKBTreeAltShiftF7
""
""
""
""
""
""
""
""

MKBTreeAltShiftF8
""
""
""
""
""
""
""
""

MKBTreeAltShiftF9
""
""
""
""
""
""
""
""

MKBTreeAltShiftF10
""
""
""
""
""
""
""
""

MKBTreeAltShiftF11
""
""
""
""
""
""
""
""

MKBTreeAltShiftF12
""
""
""
""
""
""
""
""

MKBTreeCtrlAltShiftF1
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

MKBTreeCtrlAltShiftF2
""
""
""
""
""
""
""
""

MKBTreeCtrlAltShiftF3
""
""
""
""
""
""
""
""

MKBTreeCtrlAltShiftF4
""
""
""
""
""
""
""
""

MKBTreeCtrlAltShiftF5
""
""
""
""
""
""
""
""

MKBTreeCtrlAltShiftF6
""
""
""
""
""
""
""
""

MKBTreeCtrlAltShiftF7
""
""
""
""
""
""
""
""

MKBTreeCtrlAltShiftF8
""
""
""
""
""
""
""
""

MKBTreeCtrlAltShiftF9
""
""
""
""
""
""
""
""

MKBTreeCtrlAltShiftF10
""
""
""
""
""
""
""
""

MKBTreeCtrlAltShiftF11
""
""
""
""
""
""
""
""

MKBTreeCtrlAltShiftF12
""
""
""
""
""
""
""
""

MCopyTimeInfo
l:
"Время: %8.8ls    Осталось: %8.8ls    %8.8lsб/с"
"Time: %8.8ls    Remaining: %8.8ls    %8.8lsb/s"
"Čas: %8.8ls      Zbývá: %8.8ls      %8.8lsb/s"
"Zeit: %8.8ls   Verbleibend: %8.8ls   %8.8lsb/s"
"Eltelt: %8.8ls    Maradt: %8.8ls    %8.8lsb/s"
"Czas: %8.8ls    Pozostało: %8.8ls    %8.8lsb/s"
"Tiempo: %8.8ls    Restante: %8.8ls    %8.8lsb/s"
"Час: %8.8ls    Залишилось: %8.8ls    %8.8lsб/с"

MKeyESCWasPressed
l:
"Действие было прервано"
"Operation has been interrupted"
"Operace byla přerušena"
"Vorgang wurde unterbrochen"
"A műveletet megszakította"
"Operacja została przerwana"
"Operación ha sido interrumpida"
"Дія була перервана"

MDoYouWantToStopWork
"Вы действительно хотите отменить действие?"
"Do you really want to cancel it?"
"Opravdu chcete operaci stornovat?"
"Wollen Sie den Vorgang wirklich abbrechen?"
"Valóban le akarja állítani?"
"Czy naprawdę chcesz ją anulować?"
"Desea realmente cancelar la operación?"
"Ви дійсно хочете скасувати дію?"

MDoYouWantToStopWork2
"Продолжить выполнение?"
"Continue work? "
"Pokračovat v práci?"
"Vorgang fortsetzen? "
"Folytatja?"
"Kontynuować? "
"Continuar trabajo? "
"Продовжити виконання?"

MCheckingFileInPlugin
l:
"Файл проверяется в плагине"
"The file is being checked by the plugin"
"Soubor je právě kontrolován pluginem"
"Datei wird von Plugin überprüft"
"A fájlt ez a plugin használja:"
"Plugin sprawdza plik"
"El archivo está siendo chequeado por el plugin"
"Файл перевіряється у плагіні"

MDialogType
l:
"Диалог"
"Dialog"
"Dialog"
"Dialog"
"Párbeszéd"
"Dialog"
"Diálogo"
"Діалог"

MHelpType
"Помощь"
"Help"
"Nápověda"
"Hilfe"
"Súgó"
"Pomoc"
"Ayuda"
"Допомога"

MFolderTreeType
"ПоискКаталогов"
"FolderTree"
"StromAdresáře"
"Ordnerbaum"
"MappaFa"
"Drzewo folderów"
"ArbolDirectorio"
"ПошукКаталогів"

MVMenuType
"Меню"
"Menu"
"Menu"
"Menü"
"Menü"
"Menu"
"Menú"
"Меню"

MIncorrectMask
l:
"Некорректная маска файлов"
"File-mask string contains errors"
"Řetězec masky souboru obsahuje chyby"
"Zeichenkette mit Dateimaske enthält Fehler"
"A fájlmaszk hibás"
"Maska pliku zawiera błędy"
"Cadena de máscara de archivos contiene errores"
"Неправильна маска файлів"

MPanelBracketsForLongName
l:
"{}"
"{}"
"{}"
"{}"
"{}"
"{}"
"{}"
"{}"

MComspecNotFound
l:
"Переменная окружения %COMSPEC% не определена"
"Environment variable %COMSPEC% not defined"
"Proměnná prostředí %COMSPEC% není definována"
"Umgebungsvariable %COMSPEC% nicht definiert"
"A %COMSPEC% környezeti változó nincs definiálva"
"Nie zdefiniowano zmiennej środowiskowej %COMSPEC%"
"Variable de entorno %COMSPEC% no definida"
"Змінне оточення %COMSPEC% не визначено"

MExecuteErrorMessage
"'%ls' не является внутренней или внешней командой, исполняемой программой или пакетным файлом.\n"
"'%ls' is not recognized as an internal or external command, operable program or batch file.\n"
"'%ls' nebylo nalezeno jako vniřní nebo externí příkaz, spustitelná aplikace nebo dávkový soubor.\n"
"'%ls' nicht erkannt als interner oder externer Befehl, Programm oder Stapeldatei.\n"
""%ls" nem azonítható külső vagy belső parancsként, futtatható programként vagy batch fájlként.\n"
"Nie rozpoznano '%ls' jako polecenia, programu ani skryptu.\n"
"'%ls' no es reconocida como un comando interno o externo, programa operable o archivo de lotes.\n"
"'%ls' не є внутрішньою або зовнішньою командою, яку виконує програма або пакетний файл.\n"

MOpenPluginCannotOpenFile
l:
"Ошибка открытия файла"
"Cannot open the file"
"Nelze otevřít soubor"
"Kann Datei nicht öffnen"
"A fájl nem nyitható meg"
"Nie można otworzyć pliku"
"No se puede abrir el archivo"
"Помилка відкриття файлу"

MFileFilterTitle
l:
"Фильтр"
"Filter"
"Filtr"
"Filter"
"Felhasználói szűrő"
"Filtr wyszukiwania"
"Filtro"
"Фільтр"

MFileHilightTitle
"Раскраска файлов"
"Files highlighting"
"Zvýrazňování souborů"
"Farbmarkierungen"
"Fájlkiemelés"
"Zaznaczanie plików"
"Resaltado de archivos"
"Розмальовка файлів"

MFileFilterName
"Имя &фильтра:"
"Filter &name:"
"Jmé&no filtru:"
"Filter&name:"
"Szűrő &neve:"
"Nazwa &filtra:"
"&Nombre filtro:"
"Ім'я &фільтра:"

MFileFilterMatchMask
"&Маска:"
"&Mask:"
"&Maska"
"&Maske:"
"&Maszk:"
"&Maska:"
"&Máscara:"
"&Маска:"

MFileFilterSize
"Разм&ер:"
"Si&ze:"
"Vel&ikost"
"G&röße:"
"M&éret:"
"Ro&zmiar:"
"&Tamaño:"
"Розм&ір:"

MFileFilterSizeFromSign
">="
">="
">="
">="
">="
">="
">="
">="

MFileFilterSizeToSign
"<="
"<="
"<="
"<="
"<="
"<="
"<="
"<="

MFileFilterDate
"&Дата/Время:"
"Da&te/Time:"
"Dat&um/Čas:"
"Da&tum/Zeit:"
"&Dátum/Idő:"
"Da&ta/Czas:"
"&Fecha/Hora:"
"&Дата/Час:"

MFileFilterWrited
"&записи"
upd:"&write"
upd:"&write"
upd:"&write"
upd:"&write"
upd:"&write"
"&modificación"
"&записи"

MFileFilterCreated
"&создания"
"&creation"
"&vytvoření"
"E&rstellung"
"&Létrehozás"
"&utworzenia"
"&creación"
"&створення"

MFileFilterOpened
"&доступа"
"&access"
"&přístupu"
"Z&ugriff"
"&Hozzáférés"
"&dostępu"
"&acceso"
"&доступу"

MFileFilterChanged
"&изменения"
"c&hange"
upd:"c&hange"
upd:"c&hange"
upd:"c&hange"
upd:"c&hange"
upd:"c&hange"
"&зміни"

MFileFilterDateRelative
"Относительна&я"
"Relat&ive"
"Relati&vní"
"Relat&iv"
"Relat&ív"
"Relat&ive"
"Relat&ivo"
"Відносн&а"

MFileFilterDateAfterSign
">="
">="
">="
">="
">="
">="
">="
">="

MFileFilterDateBeforeSign
"<="
"<="
"<="
"<="
"<="
"<="
"<="
"<="

MFileFilterCurrent
"Теку&щая"
"C&urrent"
"Aktuá&lní"
"Akt&uell"
"&Jelenlegi"
"&Bieżący"
"Act&ual"
"Пото&чна"

MFileFilterBlank
"С&брос"
"B&lank"
"Práz&dný"
"&Leer"
"&Üres"
"&Wyczyść"
"En b&lanco"
"С&кидання"

MFileFilterAttr
"Атрибут&ы"
"Attri&butes"
"Attri&buty"
"Attri&bute"
"Attri&bútumok"
"&Atrybuty"
"Atri&butos"
"Атрибут&и"

MFileFilterAttrR
"&Только для чтения"
"&Read only"
"Jen pro čt&ení"
"Sch&reibschutz"
"&Csak olvasható"
"&Do odczytu"
"Sólo Lectu&ra"
"&Тільки для читання"

MFileFilterAttrA
"&Архивный"
"&Archive"
"Arc&hivovat"
"&Archiv"
"&Archív"
"&Archiwalny"
"&Archivo"
"&Архівний"

MFileFilterAttrH
"&Скрытый"
"&Hidden"
"Skry&tý"
"&Versteckt"
"&Rejtett"
"&Ukryty"
"&Oculto"
"&Скритий"

MFileFilterAttrS
"С&истемный"
"&System"
"Systémo&vý"
"&System"
"Re&ndszer"
"&Systemowy"
"&Sistema"
"С&истемний"

MFileFilterAttrC
"С&жатый"
"&Compressed"
"Kompri&movaný"
"&Komprimiert"
"&Tömörített"
"S&kompresowany"
"&Comprimido"
"С&тиснутий"

MFileFilterAttrE
"&Зашифрованный"
"&Encrypted"
"Ši&frovaný"
"V&erschlüsselt"
"T&itkosított"
"&Zaszyfrowany"
"Ci&frado"
"&Зашифрований"

MFileFilterAttrD
"&Каталог"
"&Directory"
"Adr&esář"
"Ver&zeichnis"
"Map&pa"
"&Katalog"
"&Directorio"
"&Каталог"

MFileFilterAttrNI
"&Неиндексируемый"
"Not inde&xed"
"Neinde&xovaný"
"Nicht in&diziert"
"Nem inde&xelt"
"Nie z&indeksowany"
"No inde&xado"
"&Неіндексований"

MFileFilterAttrSparse
"&Разрежённый"
"S&parse"
"Říd&ký"
"Reserve"
"Ritk&ított"
"S&parse"
"Escaso"
"&Розріджений"

MFileFilterAttrT
"&Временный"
"Temporar&y"
"Doča&sný"
"Temporär"
"Átm&eneti"
"&Tymczasowy"
"Tempora&l"
"&Тимчасовий"

MFileFilterAttrReparse
"Симво&л. ссылка"
"Symbolic lin&k"
"Sybolický li&nk"
"Symbolischer Lin&k"
"S&zimbolikus link"
"Link &symboliczny"
"Enlace simbólic&o"
"Симво&л. посилання"

MFileFilterAttrOffline
"Автономны&й"
"O&ffline"
"O&ffline"
"O&ffline"
"O&ffline"
"O&ffline"
"O&ffline"
"Автономни&й"

MFileFilterAttrVirtual
"Вирт&уальный"
"&Virtual"
"Virtuální"
"&Virtuell"
"&Virtuális"
"&Wirtualny"
"&Virtual"
"Вірт&уальний"

MFileFilterAttrExecutable
"Исполняемый"
"E&xecutable"
upd:"E&xecutable"
upd:"E&xecutable"
upd:"E&xecutable"
upd:"E&xecutable"
upd:"E&xecutable"
"Виконуваний"

MFileFilterAttrBroken
"Неисправный"
"&Broken"
upd:"&Broken"
upd:"&Broken"
upd:"&Broken"
upd:"&Broken"
upd:"&Broken"
"Несправний"

MFileFilterReset
"Очистит&ь"
"Reset"
"Reset"
"Rücksetzen"
"Reset"
"Wy&czyść"
"Reinicio"
"Очистит&и"

MFileFilterCancel
"Отмена"
"Cancel"
"Storno"
"Abbruch"
"Mégsem"
"&Anuluj"
"Cancelar"
"Відміна"

MFileFilterMakeTransparent
"Выставить прозрачность"
"Make transparent"
"Zprůhlednit"
"Transparent"
"Legyen átlátszó"
"Ustaw jako przezroczysty"
"Hacer transparente"
"Виставити прозорість"

MBadFileSizeFormat
"Неправильно заполнено поле размера"
"File size field is incorrectly filled"
"Velikost souboru neobsahuje správnou hodnotu"
"Angabe der Dateigröße ist fehlerhaft"
"A fájlméret mező rosszul van kitöltve"
"Rozmiar pliku jest niepoprawny"
"Campo de tamaño de archivo no está correctamente llenado"
"Неправильно заповнено поле розміру"

MFarTitleAddonsAdmin
l:
"root"
"root"
upd:"root"
upd:"root"
upd:"root"
upd:"root"
"root"
"root"

MAdminRequired
"Нужно обладать правами администратора"
"You need to provide administrator permission"
upd:"You need to provide administrator permission"
upd:"You need to provide administrator permission"
upd:"You need to provide administrator permission"
upd:"You need to provide administrator permission"
"Usted necesita permisos de administrador"
"Потрібно мати права адміністратора"

MAdminRequiredPrivileges
"Требуются дополнительные привилегии"
"Additional privileges required"
upd:"Additional privileges required"
upd:"Additional privileges required"
upd:"Additional privileges required"
upd:"Additional privileges required"
"Privilegios adicionales requeridos"
"Потрібні додаткові привілеї"

MAdminRequiredProcess
"для обработки этого объекта:"
"to process this object:"
upd:"to process this object:"
upd:"to process this object:"
upd:"to process this object:"
upd:"to process this object:"
"para procesar este objeto:"
"для обробки цього об'єкта:"

MAdminRequiredCreate
"для создания этого объекта:"
"to create this object:"
upd:"to create this object:"
upd:"to create this object:"
upd:"to create this object:"
upd:"to create this object:"
"para crear este objeto:"
"для створення цього об'єкта:"

MAdminRequiredDelete
"для удаления этого объекта:"
"to delete this object:"
upd:"to delete this object:"
upd:"to delete this object:"
upd:"to delete this object:"
upd:"to delete this object:"
"para eliminar este objeto:"
"для видалення цього об'єкта:"

MAdminRequiredCopy
"для копирования этого объекта:"
"to copy this object:"
upd:"to copy this object:"
upd:"to copy this object:"
upd:"to copy this object:"
upd:"to copy this object:"
"para copiar este objeto:"
"для копіювання цього об'єкта:"

MAdminRequiredMove
"для перемещения этого объекта:"
"to move this object:"
upd:"to move this object:"
upd:"to move this object:"
upd:"to move this object:"
upd:"to move this object:"
"para mover este objeto:"
"для переміщення цього об'єкта:"

MAdminRequiredGetAttributes
"для получения атрибутов этого объекта:"
"to get attributes of this object:"
upd:"to get attributes of this object:"
upd:"to get attributes of this object:"
upd:"to get attributes of this object:"
upd:"to get attributes of this object:"
"para obtener atributos de este objeto:"
"для отримання атрибутів цього об'єкта:"

MAdminRequiredSetAttributes
"для установки атрибутов этого объекта:"
"to set attributes of this object:"
upd:"to set attributes of this object:"
upd:"to set attributes of this object:"
upd:"to set attributes of this object:"
upd:"to set attributes of this object:"
"para poner atributos a este objeto:"
"для встановлення атрибутів цього об'єкта:"

MAdminRequiredHardLink
"для создания этой жёсткой ссылки:"
"to create this hard link:"
upd:"to create this hard link:"
upd:"to create this hard link:"
upd:"to create this hard link:"
upd:"to create this hard link:"
"para crear este enlace rígido:"
"для створення цього жорсткого посилання:"

MAdminRequiredSymLink
"для создания этой символической ссылки:"
"to create this symbolic link:"
upd:"to create this symbolic link:"
upd:"to create this symbolic link:"
upd:"to create this symbolic link:"
upd:"to create this symbolic link:"
"para crear este enlace simbólico:"
"для створення цього символічного посилання:"

MAdminRequiredRecycle
"для перемещения этого объекта в корзину:"
"to move this object to recycle bin:"
upd:"to move this object to recycle bin:"
upd:"to move this object to recycle bin:"
upd:"to move this object to recycle bin:"
upd:"to move this object to recycle bin:"
"para mover este objeto a la papelera:"
"для переміщення цього об'єкта в кошик:"

MAdminRequiredList
"для просмотра этого объекта:"
"to list this object:"
upd:"to list this object:"
upd:"to list this object:"
upd:"to list this object:"
upd:"to list this object:"
"para listar este objeto:"
"для перегляду цього об'єкта:"

MAdminRequiredSetOwner
"для установки владельца этого объекта:"
"to set owner of this object:"
upd:"to set owner of this object:"
upd:"to set owner of this object:"
upd:"to set owner of this object:"
upd:"to set owner of this object:"
"para poner como dueño de este objeto:"
"для встановлення власника цього об'єкта:"

MAdminRequiredOpen
"для открытия этого объекта:"
"to open this object:"
upd:"to open this object:"
upd:"to open this object:"
upd:"to open this object:"
upd:"to open this object:"
"para abrir este objeto:"
"для відкриття цього об'єкта:"

MAdminDoForAll
"Выполнить это действие для &всех текущих объектов"
"Do this for &all current objects"
upd:"Do this for &all current objects"
upd:"Do this for &all current objects"
upd:"Do this for &all current objects"
upd:"Do this for &all current objects"
"Hacer esto para todos los objetos actuales"
"Виконати цю дію для &всіх поточних об'єктів"

MAdminDoNotAskAgainInTheCurrentSession
"Больше не спрашивать в текущей сессии"
"Do not ask again in the current session"
upd:"Do not ask again in the current session"
upd:"Do not ask again in the current session"
upd:"Do not ask again in the current session"
upd:"Do not ask again in the current session"
"No preguntar nuevamente en la sesión actual"
"Більше не питати у поточній сесії"

MTerminalClipboardAccessTitle
"Доступ к буферу обмена"
"Clipboard access"
upd:"Clipboard access"
upd:"Clipboard access"
upd:"Clipboard access"
upd:"Clipboard access"
upd:"Clipboard access"
"Доступ до буфера обміну"

MTerminalClipboardAccessText
"Укажите как это приложение может пользоваться буфером обмена."
"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
upd:"Please choose how this terminal application may use clipboard."
"Вкажіть, як ця програма може користуватися буфером обміну."

MTerminalClipboardAccessBlock
"&Заблокировать"
"&Block attempt"
upd:"&Block attempt"
upd:"&Block attempt"
upd:"&Block attempt"
upd:"&Block attempt"
upd:"&Block attempt"
"&Заблокувати"

MTerminalClipboardAccessTemporaryRemote
"&Удаленный буфер"
"&Remote clipboard"
upd:"&Remote clipboard"
upd:"&Remote clipboard"
upd:"&Remote clipboard"
upd:"&Remote clipboard"
upd:"&Remote clipboard"
"&Віддалений буфер"

MTerminalClipboardAccessTemporaryLocal
"&Общий буфер"
"&Share clipboard"
upd:"&Share clipboard"
upd:"&Share clipboard"
upd:"&Share clipboard"
upd:"&Share clipboard"
upd:"&Share clipboard"
"&Спільний буфер"

MTerminalClipboardAccessAlwaysLocal
"Общий буфер всег&да"
"Share clipboard &always"
upd:"Share clipboard &always"
upd:"Share clipboard &always"
upd:"Share clipboard &always"
upd:"Share clipboard &always"
upd:"Share clipboard &always"
"Загальний буфер завж&ди"

MMountsRoot
"Корень"
"Root"
upd:"Root"
upd:"Root"
upd:"Root"
upd:"Root"
upd:"Root"
"Корінь"

MMountsHome
"Дом"
"Home"
upd:"Home"
upd:"Home"
upd:"Home"
upd:"Home"
upd:"Home"
"Дом"

MMountsOther
"Др. панель"
"Other panel"
upd:"Other panel"
upd:"Other panel"
upd:"Other panel"
upd:"Other panel"
upd:"Other panel"
"Інш. панель"

#Must be the last
MNewFileName
l:
"?Новый файл?"
"?New File?"
"?Nový soubor?"
"?Neue Datei?"
"?Új fájl?"
"?Nowy plik?"
"?Nuevoi Archivo?"
"?Новий файл?"
