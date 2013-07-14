#pragma once
#include "Data.h"
namespace Asset {
	struct Vertex {
		Vertex(vec3 p_position, vec3 p_normal, optional<vec2> p_uv) :
	position(p_position), normal(p_normal), uv(p_uv) {
	}
	vec3 position;
	vec3 normal;
	optional<vec2> uv;
	};
	struct AssetGeometry {
		AssetGeometry(const vector<Vertex>& p_verticies, const vector<unsigned int>& p_indices) :
	vertices(p_verticies), indices(p_indices) {
	}
	vector<Vertex> vertices;
	vector<unsigned int> indices;	
	bool has_uvs() const {
		assert(vertices.size() > 0);
		return vertices.at(0).uv;
	}
	};
	vec3 to_vec3(const aiVector3D& v) {
		return vec3(v.x, v.y, v.z);
	}
	struct AssetMesh
	{
		AssetMesh(const AssetGeometry& p_geom, const optional<TextureData>& p_albedo_data)
			: geometry(p_geom), albedo_data(p_albedo_data) { }
		AssetGeometry geometry;
		optional<TextureData> albedo_data;
	};
	vector<AssetMesh> asset_load_meshes(string path, string textures_root, aiPostProcessSteps aux_flags) {
		const aiScene* scene = aiImportFile(path.c_str(),
			aiProcess_PreTransformVertices | aux_flags);


		vector<AssetMesh> asset_meshes;
		for (auto mesh_idx = 0u; mesh_idx < scene->mNumMeshes; mesh_idx++) {
			auto mesh = scene->mMeshes[mesh_idx];

			bool has_uvs = mesh->GetNumUVChannels() > 0;
			assert(mesh->HasNormals());
			vector<Vertex> vertices;
			for (auto vert_idx = 0u; vert_idx < mesh->mNumVertices; vert_idx++) {
				auto ai_vertex = mesh->mVertices[vert_idx];
				auto ai_normal = mesh->mNormals[vert_idx];
				if(has_uvs) {
					aiVector3D uv = mesh->mTextureCoords[0][vert_idx];
					vertices.push_back(Vertex(to_vec3(ai_vertex), to_vec3(ai_normal), vec2(uv.x, uv.y)));
				} else {
					vertices.push_back(Vertex(to_vec3(ai_vertex), to_vec3(ai_normal), optional<vec2>()));
				}
			}
			vector<unsigned int> indices;
			for (auto face_idx = 0u; face_idx < mesh->mNumFaces; face_idx++) {
				const auto& face = mesh->mFaces[face_idx];
				for (auto face_vert_idx = 0u; face_vert_idx < face.mNumIndices; face_vert_idx++) {
					indices.push_back(face.mIndices[face_vert_idx]);
				}
			}


			auto material = scene->mMaterials[mesh->mMaterialIndex];
			auto num_albedo_tex = aiGetMaterialTextureCount(material, aiTextureType_DIFFUSE);
			optional<TextureData> albedo_tex_data;
			if(num_albedo_tex > 0) {
				aiString tex_path;
				aiGetMaterialTexture(material, aiTextureType_DIFFUSE, 0, &tex_path);
				string full_tex_path = textures_root + tex_path.C_Str();
				assert(boost::filesystem::exists(full_tex_path));
				albedo_tex_data = load_texture(full_tex_path);
			}


			AssetGeometry geom(vertices, indices);
			AssetMesh asset_mesh(geom,albedo_tex_data);
			asset_meshes.push_back(asset_mesh);
		}
		return asset_meshes;
	}
}