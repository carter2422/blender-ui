/*
 * Copyright 2012, Blender Foundation.
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
 * Contributor:
 *		Dalai Felinto
 */

#ifndef _COM_ChannelMatteOperation_h
#define _COM_ChannelMatteOperation_h
#include "COM_MixBaseOperation.h"


/**
  * this program converts an input colour to an output value.
  * it assumes we are in sRGB colour space.
  */
class ChannelMatteOperation : public NodeOperation {
private:
	SocketReader *inputImageProgram;

	int color_space;   /* node->custom1 */
	int matte_channel; /* node->custom2 */
	int limit_method;  /* node->algorithm */
	int limit_channel; /* node->channel */
	float limit_max;     /* node->storage->t1 */
	float limit_min;     /* node->storage->t2 */

	float limit_range;

	/** ids to use for the operations (max and simple)
	  * alpha = in[ids[0]] - max(in[ids[1]], in[ids[2]])
	  * the simple operation is using:
	  * alpha = in[ids[0]] - in[ids[1]]
	  * but to use the same formula and operation for both we do:
		* ids[2] = ids[1]
	  * alpha = in[ids[0]] - max(in[ids[1]], in[ids[2]])
	  */
	int ids[3];
public:
	/**
	  * Default constructor
	  */
	ChannelMatteOperation();

	/**
	  * the inner loop of this program
	  */
	void executePixel(float* color, float x, float y, PixelSampler sampler, MemoryBuffer *inputBuffers[]);

	void initExecution();
	void deinitExecution();

	void setSettings(NodeChroma* nodeChroma, const int custom2)
	{
		this->limit_max = nodeChroma->t1;
		this->limit_min = nodeChroma->t2;
		this->limit_method = nodeChroma->algorithm;
		this->limit_channel = nodeChroma->channel;
		this->matte_channel = custom2;
	}
};
#endif
