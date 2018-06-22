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

#include "VertexAttribute.h"
#include "container/Vector.h"

namespace Viry3D
{
    struct BufferObject;

    class Mesh
    {
    public:
        Mesh(const Vector<Vertex>& vertices, const Vector<unsigned short>& indices);
        ~Mesh();
        const Ref<BufferObject>& GetVertexBuffer() const { return m_vertex_buffer; }
        const Ref<BufferObject>& GetIndexBuffer() const { return m_index_buffer; }
        int GetVertexCount() const { return m_vertex_count; }
        int GetIndexCount() const { return m_index_count; }

    private:
        Ref<BufferObject> m_vertex_buffer;
        Ref<BufferObject> m_index_buffer;
        int m_vertex_count;
        int m_index_count;
    };
}
