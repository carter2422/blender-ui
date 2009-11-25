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


class View3DPanel(bpy.types.Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'TOOLS'


# ********** default tools for objectmode ****************


class VIEW3D_PT_tools_objectmode(View3DPanel):
    bl_context = "objectmode"
    bl_label = "Object Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.label(text="Transform:")
        col.operator("tfm.translate")
        col.operator("tfm.rotate")
        col.operator("tfm.resize", text="Scale")

        col = layout.column(align=True)
        col.label(text="Object:")
        col.operator("object.duplicate_move")
        col.operator("object.delete")
        col.operator("object.join")

        active_object = context.active_object
        if active_object and active_object.type == 'MESH':

            col = layout.column(align=True)
            col.label(text="Shading:")
            col.operator("object.shade_smooth", text="Smooth")
            col.operator("object.shade_flat", text="Flat")

        col = layout.column(align=True)
        col.label(text="Keyframes:")
        col.operator("anim.insert_keyframe_menu", text="Insert")
        col.operator("anim.delete_keyframe_v3d", text="Remove")

        col = layout.column(align=True)
        col.label(text="Repeat:")
        col.operator("screen.repeat_last")
        col.operator("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.label(text="Grease Pencil:")
        row = col.row()
        row.operator("gpencil.draw", text="Draw").mode = 'DRAW'
        row.operator("gpencil.draw", text="Line").mode = 'DRAW_STRAIGHT'
        row.operator("gpencil.draw", text="Erase").mode = 'ERASER'

# ********** default tools for editmode_mesh ****************


class VIEW3D_PT_tools_meshedit(View3DPanel):
    bl_context = "mesh_edit"
    bl_label = "Mesh Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.label(text="Transform:")
        col.operator("tfm.translate")
        col.operator("tfm.rotate")
        col.operator("tfm.resize", text="Scale")
        col.operator("tfm.shrink_fatten", text="Along Normal")


        col = layout.column(align=True)
        col.label(text="Deform:")
        col.operator("tfm.edge_slide")
        col.operator("mesh.rip_move")
        col.operator("mesh.vertices_smooth")


        col = layout.column(align=True)
        col.label(text="Add:")
        col.operator("mesh.extrude_move")
        col.operator("mesh.subdivide")
        col.operator("mesh.loopcut")
        col.operator("mesh.duplicate_move")
        col.operator("mesh.spin")
        col.operator("mesh.screw")
        col.itemO("mesh.flip_normals")

        col = layout.column(align=True)
        col.label(text="Remove:")
        col.operator("mesh.delete")
        col.operator("mesh.merge")
        col.operator("mesh.remove_doubles")

        col = layout.column(align=True)
        col.label(text="Normals:")
        col.operator("mesh.normals_make_consistent", text="Recalculate")
        col.operator("mesh.flip_normals", text="Flip Direction")

        col = layout.column(align=True)
        col.label(text="UV Mapping:")
        col.operator("wm.call_menu", text="Unwrap").name = "VIEW3D_MT_uv_map"
        col.operator("mesh.mark_seam")
        col.operator("mesh.mark_seam", text="Clear Seam").clear = True


        col = layout.column(align=True)
        col.label(text="Shading:")
        col.operator("mesh.faces_shade_smooth", text="Smooth")
        col.operator("mesh.faces_shade_flat", text="Flat")

        col = layout.column(align=True)
        col.label(text="Repeat:")
        col.operator("screen.repeat_last")
        col.operator("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.label(text="Grease Pencil:")
        row = col.row()
        row.operator("gpencil.draw", text="Draw").mode = 'DRAW'
        row.operator("gpencil.draw", text="Line").mode = 'DRAW_STRAIGHT'
        row.operator("gpencil.draw", text="Erase").mode = 'ERASER'


class VIEW3D_PT_tools_meshedit_options(View3DPanel):
    bl_context = "mesh_edit"
    bl_label = "Mesh Options"

    def draw(self, context):
        layout = self.layout

        ob = context.active_object

        if ob:
            mesh = context.active_object.data
            col = layout.column(align=True)
            col.prop(mesh, "use_mirror_x")

# ********** default tools for editmode_curve ****************


class VIEW3D_PT_tools_curveedit(View3DPanel):
    bl_context = "curve_edit"
    bl_label = "Curve Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.label(text="Transform:")
        col.operator("tfm.translate")
        col.operator("tfm.rotate")
        col.operator("tfm.resize", text="Scale")
        
        col = layout.column(align=True)
        col.operator("tfm.transform").mode = 'TILT'
        col.operator("tfm.transform").mode = 'CURVE_SHRINKFATTEN'

        col = layout.column(align=True)
        col.label(text="Curve:")
        col.operator("curve.duplicate")
        col.operator("curve.delete")
        col.operator("curve.cyclic_toggle")
        col.operator("curve.switch_direction")
        col.operator("curve.spline_type_set")

        col = layout.column(align=True)
        col.label(text="Handles:")
        row = col.row()
        row.operator("curve.handle_type_set", text="Auto").type = 'AUTOMATIC'
        row.operator("curve.handle_type_set").type = 'VECTOR'
        row = col.row()
        row.operator("curve.handle_type_set").type = 'ALIGN'
        row.operator("curve.handle_type_set", text="Free").type = 'FREE_ALIGN'

        col = layout.column(align=True)
        col.label(text="Modeling:")
        col.operator("curve.extrude")
        col.operator("curve.subdivide")

        col = layout.column(align=True)
        col.label(text="Repeat:")
        col.operator("screen.repeat_last")
        col.operator("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.label(text="Grease Pencil:")
        row = col.row()
        row.operator("gpencil.draw", text="Draw").mode = 'DRAW'
        row.operator("gpencil.draw", text="Line").mode = 'DRAW_STRAIGHT'
        row.operator("gpencil.draw", text="Erase").mode = 'ERASER'

# ********** default tools for editmode_surface ****************


class VIEW3D_PT_tools_surfaceedit(View3DPanel):
    bl_context = "surface_edit"
    bl_label = "Surface Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.label(text="Transform:")
        col.operator("tfm.translate")
        col.operator("tfm.rotate")
        col.operator("tfm.resize", text="Scale")

        col = layout.column(align=True)
        col.label(text="Curve:")
        col.operator("curve.duplicate")
        col.operator("curve.delete")
        col.operator("curve.cyclic_toggle")
        col.operator("curve.switch_direction")

        col = layout.column(align=True)
        col.label(text="Modeling:")
        col.operator("curve.extrude")
        col.operator("curve.subdivide")

        col = layout.column(align=True)
        col.label(text="Repeat:")
        col.operator("screen.repeat_last")
        col.operator("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.label(text="Grease Pencil:")
        row = col.row()
        row.operator("gpencil.draw", text="Draw").mode = 'DRAW'
        row.operator("gpencil.draw", text="Line").mode = 'DRAW_STRAIGHT'
        row.operator("gpencil.draw", text="Erase").mode = 'ERASER'

# ********** default tools for editmode_text ****************


class VIEW3D_PT_tools_textedit(View3DPanel):
    bl_context = "text_edit"
    bl_label = "Text Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.label(text="Text Edit:")
        col.operator("font.text_copy", text="Copy")
        col.operator("font.text_cut", text="Cut")
        col.operator("font.text_paste", text="Paste")

        col = layout.column(align=True)
        col.label(text="Set Case:")
        col.operator("font.case_set", text="To Upper").case = 'UPPER'
        col.operator("font.case_set", text="To Lower").case = 'LOWER'

        col = layout.column(align=True)
        col.label(text="Style:")
        col.operator("font.style_toggle").style = 'BOLD'
        col.operator("font.style_toggle").style = 'ITALIC'
        col.operator("font.style_toggle").style = 'UNDERLINE'

        col = layout.column(align=True)
        col.label(text="Repeat:")
        col.operator("screen.repeat_last")
        col.operator("screen.repeat_history", text="History...")


# ********** default tools for editmode_armature ****************


class VIEW3D_PT_tools_armatureedit(View3DPanel):
    bl_context = "armature_edit"
    bl_label = "Armature Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.label(text="Transform:")
        col.operator("tfm.translate")
        col.operator("tfm.rotate")
        col.operator("tfm.resize", text="Scale")

        col = layout.column(align=True)
        col.label(text="Bones:")
        col.operator("armature.bone_primitive_add", text="Add")
        col.operator("armature.duplicate_move", text="Duplicate")
        col.operator("armature.delete", text="Delete")

        col = layout.column(align=True)
        col.label(text="Modeling:")
        col.operator("armature.extrude_move")
        col.operator("armature.subdivide_multi", text="Subdivide")

        col = layout.column(align=True)
        col.label(text="Repeat:")
        col.operator("screen.repeat_last")
        col.operator("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.label(text="Grease Pencil:")
        row = col.row()
        row.operator("gpencil.draw", text="Draw").mode = 'DRAW'
        row.operator("gpencil.draw", text="Line").mode = 'DRAW_STRAIGHT'
        row.operator("gpencil.draw", text="Erase").mode = 'ERASER'


class VIEW3D_PT_tools_armatureedit_options(View3DPanel):
    bl_context = "armature_edit"
    bl_label = "Armature Options"

    def draw(self, context):
        layout = self.layout

        arm = context.active_object.data

        col = layout.column(align=True)
        col.prop(arm, "x_axis_mirror")

# ********** default tools for editmode_mball ****************


class VIEW3D_PT_tools_mballedit(View3DPanel):
    bl_context = "mball_edit"
    bl_label = "Meta Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.label(text="Transform:")
        col.operator("tfm.translate")
        col.operator("tfm.rotate")
        col.operator("tfm.resize", text="Scale")

        col = layout.column(align=True)
        col.label(text="Repeat:")
        col.operator("screen.repeat_last")
        col.operator("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.label(text="Grease Pencil:")
        row = col.row()
        row.operator("gpencil.draw", text="Draw").mode = 'DRAW'
        row.operator("gpencil.draw", text="Line").mode = 'DRAW_STRAIGHT'
        row.operator("gpencil.draw", text="Erase").mode = 'ERASER'

# ********** default tools for editmode_lattice ****************


class VIEW3D_PT_tools_latticeedit(View3DPanel):
    bl_context = "lattice_edit"
    bl_label = "Lattice Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.label(text="Transform:")
        col.operator("tfm.translate")
        col.operator("tfm.rotate")
        col.operator("tfm.resize", text="Scale")

        col = layout.column(align=True)
        col.operator("lattice.make_regular")

        col = layout.column(align=True)
        col.label(text="Repeat:")
        col.operator("screen.repeat_last")
        col.operator("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.label(text="Grease Pencil:")
        row = col.row()
        row.operator("gpencil.draw", text="Draw").mode = 'DRAW'
        row.operator("gpencil.draw", text="Line").mode = 'DRAW_STRAIGHT'
        row.operator("gpencil.draw", text="Erase").mode = 'ERASER'


# ********** default tools for posemode ****************


class VIEW3D_PT_tools_posemode(View3DPanel):
    bl_context = "posemode"
    bl_label = "Pose Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column(align=True)
        col.label(text="Transform:")
        col.operator("tfm.translate")
        col.operator("tfm.rotate")
        col.operator("tfm.resize", text="Scale")

        col = layout.column(align=True)
        col.label(text="In-Between:")
        row = col.row()
        row.operator("pose.push", text="Push")
        row.operator("pose.relax", text="Relax")
        col.operator("pose.breakdown", text="Breakdowner")

        col = layout.column(align=True)
        col.label(text="Pose:")
        row = col.row()
        row.operator("pose.copy", text="Copy")
        row.operator("pose.paste", text="Paste")

        col = layout.column(align=True)
        col.operator("poselib.pose_add", text="Add To Library")

        col = layout.column(align=True)
        col.label(text="Keyframes:")

        col.operator("anim.insert_keyframe_menu", text="Insert")
        col.operator("anim.delete_keyframe_v3d", text="Remove")

        col = layout.column(align=True)
        col.label(text="Repeat:")
        col.operator("screen.repeat_last")
        col.operator("screen.repeat_history", text="History...")

        col = layout.column(align=True)
        col.label(text="Grease Pencil:")
        row = col.row()
        row.operator("gpencil.draw", text="Draw").mode = 'DRAW'
        row.operator("gpencil.draw", text="Line").mode = 'DRAW_STRAIGHT'
        row.operator("gpencil.draw", text="Erase").mode = 'ERASER'


class VIEW3D_PT_tools_posemode_options(View3DPanel):
    bl_context = "posemode"
    bl_label = "Pose Options"

    def draw(self, context):
        layout = self.layout

        arm = context.active_object.data

        col = layout.column(align=True)
        col.prop(arm, "x_axis_mirror")
        col.prop(arm, "auto_ik")

# ********** default tools for paint modes ****************


class PaintPanel(bpy.types.Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'TOOLS'

    def paint_settings(self, context):
        ts = context.tool_settings

        if context.sculpt_object:
            return ts.sculpt
        elif context.vertex_paint_object:
            return ts.vertex_paint
        elif context.weight_paint_object:
            return ts.weight_paint
        elif context.texture_paint_object:
            return ts.image_paint
        elif context.particle_edit_object:
            return ts.particle_edit

        return False


class VIEW3D_PT_tools_brush(PaintPanel):
    bl_label = "Brush"

    def poll(self, context):
        return self.paint_settings(context)

    def draw(self, context):
        layout = self.layout

        settings = self.paint_settings(context)
        brush = settings.brush

        if not context.particle_edit_object:
            col = layout.split().column()
            row = col.row()
            row.template_list(settings, "brushes", settings, "active_brush_index", rows=2)

            col.template_ID(settings, "brush", new="brush.add")

        # Particle Mode #

        # XXX This needs a check if psys is editable.
        if context.particle_edit_object:
            # XXX Select Particle System
            layout.column().prop(settings, "tool", expand=True)

            if settings.tool != 'NONE':
                col = layout.column()
                col.prop(brush, "size", slider=True)
                col.prop(brush, "strength", slider=True)

            if settings.tool == 'ADD':
                col = layout.column()
                col.prop(settings, "add_interpolate")
                sub = col.column(align=True)
                sub.active = settings.add_interpolate
                sub.prop(brush, "steps", slider=True)
                sub.prop(settings, "add_keys", slider=True)
            elif settings.tool == 'LENGTH':
                layout.prop(brush, "length_mode", expand=True)
            elif settings.tool == 'PUFF':
                layout.prop(brush, "puff_mode", expand=True)

        # Sculpt Mode #

        elif context.sculpt_object and brush:
            col = layout.column()
            col.separator()
            col.prop(brush, "sculpt_tool", expand=True)
            col.separator()

            row = col.row(align=True)
            row.prop(brush, "size", slider=True)
            row.prop(brush, "use_size_pressure", toggle=True, text="")

            if brush.sculpt_tool != 'GRAB':
                row = col.row(align=True)
                row.prop(brush, "strength", slider=True)
                row.prop(brush, "use_strength_pressure", text="")

                # XXX - TODO
                #row = col.row(align=True)
                #row.prop(brush, "jitter", slider=True)
                #row.prop(brush, "use_jitter_pressure", toggle=True, text="")

                col = layout.column()

                if brush.sculpt_tool in ('DRAW', 'PINCH', 'INFLATE', 'LAYER', 'CLAY'):
                    col.row().prop(brush, "direction", expand=True)

                if brush.sculpt_tool in ('DRAW', 'INFLATE', 'LAYER'):
                    col.prop(brush, "use_accumulate")

                if brush.sculpt_tool == 'LAYER':
                    col.prop(brush, "use_persistent")
                    col.operator("sculpt.set_persistent_base")

        # Texture Paint Mode #

        elif context.texture_paint_object and brush:
            col = layout.column(align=True)
            col.prop_enum(settings, "tool", 'DRAW')
            col.prop_enum(settings, "tool", 'SOFTEN')
            col.prop_enum(settings, "tool", 'CLONE')
            col.prop_enum(settings, "tool", 'SMEAR')

            col = layout.column()
            col.prop(brush, "color", text="")

            row = col.row(align=True)
            row.prop(brush, "size", slider=True)
            row.prop(brush, "use_size_pressure", toggle=True, text="")

            row = col.row(align=True)
            row.prop(brush, "strength", slider=True)
            row.prop(brush, "use_strength_pressure", toggle=True, text="")

            row = col.row(align=True)
            row.prop(brush, "jitter", slider=True)
            row.prop(brush, "use_jitter_pressure", toggle=True, text="")

            col.prop(brush, "blend", text="Blend")

        # Weight Paint Mode #

        elif context.weight_paint_object and brush:
            layout.prop(context.tool_settings, "vertex_group_weight", text="Weight", slider=True)
            layout.prop(context.tool_settings, "auto_normalize", text="Auto Normalize")

            col = layout.column()
            row = col.row(align=True)
            row.prop(brush, "size", slider=True)
            row.prop(brush, "use_size_pressure", toggle=True, text="")

            row = col.row(align=True)
            row.prop(brush, "strength", slider=True)
            row.prop(brush, "use_strength_pressure", toggle=True, text="")

            row = col.row(align=True)
            row.prop(brush, "jitter", slider=True)
            row.prop(brush, "use_jitter_pressure", toggle=True, text="")

        # Vertex Paint Mode #

        elif context.vertex_paint_object and brush:
            col = layout.column()
            col.prop(brush, "color", text="")

            row = col.row(align=True)
            row.prop(brush, "size", slider=True)
            row.prop(brush, "use_size_pressure", toggle=True, text="")

            row = col.row(align=True)
            row.prop(brush, "strength", slider=True)
            row.prop(brush, "use_strength_pressure", toggle=True, text="")

            # XXX - TODO
            #row = col.row(align=True)
            #row.prop(brush, "jitter", slider=True)
            #row.prop(brush, "use_jitter_pressure", toggle=True, text="")


class VIEW3D_PT_tools_brush_stroke(PaintPanel):
    bl_label = "Stroke"
    bl_default_closed = True

    def poll(self, context):
        settings = self.paint_settings(context)
        return (settings and settings.brush and (context.sculpt_object or
                             context.vertex_paint_object or
                             context.weight_paint_object or
                             context.texture_paint_object))

    def draw(self, context):
        layout = self.layout

        settings = self.paint_settings(context)
        brush = settings.brush
        texture_paint = context.texture_paint_object

        if context.sculpt_object:
            if brush.sculpt_tool != 'LAYER':
                layout.prop(brush, "use_anchor")
            layout.prop(brush, "use_rake")

        layout.prop(brush, "use_airbrush")
        col = layout.column()
        col.active = brush.use_airbrush
        col.prop(brush, "rate", slider=True)

        if not texture_paint:
            layout.prop(brush, "use_smooth_stroke")
            col = layout.column()
            col.active = brush.use_smooth_stroke
            col.prop(brush, "smooth_stroke_radius", text="Radius", slider=True)
            col.prop(brush, "smooth_stroke_factor", text="Factor", slider=True)

        layout.prop(brush, "use_space")
        row = layout.row(align=True)
        row.active = brush.use_space
        row.prop(brush, "spacing", text="Distance", slider=True)
        if texture_paint:
            row.prop(brush, "use_spacing_pressure", toggle=True, text="")


class VIEW3D_PT_tools_brush_curve(PaintPanel):
    bl_label = "Curve"
    bl_default_closed = True

    def poll(self, context):
        settings = self.paint_settings(context)
        return (settings and settings.brush and settings.brush.curve)

    def draw(self, context):
        layout = self.layout

        settings = self.paint_settings(context)
        brush = settings.brush

        layout.template_curve_mapping(brush, "curve")
        layout.operator_menu_enum("brush.curve_preset", property="shape")


class VIEW3D_PT_sculpt_options(PaintPanel):
    bl_label = "Options"

    def poll(self, context):
        return context.sculpt_object

    def draw(self, context):
        layout = self.layout

        sculpt = context.tool_settings.sculpt

        col = layout.column()
        col.prop(sculpt, "show_brush")
        col.prop(sculpt, "fast_navigate")

        split = self.layout.split()

        col = split.column()
        col.label(text="Symmetry:")
        col.prop(sculpt, "symmetry_x", text="X")
        col.prop(sculpt, "symmetry_y", text="Y")
        col.prop(sculpt, "symmetry_z", text="Z")

        col = split.column()
        col.label(text="Lock:")
        col.prop(sculpt, "lock_x", text="X")
        col.prop(sculpt, "lock_y", text="Y")
        col.prop(sculpt, "lock_z", text="Z")

# ********** default tools for weightpaint ****************


class VIEW3D_PT_tools_weightpaint(View3DPanel):
    bl_context = "weightpaint"
    bl_label = "Weight Tools"

    def draw(self, context):
        layout = self.layout

        col = layout.column()
        # col.label(text="Blend:")
        col.operator("object.vertex_group_normalize_all", text="Normalize All")
        col.operator("object.vertex_group_normalize", text="Normalize")
        col.operator("object.vertex_group_invert", text="Invert")
        col.operator("object.vertex_group_clean", text="Clean")
        col.operator("object.vertex_group_levels", text="Levels")


class VIEW3D_PT_tools_weightpaint_options(View3DPanel):
    bl_context = "weightpaint"
    bl_label = "Options"

    def draw(self, context):
        layout = self.layout

        wpaint = context.tool_settings.weight_paint

        col = layout.column()
        col.label(text="Blend:")
        col.prop(wpaint, "mode", text="")
        col.prop(wpaint, "all_faces")
        col.prop(wpaint, "normals")
        col.prop(wpaint, "spray")

        data = context.weight_paint_object.data
        if type(data) == bpy.types.Mesh:
            col.prop(data, "use_mirror_x")

# Commented out because the Apply button isn't an operator yet, making these settings useless
#		col.label(text="Gamma:")
#		col.prop(wpaint, "gamma", text="")
#		col.label(text="Multiply:")
#		col.prop(wpaint, "mul", text="")

# Also missing now:
# Soft, Vgroup, X-Mirror and "Clear" Operator.

# ********** default tools for vertexpaint ****************


class VIEW3D_PT_tools_vertexpaint(View3DPanel):
    bl_context = "vertexpaint"
    bl_label = "Options"

    def draw(self, context):
        layout = self.layout

        vpaint = context.tool_settings.vertex_paint

        col = layout.column()
        col.label(text="Blend:")
        col.prop(vpaint, "mode", text="")
        col.prop(vpaint, "all_faces")
        col.prop(vpaint, "normals")
        col.prop(vpaint, "spray")

# Commented out because the Apply button isn't an operator yet, making these settings useless
#		col.label(text="Gamma:")
#		col.prop(vpaint, "gamma", text="")
#		col.label(text="Multiply:")
#		col.prop(vpaint, "mul", text="")

# ********** default tools for texturepaint ****************


class VIEW3D_PT_tools_projectpaint(View3DPanel):
    bl_context = "texturepaint"
    bl_label = "Project Paint"

    def poll(self, context):
        return context.tool_settings.image_paint.tool != 'SMEAR'

    def draw_header(self, context):
        ipaint = context.tool_settings.image_paint

        self.layout.prop(ipaint, "use_projection", text="")

    def draw(self, context):
        layout = self.layout

        ipaint = context.tool_settings.image_paint
        settings = context.tool_settings.image_paint
        use_projection = ipaint.use_projection

        col = layout.column()
        sub = col.column()
        sub.active = use_projection
        sub.prop(ipaint, "use_occlude")
        sub.prop(ipaint, "use_backface_cull")

        split = layout.split()

        col = split.column()
        col.active = (use_projection)
        col.prop(ipaint, "use_normal_falloff")

        col = split.column()
        col.active = (ipaint.use_normal_falloff and use_projection)
        col.prop(ipaint, "normal_angle", text="")

        split = layout.split(percentage=0.7)

        col = split.column(align=False)
        col.active = (use_projection)
        col.prop(ipaint, "use_stencil_layer")

        col = split.column(align=False)
        col.active = (use_projection and ipaint.use_stencil_layer)
        col.prop(ipaint, "invert_stencil", text="Inv")

        col = layout.column()
        sub = col.column()
        sub.active = (settings.tool == 'CLONE')
        sub.prop(ipaint, "use_clone_layer")

        sub = col.column()
        sub.prop(ipaint, "seam_bleed")


class VIEW3D_PT_tools_particlemode(View3DPanel):
    '''default tools for particle mode'''
    bl_context = "particlemode"
    bl_label = "Options"

    def draw(self, context):
        layout = self.layout

        pe = context.tool_settings.particle_edit
        ob = pe.object

        layout.prop(pe, "type", text="")

        ptcache = None

        if pe.type == 'PARTICLES':
            if ob.particle_systems:
                if len(ob.particle_systems) > 1:
                    layout.template_list(ob, "particle_systems", ob, "active_particle_system_index", type='ICONS')

                ptcache = ob.particle_systems[ob.active_particle_system_index].point_cache
        else:
            for md in ob.modifiers:
                if md.type == pe.type:
                    ptcache = md.point_cache

        if ptcache and len(ptcache.point_cache_list) > 1:
            layout.template_list(ptcache, "point_cache_list", ptcache, "active_point_cache_index", type='ICONS')


        if not pe.editable:
            layout.label(text="Point cache must be baked")
            layout.label(text="to enable editing!")

        col = layout.column(align=True)
        if pe.hair:
            col.active = pe.editable
            col.prop(pe, "emitter_deflect", text="Deflect emitter")
            sub = col.row()
            sub.active = pe.emitter_deflect
            sub.prop(pe, "emitter_distance", text="Distance")

        col = layout.column(align=True)
        col.active = pe.editable
        col.label(text="Keep:")
        col.prop(pe, "keep_lengths", text="Lenghts")
        col.prop(pe, "keep_root", text="Root")
        if not pe.hair:
            col.label(text="Correct:")
            col.prop(pe, "auto_velocity", text="Velocity")

        col = layout.column(align=True)
        col.active = pe.editable
        col.label(text="Draw:")
        col.prop(pe, "draw_step", text="Path Steps")
        if pe.type == 'PARTICLES':
            col.prop(pe, "draw_particles", text="Particles")
        col.prop(pe, "fade_time")
        sub = col.row()
        sub.active = pe.fade_time
        sub.prop(pe, "fade_frames", slider=True)


bpy.types.register(VIEW3D_PT_tools_weightpaint)
bpy.types.register(VIEW3D_PT_tools_objectmode)
bpy.types.register(VIEW3D_PT_tools_meshedit)
bpy.types.register(VIEW3D_PT_tools_meshedit_options)
bpy.types.register(VIEW3D_PT_tools_curveedit)
bpy.types.register(VIEW3D_PT_tools_surfaceedit)
bpy.types.register(VIEW3D_PT_tools_textedit)
bpy.types.register(VIEW3D_PT_tools_armatureedit)
bpy.types.register(VIEW3D_PT_tools_armatureedit_options)
bpy.types.register(VIEW3D_PT_tools_mballedit)
bpy.types.register(VIEW3D_PT_tools_latticeedit)
bpy.types.register(VIEW3D_PT_tools_posemode)
bpy.types.register(VIEW3D_PT_tools_posemode_options)
bpy.types.register(VIEW3D_PT_tools_brush)
bpy.types.register(VIEW3D_PT_tools_brush_stroke)
bpy.types.register(VIEW3D_PT_tools_brush_curve)
bpy.types.register(VIEW3D_PT_sculpt_options)
bpy.types.register(VIEW3D_PT_tools_vertexpaint)
bpy.types.register(VIEW3D_PT_tools_weightpaint_options)
bpy.types.register(VIEW3D_PT_tools_projectpaint)
bpy.types.register(VIEW3D_PT_tools_particlemode)
