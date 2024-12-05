# Сборка и установка драйвера
Должна быть "общая/удалённая" `shared\portio` папка для VBox:
```
e:

cd portio\sys
```
Сборка исходников:
```
build -ceZ
```

Установка драйвера
```
c:\WinDDK\7600.16385.0\tools\devcon\i386\devcon.exe INSTALL objchk_wxp_x86\i386\genport.inf "root\portio"
```

Проверка статуса сборки
```
sc query portio 
```

Удаление драйвера
```
c:\WinDDK\7600.16385.0\tools\devcon\i386\devcon.exe remove "root\portio"
```

# Сборка пользовательской программы
Нам понадобится сборщик под 32-битную систему(к примеру `MINGW32`, который идёт вместе с `MSYS2 MSYS`)
```
gcc test.c -o test.exe
```

Запуск
```
.\test.exe
```

# Чтение из \\.\pipe\VMCOM2
Скрипт `read_port.py`
```
python read_port.py
```

