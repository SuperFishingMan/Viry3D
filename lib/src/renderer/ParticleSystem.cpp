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

#include "ParticleSystem.h"
#include "ParticleSystemRenderer.h"
#include "GameObject.h"
#include "memory/Memory.h"
#include "io/MemoryStream.h"
#include "time/Time.h"
#include "math/Mathf.h"
#include "graphics/VertexAttribute.h"
#include "graphics/Camera.h"

namespace Viry3D
{
	DEFINE_COM_CLASS(ParticleSystem);

	static float random01()
	{
		return Mathf::RandomRange(0.0f, 1.0f);
	}

	static float get_min_max_curve_lerp(float& lerp)
	{
		if (lerp < 0)
		{
			lerp = random01();
		}
		return lerp;
	}

	ParticleSystem::ParticleSystem():
		m_time_start(0),
		m_start_delay(0),
		m_time(0),
		m_time_emit(-1)
	{
	}

	ParticleSystem::~ParticleSystem()
	{
	}

	void ParticleSystem::DeepCopy(const Ref<Object>& source)
	{
		Component::DeepCopy(source);

		auto src = RefCast<ParticleSystem>(source);
		this->main = src->main;
		this->emission = src->emission;
		this->shape = src->shape;
		this->velocity_over_lifetime = src->velocity_over_lifetime;
		this->limit_velocity_over_lifetime = src->limit_velocity_over_lifetime;
		this->inherit_velocity = src->inherit_velocity;
		this->force_over_lifetime = src->force_over_lifetime;
		this->color_over_lifetime = src->color_over_lifetime;
		this->color_by_speed = src->color_by_speed;
		this->size_over_lifetime = src->size_over_lifetime;
		this->size_by_speed = src->size_by_speed;
		this->rotation_over_lifetime = src->rotation_over_lifetime;
		this->rotation_by_speed = src->rotation_by_speed;
		this->external_forces = src->external_forces;
		this->texture_sheet_animation = src->texture_sheet_animation;
	}

	void ParticleSystem::Start()
	{
		m_start_delay = main.start_delay.Evaluate(0, random01());
		m_time_start = Time::GetTime() + m_start_delay;
		m_renderer = this->GetGameObject()->GetComponent<ParticleSystemRenderer>();
	}

	void ParticleSystem::Update()
	{
		if (!m_renderer || !emission.enabled)
		{
			return;
		}

		UpdateEmission();
		UpdateParticles();
	}

	int ParticleSystem::GetParticleCount() const
	{
		return m_particles.Size();
	}

	void ParticleSystem::Emit(Particle& p)
	{
        m_particles.AddLast(p);
		m_time_emit = Time::GetTime();
	}

	void ParticleSystem::UpdateEmission()
	{
		if (!emission.enabled)
		{
			return;
		}

		if (CheckTime())
		{
			float now = Time::GetTime();
			float t = m_time;
			float rate = emission.rate_over_time.Evaluate(t, get_min_max_curve_lerp(emission.rate_over_time_lerp));
			int emit_count = 0;
			Vector<float> emit_time_offsets;

			for (auto& burst : emission.bursts)
			{
				if (m_time >= burst.time)
				{
					if (burst.cycle_count <= 0 || burst.emit_count < burst.cycle_count)
					{
						if (burst.emit_time < 0 || now - burst.emit_time >= burst.repeat_interval)
						{
							burst.emit_time = now;
							burst.emit_count++;
							emit_count += Mathf::RandomRange(burst.min_count, burst.max_count + 1);
						}
					}
				}
			}

			if (emit_count > 0)
			{
				emit_time_offsets.Resize(emit_count, 0);
			}

			float delta = now - m_time_emit;
			if (rate > 0 && (m_time_emit < 0 || delta * main.simulation_speed >= 1.0f / rate))
			{
				if (m_time_emit < 0)
				{
					emit_count += 1;

					emit_time_offsets.Add(0);
				}
				else
				{
					int count = (int) (delta * main.simulation_speed * rate);
					emit_count += count;

					for (int i = 0; i < count; i++)
					{
						emit_time_offsets.Add(i * delta / count);
					}
				}
			}

			if (emit_count > 0)
			{
				int can_emit_count = main.max_particles - GetParticleCount();
				emit_count = Mathf::Min(emit_count, can_emit_count);

				for (int i = 0; i < emit_count; i++)
				{
					float start_speed = main.start_speed.Evaluate(t, random01());
					Color start_color = main.start_color.Evaluate(t, random01());
					float start_lifetime = main.start_lifetime.Evaluate(t, random01());
					float remaining_lifetime = start_lifetime;
					Vector3 start_size(0, 0, 0);
					Vector3 position(0, 0, 0);
					Vector3 rotation(0, 0, 0);
					Vector3 velocity(0, 0, 1);

					if (main.start_size_3d)
					{
						start_size.x = main.start_size_x.Evaluate(t, random01());
						start_size.y = main.start_size_y.Evaluate(t, random01());
						start_size.z = main.start_size_z.Evaluate(t, random01());
					}
					else
					{
						float size = main.start_size.Evaluate(t, random01());
						start_size = Vector3(size, size, size);
					}

					if (main.start_rotation_3d)
					{
						rotation.x = main.start_rotation_x.Evaluate(t, random01());
						rotation.y = main.start_rotation_y.Evaluate(t, random01());
						rotation.z = main.start_rotation_z.Evaluate(t, random01());
					}
					else
					{
						rotation.x = 0;
						rotation.y = 0;
						rotation.z = main.start_rotation.Evaluate(t, random01());
					}

					if (shape.enabled)
					{
						switch (shape.shape_type)
						{
							case ParticleSystemShapeType::Sphere:
								EmitShapeSphere(position, velocity, false);
								break;
							case ParticleSystemShapeType::Hemisphere:
								EmitShapeSphere(position, velocity, true);
								break;
							case ParticleSystemShapeType::Cone:
							case ParticleSystemShapeType::ConeVolume:
								EmitShapeCone(position, velocity);
								break;
							case ParticleSystemShapeType::Box:
							case ParticleSystemShapeType::BoxShell:
							case ParticleSystemShapeType::BoxEdge:
								EmitShapeBox(position, velocity);
								break;
							case ParticleSystemShapeType::Circle:
								EmitShapeCircle(position, velocity);
								break;
							case ParticleSystemShapeType::SingleSidedEdge:
								EmitShapeEdge(position, velocity);
								break;
							default:
								Log("not implement particle emit shape: %d", shape.shape_type);
								break;
						}
					}

					Vector3 world_scale = this->GetTransform()->GetScale();
					Vector3 local_scale = this->GetTransform()->GetLocalScale();

					if (main.scaling_mode == ParticleSystemScalingMode::Hierarchy ||
						main.scaling_mode == ParticleSystemScalingMode::Shape)
					{
						position.x *= world_scale.x;
						position.y *= world_scale.y;
						position.z *= world_scale.z;
					}
					else if (main.scaling_mode == ParticleSystemScalingMode::Local)
					{
						position.x *= local_scale.x;
						position.y *= local_scale.y;
						position.z *= local_scale.z;
					}

					Particle p;
					p.emit_time = Time::GetTime() - emit_time_offsets[i];
					p.start_lifetime = start_lifetime;
					p.remaining_lifetime = remaining_lifetime;
					p.start_size = start_size;
					p.angular_velocity = Vector3::Zero();
					p.rotation = rotation;
					p.start_color = start_color;
					p.color = start_color;
					if (main.simulation_space == ParticleSystemSimulationSpace::World)
					{
						auto& mat = this->GetTransform()->GetLocalToWorldMatrix();
						p.velocity = mat.MultiplyDirection(velocity * start_speed);
						auto mat_scale_invert = Matrix4x4::Scaling(Vector3(1.0f / world_scale.x, 1.0f / world_scale.y, 1.0f / world_scale.z));
						p.position = (mat * mat_scale_invert).MultiplyPoint3x4(position);
					}
					else
					{
						p.velocity = velocity * start_speed;
						p.position = position;
					}
					p.start_velocity = p.velocity;
					p.force_velocity = Vector3::Zero();

					if (texture_sheet_animation.enabled)
					{
						if (texture_sheet_animation.animation == ParticleSystemAnimationType::SingleRow && texture_sheet_animation.use_random_row)
						{
							p.texture_sheet_animation_row = Mathf::RandomRange(0, texture_sheet_animation.num_tiles_y);
						}
					}

					Emit(p);
				}
			}
		}
	}

