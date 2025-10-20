#include "parametric_shapes.hpp"
#include "core/Log.h"

#include <glm/glm.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

bonobo::mesh_data
parametric_shapes::createQuad(float const width, float const height,
	unsigned int const horizontal_split_count,
	unsigned int const vertical_split_count)
{
	bonobo::mesh_data data;

	// 顶点/三角形数量
	const unsigned int Nx = horizontal_split_count + 1u;
	const unsigned int Nz = vertical_split_count + 1u;
	const unsigned int vertex_count = Nx * Nz;
	const unsigned int quad_count = horizontal_split_count * vertical_split_count;
	const unsigned int triangle_count = quad_count * 2u;

	// 交错顶点结构：pos(vec3) + uv(vec2)
	struct VertexPT { glm::vec3 p; glm::vec2 uv; };
	std::vector<VertexPT> vertices(vertex_count);
	std::vector<glm::uvec3> index_sets(triangle_count);

	// 居中范围
	const float x0 = -0.5f * width;
	const float z0 = -0.5f * height;

	// 顶点生成（x–z 平面, y=0），UV ∈ [0,1]^2
	for (unsigned int j = 0; j < Nz; ++j) {
		float v = (Nz > 1u) ? (static_cast<float>(j) / static_cast<float>(Nz - 1u)) : 0.0f;
		float z = z0 + v * height;
		for (unsigned int i = 0; i < Nx; ++i) {
			float u = (Nx > 1u) ? (static_cast<float>(i) / static_cast<float>(Nx - 1u)) : 0.0f;
			float x = x0 + u * width;

			const unsigned int idx = j * Nx + i;
			vertices[idx].p = glm::vec3(x, 0.0f, z);
			vertices[idx].uv = glm::vec2(u, v);
		}
	}

	// 索引（两个三角形组成一个小格）
	unsigned int t = 0u;
	for (unsigned int j = 0; j < vertical_split_count; ++j) {
		for (unsigned int i = 0; i < horizontal_split_count; ++i) {
			unsigned int i0 = j * Nx + i;
			unsigned int i1 = j * Nx + (i + 1u);
			unsigned int i2 = (j + 1u) * Nx + i;
			unsigned int i3 = (j + 1u) * Nx + (i + 1u);

			index_sets[t++] = glm::uvec3(i0, i2, i3); // 右上
			index_sets[t++] = glm::uvec3(i0, i3, i1); // 左下
		}
	}

	// === 上传到 GPU ===
	glGenVertexArrays(1, &data.vao);
	glBindVertexArray(data.vao);

	glGenBuffers(1, &data.bo);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(vertices.size() * sizeof(VertexPT)),
		vertices.data(),
		GL_STATIC_DRAW);

	// 位置
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices),
		3, GL_FLOAT, GL_FALSE,
		sizeof(VertexPT),
		reinterpret_cast<GLvoid const*>(offsetof(VertexPT, p)));

	// 纹理坐标
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords),
		2, GL_FLOAT, GL_FALSE,
		sizeof(VertexPT),
		reinterpret_cast<GLvoid const*>(offsetof(VertexPT, uv)));

	// 索引
	glGenBuffers(1, &data.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(index_sets.size() * sizeof(glm::uvec3)),
		index_sets.data(),
		GL_STATIC_DRAW);

	data.indices_nb = static_cast<GLuint>(index_sets.size() * 3u);

	// Unbind
	glBindVertexArray(0u);
	glBindBuffer(GL_ARRAY_BUFFER, 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}
