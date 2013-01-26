######################
#xgps.py.c -- Gui for my Gps Program
#Creation date: March 10, 2010
#Last updated:  April 8, 2010

#Description:
#This provides a GUI interface to assignment #2, and also utilizes
#Google maps to provide awesomness,
#also utilizes a database to store and retrieve hike information
#and display these hikes through google earth

#Problems:
	#- wont warn the removing waypts will also remove routes
	#- modal windows arent modal (allow access to original window)
	#- hikes all have the same colour
	#- cant query with % wildcard symbol

#David Farr
#0629099
#farrd@uoguelph.ca
#######################

#!/usr/bin/python

import os
from math import *
from Tkinter import *
import tkFileDialog
import tkColorChooser
from subprocess import Popen, PIPE
import MySQLdb

# import the shared library and the maps
import Gps
from GMapData import *

# some global variables
filename = ''
delete = 0
rIndex = 0
hikeno = 0
ptno = 0
fileno = 0
waypts=[]
routes=[]
trkpts=[]
tracks=[]
wColour = 0
rColour = "#00ff00"
tColour = 1
trColour = "#efef00"

# start the web server
startWebServer(49099)

# connect to database (default host)
if len(sys.argv) == 3:
	try:
		db = MySQLdb.connect (host = "db.cis.uoguelph.ca",
		                        user = sys.argv[1],
					passwd = sys.argv[2],
					db = sys.argv[1])
	except MySQLdb.Error, e:
		print "Error %d: %s" % (e.args[0], e.args[1])
		sys.exit(1)

# connect to database (provided host)
elif len(sys.argv) == 4:
	try:
		db = MySQLdb.connect (host = sys.argv[3],
		                        user = sys.argv[1],
					passwd = sys.argv[2],
					db = sys.argv[1])
	except MySQLdb.Error, e:
		print "Error %d: %s" % (e.args[0], e.args[1])
		sys.exit(1)
	
# provide proper command line prototype,
# and then exit if not entered properly
else:
	print "Invalid use of xgps"
	print "The correct command line argument is:"
	print "   python xgps.py username password [hostname]"
	sys.exit(1)
	
# make the tables if they dont exist
cursor = db.cursor()
	   
hike = """ CREATE TABLE IF NOT EXISTS HIKE (
             hikeno INT AUTO_INCREMENT,
             name VARCHAR(30),
             location VARCHAR(30),
             comment VARCHAR(80),
             rating SMALLINT,
             note TINYTEXT,
	     PRIMARY KEY(hikeno) ) """
	     
waypts = """ CREATE TABLE IF NOT EXISTS WAYPTS (
           fileno INT,
           ptno INT,
           id VARCHAR(30),
           comment VARCHAR(80),
           lat DOUBLE,
           lon DOUBLE,
	   PRIMARY KEY(fileno, ptno) ) """
	   
hikepts = """ CREATE TABLE IF NOT EXISTS HIKEPTS (
              hikeno INT,
              fileno INT,
              ptno INT,
              leg INT,
              distance FLOAT,
	      PRIMARY KEY(hikeno, leg),
	      FOREIGN KEY(hikeno) REFERENCES HIKE(hikeno),
	      FOREIGN KEY(fileno) REFERENCES WAYPTS(fileno),
	      FOREIGN KEY(fileno) REFERENCES WAYPTS(ptno) ) """

cursor.execute(hike)
cursor.execute(waypts)
cursor.execute(hikepts)

#change some settings so TRANACTION, COMMIT, ROLLBACK work
cursor.execute("SET autocommit = 0")
cursor.execute("ALTER TABLE HIKE ENGINE = InnoDB")
cursor.execute("ALTER TABLE WAYPTS ENGINE = InnoDB")
cursor.execute("ALTER TABLE HIKEPTS ENGINE = InnoDB")

# Open up a new Gps File
def openGps(event):
	
	global filename, hikeno, fileno, ptno
	filename = tkFileDialog.askopenfilename(filetypes = [('Gps', '*.gps')])
	
	if filename != () and filename != '':
		
		# read in the gps file
		status = Gps.readFile(filename)
		
		if status != 'OK':
			filename = ''
			
			if len(status) == 2:
				msgWindow("Error opening file", status[0] + ": " + str(status[1]))
			else:
				msgWindow("Error opening file", status)
				
			filemenu.entryconfig(2, state="disabled")
			filemenu.entryconfig(3, state="disabled")
			limitOptions()
		
		else:
			global delete
			delete = 1
			
			# update the title
			root.title("David's GPS Program: " + filename)
			
			# get the output from the gps file
			fileInfo = os.popen("./gpstool -info < " + filename + " 2>> log.txt")
			
			# read in the info for the components
			global waypts, routes, trkpts, tracks
			waypts, routes, trkpts, tracks = Gps.getData(waypts, routes, trkpts, tracks)
			free = Gps.freeFile()
			
			# change the options available, and update some stuff
			filemenu.entryconfig(2, state="normal")
			filemenu.entryconfig(3, state="normal")
			hikemenu.entryconfig(0, state="normal")
			hikemenu.entryconfig(1, state="normal")
			limitOptions()
			writeInfo(fileInfo)
			updateLog()
			
			# get the biggest hikeno
			getHikeno = "SELECT MAX(hikeno) FROM HIKE"
			cursor.execute(getHikeno)
			temp = cursor.fetchone()
			if temp[0] == None:
				hikeno = 0
			else:
				hikeno = temp[0]
			
			# get the biggest fileno
			getFileno = "SELECT MAX(fileno) FROM WAYPTS"
			cursor.execute(getFileno)
			temp = cursor.fetchone()
			if temp[0] == None:
				fileno = 1
			else:
				fileno = temp[0] + 1

			# set the ptno to 0
			ptno = 0

# write the remaing info out to the temporary file
def writeInfo(fileInfo):
	
	# clear the panel
	info.config(state=NORMAL)
	info.delete(1.0, END)
	info.config(state=DISABLED)
	
	# write the gps info to the panel
	info.config(state=NORMAL)
	for index in range(0, 5):
		info.insert(INSERT, fileInfo.readline())
	info.config(state=DISABLED)
	
	# write the rest of the file out to a temp file
	temp = open("temp_gps_file.gps", "w")
	for index in fileInfo.readlines():
		temp.write(index)
	temp.close()

