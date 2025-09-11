import tkinter as tk
from tkinter import messagebox
import os
import json

# old path: ..\\!McChinaDev\\Minecraft.Windows.exe
version = '018'
configuration_file_name = 'config.json'
title = '[EEPL] AutoModificator v' + version
statusbar_text = 'MDLC organization. Developed by bs2jbwsgp4vsc4qg3ayn97nd'
disable_resize = True  # Change to False if u wanna resize window
minimum_width = 380  # For situations when the title of checkboxes is so short that the rest of the text no longer fits into the form

class PrimaryFrame(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title(title)

        # Disable resizing if needed
        if disable_resize:
            self.resizable(False, False)

        # Load the config file
        with open(configuration_file_name, 'r') as config_file:
            self.data = json.load(config_file)
            self.filename = os.path.join(*self.data["path"].split('\\'))

        # Create a panel (in Tkinter, this is just a Frame)
        self.panel = tk.Frame(self)
        self.panel.pack(fill=tk.BOTH, expand=True)

        # Create a status bar
        self.statusbar = tk.Label(self, text=statusbar_text, bd=1, relief=tk.SUNKEN, anchor=tk.W)
        self.statusbar.pack(side=tk.BOTTOM, fill=tk.X)

        # Create checkboxes based on the data
        checkbox_count = len(self.data["switches"])
        self.checkboxes = []
        grid_frame = tk.Frame(self.panel)
        grid_frame.pack(pady=10)

        with open(self.filename, 'rb') as file:
            for i in range(checkbox_count):
                patches = []
                defaults = []
                for a in range(len(self.data["switches"][i]["patches"])):
                    file.seek(int(self.data["switches"][i]["patches"][a].split(' >> ')[0], 16))
                    hex_value = file.read(1).hex().upper()
                    patches.append(hex_value == self.data["switches"][i]["patches"][a].split(' >> ')[2])
                    defaults.append(hex_value == self.data["switches"][i]["patches"][a].split(' >> ')[1])
                
                result = all(patches)
                checkbox = tk.Checkbutton(grid_frame, text=self.data["switches"][i]['title'])
                checkbox.grid(row=i // 2, column=i % 2, padx=5, pady=5)
                self.checkboxes.append((checkbox, i))

                # Set the initial state of the checkbox
                var = tk.BooleanVar(value=result)
                checkbox.config(variable=var)
                checkbox.var = var

                # Disable the checkbox if values are invalid
                if not result and not all(defaults):
                    messagebox.showerror("Error", "Invalid values found")
                    checkbox.config(state=tk.DISABLED)

        # Calculate and set window size
        self.CalculateWindowSize()

        # Bind checkboxes to event
        for checkbox, i in self.checkboxes:
            checkbox.config(command=lambda i=i: self.onChecked(i))

    def CalculateWindowSize(self):
        longest_label_width = max([checkbox.winfo_reqwidth() for checkbox, _ in self.checkboxes])
        total_width = longest_label_width * 2 + 60
        total_height = sum([checkbox.winfo_reqheight() for checkbox, _ in self.checkboxes]) + 100
        if total_width < minimum_width:
            total_width = minimum_width
        self.geometry(f"{total_width}x{total_height}")

    def onChecked(self, id):
        # Get the checkbox and its value
        checkbox, _ = self.checkboxes[id]
        value = checkbox.var.get()

        # Apply hex modifications based on the checkbox value
        patches = self.data["switches"][id]["patches"]
        for i in range(len(patches)):
            self.replace_hex(patches[i].split(' >> ')[0], patches[i].split(' >> ')[2 if value else 1])

    def replace_hex(self, address, hex_value):
        with open(self.filename, 'r+b') as file:
            file.seek(int(address, 16))
            file.write(bytes.fromhex(hex_value))
            file.flush()

# Run the application
if __name__ == "__main__":
    app = PrimaryFrame()
    app.mainloop()
    
#Port to Tkinter was done via ChatGPT-4o 25.09.2024