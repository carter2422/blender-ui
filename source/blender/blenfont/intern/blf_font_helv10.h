/**
 * $Id:
 *
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2009 Blender Foundation.
 * All rights reserved.
 *
 * 
 * Contributor(s): Blender Foundation
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef BLF_FONT_HELV10_H
#define BLF_FONT_HELV10_H

static unsigned char helv10_bitmap_data[]= {
	0x80,0x00,0x80,0x80,0x80,0x80,0x80,0x80,
	0xa0,0xa0,0x50,0x50,0xf8,0x28,0x7c,0x28,
	0x28,0x20,0x70,0xa8,0x28,0x70,0xa0,0xa8,
	0x70,0x20,0x26,0x29,0x16,0x10,0x08,0x68,
	0x94,0x64,0x64,0x98,0x98,0xa4,0x60,0x50,
	0x50,0x20,0x80,0x40,0x40,0x20,0x40,0x40,
	0x80,0x80,0x80,0x80,0x40,0x40,0x20,0x80,
	0x40,0x40,0x20,0x20,0x20,0x20,0x40,0x40,
	0x80,0xa0,0x40,0xa0,0x20,0x20,0xf8,0x20,
	0x20,0x80,0x40,0x40,0xf8,0x80,0x80,0x80,
	0x40,0x40,0x40,0x40,0x20,0x20,0x70,0x88,
	0x88,0x88,0x88,0x88,0x88,0x70,0x40,0x40,
	0x40,0x40,0x40,0x40,0xc0,0x40,0xf8,0x80,
	0x40,0x30,0x08,0x08,0x88,0x70,0x70,0x88,
	0x08,0x08,0x30,0x08,0x88,0x70,0x10,0x10,
	0xf8,0x90,0x50,0x50,0x30,0x10,0x70,0x88,
	0x08,0x08,0xf0,0x80,0x80,0xf8,0x70,0x88,
	0x88,0xc8,0xb0,0x80,0x88,0x70,0x40,0x40,
	0x20,0x20,0x10,0x10,0x08,0xf8,0x70,0x88,
	0x88,0x88,0x70,0x88,0x88,0x70,0x70,0x88,
	0x08,0x68,0x98,0x88,0x88,0x70,0x80,0x00,
	0x00,0x00,0x00,0x80,0x80,0x40,0x40,0x00,
	0x00,0x00,0x00,0x40,0x20,0x40,0x80,0x40,
	0x20,0xf0,0x00,0xf0,0x80,0x40,0x20,0x40,
	0x80,0x40,0x00,0x40,0x40,0x20,0x10,0x90,
	0x60,0x3e,0x00,0x40,0x00,0x9b,0x00,0xa4,
	0x80,0xa4,0x80,0xa2,0x40,0x92,0x40,0x4d,
	0x40,0x20,0x80,0x1f,0x00,0x82,0x82,0x7c,
	0x44,0x28,0x28,0x10,0x10,0xf0,0x88,0x88,
	0x88,0xf0,0x88,0x88,0xf0,0x78,0x84,0x80,
	0x80,0x80,0x80,0x84,0x78,0xf0,0x88,0x84,
	0x84,0x84,0x84,0x88,0xf0,0xf8,0x80,0x80,
	0x80,0xf8,0x80,0x80,0xf8,0x80,0x80,0x80,
	0x80,0xf0,0x80,0x80,0xf8,0x74,0x8c,0x84,
	0x8c,0x80,0x80,0x84,0x78,0x84,0x84,0x84,
	0x84,0xfc,0x84,0x84,0x84,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x60,0x90,0x10,
	0x10,0x10,0x10,0x10,0x10,0x88,0x88,0x90,
	0x90,0xe0,0xa0,0x90,0x88,0xf0,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x92,0x92,0x92,
	0xaa,0xaa,0xc6,0xc6,0x82,0x8c,0x8c,0x94,
	0x94,0xa4,0xa4,0xc4,0xc4,0x78,0x84,0x84,
	0x84,0x84,0x84,0x84,0x78,0x80,0x80,0x80,
	0x80,0xf0,0x88,0x88,0xf0,0x02,0x7c,0x8c,
	0x94,0x84,0x84,0x84,0x84,0x78,0x88,0x88,
	0x88,0x88,0xf0,0x88,0x88,0xf0,0x70,0x88,
	0x88,0x08,0x70,0x80,0x88,0x70,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0xf8,0x78,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x10,0x28,
	0x28,0x44,0x44,0x44,0x82,0x82,0x22,0x00,
	0x22,0x00,0x22,0x00,0x55,0x00,0x49,0x00,
	0x49,0x00,0x88,0x80,0x88,0x80,0x88,0x88,
	0x50,0x50,0x20,0x50,0x88,0x88,0x10,0x10,
	0x10,0x28,0x28,0x44,0x44,0x82,0xf8,0x80,
	0x40,0x20,0x20,0x10,0x08,0xf8,0xc0,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xc0,
	0x20,0x20,0x40,0x40,0x40,0x40,0x80,0x80,
	0xc0,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0xc0,0x88,0x50,0x50,0x20,0x20,0xfc,
	0x80,0x80,0x40,0x68,0x90,0x90,0x70,0x10,
	0xe0,0xb0,0xc8,0x88,0x88,0xc8,0xb0,0x80,
	0x80,0x60,0x90,0x80,0x80,0x90,0x60,0x68,
	0x98,0x88,0x88,0x98,0x68,0x08,0x08,0x60,
	0x90,0x80,0xf0,0x90,0x60,0x40,0x40,0x40,
	0x40,0x40,0xe0,0x40,0x30,0x70,0x08,0x68,
	0x98,0x88,0x88,0x98,0x68,0x88,0x88,0x88,
	0x88,0xc8,0xb0,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x00,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x00,0x80,0x90,0x90,
	0xa0,0xc0,0xa0,0x90,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x92,0x92,
	0x92,0x92,0x92,0xec,0x88,0x88,0x88,0x88,
	0xc8,0xb0,0x70,0x88,0x88,0x88,0x88,0x70,
	0x80,0x80,0xb0,0xc8,0x88,0x88,0xc8,0xb0,
	0x08,0x08,0x68,0x98,0x88,0x88,0x98,0x68,
	0x80,0x80,0x80,0x80,0xc0,0xa0,0x60,0x90,
	0x10,0x60,0x90,0x60,0x60,0x40,0x40,0x40,
	0x40,0xe0,0x40,0x40,0x70,0x90,0x90,0x90,
	0x90,0x90,0x20,0x20,0x50,0x50,0x88,0x88,
	0x28,0x28,0x54,0x54,0x92,0x92,0x88,0x88,
	0x50,0x20,0x50,0x88,0x80,0x40,0x40,0x60,
	0xa0,0xa0,0x90,0x90,0xf0,0x80,0x40,0x20,
	0x10,0xf0,0x20,0x40,0x40,0x40,0x40,0x80,
	0x40,0x40,0x40,0x20,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x40,
	0x40,0x40,0x40,0x20,0x40,0x40,0x40,0x80,
	0x98,0x64,0x80,0x80,0x80,0x80,0x80,0x80,
	0x00,0x80,0x40,0x70,0xa8,0xa0,0xa0,0xa8,
	0x70,0x10,0xb0,0x48,0x40,0x40,0xe0,0x40,
	0x48,0x30,0x90,0x60,0x90,0x90,0x60,0x90,
	0x20,0xf8,0x20,0xf8,0x50,0x50,0x88,0x88,
	0x80,0x80,0x80,0x80,0x00,0x00,0x80,0x80,
	0x80,0x80,0x70,0x88,0x18,0x70,0xc8,0x98,
	0x70,0xc0,0x88,0x70,0xa0,0x38,0x44,0x9a,
	0xa2,0x9a,0x44,0x38,0xe0,0x00,0xa0,0x20,
	0xe0,0x28,0x50,0xa0,0x50,0x28,0x08,0x08,
	0xf8,0xe0,0x38,0x44,0xaa,0xb2,0xba,0x44,
	0x38,0xe0,0x60,0x90,0x90,0x60,0xf8,0x00,
	0x20,0x20,0xf8,0x20,0x20,0xe0,0x40,0xa0,
	0x60,0xc0,0x20,0x40,0xe0,0x80,0x40,0x80,
	0x80,0xf0,0x90,0x90,0x90,0x90,0x90,0x28,
	0x28,0x28,0x28,0x28,0x68,0xe8,0xe8,0xe8,
	0x7c,0xc0,0xc0,0x40,0x40,0x40,0xc0,0x40,
	0xe0,0x00,0xe0,0xa0,0xe0,0xa0,0x50,0x28,
	0x50,0xa0,0x21,0x00,0x17,0x80,0x13,0x00,
	0x09,0x00,0x48,0x00,0x44,0x00,0xc4,0x00,
	0x42,0x00,0x27,0x12,0x15,0x0b,0x48,0x44,
	0xc4,0x42,0x21,0x00,0x17,0x80,0x13,0x00,
	0x09,0x00,0xc8,0x00,0x24,0x00,0x44,0x00,
	0xe2,0x00,0x60,0x90,0x80,0x40,0x20,0x20,
	0x00,0x20,0x82,0x82,0x7c,0x44,0x28,0x28,
	0x10,0x10,0x00,0x10,0x20,0x82,0x82,0x7c,
	0x44,0x28,0x28,0x10,0x10,0x00,0x10,0x08,
	0x82,0x82,0x7c,0x44,0x28,0x28,0x10,0x10,
	0x00,0x28,0x10,0x82,0x82,0x7c,0x44,0x28,
	0x28,0x10,0x10,0x00,0x28,0x14,0x82,0x82,
	0x7c,0x44,0x28,0x28,0x10,0x10,0x00,0x28,
	0x82,0x82,0x7c,0x44,0x28,0x28,0x10,0x10,
	0x10,0x28,0x10,0x8f,0x80,0x88,0x00,0x78,
	0x00,0x48,0x00,0x2f,0x80,0x28,0x00,0x18,
	0x00,0x1f,0x80,0x30,0x10,0x78,0x84,0x80,
	0x80,0x80,0x80,0x84,0x78,0xf8,0x80,0x80,
	0x80,0xf8,0x80,0x80,0xf8,0x00,0x20,0x40,
	0xf8,0x80,0x80,0x80,0xf8,0x80,0x80,0xf8,
	0x00,0x20,0x10,0xf8,0x80,0x80,0xf8,0x80,
	0x80,0x80,0xf8,0x00,0x50,0x20,0xf8,0x80,
	0x80,0x80,0xf8,0x80,0x80,0xf8,0x00,0x50,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x00,0x40,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x00,0x80,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x00,0xa0,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x00,0xa0,0x78,0x44,0x42,0x42,0xf2,
	0x42,0x44,0x78,0x8c,0x8c,0x94,0x94,0xa4,
	0xa4,0xc4,0xc4,0x00,0x50,0x28,0x78,0x84,
	0x84,0x84,0x84,0x84,0x84,0x78,0x00,0x10,
	0x20,0x78,0x84,0x84,0x84,0x84,0x84,0x84,
	0x78,0x00,0x10,0x08,0x78,0x84,0x84,0x84,
	0x84,0x84,0x84,0x78,0x00,0x28,0x10,0x78,
	0x84,0x84,0x84,0x84,0x84,0x84,0x78,0x00,
	0x50,0x28,0x78,0x84,0x84,0x84,0x84,0x84,
	0x84,0x78,0x00,0x48,0x88,0x50,0x20,0x50,
	0x88,0x80,0x78,0xc4,0xa4,0xa4,0x94,0x94,
	0x8c,0x78,0x04,0x78,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x00,0x10,0x20,0x78,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x00,0x20,
	0x10,0x78,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x00,0x28,0x10,0x78,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x00,0x48,0x10,0x10,
	0x10,0x28,0x28,0x44,0x44,0x82,0x00,0x10,
	0x08,0x80,0x80,0xf0,0x88,0x88,0xf0,0x80,
	0x80,0xa0,0x90,0x90,0x90,0xa0,0x90,0x90,
	0x60,0x68,0x90,0x90,0x70,0x10,0xe0,0x00,
	0x20,0x40,0x68,0x90,0x90,0x70,0x10,0xe0,
	0x00,0x20,0x10,0x68,0x90,0x90,0x70,0x10,
	0xe0,0x00,0x50,0x20,0x68,0x90,0x90,0x70,
	0x10,0xe0,0x00,0xa0,0x50,0x68,0x90,0x90,
	0x70,0x10,0xe0,0x00,0x50,0x68,0x90,0x90,
	0x70,0x10,0xe0,0x20,0x50,0x20,0x6c,0x92,
	0x90,0x7e,0x12,0xec,0x60,0x20,0x60,0x90,
	0x80,0x80,0x90,0x60,0x60,0x90,0x80,0xf0,
	0x90,0x60,0x00,0x20,0x40,0x60,0x90,0x80,
	0xf0,0x90,0x60,0x00,0x40,0x20,0x60,0x90,
	0x80,0xf0,0x90,0x60,0x00,0x50,0x20,0x60,
	0x90,0x80,0xf0,0x90,0x60,0x00,0x50,0x40,
	0x40,0x40,0x40,0x40,0x40,0x00,0x40,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x80,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00,
	0xa0,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x00,0xa0,0x70,0x88,0x88,0x88,0x88,0x78,
	0x90,0x60,0x50,0x90,0x90,0x90,0x90,0x90,
	0xe0,0x00,0xa0,0x50,0x70,0x88,0x88,0x88,
	0x88,0x70,0x00,0x20,0x40,0x70,0x88,0x88,
	0x88,0x88,0x70,0x00,0x20,0x10,0x70,0x88,
	0x88,0x88,0x88,0x70,0x00,0x50,0x20,0x70,
	0x88,0x88,0x88,0x88,0x70,0x00,0x50,0x28,
	0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x50,
	0x20,0x00,0xf8,0x00,0x20,0x70,0x88,0xc8,
	0xa8,0x98,0x74,0x70,0x90,0x90,0x90,0x90,
	0x90,0x00,0x20,0x40,0x70,0x90,0x90,0x90,
	0x90,0x90,0x00,0x40,0x20,0x70,0x90,0x90,
	0x90,0x90,0x90,0x00,0x50,0x20,0x70,0x90,
	0x90,0x90,0x90,0x90,0x00,0x50,0x80,0x40,
	0x40,0x60,0xa0,0xa0,0x90,0x90,0x00,0x20,
	0x10,0x80,0x80,0xb0,0xc8,0x88,0x88,0xc8,
	0xb0,0x80,0x80,0x80,0x40,0x40,0x60,0xa0,
	0xa0,0x90,0x90,0x00,0x50,
};

FontDataBLF blf_font_helv10 = {
	-1, -2,
	10, 11,
	{
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0, 0, 0, 0, 12, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0, 0, 0, 0, 3, -1},
		{1, 8, -1, 0, 3, 0},
		{3, 2, -1, -6, 4, 8},
		{6, 7, 0, 0, 6, 10},
		{5, 9, 0, 1, 6, 17},
		{8, 8, 0, 0, 9, 26},
		{6, 8, -1, 0, 8, 34},
		{2, 3, -1, -5, 3, 42},
		{3, 10, 0, 2, 4, 45},
		{3, 10, -1, 2, 4, 55},
		{3, 3, 0, -5, 4, 65},
		{5, 5, 0, -1, 6, 68},
		{2, 3, 0, 2, 3, 73},
		{5, 1, -1, -3, 7, 76},
		{1, 1, -1, 0, 3, 77},
		{3, 8, 0, 0, 3, 78},
		{5, 8, 0, 0, 6, 86},
		{2, 8, -1, 0, 6, 94},
		{5, 8, 0, 0, 6, 102},
		{5, 8, 0, 0, 6, 110},
		{5, 8, 0, 0, 6, 118},
		{5, 8, 0, 0, 6, 126},
		{5, 8, 0, 0, 6, 134},
		{5, 8, 0, 0, 6, 142},
		{5, 8, 0, 0, 6, 150},
		{5, 8, 0, 0, 6, 158},
		{1, 6, -1, 0, 3, 166},
		{2, 8, 0, 2, 3, 172},
		{3, 5, -1, -1, 6, 180},
		{4, 3, 0, -2, 5, 185},
		{3, 5, -1, -1, 6, 188},
		{4, 8, -1, 0, 6, 193},
		{10, 10, 0, 2, 11, 201},
		{7, 8, 0, 0, 7, 221},
		{5, 8, -1, 0, 7, 229},
		{6, 8, -1, 0, 8, 237},
		{6, 8, -1, 0, 8, 245},
		{5, 8, -1, 0, 7, 253},
		{5, 8, -1, 0, 6, 261},
		{6, 8, -1, 0, 8, 269},
		{6, 8, -1, 0, 8, 277},
		{1, 8, -1, 0, 3, 285},
		{4, 8, 0, 0, 5, 293},
		{5, 8, -1, 0, 7, 301},
		{4, 8, -1, 0, 6, 309},
		{7, 8, -1, 0, 9, 317},
		{6, 8, -1, 0, 8, 325},
		{6, 8, -1, 0, 8, 333},
		{5, 8, -1, 0, 7, 341},
		{7, 9, -1, 1, 8, 349},
		{5, 8, -1, 0, 7, 358},
		{5, 8, -1, 0, 7, 366},
		{5, 8, 0, 0, 5, 374},
		{6, 8, -1, 0, 8, 382},
		{7, 8, 0, 0, 7, 390},
		{9, 8, 0, 0, 9, 398},
		{5, 8, -1, 0, 7, 414},
		{7, 8, 0, 0, 7, 422},
		{5, 8, -1, 0, 7, 430},
		{2, 10, -1, 2, 3, 438},
		{3, 8, 0, 0, 3, 448},
		{2, 10, 0, 2, 3, 456},
		{5, 5, 0, -3, 6, 466},
		{6, 1, 0, 2, 6, 471},
		{2, 3, 0, -5, 3, 472},
		{5, 6, 0, 0, 5, 475},
		{5, 8, 0, 0, 6, 481},
		{4, 6, 0, 0, 5, 489},
		{5, 8, 0, 0, 6, 495},
		{4, 6, 0, 0, 5, 503},
		{4, 8, 0, 0, 4, 509},
		{5, 8, 0, 2, 6, 517},
		{5, 8, 0, 0, 6, 525},
		{1, 8, 0, 0, 2, 533},
		{1, 9, 0, 1, 2, 541},
		{4, 8, 0, 0, 5, 550},
		{1, 8, 0, 0, 2, 558},
		{7, 6, 0, 0, 8, 566},
		{5, 6, 0, 0, 6, 572},
		{5, 6, 0, 0, 6, 578},
		{5, 8, 0, 2, 6, 584},
		{5, 8, 0, 2, 6, 592},
		{3, 6, 0, 0, 4, 600},
		{4, 6, 0, 0, 5, 606},
		{3, 8, 0, 0, 4, 612},
		{4, 6, 0, 0, 5, 620},
		{5, 6, 0, 0, 6, 626},
		{7, 6, 0, 0, 8, 632},
		{5, 6, 0, 0, 6, 638},
		{4, 8, 0, 2, 5, 644},
		{4, 6, 0, 0, 5, 652},
		{3, 10, 0, 2, 3, 658},
		{1, 10, -1, 2, 3, 668},
		{3, 10, 0, 2, 3, 678},
		{6, 2, 0, -3, 7, 688},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0,0,0,0,0, -1},
		{0, 0, 0, 0, 3, -1},
		{1, 8, -1, 2, 3, 690},
		{5, 8, 0, 1, 6, 698},
		{5, 8, 0, 0, 6, 706},
		{4, 6, 0, -1, 5, 714},
		{5, 8, 0, 0, 6, 720},
		{1, 10, -1, 2, 3, 728},
		{5, 10, 0, 2, 6, 738},
		{3, 1, 0, -7, 3, 748},
		{7, 7, -1, 0, 9, 749},
		{3, 5, 0, -3, 4, 756},
		{5, 5, 0, 0, 6, 761},
		{5, 3, -1, -2, 7, 766},
		{3, 1, 0, -3, 4, 769},
		{7, 7, -1, 0, 9, 770},
		{3, 1, 0, -7, 3, 777},
		{4, 4, 0, -3, 4, 778},
		{5, 7, 0, 0, 6, 782},
		{3, 4, 0, -3, 3, 789},
		{3, 4, 0, -3, 3, 793},
		{2, 2, 0, -6, 3, 797},
		{4, 8, 0, 2, 5, 799},
		{6, 10, 0, 2, 6, 807},
		{2, 1, 0, -3, 3, 817},
		{2, 2, 0, 2, 3, 818},
		{2, 4, 0, -3, 3, 820},
		{3, 5, 0, -3, 4, 824},
		{5, 5, 0, 0, 6, 829},
		{9, 8, 0, 0, 9, 834},
		{8, 8, 0, 0, 9, 850},
		{9, 8, 0, 0, 9, 858},
		{4, 8, -1, 2, 6, 874},
		{7, 11, 0, 0, 7, 882},
		{7, 11, 0, 0, 7, 893},
		{7, 11, 0, 0, 7, 904},
		{7, 11, 0, 0, 7, 915},
		{7, 10, 0, 0, 7, 926},
		{7, 11, 0, 0, 7, 936},
		{9, 8, 0, 0, 10, 947},
		{6, 10, -1, 2, 8, 963},
		{5, 11, -1, 0, 7, 973},
		{5, 11, -1, 0, 7, 984},
		{5, 11, -1, 0, 7, 995},
		{5, 10, -1, 0, 7, 1006},
		{2, 11, 0, 0, 3, 1016},
		{2, 11, -1, 0, 3, 1027},
		{3, 11, 0, 0, 3, 1038},
		{3, 10, 0, 0, 3, 1049},
		{7, 8, 0, 0, 8, 1059},
		{6, 11, -1, 0, 8, 1067},
		{6, 11, -1, 0, 8, 1078},
		{6, 11, -1, 0, 8, 1089},
		{6, 11, -1, 0, 8, 1100},
		{6, 11, -1, 0, 8, 1111},
		{6, 10, -1, 0, 8, 1122},
		{5, 5, 0, -1, 6, 1132},
		{6, 10, -1, 1, 8, 1137},
		{6, 11, -1, 0, 8, 1147},
		{6, 11, -1, 0, 8, 1158},
		{6, 11, -1, 0, 8, 1169},
		{6, 10, -1, 0, 8, 1180},
		{7, 11, 0, 0, 7, 1190},
		{5, 8, -1, 0, 7, 1201},
		{4, 8, 0, 0, 5, 1209},
		{5, 9, 0, 0, 5, 1217},
		{5, 9, 0, 0, 5, 1226},
		{5, 9, 0, 0, 5, 1235},
		{5, 9, 0, 0, 5, 1244},
		{5, 8, 0, 0, 5, 1253},
		{5, 9, 0, 0, 5, 1261},
		{7, 6, 0, 0, 8, 1270},
		{4, 8, 0, 2, 5, 1276},
		{4, 9, 0, 0, 5, 1284},
		{4, 9, 0, 0, 5, 1293},
		{4, 9, 0, 0, 5, 1302},
		{4, 8, 0, 0, 5, 1311},
		{2, 9, 1, 0, 2, 1319},
		{2, 9, 0, 0, 2, 1328},
		{3, 9, 1, 0, 2, 1337},
		{3, 8, 0, 0, 2, 1346},
		{5, 9, 0, 0, 6, 1354},
		{4, 9, 0, 0, 5, 1363},
		{5, 9, 0, 0, 6, 1372},
		{5, 9, 0, 0, 6, 1381},
		{5, 9, 0, 0, 6, 1390},
		{5, 9, 0, 0, 6, 1399},
		{5, 8, 0, 0, 6, 1408},
		{5, 5, 0, -1, 6, 1416},
		{6, 6, 0, 0, 6, 1421},
		{4, 9, 0, 0, 5, 1427},
		{4, 9, 0, 0, 5, 1436},
		{4, 9, 0, 0, 5, 1445},
		{4, 8, 0, 0, 5, 1454},
		{4, 11, 0, 2, 5, 1462},
		{5, 10, 0, 2, 6, 1473},
		{4, 10, 0, 2, 5, 1483},
	},
	helv10_bitmap_data,
	0
};

#endif /* BLF_FONT_HELV10_H */
