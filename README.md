# AutoModificator

RU

Python программа для изменения файлов. Изменения вносятся исходя из данных указанных в файле config.txt лежащим рядом с exe AutoModificator после нажатия рычагов в окне программы.
В файле конфига первая строка - это путь указывающий на файл, который будет редактироваться, который можно изменить на нужный (путь записывается в питоновской форме).
2 и 3 строки - принадлежат первому рычагу в окне программы.
4 и 5 строки - принадлежат второму рычагу в окне программы.
в строках 2, 3, 4, 5 нужно указывать hex смещение/адрес того места где нужно изменить байты, перед смещением вы должны написать - (0x), далее после ( >> ) указывается исходные байты в hex форме находящиеся по указанному смещению/адресу, затем после второго ( >> ) указывается те байты, на которые нужно изменить исходное значение. Пример записи файла config.txt используемого в реальном проекте:

'..','!McChinaDev','unpacked_Minecraft.Windows.exe'

0x2A9ACDE >> 0C >> FF
0x2A9AF70 >> 0C >> FF
0x396A191 >> 80 >> 00
0x396A192 >> 00 >> 01

( Штука ('..') в пути - это означает - одну папку назад).
Программа может быть применена практически для любого файла, но используется в основном для Minecraft Bedrock Edition. Запускать изменения в отношении исполняемого файла нужно когда он выключен, что в принципе очевидно.

EN

A Python program for changing files. Changes are made based on the data specified in the file config.txt located in same folder with AutoModificator exe after pressing the switchers in the program window.
In the config file, the first line is the path specified the file that should be edited. Path can be changed to the desired one (the path is written in python form).
Lines 2 and 3 belong to the first switch in the program window.
Lines 4 and 5 belong to the second switch in the program window.
in lines 2, 3, 4, 5, you need to specify the hex offset/address of the place where the bytes need to be changed, before the offset you must write - (0x), then after ( >> ) indicate the original bytes in hex form located at the specified offset/address, then after the second ( >> ) indicate those bytes to change the original value to. Example of writing a file config.txt used in a real project:

'..','!McChinaDev','unpacked_Minecraft.Windows.exe'
0x2A9ACDE >> 0C >> FF
0x2A9AF70 >> 0C >> FF
0x396A191 >> 80 >> 00
0x396A192 >> 00 >> 01

(The thing ('..') in the path means one folder back).
The program can be applied to almost any file, but is mainly used for Minecraft Bedrock Edition. You need to run changes to the executable file when it is turned off, which is basically obvious.
