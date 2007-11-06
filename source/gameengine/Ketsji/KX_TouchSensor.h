/**
 * Senses touch and collision events
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

#ifndef __KX_TOUCHSENSOR
#define __KX_TOUCHSENSOR

#include "SCA_ISensor.h"
#include "ListValue.h"

struct PHY_CollData;

#include "KX_ClientObjectInfo.h"

class KX_TouchEventManager;

class KX_TouchSensor : public SCA_ISensor
{
protected:
	Py_Header;

	/**
	 * The sensor should only look for objects with this property.
	 */
	STR_String				m_touchedpropname;	
	bool					m_bFindMaterial;
	class SCA_EventManager*	m_eventmgr;
	
	class PHY_IPhysicsController*	m_physCtrl;
	class PHY_ResponseTable*		m_responstTable;
	class PHY_PhysicsController*	m_responsObject;

	bool					m_bCollision;
	bool					m_bTriggered;
	bool					m_bLastTriggered;
	SCA_IObject*		    m_hitObject;
	class CListValue*		m_colliders;
	
public:
	KX_TouchSensor(class SCA_EventManager* eventmgr,
		class KX_GameObject* gameobj,
		bool fFindMaterial,
		const STR_String& touchedpropname,
		PyTypeObject* T=&Type) ;
	virtual ~KX_TouchSensor();

	virtual CValue* GetReplica();
	virtual void SynchronizeTransform();
	virtual bool Evaluate(CValue* event);
	virtual void ReParent(SCA_IObject* parent);
	
	virtual void RegisterSumo(KX_TouchEventManager* touchman);

//	virtual DT_Bool HandleCollision(void* obj1,void* obj2,
//						 const DT_CollData * coll_data); 

	virtual bool	NewHandleCollision(void*obj1,void*obj2,const PHY_CollData* colldata);

	PHY_PhysicsController*	GetPhysicsController() { return m_responsObject;}
  

	virtual bool IsPositiveTrigger() {
		bool result = m_bTriggered;
		if (m_invert) result = !result;
		return result;
	}

	
	virtual void EndFrame();

	// todo: put some info for collision maybe

	/* --------------------------------------------------------------------- */
	/* Python interface ---------------------------------------------------- */
	/* --------------------------------------------------------------------- */
	
	virtual PyObject* _getattr(const STR_String& attr);

	/* 1. setProperty */
	KX_PYMETHOD_DOC(KX_TouchSensor,SetProperty);
	/* 2. getProperty */
	KX_PYMETHOD_DOC(KX_TouchSensor,GetProperty);
	/* 3. getHitObject */
	KX_PYMETHOD_DOC(KX_TouchSensor,GetHitObject);
	/* 4. getHitObject */
	KX_PYMETHOD_DOC(KX_TouchSensor,GetHitObjectList);
	/* 5. getTouchMaterial */
	KX_PYMETHOD_DOC(KX_TouchSensor,GetTouchMaterial);
	/* 6. setTouchMaterial */
	KX_PYMETHOD_DOC(KX_TouchSensor,SetTouchMaterial);
	
};

#endif //__KX_TOUCHSENSOR

