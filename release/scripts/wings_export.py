#!BPY

"""
Name: 'Wings3D...'
Blender: 232
Group: 'Export'
Tooltip: 'Export selected mesh to Wings3D File Format (*.wings)'
"""

# +---------------------------------------------------------+
# | Copyright (c) 2002 Anthony D'Agostino                   |
# | http://www.redrival.com/scorpius                        |
# | scorpius@netzero.com                                    |
# | Feb 19, 2002                                            |
# | Released under the Blender Artistic Licence (BAL)       |
# | Import Export Suite v0.5                                |
# +---------------------------------------------------------+
# | Read and write Wings3D File Format (*.wings)            |
# +---------------------------------------------------------+

import Blender, mod_meshtools
import struct, time, sys, os, zlib, cStringIO

# ===============================================
# === Write The 'Header' Common To All Chunks ===
# ===============================================
def write_chunkheader(data, version, tag, name):
	data.write(struct.pack(">BB", version, tag))
	data.write(struct.pack(">BH", 0x64, len(name)))
	data.write(name)

# ===================
# === Write Faces ===
# ===================
def write_faces(data, mesh):
	numfaces = len(mesh.faces)
	data.write(struct.pack(">BL", 0x6C, numfaces))
	#for i in range(numfaces):
	#	 if not i%100 and mod_meshtools.show_progress: Blender.Window.DrawProgressBar(float(i)/numfaces, "Writing Faces")
	#	 data.write("\x6A")
	data.write("\x6A" * numfaces) # same, but faster than the above loop
	data.write("\x6A")

# ===================
# === Write Verts ===
# ===================
def write_verts(data, mesh):
	numverts = len(mesh.verts)
	data.write(struct.pack(">BL", 0x6C, numverts))
	for i in range(numverts):
		if not i%100 and mod_meshtools.show_progress: Blender.Window.DrawProgressBar(float(i)/numverts, "Writing Verts")
		data.write(struct.pack(">BLBL", 0x6C, 1, 0x6D, 24))
		#data.write("\x6c\x00\x00\x00\x01\x6D\x00\x00\x00\x30")
		x, y, z = mesh.verts[i].co
		data.write(struct.pack(">ddd", x, z, -y))
		data.write("\x6A")
	data.write("\x6A")

# ===================
# === Write Edges ===
# ===================
def write_edges(data, mesh, edge_table):
	numedges = len(edge_table)
	data.write(struct.pack(">BL", 0x6C, numedges))
	keys = edge_table.keys()
	keys.sort()
	for key in keys:
		i = edge_table[key][6]
		if not i%100 and mod_meshtools.show_progress: Blender.Window.DrawProgressBar(float(i)/numedges, "Writing Edges")

		if mod_meshtools.has_vertex_colors(mesh):
			r1, g1, b1 = edge_table[key][7]
			r2, g2, b2 = edge_table[key][8]
			data.write("\x6C\x00\x00\x00\x02")
			data.write("\x68\x02\x64\x00\x05color")
			data.write("\x6D\x00\x00\x00\x30")
			data.write(struct.pack(">dddddd", r1, g1, b1, r2, g2, b2))
			#print "%f %f %f - %f %f %f" % (r1, g1, b1, r2, g2, b2)
		else:
			data.write("\x6C\x00\x00\x00\x01")  # BL

		#$write_chunkheader(data, 0x68, 0x09, "edge")
		data.write("\x68\x09\x64\x00\x04edge") # faster

		# Sv Ev (Reversed)
		data.write(struct.pack(">BLBL", 0x62, key[1], 0x62, key[0]))

		# Lf Rf LP LS RP RS
		for i in range(6):
			if edge_table[key][i] < 256:
				data.write(struct.pack(">BB", 0x61, edge_table[key][i]))
			else:
				data.write(struct.pack(">BL", 0x62, edge_table[key][i]))

		data.write("\x6A")

	data.write("\x6A")

# ===============================
# === Write The Material Mode ===
# ===============================
def write_mode(data, mesh):
	data.write("\x6A")
	data.write(struct.pack(">BL", 0x6C, 1))
	write_chunkheader(data, 0x68, 0x02, "mode")
	if mod_meshtools.has_vertex_colors(mesh):
		data.write(struct.pack(">BH6s", 0x64, 6, "vertex"))
	else:
		data.write(struct.pack(">BH8s", 0x64, 8, "material"))
	data.write("\x6A")

