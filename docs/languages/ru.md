## Инструкция на Русском

> Программа может быть применена практически для любого файла, но используется в основном для Minecraft Bedrock Edition. Запускать изменения в отношении исполняемого файла нужно когда он выключен, что в принципе очевидно.

Изменения происходят после нажатия переключателей в окне программы на основании данных, указанных в файле ``config.json``, который находится рядом с ``AutoModificator.exe``.

Файл конфигурации формата JSON может содержать в себе путь к редактируемой программе и настройки переключателей в интерфейсе которые подразумевают под собой название и патчи применяемые переключателем.

Чтобы открыть Json файл используя стандартный текстовый редактор блокнот:

> Обратите внимание, пример уканазый ниже содержит комментарии которые не могут содержаться в настоящем файле конфигурации и будут вызывать ошибку.
```json
{
    // Путь к редактируемой программе
    "path": "..\\Minecraft.Windows.exe", 

    // Настройки перекючателей
    "switches": [
      {
        "title": "Switch 1", // Название переключателя
        "patches": [ // Применяемые патчи
          "0x31D5EBC >> 0C >> FF",
          "0x31D665B >> 0C >> FF"
        ]
      },
      // Пример добавления переключателя
      {
        "title": "Switch 2",
        "patches": [
          "0x3D49457 >> 80 >> 00",
          "0x3D49458 >> 00 >> 08".
          "0x3D49456 >> 00 >> FF"
        ]
      }
    ]
}
```

Двоеточие ('..') в пути означает родительскую папку (одну папку назад). В пути до изменяемого файла нужно писать не 1 бак-слеш, а 2, пример: "..\\!McChinaDev\\unpacked_Minecraft.Windows.exe". Если файл, который будет изменятся программой находится в одной папке с ней, то в пути нужно просто указать имя файла, пример: "Minecraft.Windows.exe". Количество патчей на один переключатель неограничено, чтобы написать свой патч нужно указать hex смещение/адрес того места где нужно изменить байты, далее после ( >> ) указываются исходные байты в формате hex, находящиеся по указанному смещению/адресу, затем после второго ( >> ) указывается те байты, на которые нужно изменить исходное значение.

<table style="border-collapse:collapse;border-spacing:0" class="tg"><thead><tr><th style="background-color:#efefef;border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;font-weight:normal;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal" colspan="5"><span style="font-weight:bold">Патч</span></th></tr></thead><tbody><tr><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal">Адрес</td><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal" colspan="2">Исходные байты</td><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal" colspan="2">Целевые байты</td></tr><tr><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:left;vertical-align:top;word-break:normal">0x3D49457</td><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:left;vertical-align:top;word-break:normal">&gt;&gt;</td><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:left;vertical-align:top;word-break:normal">80</td><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:left;vertical-align:top;word-break:normal">&gt;&gt;</td><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:left;vertical-align:top;word-break:normal">00</td></tr></tbody></table>



Примеры конфигов и их визуализация:
1) Вообще без переключателей:
```
{
    "path": "..\\!McChinaDev\\unpacked_Minecraft.Windows.exe",
    "switches": [
    ]
}

```
Скриншот окна программы: 

2) Один переключатель и 1 Hex смещение:
```
{
    "path": "Minecraft.Windows.exe",
    "switches": [
      {
        "title": "Patch The End height imit",
        "patches": [
          "0x3D50A29 >> 01 >> 08"
        ]
      }
    ]
}
```
Скриншот окна программы: 
В "title": "Patch The End height imit", указано имя переключателя, 
которое будет отображаться в окне программы - Patch The End height imit 

3) 2 переключателя и 2 Hex смещения в каждом из них:
```
{
    "path": "..\\!McChinaDev\\unpacked_Minecraft.Windows.exe",
    "switches": [
      {
        "title": "Enable/Disable the EPPL Mod",
        "patches": [
          "0x2A9ACDE >> 0C >> FF",
          "0x2A9AF70 >> 0C >> FF"
        ]
      },
      {
        "title": "Patch Nether height limit",
        "patches": [
          "0x396A191 >> 80 >> 00",
          "0x396A192 >> 00 >> 08"
        ]
      }
    ]
}
```
Скриншот окна программы: 

4) 3 переключателя, 2 из них имеют по 2 Hex смещения, а один имеет 1 Hex смещение:
```
{
    "path": "Minecraft.Windows.exe",
    "switches": [
      {
        "title": "Enable/Disable the EPPL Mod",
        "patches": [
          "0x31D5EBC >> 0C >> FF",
          "0x31D665B >> 0C >> FF"
        ]
      },
      {
        "title": "Patch The End height imit",
        "patches": [
          "0x3D50A29 >> 01 >> 08"
        ]
      },
      {
        "title": "Patch Nether height limit",
        "patches": [
          "0x3D49457 >> 80 >> 00",
          "0x3D49458 >> 00 >> 08"
        ]
      }
    ]
}
```
Скриншот окна программы:

5) 1 переключатель и много Hex смещений:
```
{
    "path": "Minecraft.Windows.exe",
    "switches": [
      {
        "title": "Enable/Disable the EPPL Mod",
        "patches": [
          "0x31D5EBC >> 0C >> FF",
          "0x31D5EBC >> 0C >> FF",
          "0x31D5EBC >> 0C >> FF",
          "0x31D5EBC >> 0C >> FF",
          "0x31D5EBC >> 0C >> FF",
          "0x31D665B >> 0C >> FF"
        ]
      }
    ]
}
```
Скриншот окна программы:
Обратите внимание, что все Hex смещения кроме последнего имеют запятую в конце строки. Её обязательно нужно ставить или будет ошибка Json формата.

Блоки на каждый переключатель выглядят например так:
``` 
     {
        "title": "Enable/Disable the EPPL Mod",
        "patches": [
          "0x31D5EBC >> 0C >> FF",
          "0x31D665B >> 0C >> FF"
        ]
      },
      {
        "title": "Patch The End height imit",
        "patches": [
          "0x3D50A29 >> 01 >> 08"
        ]
      },
      {
        "title": "Patch Nether height limit",
        "patches": [
          "0x3D49457 >> 80 >> 00",
          "0x3D49458 >> 00 >> 08"
        ]
      }
```
Отдельный блок выглядит примерно так:
```
     {
        "title": "Enable/Disable the EPPL Mod",
        "patches": [
          "0x31D5EBC >> 0C >> FF",
          "0x31D665B >> 0C >> FF"
        ]
      },
```
После каждого блока в конце идёт запятая, чтобы Json формат не нарушался.
Последний блок записывается без запятой в конце.
```
      {
        "title": "Patch Nether height limit",
        "patches": [
          "0x3D49457 >> 80 >> 00",
          "0x3D49458 >> 00 >> 08"
        ]
      }
```

Если возникают трудности с написанием своих файлов конфига, то ещё раз просмторите примеры или возьмите их за основу, а потом отредактируйте их на свой лад. Так-же в случае возникновения проблем с написанием своего собственного файла конфига рекомендуется изучить это: https://ru.wikipedia.org/wiki/JSON#%D0%A1%D0%B8%D0%BD%D1%82%D0%B0%D0%BA%D1%81%D0%B8%D1%81