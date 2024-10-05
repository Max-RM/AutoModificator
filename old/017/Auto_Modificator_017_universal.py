import wx, os, json
# old path: ..\\!McChinaDev\\Minecraft.Windows.exe
version = '017'
configuration_file_name = 'config.json'
title = '[EEPL] AutoModificator v' + version
statusbar_text = 'MDLC organization. Developed by bs2jbwsgp4vsc4qg3ayn97nd'
disable_resize = True # Change to False if u wanna resize window
minimum_width = 380 # For situations when the title of checkboxes is so short that the rest of the text no longer fits into the form

class PrimaryFrame(wx.Frame):
    def __init__(self):
        with open(configuration_file_name, 'r') as config_file:
            self.data = json.load(config_file)
            self.filename = os.path.join(*self.data["path"].split('\\'))
        if (disable_resize): super().__init__(parent=None, title=title, style=wx.DEFAULT_FRAME_STYLE & ~(wx.RESIZE_BORDER | wx.MAXIMIZE_BOX))
        else: super().__init__(parent=None, title=title)
        self.panel = wx.Panel(self)
        self.statusbar = self.CreateStatusBar()
        self.statusbar.SetFieldsCount(1)
        self.statusbar.SetStatusText(statusbar_text, 0)
        checkbox_count = len(self.data["switches"])
        panel_sizer = wx.BoxSizer(wx.VERTICAL)
        grid_sizer = wx.GridSizer(rows=(checkbox_count + 1) // 2 , cols=2, hgap=5, vgap=5)
        self.checkboxes = []
        with open(self.filename, 'rb') as file:
            for i in range(len(self.data["switches"])): 
              patches = []
              defaults = []
              for a in range(len(self.data["switches"][i]["patches"])):
                file.seek(int(self.data["switches"][i]["patches"][a].split(' >> ')[0], 16))
                hex_value = file.read(1).hex().upper()
                patches.append(hex_value == self.data["switches"][i]["patches"][a].split(' >> ')[2])
                defaults.append(hex_value == self.data["switches"][i]["patches"][a].split(' >> ')[1])
              result = all(x for x in patches) 
              checkbox = wx.CheckBox(self.panel, id=i, label=self.data["switches"][i]['title'])
              grid_sizer.Add(checkbox, 0, wx.EXPAND)
              self.checkboxes.append(checkbox)
              checkbox.SetValue(result)
              if not result and not all(x for x in defaults):
                 wx.MessageBox("Error: Invalid values found", "Error", style=wx.OK | wx.ICON_ERROR)
                 checkbox.Disable()
        panel_sizer.Add(grid_sizer, 0, wx.EXPAND | wx.ALL, border=10)
        self.panel.SetSizer(panel_sizer)
        self.CalculateWindowSize()
        self.Bind(wx.EVT_CHECKBOX, self.onChecked)

    def CalculateWindowSize(self):
        longest_label_width = max([checkbox.GetBestSize().GetWidth() for checkbox in self.checkboxes])
        total_width = longest_label_width * 2 + 60
        total_height = sum([checkbox.GetBestSize().GetHeight() for checkbox in self.checkboxes]) + 100
        if total_width < minimum_width: total_width = minimum_width
        self.SetSize((total_width, total_height))

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