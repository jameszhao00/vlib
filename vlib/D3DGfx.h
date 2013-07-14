#pragma once
#include "Gfx.h"
#include <glm/glm.hpp>
#include <vector>
#include "D3DGfx.h"
#include <d3d11.h>
#include <atlbase.h>
#include "data.h"
using namespace glm;
using namespace Gfx;
using namespace std;

namespace D3D11 {

	template<typename TDerived>
	class IState {
		int distance(const TDerived&) const = 0;
	};
	class RasterState : IState<RasterState> {
	};
	class BlendState : IState<BlendState> {
	};
	class DepthState : IState<DepthState> {
	};
	class ShaderState : IState<ShaderState> {
	};
	class GeometryState : IState<GeometryState> {
	};
	class RenderTargetState : IState<RenderTargetState> {
		vector<CComPtr<
	};
	class ViewportState : IState<ViewportState> {
	};
	class State : IState<State> {
		RasterState& raster;
		BlendState& blend;
		DepthState& depth;
		ShaderState& shader;
		GeometryState& geom;
		ViewportState& viewport;
		RenderTargetState& render_target;
	};
	class Texture {
	public:
		Texture(ivec2 size,
			CComPtr<ID3D11Texture2D> texture, 
			CComPtr<ID3D11ShaderResourceView> srv, 
			CComPtr<ID3D11RenderTargetView> rtv,
			CComPtr<ID3D11DepthStencilView> dsv) 
			: size_(size), rtv_(rtv), srv_(srv), dsv_(dsv) {
		}
		ivec2 size_;
		CComPtr<ID3D11RenderTargetView> rtv_;
		CComPtr<ID3D11ShaderResourceView> srv_;
		CComPtr<ID3D11DepthStencilView> dsv_;
		CComPtr<ID3D11Texture2D> texture_;
	};
	class Device {
	public:
		void initialize(HWND window, int w, int h) {
			D3D_FEATURE_LEVEL requested = D3D_FEATURE_LEVEL_11_0;

			DXGI_SWAP_CHAIN_DESC swap_chain_desc;
			memset(&swap_chain_desc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));
			swap_chain_desc.BufferCount = 1;
			swap_chain_desc.BufferDesc.Width = w;
			swap_chain_desc.BufferDesc.Height = h;
			swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swap_chain_desc.BufferDesc.RefreshRate.Numerator = 0;
			swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
			swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swap_chain_desc.OutputWindow = window;
			swap_chain_desc.SampleDesc.Count = 1;
			swap_chain_desc.SampleDesc.Quality = 0;
			swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

