import wx, os, json, array
# old path: ..\\!McChinaDev\\Minecraft.Windows.exe
version = '016'
configuration_file_name = 'config.json'

class PrimaryFrame(wx.Frame):
    def __init__(self):
        super().__init__(parent=None, title='EPPL_Auto_Modificator_v' + version, size=(480, 200))
        with open(configuration_file_name, 'r') as config_file:
            self.data = json.load(config_file)
            self.filename = os.path.join(*self.data["path"].split('\\'))

        self.panel = wx.Panel(self)
        with open(self.filename, 'rb') as file:
            for i in range(len(self.data["switches"])): 
              patches = array.array('b')
              defaults = array.array('b')
              for a in range(len(self.data["switches"][i]["patches"])):
                file.seek(int(self.data["switches"][i]["patches"][a].split(' >> ')[0], 16))
                hex_value = file.read(1).hex().upper()
                patches.append(hex_value == self.data["switches"][i]["patches"][a].split(' >> ')[2])
                defaults.append(hex_value == self.data["switches"][i]["patches"][a].split(' >> ')[1])
              result = all(x for x in patches) 
              checkbox = wx.CheckBox(self.panel, id=i, label=self.data["switches"][i]['title'], pos=(10, 10 + (i * 30)))
              checkbox.SetValue(result)
              if not result and not all(x for x in defaults):
                 wx.MessageBox("Error: Invalid values found", "Error", style=wx.OK | wx.ICON_ERROR)
                 checkbox.Disable()
        wx.StaticText(self.panel, label='MDLC organization. Developed by bs2jbwsgp4vsc4qg3ayn97nd (DiscordUsername)', pos=(10, 140))
        self.Bind(wx.EVT_CHECKBOX, self.onChecked)

    def onChecked(self, e):
        cb = e.GetEventObject()
        id = cb.GetId()
        value = cb.GetValue()
        label = cb.GetLabel() # if you want print message like "${label} was activated!"
        patches = self.data["switches"][id]["patches"]
        for i in range(len(patches)):
            self.replace_hex(patches[i].split(' >> ')[0], patches[i].split(' >> ')[2 if value else 1])

    def replace_hex(self, address, hex_value):
        with open(self.filename, 'r+b') as file:
            file.seek(int(address, 16))
            file.write(bytes.fromhex(hex_value))
            file.flush()

app = wx.App()
PrimaryFrame().Show()
app.MainLoop()