# GMapData module

# Andrew Berry, CIS 2750 A3 Prototype
# February 14, 2007
# W Gardner, repackaged as module with OO
# "EZ" edition, doesn't use RPC, but outputs HTML/Javascript directly
# February 20, 2007
# W Gardner, allow browser to be closed w/o "no running window found" and
# "no such process" errors.
# February 20, 2010

import os, signal, time
import CGIHTTPServer, BaseHTTPServer

### Global variables

serverPids = [0,0]     # list of server process IDs [web server,browser]


### Public functions

# Start CGI/HTML web server (do once).  Pages to be served must be in
# public_html.
#
# portNum: port number that HTTP server should listen on. Recommend 4xxxx,
#   where "xxxx" are the last 4 digits of your student number. 
#
def startWebServer( portNum=49099 ):
	pid = os.fork()
	serverPids[0] = pid
	if (pid == 0):			# in child process
		# The website is served from public_html, and CGI programs
		# of any type (perl, python, etc) can be placed in cgi-bin
		class Handler(CGIHTTPServer.CGIHTTPRequestHandler):
			cgi_directories = ["/cgi-bin"]

		os.chdir('public_html');
		httpd = BaseHTTPServer.HTTPServer(("", portNum), Handler)
		print "Web server on port:", portNum
		httpd.serve_forever()
		

# Kill all servers
#
def killServers():
	print "Killing all servers...",serverPids
	for pid in serverPids:
		try:
			print " Sending SIGHUP to ",pid
			os.kill(pid, signal.SIGHUP)
		except OSError:
			print "OS Error: PID " + str(pid) + " has already died."
		
		
# Launch Firefox browser (add tab to browser)
#
# url: URL to be opened in Firefox browser, "http://localhost:8080/" (replace
#   8080 with the port number of the web server).
# options: as list (not string); see "man firefox"
#
def launchBrowser( url, options=[] ):
	FIREFOX = "/usr/bin/firefox"
	if (serverPids[1] == 0 ):		# open first browser instance
		pid = os.fork()
		serverPids[1] = pid
		if (pid == 0):			# in child process
			os.execv(FIREFOX, [FIREFOX]+options+[url]);
	else:							# open new tab in existing browser
		pid = os.fork()
		if (pid == 0):			# in child process
			os.execl(FIREFOX, FIREFOX, url);
		else:
			os.wait()


### GMapData class
#
class GMapData:
	# Constructor
	#   header: string to display above map with <H1> tag
	#   center: list of [lat, long] for center of map
	#   zoom: integer with initial zoom setting for map
	def __init__( self, h="50 Stone Road E, Guelph, ON", c=[43.530318,-80.223241], z=15 ):
		self.header = h
		self.center = c
		self.zoom = z
		self.lineColors = ["#ff0000","#00ff00","#0000ff"]
		self.points = []
		self.overlays = []

	# setColors
	#   palette: list of color values for lines, in Google Maps RGB format,
	#       e.g., "#33a570" (string with 3 pairs of hex digits)
	def setColors( self, palette ):
		self.lineColors = palette
		
	# addPoint
	#   point: list of [lat, long] to be added to points array for waypoint,
	#       trackpoint, or connecting line
	def addPoint( self, point ):
		self.points.append( (point[0], point[1]) )	# make into tuple
		
	# addOverlay
	#   start: index in array of points where this overlay starts
	#   np: number of points in line, or 1 for single point
	#   style: for a single point, icon color (0=default Google Maps icon,
	#       1=red, 2=green, 3=blue); for a line, index in color palette
	def addOverlay( self, start, np, style):
		self.overlays.append( (start, np, style) );

	# serve
	#	path: where to generate HTML
	def serve( self, path ):
		htm = open( path, "w" )		# open for writing (truncate existing)

  		# use specified map center coord and zoom level
		print >> htm, """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
 <head>
  <meta http-equiv="content-type" content="text/html; charset=utf-8"/>
  <title>xpgs</title>
   <link rel="stylesheet" href="style.css" type="text/css" />
  <script src="http://maps.google.com/maps?file=api&amp;v=2&amp;key=ABQIAAAASveZAKDchy8aE-6K6klrVRTwM0brOpm-All5BF6PoaKBxRWWERS4kZrUi1p4QOZXGVxTLHvCXsg94A"
          type="text/javascript"></script>
  <script type="text/javascript">
  //<![CDATA[
  function load() {
    if (GBrowserIsCompatible()) {
      var baseIcon = new GIcon();
      baseIcon.shadow = "http://labs.google.com/ridefinder/images/mm_20_shadow.png"
      baseIcon.iconSize = new GSize(12, 20);
      baseIcon.shadowSize = new GSize(22, 20);
      baseIcon.iconAnchor = new GPoint(6, 20);
      baseIcon.infoWindowAnchor = new GPoint(5, 1);
      var icons = [new GIcon(baseIcon),new GIcon(baseIcon),new GIcon(baseIcon)];
      icons[0].image = "http://labs.google.com/ridefinder/images/mm_20_red.png";
      icons[1].image = "http://labs.google.com/ridefinder/images/mm_20_blue.png";
      icons[2].image = "http://labs.google.com/ridefinder/images/mm_20_green.png";
      var map = new GMap2(document.getElementById("map"));
      map.addControl(new GSmallMapControl());
      map.addControl(new GMapTypeControl());
      map.setCenter(new GLatLng(%f, %f), %d);""" % (self.center[0], self.center[1], self.zoom)

		# add overlays to map
		for draw in self.overlays:
			if ( draw[1]==1 ):		# single point
				print >> htm, '      var p = new GLatLng(%f, %f);' % self.points[draw[0]]
				if ( draw[2]==0 ):	# style = default Google Maps marker
					print >> htm, '      map.addOverlay(new GMarker(p));'
				else:				# style = colored icon
					print >> htm, '      map.addOverlay(new GMarker(p,icons[%d]));' % draw[2]

			else:					# polyline
				print >> htm, '      var points = [];'
				for pt in self.points[draw[0]:draw[0]+draw[1]]:
					print >> htm, '      points.push(new GLatLng(%f, %f));' % pt
				print >> htm, '      map.addOverlay(new GPolyline(points,"%s"));' % self.lineColors[draw[2]]
			
		# display header above map
		print >> htm, """    }
  }
  //]]>
  </script>
 </head>
 <body onload="load()" onunload="GUnload()">
  <div id="content">
	<h1>%s</h1>
	<div id="map"></div>
  </div>
 </body>
</html>""" % self.header

		htm.close()