# ======================
# === Write Material ===
# ======================
def write_material(data, mesh):
	data.write("\x6A")
	data.write(struct.pack(">BL", 0x6C, 1))
	write_chunkheader(data, 0x68, 0x02, "my default")

	data.write(struct.pack(">BL", 0x6C, 2))
	write_chunkheader(data, 0x68, 0x02, "maps")
	data.write("\x6A")
	write_chunkheader(data, 0x68, 0x02, "opengl")

	# === The Material Components ===
	data.write(struct.pack(">BL", 0x6C, 5))

	write_chunkheader(data, 0x68, 0x02, "diffuse")
	data.write("\x68\x04")
	data.write("\x63"+"1.00000000000000000000"+"e+000"+"\x00"*4)
	data.write("\x63"+"1.00000000000000000000"+"e+000"+"\x00"*4)
	data.write("\x63"+"1.00000000000000000000"+"e+000"+"\x00"*4)
	data.write("\x63"+"1.00000000000000000000"+"e+000"+"\x00"*4)

	write_chunkheader(data, 0x68, 0x02, "ambient")
	data.write("\x68\x04")
	data.write("\x63"+"1.00000000000000000000"+"e+000"+"\x00"*4)
	data.write("\x63"+"1.00000000000000000000"+"e+000"+"\x00"*4)
	data.write("\x63"+"1.00000000000000000000"+"e+000"+"\x00"*4)
	data.write("\x63"+"1.00000000000000000000"+"e+000"+"\x00"*4)

	write_chunkheader(data, 0x68, 0x02, "specular")
	data.write("\x68\x04")
	data.write("\x63"+"1.00000000000000000000"+"e+000"+"\x00"*4)
	data.write("\x63"+"1.00000000000000000000"+"e+000"+"\x00"*4)
	data.write("\x63"+"1.00000000000000000000"+"e+000"+"\x00"*4)
	data.write("\x63"+"1.00000000000000000000"+"e+000"+"\x00"*4)

	write_chunkheader(data, 0x68, 0x02, "emission")
	data.write("\x68\x04")
	data.write("\x63"+"0.00000000000000000000"+"e+000"+"\x00"*4)
	data.write("\x63"+"0.00000000000000000000"+"e+000"+"\x00"*4)
	data.write("\x63"+"0.00000000000000000000"+"e+000"+"\x00"*4)
	data.write("\x63"+"0.00000000000000000000"+"e+000"+"\x00"*4)

	write_chunkheader(data, 0x68, 0x02, "shininess")
	data.write("\x63"+"0.00000000000000000000"+"e+000"+"\x00"*4)

	#write_chunkheader(data, 0x68, 0x02, "twosided")
	#data.write(struct.pack(">BH4s", 0x64, 4, "true"))

	data.write("\x6A"*3)    # use *4 if no ambient light

# =====================
# === Generate Data ===
# =====================
def generate_data(objname, edge_table, mesh):
	data = cStringIO.StringIO()

	# === wings chunk ===
	write_chunkheader(data, 0x68, 0x03, "wings")

	numobjs = 1 # len(Blender.Object.GetSelected())
	data.write("\x61\x02\x68\x03") # misc bytes
	data.write(struct.pack(">BL", 0x6C, numobjs))

	# === object chunk ===
	write_chunkheader(data, 0x68, 0x04, "object")
	data.write(struct.pack(">BH", 0x6B, len(objname)))
	data.write(objname)

	# === winged chunk ===
	write_chunkheader(data, 0x68, 0x05, "winged")
	write_edges(data, mesh, edge_table)
	write_faces(data, mesh)
	write_verts(data, mesh)
	write_mode(data, mesh)
	write_material(data, mesh)
	write_ambient_light(data)
	return data.getvalue()

