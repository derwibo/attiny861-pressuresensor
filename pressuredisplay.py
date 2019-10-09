import os
import termios
import tkinter as tk
import threading
import struct

#termatt = [0, 4, 3261, 35384, termios.B38400, termios.B38400, [b'\x03', b'\x1c', b'\x7f', b'\x15', b'\x04', 0, 1, b'\x00', b'\x11', b'\x13', b'\x1a', b'\x00', b'\x12', b'\x0f', b'\x17', b'\x16', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00']]
termatt = [0, 4, 3261, 35384, termios.B19200, termios.B19200, [b'\x03', b'\x1c', b'\x7f', b'\x15', b'\x04', 0, 1, b'\x00', b'\x11', b'\x13', b'\x1a', b'\x00', b'\x12', b'\x0f', b'\x17', b'\x16', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00', b'\x00']]

ttydev = '/dev/ttyUSB0'

class MainApp:
    def __init__(self):
        self.tk = tk.Tk()
        self.tk.title("Pressure View")
        self.tty = None
        self.commthread = None
    def run(self):
        self.tty = os.open(ttydev, os.O_RDONLY)
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

        self.font = tk.font.Font(family='Helvetica', size=24)

        #self.grid_rowconfigure(0, weight=1)
        #self.grid_columnconfigure(0, weight=1)
        
        self.msgcount = tk.StringVar()
        self.msgcount.set('0')
        tk.Label(self, text='Msg-Nr.:', font=self.font).grid(column=0, row=0, sticky=tk.N+tk.W+tk.E+tk.S)
        tk.Label(self, textvariable=self.msgcount, font=self.font).grid(column=1, row=0, sticky=tk.N+tk.W+tk.E+tk.S)

        self.chn1 = tk.StringVar()
        self.chn1.set('0')
        tk.Label(self, text='Kanal 1', font=self.font).grid(column=0, row=1, sticky=tk.N+tk.W+tk.S)
        tk.Label(self, textvariable=self.chn1, font=self.font).grid(column=1, row=1, sticky=tk.N+tk.E+tk.S)
        tk.Label(self, text='bar', font=self.font).grid(column=2, row=1, sticky=tk.N+tk.W+tk.S)

        self.chn2 = tk.StringVar()
        self.chn2.set('0')
        tk.Label(self, text='Kanal 2', font=self.font).grid(column=0, row=2, sticky=tk.N+tk.W+tk.S)
        tk.Label(self, textvariable=self.chn2, font=self.font).grid(column=1, row=2, sticky=tk.N+tk.E+tk.S)
        tk.Label(self, text='bar', font=self.font).grid(column=2, row=2, sticky=tk.N+tk.W+tk.S)

        self.chn3 = tk.StringVar()
        self.chn3.set('0')
        tk.Label(self, text='Kanal 3', font=self.font).grid(column=0, row=3, sticky=tk.N+tk.W+tk.S)
        tk.Label(self, textvariable=self.chn3, font=self.font).grid(column=1, row=3, sticky=tk.N+tk.E+tk.S)
        tk.Label(self, text='bar', font=self.font).grid(column=2, row=3, sticky=tk.N+tk.W+tk.S)

        self.chn4 = tk.StringVar()
        self.chn4.set('0')
        tk.Label(self, text='Kanal 4', font=self.font).grid(column=0, row=4, sticky=tk.N+tk.W+tk.S)
        tk.Label(self, textvariable=self.chn4, font=self.font).grid(column=1, row=4, sticky=tk.N+tk.E+tk.S)
        tk.Label(self, text='bar', font=self.font).grid(column=2, row=4, sticky=tk.N+tk.W+tk.S)
        

class CommThread(threading.Thread):
    def __init__(self, owner, tty):
        threading.Thread.__init__(self)
        self.owner = owner
        self.tty   = tty
        self.exit  = False
    def run(self):
        self.exit = False
        cnt = 0
        dDampTime = 0.2
        dScriptRate = 0.1
        dTstern = 1.0 / ((dDampTime / dScriptRate) + 1.0);
        pd1 = 0.0
        pd2 = 0.0
        pd3 = 0.0
        pd4 = 0.0
        while(not self.exit):
            c = os.read(self.tty, 1)
            if len(c) > 0 and c[0] == 0x12 :
                n = os.read(self.tty, 1)
                bindata = bytearray(b'')
                while len(bindata) < 8 :
                    bindata.extend(os.read(self.tty, 1))
                if len(bindata) == 8 :
                    # print('Data received')
                    data = struct.unpack('HHHH', bindata)
                    cnt = cnt + 1
                    p1 = 250.0 / 640.0 * (float(data[0]) - 160.0)
                    p2 = 250.0 / 640.0 * (float(data[1]) - 160.0)
                    p3 = 250.0 / 640.0 * (float(data[2]) - 160.0)
                    p4 = 250.0 / 640.0 * (float(data[3]) - 160.0)
                    #pd1 = dTstern * (p1- pd1) + pd1
                    #pd2 = dTstern * (p2- pd2) + pd2
                    #pd3 = dTstern * (p3- pd3) + pd3
                    #pd4 = dTstern * (p4- pd4) + pd4
                    string = '%d - ' % cnt + '%d %d %d %d' % data
                    if not self.exit :
                        self.owner.mainwnd.msgcount.set('%d' % cnt)
                        self.owner.mainwnd.chn1.set('%5.2f' % p1)
                        self.owner.mainwnd.chn2.set('%5.2f' % p2)
                        self.owner.mainwnd.chn3.set('%5.2f' % p3)
                        self.owner.mainwnd.chn4.set('%5.2f' % p4)
                else:
                    print('Invalid length: {0:d}'.format(len(bindata)))
                    
                
                
                
            
        
app = MainApp()
app.run()