//bonobo::mesh_data
//parametric_shapes::createQuad(float const width, float const height,
//                              unsigned int const horizontal_split_count,
//                              unsigned int const vertical_split_count)
//{
//	auto const vertices = std::array<glm::vec3, 4>{
//		glm::vec3(0.0f,  0.0f,   0.0f),
//		glm::vec3(width, 0.0f,   0.0f),
//		glm::vec3(width, height, 0.0f),
//		glm::vec3(0.0f,  height, 0.0f)
//	};
//
//	auto const index_sets = std::array<glm::uvec3, 2>{
//		glm::uvec3(0u, 1u, 2u),
//		glm::uvec3(0u, 2u, 3u)
//	};
//
//	bonobo::mesh_data data;
//
//	if (horizontal_split_count > 0u || vertical_split_count > 0u)
//	{
//		LogError("parametric_shapes::createQuad() does not support tesselation.");
//		return data;
//	}
//
//	//
//	// NOTE:
//	//
//	// Only the values preceeded by a `\todo` tag should be changed, the
//	// other ones are correct!
//	//
//
//	// Create a Vertex Array Object: it will remember where we stored the
//	// data on the GPU, and  which part corresponds to the vertices, which
//	// one for the normals, etc.
//	// 
//	//
//	// The following function will create new Vertex Arrays, and pass their
//	// name in the given array (second argument). Since we only need one,
//	// pass a pointer to `data.vao`.
//	glGenVertexArrays(1, &data.vao);
//
//	// To be able to store information, the Vertex Array has to be bound
//	// first.
//	glBindVertexArray(data.vao);
//
//	// To store the data, we need to allocate buffers on the GPU. Let's
//	// allocate a first one for the vertices.
//	//
//	// The following function's syntax is similar to `glGenVertexArray()`:
//	// it will create multiple OpenGL objects, in this case buffers, and
//	// return their names in an array. Have the buffer's name stored into
//	// `data.bo`.
//	glGenBuffers(1,&data.bo);
//
//	// Similar to the Vertex Array, we need to bind it first before storing
//	// anything in it. The data stored in it can be interpreted in
//	// different ways. Here, we will say that it is just a simple 1D-array
//	// and therefore bind the buffer to the corresponding target.
//	glBindBuffer(GL_ARRAY_BUFFER, data.bo); /*! \todo bind the previously generated Buffer */
//
//	glBufferData(GL_ARRAY_BUFFER,
//				 /*! \todo how many bytes should the buffer contain? */vertices.size() * sizeof(glm::vec3),
//	             /* where is the data stored on the CPU? */vertices.data(),
//	             /* inform OpenGL that the data is modified once, but used often */GL_STATIC_DRAW);
//
//	// Vertices have been just stored into a buffer, but we still need to
//	// tell Vertex Array where to find them, and how to interpret the data
//	// within that buffer.
//	//
//	// You will see shaders in more detail in lab 3, but for now they are
//	// just pieces of code running on the GPU and responsible for moving
//	// all the vertices to clip space, and assigning a colour to each pixel
//	// covered by geometry.
//	// Those shaders have inputs, some of them are the data we just stored
//	// in a buffer object. We need to tell the Vertex Array which inputs
//	// are enabled, and this is done by the following line of code, which
//	// enables the input for vertices:
//	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
//
//	// Once an input is enabled, we need to explain where the data comes
//	// from, and how it interpret it. When calling the following function,
//	// the Vertex Array will automatically use the current buffer bound to
//	// GL_ARRAY_BUFFER as its source for the data. How to interpret it is
//	// specified below:
//	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices),
//	                      3,/*! \todo how many components do our vertices have? */ 
//	                      /* what is the type of each component? */GL_FLOAT,
//	                      /* should it automatically normalise the values stored */GL_FALSE,
//	                      /* once all components of a vertex have been read, how far away (in bytes) is the next vertex? */ 0,
//	                      /* how far away (in bytes) from the start of the buffer is the first vertex? */reinterpret_cast<GLvoid const*>(0x0));
//
//	// Now, let's allocate a second one for the indices.
//	//
//	// Have the buffer's name stored into `data.ibo`.
//	glGenBuffers(1, &data.ibo);
//
//	// We still want a 1D-array, but this time it should be a 1D-array of
//	// elements, aka. indices!
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, /*! \todo bind the previously generated Buffer */data.ibo);
//
//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_sets.size() * sizeof(glm::uvec3), /*! \todo how many bytes should the buffer contain? */
//	             /* where is the data stored on the CPU? */index_sets.data(),
//	             /* inform OpenGL that the data is modified once, but used often */GL_STATIC_DRAW);
//
//	data.indices_nb = static_cast<GLuint>(index_sets.size() * 3u); /*! \todo how many indices do we have? */
//
//	// All the data has been recorded, we can unbind them.
//	glBindVertexArray(0u);
//	glBindBuffer(GL_ARRAY_BUFFER, 0u);
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
//
//	return data;
//}