# update the log file on the screen
def updateLog():

	# write any error out to the log area
	log.config(state=NORMAL)
	logFile = open("log.txt", "r")
	log.insert(INSERT, logFile.read())
	log.config(state=DISABLED)
	logFile.close()
	
	# read in the stuff for routes and tracks
	routeList.delete(0, END)
	trackList.delete(0, END)
	for item in routes:
		routeList.insert(END, str(item[0]) + " - " + item[1])
		
	for item in tracks:
		trackList.insert(END, str(item[0]) + " - " + item[1])

# save the file
def save(event):

	if filename != () and filename != '':
		# open the necessary files
		fTo = open(filename, "w")
		fFrom = open("temp_gps_file.gps", "r")
		
		fTo.write(fFrom.read())
		
		# close the files
		fTo.close()
		fFrom.close()

# save the file under a different name
def saveAs(event):
	
	global filename

	if filename != () and filename != '':
		fTo = tkFileDialog.asksaveasfile(filetypes = [('Gps', '*.gps')])
		filename = fTo.name
		root.title("David's GPS Program: " + filename)
		
		# open the necessary files
		fFrom = open("temp_gps_file.gps", "r")
		fTo.write(fFrom.read())
		
		# close the files
		fTo.close()
		fFrom.close()

# the discard and keep option,
# this function will either keep components or remove them
# NOTE will not warn about waypoints implicating the removal of routes
def remove(option):
	msg = Toplevel(padx=80, pady=5)
	
	if(option == "keep"):
		msg.title("Choose what should be kept")
	else:
		msg.title("Choose what should be discarded")
	
	def kill(event):
		msg.destroy()
	msg.bind("<Escape>", kill)
	
	w = StringVar()
	r = StringVar()
	t = StringVar()
	def ok():
		msg.destroy()
		
		global waypts, routes, trkpts, tracks
		
		# get the arguments
		_w = w.get()
		_r = r.get()
		_t = t.get()
		args = _w + _r + _t
		
		cont = 1
		if (option == "discard" and _w == "w" and _t == "t") or (option == "keep" and _w == "" and _t == "") or (option == "discard" and _w == "w" and not trkpts) or (option == "discard" and _t == "t" and not waypts):
			modal("Error", "Cannot discard all components")
			cont = 0
		
		cont2 = 1
		#if (cont == 1) and ((option == "discard" and _w == "w" and _r == "" and routes) or (option == "keep" and _w == "" and _r == "r")):
			#cont2 = modal("Warning", "Discarding waypoints will also remove the routes, do you wish to continue?")

		# make sure this can be done
		if cont == 1 and cont2 == 1:
			
			# discard/keep
			fileInfo = os.popen("./gpstool -" + option + " " + args + " < temp_gps_file.gps 2>> log.txt | ./gpstool -info 2>> log.txt")
			writeInfo(fileInfo)
				
			# read in the adjusted file
			status = Gps.readFile("temp_gps_file.gps")
			if status != 'OK':
				if len(status) == 2:
					msgWindow("Error opening file", status[0] + ": " + str(status[1]))
				else:
					msgWindow("Error opening file", status)
			
			else:
				# read in the info for the components
				waypts, routes, trkpts, tracks = Gps.getData(waypts, routes, trkpts, tracks)
				free = Gps.freeFile()
			
				updateLog()
				limitOptions()
		
	Label(msg, text="Waypoints").grid(row=0, column=0, padx=10, pady=5)
	Checkbutton(msg, variable=w, onvalue="w", offvalue="").grid(row=0, column=1)
	
	Label(msg, text="Routes").grid(row=1, column=0, padx=10, pady=5)
	Checkbutton(msg, variable=r, onvalue="r", offvalue="").grid(row=1, column=1)
	
	Label(msg, text="Trackpoints").grid(row=2, column=0, padx=10, pady=5)
	Checkbutton(msg, variable=t, onvalue="t", offvalue="").grid(row=2, column=1)
	
	Button(msg, text="Cancel", command=msg.destroy).grid(row=3, column=0, padx=5)
	Button(msg, text="OK", command=ok).grid(row=3, column=1)
	
# the merge option
# this will merge new file to the current one
def merge():
	
	file = tkFileDialog.askopenfilename(filetypes = [('Gps', '*.gps')], title = "Choose a file to merge")
	
	# merge the files
	fileInfo = os.popen("./gpstool -merge " + file + " < temp_gps_file.gps 2>> log.txt | ./gpstool -info 2>> log.txt")
	writeInfo(fileInfo)
		
	# read in the sorted file
	status = Gps.readFile("temp_gps_file.gps")
	if status != 'OK':
		if len(status) == 2:
			msgWindow("Error opening file", status[0] + ": " + str(status[1]))
		else:
			msgWindow("Error opening file", status)
	
	else:
		# read in the info for the components
		global waypts, routes, trkpts, tracks
		waypts, routes, trkpts, tracks = Gps.getData(waypts, routes, trkpts, tracks)
		free = Gps.freeFile()

		updateLog()
		limitOptions()
	
# the sort option
# sorts the waypoints alphabetically
def sort():
	
	# sort the file
	fileInfo = os.popen("./gpstool -sortwp < temp_gps_file.gps 2>> log.txt | ./gpstool -info 2>> log.txt")
	writeInfo(fileInfo)
		
	# read in the sorted file
	status = Gps.readFile("temp_gps_file.gps")
	if status != 'OK':
		if len(status) == 2:
			msgWindow("Error opening file", status[0] + ": " + str(status[1]))
		else:
			msgWindow("Error opening file", status)
	
	else:
		# read in the info for the components
		global waypts, routes, trkpts, tracks
		waypts, routes, trkpts, tracks = Gps.getData(waypts, routes, trkpts, tracks)
		free = Gps.freeFile()

		updateLog()
		limitOptions()
	
# limit the menu options
def limitOptions():
	
	# change the options if some are empty
	if not waypts:
		w.set(1)
		editmenu.entryconfig(2, state="disabled")
	else:
		w.set(0)
		filemenu.entryconfig(5, state="normal")
		editmenu.entryconfig(0, state="normal")
		editmenu.entryconfig(1, state="normal")
		editmenu.entryconfig(2, state="normal")
		
	if not routes:
		r.set(1)
		routeList.delete(0, END)
	else:
		w.set(0)
		filemenu.entryconfig(5, state="normal")
		editmenu.entryconfig(0, state="normal")
		editmenu.entryconfig(1, state="normal")
		
	if not trkpts:
		t.set(1)
	else:
		w.set(0)
		filemenu.entryconfig(5, state="normal")
		editmenu.entryconfig(0, state="normal")
		editmenu.entryconfig(1, state="normal")
		
	if not tracks:
		tr.set(1)
		trackList.delete(0, END)
	else:
		w.set(0)
		
	# check for all
	if not waypts and not routes and not trkpts:
		filemenu.entryconfig(2, state="disabled")
		filemenu.entryconfig(3, state="disabled")
		filemenu.entryconfig(5, state="disabled")
		editmenu.entryconfig(0, state="disabled")
		editmenu.entryconfig(1, state="disabled")
	
