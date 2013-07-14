#pragma once
#include "Asset.h"
using namespace Asset;
namespace CompiledAsset {
	struct BakedGeometry {
		BakedGeometry() { } // for serialization
		BakedGeometry(const DataBuffer<float>& p_positions, const DataBuffer<float>& p_normals,
			const optional<DataBuffer<float>>& p_uvs,
			const DataBuffer<unsigned int>& p_indices) :
		positions(p_positions), normals(p_normals), indices(p_indices), uvs(p_uvs) { }
		DataBuffer<float> positions;
		DataBuffer<float> normals;
		optional<DataBuffer<float>> uvs;
		DataBuffer<unsigned int> indices;

		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & positions;
			ar & normals;
			ar & uvs;
			ar & indices;
		}
	};
	struct BakedTexture {
		BakedTexture() { }
		BakedTexture(const TextureData& p_texture_data) : texture_data(p_texture_data) { }
		TextureData texture_data;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & texture_data;
		}
	};
	struct BakedMesh {
		BakedMesh() { }
		BakedMesh(const BakedGeometry& p_geom, optional<BakedTexture> p_albedo_tex) 
			: geometry(p_geom), albedo_tex(p_albedo_tex) { }

		BakedGeometry geometry;
		optional<BakedTexture> albedo_tex;

		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & geometry;
			ar & albedo_tex;
		}
	};
	optional<BakedTexture> bake_texture(const optional<TextureData>& texture) {
		if(texture) {
			return BakedTexture(*texture);
		}
		return optional<BakedTexture>();
	}
	BakedGeometry bake_geometry(const AssetGeometry& geom) {
		vector<float> positions;
		vector<float> normals;
		vector<float> uvs;
		//for (auto v : geom.vertices) {
		for(size_t idx = 0; idx < geom.vertices.size(); idx++) {
			auto& v = geom.vertices[idx];
			positions.push_back(v.position.x);
			positions.push_back(v.position.y);
			positions.push_back(v.position.z);
			normals.push_back(v.normal.x);
			normals.push_back(v.normal.y);
			normals.push_back(v.normal.z);
			if(v.uv) {
				uvs.push_back(v.uv->x);
				uvs.push_back(v.uv->y);
			}
		}
		const vector<unsigned int>& indices = geom.indices;
		return BakedGeometry(positions, normals, geom.has_uvs() ? optional<DataBuffer<float>>(DataBuffer<float>(uvs)) : optional<DataBuffer<float>>(), indices);
	}
	vector<BakedMesh> bake_meshes(const vector<AssetMesh>& meshes) {
		vector<BakedMesh> baked_meshes;
		transform(begin(meshes), end(meshes), back_inserter(baked_meshes), [](const AssetMesh& mesh) -> BakedMesh {
			auto baked_geom = bake_geometry(mesh.geometry);
			auto baked_albedo = bake_texture(mesh.albedo_data);
			return BakedMesh(baked_geom, baked_albedo);
		});
		return baked_meshes;
	}
}