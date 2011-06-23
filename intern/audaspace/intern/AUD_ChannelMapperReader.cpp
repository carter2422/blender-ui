/*
 * $Id$
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * Copyright 2009-2011 Jörg Hermann Müller
 *
 * This file is part of AudaSpace.
 *
 * Audaspace is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * AudaSpace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Audaspace; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file audaspace/intern/AUD_ChannelMapperReader.cpp
 *  \ingroup audaspaceintern
 */

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#include "AUD_ChannelMapperReader.h"

AUD_ChannelMapperReader::AUD_ChannelMapperReader(AUD_Reference<AUD_IReader> reader,
												 AUD_Channels channels) :
		AUD_EffectReader(reader), m_target_channels(channels),
	m_source_channels(AUD_CHANNELS_INVALID), m_mapping(0)
{
}

AUD_ChannelMapperReader::~AUD_ChannelMapperReader()
{
	delete[] m_mapping;
}

void AUD_ChannelMapperReader::setChannels(AUD_Channels channels)
{
	m_target_channels = channels;
	calculateMapping();
}

float AUD_ChannelMapperReader::angleDistance(float alpha, float beta)
{
	alpha = fabs(alpha - beta);

	if(alpha > M_PI)
		alpha = fabs(alpha - 2 * M_PI);

	return alpha;
}

void AUD_ChannelMapperReader::calculateMapping()
{
	delete[] m_mapping;
	m_mapping = new float[m_source_channels * m_target_channels];

	const AUD_Channel* source_channels = CHANNEL_MAPS[m_source_channels - 1];
	const AUD_Channel* target_channels = CHANNEL_MAPS[m_target_channels - 1];

	int lfe = -1;

	for(int i = 0; i < m_target_channels; i++)
	{
		if(target_channels[i] == AUD_CHANNEL_LFE)
		{
			lfe = i;
			break;
		}
	}

	const float* source_angles = CHANNEL_ANGLES[m_source_channels - 1];
	const float* target_angles = CHANNEL_ANGLES[m_target_channels - 1];

	int channel_min1, channel_min2;
	float angle_min1, angle_min2, angle;

	for(int i = 0; i < m_source_channels; i++)
	{
		if(source_channels[i] == AUD_CHANNEL_LFE)
		{
			if(lfe != -1)
				m_mapping[lfe * m_source_channels + i] = 1;

			continue;
		}

		channel_min1 = channel_min2 = -1;
		angle_min1 = angle_min2 = 2 * M_PI;

		for(int j = 0; j < m_target_channels; j++)
		{
			angle = angleDistance(source_angles[i], target_angles[j]);
			if(angle < angle_min1)
			{
				channel_min2 = channel_min1;
				angle_min2 = angle_min1;

				channel_min1 = j;
				angle_min1 = angle;
			}
			else if(angle < angle_min2)
			{
				channel_min2 = j;
				angle_min2 = angle;
			}
		}

		if(channel_min2 == -1)
		{
			m_mapping[channel_min1 * m_source_channels + i] = 1;
		}
		else
		{
			angle = angle_min1 + angle_min2;
			m_mapping[channel_min1 * m_source_channels + i] = cos(M_PI_2 * angle_min1 / angle);
			m_mapping[channel_min2 * m_source_channels + i] = cos(M_PI_2 * angle_min2 / angle);
		}
	}
}

AUD_Specs AUD_ChannelMapperReader::getSpecs() const
{
	AUD_Specs specs = m_reader->getSpecs();
	specs.channels = m_target_channels;
	return specs;
}

void AUD_ChannelMapperReader::read(int& length, bool& eos, sample_t* buffer)
{
	AUD_Channels channels = m_reader->getSpecs().channels;
	if(channels != m_source_channels)
	{
		m_source_channels = channels;
		calculateMapping();
	}

	if(m_source_channels == m_target_channels)
	{
		m_reader->read(length, eos, buffer);
		return;
	}

	m_buffer.assureSize(length * channels * sizeof(sample_t));

	sample_t* in = m_buffer.getBuffer();

	m_reader->read(length, eos, in);

	sample_t sum;

	for(int i = 0; i < length; i++)
	{
		for(int j = 0; j < m_target_channels; j++)
		{
			sum = 0;
			for(int k = 0; k < m_source_channels; k++)
				sum += m_mapping[j * m_source_channels + k] * in[i * m_source_channels + k];
			buffer[i * m_target_channels + j] = sum;
		}
	}
}

const AUD_Channel AUD_ChannelMapperReader::MONO_MAP[] =
{
	AUD_CHANNEL_FRONT_CENTER
};

const AUD_Channel AUD_ChannelMapperReader::STEREO_MAP[] =
{
	AUD_CHANNEL_FRONT_LEFT,
	AUD_CHANNEL_FRONT_RIGHT
};

const AUD_Channel AUD_ChannelMapperReader::STEREO_LFE_MAP[] =
{
	AUD_CHANNEL_FRONT_LEFT,
	AUD_CHANNEL_FRONT_RIGHT,
	AUD_CHANNEL_LFE
};

const AUD_Channel AUD_ChannelMapperReader::SURROUND4_MAP[] =
{
	AUD_CHANNEL_FRONT_LEFT,
	AUD_CHANNEL_FRONT_RIGHT,
	AUD_CHANNEL_REAR_LEFT,
	AUD_CHANNEL_REAR_RIGHT
};

