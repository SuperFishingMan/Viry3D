/*
* Viry3D
* Copyright 2014-2017 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include "Component.h"

namespace Viry3D
{
	class Collider: public Component
	{
		DECLARE_COM_CLASS(Collider, Component);
	public:
		virtual void SetIsRigidbody(bool value);

	protected:
		void* m_collider;
		bool m_in_world;
		bool m_is_rigidbody;

		Collider():
			m_collider(NULL),
			m_in_world(false),
			m_is_rigidbody(false)
		{
		}
		virtual ~Collider();
		virtual void OnEnable();
		virtual void OnDisable();
		virtual void OnLayerChanged();
	};
}
