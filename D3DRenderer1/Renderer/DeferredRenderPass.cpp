#include "RenderPass.h"

bool DeferredRenderPass::init(int width, int height, int noRenderTargets, int MSAALevels, int MSAAQuality)
{
	return true;
}

void DeferredRenderPass::destroy()
{

}

void DeferredRenderPass::begin(float r, float g, float b)
{

}

void DeferredRenderPass::bindRenderTargets(int startLocation)
{

}

void DeferredRenderPass::unbindRenderTargets(int startLocation)
{

}

std::vector<ID3D11ShaderResourceView*> DeferredRenderPass::getRenderBufferViews()
{
	return m_renderBufferViews;
}

ID3D11ShaderResourceView* DeferredRenderPass::getDepthBufferView()
{
	return m_depthBufferView;
}