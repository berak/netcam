#!/usr/bin/env python

import sys
import time
import socket
import threading

import Image

import ImageTk
import Tkinter

try:
    from io import BytesIO
except ImportError:
    from StringIO import StringIO as BytesIO



class WebListener( threading.Thread ):
	def __init__(self, port = 7777, host = '192.168.0.1', uri = '/'):
		threading.Thread.__init__(self)
		self.host = host
		self.uri = uri
		self.port = port
		self.img = None
		
	def readline(self):
		s = BytesIO()
		while self.web:
			c = self.web.recv(1)
			if not c:
				break
			elif c==b'\r':
				continue
			elif c==b'\n':
				break
			else:
				s.write(c)
		return s.getvalue()

	def readhead( self ):
		nb = 0
		while self.web:
			s = self.readline();
			c = "Content-Length: "
			if s.find(c)==0:
				nb = int(s[len(c):])
			print "#",s
			if not s : return nb
								
	def run( self ):
		i = 0
		self.web = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		print self.host, str(self.web)
		try:
			self.web.connect((self.host, self.port))
		except:
			print "no connection to " + str(self.host) + " " + str(self.port)
			return
		self.web.sendall( "GET "+self.uri+" HTTP/1.1\r\nHOST:"+self.host+"\r\n\r\n" )
		self.readhead()
		while self.web:
			try:
				nb = self.readhead()
				if nb == 0:
					return
				res = BytesIO()
				while ( nb>0 ):
					c = self.web.recv(nb);
					if not c : break
					res.write(c)
					nb -= len(c)
					print len(c), str(c[:20])
				try:
					self.img = Image.open(BytesIO(res.getvalue()))					
				except:
					print "invalid result"
					pass
				print i, nb,nr
				#~ self.img.save("./my.jpg");
				time.sleep(0.02)
				i += 1
			except:
				return



class VizThread( threading.Thread ):
	def __init__(self, web):
		threading.Thread.__init__(self)
		self.web = web
		self.tick = 0

	def run(self):
		self.root = Tkinter.Tk()
		self.root.bind("<Escape>", lambda a:self._quit() )
		self.root.geometry("330x250")
		self.label = Tkinter.Label(self.root)
		self.label.pack()
		self.poll()
		self.root.mainloop()
		
	def _quit(self):
		self.web.web.close(); self.root.quit()		

	def update(self, im):
		if not im:
			return False
		self.tkpi = ImageTk.PhotoImage(im)
		self.label.config(image=self.tkpi)	
		return True

	def poll(self):
		if self.update(self.web.img):
			self.root.title( "frame " + str(self.tick) )
			self.tick += 1
			self.label.after( 100,self.poll )
		else:
			self.root.title( "frame -" )
			self.label.after( 1000, self.poll )
		

if __name__ == "__main__":
	#~ t1 = WebListener( port=80, host="iosoft.evo.bg", uri="/cgi-bin/stream.mjpg")
	t1 = WebListener()
	vz = VizThread(t1)
	vz.start()
	t1.run()