	bool ParticleSystem::CheckTime()
	{
		float now = Time::GetTime();
		float duration = main.duration;
		
		if (now >= m_time_start && duration > 0)
		{
			float play_time = (now - m_time_start) * main.simulation_speed;
			if (play_time < duration)
			{
				m_time = play_time;
				return true;
			}
			else if (main.loop)
			{
				m_time = 0;
				m_time_start = Time::GetTime();
				emission.ResetBurstState();
				return true;
			}
		}

		return false;
	}

	void ParticleSystem::UpdateParticles()
	{
		for (auto i = m_particles.begin(); i != m_particles.end(); )
		{
			auto& p = *i;

			if (p.remaining_lifetime > 0)
			{
				UpdateParticleLifetime(p);
				UpdateParticleVelocity(p);
				UpdateParticleAngularVelocity(p);
				UpdateParticleColor(p);
				UpdateParticleSize(p);
				UpdateParticleUV(p);
				UpdateParticlePosition(p);
				UpdateParticleRotation(p);
			}

			if (p.remaining_lifetime <= 0)
			{
				i = m_particles.Remove(i);
				continue;
			}

			++i;
		}
	}

	void ParticleSystem::FillVertexBuffer(void* param, const ByteBuffer& buffer)
	{
		auto ps = (ParticleSystem*) param;
		auto vs = (Vertex*) buffer.Bytes();

		auto camera_to_world = Camera::Current()->GetTransform()->GetLocalToWorldMatrix();
		auto world_to_camera = Camera::Current()->GetTransform()->GetWorldToLocalMatrix();
		auto local_to_world = ps->GetTransform()->GetLocalToWorldMatrix();
		auto world_scale = ps->GetTransform()->GetScale();
		auto mat_scale_invert = Matrix4x4::Scaling(Vector3(1.0f / world_scale.x, 1.0f / world_scale.y, 1.0f / world_scale.z));

		int index = 0;
		for (auto i = ps->m_particles.begin(); i != ps->m_particles.end(); ++i, ++index)
		{
			const auto& p = *i;

			Vector3 pos_world;
			Vector3 velocity_world;

			if (ps->main.simulation_space == ParticleSystemSimulationSpace::World)
			{
				pos_world = p.position;
				velocity_world = p.velocity;
			}
			else
			{
                Matrix4x4 to_world = local_to_world * mat_scale_invert;
				pos_world = to_world.MultiplyPoint3x4(p.position);
				velocity_world = to_world.MultiplyDirection(p.velocity);
			}

			auto render_mode = ps->m_renderer->render_mode;

			if (render_mode == ParticleSystemRenderMode::Stretch && Mathf::FloatEqual(p.velocity.SqrMagnitude(), 0))
			{
				render_mode = ParticleSystemRenderMode::Billboard;
			}

			if (render_mode == ParticleSystemRenderMode::Billboard)
			{
				auto& v0 = vs[index * 4 + 0];
				auto& v1 = vs[index * 4 + 1];
				auto& v2 = vs[index * 4 + 2];
				auto& v3 = vs[index * 4 + 3];

				auto rot = Quaternion::Euler(p.rotation * Mathf::Rad2Deg);
				Vector3 pos_view = world_to_camera.MultiplyPoint3x4(pos_world);
				v0.vertex = pos_view + rot * Vector3(-p.size.x * 0.5f, p.size.y * 0.5f, 0.0f);
				v1.vertex = pos_view + rot * Vector3(-p.size.x * 0.5f, -p.size.y * 0.5f, 0.0f);
				v2.vertex = pos_view + rot * Vector3(p.size.x * 0.5f, -p.size.y * 0.5f, 0.0f);
				v3.vertex = pos_view + rot * Vector3(p.size.x * 0.5f, p.size.y * 0.5f, 0.0f);
				v0.vertex = camera_to_world.MultiplyPoint3x4(v0.vertex);
				v1.vertex = camera_to_world.MultiplyPoint3x4(v1.vertex);
				v2.vertex = camera_to_world.MultiplyPoint3x4(v2.vertex);
				v3.vertex = camera_to_world.MultiplyPoint3x4(v3.vertex);
			}
			else if (render_mode == ParticleSystemRenderMode::Stretch)
			{
				auto& v0 = vs[index * 4 + 0];
				auto& v1 = vs[index * 4 + 1];
				auto& v2 = vs[index * 4 + 2];
				auto& v3 = vs[index * 4 + 3];
                
				Vector3 forward = Vector3::Normalize(velocity_world);
				Vector3 right = forward * Camera::Current()->GetTransform()->GetForward();
				Quaternion rot;
				if (Mathf::FloatEqual(right.SqrMagnitude(), 0))
				{
					rot = Quaternion::FromToRotation(Vector3(0, 0, 1), forward);
				}
				else
				{
					Vector3 up = forward * right;
					rot = Quaternion::LookRotation(forward, up);
				}
				float length = p.size.y * ps->m_renderer->length_scale + velocity_world.Magnitude() * ps->m_renderer->velocity_scale;
                v0.vertex = pos_world + rot * Vector3(-p.size.x * 0.5f, 0.0f, 0);
                v1.vertex = pos_world + rot * Vector3(-p.size.x * 0.5f, 0.0f, -length);
                v2.vertex = pos_world + rot * Vector3(p.size.x * 0.5f, 0.0f, -length);
                v3.vertex = pos_world + rot * Vector3(p.size.x * 0.5f, 0.0f, 0);
			}
			else if (render_mode == ParticleSystemRenderMode::HorizontalBillboard)
			{
				auto& v0 = vs[index * 4 + 0];
				auto& v1 = vs[index * 4 + 1];
				auto& v2 = vs[index * 4 + 2];
				auto& v3 = vs[index * 4 + 3];
				
				auto rot = Quaternion::Euler(Vector3(0, 0, p.rotation.z) * Mathf::Rad2Deg);
				v0.vertex = pos_world + rot * Vector3(-p.size.x * 0.5f, 0, p.size.y * 0.5f);
				v1.vertex = pos_world + rot * Vector3(-p.size.x * 0.5f, 0, -p.size.y * 0.5f);
				v2.vertex = pos_world + rot * Vector3(p.size.x * 0.5f, 0, -p.size.y * 0.5f);
				v3.vertex = pos_world + rot * Vector3(p.size.x * 0.5f, 0, p.size.y * 0.5f);
			}
			else if (render_mode == ParticleSystemRenderMode::VerticalBillboard)
			{
				auto& v0 = vs[index * 4 + 0];
				auto& v1 = vs[index * 4 + 1];
				auto& v2 = vs[index * 4 + 2];
				auto& v3 = vs[index * 4 + 3];

				auto cam_rot = Camera::Current()->GetTransform()->GetRotation().ToEulerAngles();
				auto rot = Quaternion::Euler(Vector3(0, cam_rot.y, p.rotation.z * Mathf::Rad2Deg));
				v0.vertex = pos_world + rot * Vector3(-p.size.x * 0.5f, p.size.y * 0.5f, 0);
				v1.vertex = pos_world + rot * Vector3(-p.size.x * 0.5f, -p.size.y * 0.5f, 0);
				v2.vertex = pos_world + rot * Vector3(p.size.x * 0.5f, -p.size.y * 0.5f, 0);
				v3.vertex = pos_world + rot * Vector3(p.size.x * 0.5f, p.size.y * 0.5f, 0);
			}
			else if (render_mode == ParticleSystemRenderMode::Mesh)
			{
				auto rot = Quaternion::Euler(p.rotation * Mathf::Rad2Deg);

				const auto& mesh = ps->m_renderer->mesh;
				int vertex_count = mesh->vertices.Size();
				for (int j = 0; j < vertex_count; j++)
				{
					auto& v = vs[index * vertex_count + j];

                    Vector3 pos_view = world_to_camera.MultiplyPoint3x4(pos_world);
                    v.vertex = pos_view + Matrix4x4::TRS(Vector3(0, 0, 0), rot, p.size).MultiplyPoint3x4(mesh->vertices[j]);
                    v.vertex = camera_to_world.MultiplyPoint3x4(v.vertex);

					v.uv = mesh->uv[j];
					if (mesh->colors.Size() > 0)
					{
						v.color = p.color * mesh->colors[j];
					}
					else
					{
						v.color = p.color;
					}
				}
			}

			if (render_mode != ParticleSystemRenderMode::Mesh)
			{
				auto& v0 = vs[index * 4 + 0];
				auto& v1 = vs[index * 4 + 1];
				auto& v2 = vs[index * 4 + 2];
				auto& v3 = vs[index * 4 + 3];

				if (render_mode == ParticleSystemRenderMode::Stretch)
				{
					v0.uv = Vector2(p.uv_scale_offset.z, p.uv_scale_offset.y + p.uv_scale_offset.w);
					v1.uv = Vector2(p.uv_scale_offset.x + p.uv_scale_offset.z, p.uv_scale_offset.y + p.uv_scale_offset.w);
					v2.uv = Vector2(p.uv_scale_offset.x + p.uv_scale_offset.z, p.uv_scale_offset.w);
					v3.uv = Vector2(p.uv_scale_offset.z, p.uv_scale_offset.w);
				}
				else
				{
					v0.uv = Vector2(p.uv_scale_offset.z, p.uv_scale_offset.w);
					v1.uv = Vector2(p.uv_scale_offset.z, p.uv_scale_offset.y + p.uv_scale_offset.w);
					v2.uv = Vector2(p.uv_scale_offset.x + p.uv_scale_offset.z, p.uv_scale_offset.y + p.uv_scale_offset.w);
					v3.uv = Vector2(p.uv_scale_offset.x + p.uv_scale_offset.z, p.uv_scale_offset.w);
				}
				
				v0.color = p.color;
				v1.color = p.color;
				v2.color = p.color;
				v3.color = p.color;
			}
		}
	}

