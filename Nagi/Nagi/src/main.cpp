#include <pch.h>
#include "GraphicsContext.h"
#include "Window.h"

// Helpers
static std::vector<char> readFile(const std::string& filePath)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	if (!file.is_open())
		assert(false);	// failed to open file

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);  // go back to the beginning
	file.read(buffer.data(), fileSize);     // read "filSize" from file, put into buffer.data()

	file.close();

	return buffer;
}

int main()
{
	Nagi::Window win(1920, 1080);
	Nagi::GraphicsContext gfxCon(win);
	auto dev = gfxCon.getDevice();

	try
	{
		// ========================== Create Render Pass
		std::array<vk::AttachmentDescription, 2> attachmentDescs;

		// Color
		attachmentDescs[0] = vk::AttachmentDescription({},
			gfxCon.getSwapchainImageFormat(),		// format
			vk::SampleCountFlagBits::e1,			// sample
			vk::AttachmentLoadOp::eClear,			// attachment load/store op
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,		// stencil load/store op
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,			// enter renderpass in this layout
			vk::ImageLayout::ePresentSrcKHR			// exit renderpass in this layout
		);

		// Depth
		attachmentDescs[1] = vk::AttachmentDescription({},
			vk::Format::eD32Sfloat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal
		);

		// Reference to the attachment descriptions
		vk::AttachmentReference colorRef(0, vk::ImageLayout::eAttachmentOptimalKHR);				// 2nd arg -> layout to use during in subpass using this ref
		vk::AttachmentReference depthRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

		vk::SubpassDescription subpassDesc({}, vk::PipelineBindPoint::eGraphics, {}, colorRef, {}, &depthRef);

		//vk::SubpassDependency extInDep(
		//	VK_SUBPASS_EXTERNAL,
		//	0,
		//	// wait for prev until depth testing of prev frame finished (write on srcStages) before starting depth test on this frame 
		//	vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
		//	vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
		//	vk::AccessFlagBits::eDepthStencilAttachmentWrite,	// why only write? (stated on sync examples Khronos Group)
		//	vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead
		//);
		
		// Corrected (sync validation WAW hazard)
		vk::SubpassDependency extInDep(
			VK_SUBPASS_EXTERNAL,
			0,
			// We have early AND late because the accessFlags only applies to explicitly specified stages (the mem dep does NOT apply to the implicit stages that come with exec dep)
			vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests |	// wait for prev frame depth testing (happens in early and late frag test)
			vk::PipelineStageFlagBits::eColorAttachmentOutput,													// form exec dep chain with present sem

			vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests |	// halt this frames depth testing before the prev is done
			vk::PipelineStageFlagBits::eColorAttachmentOutput,													// form exec dep chain with present sem (note that we have mem dep for this (dst) stage!)

			{},	// Sync from previous guaranteed by the semaphore (?? I remember something like this from krOoze..)
			// Semaphore guarantees something memory..
			vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead | // assign mem dep on the depth resource for which to halt
			vk::AccessFlagBits::eColorAttachmentWrite							// ensure that the layout transition happens after color output is unblocked but before color output writing through mem dep!
		);


		//// Not caring about depth
		//vk::SubpassDependency extInDep(
		//	VK_SUBPASS_EXTERNAL,
		//	0,
		//	vk::PipelineStageFlagBits::eColorAttachmentOutput,		
		//	vk::PipelineStageFlagBits::eColorAttachmentOutput,									
		//	{},		
		//	vk::AccessFlagBits::eColorAttachmentWrite
		//);
		
		// Using implicit external subpass

		auto rendPass = dev.createRenderPassUnique(vk::RenderPassCreateInfo({}, attachmentDescs, subpassDesc, extInDep));


		// =========================== Create Graphics Pipeline
		auto vertBin = readFile("compiled_shaders/vs.spv");
		auto fragBin = readFile("compiled_shaders/ps.spv");
		auto vertMod = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, vertBin.size(), reinterpret_cast<uint32_t*>(vertBin.data())));
		auto fragMod = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, fragBin.size(), reinterpret_cast<uint32_t*>(fragBin.data())));
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageC = {
			vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertMod.get(), "main"),
			vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragMod.get(), "main")
		};

		vk::PipelineVertexInputStateCreateInfo vertInC;		// empty for now (no vertices in)

		vk::PipelineInputAssemblyStateCreateInfo iaC({}, vk::PrimitiveTopology::eTriangleList);

		auto scExtent = gfxCon.getSwapchainExtent();
		vk::Viewport vp = vk::Viewport(0.f, 0.f, static_cast<float>(scExtent.width), static_cast<float>(scExtent.height), 0.0, 1.0);
		vk::Rect2D scissor = vk::Rect2D({ 0, 0 }, scExtent);
		vk::PipelineViewportStateCreateInfo vpC({}, 1, &vp, 1, &scissor);	// no stencil

		vk::PipelineRasterizationStateCreateInfo rsC({},
			false,
			false,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eBack,
			vk::FrontFace::eCounterClockwise,	// default
			{}, {}, {}, {},						// depth bias
			1.0f								// line width
		);

		vk::PipelineMultisampleStateCreateInfo msC({},
			vk::SampleCountFlagBits::e1
			// everything else is off (sample shading and sample mask
		);

		vk::PipelineDepthStencilStateCreateInfo dsC({},
			true,
			true,
			vk::CompareOp::eLessOrEqual
			// everything else off (depth bound test args and stencil)
		);

		vk::PipelineColorBlendAttachmentState colorBlendAttachment(
			false,
			// blend op bogus (it is disabled so these values below dont matter)
			vk::BlendFactor::eOne,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eOne,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,

			// specifies which channels to 'let through' (enabled for writing)
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		);

		vk::PipelineColorBlendStateCreateInfo cbC({},
			false,
			{},		// no logic Op (disabled)
			1,
			&colorBlendAttachment,
			{}		// we are not blending so this the blendConstants are irrelevant
		);

		auto pipelineLayout = dev.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo());	// no descriptor sets or push constants yet

		auto gfxPipeline = dev.createGraphicsPipelineUnique({},
			vk::GraphicsPipelineCreateInfo({},
				shaderStageC,
				&vertInC,
				&iaC,
				{},
				&vpC,
				&rsC,
				&msC,
				&dsC,
				&cbC,
				{},
				pipelineLayout.get(),
				rendPass.get(),
				0
			)
		).value;

		// Create framebuffers
		// dep: (1) sc view and (2) depth view
		// dep: (3) swapchain image count
		uint32_t scImageCount = gfxCon.getSwapchainImageCount();
		auto scViews = gfxCon.getSwapchainViews();
		auto depthV = gfxCon.getDepthView();

		std::vector<vk::UniqueFramebuffer> fbs;
		fbs.reserve(scImageCount);
		for (uint32_t i = 0; i < scImageCount; ++i)
		{
			std::array<vk::ImageView, 2> attachments{ scViews[i], depthV };
			vk::FramebufferCreateInfo fbc({}, rendPass.get(), attachments, scExtent.width, scExtent.height, 1);
			fbs.push_back(dev.createFramebufferUnique(fbc));
		}

		while (win.isRunning())
		{
			win.processEvents();

			auto frameRes = 
			gfxCon.beginFrame();
			auto& cmd = frameRes.gfxCmdBuffer;

			// Setup render pass info
			std::array<vk::ClearValue, 2> clearValues = {
				vk::ClearColorValue(std::array<float, 4>({0.f, 0.f, 0.f, 1.f})),
				vk::ClearDepthStencilValue( /*depth*/ 1.f, /*stencil*/ 0)
			};

			vk::RenderPassBeginInfo rpInfo(rendPass.get(), fbs[frameRes.imageIdx].get(), vk::Rect2D({ 0, 0 }, scExtent), clearValues);

			// Record command buffer
			cmd->begin(vk::CommandBufferBeginInfo());

			cmd->beginRenderPass(rpInfo, {});
			cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, gfxPipeline.get());
			cmd->draw(3, 2, 0, 0);
			cmd->endRenderPass();

			cmd->end();

			// Setup submit info
			std::array<vk::PipelineStageFlags, 1> waitStages{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
			vk::SubmitInfo submitInfo(
				frameRes.sync->imageAvailableSemaphore,
				waitStages,
				*cmd,
				frameRes.sync->renderFinishedSemaphore
			);

			gfxCon.submitQueue(submitInfo);

			gfxCon.endFrame();
		}

		// Idle to wait for GPU resources to stop being used before destruction
		dev.waitIdle();

	}
	catch (vk::SystemError& err)
	{
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		assert(false);
	}
	catch (std::exception& err)
	{
		std::cout << "std::exception: " << err.what() << std::endl;
		assert(false);
	}
	catch (...)
	{
		std::cout << "Unknown error\n";
		assert(false);
	}



	return 0;
}