# Update the display of waypts, trkpts, routes, and tracks
def updateDisplay():
	
	if filename != () and filename != '':
		# calculate the average lat/lon
		global waypts, trkpts, routes, tracks
		lat = 0
		lon = 0
		avgLat = 0
		avgLon = 0
		
		for item in waypts:
			lat = lat + item[0]
			lon = lon + item[1]
			
		for item in trkpts:
			lat = lat + item[0]
			lon = lon + item[1]
			
		avgLat = lat / (len(waypts) + len(trkpts))
		avgLon = lon / (len(waypts) + len(trkpts))
		
		# make the map, centre it at the average lat/lon
		gMap = GMapData(filename, [avgLat, avgLon], 10)
		count = 0
		
		# set the colours 0->routes, 1->tracks
		colours = (rColour, trColour, "#FFFFFF")
		gMap.setColors(colours)
		
		# add the waypoints
		if(w.get() == 0):
			for item in waypts:
				gMap.addPoint(item)
				gMap.addOverlay(count, 1, wColour)
				count = count + 1
		
		# add the trackpoints
		if(t.get() == 0):
			for item in trkpts:
				gMap.addPoint(item)
				gMap.addOverlay(count, 1, tColour)
				count = count + 1
			
		# add the routes
		rSelect = routeList.curselection()
		rSelect = map(int, rSelect)
		
		if(r.get() == 0):
			for index in rSelect:
				nLegs = 0
				start = count
				
				for leg in routes[index][2]:
					gMap.addPoint(waypts[leg])
					count = count + 1
					nLegs = nLegs + 1
					
				gMap.addOverlay(start, nLegs, 0)
			
		# add the tracks
		trSelect = trackList.curselection()
		trSelect = map(int, trSelect)
		
		if(tr.get() == 0):
			for index in trSelect:
				
				# get the upper value
				if(index < len(tracks) - 1):
					upper = tracks[index + 1][0]
				else:
					upper = len(trkpts) - 1
				
				nTracks = 0
				start = count
				for trk in range(tracks[index][0], upper - 1):
					gMap.addPoint(trkpts[trk])
					count = count + 1
					nTracks = nTracks + 1
					
				gMap.addOverlay(start, nTracks, 1)
			
		gMap.serve("public_html/index.html")
		launchBrowser("http://localhost:49099/")
	
# clears the log area
def clear():
	
	# clear the gui area
	log.config(state=NORMAL)
	log.delete(1.0, END)
	log.config(state=DISABLED)
	
	# clear the log file
	logClear = open("log.txt", "w")
	logClear.write("")
	logClear.close()
	
# exits the GUI, removing temporary files and killing the servers beforehand
def exitGui():
	# delete the temporary files and kill the gui
	if delete == 1:
		os.system("rm temp_gps_file.gps")
		os.system("rm log.txt")
		
	db.close()
	killServers()
	root.destroy()
	
# choose colours for routes and tracks
def chooseColour(option):
	global rColour, trColour
	colour = tkColorChooser.askcolor()
	
	if option == 'r':
		rBox.create_rectangle(0, 0, 20, 20, fill=colour[1])
		rColour = colour[1]
	else:
		trBox.create_rectangle(0, 0, 20, 20, fill=colour[1])
		trColour = colour[1]

# choose colours for waypnts and trkpts
def getColour(option):
		
	msg = Toplevel(padx=40, pady=5)
	msg.title("Choose Colour")
	
	def kill(event):
		msg.destroy()
	msg.bind("<Escape>", kill)
	
	colour = StringVar()
	def ok():
		global wColour, tColour
		
		msg.destroy()
		if option == 'w':
			temp = colour.get()
			if temp == "red":
				wColour = 0
			elif temp == "blue":
				wColour = 1;
			else:
				wColour = 2
			
			wBox.create_rectangle(0, 0, 20, 20, fill=colour.get())
			wBox.create_rectangle(0, 0, 20, 20, fill=colour.get())
			wBox.create_rectangle(0, 0, 20, 20, fill=colour.get())
		else:
			temp = colour.get()
			if temp == "red":
				tColour = 0
			elif temp == "blue":
				tColour = 1;
			else:
				tColour = 2
				
			tBox.create_rectangle(0, 0, 20, 20, fill=colour.get())
			tBox.create_rectangle(0, 0, 20, 20, fill=colour.get())
			tBox.create_rectangle(0, 0, 20, 20, fill=colour.get())
		
	Label(msg, text="Red").grid(row=0, column=0, padx=10, pady=5)
	Radiobutton(msg, variable=colour, value="red").grid(row=0, column=1)
	
	Label(msg, text="Green").grid(row=1, column=0, padx=10, pady=5)
	Radiobutton(msg, variable=colour, value="green").grid(row=1, column=1)
	
	Label(msg, text="Blue").grid(row=2, column=0, padx=10, pady=5)
	Radiobutton(msg, variable=colour, value="blue").grid(row=2, column=1)
	
	Button(msg, text="Cancel", command=msg.destroy).grid(row=3, column=0, padx=5)
	Button(msg, text="OK", command=ok).grid(row=3, column=1)
	
