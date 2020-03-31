#include <iostream>

#define NOMINMAX	//Winapi fucking sucks
#define GLFW_EXPOSE_NATIVE_WIN32
#define STB_IMAGE_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Renderer/BaseRenderer.h"
#include "Renderer/VertexShader.h"
#include "Renderer/PixelShader.h"
#include "Renderer/Object.h"
#include "Renderer/RenderPass.h"
#include "Renderer/Camera.h"
#include "Renderer/Light.h"
#include "Renderer/ConstantBuffer.h"
#include "Renderer/ShadowMapping.h"
#include "Renderer/AmbientOcclusion.h"
#include "Renderer/Voxelization.h"

//ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_dx11.h>


#define WIDTH 1600
#define HEIGHT 900
#define FULLSCREEN 0

bool doVsync = false;

int MultisampleLevel = 1;
int MultisampleQuality = 0;

struct FrameFlags
{
	XMFLOAT2 resolution;
	int doFXAA;
	int doSSAO;
	int doSSR;
	int doVoxelReflections;
	int doTexturing;
	float ssaoRadius;
	int m_kernelSize;
	int m_ssaoPower;
	unsigned int coarseStepCount;
	float coarseStepIncrease;
	unsigned int fineStepCount;
	float tolerance;
	float ssrReflectiveness;
	float ssrMetallic;
	//int unusedAlignment[1];
};

