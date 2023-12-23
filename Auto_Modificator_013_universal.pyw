import wx
import os

class MyFrame(wx.Frame):
    def __init__(self):
        super().__init__(parent=None, title='EPPL_Auto_Modificator_v013', size=(350, 200))

        # Считываем данные из файла конфига
        with open('config.txt', 'r') as config_file:
            lines = config_file.readlines()
            # Преобразуем первую строку в кортеж и сохраняем в переменную
            self.wariable0 = tuple(map(lambda x: x.strip("' ,"), lines[0].strip().split(',')))

        # Создаем путь к файлу используя переменную
        self.filename = os.path.join(*self.wariable0)

        self.panel = wx.Panel(self)
        self.checkbox = wx.CheckBox(self.panel, label='Enable/Disable the EPPL Mod', pos=(10, 10))
        self.checkbox.Bind(wx.EVT_CHECKBOX, self.on_checkbox_toggle)
        self.checkbox2 = wx.CheckBox(self.panel, label='Patch Nether height', pos=(10, 40))
        self.checkbox2.Bind(wx.EVT_CHECKBOX, self.on_checkbox2_toggle)

        static_text = wx.StaticText(self.panel, label='MDLC organization. Developed by MaxRM using ChatGPT', pos=(10, 140))

        self.read_config_data()
        self.update_checkbox_state()
        self.update_checkbox2_state()

    def read_config_data(self):
        # Считываем остальные данные из файла конфига
        with open('config.txt', 'r') as config_file:
            lines = config_file.readlines()
            self.config_data = [line.strip().split(' >> ') for line in lines[1:]]

    def update_checkbox_state(self):
        with open(self.filename, 'rb') as file:
            file.seek(int(self.config_data[0][0], 16))
            hex_value1 = file.read(1).hex().upper()
            file.seek(int(self.config_data[1][0], 16))
            hex_value2 = file.read(1).hex().upper()
            if hex_value1 == self.config_data[0][2] and hex_value2 == self.config_data[1][2]:
                self.checkbox.SetValue(True)
            elif hex_value1 == self.config_data[0][1] and hex_value2 == self.config_data[1][1]:
                self.checkbox.SetValue(False)
            else:
                self.checkbox.Disable()
                wx.MessageBox("Error: Invalid values found", "Error", style=wx.OK | wx.ICON_ERROR)
                self.Close()

    def update_checkbox2_state(self):
        with open(self.filename, 'rb') as file:
            file.seek(int(self.config_data[2][0], 16))
            hex_value2_1 = file.read(1).hex().upper()
            file.seek(int(self.config_data[3][0], 16))
            hex_value2_2 = file.read(1).hex().upper()
            if hex_value2_1 == self.config_data[2][2] and hex_value2_2 == self.config_data[3][2]:
                self.checkbox2.SetValue(True)
            elif hex_value2_1 == self.config_data[2][1] and hex_value2_2 == self.config_data[3][1]:
                self.checkbox2.SetValue(False)
            else:
                self.checkbox2.Disable()
                wx.MessageBox("Error: Invalid values found for second checkbox", "Error", style=wx.OK | wx.ICON_ERROR)
                self.Close()

    def replace_hex(self, address, hex_value):
        with open(self.filename, 'r+b') as file:
            file.seek(int(address, 16))
            file.write(bytes.fromhex(hex_value))
            file.flush()

    def on_checkbox_toggle(self, event):
        if self.checkbox.GetValue():
            self.replace_hex(self.config_data[0][0], self.config_data[0][2])
            self.replace_hex(self.config_data[1][0], self.config_data[1][2])
        else:
            self.replace_hex(self.config_data[0][0], self.config_data[0][1])
            self.replace_hex(self.config_data[1][0], self.config_data[1][1])
        self.update_checkbox_state()

    def on_checkbox2_toggle(self, event):
        if self.checkbox2.GetValue():
            self.replace_hex(self.config_data[2][0], self.config_data[2][2])
            self.replace_hex(self.config_data[3][0], self.config_data[3][2])  # Added line to update the second value
            self.update_checkbox2_state()
        else:
            self.replace_hex(self.config_data[2][0], self.config_data[2][1])
            self.replace_hex(self.config_data[3][0], self.config_data[3][1])  # Added line to update the second value
            self.update_checkbox2_state()

app = wx.App()
frame = MyFrame()
frame.Show()
app.MainLoop()
# Создано MaxRM с использованием ChatGPT
