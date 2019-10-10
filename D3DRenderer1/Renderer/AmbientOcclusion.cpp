#include "AmbientOcclusion.h"

bool AmbientOcclusionPass::init(float width, float height, int noAOSamples, int randomTexWidth, int randomTexHeight)
{
	if (!m_AORenderPass.init(width, height, RENDERPASS_TEXTUREBUF, 1, 0))
		return false;
	if (!m_AOFinalRenderPass.init(width, height, RENDERPASS_TEXTUREBUF, 1, 0))
		return false;

	if (!m_AOVertexShader.init(R"(Shaders\Deferred\SSAO\vertexShader.hlsl)"))
		return false;
	if (!m_AOPixelShader.init(R"(Shaders\Deferred\SSAO\pixelShader.hlsl)"))
		return false;
	if (!m_AOBlurVertexShader.init(R"(Shaders\Deferred\SSAO\blurVertexShader.hlsl)"))
		return false;
	if (!m_AOBlurPixelShader.init(R"(Shaders\Deferred\SSAO\blurPixelShader.hlsl)"))
		return false;

	std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
	std::default_random_engine generator;
	for (unsigned int i = 0; i < noAOSamples; i++)
	{
		XMFLOAT4 sample;
		sample.x = randomFloats(generator) * 2.0 - 1.0;
		sample.y = randomFloats(generator) * 2.0 - 1.0;
		sample.z = randomFloats(generator);
		sample.w = 1.0f;

		XMVECTOR temp = XMLoadFloat4(&sample);
		temp = XMVector4Normalize(temp);

		XMStoreFloat4(&sample, temp);

		float rand = randomFloats(generator);
		sample.x *= rand;
		sample.y *= rand;
		sample.z *= rand;

		float scale = (float)i / (float)noAOSamples;
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample.x *= scale;
		sample.y *= scale;
		sample.z *= scale;
		m_ssaoKernels.push_back(sample);

	}

	if (!m_KernelConstBuffer.init(&m_ssaoKernels[0], m_ssaoKernels.size() * sizeof(XMFLOAT4)))
		return false;

	std::vector<float> ssaoNoise;
	for (unsigned int i = 0; i < (randomTexWidth*randomTexHeight)*4; i++)
	{
		ssaoNoise.push_back(randomFloats(generator) * 2.0f - 1.0f);
		ssaoNoise.push_back(randomFloats(generator) * 2.0f - 1.0f);
		ssaoNoise.push_back(0.0f);
		ssaoNoise.push_back(0.0f);
		
	}

	D3D11_TEXTURE2D_DESC noiseTexDesc = {};
	noiseTexDesc.ArraySize = 1;
	noiseTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	noiseTexDesc.CPUAccessFlags = 0;
	noiseTexDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	noiseTexDesc.Width = randomTexWidth;
	noiseTexDesc.Height = randomTexHeight;
	noiseTexDesc.MipLevels = 1;
	noiseTexDesc.SampleDesc.Count = 1;
	noiseTexDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA texData = {};
	texData.pSysMem = &ssaoNoise[0];
	texData.SysMemPitch = randomTexWidth * 4;

	HRESULT result = D3DContext::getCurrent()->getDevice()->CreateTexture2D(&noiseTexDesc, &texData, &noiseTexture);
	if (FAILED(result))
	{
		std::cout << "Failed to create noise texture.. " << std::endl;
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC noiseTexView = {};
	noiseTexView.Format = noiseTexDesc.Format;
	noiseTexView.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	noiseTexView.Texture2D.MipLevels = 1;

	result = D3DContext::getCurrent()->getDevice()->CreateShaderResourceView(noiseTexture, &noiseTexView, &noiseTextureView);
	if (FAILED(result))
	{
		std::cout << "Failed to create noise SRV.. " << std::endl;
		return false;
	}

	D3D11_SAMPLER_DESC noiseSamplerViewDesc = {};
	noiseSamplerViewDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	noiseSamplerViewDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	noiseSamplerViewDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	noiseSamplerViewDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	
	result = D3DContext::getCurrent()->getDevice()->CreateSamplerState(&noiseSamplerViewDesc, &noiseSamplerState);
	if (FAILED(result))
	{
		std::cout << "Failed to create noise sampler state.." << std::endl;
		return false;
	}

	if (!m_fsQuad.init())
		return false;


	return true;
}

void AmbientOcclusionPass::destroy()
{
	//todo
}

void AmbientOcclusionPass::begin()
{
	m_AORenderPass.begin(1, 1, 1, 0);
	m_AOVertexShader.bind();
	m_AOPixelShader.bind();
}

void AmbientOcclusionPass::renderAO()
{

	m_KernelConstBuffer.uploadToPixelShader(2);	//Hardcoded for now just so it can work
	D3DContext::getCurrent()->getDeviceContext()->PSSetSamplers(2, 1, &noiseSamplerState);
	D3DContext::getCurrent()->getDeviceContext()->PSSetShaderResources(5, 1, &noiseTextureView);
	m_fsQuad.draw();

	m_AOFinalRenderPass.begin(1, 1, 1, 0);
	m_AOBlurVertexShader.bind();
	m_AOBlurPixelShader.bind();
	m_AORenderPass.bindRenderTargetSRV(0, 0);
	m_fsQuad.draw();
	m_AORenderPass.unbindRenderTargetSRV(0);
}

void AmbientOcclusionPass::bindAOTexture(int samplerStateBinding, int textureBindingPoint)
{
	m_AOFinalRenderPass.bindRenderTargetSRV(textureBindingPoint,samplerStateBinding);
}

void AmbientOcclusionPass::unbindAOTexture(int textureBindingPoint)
{
	m_AOFinalRenderPass.unbindRenderTargetSRV(textureBindingPoint);
}

ID3D11ShaderResourceView* AmbientOcclusionPass::getAOTexture()
{
	return m_AOFinalRenderPass.getRenderBufferView();
}