bonobo::mesh_data
parametric_shapes::createSphere(float const radius,
                                unsigned int const longitude_split_count,
                                unsigned int const latitude_split_count)
{

	//! \todo Implement this function
	bonobo::mesh_data data;

	if (radius <= 0.0f || longitude_split_count < 2u || latitude_split_count < 1u) {
		LogError("createSphere(): invalid parameters (r>0, L>=2, S>=1).");
		return data;
	}

	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;   // dp/dθ
		glm::vec3 binormal;  // orothogonal to both normal and tangent
		glm::vec3 texcoord;  // u = θ/(2π), v = ϕ/π
	};

	auto const L = longitude_split_count;
	auto const S = latitude_split_count;

	// (L + 1) * (S + 1) vertices in total
	std::vector<Vertex> vertices;
	vertices.reserve((L + 1)* (S + 1));

	auto P = [&](float theta, float phi) -> glm::vec3 {
		// p(θ,ϕ) = { r sinθ sinϕ,  -r cosϕ,  r cosθ sinϕ }.
		return glm::vec3(
			radius * std::sin(theta) * std::sin(phi),
			-radius * std::cos(phi),
			radius * std::cos(theta) * std::sin(phi)
		);
		};

	auto dP_dtheta = [&](float theta, float phi) -> glm::vec3 {
		
		return glm::vec3(
			radius * std::cos(theta) * std::sin(phi),
			0.0f,
			-radius * std::sin(theta) * std::sin(phi)
		);
		};

	auto dP_dphi = [&](float theta, float phi) -> glm::vec3 {
		return glm::vec3(
			radius * std::sin(theta) * std::cos(phi),
			radius * std::sin(phi),
			radius * std::cos(theta) * std::cos(phi)
		);
		};

	// === 1) vertices（position、normal、tangent、binormal、UV）===

	for (unsigned int i = 0; i <= S; ++i) {
		float const v = static_cast<float>(i) / static_cast<float>(S); // The normalized ratio of the current latitude at[0, 1]
		float const phi = v * glm::pi<float>();        // ϕ ∈ [0, π]

		for (unsigned int j = 0; j <= L; ++j) {
			float const u = static_cast<float>(j) / static_cast<float>(L);
			float const theta = u * glm::two_pi<float>(); // θ ∈ [0, 2π]

			glm::vec3 pos = P(theta, phi); // Generate spherical points according to formula (1a)
			glm::vec3 nrm = glm::normalize(pos);  // unit sphere

			// —— tangent（∂p/∂θ), note: 
			// 非极点：用 ∂p/∂θ 并做正交化；极点：用种子向量与法线叉乘得到稳定切线
			glm::vec3 tan;
			if (i == 0u || i == S) {
				// Pole: ∂p/∂θ degenerates (≈0), cannot be directly calculated; instead,
				// use a seed that is not collinear with the normal.
				// Vectors that are not parallel to the normal are regarded as "seeds".
				glm::vec3 seed = (std::abs(nrm.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
				tan = glm::normalize(glm::cross(seed, nrm));
			}
			else {
				tan = dP_dtheta(theta, phi);
				tan = glm::normalize(tan - nrm * glm::dot(tan, nrm));//Remove the normal component and unit it
			}

			// right-handed --> Form orthogonal bases.
			glm::vec3 bin = glm::normalize(glm::cross(nrm, tan));

			// 基本 UV（下一次作业才用到，但现在先写好）
			glm::vec3 uv(u, v, 0);

			vertices.push_back(Vertex{ pos, nrm, tan, bin, uv });
		}
	}

	// === 2) index（Each small quadrilateral is split into two triangles;
	// note that j = L should not be connected with j = 0.）===
	std::vector<GLuint> indices;
	indices.reserve(L * S * 6u);
	for (unsigned int i = 0; i < S; ++i) {
		for (unsigned int j = 0; j < L; ++j) {
			GLuint const a = i * (L + 1u) + j;
			GLuint const b = a + 1u;
			GLuint const c = (i + 1u) * (L + 1u) + j;
			GLuint const d = c + 1u;

			//  CCW order
			indices.push_back(a); indices.push_back(c); indices.push_back(b);
			indices.push_back(b); indices.push_back(c); indices.push_back(d);
		}
	}

	// === 3) write to GPU（VBO + IBO） ===
	glGenVertexArrays(1, &data.vao);
	glBindVertexArray(data.vao);

	glGenBuffers(1, &data.bo);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
		vertices.data(),
		GL_STATIC_DRAW);

	glGenBuffers(1, &data.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)),
		indices.data(),
		GL_STATIC_DRAW);

	// === 4) VAO attribute pointer configuration ===
	GLsizei const stride = static_cast<GLsizei>(sizeof(Vertex));
	// position
	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::vertices),
		3, GL_FLOAT, GL_FALSE, stride,
		reinterpret_cast<GLvoid const*>(offsetof(Vertex, position)));
	// normal
	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::normals),
		3, GL_FLOAT, GL_FALSE, stride,
		reinterpret_cast<GLvoid const*>(offsetof(Vertex, normal)));
	// tangent
	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::tangents),
		3, GL_FLOAT, GL_FALSE, stride,
		reinterpret_cast<GLvoid const*>(offsetof(Vertex, tangent)));
	// （binormal/bitangent）
	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::binormals),
		3, GL_FLOAT, GL_FALSE, stride,
		reinterpret_cast<GLvoid const*>(offsetof(Vertex, binormal)));
	// texture uv coordinate
	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::texcoords),
		3, GL_FLOAT, GL_FALSE, stride,
		reinterpret_cast<GLvoid const*>(offsetof(Vertex, texcoord)));

	data.indices_nb = static_cast<GLuint>(indices.size());

	// unbind
	glBindVertexArray(0u);
	glBindBuffer(GL_ARRAY_BUFFER, 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}

