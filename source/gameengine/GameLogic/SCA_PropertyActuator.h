/**
 * SCA_PropertyActuator.h
 *
 * $Id$
 *
 * ***** BEGIN GPL/BL DUAL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. The Blender
 * Foundation also sells licenses for use in proprietary software under
 * the Blender License.  See http://www.blender.org/BL/ for information
 * about this.
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
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */

#ifndef __KX_PROPERTYACTUATOR
#define __KX_PROPERTYACTUATOR

#include "SCA_IActuator.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

class SCA_PropertyActuator : public SCA_IActuator
{
	Py_Header;
	
	enum KX_ACT_PROP_MODE {
		KX_ACT_PROP_NODEF = 0,
		KX_ACT_PROP_ASSIGN,
		KX_ACT_PROP_ADD,
		KX_ACT_PROP_COPY,
		KX_ACT_PROP_MAX
	};
	
	/**check whether this value is valid */
	bool isValid(KX_ACT_PROP_MODE mode);
	
	int			m_type;
	STR_String	m_propname;
	STR_String	m_exprtxt;
	CValue*		m_sourceObj; // for copy property actuator

public:



	SCA_PropertyActuator(

		SCA_IObject* gameobj,

		CValue* sourceObj,

		const STR_String& propname,

		const STR_String& expr,

		int acttype,

		PyTypeObject* T=&Type

	);


	~SCA_PropertyActuator();


		CValue* 

	GetReplica(

	);


		bool 

	Update(

		double curtime,

		double deltatime

	);

	/* --------------------------------------------------------------------- */
	/* Python interface ---------------------------------------------------- */
	/* --------------------------------------------------------------------- */

	PyObject*  _getattr(char *attr);

	// python wrapped methods
	KX_PYMETHOD_DOC(SCA_PropertyActuator,SetProperty);
	KX_PYMETHOD_DOC(SCA_PropertyActuator,GetProperty);
	KX_PYMETHOD_DOC(SCA_PropertyActuator,SetValue);
	KX_PYMETHOD_DOC(SCA_PropertyActuator,GetValue);
	
	/* 5. - ... setObject, getObject, setProp2, getProp2, setMode, getMode*/
	
};

#endif //__KX_PROPERTYACTUATOR_DOC

