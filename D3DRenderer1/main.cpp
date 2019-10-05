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
	if (!m_vertexShader.init(R"(Shaders\Deferred\vertexShader.hlsl)"))
		return -1;
	PixelShader m_pixelShader;
	if (!m_pixelShader.init(R"(Shaders\Deferred\pixelShader.hlsl)"))
		return -1;

	PerspectiveCamera m_camera;
	if (!m_camera.init(WIDTH, HEIGHT, 60.0f, 1.0f, 10000.0f))
		return -1;
	m_camera.cameraChangeInfo.position = XMVectorSet(0, 0, 5.0f, 0);
	m_camera.cameraChangeInfo.lookAt = XMVectorSet(0, 0, -1.0f, 0);
	Object m_object;
	m_object.init(R"(Models\sponza\sponza.obj)");
	Object m_object2;
	m_object2.init(R"(Models\materialball\export3dcoat.obj)");
	m_object2.scale(XMVectorSet(10, 10, 10, 10));
	m_object2.rotate(XMVectorSet(0, 1, 0, 0), 90.0f);
	m_object2.translate(XMVectorSet(0.0f, 10.5f, 0.0f, 1.0f));
	Object m_object3;
	m_object3.init(R"(Models\nanosuit\nanosuit.obj)");
	m_object3.scale(XMVectorSet(10, 10, 10, 10));
	m_object3.rotate(XMVectorSet(0, 1, 0, 0), 90.0f);
	m_object3.translate(XMVectorSet(0.0f, 0.0f, -15.0f, 1.0f));


	RenderPass m_renderPass;
	if (!m_renderPass.init(WIDTH, HEIGHT,RENDERPASS_SWAPCHAINBUF,1,MultisampleQuality))	//if RENDERPASS_SWAPCHAINBUF specified then no rendertargetview, render buffer or shader resource view is created.
		return -1;
	m_renderPass.specifyRenderTarget(D3DContext::getCurrent()->getBackBuffer());	//Change render target to hardware render target
	DeferredRenderPass m_deferredRenderPass;
	if (!m_deferredRenderPass.init(WIDTH, HEIGHT, 4, MultisampleLevel,MultisampleQuality))
		return -1;


	DirectionalLight m_basicLight;
	m_basicLight.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);
	m_basicLight.direction = XMFLOAT4(-1000.0f, -5560.0f, 500.0f ,0.0f);

	ConstantBuffer m_lightUploadBuffer;
	if (!m_lightUploadBuffer.init(&m_basicLight, sizeof(DirectionalLight)))
		return -1;

	DirectionalShadowMap m_shadowMap;
	if (!m_shadowMap.init(4096,4096, 4096,4096, m_basicLight))
		return -1;

	
	//Deferred quad stuff for testing
	FullscreenQuad m_fsQuad;
	if (!m_fsQuad.init())
		return -1;
	VertexShader m_qVertexShader;
	if (!m_qVertexShader.init(R"(Shaders\DeferredQuad\vertexShader.hlsl)"))
		return -1;
	PixelShader m_qPixelShader;
	if (!m_qPixelShader.init(R"(Shaders\DeferredQuad\pixelShader.hlsl)"))
		return -1;


	//For camera
	float pitch = 0, yaw = 0;
	XMFLOAT4 cameraFront;	//DirectXMath sucks.

	//Calculating deltatime
	float oldTime = 0, newTime=0, deltaTime=1;
	float cameraSpeed = 250.0f;

	//These are set before because binds per-frame is TERRIBLE for performance.
	m_lightUploadBuffer.uploadToPixelShader(0);
	m_shadowMap.bindShadowCamera(2);
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
			ImGui::DragFloat3("Light LookAt", (float*)&m_basicLight.direction);
			ImGui::Text("Position: X %.2f Y %.2f Z %.2f",temp.x,temp.y,temp.z);
			ImGui::Image((ImTextureID)m_shadowMap.getDepthBufferView(), ImVec2(256, 256));
		}
		ImGui::End();

		m_shadowMap.beginFrame(m_basicLight);
		m_object.draw();
		m_object2.draw();
		m_object3.draw();
		m_shadowMap.endFrame();

		D3DContext::setViewport(WIDTH, HEIGHT);
		//Hardware render pass. initial
		//m_renderPass.begin(0.564f, 0.8f, 0.976f);
		m_deferredRenderPass.begin(0.564f, 0.8f, 0.976f);
		m_vertexShader.bind();
		m_pixelShader.bind();
		//CPU Updating
		m_camera.update();
		m_camera.bind(0);
		m_lightUploadBuffer.update((void*)&m_basicLight, sizeof(DirectionalLight));


		//GPU Drawing
		m_object.draw();
		m_object2.draw();
		m_object3.draw();

		m_renderPass.begin(1.0f, 1.0f, 1.0f);
		m_qVertexShader.bind();
		m_qPixelShader.bind();

		m_shadowMap.bindDepthTexturePS(1, 4);
		m_deferredRenderPass.bindRenderTargets(0, 0);
		m_fsQuad.draw();
		m_shadowMap.unbindDepthTexturePS(4);
		m_deferredRenderPass.unbindRenderTargets(0);	//Necessary to stop undefined behaviour

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