	void ParticleSystem::FillIndexBuffer(void* param, const ByteBuffer& buffer)
	{
		auto ps = (ParticleSystem*) param;
		MemoryStream ms(buffer);

		int index_offset = 0;
		for (const auto& i : ps->m_particles)
		{
			if (ps->m_renderer->render_mode == ParticleSystemRenderMode::Mesh)
			{
				for (auto index : ps->m_renderer->mesh->triangles)
				{
					ms.Write<unsigned short>(index_offset + index);
				}
				index_offset += ps->m_renderer->mesh->vertices.Size();
			}
			else
			{
				ms.Write<unsigned short>(index_offset + 0);
				ms.Write<unsigned short>(index_offset + 1);
				ms.Write<unsigned short>(index_offset + 2);
				ms.Write<unsigned short>(index_offset + 0);
				ms.Write<unsigned short>(index_offset + 2);
				ms.Write<unsigned short>(index_offset + 3);
				index_offset += 4;
			}
		}

		ms.Close();
	}

	void ParticleSystem::UpdateBuffer()
	{
		if (m_particles.Size() == 0)
		{
			return;
		}

		int vertex_count_per_particle = 4;
		int index_count_per_particle = 6;

		if (m_renderer->render_mode == ParticleSystemRenderMode::Mesh)
		{
			vertex_count_per_particle = m_renderer->mesh->vertices.Size();
			index_count_per_particle = m_renderer->mesh->triangles.Size();
		}

		int vertex_count = m_particles.Size() * vertex_count_per_particle;
		assert(vertex_count < 65536);
		int vertex_buffer_size = vertex_count * sizeof(Vertex);
		if (!m_vertex_buffer || m_vertex_buffer->GetSize() < vertex_buffer_size)
		{
			m_vertex_buffer = VertexBuffer::Create(vertex_buffer_size, true);
		}
		m_vertex_buffer->Fill(this, ParticleSystem::FillVertexBuffer);

		int index_count = m_particles.Size() * index_count_per_particle;
		int index_buffer_size = index_count * sizeof(unsigned short);
		if (!m_index_buffer || m_index_buffer->GetSize() < index_buffer_size)
		{
			m_index_buffer = IndexBuffer::Create(index_buffer_size, false);
			m_index_buffer->Fill(this, ParticleSystem::FillIndexBuffer);
		}
	}

