// The Xbox 360's video through vulkan4xenon: see video_vk.h. The device, swapchain, render pass and pipeline are made
// as vulkan4xenon's own tests make them (vulkan4xenon-main/tests/harness.c, harness_scene.c, test_texture.c), which
// passed on the console (run 001).
#include "platform/xenon/video_vk.h"

#include <chrono>
#include <cstdio>
#include <cstring>

#include "core/ppu.h"
#include "platform/xenon/screen_shaders.h"
#include "tiled_index.h"  // (vulkan4xenon-main/tests: libxenon's tiled layout of a front buffer, BSD, in xsdk's notices)
#include "xsdk/display.h"
#include "xsdk/present.h"

namespace {

constexpr uint64_t kTimeoutNs = 1000000000ull;
constexpr uint32_t kFrameBytes = kLineW * kFrameH * 4;

bool ok(const char* call, VkResult r) {
  if (r != VK_SUCCESS) std::printf("video: %s devolvió %d\n", call, (int)r);
  return r == VK_SUCCESS;
}

uint64_t nowUs() {
  return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

void transition(VkCommandBuffer cb, VkImage image, VkImageLayout from, VkImageLayout to) {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  barrier.oldLayout = from;
  barrier.newLayout = to;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);
}

// The quad's corners, two triangles: (x, y, 0, 1), x and y 0 or 1.
const float kQuad[6][4] = {{0, 0, 0, 1}, {1, 0, 0, 1}, {0, 1, 0, 1}, {1, 0, 0, 1}, {1, 1, 0, 1}, {0, 1, 0, 1}};

struct Push {
  float scale[4], offset[4], uv[4];
};

}  // namespace

bool VideoVk::init() {
  const char* instanceExt[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_DISPLAY_EXTENSION_NAME};
  VkApplicationInfo app{};
  app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app.pApplicationName = "Super Mario All-Stars (Xbox 360)";
  app.apiVersion = VK_API_VERSION_1_0;
  VkInstanceCreateInfo ii{};
  ii.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ii.pApplicationInfo = &app;
  ii.enabledExtensionCount = 2;
  ii.ppEnabledExtensionNames = instanceExt;
  if (!ok("vkCreateInstance", vkCreateInstance(&ii, nullptr, &instance_))) return false;
  uint32_t count = 1;
  if (!ok("vkEnumeratePhysicalDevices", vkEnumeratePhysicalDevices(instance_, &count, &physical_))) return false;
  const float priority = 1.0f;
  VkDeviceQueueCreateInfo qi{};
  qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  qi.queueCount = 1;
  qi.pQueuePriorities = &priority;
  const char* deviceExt[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo di{};
  di.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  di.queueCreateInfoCount = 1;
  di.pQueueCreateInfos = &qi;
  di.enabledExtensionCount = 1;
  di.ppEnabledExtensionNames = deviceExt;
  if (!ok("vkCreateDevice", vkCreateDevice(physical_, &di, nullptr, &device_))) return false;
  vkGetDeviceQueue(device_, 0, 0, &queue_);
  VkCommandPoolCreateInfo pi{};
  pi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  if (!ok("vkCreateCommandPool", vkCreateCommandPool(device_, &pi, nullptr, &pool_))) return false;
  return createSwapchain() && createPipeline() && createTexture();
}

bool VideoVk::createSwapchain() {
  // The display's one mode on plane 0, as vulkan4xenon's tests find it (harness.c, create_surface).
  uint32_t count = 1;
  VkDisplayPropertiesKHR display;
  if (!ok("vkGetPhysicalDeviceDisplayPropertiesKHR", vkGetPhysicalDeviceDisplayPropertiesKHR(physical_, &count, &display)))
    return false;
  count = 1;
  VkDisplayModePropertiesKHR mode;
  if (!ok("vkGetDisplayModePropertiesKHR", vkGetDisplayModePropertiesKHR(physical_, display.display, &count, &mode)))
    return false;
  std::printf("video: pantalla %s, %ux%u a %u mHz\n", display.displayName, (unsigned)mode.parameters.visibleRegion.width,
              (unsigned)mode.parameters.visibleRegion.height, (unsigned)mode.parameters.refreshRate);
  VkDisplaySurfaceCreateInfoKHR si{};
  si.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
  si.displayMode = mode.displayMode;
  si.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  si.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
  si.imageExtent = mode.parameters.visibleRegion;
  if (!ok("vkCreateDisplayPlaneSurfaceKHR", vkCreateDisplayPlaneSurfaceKHR(instance_, &si, nullptr, &surface_)))
    return false;
  VkSwapchainCreateInfoKHR sc{};
  sc.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sc.surface = surface_;
  sc.minImageCount = kImages;
  sc.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
  sc.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  sc.imageExtent = {kWidth, kHeight};
  sc.imageArrayLayers = 1;
  sc.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  sc.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  sc.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  sc.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  sc.clipped = VK_TRUE;
  if (!ok("vkCreateSwapchainKHR", vkCreateSwapchainKHR(device_, &sc, nullptr, &swapchain_))) return false;
  imageCount_ = 3;
  if (!ok("vkGetSwapchainImagesKHR", vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount_, images_))) return false;
  for (uint32_t i = 0; i < imageCount_; i++) {
    VkImageViewCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image = images_[i];
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = VK_FORMAT_B8G8R8A8_UNORM;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    if (!ok("vkCreateImageView", vkCreateImageView(device_, &vi, nullptr, &views_[i]))) return false;
  }
  // one subpass that clears the image (the bars beside the picture are black) and stores it for presenting
  VkAttachmentDescription att{};
  att.format = VK_FORMAT_B8G8R8A8_UNORM;
  att.samples = VK_SAMPLE_COUNT_1_BIT;
  att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkSubpassDescription sub{};
  sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  sub.colorAttachmentCount = 1;
  sub.pColorAttachments = &ref;
  VkRenderPassCreateInfo rp{};
  rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rp.attachmentCount = 1;
  rp.pAttachments = &att;
  rp.subpassCount = 1;
  rp.pSubpasses = &sub;
  if (!ok("vkCreateRenderPass", vkCreateRenderPass(device_, &rp, nullptr, &renderPass_))) return false;
  for (uint32_t i = 0; i < imageCount_; i++) {
    VkFramebufferCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fi.renderPass = renderPass_;
    fi.attachmentCount = 1;
    fi.pAttachments = &views_[i];
    fi.width = kWidth;
    fi.height = kHeight;
    fi.layers = 1;
    if (!ok("vkCreateFramebuffer", vkCreateFramebuffer(device_, &fi, nullptr, &framebuffers_[i]))) return false;
  }
  return true;
}

