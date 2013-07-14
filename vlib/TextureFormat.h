#pragma once

namespace texformats {
	enum GfxTextureFormat {
		RGBA16F,	
		RGBA_UNORM,
		RGBA_UNORM_SRGB,
		RGB_UNORM
	};
}
enum TexturePurpose {
	Depth,
	Color,
	Normal,
	TextureResource
};