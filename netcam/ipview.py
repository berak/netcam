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
		self.frame = 0
		
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
		nlines = 0
		while self.web:
			s = self.readline();
			c = "Content-Length: "
			if s.find(c)==0:
				nb = int(s[len(c):])
			#~ print "#",s
			if (not s) and (nlines>1) : return nb
			nlines += 1
								
	def run( self ):
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
				nr = 0
				res = BytesIO()
				if nb == 0: # header had no 'Content-Length'
					# slow. read single bytes, check for jpeg-end sequence ff d9
					st = 0
					while(nr < 2000000):
						c = self.web.recv(1);
						res.write(c)
						b = ord(c)
						if (b == 0xff): st = 1
						elif (b == 0xd9) and (st == 1): break
						else: st = 0
						nr += 1
						
				# else  fast. read chunks
				while ( nb>0 ):
					c = self.web.recv(nb);
					if not c : break
					res.write(c)
					nb -= len(c)
					nr += len(c)
				
				try:
					self.img = Image.open(BytesIO(res.getvalue()))	
				except Exception, e:
					print "invalid result ", e, res.getvalue()[:40]
					break
					
				print self.frame, nr
				#~ self.img.save("./my.jpg");
				time.sleep(0.02)
				self.frame += 1
			except:
				break
		self.web.close()



class VizThread( threading.Thread ):
	def __init__(self, web):
		threading.Thread.__init__(self)
		self.web = web

	def run(self):
		self.root = Tkinter.Tk()
		self.root.bind("<Escape>", lambda a:self._quit() )
		self.root.geometry("330x250")
		self.label = Tkinter.Label(self.root)
		self.label.pack()
		self.poll()
		self.root.mainloop()
		
	def _quit(self):
		self.web.web.close()
		self.root.quit()		

	def update(self, im):
		if not im:
			return False
		self.tkpi = ImageTk.PhotoImage(im)
		self.label.config(image=self.tkpi)	
		return True

	def poll(self):
		if self.update(self.web.img):
			self.root.title( "frame " + str(self.web.frame) )
		else:
			self.root.title( "frame -" )
		self.label.after( 100, self.poll )
		

if __name__ == "__main__":
	#~ t1 = WebListener( port=80, host="traf4.murfreesborotn.gov", uri="/axis-cgi/mjpg/video.cgi")
	#~ t1 = WebListener( port=80, host="camera.headend.csb1.ucla.net", uri="/mjpg/video.mjpg") 
	#~ t1 = WebListener( port=8192, host="axis-91ffb4.axiscam.net", uri="/mjpg/video.mjpg?camera=1") # !!!1.9mb!!
	#~ t1 = WebListener( port=80, host="trafficcam13.greensboro-nc.gov", uri="/mjpg/video.mjpg?camera=1")
	#~ t1 = WebListener( port=80, host="tauchen-hamburg.axiscam.net", uri="/mjpg/video.mjpg?camera=1")
	t1 = WebListener( port=80, host="85.235.174.106", uri="/mjpg/video.mjpg")
	#~ t1 = WebListener( port=8081, host="192.82.150.11", uri="/mjpg/video.mjpg?camera=1")
	#~ t1 = WebListener( port=81, host="80.36.62.47", uri="/mjpg/video.mjpg?camera=1")
	#~ t1 = WebListener( port=80, host="iosoft.evo.bg", uri="/cgi-bin/stream.mjpg")
	#~ t1 = WebListener(7777, "localhost")
	#~ t1 = WebListener(7777, "192.168.0.1")
	vz = VizThread(t1)
	vz.start()
	t1.run()
