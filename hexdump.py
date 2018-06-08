import os
import termios
import tkinter as tk
import threading
import struct

termatt = [0, 4, 3261, 35384, 13, 13, [b'\x03', b'\x1c', b'\x7f', b'\x15', b'\x04', 0, 1, b'\x00', b'\x11', b'\x13', b'\x1a', b'\x00', b'\x12', b'\x0f', b'\x17', b'\x16', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00']]

class MainApp:
    def __init__(self):
        self.tk = tk.Tk()
        self.tk.title("TTY-Hexdump")
        self.tty = None
        self.commthread = None
    def run(self):
        self.tty = os.open('/dev/ttyUSB1', os.O_RDONLY)
        attold = termios.tcgetattr(self.tty)
        termios.tcsetattr(self.tty, termios.TCSANOW, termatt)
        self.mainwnd = MainWnd(self.tk, self)
        self.mainwnd.pack(fill=tk.BOTH, expand=1)
        self.tk.protocol("WM_DELETE_WINDOW", self.mainwnd.quit)
        self.commthread = CommThread(self, self.tty)
        self.commthread.start()
        self.mainwnd.mainloop()
        self.commthread.exit = True
        self.tk.destroy()
        self.commthread.join(5.0)
        termios.tcsetattr(self.tty, termios.TCSANOW, attold)
        os.close(self.tty)

class MainWnd(tk.Frame):
    def __init__(self, parent, app):
        tk.Frame.__init__(self, parent)
        self.app = app

        #self.grid_rowconfigure(0, weight=1)
        #self.grid_columnconfigure(0, weight=1)
        
        self.textedit = tk.Text(self)
        self.textedit.pack()
        self.textedit.insert(tk.END, 'Hello world\n')

class CommThread(threading.Thread):
    def __init__(self, owner, tty):
        threading.Thread.__init__(self)
        self.owner = owner
        self.tty   = tty
        self.exit  = False
    def run(self):
        self.exit = False
        while(not self.exit):
            bindata = bytearray(b'')
            while len(bindata) < 10 :
                bindata.extend(os.read(self.tty, 1))
            if len(bindata) == 10 :
                print('Data received')
                data = struct.unpack('BBBBBBBBBB', bindata)
                string = '%x %x %x %x %x %x %x %x %x %x\n' % data
                if not self.exit :
                    self.owner.mainwnd.textedit.insert(tk.END, string)
            else:
                print('Invalid length: {0:d}'.format(len(bindata)))
                    
                
                
                
            
        
app = MainApp()
app.run()