# This function allows the user to store or omit specific
# routes as hikes in an sql database
def store():
	global db, cursor, routes, waypts
	
	# start the window with proper route info
	def initialise():
		global rIndex

		cursor.execute("START TRANSACTION")
		name.insert(INSERT, routes[rIndex][1])
		comment.set(routes[rIndex][1])
		rIndex = rIndex + 1
		
	# saves a route as a hike
	def save():
		global rIndex, fileno, hikeno, ptno
		
		# add the info to the database
		location = countries.get(ACTIVE)
		if countries.select_includes(39) != 0:
			temp = regions.curselection()
			if len(temp) > 0:
				location = location + ", " + regions.get(ACTIVE)
		
		elif countries.select_includes(236) != 0:
			temp = regions.curselection()
			if len(temp) > 0:
				location = location + ", " + regions.get(ACTIVE)
			
		noteText = note.get(1.0, END)
		noteText = os.linesep.join([s for s in noteText.splitlines() if s])
		
		# the HIKE table
		route = "INSERT INTO HIKE(name, location, comment, rating, note)\
				VALUES('%s', '%s', '%s', '%d', '%s')" % (name.get(), location, routes[rIndex - 1][1], rating.get(), noteText)
		cursor.execute(route)
		
		hikeno = hikeno + 1
	
		# the WAYPTS table
		leg = 0
		oldLat = 0
		oldLon = 0
		for item in routes[rIndex - 1][2]:
			
			# insert stuff into the waypts
			waypt = "INSERT INTO WAYPTS(fileno, ptno, id, comment, lat, lon)\
				 VALUES('%d', '%d', '%s', '%s', '%f', '%f')" % (fileno, ptno, waypts[item][2], waypts[item][3], waypts[item][0], waypts[item][1])
			cursor.execute(waypt)
			
			# calculate the distance
			if oldLat == 0 and oldLon == 0:
				distance = 0
			else:
				distance = calcDistance(oldLat, waypts[item][0], oldLon, waypts[item][1])
			
			#insert stuff into the hikepts
			hikept = "INSERT INTO HIKEPTS(hikeno, fileno, ptno, leg, distance)\
				  VALUES('%d', '%d', '%d', '%d', '%f')" % (hikeno, fileno, ptno, leg, distance)
			cursor.execute(hikept)
			
			# update stuff
			oldLat = waypts[item][0]
			oldLon = waypts[item][1]
			leg = leg + 1
			ptno = ptno + 1
			
		# if this is the last route
		if rIndex == len(routes):
			cursor.execute("COMMIT")
			rIndex = 0
			store.destroy()
		else:
			name.delete(0, END)
			name.insert(INSERT, routes[rIndex][1])
			comment.set(routes[rIndex][1])
			rating.set(0)
			note.delete(1.0, END)
			rIndex = rIndex + 1
		
	# moves on to the hike, does not store the current one
	def omit():
		global rIndex
		
		# if this is the last route
		if rIndex == len(routes):
			cursor.execute("COMMIT")
			rIndex = 0
			store.destroy()
		else:
			name.delete(0, END)
			name.insert(INSERT, routes[rIndex][1])
			comment.set(routes[rIndex][1])
			rating.set(0)
			note.delete(1.0, END)
			rIndex = rIndex + 1
	
	# exits this window and makes sure nothing was put up to the server
	def cancel():
		global rIndex
		
		cursor.execute("ROLLBACK")
		getHikeno = "SELECT MAX(hikeno) FROM HIKE"
		cursor.execute(getHikeno)
		temp = cursor.fetchone()
		if temp[0] == None:
			hikeno = 1
		else:
			hikeno = temp[0]

		rIndex = 0
		store.destroy()
		
	# if Canada or US is selected this fills the provinces/states
	def fillRegion(event):
		regions.delete(0, END)
		
		if countries.select_includes(39) != 0:
			pList = open("provinces.txt", "r")
			for item in pList.readlines():
				regions.insert(END, item.strip())
			pList.close()
			
		elif countries.select_includes(236) != 0:
			pList = open("states.txt", "r")
			for item in pList.readlines():
				regions.insert(END, item.strip())
			pList.close()
			
	# calculates the distance between two coordinates
	def calcDistance(lat1, lat2, lon1, lon2):
		
		# calculate the delta
		lat = lat2 - lat1
		lon = lon2 - lon1
		
		a = (sin(radians(lat/2.0)) ** 2.0) + cos(radians(lat1)) * cos(radians(lat2)) * (sin(radians(lon/2.0)) ** 2.0)
		c = 2.0 * atan2(radians(sqrt(a)), radians(sqrt(1.0 - a)))
		
		distance = 6378.1 * c
		return distance
		
	# the store GUI
	store = Toplevel(padx=10, pady=10)
	store.title("Hike Information")
	store.protocol("WM_DELETE_WINDOW", cancel)
		
	infoFrame = Frame(store, pady=5)
	infoFrame.grid(row=0, column=0)
	
	listFrame = Frame(store, pady=5)
	listFrame.grid(row=1, column=0)
	
	ratingFrame = Frame(store, pady=5)
	ratingFrame.grid(row=2, column=0)
	
	noteFrame = Frame(store, pady=5)
	noteFrame.grid(row=3, column=0)
	
	buttonFrame = Frame(store, pady=5)
	buttonFrame.grid(row=4, column=0)
	
	# the info frame
	Label(infoFrame, text="Name:", padx=10, pady=5).grid(row=0, column=0)
	name = Entry(infoFrame, width=35)
	name.grid(row=0, column=1)
	
	comment = StringVar()
	Label(infoFrame, text="Comment:", padx=10).grid(row=1, column=0)
	Label(infoFrame, textvariable=comment, width=35).grid(row=1, column=1)
	
	# the list frame
	countryFrame = Frame(listFrame)
	countryFrame.grid(row=0, column=0)
	
	regionFrame = Frame(listFrame)
	regionFrame.grid(row=0, column=1)
	
	Label(countryFrame, text="Country:", width=30, anchor=W, justify=LEFT).pack()
	scroll1 = Scrollbar(countryFrame, orient=VERTICAL)
	countries = Listbox(countryFrame, exportselection=0, yscrollcommand=scroll1.set, font=("Helvetica", 10), width=30, height=10)
	scroll1.config(command=countries.yview)
	scroll1.pack(side=RIGHT, fill=Y)
	countries.bind("<ButtonRelease-1>", fillRegion)
	countries.pack(side=LEFT, fill=BOTH, expand=1)
	
	Label(regionFrame, text="Province/State:", width=30, anchor=W, justify=LEFT).pack()
	scroll2 = Scrollbar(regionFrame, orient=VERTICAL)
	regions = Listbox(regionFrame, exportselection=0, yscrollcommand=scroll2.set, font=("Helvetica", 10), width=30, height=10)
	scroll2.config(command=regions.yview)
	scroll2.pack(side=RIGHT, fill=Y)
	regions.pack(side=LEFT, fill=BOTH, expand=1)
	
	# the rating frame
	Label(ratingFrame, text="Rating:", padx=10).grid(row=0, column=0)
	
	rating = IntVar()
	r0 = Radiobutton(ratingFrame, text="0", variable=rating, value=0, padx=5)
	r1 = Radiobutton(ratingFrame, text="1", variable=rating, value=1, padx=5)
	r2 = Radiobutton(ratingFrame, text="2", variable=rating, value=2, padx=5)
	r3 = Radiobutton(ratingFrame, text="3", variable=rating, value=3, padx=5)
	r4 = Radiobutton(ratingFrame, text="4", variable=rating, value=4, padx=5)
	r5 = Radiobutton(ratingFrame, text="5", variable=rating, value=5, padx=5)
	
	r0.grid(row=0, column=1)
	r1.grid(row=0, column=2)
	r2.grid(row=0, column=3)
	r3.grid(row=0, column=4)
	r4.grid(row=0, column=5)
	r5.grid(row=0, column=6)
	
	# the note frame
	Label(noteFrame, text="Note:", width=45, anchor=W, justify=LEFT).grid(row=0, column=0)
	note = Text(noteFrame, wrap=WORD, width=50, height=5)
	note.grid(row=1, column=0)
	
	# the button frame
	Button(buttonFrame, text="Store Route", padx=15, command=save).grid(row=0, column=0)
	Button(buttonFrame, text="Omit Route", padx=15, command=omit).grid(row=0, column=1)
	Button(buttonFrame, text="Cancel", padx=15, command=cancel).grid(row=0, column=2)
	
	# fill the countries and ititialise
	cList = open("countrylist.txt", "r")
	for item in cList.readlines():
		countries.insert(END, item.strip())
	cList.close()
		
	initialise()
	store.mainloop()
	