bonobo::mesh_data
parametric_shapes::createTorus(float const major_radius,
	float const minor_radius,
	unsigned int const major_split_count,
	unsigned int const minor_split_count)
{
	bonobo::mesh_data data;

	if (major_radius <= 0.0f || minor_radius <= 0.0f ||
		major_split_count < 3u || minor_split_count < 3u) {
		LogError("createTorus(): invalid parameters.");
		return data;
	}

	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 binormal;
		glm::vec2 texcoord;
	};

	unsigned int L = major_split_count;
	unsigned int S = minor_split_count;

	std::vector<Vertex> vertices;
	vertices.reserve((L + 1) * (S + 1));

	for (unsigned int i = 0; i <= L; ++i) {
		float theta = (static_cast<float>(i) / L) * glm::two_pi<float>();
		for (unsigned int j = 0; j <= S; ++j) {
			float phi = (static_cast<float>(j) / S) * glm::two_pi<float>();

			// position
			glm::vec3 pos(
				(major_radius + minor_radius * std::cos(theta)) * std::cos(phi),
				-minor_radius * std::sin(theta),
				(major_radius + minor_radius * std::cos(theta)) * std::sin(phi)
			);

			// tangent dP/dθ
			glm::vec3 tan(
				-minor_radius * std::sin(theta) * std::cos(phi),
				-minor_radius * std::cos(theta),
				-minor_radius * std::sin(theta) * std::sin(phi)
			);

			// binormal dP/dφ
			glm::vec3 bin(
				-(major_radius + minor_radius * std::cos(theta)) * std::sin(phi),
				0.0f,
				(major_radius + minor_radius * std::cos(theta)) * std::cos(phi)
			);

			glm::vec3 nrm = glm::normalize(glm::cross(tan, bin));
			tan = glm::normalize(tan);
			bin = glm::normalize(bin);

			glm::vec2 uv(
				static_cast<float>(i) / L,
				static_cast<float>(j) / S
			);

			vertices.push_back(Vertex{ pos, nrm, tan, bin, uv });
		}
	}

	// === Indices ===
	std::vector<GLuint> indices;
	indices.reserve(L * S * 6u);

	for (unsigned int i = 0; i < L; ++i) {
		for (unsigned int j = 0; j < S; ++j) {
			GLuint a = i * (S + 1) + j;
			GLuint b = a + 1;
			GLuint c = (i + 1) * (S + 1) + j;
			GLuint d = c + 1;

			indices.push_back(a); indices.push_back(c); indices.push_back(b);
			indices.push_back(b); indices.push_back(c); indices.push_back(d);
		}
	}

	// === Upload to GPU ===
	glGenVertexArrays(1, &data.vao);
	glBindVertexArray(data.vao);

	glGenBuffers(1, &data.bo);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
		vertices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &data.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
		indices.data(), GL_STATIC_DRAW);

	GLsizei stride = sizeof(Vertex);

	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::vertices),
		3, GL_FLOAT, GL_FALSE, stride,
		reinterpret_cast<GLvoid const*>(offsetof(Vertex, position)));

	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::normals),
		3, GL_FLOAT, GL_FALSE, stride,
		reinterpret_cast<GLvoid const*>(offsetof(Vertex, normal)));

	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::tangents),
		3, GL_FLOAT, GL_FALSE, stride,
		reinterpret_cast<GLvoid const*>(offsetof(Vertex, tangent)));

	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::binormals),
		3, GL_FLOAT, GL_FALSE, stride,
		reinterpret_cast<GLvoid const*>(offsetof(Vertex, binormal)));

	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::texcoords),
		2, GL_FLOAT, GL_FALSE, stride,
		reinterpret_cast<GLvoid const*>(offsetof(Vertex, texcoord)));

	data.indices_nb = static_cast<GLuint>(indices.size());

	glBindVertexArray(0u);
	glBindBuffer(GL_ARRAY_BUFFER, 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}