	void ParticleSystem::GetIndexRange(int submesh_index, int& start, int& count)
	{
		int index_count_per_particle = 6;
		if (m_renderer->render_mode == ParticleSystemRenderMode::Mesh)
		{
			index_count_per_particle = m_renderer->mesh->triangles.Size();
		}

		start = 0;
		count = m_particles.Size() * index_count_per_particle;
	}

	void ParticleSystem::UpdateParticleVelocity(Particle& p)
	{
		Vector3 v = Vector3(0, 0, 0);
		float delta_time = Time::GetDeltaTime() * main.simulation_speed;
		float lifetime_t = Mathf::Clamp01((p.start_lifetime - p.remaining_lifetime) / p.start_lifetime);
		auto mat_scale = Matrix4x4::Scaling(this->GetTransform()->GetScale());
		auto local_to_world = this->GetTransform()->GetLocalToWorldMatrix();
		auto world_to_local = this->GetTransform()->GetWorldToLocalMatrix();

        if (main.simulation_space == ParticleSystemSimulationSpace::World)
        {
            v += p.start_velocity;
        }
        else
        {
            v += mat_scale.MultiplyPoint3x4(p.start_velocity);
        }

		if (velocity_over_lifetime.enabled)
		{
			float x = velocity_over_lifetime.x.Evaluate(lifetime_t, get_min_max_curve_lerp(p.velocity_over_lifetime_x_lerp));
			float y = velocity_over_lifetime.y.Evaluate(lifetime_t, get_min_max_curve_lerp(p.velocity_over_lifetime_y_lerp));
			float z = velocity_over_lifetime.z.Evaluate(lifetime_t, get_min_max_curve_lerp(p.velocity_over_lifetime_z_lerp));

			if (main.simulation_space == ParticleSystemSimulationSpace::World)
			{
				if (velocity_over_lifetime.space == ParticleSystemSimulationSpace::World)
				{
					v += mat_scale.MultiplyPoint3x4(Vector3(x, y, z));
				}
				else
				{
					v += local_to_world.MultiplyDirection(mat_scale.MultiplyPoint3x4(Vector3(x, y, z)));
				}
			}
			else
			{
				if (velocity_over_lifetime.space == ParticleSystemSimulationSpace::World)
				{
					v += world_to_local.MultiplyDirection(mat_scale.MultiplyPoint3x4(Vector3(x, y, z)));
				}
				else
				{
					v += mat_scale.MultiplyPoint3x4(Vector3(x, y, z));
				}
			}
		}

		if (force_over_lifetime.enabled)
		{
            float x = force_over_lifetime.x.Evaluate(lifetime_t, get_min_max_curve_lerp(p.force_over_lifetime_x_lerp));
            float y = force_over_lifetime.y.Evaluate(lifetime_t, get_min_max_curve_lerp(p.force_over_lifetime_y_lerp));
            float z = force_over_lifetime.z.Evaluate(lifetime_t, get_min_max_curve_lerp(p.force_over_lifetime_z_lerp));

            Vector3 force = Vector3(x, y, z);

			if (main.simulation_space == ParticleSystemSimulationSpace::World)
			{
				if (force_over_lifetime.space == ParticleSystemSimulationSpace::World)
				{
                    p.force_velocity += mat_scale.MultiplyPoint3x4(force) * delta_time;
				}
				else
				{
                    Vector3 local_v = mat_scale.MultiplyPoint3x4(force);
                    float len = local_v.Magnitude();
                    Vector3 world_v = local_to_world.MultiplyDirection(local_v);
                    if (len > 0)
                    {
                        world_v = Vector3::Normalize(world_v) * len;
                    }
					p.force_velocity += world_v * delta_time;
				}
			}
			else
			{
				if (force_over_lifetime.space == ParticleSystemSimulationSpace::World)
				{
                    Vector3 world_v = mat_scale.MultiplyPoint3x4(force);
                    float len = world_v.Magnitude();
                    Vector3 local_v = world_to_local.MultiplyDirection(world_v);
                    if (len > 0)
                    {
                        local_v = Vector3::Normalize(local_v) * len;
                    }
					p.force_velocity += local_v * delta_time;
				}
				else
				{
					p.force_velocity += mat_scale.MultiplyPoint3x4(force) * delta_time;
				}
			}
		}

        // apply gravity
        {
            float gravity = -10.0f * main.gravity_modifier.Evaluate(lifetime_t, get_min_max_curve_lerp(main.gravity_modifier_lerp));

            p.force_velocity += Vector3(0, gravity, 0) * delta_time;
        }

		v += p.force_velocity;

		if (limit_velocity_over_lifetime.enabled)
		{
			if (limit_velocity_over_lifetime.separate_axes)
			{
				float x = limit_velocity_over_lifetime.limit_x.Evaluate(lifetime_t, get_min_max_curve_lerp(p.limit_velocity_over_lifetime_limit_x_lerp));
				float y = limit_velocity_over_lifetime.limit_y.Evaluate(lifetime_t, get_min_max_curve_lerp(p.limit_velocity_over_lifetime_limit_y_lerp));
				float z = limit_velocity_over_lifetime.limit_z.Evaluate(lifetime_t, get_min_max_curve_lerp(p.limit_velocity_over_lifetime_limit_z_lerp));
				Vector3 limit;

				if (main.simulation_space == ParticleSystemSimulationSpace::World)
				{
					if (limit_velocity_over_lifetime.space == ParticleSystemSimulationSpace::World)
					{
						limit = mat_scale.MultiplyPoint3x4(Vector3(x, y, z));
					}
					else
					{
						limit = local_to_world.MultiplyDirection(mat_scale.MultiplyPoint3x4(Vector3(x, y, z)));
					}
				}
				else
				{
					if (limit_velocity_over_lifetime.space == ParticleSystemSimulationSpace::World)
					{
						limit = world_to_local.MultiplyDirection(mat_scale.MultiplyPoint3x4(Vector3(x, y, z)));
					}
					else
					{
						limit = mat_scale.MultiplyPoint3x4(Vector3(x, y, z));
					}
				}

				if (v.x > limit.x)
				{
					float exceed = v.x - limit.x;
					exceed *= 1.0f - limit_velocity_over_lifetime.dampen;
					v.x = limit.x + exceed;
				}
				if (v.y > limit.y)
				{
					float exceed = v.y - limit.y;
					exceed *= 1.0f - limit_velocity_over_lifetime.dampen;
					v.y = limit.y + exceed;
				}
				if (v.z > limit.z)
				{
					float exceed = v.z - limit.z;
					exceed *= 1.0f - limit_velocity_over_lifetime.dampen;
					v.z = limit.z + exceed;
				}
			}
			else
			{
				float limit = limit_velocity_over_lifetime.limit.Evaluate(lifetime_t, get_min_max_curve_lerp(p.limit_velocity_over_lifetime_limit_lerp));
				float mag = v.Magnitude();
				if (mag > limit)
				{
					float exceed = mag - limit;
					exceed *= 1.0f - limit_velocity_over_lifetime.dampen;
					v = Vector3::Normalize(v) * (limit + exceed);
				}
			}
		}

		p.velocity = v;
	}