# This function allows the user to perform queries
# in an sql database as well as view hikes in google maps
def query():
	
	# see how many hikes are in the database
	cursor.execute("SELECT COUNT(*) from HIKE")
	fetch = cursor.fetchone()
	numHikes = fetch[0]
	
	# don't open window if there are no hikes
	if numHikes > 0:
		
		hikeno = []
	
		# start the window with the info in the database
		def intitialise():
			cursor.execute("SELECT name from HIKE ORDER BY name")
			while (1):
				entry = cursor.fetchone()
				if entry == None:
					break
				
				hikes.insert(END, entry[0])
				
			cursor.execute("SELECT hikeno from HIKE ORDER BY name")
			while(1):
				entry = cursor.fetchone()
				if entry == None:
					break
				
				hikeno.append(entry[0])
				
		# fills the info when ONE hike is selected
		def fillInfo(event):
			
			#get currently selected hike
			select = hikes.curselection();
	
			if len(select) == 1:
				index = int(select[0])
				
				getInfo = "SELECT location from HIKE WHERE hikeno = '%d'" % (hikeno[index])
				cursor.execute(getInfo)
				fetch = cursor.fetchone()
				location.set(fetch[0])
				
				getInfo = "SELECT comment from HIKE WHERE hikeno = '%d'" % (hikeno[index])
				cursor.execute(getInfo)
				fetch = cursor.fetchone()
				comment.set(fetch[0])
				
				getInfo = "SELECT rating from HIKE WHERE hikeno = '%d'" % (hikeno[index])
				cursor.execute(getInfo)
				fetch = cursor.fetchone()
				rating.set(fetch[0])
				
				getInfo = "SELECT COUNT(hikeno) from HIKEPTS WHERE hikeno = '%d'" % (hikeno[index])
				cursor.execute(getInfo)
				fetch = cursor.fetchone()
				legs.set(fetch[0])
				
				getInfo = "SELECT note from HIKE WHERE hikeno = '%d'" % (hikeno[index])
				cursor.execute(getInfo)
				fetch = cursor.fetchone()
				noteText.config(state=NORMAL)
				noteText.delete(1.0, END)
				noteText.insert(INSERT, fetch[0])
				noteText.config(state=DISABLED)
			
			# clear everything if multiple selected
			else:
				location.set("")
				comment.set("")
				rating.set("")
				legs.set("")
				noteText.config(state=NORMAL)
				noteText.delete(1.0, END)
				noteText.config(state=DISABLED)
				
		# displays the hike in google maps
		def mapIt():
			select = hikes.curselection();
			
			allHikes = []
			for item in select:
			
				# get the ones selected in the list
				index = int(item[0])
				
				# get the fileno and ptnos to reference the WAYPTS table
				getInfo = "SELECT DISTINCT fileno from HIKEPTS WHERE hikeno = '%d'" % (hikeno[index])
				cursor.execute(getInfo)
				fetch = cursor.fetchone()
				tempFileno = fetch[0]
				
				getInfo = "SELECT ptno from HIKEPTS WHERE hikeno = '%d' AND fileno = '%d'" % (hikeno[index], tempFileno)
				cursor.execute(getInfo)
				fetch = cursor.fetchall()
				
				# get the lat/lon with information from the WAYPTS table
				mapData = []
				for item in fetch:
					
					tempPtno = int(item[0])
					coordinate = []
					
					# get the latitude
					getLat = "SELECT lat from WAYPTS where fileno = '%d' AND ptno = '%d'" % (tempFileno, tempPtno)
					cursor.execute(getLat)
					temp = cursor.fetchone()
					coordinate.append(temp[0])
					
					# get the longitude
					getLon = "SELECT lon from WAYPTS where fileno = '%d' AND ptno = '%d'" % (tempFileno, tempPtno)
					cursor.execute(getLon)
					temp = cursor.fetchone()
					coordinate.append(temp[0])
					
					mapData.append(coordinate)
					
				# add the hike to the list
				allHikes.append(mapData)
					
			# generate the map
			if len(select) > 0:
				
				# calculate average lat/lon
				lat = 0
				lon = 0
				noCoords = 0
				for thing in allHikes:
					for item in thing:
						lat = lat + item[0]
						lon = lon + item[1]
						noCoords = noCoords + 1
						
				avgLat = lat / noCoords
				avgLon = lon / noCoords
					
				# make the map, centre it at the average lat/lon
				gMap = GMapData("hello world", [avgLat, avgLon], 10)
					
				count = 0
				for thing in allHikes:
					start = count
					points = 0
					for item in thing:
						gMap.addPoint(item)
						gMap.addOverlay(count, 1, 0)
						count = count + 1
						points = points + 1
						
					gMap.addOverlay(start, points, 1)
					
				gMap.serve("public_html/index.html")
				launchBrowser("http://localhost:49099/")
			
		# determines the shortest hike in a specific location
		def handleQ1():
			
			# find the corresponding hike
			place = q1.get()
			getInfo = "SELECT hikeno from HIKE WHERE location = '%s'" % (place)
			cursor.execute(getInfo)
			fetch = cursor.fetchall()
			
			hike = 0
			minDist = 999999
			for item in fetch:
				getInfo = "SELECT distance from HIKEPTS WHERE hikeno = '%d'" % (int(item[0]))
				cursor.execute(getInfo)
				legs = cursor.fetchall()
				
				curDist = 0
				for leg in legs:
					curDist = curDist + leg[0]
					
				# see if this one is the smallest
				if curDist < minDist:
					minDist = curDist
					hike = int(item[0])
						
			if len(fetch) > 0:
				getInfo = "SELECT name from HIKE WHERE hikeno = '%d'" % (hike)
				cursor.execute(getInfo)
				fetch = cursor.fetchone()
				name = fetch[0]
				
				result.config(state=NORMAL)
				result.insert(INSERT, "The shortest hike in " + place + " is " + name + " which is " + str(minDist) + "km\n")
				result.insert(INSERT, "----------------------------------------------------------\n")
				result.yview(END)
				result.config(state=DISABLED)
			else:
				result.config(state=NORMAL)
				result.insert(INSERT, "There are no hikes in " + place + "\n")
				result.insert(INSERT, "----------------------------------------------------------\n")
				result.yview(END)
				result.config(state=DISABLED)
			
		# determines the longest hike in a specific location
		def handleQ2():
			
			# find the corresponding hike
			place = q2.get()
			getInfo = "SELECT hikeno from HIKE WHERE location = '%s'" % (place)
			cursor.execute(getInfo)
			fetch = cursor.fetchall()
			
			hike = 0
			maxDist = 0
			for item in fetch:
				getInfo = "SELECT distance from HIKEPTS WHERE hikeno = '%d'" % (int(item[0]))
				cursor.execute(getInfo)
				legs = cursor.fetchall()
				
				curDist = 0
				for leg in legs:
					curDist = curDist + leg[0]
					
				# see if this one is the smallest
				if curDist > maxDist:
					maxDist = curDist
					hike = int(item[0])
						
			if len(fetch) > 0:
				getInfo = "SELECT name from HIKE WHERE hikeno = '%d'" % (hike)
				cursor.execute(getInfo)
				fetch = cursor.fetchone()
				name = fetch[0]
				
				result.config(state=NORMAL)
				result.insert(INSERT, "The longest hike in " + place + " is " + name + " which is " + str(maxDist) + "km\n")
				result.insert(INSERT, "----------------------------------------------------------\n")
				result.yview(END)
				result.config(state=DISABLED)
			else:
				result.config(state=NORMAL)
				result.insert(INSERT, "There are no hikes in " + place + "\n")
				result.insert(INSERT, "----------------------------------------------------------\n")
				result.yview(END)
				result.config(state=DISABLED)
			
		# determines how many hikes in a specific location
		def handleQ3():
			
			# find the corresponding hikes
			place = q3.get()
			getInfo = "SELECT COUNT(location) from HIKE WHERE location = '%s'" % (place)
			cursor.execute(getInfo)
			fetch = cursor.fetchone()
			
			# print them out
			result.config(state=NORMAL)
			if int(fetch[0]) != 1:
				result.insert(INSERT, "There are " + str(fetch[0]) + " hikes in " + place + "\n")
			else:
				result.insert(INSERT, "There is " + str(fetch[0]) + " hike in " + place + "\n")
				
			result.insert(INSERT, "----------------------------------\n")
			result.yview(END)
			result.config(state=DISABLED)
			
		# determines how many hikes have more then ___ legs
		def handleQ4():
			
			# find the corresponding hikes
			num = int(q4.get())
			getInfo = "SELECT DISTINCT hikeno from HIKEPTS WHERE leg >= '%d'" % (num)
			cursor.execute(getInfo)
			
			spots = []
			while(1):
				entry = cursor.fetchone()
				if entry == None:
					break
				
				spots.append(entry[0])
				
			# print them out
			result.config(state=NORMAL)
			if len(spots) == 0:
				result.insert(INSERT, "No hikes have more than " + str(num) + " legs.\n")
			else:
				result.insert(INSERT, str(len(spots)) + " hikes have more than " + str(num) + " legs, they are:\n")
				
			for item in spots:
				getName = "SELECT name from HIKE WHERE hikeno = '%d'" % (item)
				cursor.execute(getName)
				entry = cursor.fetchone()
				result.insert(INSERT, str(entry[0]) + "\n")
				
			result.insert(INSERT, "----------------------------------\n")
			result.yview(END)
			result.config(state=DISABLED)
			
		# the ad-hoc query option
		def handleQ5():
			command = freeOpt.get()
			
			# try to do the user inputted command
			try:
				cursor.execute(command)
				info = cursor.fetchall()
				result.config(state=NORMAL)
				
				# display the result of the query
				for item in info:
					for thing in item:
						result.insert(INSERT, str(thing) + "     ")
						
					result.insert(INSERT, "\n")
				
				result.insert(INSERT, "----------------------------------\n")
				result.yview(END)
				result.config(state=DISABLED)
				
			except:
				result.config(state=NORMAL)
				result.insert(INSERT, "Error: invalid command\n")
				result.insert(INSERT, "----------------------------------\n")
				result.yview(END)
				result.config(state=DISABLED)
			
		# clears the result box
		def clear():
			result.config(state=NORMAL)
			result.delete(1.0, END)
			result.config(state=DISABLED)
		
		# the query GUI
		query = Toplevel(padx=10, pady=10)
		query.title("Query Information")
		
		listFrame = Frame(query, padx=10, pady=5)
		listFrame.grid(row=0, column=0)
		
		infoFrame = Frame(query, padx=10, pady=5)
		infoFrame.grid(row=0, column=1)
		
		mapFrame = Frame(query, padx=10, pady=5)
		Button(mapFrame, text="Map It", padx=5, pady=5, command=mapIt).pack()
		mapFrame.grid(row=1, column=0)
		
		queryFrame = Frame(query, padx=10, pady=5)
		queryFrame.grid(row=2, column=0, columnspan=2)
		
		resultFrame = Frame(query, padx=10, pady=5)
		resultFrame.grid(row=3, column=0, columnspan=2)
		
		# the frame for the hikes listbox
		Label(listFrame, text="Current Hikes:", width=30, anchor=W, justify=LEFT).pack()
		scroll1 = Scrollbar(listFrame, orient=VERTICAL)
		hikes = Listbox(listFrame, exportselection=0, selectmode=MULTIPLE, yscrollcommand=scroll1.set, font=("Helvetica", 10), width=30, height=10)
		scroll1.config(command=hikes.yview)
		scroll1.pack(side=RIGHT, fill=Y)
		hikes.bind("<ButtonRelease-1>", fillInfo)
		hikes.pack(side=LEFT, fill=BOTH, expand=1)
		
		# the info Frame
		location = StringVar()
		Label(infoFrame, text="Location:", pady=5, anchor=W, justify=LEFT).grid(row=0, column=0)
		Label(infoFrame, textvariable=location, width=30, padx=5).grid(row=0, column=1)
		
		comment = StringVar()
		Label(infoFrame, text="Comment:", pady=5, anchor=W, justify=LEFT).grid(row=1, column=0)
		Label(infoFrame, textvariable=comment, padx=5).grid(row=1, column=1)
		
		rating = StringVar()
		Label(infoFrame, text="Rating:", pady=5, anchor=W, justify=LEFT).grid(row=2, column=0)
		Label(infoFrame, textvariable=rating, padx=5).grid(row=2, column=1)
		
		legs = StringVar()
		Label(infoFrame, text="Number of legs:", pady=5, anchor=W, justify=LEFT).grid(row=3, column=0)
		Label(infoFrame, textvariable=legs, padx=5).grid(row=3, column=1)
		
		Label(infoFrame, text="Notes:", anchor=W, justify=LEFT).grid(row=4, column=0)
		noteText = Text(infoFrame, wrap=WORD, width=40, height=5)
		noteText.config(state=DISABLED)
		noteText.grid(row=5, column=0, columnspan=2)
		
		# the query frame
		Label(queryFrame, text="Queries:", font=("Helvetica", 12), width=45, pady=5, anchor=W, justify=LEFT).grid(row=0, column=0)
		Label(queryFrame, text="Find the name of the shortest hike in:", width=45, pady=5, anchor=W, justify=LEFT).grid(row=1, column=0)
		Label(queryFrame, text="Find the name of the longest hike in:", width=45, pady=5, anchor=W, justify=LEFT).grid(row=2, column=0)
		Label(queryFrame, text="How many hikes are in:", width=45, pady=5, anchor=W, justify=LEFT).grid(row=3, column=0)
		Label(queryFrame, text="How many hikes have more than this number of legs:", width=45, pady=5, anchor=W, justify=LEFT).grid(row=4, column=0)
		freeOpt = Entry(queryFrame, width=65)
		freeOpt.insert(INSERT, "SELECT ")
		freeOpt.grid(row=5, column=0, columnspan=2)
		
		q1 = Entry(queryFrame, width=20)
		q1.grid(row=1, column=1)
		q2 = Entry(queryFrame, width=20)
		q2.grid(row=2, column=1)
		q3 = Entry(queryFrame, width=20)
		q3.grid(row=3, column=1)
		q4 = Entry(queryFrame, width=20)
		q4.grid(row=4, column=1)
		
		Button(queryFrame, text="Submit", padx=5, pady=5, command=handleQ1).grid(row=1, column=2)
		Button(queryFrame, text="Submit", padx=5, pady=5, command=handleQ2).grid(row=2, column=2)
		Button(queryFrame, text="Submit", padx=5, pady=5, command=handleQ3).grid(row=3, column=2)
		Button(queryFrame, text="Submit", padx=5, pady=5, command=handleQ4).grid(row=4, column=2)
		Button(queryFrame, text="Submit", padx=5, pady=5, command=handleQ5).grid(row=5, column=2)
		
		# the results frame
		Label(resultFrame, text="Results:", font=("Helvetica", 12), width=69, anchor=W, justify=LEFT).pack()
		
		scroll = Scrollbar(resultFrame)
		scroll.pack(side=RIGHT, fill=Y)
		result = Text(resultFrame, yscrollcommand=scroll.set, wrap=WORD, width=80, height=8)
		result.config(state=DISABLED)
		scroll.config(command=result.yview)
		result.pack(side=LEFT, fill=BOTH)
		
		Button(query, text="Clear", padx=5, command=clear).grid(row=4, column=0, columnspan=2)
		
		intitialise()
		query.mainloop()
	
