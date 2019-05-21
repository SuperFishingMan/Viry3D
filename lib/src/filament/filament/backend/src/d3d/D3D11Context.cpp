/*
* Viry3D
* Copyright 2014-2019 by Stack - stackos@qq.com
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

#include "D3D11Context.h"
#include <assert.h>

namespace filament
{
	namespace backend
	{
		static bool SDKLayersAvailable()
		{
			HRESULT hr = D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_NULL,       // ���贴��ʵ��Ӳ���豸��
				0,
				D3D11_CREATE_DEVICE_DEBUG,  // ���� SDK �㡣
				nullptr,                    // �κι��ܼ��𶼻�������
				0,
				D3D11_SDK_VERSION,          // ���� Microsoft Store Ӧ�ã�ʼ�ս���ֵ����Ϊ D3D11_SDK_VERSION��
				nullptr,                    // ���豣�� D3D �豸���á�
				nullptr,                    // ����֪�����ܼ���
				nullptr                     // ���豣�� D3D �豸���������á�
			);

			return SUCCEEDED(hr);
		}

		D3D11Context::D3D11Context()
		{
			UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
			sdk_layers_available = SDKLayersAvailable();
			if (sdk_layers_available)
			{
				creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
			}
#endif

			D3D_FEATURE_LEVEL feature_levels[] =
			{
				D3D_FEATURE_LEVEL_12_1,
				D3D_FEATURE_LEVEL_12_0,
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1,
				D3D_FEATURE_LEVEL_10_0,
				D3D_FEATURE_LEVEL_9_3,
				D3D_FEATURE_LEVEL_9_2,
				D3D_FEATURE_LEVEL_9_1
			};

			ID3D11DeviceContext* context_0 = nullptr;
			ID3D11Device* device_0 = nullptr;
			HRESULT hr = D3D11CreateDevice(
				nullptr,					// ָ�� nullptr ��ʹ��Ĭ����������
				D3D_DRIVER_TYPE_HARDWARE,	// ����ʹ��Ӳ��ͼ������������豸��
				0,							// ӦΪ 0���������������� D3D_DRIVER_TYPE_SOFTWARE��
				creation_flags,				// ���õ��Ժ� Direct2D �����Ա�־��
				feature_levels,				// ��Ӧ�ó������֧�ֵĹ��ܼ�����б�
				ARRAYSIZE(feature_levels),	// ������б�Ĵ�С��
				D3D11_SDK_VERSION,			// ���� Microsoft Store Ӧ�ã�ʼ�ս���ֵ����Ϊ D3D11_SDK_VERSION��
				&device_0,					// ���ش����� Direct3D �豸��
				&feature_level,				// �����������豸�Ĺ��ܼ���
				&context_0					// �����豸�ļ�ʱ�����ġ�
			);
			
			if (FAILED(hr))
			{
				hr = D3D11CreateDevice(
					nullptr,
					D3D_DRIVER_TYPE_WARP, // ���� WARP �豸������Ӳ���豸��
					0,
					creation_flags,
					feature_levels,
					ARRAYSIZE(feature_levels),
					D3D11_SDK_VERSION,
					&device_0,
					&feature_level,
					&context_0
				);

				assert(SUCCEEDED(hr));
			}

			device_0->QueryInterface(__uuidof(ID3D11Device3), (void**) &device);
			context_0->QueryInterface(__uuidof(ID3D11DeviceContext3), (void**) &context);
			
			SAFE_RELEASE(device_0);
			SAFE_RELEASE(context_0);
		}

		D3D11Context::~D3D11Context()
		{
			for (auto i : rasterizer_states)
			{
				SAFE_RELEASE(i.second.raster);
				SAFE_RELEASE(i.second.blend);
				SAFE_RELEASE(i.second.depth);
			}
			rasterizer_states.clear();

			SAFE_RELEASE(device);
			SAFE_RELEASE(context);
		}
	}
}