const AUD_Channel AUD_ChannelMapperReader::SURROUND5_MAP[] =
{
	AUD_CHANNEL_FRONT_LEFT,
	AUD_CHANNEL_FRONT_RIGHT,
	AUD_CHANNEL_FRONT_CENTER,
	AUD_CHANNEL_REAR_LEFT,
	AUD_CHANNEL_REAR_RIGHT
};

const AUD_Channel AUD_ChannelMapperReader::SURROUND51_MAP[] =
{
	AUD_CHANNEL_FRONT_LEFT,
	AUD_CHANNEL_FRONT_RIGHT,
	AUD_CHANNEL_FRONT_CENTER,
	AUD_CHANNEL_LFE,
	AUD_CHANNEL_REAR_LEFT,
	AUD_CHANNEL_REAR_RIGHT
};

const AUD_Channel AUD_ChannelMapperReader::SURROUND61_MAP[] =
{
	AUD_CHANNEL_FRONT_LEFT,
	AUD_CHANNEL_FRONT_RIGHT,
	AUD_CHANNEL_FRONT_CENTER,
	AUD_CHANNEL_LFE,
	AUD_CHANNEL_REAR_CENTER,
	AUD_CHANNEL_REAR_LEFT,
	AUD_CHANNEL_REAR_RIGHT
};

const AUD_Channel AUD_ChannelMapperReader::SURROUND71_MAP[] =
{
	AUD_CHANNEL_FRONT_LEFT,
	AUD_CHANNEL_FRONT_RIGHT,
	AUD_CHANNEL_FRONT_CENTER,
	AUD_CHANNEL_LFE,
	AUD_CHANNEL_REAR_LEFT,
	AUD_CHANNEL_REAR_RIGHT,
	AUD_CHANNEL_SIDE_LEFT,
	AUD_CHANNEL_SIDE_RIGHT
};

const AUD_Channel* AUD_ChannelMapperReader::CHANNEL_MAPS[] =
{
	AUD_ChannelMapperReader::MONO_MAP,
	AUD_ChannelMapperReader::STEREO_MAP,
	AUD_ChannelMapperReader::STEREO_LFE_MAP,
	AUD_ChannelMapperReader::SURROUND4_MAP,
	AUD_ChannelMapperReader::SURROUND5_MAP,
	AUD_ChannelMapperReader::SURROUND51_MAP,
	AUD_ChannelMapperReader::SURROUND61_MAP,
	AUD_ChannelMapperReader::SURROUND71_MAP
};

const float AUD_ChannelMapperReader::MONO_ANGLES[] =
{
	0.0f * M_PI / 180.0f
};

const float AUD_ChannelMapperReader::STEREO_ANGLES[] =
{
	-90.0f * M_PI / 180.0f,
	 90.0f * M_PI / 180.0f
};

const float AUD_ChannelMapperReader::STEREO_LFE_ANGLES[] =
{
   -90.0f * M_PI / 180.0f,
	90.0f * M_PI / 180.0f,
	 0.0f * M_PI / 180.0f
};

const float AUD_ChannelMapperReader::SURROUND4_ANGLES[] =
{
	 -45.0f * M_PI / 180.0f,
	  45.0f * M_PI / 180.0f,
	-135.0f * M_PI / 180.0f,
	 135.0f * M_PI / 180.0f
};

const float AUD_ChannelMapperReader::SURROUND5_ANGLES[] =
{
	 -30.0f * M_PI / 180.0f,
	  30.0f * M_PI / 180.0f,
	   0.0f * M_PI / 180.0f,
	-110.0f * M_PI / 180.0f,
	 110.0f * M_PI / 180.0f
};

const float AUD_ChannelMapperReader::SURROUND51_ANGLES[] =
{
	  -30.0f * M_PI / 180.0f,
	   30.0f * M_PI / 180.0f,
	   0.0f * M_PI / 180.0f,
	   0.0f * M_PI / 180.0f,
	-110.0f * M_PI / 180.0f,
	 110.0f * M_PI / 180.0f
};

const float AUD_ChannelMapperReader::SURROUND61_ANGLES[] =
{
	  -30.0f * M_PI / 180.0f,
	   30.0f * M_PI / 180.0f,
	   0.0f * M_PI / 180.0f,
	   0.0f * M_PI / 180.0f,
	 180.0f * M_PI / 180.0f,
	-110.0f * M_PI / 180.0f,
	 110.0f * M_PI / 180.0f
};

const float AUD_ChannelMapperReader::SURROUND71_ANGLES[] =
{
	  -30.0f * M_PI / 180.0f,
	   30.0f * M_PI / 180.0f,
	   0.0f * M_PI / 180.0f,
	   0.0f * M_PI / 180.0f,
	-110.0f * M_PI / 180.0f,
	 110.0f * M_PI / 180.0f
	-150.0f * M_PI / 180.0f,
	 150.0f * M_PI / 180.0f
};

const float* AUD_ChannelMapperReader::CHANNEL_ANGLES[] =
{
	AUD_ChannelMapperReader::MONO_ANGLES,
	AUD_ChannelMapperReader::STEREO_ANGLES,
	AUD_ChannelMapperReader::STEREO_LFE_ANGLES,
	AUD_ChannelMapperReader::SURROUND4_ANGLES,
	AUD_ChannelMapperReader::SURROUND5_ANGLES,
	AUD_ChannelMapperReader::SURROUND51_ANGLES,
	AUD_ChannelMapperReader::SURROUND61_ANGLES,
	AUD_ChannelMapperReader::SURROUND71_ANGLES
};
