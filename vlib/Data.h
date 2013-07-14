#pragma once
template<typename T>
struct DataBuffer {
	DataBuffer() { } // for serialization
	DataBuffer(const vector<T>& p_data) :
	data(p_data) {
	}
	vector<T> data;
	size_t count() const {
		return data.size();
	}

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & data;
	}
};

struct TextureData {
	TextureData(const vector<unsigned char>& p_data, int p_w, int p_h, int p_byte_per_pixel)
		: data(p_data), w(p_w), h(p_h), byte_per_pixel(p_byte_per_pixel) { }
	TextureData() { }

	vector<unsigned char> data;
	int w, h;
	int byte_per_pixel;	

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & data;
		ar & w;
		ar & h;
		ar & byte_per_pixel;
	}
};
TextureData load_texture(const string& path) {
	int w, h, byte_per_pixel;
	auto data = stbi_load(path.c_str(), &w, &h, &byte_per_pixel, 0);
	assert(data != nullptr);
	TextureData texture_data(vector<unsigned char>((char*)data, (char*)data + w * h * byte_per_pixel), w, h, byte_per_pixel);
	stbi_image_free(data);
	return texture_data;
}

struct Degree {
	Degree(float p_value) : value(p_value) { }
	float value;
};

string read_all(string path) {
	std::ifstream ifs(path);
	return string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

size_t stride(vec3 _) {
	return sizeof(float);
}
size_t stride(mat4 _) {
	return sizeof(float);
}
size_t byte_size(vec3 _) {
	return sizeof(float) * 3;
}
size_t byte_size(mat4 _) {
	return sizeof(float) * 4 * 4;
}

template<typename TKey, typename TValue>
const TValue& get(const map<TKey, TValue>& src, const TKey& key) {

	auto r = src.find(key);
	if(r == src.end()) {
		//for(auto& kv : src) {
		for(auto kv = src.begin(); kv != src.end(); kv++) {
			printf("%s, %d\n", kv->first.name.c_str(), kv->first.type);
		}
		auto r2 = src.find(key);
		printf("%s", r2);
		assert(false);
	}
	return r->second;
}