# ===========================
# === Write Ambient Light ===
# ===========================
def write_ambient_light(data):
	light = [	# A quick cheat ;)
	0x6C, 0x00, 0x00, 0x00, 0x01, 0x68, 0x02, 0x64, 0x00, 0x06, 0x6C, 0x69,
	0x67, 0x68, 0x74, 0x73, 0x6C, 0x00, 0x00, 0x00, 0x01, 0x68, 0x02, 0x6B,
	0x00, 0x07, 0x41, 0x6D, 0x62, 0x69, 0x65, 0x6E, 0x74, 0x6C, 0x00, 0x00,
	0x00, 0x08, 0x68, 0x02, 0x64, 0x00, 0x07, 0x76, 0x69, 0x73, 0x69, 0x62,
	0x6C, 0x65, 0x64, 0x00, 0x04, 0x74, 0x72, 0x75, 0x65, 0x68, 0x02, 0x64,
	0x00, 0x06, 0x6C, 0x6F, 0x63, 0x6B, 0x65, 0x64, 0x64, 0x00, 0x05, 0x66,
	0x61, 0x6C, 0x73, 0x65, 0x68, 0x02, 0x64, 0x00, 0x06, 0x6F, 0x70, 0x65,
	0x6E, 0x67, 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x03, 0x68, 0x02, 0x64, 0x00,
	0x04, 0x74, 0x79, 0x70, 0x65, 0x64, 0x00, 0x07, 0x61, 0x6D, 0x62, 0x69,
	0x65, 0x6E, 0x74, 0x68, 0x02, 0x64, 0x00, 0x07, 0x61, 0x6D, 0x62, 0x69,
	0x65, 0x6E, 0x74, 0x68, 0x04, 0x63, 0x31, 0x2E, 0x30, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x65, 0x2B, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00,
	0x00, 0x63, 0x31, 0x2E, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x65, 0x2B, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x63, 0x31, 0x2E,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x65, 0x2B, 0x30, 0x30,
	0x30, 0x00, 0x00, 0x00, 0x00, 0x63, 0x31, 0x2E, 0x30, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x65, 0x2B, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00,
	0x00, 0x68, 0x02, 0x64, 0x00, 0x08, 0x70, 0x6F, 0x73, 0x69, 0x74, 0x69,
	0x6F, 0x6E, 0x68, 0x03, 0x63, 0x30, 0x2E, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x65, 0x2B, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00,
	0x63, 0x33, 0x2E, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x65,
	0x2B, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x63, 0x30, 0x2E, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x65, 0x2B, 0x30, 0x30, 0x30,
	0x00, 0x00, 0x00, 0x00, 0x6A, 0x68, 0x02, 0x64, 0x00, 0x07, 0x76, 0x69,
	0x73, 0x69, 0x62, 0x6C, 0x65, 0x64, 0x00, 0x04, 0x74, 0x72, 0x75, 0x65,
	0x68, 0x02, 0x64, 0x00, 0x06, 0x6C, 0x6F, 0x63, 0x6B, 0x65, 0x64, 0x64,
	0x00, 0x05, 0x66, 0x61, 0x6C, 0x73, 0x65, 0x68, 0x02, 0x64, 0x00, 0x06,
	0x79, 0x61, 0x66, 0x72, 0x61, 0x79, 0x6C, 0x00, 0x00, 0x00, 0x0B, 0x68,
	0x02, 0x64, 0x00, 0x09, 0x6D, 0x69, 0x6E, 0x69, 0x6D, 0x69, 0x7A, 0x65,
	0x64, 0x64, 0x00, 0x04, 0x74, 0x72, 0x75, 0x65, 0x68, 0x02, 0x64, 0x00,
	0x05, 0x70, 0x6F, 0x77, 0x65, 0x72, 0x63, 0x31, 0x2E, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x65, 0x2B, 0x30, 0x30, 0x30, 0x00, 0x00,
	0x00, 0x00, 0x68, 0x02, 0x64, 0x00, 0x04, 0x74, 0x79, 0x70, 0x65, 0x64,
	0x00, 0x09, 0x68, 0x65, 0x6D, 0x69, 0x6C, 0x69, 0x67, 0x68, 0x74, 0x68,
	0x02, 0x64, 0x00, 0x07, 0x73, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x73, 0x62,
	0x00, 0x00, 0x01, 0x00, 0x68, 0x02, 0x64, 0x00, 0x05, 0x64, 0x65, 0x70,
	0x74, 0x68, 0x61, 0x03, 0x68, 0x02, 0x64, 0x00, 0x0A, 0x62, 0x61, 0x63,
	0x6B, 0x67, 0x72, 0x6F, 0x75, 0x6E, 0x64, 0x64, 0x00, 0x09, 0x75, 0x6E,
	0x64, 0x65, 0x66, 0x69, 0x6E, 0x65, 0x64, 0x68, 0x02, 0x64, 0x00, 0x18,
	0x62, 0x61, 0x63, 0x6B, 0x67, 0x72, 0x6F, 0x75, 0x6E, 0x64, 0x5F, 0x66,
	0x69, 0x6C, 0x65, 0x6E, 0x61, 0x6D, 0x65, 0x5F, 0x48, 0x44, 0x52, 0x49,
	0x6A, 0x68, 0x02, 0x64, 0x00, 0x19, 0x62, 0x61, 0x63, 0x6B, 0x67, 0x72,
	0x6F, 0x75, 0x6E, 0x64, 0x5F, 0x66, 0x69, 0x6C, 0x65, 0x6E, 0x61, 0x6D,
	0x65, 0x5F, 0x69, 0x6D, 0x61, 0x67, 0x65, 0x6A, 0x68, 0x02, 0x64, 0x00,
	0x1A, 0x62, 0x61, 0x63, 0x6B, 0x67, 0x72, 0x6F, 0x75, 0x6E, 0x64, 0x5F,
	0x65, 0x78, 0x70, 0x6F, 0x73, 0x75, 0x72, 0x65, 0x5F, 0x61, 0x64, 0x6A,
	0x75, 0x73, 0x74, 0x61, 0x00, 0x68, 0x02, 0x64, 0x00, 0x12, 0x62, 0x61,
	0x63, 0x6B, 0x67, 0x72, 0x6F, 0x75, 0x6E, 0x64, 0x5F, 0x6D, 0x61, 0x70,
	0x70, 0x69, 0x6E, 0x67, 0x64, 0x00, 0x05, 0x70, 0x72, 0x6F, 0x62, 0x65,
	0x68, 0x02, 0x64, 0x00, 0x10, 0x62, 0x61, 0x63, 0x6B, 0x67, 0x72, 0x6F,
	0x75, 0x6E, 0x64, 0x5F, 0x70, 0x6F, 0x77, 0x65, 0x72, 0x63, 0x31, 0x2E,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x65, 0x2B, 0x30, 0x30,
	0x30, 0x00, 0x00, 0x00, 0x00, 0x6A, 0x68, 0x02, 0x64, 0x00, 0x07, 0x76,
	0x69, 0x73, 0x69, 0x62, 0x6C, 0x65, 0x64, 0x00, 0x04, 0x74, 0x72, 0x75,
	0x65, 0x68, 0x02, 0x64, 0x00, 0x06, 0x6C, 0x6F, 0x63, 0x6B, 0x65, 0x64,
	0x64, 0x00, 0x05, 0x66, 0x61, 0x6C, 0x73, 0x65, 0x6A, 0x6A, 0x6A]
	data.write("".join(map(chr, light)))