bool VideoVk::createPipeline() {
  VkDescriptorSetLayoutBinding b{};
  b.binding = 0;
  b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  b.descriptorCount = 1;
  b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  VkDescriptorSetLayoutCreateInfo sl{};
  sl.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  sl.bindingCount = 1;
  sl.pBindings = &b;
  if (!ok("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(device_, &sl, nullptr, &setLayout_))) return false;
  VkPushConstantRange range{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push)};
  VkPipelineLayoutCreateInfo li{};
  li.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  li.setLayoutCount = 1;
  li.pSetLayouts = &setLayout_;
  li.pushConstantRangeCount = 1;
  li.pPushConstantRanges = &range;
  if (!ok("vkCreatePipelineLayout", vkCreatePipelineLayout(device_, &li, nullptr, &layout_))) return false;

  VkShaderModule modules[2] = {};
  const uint32_t* code[2] = {screen_vert_spirv, screen_frag_spirv};
  const uint32_t bytes[2] = {SCREEN_VERT_SPIRV_BYTES, SCREEN_FRAG_SPIRV_BYTES};
  for (int i = 0; i < 2; i++) {
    VkShaderModuleCreateInfo mi{};
    mi.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    mi.codeSize = bytes[i];
    mi.pCode = code[i];
    if (!ok("vkCreateShaderModule", vkCreateShaderModule(device_, &mi, nullptr, &modules[i]))) return false;
  }
  VkVertexInputBindingDescription vb{0, 16, VK_VERTEX_INPUT_RATE_VERTEX};
  VkVertexInputAttributeDescription va{0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0};
  VkPipelineVertexInputStateCreateInfo input{};
  input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  input.vertexBindingDescriptionCount = 1;
  input.pVertexBindingDescriptions = &vb;
  input.vertexAttributeDescriptionCount = 1;
  input.pVertexAttributeDescriptions = &va;
  VkPipelineInputAssemblyStateCreateInfo ia{};
  ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  const VkViewport viewport{0.0f, 0.0f, (float)kWidth, (float)kHeight, 0.0f, 1.0f};
  const VkRect2D scissor{{0, 0}, {kWidth, kHeight}};
  VkPipelineViewportStateCreateInfo vs{};
  vs.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vs.viewportCount = 1;
  vs.pViewports = &viewport;
  vs.scissorCount = 1;
  vs.pScissors = &scissor;
  VkPipelineRasterizationStateCreateInfo rs{};
  rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rs.polygonMode = VK_POLYGON_MODE_FILL;
  rs.cullMode = VK_CULL_MODE_NONE;
  rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rs.lineWidth = 1.0f;
  VkPipelineMultisampleStateCreateInfo ms{};
  ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  VkPipelineColorBlendAttachmentState opaque{};
  opaque.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  VkPipelineColorBlendStateCreateInfo blend{};
  blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blend.attachmentCount = 1;
  blend.pAttachments = &opaque;
  VkPipelineShaderStageCreateInfo stages[2] = {};
  for (int i = 0; i < 2; i++) {
    stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[i].stage = i ? VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_VERTEX_BIT;
    stages[i].module = modules[i];
    stages[i].pName = "main";
  }
  VkGraphicsPipelineCreateInfo gi{};
  gi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  gi.stageCount = 2;
  gi.pStages = stages;
  gi.pVertexInputState = &input;
  gi.pInputAssemblyState = &ia;
  gi.pViewportState = &vs;
  gi.pRasterizationState = &rs;
  gi.pMultisampleState = &ms;
  gi.pColorBlendState = &blend;
  gi.layout = layout_;
  gi.renderPass = renderPass_;
  const bool made = ok("vkCreateGraphicsPipelines",
                       vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &gi, nullptr, &pipeline_));
  vkDestroyShaderModule(device_, modules[0], nullptr);
  vkDestroyShaderModule(device_, modules[1], nullptr);
  if (!made) return false;

  // the quad's corners
  VkBufferCreateInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bi.size = sizeof kQuad;
  bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  if (!ok("vkCreateBuffer (quad)", vkCreateBuffer(device_, &bi, nullptr, &quad_))) return false;
  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(device_, quad_, &req);
  VkMemoryAllocateInfo ai{};
  ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  ai.allocationSize = req.size;
  if (!ok("vkAllocateMemory (quad)", vkAllocateMemory(device_, &ai, nullptr, &quadMemory_))) return false;
  if (!ok("vkBindBufferMemory (quad)", vkBindBufferMemory(device_, quad_, quadMemory_, 0))) return false;
  void* p = nullptr;
  if (!ok("vkMapMemory (quad)", vkMapMemory(device_, quadMemory_, 0, VK_WHOLE_SIZE, 0, &p))) return false;
  std::memcpy(p, kQuad, sizeof kQuad);
  vkUnmapMemory(device_, quadMemory_);
  return true;
}

