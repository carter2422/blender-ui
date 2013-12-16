/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor(s): Jonathan Smith
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/editors/util/numinput.c
 *  \ingroup edutil
 */


#include "BLI_utildefines.h"
#include "BLI_math.h"
#include "BLI_string.h"

#include "BKE_context.h"
#include "BKE_unit.h"

#include "DNA_scene_types.h"

#include "WM_types.h"

#ifdef WITH_PYTHON
#include "BPY_extern.h"
#endif

#include "ED_numinput.h"

/* ************************** Functions *************************** */

/* ************************** NUMINPUT **************************** */

void initNumInput(NumInput *n)
{
	n->unit_sys = USER_UNIT_NONE;
	n->unit_type[0] = n->unit_type[1] = n->unit_type[2] = B_UNIT_NONE;
	n->idx = 0;
	n->idx_max = 0;
	n->flag = 0;
	n->edited[0] = n->edited[1] = n->edited[2] = false;
	zero_v3(n->org_val);
	zero_v3(n->val);
	n->str[0] = '\0';
	n->str_cur = -1;
	n->increment = 1.0f;
}

/* str must be NUM_STR_REP_LEN * (idx_max + 1) length. */
void outputNumInput(NumInput *n, char *str)
{
	short i, j;
	const int ln = NUM_STR_REP_LEN;

	for (j = 0; j <= n->idx_max; j++) {
		/* if AFFECTALL and no number typed and cursor not on number, use first number */
		i = (n->flag & NUM_AFFECT_ALL && n->idx != j && !n->edited[j]) ? 0 : j;

		if (n->edited[i]) {
			if (i == n->idx && n->str[0]) {
				char before_cursor[NUM_STR_REP_LEN];
				BLI_strncpy(before_cursor, n->str, n->str_cur + 2);  /* +2 because of trailing '\0' */
				BLI_snprintf(&str[j * ln], ln, "%s|%s", before_cursor, &n->str[n->str_cur + 1]);
			}
			else {
				const char *cur = (i == n->idx) ? "|" : "";
				if (n->unit_use_radians && n->unit_type[i] == B_UNIT_ROTATION) {
					/* Radian exception... */
					BLI_snprintf(&str[j * ln], ln, "%s%.6gr%s", cur, n->val[i], cur);
				}
				else {
					const int prec = 4; /* draw-only, and avoids too much issues with radian->degrees conversion. */
					char tstr[NUM_STR_REP_LEN];
					bUnit_AsString(tstr, ln, (double)n->val[i], prec, n->unit_sys, n->unit_type[i], true, false);
					BLI_snprintf(&str[j * ln], ln, "%s%s%s", cur, tstr, cur);
				}
			}
		}
		else {
			const char *cur = (i == n->idx) ? "|" : "";
			BLI_snprintf(&str[j * ln], ln, "%sNONE%s", cur, cur);
		}
	}
}

bool hasNumInput(const NumInput *n)
{
	short i;

	for (i = 0; i <= n->idx_max; i++) {
		if (n->edited[i])
			return true;
	}

	return false;
}

/**
 * \warning \a vec must be set beforehand otherwise we risk uninitialized vars.
 */
void applyNumInput(NumInput *n, float *vec)
{
	short i, j;
	float val;

	if (hasNumInput(n)) {
		for (j = 0; j <= n->idx_max; j++) {
			/* if AFFECTALL and no number typed and cursor not on number, use first number */
			i = (n->flag & NUM_AFFECT_ALL && n->idx != j && !n->edited[j]) ? 0 : j;
			val = (!n->edited[i] && n->flag & NUM_NULL_ONE) ? 1.0f : n->val[i];

			if (n->flag & NUM_NO_NEGATIVE && val < 0.0f) {
				val = 0.0f;
			}
			if (n->flag & NUM_NO_ZERO && val == 0.0f) {
				val = 0.0001f;
			}
			if (n->flag & NUM_NO_FRACTION && val != floorf(val)) {
				val = (float)round((double)val);
				if (n->flag & NUM_NO_ZERO && val == 0.0f) {
					val = 1.0f;
				}
			}
			vec[j] = val;
		}
	}
}