	void ParticleSystem::UpdateParticleAngularVelocity(Particle& p)
	{
		Vector3 v = Vector3(0, 0, 0);

		if (rotation_over_lifetime.enabled)
		{
			float lifetime_t = Mathf::Clamp01((p.start_lifetime - p.remaining_lifetime) / p.start_lifetime);

			if (rotation_over_lifetime.separate_axes)
			{
				v.x += rotation_over_lifetime.x.Evaluate(lifetime_t, get_min_max_curve_lerp(p.rotation_over_lifetime_x_lerp));
				v.y += rotation_over_lifetime.y.Evaluate(lifetime_t, get_min_max_curve_lerp(p.rotation_over_lifetime_y_lerp));
				v.z += rotation_over_lifetime.z.Evaluate(lifetime_t, get_min_max_curve_lerp(p.rotation_over_lifetime_z_lerp));
			}
			else
			{
				v.z += rotation_over_lifetime.z.Evaluate(lifetime_t, get_min_max_curve_lerp(p.rotation_over_lifetime_z_lerp));
			}
		}

		if (rotation_by_speed.enabled)
		{
			float speed = p.velocity.Magnitude();
			float speed_t = (speed - rotation_by_speed.range.x) / (rotation_by_speed.range.y - rotation_by_speed.range.x);
			speed_t = Mathf::Clamp01(speed_t);

			if (rotation_by_speed.separate_axes)
			{
				v.x += rotation_by_speed.x.Evaluate(speed_t, get_min_max_curve_lerp(p.rotation_by_speed_x_lerp));
				v.y += rotation_by_speed.y.Evaluate(speed_t, get_min_max_curve_lerp(p.rotation_by_speed_y_lerp));
				v.z += rotation_by_speed.z.Evaluate(speed_t, get_min_max_curve_lerp(p.rotation_by_speed_z_lerp));
			}
			else
			{
				v.z += rotation_by_speed.z.Evaluate(speed_t, get_min_max_curve_lerp(p.rotation_by_speed_z_lerp));
			}
		}

		p.angular_velocity = v;
	}

	void ParticleSystem::UpdateParticleColor(Particle& p)
	{
		Color c = p.start_color;

		if (color_over_lifetime.enabled)
		{
			float lifetime_t = Mathf::Clamp01((p.start_lifetime - p.remaining_lifetime) / p.start_lifetime);
			c *= color_over_lifetime.color.Evaluate(lifetime_t, get_min_max_curve_lerp(p.color_over_lifetime_color_lerp));
		}

		if (color_by_speed.enabled)
		{
			float speed = p.velocity.Magnitude();
			float speed_t = (speed - color_by_speed.range.x) / (color_by_speed.range.y - color_by_speed.range.x);
			speed_t = Mathf::Clamp01(speed_t);

			c *= color_by_speed.color.Evaluate(speed_t, get_min_max_curve_lerp(p.color_by_speed_color_lerp));
		}

		p.color = c;
	}