# ==========================
# === Write Wings Format ===
# ==========================
def write(filename):
	start = time.clock()

	objects = Blender.Object.GetSelected()
	objname = objects[0].name
	meshname = objects[0].data.name
	mesh = Blender.NMesh.GetRaw(meshname)
	obj = Blender.Object.Get(objname)

	try:
		edge_table = mod_meshtools.generate_edgetable(mesh)
	except:
		edge_table = {}
		message = "Unable to generate\nEdge Table for mesh.\n"
		message += "Object name is: " + meshname
		mod_meshtools.print_boxed(message)
		#return 

		if 0:
			import Tkinter, tkMessageBox
			sys.argv=['wings.pyo','wings.pyc'] # ?

			#Tkinter.NoDefaultRoot()
			win1 = Tkinter.Tk()
			ans = tkMessageBox.showerror("Error", message)
			win1.pack()
			print ans
			if ans:
				win1.quit()
			win1.mainloop()

		else:
			from Tkinter import Label
			sys.argv = 'wings.py'
			widget = Label(None, text=message)
			#widget.title("Error")
			widget.pack()
			widget.mainloop()

	data = generate_data(objname, edge_table, mesh)
	dsize = len(data)
	Blender.Window.DrawProgressBar(0.98, "Compressing Data")
	data = zlib.compress(data, 6)
	fsize = len(data)+6
	header = "#!WINGS-1.0\r\n\032\04"
	misc = 0x8350

	file = open(filename, "wb")
	file.write(header)
	file.write(struct.pack(">L", fsize))
	file.write(struct.pack(">H", misc))
	file.write(struct.pack(">L", dsize))
	file.write(data)

	# Blender.Window.RedrawAll()
	Blender.Window.DrawProgressBar(1.0, '')  # clear progressbar
	file.close()
	end = time.clock()
	seconds = " in %.2f %s" % (end-start, "seconds")
	message = "Successfully exported " + os.path.basename(filename) + seconds + '\n\n'
	message += "objname : " + objname + '\n'
	message += "faces   : " + `len(mesh.faces)` + '\n'
	message += "edges   : " + `len(edge_table)` + '\n'
	message += "verts   : " + `len(mesh.verts)` + '\n'
	mod_meshtools.print_boxed(message)

def fs_callback(filename):
	if filename.find('.wings', -6) <= 0: filename += '.wings'
	write(filename)

Blender.Window.FileSelector(fs_callback, "Wings3D Export")