			D3D11CreateDeviceAndSwapChain(
				nullptr,
				D3D_DRIVER_TYPE_HARDWARE,
				nullptr,
				D3D11_CREATE_DEVICE_DEBUG,
				&requested,
				1,
				D3D11_SDK_VERSION,
				&swap_chain_desc,
				&swap_chain_,
				&device_,
				nullptr,
				&immediate_context_);
			reset_backbuffer();
		}
		void draw(const State& state) {
			
		}
		unique_ptr<Texture> create_texture(ivec2 size, TexturePurpose purpose) {
			if(purpose == Color || purpose == Normal) {
				D3D11_TEXTURE2D_DESC tex_desc;

				tex_desc.Width     = size.x;
				tex_desc.Height    = size.y;
				tex_desc.MipLevels = 1;
				tex_desc.ArraySize = 1;
				tex_desc.Format    = DXGI_FORMAT_R16G16B16A16_FLOAT;
				tex_desc.SampleDesc.Count   = 1;  
				tex_desc.SampleDesc.Quality = 0;  
				tex_desc.Usage          = D3D11_USAGE_DEFAULT;
				tex_desc.BindFlags      = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
				tex_desc.CPUAccessFlags = 0;
				tex_desc.MiscFlags      = 0;

				CComPtr<ID3D11Texture2D> tex;
				device_->CreateTexture2D(&tex_desc, nullptr, &tex);
				return unique_ptr<Texture>(new Texture(size, tex, make_color_srv(*tex), make_rtv(*tex), nullptr));
			} else if(purpose == Depth) {
				D3D11_TEXTURE2D_DESC tex_desc;

				tex_desc.Width     = size.x;
				tex_desc.Height    = size.y;
				tex_desc.MipLevels = 1;
				tex_desc.ArraySize = 1;
				tex_desc.Format    = DXGI_FORMAT_R32_TYPELESS;
				tex_desc.SampleDesc.Count   = 1;  
				tex_desc.SampleDesc.Quality = 0;  
				tex_desc.Usage          = D3D11_USAGE_DEFAULT;
				tex_desc.BindFlags      = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
				tex_desc.CPUAccessFlags = 0; 
				tex_desc.MiscFlags      = 0;

				CComPtr<ID3D11Texture2D> tex;
				device_->CreateTexture2D(&tex_desc, nullptr, &tex);
				return unique_ptr<Texture>(new Texture(size, tex, make_depth_srv(size, *tex), nullptr, make_dsv(*tex)));
			}
		}
		unique_ptr<Texture> cache_texture(TextureData& tex_data) {
			D3D11_TEXTURE2D_DESC tex_desc;

			tex_desc.Width     = tex_data.w;
			tex_desc.Height    = tex_data.h;
			tex_desc.MipLevels = 1;
			tex_desc.ArraySize = 1;
			tex_desc.Format    = DXGI_FORMAT_R16G16B16A16_FLOAT;
			tex_desc.SampleDesc.Count   = 1;  
			tex_desc.SampleDesc.Quality = 0;  
			tex_desc.Usage          = D3D11_USAGE_DEFAULT;
			tex_desc.BindFlags      = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			tex_desc.CPUAccessFlags = 0;
			tex_desc.MiscFlags      = 0;

			D3D11_SUBRESOURCE_DATA subresc;			
			memset(&subresc, sizeof(subresc), 0);
			subresc.pSysMem = tex_data.data.data();

			CComPtr<ID3D11Texture2D> tex;
			device_->CreateTexture2D(&tex_desc, &subresc, &tex);
			return unique_ptr<Texture>(new Texture(ivec2(tex_data.w, tex_data.h),
				tex, make_color_srv(*tex), make_rtv(*tex), nullptr));
		}
		unique_ptr<RenderTargetState> create_render_target_state(const vector<Texture&>) {

		}
		unique_ptr<GeometryState> cache_geometry() {

		}
		void cache_mesh() {

		}
		void present() {
			swap_chain_->Present(0, 0);
		}
	private:
		void apply(IBlendState& state) {

		}
		void apply(IRasterState& state) {

		}
		void apply(IDepthState& state) {

		}
		void apply(IShaderState& state) {

		}
		void apply(IGeometryState& state) {

		}
		void apply(IRenderTargetState& state) {

		}
		CComPtr<ID3D11ShaderResourceView> make_color_srv(ID3D11Texture2D& tex) {
			ID3D11ShaderResourceView* srv;
			device_->CreateShaderResourceView(&tex, nullptr, &srv);
			return srv;
		}
		CComPtr<ID3D11ShaderResourceView> make_depth_srv(ivec2 size, ID3D11Texture2D& tex) {
			ID3D11ShaderResourceView* srv;
			D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

			srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
			srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srv_desc.Texture2D.MostDetailedMip = 0;
			srv_desc.Texture2D.MipLevels = 1;

			device_->CreateShaderResourceView(&tex, &srv_desc, &srv);
			return srv;
		}
		CComPtr<ID3D11RenderTargetView> make_rtv(ID3D11Texture2D& tex) {
			ID3D11RenderTargetView* rtv;
			device_->CreateRenderTargetView(&tex, nullptr, &rtv);
			return rtv;
		}
		CComPtr<ID3D11DepthStencilView> make_dsv(ID3D11Texture2D& tex) {
			ID3D11DepthStencilView* dsv;
			device_->CreateDepthStencilView(&tex, nullptr, &dsv);
			return dsv;
		}
		void reset_backbuffer() {
			ID3D11Texture2D* back_buffer_tex;
			swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&back_buffer_tex);
			D3D11_TEXTURE2D_DESC tex_desc;
			back_buffer_tex->GetDesc(&tex_desc);
			backbuffer_ = Texture(ivec2(tex_desc.Width, tex_desc.Height), 
				back_buffer_tex,
				make_color_srv(*back_buffer_tex),
				nullptr,
				nullptr);
		}
	private:
		Texture backbuffer_;
		CComPtr<ID3D11Device> device_;
		CComPtr<IDXGISwapChain> swap_chain_;
		CComPtr<ID3D11DeviceContext> immediate_context_;
	};
}