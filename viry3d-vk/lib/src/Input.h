/*
* Viry3D
* Copyright 2014-2018 by Stack - stackos@qq.com
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

#include "math/Vector2.h"
#include "math/Vector3.h"
#include "container/Vector.h"
#include "KeyCode.h"

namespace Viry3D
{
	enum class TouchPhase
	{
		Began,          //A finger touched the screen.
		Moved,          //A finger moved on the screen.
		Ended,          //A finger was lifted from the screen. This is the final phase of a touch.
	};

	struct Touch
	{
		Vector2 deltaPosition;
		float time;
		float deltaTime;
		int fingerId;
		TouchPhase phase;
		Vector2 position;
		int tapCount;

		Touch() { }
	};

	class Input
	{
	public:
		static int GetTouchCount();
		static const Touch& GetTouch(int index);
		static void Update();
		static bool GetKeyDown(KeyCode key);
		static bool GetKey(KeyCode key);
		static bool GetKeyUp(KeyCode key);
		static bool GetMouseButtonDown(int index);
		static bool GetMouseButton(int index);
		static bool GetMouseButtonUp(int index);
		static const Vector3& GetMousePosition();
        static float GetMouseScrollWheel();
	};
}
