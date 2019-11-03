#include "BaseRenderer.h"

bool D3D::init(int width, int height, bool fullscreen, HWND hwnd, int MSAALevels, int& MSAAQuality)
{
	HRESULT result;	//Stores result for each operation

	//Create a factory so separate swapchain can be created with CreateSwapChain
	IDXGIFactory* factory;
	CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);

	//Create D3D Device
	D3D_FEATURE_LEVEL featureLevels[1] = { D3D_FEATURE_LEVEL_12_0 };
	result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, featureLevels, 1, D3D11_SDK_VERSION, &m_device, nullptr, &m_deviceContext);
	if (FAILED(result))
	{
		std::cout << "Failed to create D3D11 Device. HRESULT " << result << std::endl;
		return false;
	}

	//Create d3d 11.3 device
	m_device->QueryInterface(__uuidof(ID3D11Device3), (void**)&m_d3d113device);
	m_deviceContext->QueryInterface(__uuidof(ID3D11DeviceContext3), (void**)&m_d3d113deviceContext);
	UINT quality;
	m_device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, MSAALevels, &quality);
	MSAAQuality = quality - 1;

	//Swapchain desc telling d3d it renders to the given HWND
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.SampleDesc.Count = MSAALevels;
	swapChainDesc.SampleDesc.Quality = MSAAQuality;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Windowed = !fullscreen;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	//Could use IDXGIFactory2::CreateSwapChainForHwnd but then fuckery is required to get MSAA
	result = factory->CreateSwapChain(m_device, &swapChainDesc, &m_swapChain);
	if (FAILED(result))
	{
		std::cout << "Failed to create DXGI Swapchain. HRESULT " << result << std::endl;
		return false;
	}
 
	//Get reference to the swapchain's backbuffer so a render target can be created from it
	ID3D11Texture2D* backBufferRef;
	m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferRef);

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = swapChainDesc.BufferDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;

	result = m_device->CreateRenderTargetView(backBufferRef, &rtvDesc, &m_backBuffer);
	//Before checking result, release the dangling ref pointer otherwise d3d screams at us
	backBufferRef->Release();
	if (FAILED(result))
	{
		std::cout << "Failed to create backbuffer RTV. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_TEXTURE2D_DESC dsDesc = {};
	dsDesc.ArraySize = 1;
	dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dsDesc.Width = width;
	dsDesc.Height = height;
	dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsDesc.CPUAccessFlags = 0;
	dsDesc.MipLevels = 1;
	dsDesc.SampleDesc.Count = MSAALevels;
	dsDesc.SampleDesc.Quality = MSAAQuality;
	dsDesc.Usage = D3D11_USAGE_DEFAULT;

	result = m_device->CreateTexture2D(&dsDesc, nullptr, &m_depthStencilTex);
	if (FAILED(result))
	{
		std::cout << "Failed to create depth-stencil texture for view. your card possibly doesn't support D24_UNORM_S8_UINT. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = dsDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	//dsvDesc.Texture2D.MipSlice = 0;

	result = m_device->CreateDepthStencilView(m_depthStencilTex, &dsvDesc, &m_depthBuffer);
	if (FAILED(result))
	{
		std::cout << "Failed to create default depth stencil view. HRESULT " << result << std::endl;
		return false;
	}

	m_viewport = {};
	m_viewport.Width = width;
	m_viewport.Height = height;
	m_viewport.MinDepth = 0;
	m_viewport.MaxDepth = 1;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_deviceContext->RSSetViewports(1, &m_viewport);

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.AntialiasedLineEnable = FALSE;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;	//Chosen based on 24 bit depth target.
	rasterDesc.DepthBiasClamp = 0;
	rasterDesc.DepthClipEnable = TRUE;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.MultisampleEnable = TRUE;
	rasterDesc.ScissorEnable = FALSE;
	rasterDesc.SlopeScaledDepthBias = 0;

	m_device->CreateRasterizerState(&rasterDesc, &m_rasterizerState);
	m_deviceContext->RSSetState(m_rasterizerState);
	
	D3D11_BLEND_DESC m_blendStateDesc = {};
	m_blendStateDesc.AlphaToCoverageEnable = TRUE;
	m_blendStateDesc.IndependentBlendEnable = TRUE;
	m_blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	m_blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	m_blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	m_blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	m_blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	m_blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	m_blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	m_blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;

	//m_device->CreateBlendState(&m_blendStateDesc, &m_blendState);
	//m_deviceContext->OMSetBlendState(m_blendState, nullptr, 0xffffffff);

	std::cout << "Renderer initialized successfully." << std::endl;

	return true;
}

ID3D11Device3* D3D::getDevice()
{
	return m_d3d113device;
}

ID3D11DeviceContext3* D3D::getDeviceContext()
{
	return m_d3d113deviceContext;
}

ID3D11RenderTargetView* D3D::getBackBuffer()
{
	return m_backBuffer;
}

ID3D11DepthStencilView* D3D::getDepthBuffer()
{
	return m_depthBuffer;
}

IDXGISwapChain* D3D::getSwapChain()
{
	return m_swapChain;
}

ID3D11RasterizerState * D3D::getRasterizerState()
{
	return m_rasterizerState;
}

//D3DCurrent class which holds a static ref to the current d3d instance.

void D3DContext::Register(D3D& d3d)
{
	m_current = &d3d;	//hmm

	//Setting up some global sampler states
	D3D11_SAMPLER_DESC m_samplerStateAnisoFilterDesc = {};
	m_samplerStateAnisoFilterDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	m_samplerStateAnisoFilterDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	m_samplerStateAnisoFilterDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	m_samplerStateAnisoFilterDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	m_samplerStateAnisoFilterDesc.MaxAnisotropy = 8;
	m_samplerStateAnisoFilterDesc.MinLOD = 0;
	m_samplerStateAnisoFilterDesc.MaxLOD = D3D11_FLOAT32_MAX;
	//m_samplerStateAnisoFilterDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	m_current->getDevice()->CreateSamplerState(&m_samplerStateAnisoFilterDesc, &m_samplerStateAnisoFilter);

	D3D11_SAMPLER_DESC m_samplerStateNearestNoMipsDesc = {};
	m_samplerStateNearestNoMipsDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_samplerStateNearestNoMipsDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_samplerStateNearestNoMipsDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_samplerStateNearestNoMipsDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	m_samplerStateNearestNoMipsDesc.MinLOD = 0;
	m_samplerStateNearestNoMipsDesc.MaxLOD = 0;
	//m_samplerStateNearestNoMipsDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	m_current->getDevice()->CreateSamplerState(&m_samplerStateNearestNoMipsDesc, &m_samplerStateNearestNoMips);
}

D3D* D3DContext::getCurrent()
{
	return m_current;
}

void D3DContext::clearDefaultView(float r, float g, float b, float a)
{
	float col[4] = { r,g,b,a };
	ID3D11RenderTargetView* rtv = m_current->getBackBuffer();
	m_current->getDeviceContext()->OMSetRenderTargets(1, &rtv, m_current->getDepthBuffer());
	m_current->getDeviceContext()->ClearRenderTargetView(rtv,col);
	m_current->getDeviceContext()->ClearDepthStencilView(m_current->getDepthBuffer(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

bool D3DContext::present(bool doVsync)
{
	HRESULT result;
	result = m_current->getSwapChain()->Present(doVsync, 0);
	if (result == DXGI_ERROR_DEVICE_REMOVED)
	{
		std::cout << "DXGI present error. Device was removed or reset." << std::endl;
		std::cout << "HRESULT: " << result << std::endl;
		std::cout << "Device removed reason: " << m_current->getDevice()->GetDeviceRemovedReason() << std::endl;
		return false;
	}

	return true;
}

void D3DContext::setViewport(int width, int height)
{
	m_current->m_viewport.Width = width;
	m_current->m_viewport.Height = height;
	m_current->getDeviceContext()->RSSetViewports(1, &m_current->m_viewport);
}

void D3DContext::setDefaultRasterState()
{
	m_current->getDeviceContext()->RSSetState(m_current->getRasterizerState());
}

D3D* D3DContext::m_current = nullptr;	
ID3D11SamplerState* D3DContext::m_samplerStateAnisoFilter = nullptr;
ID3D11SamplerState* D3DContext::m_samplerStateNearestNoMips = nullptr;

