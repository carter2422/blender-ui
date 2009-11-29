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


class FILEBROWSER_HT_header(bpy.types.Header):
    bl_space_type = 'FILE_BROWSER'

    def draw(self, context):
        layout = self.layout

        st = context.space_data
        params = st.params

        layout.template_header(menus=False)

        row = layout.row()
        row.separator()

        row = layout.row(align=True)
        row.operator("file.previous", text="", icon='ICON_BACK')
        row.operator("file.next", text="", icon='ICON_FORWARD')
        row.operator("file.parent", text="", icon='ICON_FILE_PARENT')
        row.operator("file.refresh", text="", icon='ICON_FILE_REFRESH')

        row = layout.row()
        row.separator()

        row = layout.row(align=True)
        row.operator("file.directory_new", text="", icon='ICON_NEWFOLDER')

        layout.prop(params, "display", expand=True, text="")
        layout.prop(params, "sort", expand=True, text="")

        layout.prop(params, "hide_dot", text="Hide Invisible")
        layout.prop(params, "do_filter", text="", icon='ICON_FILTER')

        row = layout.row(align=True)
        row.active = params.do_filter

        row.prop(params, "filter_folder", text="")
        row.prop(params, "filter_blender", text="")
        row.prop(params, "filter_image", text="")
        row.prop(params, "filter_movie", text="")
        row.prop(params, "filter_script", text="")
        row.prop(params, "filter_font", text="")
        row.prop(params, "filter_sound", text="")
        row.prop(params, "filter_text", text="")

bpy.types.register(FILEBROWSER_HT_header)
