# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>

import bpy
from rigify_utils import copy_bone_simple, get_side_name, bone_class_instance
from rna_prop_ui import rna_idprop_ui_prop_get

METARIG_NAMES = ("cpy",)

# note, this example is just a bone with copy property.
def metarig_template():
    # generated by rigify.write_meta_rig
    bpy.ops.object.mode_set(mode='EDIT')
    obj = bpy.context.active_object
    arm = obj.data
    bone = arm.edit_bones.new('Bone')
    bone.head[:] = 0.0000, 0.0000, 0.0000
    bone.tail[:] = 0.0000, 0.0000, 1.0000
    bone.roll = 0.0000
    bone.connected = False

    bpy.ops.object.mode_set(mode='OBJECT')
    pbone = obj.pose.bones['Bone']
    pbone['type'] = 'copy'

def metarig_definition(obj, orig_bone_name):
    return [orig_bone_name]


def main(obj, bone_definition, base_names):
    arm = obj.data
    mt = bone_class_instance(obj, METARIG_NAMES)
    mt.cpy = bone_definition[0]
    mt.update()
    cp = mt.copy(to_fmt="%s_cpy")
    bpy.ops.object.mode_set(mode='OBJECT')
    
    cp.update()
    mt.update()
    
    con = cp.cpy_p.constraints.new('COPY_ROTATION')
    con.target = obj
    con.subtarget = mt.cpy
    
    con = cp.cpy_p.constraints.new('COPY_LOCATION')
    con.target = obj
    con.subtarget = mt.cpy

    return [mt.cpy]