bonobo::mesh_data
parametric_shapes::createCircleRing(float const radius,
                                    float const spread_length,
                                    unsigned int const circle_split_count,
                                    unsigned int const spread_split_count)
{
	auto const circle_slice_edges_count = circle_split_count + 1u;
	auto const spread_slice_edges_count = spread_split_count + 1u;
	auto const circle_slice_vertices_count = circle_slice_edges_count + 1u;
	auto const spread_slice_vertices_count = spread_slice_edges_count + 1u;
	auto const vertices_nb = circle_slice_vertices_count * spread_slice_vertices_count;

	auto vertices  = std::vector<glm::vec3>(vertices_nb);
	auto normals   = std::vector<glm::vec3>(vertices_nb);
	auto texcoords = std::vector<glm::vec3>(vertices_nb);
	auto tangents  = std::vector<glm::vec3>(vertices_nb);
	auto binormals = std::vector<glm::vec3>(vertices_nb);

	float const spread_start = radius - 0.5f * spread_length;
	float const d_theta = glm::two_pi<float>() / (static_cast<float>(circle_slice_edges_count));
	float const d_spread = spread_length / (static_cast<float>(spread_slice_edges_count));

	// generate vertices iteratively
	size_t index = 0u;
	float theta = 0.0f;
	for (unsigned int i = 0u; i < circle_slice_vertices_count; ++i) {
		float const cos_theta = std::cos(theta);
		float const sin_theta = std::sin(theta);

		float distance_to_centre = spread_start;
		for (unsigned int j = 0u; j < spread_slice_vertices_count; ++j) {
			// vertex
			vertices[index] = glm::vec3(distance_to_centre * cos_theta,
			                            distance_to_centre * sin_theta,
			                            0.0f);

			// texture coordinates
			texcoords[index] = glm::vec3(static_cast<float>(j) / (static_cast<float>(spread_slice_vertices_count)),
			                             static_cast<float>(i) / (static_cast<float>(circle_slice_vertices_count)),
			                             0.0f);

			// tangent
			auto const t = glm::vec3(cos_theta, sin_theta, 0.0f);
			tangents[index] = t;

			// binormal
			auto const b = glm::vec3(-sin_theta, cos_theta, 0.0f);
			binormals[index] = b;

			// normal
			auto const n = glm::cross(t, b);
			normals[index] = n;

			distance_to_centre += d_spread;
			++index;
		}

		theta += d_theta;
	}

	// create index array
	auto index_sets = std::vector<glm::uvec3>(2u * circle_slice_edges_count * spread_slice_edges_count);

	// generate indices iteratively
	index = 0u;
	for (unsigned int i = 0u; i < circle_slice_edges_count; ++i)
	{
		for (unsigned int j = 0u; j < spread_slice_edges_count; ++j)
		{
			index_sets[index] = glm::uvec3(spread_slice_vertices_count * (i + 0u) + (j + 0u),
			                               spread_slice_vertices_count * (i + 0u) + (j + 1u),
			                               spread_slice_vertices_count * (i + 1u) + (j + 1u));
			++index;

			index_sets[index] = glm::uvec3(spread_slice_vertices_count * (i + 0u) + (j + 0u),
			                               spread_slice_vertices_count * (i + 1u) + (j + 1u),
			                               spread_slice_vertices_count * (i + 1u) + (j + 0u));
			++index;
		}
	}

	bonobo::mesh_data data;
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	glBindVertexArray(data.vao);

	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const tangents_offset = texcoords_offset + texcoords_size;
	auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
	auto const binormals_offset = tangents_offset + tangents_size;
	auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size
	                                            +normals_size
	                                            +texcoords_size
	                                            +tangents_size
	                                            +binormals_size
	                                            );
	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

	glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(tangents.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

	glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(binormals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = static_cast<GLsizei>(index_sets.size() * 3u);
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(index_sets.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(index_sets.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}