	void ParticleSystem::UpdateParticleSize(Particle& p)
	{
		Vector3 s = p.start_size;

		if (main.scaling_mode == ParticleSystemScalingMode::Hierarchy)
		{
			Vector3 scale = this->GetTransform()->GetScale();
			s.x *= scale.x;
			s.y *= scale.y;
			s.z *= scale.z;
		}
		else if (main.scaling_mode == ParticleSystemScalingMode::Local)
		{
			Vector3 scale = this->GetTransform()->GetLocalScale();
			s.x *= scale.x;
			s.y *= scale.y;
			s.z *= scale.z;
		}

		if (size_over_lifetime.enabled)
		{
			float lifetime_t = Mathf::Clamp01((p.start_lifetime - p.remaining_lifetime) / p.start_lifetime);

			if (size_over_lifetime.separate_axes)
			{
				s.x *= size_over_lifetime.x.Evaluate(lifetime_t, get_min_max_curve_lerp(p.size_over_lifetime_x_lerp));
				s.y *= size_over_lifetime.y.Evaluate(lifetime_t, get_min_max_curve_lerp(p.size_over_lifetime_y_lerp));
				s.z *= size_over_lifetime.z.Evaluate(lifetime_t, get_min_max_curve_lerp(p.size_over_lifetime_z_lerp));
			}
			else
			{
				s *= size_over_lifetime.size.Evaluate(lifetime_t, get_min_max_curve_lerp(p.size_over_lifetime_size_lerp));
			}
		}

		if (size_by_speed.enabled)
		{
			float speed = p.velocity.Magnitude();
			float speed_t = (speed - size_by_speed.range.x) / (size_by_speed.range.y - size_by_speed.range.x);
			speed_t = Mathf::Clamp01(speed_t);

			if (size_by_speed.separate_axes)
			{
				s.x *= size_by_speed.x.Evaluate(speed_t, get_min_max_curve_lerp(p.size_by_speed_x_lerp));
				s.y *= size_by_speed.y.Evaluate(speed_t, get_min_max_curve_lerp(p.size_by_speed_y_lerp));
				s.z *= size_by_speed.z.Evaluate(speed_t, get_min_max_curve_lerp(p.size_by_speed_z_lerp));
			}
			else
			{
				s *= size_by_speed.size.Evaluate(speed_t, get_min_max_curve_lerp(p.size_by_speed_size_lerp));
			}
		}

		p.size = s;
	}

	void ParticleSystem::UpdateParticleUV(Particle& p)
	{
		if (texture_sheet_animation.enabled)
		{
			p.uv_scale_offset.x = 1.0f / texture_sheet_animation.num_tiles_x;
			p.uv_scale_offset.y = 1.0f / texture_sheet_animation.num_tiles_y;

			int row = 0;
			int start_frame;
			int frame_over_time;

			if (texture_sheet_animation.animation == ParticleSystemAnimationType::SingleRow)
			{
				if (texture_sheet_animation.use_random_row)
				{
					row = p.texture_sheet_animation_row;
				}
				else
				{
					row = texture_sheet_animation.row_index;
				}
			}

			if (texture_sheet_animation.animation == ParticleSystemAnimationType::SingleRow)
			{
				start_frame = (int) (texture_sheet_animation.num_tiles_x * texture_sheet_animation.start_frame.Evaluate(0, get_min_max_curve_lerp(p.texture_sheet_animation_start_frame_lerp)));
			}
			else
			{
				start_frame = (int) (texture_sheet_animation.num_tiles_x * texture_sheet_animation.num_tiles_y * texture_sheet_animation.start_frame.Evaluate(0, get_min_max_curve_lerp(p.texture_sheet_animation_start_frame_lerp)));
			}

			float lifetime_t = Mathf::Clamp01((p.start_lifetime - p.remaining_lifetime) / p.start_lifetime);
			if (texture_sheet_animation.animation == ParticleSystemAnimationType::SingleRow)
			{
				frame_over_time = (int) (texture_sheet_animation.num_tiles_x * texture_sheet_animation.frame_over_time.Evaluate(lifetime_t, get_min_max_curve_lerp(p.texture_sheet_animation_frame_over_time_lerp)));
			}
			else
			{
				frame_over_time = (int) (texture_sheet_animation.num_tiles_x * texture_sheet_animation.num_tiles_y * texture_sheet_animation.frame_over_time.Evaluate(lifetime_t, get_min_max_curve_lerp(p.texture_sheet_animation_frame_over_time_lerp)));
			}

			int frame = row * texture_sheet_animation.num_tiles_x + start_frame + frame_over_time;
			frame = frame % (texture_sheet_animation.num_tiles_x * texture_sheet_animation.num_tiles_y);
			int x = frame % texture_sheet_animation.num_tiles_x;
			int y = frame / texture_sheet_animation.num_tiles_x;
			p.uv_scale_offset.z = x * p.uv_scale_offset.x;
			p.uv_scale_offset.w = y * p.uv_scale_offset.y;
		}
		else
		{
			p.uv_scale_offset = Vector4(1, 1, 0, 0);
		}
	}

	void ParticleSystem::UpdateParticlePosition(Particle& p)
	{
		float delta_time = Time::GetDeltaTime() * main.simulation_speed;

		p.position += p.velocity * delta_time;
	}

	void ParticleSystem::UpdateParticleRotation(Particle& p)
	{
		float delta_time = Time::GetDeltaTime() * main.simulation_speed;

		p.rotation += p.angular_velocity * delta_time;
	}

	void ParticleSystem::UpdateParticleLifetime(Particle& p)
	{
		p.remaining_lifetime = p.start_lifetime - (Time::GetTime() - p.emit_time) * main.simulation_speed;
	}

	void ParticleSystem::EmitShapeSphere(Vector3& position, Vector3& velocity, bool hemi)
	{
		Vector3 dir;
		do
		{
			dir.x = Mathf::RandomRange(-0.5f, 0.5f);
			dir.y = Mathf::RandomRange(-0.5f, 0.5f);
			if (hemi)
			{
				dir.z = Mathf::RandomRange(0.0f, 0.5f);
			}
			else
			{
				dir.z = Mathf::RandomRange(-0.5f, 0.5f);
			}
		} while (Mathf::FloatEqual(dir.SqrMagnitude(), 0));
		dir.Normalize();

		float radius = Mathf::RandomRange(shape.radius * (1.0f - shape.radius_thickness), shape.radius);
		position = dir * radius;
		velocity = dir;

		if (shape.random_direction_amount > 0)
		{
			Vector3 dir;
			do
			{
				dir.x = Mathf::RandomRange(-0.5f, 0.5f);
				dir.y = Mathf::RandomRange(-0.5f, 0.5f);
				dir.z = Mathf::RandomRange(-0.5f, 0.5f);
			} while (Mathf::FloatEqual(dir.SqrMagnitude(), 0));

			velocity = Vector3::Lerp(velocity, Vector3::Normalize(dir), shape.random_direction_amount);
			velocity.Normalize();
		}
	}

