#pragma once
#include <exception>
#include <cstdio>
namespace boost {
	void throw_exception( std::exception const & e ) {
		printf("exception %s", e.what());
	}
}
#include "stbi_image.h"

#include <GL/glew.h>
#include <memory>
#include <utility>
#include <map>
#include <array>
#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <boost/optional.hpp>
#include <GLFW/glfw3.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/optional.hpp>

#include <boost/filesystem.hpp>

#include <d3d11.h>
#include <atlbase.h>

using namespace glm;
using namespace std;
using namespace boost;