bool VideoVk::createTexture() {
  // the frame buffer's whole kLineW x kFrameH words, as R8G8B8A8 (the shader takes the colour from g, b, a)
  VkImageCreateInfo ii{};
  ii.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ii.imageType = VK_IMAGE_TYPE_2D;
  ii.format = VK_FORMAT_R8G8B8A8_UNORM;
  ii.extent = {(uint32_t)kLineW, (uint32_t)kFrameH, 1};
  ii.mipLevels = 1;
  ii.arrayLayers = 1;
  ii.samples = VK_SAMPLE_COUNT_1_BIT;
  ii.tiling = VK_IMAGE_TILING_OPTIMAL;
  ii.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (!ok("vkCreateImage", vkCreateImage(device_, &ii, nullptr, &texture_))) return false;
  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(device_, texture_, &req);
  VkMemoryAllocateInfo ai{};
  ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  ai.allocationSize = req.size;
  if (!ok("vkAllocateMemory (textura)", vkAllocateMemory(device_, &ai, nullptr, &textureMemory_))) return false;
  if (!ok("vkBindImageMemory", vkBindImageMemory(device_, texture_, textureMemory_, 0))) return false;
  VkImageViewCreateInfo vi{};
  vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  vi.image = texture_;
  vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
  vi.format = VK_FORMAT_R8G8B8A8_UNORM;
  vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  if (!ok("vkCreateImageView (textura)", vkCreateImageView(device_, &vi, nullptr, &textureView_))) return false;
  VkSamplerCreateInfo sm{};
  sm.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sm.magFilter = VK_FILTER_NEAREST;  // (hito 3: nearest; the PC port's filters come with the options)
  sm.minFilter = VK_FILTER_NEAREST;
  sm.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  sm.addressModeU = sm.addressModeV = sm.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  if (!ok("vkCreateSampler", vkCreateSampler(device_, &sm, nullptr, &sampler_))) return false;
  VkDescriptorPoolSize ps{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};
  VkDescriptorPoolCreateInfo dp{};
  dp.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  dp.maxSets = 1;
  dp.poolSizeCount = 1;
  dp.pPoolSizes = &ps;
  if (!ok("vkCreateDescriptorPool", vkCreateDescriptorPool(device_, &dp, nullptr, &descriptorPool_))) return false;
  VkDescriptorSetAllocateInfo da{};
  da.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  da.descriptorPool = descriptorPool_;
  da.descriptorSetCount = 1;
  da.pSetLayouts = &setLayout_;
  if (!ok("vkAllocateDescriptorSets", vkAllocateDescriptorSets(device_, &da, &set_))) return false;
  VkDescriptorImageInfo img{sampler_, textureView_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  VkWriteDescriptorSet w{};
  w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  w.dstSet = set_;
  w.descriptorCount = 1;
  w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  w.pImageInfo = &img;
  vkUpdateDescriptorSets(device_, 1, &w, 0, nullptr);

  // the staging buffers, in turn: memory type 0 (vulkan4xenon copies from it on the GPU when its address is 4 KB
  // aligned and its rows are a linear texture's: kLineW = 704 words, 2816 bytes, 11 x 256)
  for (Slot& s : slots_) {
    VkBufferCreateInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = kFrameBytes;
    bi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (!ok("vkCreateBuffer (staging)", vkCreateBuffer(device_, &bi, nullptr, &s.staging))) return false;
    VkMemoryRequirements br;
    vkGetBufferMemoryRequirements(device_, s.staging, &br);
    VkMemoryAllocateInfo ma{};
    ma.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ma.allocationSize = br.size;
    ma.memoryTypeIndex = 0;
    if (!ok("vkAllocateMemory (staging)", vkAllocateMemory(device_, &ma, nullptr, &s.memory))) return false;
    if (!ok("vkBindBufferMemory (staging)", vkBindBufferMemory(device_, s.staging, s.memory, 0))) return false;
    if (!ok("vkMapMemory (staging)", vkMapMemory(device_, s.memory, 0, VK_WHOLE_SIZE, 0, &s.mapped))) return false;
    std::printf("video: staging %p (alineado a 4 KB: %s), %u bytes\n", s.mapped,
                ((uintptr_t)s.mapped & 4095) == 0 ? "si" : "NO, la copia irá por la CPU", (unsigned)kFrameBytes);
    VkCommandBufferAllocateInfo ca{};
    ca.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ca.commandPool = pool_;
    ca.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ca.commandBufferCount = 1;
    if (!ok("vkAllocateCommandBuffers", vkAllocateCommandBuffers(device_, &ca, &s.commands))) return false;
    VkFenceCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (!ok("vkCreateFence", vkCreateFence(device_, &fi, nullptr, &s.fence))) return false;
  }
  return true;
}

bool VideoVk::present(const uint32_t* frame, int extL, int extR, int height) {
  Slot& s = slots_[slot_];
  slot_ = (slot_ + 1) % kSlots;
  const uint64_t t0 = nowUs();
  if (s.pending) {
    if (!ok("vkWaitForFences", vkWaitForFences(device_, 1, &s.fence, VK_TRUE, kTimeoutNs))) return false;
    vkResetFences(device_, 1, &s.fence);
    s.pending = false;
  }
  const uint64_t t1 = nowUs();
  std::memcpy(s.mapped, frame, kFrameBytes);
  const uint64_t t2 = nowUs();
  uint32_t index = 0;
  if (!ok("vkAcquireNextImageKHR",
          vkAcquireNextImageKHR(device_, swapchain_, kTimeoutNs, VK_NULL_HANDLE, VK_NULL_HANDLE, &index)))
    return false;
  const uint64_t t3 = nowUs();

  // The picture: the height filled, 4:3 for the 256 pixels (the PC port's default aspect), the margins on either side
  // at that width, the 4:3 part centred (wider than the screen at the widest margins: cut at its edges).
  if (height <= 0 || height > kFrameH) height = 224;
  const float sy = (float)kHeight / (float)height;
  const float sx = sy * 7.0f / 6.0f;  // (256 x 224 as 4:3: 4/3 * 224/256 = 7/6; 3.75 at 224 lines)
  const float left = kWidth / 2.0f - 128.0f * sx - (float)extL * sx;
  const float width = (256.0f + (float)extL + (float)extR) * sx;
  Push push{};
  push.scale[0] = width / (kWidth / 2.0f);
  push.scale[1] = (float)height * sy / (kHeight / 2.0f);
  push.scale[2] = push.scale[3] = 1.0f;
  push.offset[0] = left / (kWidth / 2.0f) - 1.0f;
  push.offset[1] = -1.0f;
  push.uv[0] = (256.0f + (float)extL + (float)extR) / (float)kLineW;
  push.uv[1] = (float)height / (float)kFrameH;
  push.uv[2] = (float)(kMaxExt - extL) / (float)kLineW;
  push.uv[3] = 0.0f;

  VkCommandBuffer cb = s.commands;
  vkResetCommandBuffer(cb, 0);
  VkCommandBufferBeginInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(cb, &bi);
  transition(cb, texture_, textureReady_ ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  VkBufferImageCopy region{};
  region.bufferRowLength = kLineW;
  region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  region.imageExtent = {(uint32_t)kLineW, (uint32_t)kFrameH, 1};
  vkCmdCopyBufferToImage(cb, s.staging, texture_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  transition(cb, texture_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  textureReady_ = true;
  VkClearValue clear{};
  VkRenderPassBeginInfo rp{};
  rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp.renderPass = renderPass_;
  rp.framebuffer = framebuffers_[index];
  rp.renderArea = {{0, 0}, {kWidth, kHeight}};
  rp.clearValueCount = 1;
  rp.pClearValues = &clear;
  vkCmdBeginRenderPass(cb, &rp, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, layout_, 0, 1, &set_, 0, nullptr);
  const VkDeviceSize zero = 0;
  vkCmdBindVertexBuffers(cb, 0, 1, &quad_, &zero);
  vkCmdPushConstants(cb, layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof push, &push);
  vkCmdDraw(cb, 6, 1, 0, 0);
  vkCmdEndRenderPass(cb);
  if (!ok("vkEndCommandBuffer", vkEndCommandBuffer(cb))) return false;
  VkSubmitInfo submit{};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &cb;
  if (!ok("vkQueueSubmit", vkQueueSubmit(queue_, 1, &submit, s.fence))) return false;
  s.pending = true;
  VkPresentInfoKHR pr{};
  pr.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  pr.swapchainCount = 1;
  pr.pSwapchains = &swapchain_;
  pr.pImageIndices = &index;
  if (!ok("vkQueuePresentKHR", vkQueuePresentKHR(queue_, &pr))) return false;
  const uint64_t t4 = nowUs();
  stats_.waitUs += (t1 - t0) + (t3 - t2);
  stats_.copyUs += t2 - t1;
  stats_.submitUs += t4 - t3;
  stats_.frames++;
  return true;
}

bool VideoVk::screenPixel(int x, int y, int* r, int* g, int* b) const {
  const uint32_t shown = xsdk_display_shown_address();
  for (uint32_t i = 0; i < imageCount_; i++) {
    if (shown != xsdk_present_buffer_address(i)) continue;
    // a front buffer word holds B, G, R, A from the top byte down (vulkan4xenon tests/harness.c, harness_pixel)
    const uint32_t word = xsdk_present_pixels(shown)[tiled_index((uint32_t)x, (uint32_t)y, kWidth)];
    *r = (int)(word >> 8 & 0xff);
    *g = (int)(word >> 16 & 0xff);
    *b = (int)(word >> 24);
    return true;
  }
  return false;
}

void VideoVk::shutdown() {
  if (device_) {
    vkDeviceWaitIdle(device_);
    for (Slot& s : slots_) {
      if (s.fence) vkDestroyFence(device_, s.fence, nullptr);
      if (s.mapped) vkUnmapMemory(device_, s.memory);
      if (s.staging) vkDestroyBuffer(device_, s.staging, nullptr);
      if (s.memory) vkFreeMemory(device_, s.memory, nullptr);
      s = Slot();
    }
    if (descriptorPool_) vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
    if (sampler_) vkDestroySampler(device_, sampler_, nullptr);
    if (textureView_) vkDestroyImageView(device_, textureView_, nullptr);
    if (texture_) vkDestroyImage(device_, texture_, nullptr);
    if (textureMemory_) vkFreeMemory(device_, textureMemory_, nullptr);
    if (quad_) vkDestroyBuffer(device_, quad_, nullptr);
    if (quadMemory_) vkFreeMemory(device_, quadMemory_, nullptr);
    if (pipeline_) vkDestroyPipeline(device_, pipeline_, nullptr);
    if (layout_) vkDestroyPipelineLayout(device_, layout_, nullptr);
    if (setLayout_) vkDestroyDescriptorSetLayout(device_, setLayout_, nullptr);
    for (uint32_t i = 0; i < imageCount_; i++) {
      if (framebuffers_[i]) vkDestroyFramebuffer(device_, framebuffers_[i], nullptr);
      if (views_[i]) vkDestroyImageView(device_, views_[i], nullptr);
    }
    if (renderPass_) vkDestroyRenderPass(device_, renderPass_, nullptr);
    if (swapchain_) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    if (pool_) vkDestroyCommandPool(device_, pool_, nullptr);
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }
  if (instance_) {
    if (surface_) vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }
}