int main()
{
	if (!glfwInit())
	{
		std::cout << "Failed to init GLFW. " << std::endl;
		return -1;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	//Don't want an OpenGL context. just a window
	GLFWwindow* m_window = glfwCreateWindow(WIDTH, HEIGHT, "Direct3D", (FULLSCREEN) ? glfwGetPrimaryMonitor() : nullptr, nullptr);
	if (!m_window)
	{
		std::cout << "Failed to create a win32 window." << std::endl;
		return -1;
	}

	HWND hwnd = glfwGetWin32Window(m_window);

	D3D m_context;
	if (!m_context.init(WIDTH, HEIGHT, false,hwnd, 1, MultisampleQuality))
		return -1;

	//Make this d3d context the global current one
	D3DContext::Register(m_context);

	//Setting up ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();	(void)io;

	ImGui::StyleColorsClassic();

	ImGui_ImplGlfw_InitForVulkan(m_window, true);
	ImGui_ImplDX11_Init(D3DContext::getCurrent()->getDevice(), D3DContext::getCurrent()->getDeviceContext());

	//Now intializing direct3d stuff
	VertexShader m_vertexShader;
	if(!m_vertexShader.loadCompiled(R"(CompiledShaders\Deferred\vertexShader.cso)"))
		return -1;
	PixelShader m_pixelShader;
	if (!m_pixelShader.loadCompiled(R"(CompiledShaders\Deferred\pixelShader.cso)"))
		return -1;

	PerspectiveCamera m_camera;
	if (!m_camera.init(WIDTH, HEIGHT, 60.0f, 1.0f, 10000.0f))
		return -1;
	m_camera.cameraChangeInfo.position = XMVectorSet(0, 0, 5.0f, 0);
	m_camera.cameraChangeInfo.lookAt = XMVectorSet(0, 0, -1.0f, 0);
	Object m_object;
	m_object.init(R"(Models\sponza\sponza.obj)");
	m_object.scale(XMVectorSet(0.1, 0.1, 0.1, 0));
	Object m_object2;
	m_object2.init(R"(Models\materialball\export3dcoat.obj)");
	//m_object2.scale(XMVectorSet(0.05, 0.05, 0.05, 0));
	m_object2.rotate(XMVectorSet(0, 1, 0, 0), 90.0f);
	m_object2.translate(XMVectorSet(0.0f, 8.5f, 0.0f, 0.0f));
	Object m_object3;
	m_object3.init(R"(Models\nanosuit\nanosuit.obj)");
	//m_object3.scale(XMVectorSet(0.05, 0.05, 0.05, 0));
	m_object3.rotate(XMVectorSet(0, 1, 0, 0), 90.0f);
	m_object3.translate(XMVectorSet(-15.5f, 0.0f, -0.0f, 0.0f));


	RenderPass m_renderPass;
	if (!m_renderPass.init(WIDTH, HEIGHT,RENDERPASS_SWAPCHAINBUF, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,1,MultisampleQuality))	//if RENDERPASS_SWAPCHAINBUF specified then no rendertargetview, render buffer or shader resource view is created.
		return -1;
	m_renderPass.specifyRenderTarget(D3DContext::getCurrent()->getBackBuffer());	//Change render target to hardware render target

	RenderPass m_deferredResolvePass;
	if (!m_deferredResolvePass.init(WIDTH, HEIGHT, RENDERPASS_TEXTUREBUF, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, MultisampleQuality))
		return -1;

	DXGI_FORMAT formats[5] = { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R32G32B32A32_FLOAT,DXGI_FORMAT_R32G32B32A32_FLOAT};;
	DeferredRenderPass m_deferredRenderPass;
	if (!m_deferredRenderPass.init(WIDTH, HEIGHT, 5, formats, MultisampleLevel,MultisampleQuality))
		return -1;


	DirectionalLight m_basicLight;
	m_basicLight.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);
	m_basicLight.direction = XMFLOAT4(-1000.0f, -5560.0f, 500.0f ,0.0f);
	m_basicLight.specularPower = 128.0f;

	ConstantBuffer m_lightUploadBuffer;
	if (!m_lightUploadBuffer.init(&m_basicLight, sizeof(DirectionalLight)))
		return -1;

	XMFLOAT2 shadowDimensions = XMFLOAT2(410, 410);
	DirectionalShadowMap m_shadowMap;
	if (!m_shadowMap.init(8192,8192, shadowDimensions.x,shadowDimensions.y, m_basicLight))
		return -1;

	
	//Deferred quad stuff for testing
	FullscreenQuad m_fsQuad;
	if (!m_fsQuad.init())
		return -1;
	VertexShader m_qVertexShader;
	if (!m_qVertexShader.loadCompiled(R"(CompiledShaders\DeferredQuad\vertexShader.cso)"))
		return -1;
	PixelShader m_qPixelShader;
	if (!m_qPixelShader.loadCompiled(R"(CompiledShaders\DeferredQuad\pixelShader.cso)"))
		return -1;

	VertexShader m_fxaaVertexShader;
	if (!m_fxaaVertexShader.loadCompiled(R"(CompiledShaders\DeferredQuad\FXAA\vertexShader.cso)"))
		return -1;
	PixelShader m_fxaaPixelShader;
	if (!m_fxaaPixelShader.loadCompiled(R"(CompiledShaders\DeferredQuad\FXAA\pixelShader.cso)"))
		return -1;

	FrameFlags m_frameFlags = {};
	m_frameFlags.resolution = XMFLOAT2(1600, 900);
	m_frameFlags.doFXAA = 1;
	m_frameFlags.doSSAO = 1;
	m_frameFlags.doSSR = 0;
	m_frameFlags.doVoxelReflections = 1;
	m_frameFlags.doTexturing = 1;
	m_frameFlags.ssaoRadius = 10.0f;
	m_frameFlags.m_kernelSize = 64;
	m_frameFlags.m_ssaoPower = 1;
	m_frameFlags.coarseStepCount = 32;
	m_frameFlags.fineStepCount = 32;
	m_frameFlags.coarseStepIncrease = 1.125f;
	m_frameFlags.tolerance = 999.0f;
	m_frameFlags.ssrMetallic = 1.0f;
	m_frameFlags.ssrReflectiveness = 1.0f;
	ConstantBuffer m_flagsBuffer;
	if (!m_flagsBuffer.init(&m_frameFlags, sizeof(FrameFlags)))
		return -1;

	AmbientOcclusionPass m_ambientOcclusionPass;
	m_ambientOcclusionPass.init(WIDTH, HEIGHT,m_frameFlags.m_kernelSize,4,4);

	//SSR
	VertexShader m_ssrVertexShader;
	if (!m_ssrVertexShader.loadCompiled(R"(CompiledShaders\DeferredQuad\SSR\vertexShader.cso)"))
		return -1;
	PixelShader m_ssrPixelShader;
	if (!m_ssrPixelShader.loadCompiled(R"(CompiledShaders\DeferredQuad\SSR\pixelShader.cso)"))
		return -1;
	
	RenderPass m_ssrRenderPass;
	if (!m_ssrRenderPass.init(WIDTH, HEIGHT, RENDERPASS_TEXTUREBUF, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 0))
		return -1;

	SceneVoxelizer m_voxelizer;
	m_voxelizer.init();

	//For camera
	float pitch = 0, yaw = 0;
	XMFLOAT4 cameraFront;	//DirectXMath sucks.

	//Calculating deltatime
	float oldTime = 0, newTime=0, deltaTime=1;
	float cameraSpeed = 25.0f;

	//These are set before because binds per-frame is TERRIBLE for performance.
	m_lightUploadBuffer.uploadToPixelShader(0);
	m_shadowMap.bindShadowCamera(2);
	m_flagsBuffer.uploadToPixelShader(1);
	bool voxelized = false;
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Renderer");
		{
			XMFLOAT4 temp;
			XMStoreFloat4(&temp, m_camera.cameraChangeInfo.position);
			ImGui::Text("Position: X %.2f Y %.2f Z %.2f",temp.x,temp.y,temp.z);
			ImGui::Checkbox("FXAA Enable", (bool*)&m_frameFlags.doFXAA);
			ImGui::Checkbox("SSAO Enable", (bool*)&m_frameFlags.doSSAO);
			ImGui::Checkbox("SSR Enable", (bool*)&m_frameFlags.doSSR);
			ImGui::Checkbox("Voxel Reflections Enable", (bool*)&m_frameFlags.doVoxelReflections);
			ImGui::Checkbox("Textures", (bool*)&m_frameFlags.doTexturing);
			ImGui::DragFloat("SSAO Radius", (float*)&m_frameFlags.ssaoRadius);
			ImGui::DragInt("SSAO Kernel Size", (int*)&m_frameFlags.m_kernelSize);
			ImGui::DragInt("SSAO Power", (int*)&m_frameFlags.m_ssaoPower);
			if (ImGui::RadioButton("Update SSAO Kernels", true))
				m_ambientOcclusionPass.updateKernel(m_frameFlags.m_kernelSize);
			ImGui::DragFloat2("Shadow Dimensions", (float*)&shadowDimensions);
			ImGui::Image((ImTextureID)m_shadowMap.getDepthBufferView(), ImVec2(256, 256));
		}
		ImGui::End();

		ImGui::Begin("Light Properties");
		{
			ImGui::DragFloat3("Light LookAt", (float*)&m_basicLight.direction);
			ImGui::ColorPicker3("Light Color", (float*)&m_basicLight.color);
			ImGui::DragFloat("Specular Power", &m_basicLight.specularPower);
		}
		ImGui::End();

		ImGui::Begin("SSR Properties");
		{
			ImGui::DragInt("Coarse Step Count", (int*)&m_frameFlags.coarseStepCount);
			ImGui::DragInt("Fine Step Count", (int*)&m_frameFlags.fineStepCount);
			ImGui::DragFloat("Coarse Step Increase", (float*)&m_frameFlags.coarseStepIncrease);
			ImGui::DragFloat("Tolerance", (float*)&m_frameFlags.tolerance);
			ImGui::DragFloat("Reflectiveness", (float*)&m_frameFlags.ssrReflectiveness,0.005f,0.0f,1.0f);
			ImGui::DragFloat("Metallic factor", (float*)&m_frameFlags.ssrMetallic,0.005f,0.0f,1.0f);
		}
		ImGui::End();

		ImGui::Begin("Deferred Renderer Debug");
		{
			ImGui::Image((ImTextureID)m_deferredRenderPass.getDepthBufferView(), ImVec2(320, 180));
			for (int i = 0; i < m_deferredRenderPass.m_noRenderTargets; i++)
				ImGui::Image((ImTextureID)m_deferredRenderPass.getRenderBufferViews()[i], ImVec2(320, 180));
			ImGui::Image((ImTextureID)m_ambientOcclusionPass.getAOTexture(), ImVec2(320, 180));
			ImGui::Image((ImTextureID)m_ssrRenderPass.getRenderBufferView(), ImVec2(320, 180));
		}
		ImGui::End();
		//CPU Updating
		m_lightUploadBuffer.update((void*)&m_basicLight, sizeof(DirectionalLight));
		m_flagsBuffer.update((void*)&m_frameFlags, sizeof(FrameFlags));
		m_camera.update();

		m_shadowMap.beginFrame(m_basicLight,shadowDimensions);
		m_object.draw();
		m_object2.draw();
		m_object3.draw();
		m_shadowMap.endFrame();

		m_camera.bind(0);

		if (!voxelized)
		{
			m_voxelizer.beginVoxelizationPass();
			m_object.draw();
			m_object2.draw();
			m_object3.draw();
			m_voxelizer.endVoxelizationPass();
			voxelized=true;
		}

		D3DContext::setViewport(WIDTH, HEIGHT);

		m_deferredRenderPass.begin(0.564f, 0.8f, 0.976f, 0.0f);
		m_vertexShader.bind();
		m_pixelShader.bind();

		//GPU Drawing
		m_object.draw();
		m_object2.draw();
		m_object3.draw();

		//SSAO Pass just after GPU drawing
		if (m_frameFlags.doSSAO)
		{
			m_ambientOcclusionPass.begin();
			m_deferredRenderPass.bindRenderTargets(0, 0);
			m_deferredRenderPass.bindDepthStencilTarget(6, 0);
			m_ambientOcclusionPass.renderAO();
			m_deferredRenderPass.unbindDepthStencilTarget(6);
		}

		//Drawing quad to combine deferred targets and get result

		m_deferredResolvePass.begin(0.564f,0.8f,0.976f, 1.0f);
		m_qVertexShader.bind();
		m_qPixelShader.bind();
		if(m_frameFlags.doSSAO)
			m_ambientOcclusionPass.bindAOTexture(0, 5);
		m_voxelizer.bindVoxelTexture(6, 2);
		m_deferredRenderPass.bindRenderTargets(0, 0);
		m_shadowMap.bindDepthTexturePS(1, 4);
		m_fsQuad.draw();
		m_shadowMap.unbindDepthTexturePS(4);
		m_deferredRenderPass.unbindRenderTargets(0);	//Necessary to stop undefined behaviour
		m_ambientOcclusionPass.unbindAOTexture(5);
		m_voxelizer.unbindVoxelTexture(6);


		//Doing screenspace reflections!!!
		m_ssrRenderPass.begin(1, 1, 1, 1);
		m_ssrVertexShader.bind();
		m_ssrPixelShader.bind();
		m_deferredRenderPass.bindRenderTargets(0, 0);
		m_deferredRenderPass.bindDepthStencilTarget(6, 0);
		m_deferredResolvePass.bindRenderTargetSRV(5, 0);
		m_fsQuad.draw();
		m_deferredRenderPass.unbindRenderTargets(0);
		m_deferredRenderPass.unbindDepthStencilTarget(6);
		m_deferredResolvePass.unbindRenderTargetSRV(5);

		//Drawing another quad for FXAA Renderpass

		m_renderPass.begin(1.0f, 1.0f, 1.0f, 1.0f);
		m_fxaaVertexShader.bind();
		m_fxaaPixelShader.bind();

		//m_deferredResolvePass.bindRenderTargetSRV(0, 0);
		m_ssrRenderPass.bindRenderTargetSRV(0, 0);
		m_fsQuad.draw();
		m_deferredResolvePass.unbindRenderTargetSRV(0);
		m_ssrRenderPass.unbindRenderTargetSRV(0);

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


		if (!D3DContext::present(doVsync))
			glfwSetWindowShouldClose(m_window, GLFW_TRUE);	//quit if device removed otherwise everything breaks 

		//Set forward vector for camera.
		cameraFront.x = XMScalarCos(XMConvertToRadians(pitch)) * XMScalarCos(XMConvertToRadians(yaw));
		cameraFront.y = XMScalarSin(XMConvertToRadians(pitch));
		cameraFront.z = XMScalarCos(XMConvertToRadians(pitch)) * XMScalarSin(XMConvertToRadians(yaw));
		m_camera.cameraChangeInfo.lookAt = XMLoadFloat4(&cameraFront);	//Store from accessible float4 to vector
		XMVector3Normalize(m_camera.cameraChangeInfo.lookAt);	//Normalize length

		//Input Handling
		if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
			m_camera.cameraChangeInfo.position += m_camera.cameraChangeInfo.lookAt * deltaTime * cameraSpeed;
		if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
			m_camera.cameraChangeInfo.position -= m_camera.cameraChangeInfo.lookAt * deltaTime * cameraSpeed;
		if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
			m_camera.cameraChangeInfo.position += XMVector3Normalize(XMVector3Cross(m_camera.cameraChangeInfo.lookAt, m_camera.cameraChangeInfo.up)) * deltaTime * cameraSpeed;
		if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
			m_camera.cameraChangeInfo.position -= XMVector3Normalize(XMVector3Cross(m_camera.cameraChangeInfo.lookAt, m_camera.cameraChangeInfo.up)) * deltaTime * cameraSpeed;
		if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS)
			m_camera.cameraChangeInfo.position += m_camera.cameraChangeInfo.up * deltaTime * cameraSpeed;
		if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			m_camera.cameraChangeInfo.position -= m_camera.cameraChangeInfo.up * deltaTime * cameraSpeed;
		if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
			yaw += 0.5f * deltaTime * 250.0f;
		if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
			yaw -= 0.5f * deltaTime * 250.0f;
		if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
			pitch += 0.5f * deltaTime * 250.0f;
		if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
			pitch -= 0.5f * deltaTime * 250.0f;
		deltaTime = newTime - oldTime;
		oldTime = newTime;
		newTime = glfwGetTime();

		std::string wTitle = std::string("FPS: ") + std::to_string(1.0f / deltaTime);
		glfwSetWindowTitle(m_window, wTitle.c_str());
	}
	glfwTerminate();
	return 0;
}