# a simple, modal window
# NOTE, not exactly modal as it doesnt take control of other window
def msgWindow(title, message):
	msg = Toplevel(padx=5, pady=5)
	msg.title(title)
	
	def kill(event):
		msg.destroy()
	msg.bind("<Escape>", kill)
	
	Label(msg, text=message).grid(row=0, column=0)	
	Button(msg, text="Cancel", command=msg.destroy).grid(row=1, column=0, sticky=E, padx=5)
	Button(msg, text="OK", command=msg.destroy).grid(row=1, column=1, sticky=E)
	
	msg.mainloop()
	
# a simple, selection window
# NOTE, doesnt return the values when would be expected
def modal(title, message):
	msg = Toplevel(width=100, padx=5, pady=5)
	msg.title(title)
	
	def ok():
		msg.destroy()
		print "here"
		return 1
	
	def cancel():
		msg.destroy()
		return 0
	
	def kill(event):
		msg.destroy()
		return 0;
	msg.bind("<Escape>", cancel)
	
	Label(msg, text=message).grid(row=0, column=0)
	Button(msg, text="Cancel", command=cancel).grid(row=1, column=0, sticky=E, padx=5)
	Button(msg, text="OK", command=ok).grid(row=1, column=1, sticky=E)
	
	msg.mainloop()
	
	
