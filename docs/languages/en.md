## Instructions in English

> The program can patch any file, but it is mainly used for Minecraft Bedrock Edition. You can write changes to the executable file only when it is not in use.

Changes occur after clicking switches in the program window based on the data specified in the ``config.json`` file, which is located next to ``AutoModificator.exe``

In the JSON file you can store the path to the program being edited and the settings of the switches in the interface, which imply the name and patches applied by the switch

> Please note that the example below contains comments that cannot be saved in the given file configuration and will generate an error.
```json
{
      // Path to the edited program
      "path": "..\\Minecraft.Windows.exe",

      // Switch settings
      "switches": [
        {
          "title": "Switch 1", // Switch title
          "patches": [ // Patches to apply
            "0x31D5EBC >> 0C >> FF",
            "0x31D665B >> 0C >> FF"
          ]
        },
        // Example of adding a switch
        {
          "title": "Switch 2",
          "patches": [
            "0x3D49457 >> 80 >> 00",
            "0x3D49458 >> 00 >> 08."
            "0x3D49456 >> 00 >> FF"
          ]
        }
      ]
}
```

A colon ('..') in the path indicates the parent folder (one folder back). The number of patches per switch is unlimited, to write your own patch you need to specify the hex offset/address of the place where you want to change the bytes, then after ( >> ) the original bytes in hex format located at the specified offset/address are indicated, then after the second ( >> ) indicates the bytes to which you want to change the original value.

<table style="border-collapse:collapse;border-spacing:0" class="tg"><thead><tr><th style="background-color:#efefef;border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;font-weight:normal;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal" colspan="5"><span style="font-weight:bold">Patch</span></th></tr></thead><tbody><tr><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal">Address</td><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal" colspan="2">Original value</td><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:center;vertical-align:top;word-break:normal" colspan="2">Result</td></tr><tr><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:left;vertical-align:top;word-break:normal">0x3D49457</td><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:left;vertical-align:top;word-break:normal">&gt;&gt;</td><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:left;vertical-align:top;word-break:normal">80</td><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:left;vertical-align:top;word-break:normal">&gt;&gt;</td><td style="border-color:#c0c0c0;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;overflow:hidden;padding:10px 5px;text-align:left;vertical-align:top;word-break:normal">00</td></tr></tbody></table>