	void ParticleSystem::EmitShapeCone(Vector3& position, Vector3& velocity)
	{
		float angle = Mathf::Clamp(shape.angle, 1.0f, 89.0f);
		Vector3 origin = Vector3(0, 0, -shape.radius / tanf(angle * Mathf::Deg2Rad));
		float arc = Mathf::RandomRange(0.0f, shape.arc);
		float z = 0;
		float radius = 0;

		switch (shape.shape_type)
		{
			case ParticleSystemShapeType::Cone:
			{
				z = 0;
				radius = Mathf::RandomRange(shape.radius * (1.0f - shape.radius_thickness), shape.radius);
				break;
			}
			case ParticleSystemShapeType::ConeVolume:
			{
				z = Mathf::RandomRange(0.0f, shape.length);
				radius = tanf(angle * Mathf::Deg2Rad) / (fabsf(origin.z) + z);
				radius = Mathf::RandomRange(radius * (1.0f - shape.radius_thickness), radius);
				break;
			}
            default:
                break;
		}

		float x = radius * cosf(arc * Mathf::Deg2Rad);
		float y = radius * sinf(arc * Mathf::Deg2Rad);
		position = Vector3(x, y, z);
		velocity = Vector3::Normalize(position - origin);

		if (shape.random_direction_amount > 0)
		{
			Vector3 dir;
			float r = Mathf::RandomRange(0.0f, shape.radius);
			Vector3 pos = Vector3(r * cosf(arc * Mathf::Deg2Rad), r * sinf(arc * Mathf::Deg2Rad), 0);
			dir = pos - origin;

			velocity = Vector3::Lerp(velocity, Vector3::Normalize(dir), shape.random_direction_amount);
			velocity.Normalize();
		}

		if (shape.spherical_direction_amount > 0)
		{
			Vector3 dir = position;

			velocity = Vector3::Lerp(velocity, Vector3::Normalize(dir), shape.spherical_direction_amount);
			velocity.Normalize();
		}
	}

	void ParticleSystem::EmitShapeBox(Vector3& position, Vector3& velocity)
	{
		float x = Mathf::RandomRange(-0.5f, 0.5f) * shape.scale.x;
		float y = Mathf::RandomRange(-0.5f, 0.5f) * shape.scale.y;
		float z = Mathf::RandomRange(-0.5f, 0.5f) * shape.scale.z;

		switch (shape.shape_type)
		{
			case ParticleSystemShapeType::Box:
			{
				position.x = x;
				position.y = y;
				position.z = z;
				break;
			}
			case ParticleSystemShapeType::BoxShell:
			{
				int side = Mathf::RandomRange(0, 6);
				switch (side)
				{
					case 0:
						position.x = -0.5f * shape.scale.x;
						position.y = y;
						position.z = z;
						break;
					case 1:
						position.x = 0.5f * shape.scale.x;
						position.y = y;
						position.z = z;
						break;
					case 2:
						position.x = x;
						position.y = -0.5f * shape.scale.y;
						position.z = z;
						break;
					case 3:
						position.x = x;
						position.y = 0.5f * shape.scale.y;
						position.z = z;
						break;
					case 4:
						position.x = x;
						position.y = y;
						position.z = -0.5f * shape.scale.z;
						break;
					case 5:
						position.x = x;
						position.y = y;
						position.z = 0.5f * shape.scale.z;
						break;
				}
				break;
			}
			case ParticleSystemShapeType::BoxEdge:
			{
				int edge = Mathf::RandomRange(0, 12);
				switch (edge)
				{
					case 0:
						position.x = -0.5f * shape.scale.x;
						position.y = -0.5f * shape.scale.y;
						position.z = z;
						break;
					case 1:
						position.x = -0.5f * shape.scale.x;
						position.y = 0.5f * shape.scale.y;
						position.z = z;
						break;
					case 2:
						position.x = 0.5f * shape.scale.x;
						position.y = -0.5f * shape.scale.y;
						position.z = z;
						break;
					case 3:
						position.x = 0.5f * shape.scale.x;
						position.y = 0.5f * shape.scale.y;
						position.z = z;
						break;
					case 4:
						position.x = x;
						position.y = -0.5f * shape.scale.y;
						position.z = -0.5f * shape.scale.z;
						break;
					case 5:
						position.x = x;
						position.y = -0.5f * shape.scale.y;
						position.z = 0.5f * shape.scale.z;
						break;
					case 6:
						position.x = x;
						position.y = 0.5f * shape.scale.y;
						position.z = -0.5f * shape.scale.z;
						break;
					case 7:
						position.x = x;
						position.y = 0.5f * shape.scale.y;
						position.z = 0.5f * shape.scale.z;
						break;
					case 8:
						position.x = -0.5f * shape.scale.x;
						position.y = y;
						position.z = -0.5f * shape.scale.z;
						break;
					case 9:
						position.x = -0.5f * shape.scale.x;
						position.y = y;
						position.z = 0.5f * shape.scale.z;
						break;
					case 10:
						position.x = 0.5f * shape.scale.x;
						position.y = y;
						position.z = -0.5f * shape.scale.z;
						break;
					case 11:
						position.x = 0.5f * shape.scale.x;
						position.y = y;
						position.z = 0.5f * shape.scale.z;
						break;
				}
				break;
			}
            default:
                break;
		}

		if (shape.random_direction_amount > 0)
		{
			Vector3 dir;
			do
			{
				dir.x = Mathf::RandomRange(-0.5f, 0.5f);
				dir.y = Mathf::RandomRange(-0.5f, 0.5f);
				dir.z = Mathf::RandomRange(-0.5f, 0.5f);
			} while (Mathf::FloatEqual(dir.SqrMagnitude(), 0));

			velocity = Vector3::Lerp(velocity, Vector3::Normalize(dir), shape.random_direction_amount);
			velocity.Normalize();
		}

		if (shape.spherical_direction_amount > 0)
		{
			Vector3 dir = position;

			velocity = Vector3::Lerp(velocity, Vector3::Normalize(dir), shape.spherical_direction_amount);
			velocity.Normalize();
		}
	}

