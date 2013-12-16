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
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file ED_numinput.h
 *  \ingroup editors
 */

#ifndef __ED_NUMINPUT_H__
#define __ED_NUMINPUT_H__

#define NUM_STR_REP_LEN 20

typedef struct NumInput {
	int    unit_sys;
	int    unit_type[3];              /* Each value can have a different type */
	bool   unit_use_radians;
	short  idx;
	short  idx_max;
	short  flag;                      /* Different flags to indicate different behaviors */
	short  edited[3];                 /* Relevant value has been edited */
	float  org_val[3];                /* Original value of the input, for reset */
	float  val[3];                    /* Direct value of the input */
	char   str[NUM_STR_REP_LEN];      /* String as typed by user for edited value (we assume we are in ASCII world!) */
	short  str_cur;                   /* Current position of cursor in edited value str */
	float  increment;
} NumInput;

/* NumInput.flag */
enum {
	NUM_NULL_ONE        = (1 << 1),
	NUM_NO_NEGATIVE     = (1 << 2),
	NUM_NO_ZERO         = (1 << 3),
	NUM_NO_FRACTION     = (1 << 4),
	NUM_AFFECT_ALL      = (1 << 5),
};

/*********************** NumInput ********************************/

void initNumInput(NumInput *n);
void outputNumInput(NumInput *n, char *str);
bool hasNumInput(const NumInput *n);
void applyNumInput(NumInput *n, float *vec);
bool handleNumInput(struct bContext *C, NumInput *n, const struct wmEvent *event);

#define NUM_MODAL_INCREMENT_UP   18
#define NUM_MODAL_INCREMENT_DOWN 19

#endif  /* __ED_NUMINPUT_H__ */