#######################
#
#-- FROM HERE DOWN IS THE GUI
#
#######################

root = Tk()
root.title("David's GPS Program")
root.protocol("WM_DELETE_WINDOW", exitGui)

# Bind some keybard inputs
root.bind("<Control-o>", openGps)
root.bind("<Control-s>", save)
root.bind("<Control-Alt-s>", saveAs)
	
# Make the menu
menubar = Menu(root)

# Make the file dropdown menu
filemenu = Menu(menubar, tearoff=0)
filemenu.add_command(label="Open...", command=lambda: openGps(''))
filemenu.add_separator()
filemenu.add_command(label="Save", state="disabled", command=lambda: save(''))
filemenu.add_command(label="Save As...", state="disabled", command=lambda: saveAs(''))
filemenu.add_separator()
filemenu.add_command(label="Merge...", state="disabled", command=merge)
filemenu.add_separator()
filemenu.add_command(label="Exit", command=exitGui)

# Make the edit dropdown menu
editmenu = Menu(menubar, tearoff=0)
editmenu.add_command(label="Discard...", state="disabled", command=lambda: remove("discard"))
editmenu.add_command(label="Keep...", state="disabled", command=lambda: remove("keep"))
sort = editmenu.add_command(label="Sort", state="disabled", command=sort)