	void ParticleSystem::EmitShapeCircle(Vector3& position, Vector3& velocity)
	{
		float arc = Mathf::RandomRange(0.0f, shape.arc);
		float radius = Mathf::RandomRange(shape.radius * (1.0f - shape.radius_thickness), shape.radius);

		float x = radius * cosf(arc * Mathf::Deg2Rad);
		float y = radius * sinf(arc * Mathf::Deg2Rad);
		position = Vector3(x, y, 0);
		velocity = Vector3::Normalize(position);

		if (shape.random_direction_amount > 0)
		{
			Vector3 dir;
			do
			{
				dir.x = Mathf::RandomRange(-0.5f, 0.5f);
				dir.y = Mathf::RandomRange(-0.5f, 0.5f);
				dir.z = 0;
			} while (Mathf::FloatEqual(dir.SqrMagnitude(), 0));

			velocity = Vector3::Lerp(velocity, Vector3::Normalize(dir), shape.random_direction_amount);
			velocity.Normalize();
		}
	}

	void ParticleSystem::EmitShapeEdge(Vector3& position, Vector3& velocity)
	{
		float x = Mathf::RandomRange(-1.0f, 1.0f) * shape.radius;

		position = Vector3(x, 0, 0);
		velocity = Vector3(0, 1, 0);

		if (shape.random_direction_amount > 0)
		{
			Vector3 dir;
			do
			{
				dir.x = Mathf::RandomRange(-0.5f, 0.5f);
				dir.y = Mathf::RandomRange(-0.5f, 0.5f);
				dir.z = Mathf::RandomRange(-0.5f, 0.5f);
			} while (Mathf::FloatEqual(dir.SqrMagnitude(), 0));

			velocity = Vector3::Lerp(velocity, Vector3::Normalize(dir), shape.random_direction_amount);
			velocity.Normalize();
		}

		if (shape.spherical_direction_amount > 0)
		{
			Vector3 dir = position;

			velocity = Vector3::Lerp(velocity, Vector3::Normalize(dir), shape.spherical_direction_amount);
			velocity.Normalize();
		}
	}

	float ParticleSystem::MinMaxCurve::Evaluate(float time, float lerp)
	{
		time = Mathf::Clamp01(time);

		if (mode == ParticleSystemCurveMode::Constant)
		{
			return constant;
		}
		else if (mode == ParticleSystemCurveMode::TwoConstants)
		{
			return Mathf::Lerp(constant_min, constant_max, lerp);
		}
		else if (mode == ParticleSystemCurveMode::Curve)
		{
			return curve.Evaluate(time) * curve_multiplier;
		}
		else if (mode == ParticleSystemCurveMode::TwoCurves)
		{
			return Mathf::Lerp(curve_min.Evaluate(time), curve_max.Evaluate(time), lerp) * curve_multiplier;
		}

		return 0;
	}

	Color Gradient::Evaluate(float time)
	{
		Color c;

		if (time <= r.keys[0].time)
		{
			c.r = r.keys[0].value;
			c.g = g.keys[0].value;
			c.b = b.keys[0].value;
		}
		else if (time >= r.keys[r.keys.Size() - 1].time)
		{
			c.r = r.keys[r.keys.Size() - 1].value;
			c.g = g.keys[r.keys.Size() - 1].value;
			c.b = b.keys[r.keys.Size() - 1].value;
		}
		else
		{
			int index;

			for (int i = 0; i < r.keys.Size() - 1; i++)
			{
				if (time > r.keys[i].time && time <= r.keys[i + 1].time)
				{
					index = i;
					break;
				}
			}

			if (mode == GradientMode::Blend)
			{
				float t = (time - r.keys[index].time) / (r.keys[index + 1].time - r.keys[index].time);
				c.r = Mathf::Lerp(r.keys[index].value, r.keys[index + 1].value, t);
				c.g = Mathf::Lerp(g.keys[index].value, g.keys[index + 1].value, t);
				c.b = Mathf::Lerp(b.keys[index].value, b.keys[index + 1].value, t);
			}
			else if (mode == GradientMode::Fixed)
			{
				c.r = r.keys[index + 1].value;
				c.g = g.keys[index + 1].value;
				c.b = b.keys[index + 1].value;
			}
		}

		if (time <= a.keys[0].time)
		{
			c.a = a.keys[0].value;
		}
		else if (time >= a.keys[a.keys.Size() - 1].time)
		{
			c.a = a.keys[a.keys.Size() - 1].value;
		}
		else
		{
			int index;

			for (int i = 0; i < a.keys.Size() - 1; i++)
			{
				if (time > a.keys[i].time && time <= a.keys[i + 1].time)
				{
					index = i;
					break;
				}
			}

			if (mode == GradientMode::Blend)
			{
				float t = (time - a.keys[index].time) / (a.keys[index + 1].time - a.keys[index].time);
				c.a = Mathf::Lerp(a.keys[index].value, a.keys[index + 1].value, t);
			}
			else if (mode == GradientMode::Fixed)
			{
				c.a = a.keys[index + 1].value;
			}
		}

		return c;
	}

	Color ParticleSystem::MinMaxGradient::Evaluate(float time, float lerp)
	{
		time = Mathf::Clamp01(time);

		if (mode == ParticleSystemGradientMode::Color)
		{
			return color;
		}
		else if (mode == ParticleSystemGradientMode::Gradient)
		{
			return gradient.Evaluate(time);
		}
		else if (mode == ParticleSystemGradientMode::TwoColors)
		{
			float r = Mathf::Lerp(color_min.r, color_max.r, lerp);
			float g = Mathf::Lerp(color_min.g, color_max.g, lerp);
			float b = Mathf::Lerp(color_min.b, color_max.b, lerp);
			float a = Mathf::Lerp(color_min.a, color_max.a, lerp);
			return Color(r, g, b, a);
		}
		else if (mode == ParticleSystemGradientMode::TwoGradients)
		{
			Color min = gradient_min.Evaluate(time);
			Color max = gradient_max.Evaluate(time);
			float r = Mathf::Lerp(min.r, max.r, lerp);
			float g = Mathf::Lerp(min.g, max.g, lerp);
			float b = Mathf::Lerp(min.b, max.b, lerp);
			float a = Mathf::Lerp(min.a, max.a, lerp);
			return Color(r, g, b, a);
		}
		else if (mode == ParticleSystemGradientMode::RandomColor)
		{
			return gradient.Evaluate(lerp);
		}

		return Color();
	}
}