bool handleNumInput(bContext *C, NumInput *n, const wmEvent *event)
{
	char chr = '\0';
	bool updated = false;
	short idx = n->idx, idx_max = n->idx_max;
	short dir = 1, cur;
	double val;

	switch (event->type) {
		case EVT_MODAL_MAP:
			if (ELEM(event->val, NUM_MODAL_INCREMENT_UP, NUM_MODAL_INCREMENT_DOWN)) {
				n->val[idx] += (event->val == NUM_MODAL_INCREMENT_UP) ? n->increment : -n->increment;
				if (n->str_cur >= 0) {
					n->str[0] = '\0';
					n->str_cur = -1;
				}
				else {
					n->edited[idx] = true;
				}
				updated = true;
			}
			else {
				/* might be a char too... */
				chr = event->ascii;
			}
			break;
		case BACKSPACEKEY:
			cur = n->str_cur;
			if (!n->edited[idx]) {
				copy_v3_v3(n->val, n->org_val);
				n->edited[0] = n->edited[1] = n->edited[2] = false;
			}
			else if (event->shift || !n->str[0]) {
				n->val[idx] = n->org_val[idx];
				n->edited[idx] = false;
				n->str[0] = '\0';
				n->str_cur = -1;
			}
			else if (cur > -1) {
				if (cur + 1 < NUM_STR_REP_LEN && n->str[cur + 1]) {
					char tstr[NUM_STR_REP_LEN];
					BLI_strncpy(tstr, &n->str[cur + 1], NUM_STR_REP_LEN);
					BLI_strncpy(&n->str[cur], tstr, NUM_STR_REP_LEN - cur);
				}
				else {
					n->str[cur] = '\0';
				}
				n->str_cur--;
			}
			else {
				return false;
			}
			updated = true;
			break;
		case LEFTARROWKEY:
			dir = -1;
			/* fall-through */
		case RIGHTARROWKEY:
			cur = n->str_cur + dir;
			if (cur >= NUM_STR_REP_LEN || cur < -1 || (cur >= 0 && !n->str[cur]))
				return false;
			n->str_cur = cur;
			return true;
		case TABKEY:
			n->org_val[idx] = n->val[idx];

			idx += event->ctrl ? -1 : 1;
			if (idx > idx_max)
				idx = 0;
			else if (idx < 0)
				idx = idx_max;
			n->idx = idx;
			n->str[0] = '\0';
			n->str_cur = -1;
			n->val[idx] = n->org_val[idx];
			return true;
		default:
			chr = event->ascii;
			break;
	}

	if (chr) {
		cur = n->str_cur + 1;
		if (cur >= NUM_STR_REP_LEN)
			return false;

		if (n->str[cur] && (cur + 1 < NUM_STR_REP_LEN)) {
			char tstr[NUM_STR_REP_LEN];
			BLI_strncpy(tstr, &n->str[cur], NUM_STR_REP_LEN);
			n->str[cur] = chr;
			BLI_strncpy(&n->str[cur + 1], tstr, NUM_STR_REP_LEN - (cur + 1));
		}
		else {
			n->str[cur] = chr;
			n->str[cur + 1] = '\0';
		}
		n->str_cur++;
		n->edited[idx] = true;
	}
	else if (!updated) {
		return false;
	}

	/* At this point, our value has changed, try to interpret it with python (if str is not empty!). */
	if (n->str[0]) {
#ifdef WITH_PYTHON
		char str_unit_convert[NUM_STR_REP_LEN * 6];  /* Should be more than enough! */
		const char *default_unit = NULL;
		char *c;

		/* Make radian default unit when needed. */
		if (n->unit_use_radians && n->unit_type[idx] == B_UNIT_ROTATION)
			default_unit = "r";

		BLI_strncpy(str_unit_convert, n->str, sizeof(str_unit_convert));

		bUnit_ReplaceString(str_unit_convert, sizeof(str_unit_convert), default_unit, 1.0,
		                    n->unit_sys, n->unit_type[idx]);

		/* Note: with angles, we always get values as radians here... */
		if (BPY_button_exec(C, str_unit_convert, &val, false) != -1) {
			n->val[idx] = (float)val;
		}
#else  /* Very unlikely, but does not harm... */
		n->val[idx] = (float)atof(n->str);
#endif  /* WITH_PYTHON */
	}

	/* REDRAW SINCE NUMBERS HAVE CHANGED */
	return true;
}