# Make the hike dropdown menu
hikemenu = Menu(menubar, tearoff=0)
hikemenu.add_command(label="Store...", state="disabled", command=store)
hikemenu.add_command(label="Query...", command=query)

# Add all the dropdowns/buttons to the main menu
menubar.add_cascade(label="File", menu=filemenu)
menubar.add_cascade(label="Edit", menu=editmenu)
menubar.add_cascade(label="Hikes", menu=hikemenu)
#menubar.add_command(label="Help", command=help)
root.config(menu=menubar)

# Make the File Info Display frame
Label(root, text="File Info Display", anchor=S, height=2, font=("Helvetica", 16)).pack()
infoFrame = Frame(borderwidth=3, relief=GROOVE)

scroll = Scrollbar(infoFrame)
scroll.pack(side=RIGHT, fill=Y)
info = Text(infoFrame, yscrollcommand=scroll.set, width=95, height=8)
info.config(state=DISABLED)
scroll.config(command=info.yview)
info.pack(side=LEFT, fill=BOTH)

infoFrame.pack()

# Make the Map Control Panel frame
Label(root, text="Map Control Panel Display", anchor=S, height=2, font=("Helvetica", 16)).pack()
mapFrame = Frame(borderwidth=3, relief=GROOVE)

# Make the labels and radio buttons
on = Label(mapFrame, text="ON")
off = Label(mapFrame, text="OFF")
colour = Label(mapFrame, text="Colour")

waypt = Label(mapFrame, text="Waypoints")
routes = Label(mapFrame, text="Routes")
trkpt = Label(mapFrame, text="Trackpoints")
track = Label(mapFrame, text="Tracks")

w = IntVar()
wOn = Radiobutton(mapFrame, variable=w, value=0)
wOff = Radiobutton(mapFrame, variable=w, value=1)
wButton = Button(mapFrame, text="Change", command=lambda: getColour('w'))
wBox = Canvas(mapFrame, width=20, height=20)
wBox.create_rectangle(0, 0, 20, 20, fill='red')

r = IntVar()
rOn = Radiobutton(mapFrame, variable=r, value=0)
rOff = Radiobutton(mapFrame, variable=r, value=1)
rButton = Button(mapFrame, text="Choose", command=lambda: chooseColour('r'))
rBox = Canvas(mapFrame, width=20, height=20)
rBox.create_rectangle(0, 0, 20, 20, fill='green')

t = IntVar()
tOn = Radiobutton(mapFrame, variable=t, value=0)
tOff = Radiobutton(mapFrame, variable=t, value=1)
tButton = Button(mapFrame, text="Choose", command=lambda: getColour('t'))
tBox = Canvas(mapFrame, width=20, height=20)
tBox.create_rectangle(0, 0, 20, 20, fill='blue')

tr = IntVar()
trOn = Radiobutton(mapFrame, variable=tr, value=0)
trOff = Radiobutton(mapFrame, variable=tr, value=1)
trButton = Button(mapFrame, text="Choose", command=lambda: chooseColour('tr'))
trBox = Canvas(mapFrame, width=20, height=20)
trBox.create_rectangle(0, 0, 20, 20, fill='yellow')

update = Button(mapFrame, text="Update Display", command=updateDisplay)

# Place them in a grid
on.grid(row=0, column=1, padx=20)
off.grid(row=0, column=2, padx=20)
colour.grid(row=0, column=3, padx=20)

waypt.grid(row=1, column=0, sticky=E, pady=3)
routes.grid(row=2, column=0, sticky=E, pady=3)
trkpt.grid(row=3, column=0, sticky=E, pady=3)
track.grid(row=4, column=0, sticky=E, pady=3)

wOn.grid(row=1, column=1)
wOff.grid(row=1, column=2)
wButton.grid(row=1, column=3, pady=3)
wBox.grid(row=1, column=4, padx=5)

rOn.grid(row=2, column=1)
rOff.grid(row=2, column=2)
rButton.grid(row=2, column=3, pady=3)
rBox.grid(row=2, column=4, padx=5)

tOn.grid(row=3, column=1)
tOff.grid(row=3, column=2)
tButton.grid(row=3, column=3, pady=3)
tBox.grid(row=3, column=4, padx=5)

trOn.grid(row=4, column=1)
trOff.grid(row=4, column=2)
trButton.grid(row=4, column=3, pady=3)
trBox.grid(row=4, column=4, padx=5)

update.grid(row=4, column = 5, padx=10)

mapFrame.pack()

listFrame = Frame()
routeFrame = Frame(listFrame, padx=15, pady=5)
trackFrame = Frame(listFrame, padx=15, pady=5)
routeFrame.grid(row=0, column=0)
trackFrame.grid(row=0, column=1)

# display the routes and tracks
Label(routeFrame, text="Routes", font=("Helvetica", 12), width=40, anchor=W, justify=LEFT).pack()
listScroll = Scrollbar(routeFrame, orient=VERTICAL)
routeList = Listbox(routeFrame, selectmode=MULTIPLE, exportselection=0, yscrollcommand=listScroll.set, width=40, height=7)
listScroll.config(command=routeList.yview)
listScroll.pack(side=RIGHT, fill=Y)
routeList.pack(side=LEFT, fill=BOTH, expand=1)

Label(trackFrame, text="Tracks", font=("Helvetica", 12), width=40, anchor=W, justify=LEFT).pack()
listScroll2 = Scrollbar(trackFrame, orient=VERTICAL)
trackList = Listbox(trackFrame, selectmode=MULTIPLE, exportselection=0, yscrollcommand=listScroll2.set, width=40, height=7)
listScroll2.config(command=trackList.yview)
listScroll2.pack(side=RIGHT, fill=Y)
trackList.pack(side=LEFT, fill=BOTH, expand=1)

listFrame.pack()

# Make the Log Display frame
Label(root, text="Log Display", anchor=S, height=2, font=("Helvetica", 16)).pack()
logFrame = Frame(width=700, height=50, borderwidth=3, relief=GROOVE)

logScroll = Scrollbar(logFrame)
logScroll.pack(side=RIGHT, fill=Y)
log = Text(logFrame, yscrollcommand=logScroll.set, width=95, height=8)
log.config(state=DISABLED)
logScroll.config(command=log.yview)
log.pack(side=LEFT, fill=BOTH)

logFrame.pack()

Button(root, text='Clear', command=clear).pack(pady=5) # the clear button for the log info

root.mainloop()
