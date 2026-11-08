// The app's video on the Xbox 360: see video_xenon.h. The device and swapchain as VideoVk makes them (video_vk.cpp,
// run 015); the canvas as platform/android/video_gles.cpp's GlesCanvas, method for method, its text from
// stb_truetype in place of FreeType (the same metrics: ascent and line height rounded up, advances in whole pixels
// as FreeType's hinted ones, a pixel of room round each glyph).
#include "platform/xenon/video_xenon.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <tuple>

#include "core/ppu.h"
#include "platform/xenon/canvas_shaders.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

// The fonts, built into the .xex (fonts_xenon.cpp): Luckiest Guy (titles), DejaVu Sans and DejaVu Sans Bold.
extern "C" const unsigned char smas_font_title[], smas_font_title_end[];
extern "C" const unsigned char smas_font_sans[], smas_font_sans_end[];
extern "C" const unsigned char smas_font_bold[], smas_font_bold_end[];

namespace {

constexpr uint64_t kTimeoutNs = 1000000000ull;
constexpr uint32_t kWidth = 1280, kHeight = 720, kImages = 2, kSlots = 2;
constexpr int kMaxQuads = 16384;                 // per draw (16-bit indices)
constexpr size_t kVertexBytes = 8u << 20;        // per frame: about 52,000 quads
constexpr uint32_t kMaxSets = 2048;              // descriptor sets before the pool is emptied
constexpr int kGamePush = 28;                    // game.frag's push constants: 7 vec4, the first canvas.vert's

uint64_t nowUs() {
  return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

bool ok(const char* call, VkResult r) {
  if (r != VK_SUCCESS) std::printf("video: %s devolvió %d\n", call, (int)r);
  return r == VK_SUCCESS;
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

// A quad's corner, as video_gles.cpp's OVert; the colour as the bytes r, g, b, a (R8G8B8A8_UNORM reads by address).
struct OVert {
  float x, y, u, v;
  uint8_t col[4];
  float p[4];
  float mode;
};
static_assert(sizeof(OVert) == 40, "vertex layout");

struct Rgba {
  uint8_t c[4];
};

Rgba rgba(ColorF c) {
  auto b = [](float v) { return (uint8_t)std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f); };
  return Rgba{{b(c.r), b(c.g), b(c.b), b(c.a)}};
}

// The text's code points: ASCII, Latin-1 and the symbols the menus use (video_gles.cpp's).
const uint32_t kExtraCps[] = {0x2190, 0x2191, 0x2192, 0x2193, 0x25B6, 0x25C0, 0x2713, 0x2022, 0x266A, 0x2026,
                              0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2605, 0x24B6, 0x24B7, 0x2122,
                              0x00D7, 0x2715};
const float kSizes[] = {9, 11, 13, 15, 17, 19, 22, 25, 28, 32, 36, 41, 47, 54, 62, 71, 82, 94, 108, 124, 142};

}  // namespace

// ---------------------------------------------------------------------------
// A texture: an optimal image, and a staging buffer for each frame slot that has filled it (made when first used).
// The staging rows are a linear texture's (a multiple of 256 bytes) and the buffers are 4 KB aligned, which
// vulkan4xenon copies on the GPU (video_vk.cpp).
struct Tex {
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  int w = 0, h = 0, bpp = 4;
  uint32_t pitch = 0;  // staging row, bytes
  VkBuffer staging[kSlots] = {};
  VkDeviceMemory stagingMemory[kSlots] = {};
  uint8_t* mapped[kSlots] = {};
  bool ready = false;          // copied at least once (its layout is SHADER_READ_ONLY)
  uint64_t copyFrame = ~0ull;  // the frame whose copy list holds it
  int copySlot = 0;
};

struct VideoDevice {
  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physical = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue queue = VK_NULL_HANDLE;
  VkCommandPool pool = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  uint32_t imageCount = 0;
  VkImage images[3] = {};
  VkImageView views[3] = {};
  VkFramebuffer framebuffers[3] = {};
  VkRenderPass renderPass = VK_NULL_HANDLE;
  VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
  VkPipelineLayout layout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  // the game picture's (game.frag): its push constants are canvas.vert's and the filters' (Video::draw)
  VkPipelineLayout gameLayout = VK_NULL_HANDLE;
  VkPipeline gamePipeline = VK_NULL_HANDLE;
  VkSampler nearest = VK_NULL_HANDLE, linear = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  uint32_t setsUsed = 0;
  VkBuffer indices = VK_NULL_HANDLE;
  VkDeviceMemory indexMemory = VK_NULL_HANDLE;
  struct Slot {
    VkCommandBuffer commands = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool pending = false;
    VkBuffer vertices = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    uint8_t* mapped = nullptr;
    size_t used = 0;
  };
  Slot slots[kSlots];
  int slot = 0;
  uint64_t frameNo = 0;
  bool inFrame = false;
  std::vector<Tex*> copies;  // this frame's
  struct Batch {
    size_t offset;
    uint32_t quads;
    Tex *atlas, *image;
    bool smooth;
    VkRect2D scissor;
    bool game;  // drawn by gamePipeline, with push
    float push[kGamePush];
  };
  std::vector<Batch> batches;
  std::map<std::tuple<Tex*, Tex*, bool>, VkDescriptorSet> sets;
  std::vector<std::pair<Tex*, uint64_t>> graveyard;  // freed, and in which frame
  Tex* white8 = nullptr;     // (bound where a draw samples no atlas)
  Tex* whiteRgba = nullptr;  // (... and no picture)
  Video::Stats stats;
  uint32_t dropped = 0;

  bool init();
  bool createSwapchain();
  bool createPipeline();
  bool allocBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* buf, VkDeviceMemory* mem, void** mapped);
  void beginFrame();
  Tex* makeTex(int w, int h, bool r8);
  void destroyTex(Tex* t);
  void freeTex(Tex* t);
  uint8_t* stage(Tex* t);  // where this frame's picture of t goes (pitch bytes a row), its copy queued
  void addQuads(const OVert* v, size_t quads, Tex* atlas, Tex* image, bool smooth, const VkRect2D& scissor);
  void addGame(const OVert v[4], Tex* t, const float push[kGamePush]);  // t linear at binding 0, point at 1
  VkDescriptorSet descriptorSet(Tex* atlas, Tex* image, bool smooth);
  void present();
  void shutdown();
};

bool VideoDevice::init() {
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
  if (!ok("vkCreateInstance", vkCreateInstance(&ii, nullptr, &instance))) return false;
  uint32_t count = 1;
  if (!ok("vkEnumeratePhysicalDevices", vkEnumeratePhysicalDevices(instance, &count, &physical))) return false;
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
  if (!ok("vkCreateDevice", vkCreateDevice(physical, &di, nullptr, &device))) return false;
  vkGetDeviceQueue(device, 0, 0, &queue);
  VkCommandPoolCreateInfo pi{};
  pi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  if (!ok("vkCreateCommandPool", vkCreateCommandPool(device, &pi, nullptr, &pool))) return false;
  if (!createSwapchain() || !createPipeline()) return false;
  for (Slot& s : slots) {
    void* p = nullptr;
    if (!allocBuffer(kVertexBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &s.vertices, &s.vertexMemory, &p)) return false;
    s.mapped = (uint8_t*)p;
    VkCommandBufferAllocateInfo ca{};
    ca.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ca.commandPool = pool;
    ca.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ca.commandBufferCount = 1;
    if (!ok("vkAllocateCommandBuffers", vkAllocateCommandBuffers(device, &ca, &s.commands))) return false;
    VkFenceCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (!ok("vkCreateFence", vkCreateFence(device, &fi, nullptr, &s.fence))) return false;
  }
  // the quads' indices: 0 1 2, 2 1 3 for each
  std::vector<uint16_t> idx((size_t)kMaxQuads * 6);
  for (int q = 0; q < kMaxQuads; q++) {
    const uint16_t b = (uint16_t)(q * 4);
    uint16_t* o = &idx[(size_t)q * 6];
    o[0] = b;
    o[1] = (uint16_t)(b + 1);
    o[2] = (uint16_t)(b + 2);
    o[3] = (uint16_t)(b + 2);
    o[4] = (uint16_t)(b + 1);
    o[5] = (uint16_t)(b + 3);
  }
  void* p = nullptr;
  if (!allocBuffer(idx.size() * 2, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &indices, &indexMemory, &p)) return false;
  std::memcpy(p, idx.data(), idx.size() * 2);
  vkUnmapMemory(device, indexMemory);
  // the placeholders, white
  white8 = makeTex(1, 1, true);
  whiteRgba = makeTex(1, 1, false);
  if (!white8 || !whiteRgba) return false;
  std::memset(stage(white8), 0xff, white8->pitch);
  std::memset(stage(whiteRgba), 0xff, whiteRgba->pitch);
  return true;
}

bool VideoDevice::createSwapchain() {
  uint32_t count = 1;
  VkDisplayPropertiesKHR display;
  if (!ok("vkGetPhysicalDeviceDisplayPropertiesKHR", vkGetPhysicalDeviceDisplayPropertiesKHR(physical, &count, &display)))
    return false;
  count = 1;
  VkDisplayModePropertiesKHR mode;
  if (!ok("vkGetDisplayModePropertiesKHR", vkGetDisplayModePropertiesKHR(physical, display.display, &count, &mode)))
    return false;
  std::printf("video: pantalla %s, %ux%u a %u mHz\n", display.displayName, (unsigned)mode.parameters.visibleRegion.width,
              (unsigned)mode.parameters.visibleRegion.height, (unsigned)mode.parameters.refreshRate);
  VkDisplaySurfaceCreateInfoKHR si{};
  si.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
  si.displayMode = mode.displayMode;
  si.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  si.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
  si.imageExtent = mode.parameters.visibleRegion;
  if (!ok("vkCreateDisplayPlaneSurfaceKHR", vkCreateDisplayPlaneSurfaceKHR(instance, &si, nullptr, &surface)))
    return false;
  VkSwapchainCreateInfoKHR sc{};
  sc.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sc.surface = surface;
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
  if (!ok("vkCreateSwapchainKHR", vkCreateSwapchainKHR(device, &sc, nullptr, &swapchain))) return false;
  imageCount = 3;
  if (!ok("vkGetSwapchainImagesKHR", vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images))) return false;
  for (uint32_t i = 0; i < imageCount; i++) {
    VkImageViewCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image = images[i];
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = VK_FORMAT_B8G8R8A8_UNORM;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    if (!ok("vkCreateImageView", vkCreateImageView(device, &vi, nullptr, &views[i]))) return false;
  }
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
  if (!ok("vkCreateRenderPass", vkCreateRenderPass(device, &rp, nullptr, &renderPass))) return false;
  for (uint32_t i = 0; i < imageCount; i++) {
    VkFramebufferCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fi.renderPass = renderPass;
    fi.attachmentCount = 1;
    fi.pAttachments = &views[i];
    fi.width = kWidth;
    fi.height = kHeight;
    fi.layers = 1;
    if (!ok("vkCreateFramebuffer", vkCreateFramebuffer(device, &fi, nullptr, &framebuffers[i]))) return false;
  }
  return true;
}

bool VideoDevice::createPipeline() {
  VkDescriptorSetLayoutBinding b[2] = {};
  for (int i = 0; i < 2; i++) {
    b[i].binding = (uint32_t)i;
    b[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    b[i].descriptorCount = 1;
    b[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  }
  VkDescriptorSetLayoutCreateInfo sl{};
  sl.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  sl.bindingCount = 2;
  sl.pBindings = b;
  if (!ok("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(device, &sl, nullptr, &setLayout))) return false;
  VkPushConstantRange range{VK_SHADER_STAGE_VERTEX_BIT, 0, 16};
  VkPipelineLayoutCreateInfo li{};
  li.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  li.setLayoutCount = 1;
  li.pSetLayouts = &setLayout;
  li.pushConstantRangeCount = 1;
  li.pPushConstantRanges = &range;
  if (!ok("vkCreatePipelineLayout", vkCreatePipelineLayout(device, &li, nullptr, &layout))) return false;
  VkPushConstantRange gameRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, kGamePush * 4};
  li.pPushConstantRanges = &gameRange;
  if (!ok("vkCreatePipelineLayout", vkCreatePipelineLayout(device, &li, nullptr, &gameLayout))) return false;

  VkShaderModule modules[3] = {};
  const uint32_t* code[3] = {canvas_vert_spirv, canvas_frag_spirv, game_frag_spirv};
  const uint32_t bytes[3] = {CANVAS_VERT_SPIRV_BYTES, CANVAS_FRAG_SPIRV_BYTES, GAME_FRAG_SPIRV_BYTES};
  for (int i = 0; i < 3; i++) {
    VkShaderModuleCreateInfo mi{};
    mi.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    mi.codeSize = bytes[i];
    mi.pCode = code[i];
    if (!ok("vkCreateShaderModule", vkCreateShaderModule(device, &mi, nullptr, &modules[i]))) return false;
  }
  VkVertexInputBindingDescription vb{0, sizeof(OVert), VK_VERTEX_INPUT_RATE_VERTEX};
  const VkVertexInputAttributeDescription va[5] = {
      {0, 0, VK_FORMAT_R32G32_SFLOAT, (uint32_t)offsetof(OVert, x)},
      {1, 0, VK_FORMAT_R32G32_SFLOAT, (uint32_t)offsetof(OVert, u)},
      {2, 0, VK_FORMAT_R8G8B8A8_UNORM, (uint32_t)offsetof(OVert, col)},
      {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, (uint32_t)offsetof(OVert, p)},
      {4, 0, VK_FORMAT_R32_SFLOAT, (uint32_t)offsetof(OVert, mode)},
  };
  VkPipelineVertexInputStateCreateInfo input{};
  input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  input.vertexBindingDescriptionCount = 1;
  input.pVertexBindingDescriptions = &vb;
  input.vertexAttributeDescriptionCount = 5;
  input.pVertexAttributeDescriptions = va;
  VkPipelineInputAssemblyStateCreateInfo ia{};
  ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  const VkViewport viewport{0.0f, 0.0f, (float)kWidth, (float)kHeight, 0.0f, 1.0f};
  VkPipelineViewportStateCreateInfo vs{};
  vs.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vs.viewportCount = 1;
  vs.pViewports = &viewport;
  vs.scissorCount = 1;  // (dynamic: the canvas's clips)
  const VkDynamicState dynamic[1] = {VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dy{};
  dy.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dy.dynamicStateCount = 1;
  dy.pDynamicStates = dynamic;
  VkPipelineRasterizationStateCreateInfo rs{};
  rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rs.polygonMode = VK_POLYGON_MODE_FILL;
  rs.cullMode = VK_CULL_MODE_NONE;
  rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rs.lineWidth = 1.0f;
  VkPipelineMultisampleStateCreateInfo ms{};
  ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  // video_gles.cpp's blend: SRC_ALPHA, ONE_MINUS_SRC_ALPHA; the alpha ONE, ONE_MINUS_SRC_ALPHA
  VkPipelineColorBlendAttachmentState over{};
  over.blendEnable = VK_TRUE;
  over.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  over.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  over.colorBlendOp = VK_BLEND_OP_ADD;
  over.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  over.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  over.alphaBlendOp = VK_BLEND_OP_ADD;
  over.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  VkPipelineColorBlendStateCreateInfo blend{};
  blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blend.attachmentCount = 1;
  blend.pAttachments = &over;
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
  gi.pDynamicState = &dy;
  gi.layout = layout;
  gi.renderPass = renderPass;
  bool made =
      ok("vkCreateGraphicsPipelines", vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gi, nullptr, &pipeline));
  // the game picture: opaque, as video_gles.cpp draws it without blending (its fade darkens towards black)
  VkPipelineColorBlendAttachmentState opaque = over;
  opaque.blendEnable = VK_FALSE;
  blend.pAttachments = &opaque;
  stages[1].module = modules[2];
  gi.layout = gameLayout;
  made = made && ok("vkCreateGraphicsPipelines (juego)",
                    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gi, nullptr, &gamePipeline));
  for (VkShaderModule m : modules) vkDestroyShaderModule(device, m, nullptr);
  if (!made) return false;

  for (int i = 0; i < 2; i++) {
    VkSamplerCreateInfo sm{};
    sm.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sm.magFilter = sm.minFilter = i ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    sm.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sm.addressModeU = sm.addressModeV = sm.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (!ok("vkCreateSampler", vkCreateSampler(device, &sm, nullptr, i ? &linear : &nearest))) return false;
  }
  VkDescriptorPoolSize ps{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxSets * 2};
  VkDescriptorPoolCreateInfo dp{};
  dp.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  dp.maxSets = kMaxSets;
  dp.poolSizeCount = 1;
  dp.pPoolSizes = &ps;
  return ok("vkCreateDescriptorPool", vkCreateDescriptorPool(device, &dp, nullptr, &descriptorPool));
}

bool VideoDevice::allocBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* buf, VkDeviceMemory* mem,
                              void** mapped) {
  VkBufferCreateInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bi.size = size;
  bi.usage = usage;
  if (!ok("vkCreateBuffer", vkCreateBuffer(device, &bi, nullptr, buf))) return false;
  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(device, *buf, &req);
  VkMemoryAllocateInfo ai{};
  ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  ai.allocationSize = req.size;
  ai.memoryTypeIndex = 0;  // (host visible)
  if (!ok("vkAllocateMemory", vkAllocateMemory(device, &ai, nullptr, mem))) return false;
  if (!ok("vkBindBufferMemory", vkBindBufferMemory(device, *buf, *mem, 0))) return false;
  return ok("vkMapMemory", vkMapMemory(device, *mem, 0, VK_WHOLE_SIZE, 0, mapped));
}

// A frame begins at its first upload, draw or picture: its slot's last use (two frames ago) is waited for.
void VideoDevice::beginFrame() {
  if (inFrame) return;
  inFrame = true;
  Slot& s = slots[slot];
  const uint64_t t0 = nowUs();
  if (s.pending) {
    ok("vkWaitForFences", vkWaitForFences(device, 1, &s.fence, VK_TRUE, kTimeoutNs));
    vkResetFences(device, 1, &s.fence);
    s.pending = false;
  }
  stats.waitUs += nowUs() - t0;
  stats.fenceUs += nowUs() - t0;
  s.used = 0;
  batches.clear();
  copies.clear();
  // the textures freed two frames ago or more: no frame in flight draws them
  size_t keep = 0;
  for (auto& g : graveyard) {
    if (g.second + kSlots <= frameNo) destroyTex(g.first);
    else graveyard[keep++] = g;
  }
  graveyard.resize(keep);
}

Tex* VideoDevice::makeTex(int w, int h, bool r8) {
  if (w <= 0 || h <= 0) return nullptr;
  Tex* t = new Tex();
  t->w = w;
  t->h = h;
  t->bpp = r8 ? 1 : 4;
  t->pitch = ((uint32_t)(w * t->bpp) + 255u) & ~255u;
  const VkFormat format = r8 ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
  VkImageCreateInfo ii{};
  ii.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ii.imageType = VK_IMAGE_TYPE_2D;
  ii.format = format;
  ii.extent = {(uint32_t)w, (uint32_t)h, 1};
  ii.mipLevels = 1;
  ii.arrayLayers = 1;
  ii.samples = VK_SAMPLE_COUNT_1_BIT;
  ii.tiling = VK_IMAGE_TILING_OPTIMAL;
  ii.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  bool good = ok("vkCreateImage", vkCreateImage(device, &ii, nullptr, &t->image));
  if (good) {
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, t->image, &req);
    VkMemoryAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = req.size;
    good = ok("vkAllocateMemory (imagen)", vkAllocateMemory(device, &ai, nullptr, &t->memory)) &&
           ok("vkBindImageMemory", vkBindImageMemory(device, t->image, t->memory, 0));
  }
  if (good) {
    VkImageViewCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image = t->image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = format;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    good = ok("vkCreateImageView (imagen)", vkCreateImageView(device, &vi, nullptr, &t->view));
  }
  if (!good) {
    destroyTex(t);
    return nullptr;
  }
  return t;
}

void VideoDevice::destroyTex(Tex* t) {
  if (!t) return;
  for (int i = 0; i < (int)kSlots; i++) {
    if (t->mapped[i]) vkUnmapMemory(device, t->stagingMemory[i]);
    if (t->staging[i]) vkDestroyBuffer(device, t->staging[i], nullptr);
    if (t->stagingMemory[i]) vkFreeMemory(device, t->stagingMemory[i], nullptr);
  }
  if (t->view) vkDestroyImageView(device, t->view, nullptr);
  if (t->image) vkDestroyImage(device, t->image, nullptr);
  if (t->memory) vkFreeMemory(device, t->memory, nullptr);
  delete t;
}

void VideoDevice::freeTex(Tex* t) {
  if (!t) return;
  for (auto it = sets.begin(); it != sets.end();)
    it = std::get<0>(it->first) == t || std::get<1>(it->first) == t ? sets.erase(it) : std::next(it);
  graveyard.push_back({t, frameNo});
}

uint8_t* VideoDevice::stage(Tex* t) {
  beginFrame();
  if (!t->staging[slot]) {
    void* p = nullptr;
    if (!allocBuffer((VkDeviceSize)t->pitch * t->h, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &t->staging[slot],
                     &t->stagingMemory[slot], &p))
      return nullptr;
    t->mapped[slot] = (uint8_t*)p;
    if ((uintptr_t)p & 4095) std::printf("video: staging de %dx%d sin alinear a 4 KB (%p)\n", t->w, t->h, p);
  }
  if (t->copyFrame != frameNo) {
    t->copyFrame = frameNo;
    copies.push_back(t);
  }
  t->copySlot = slot;
  return t->mapped[slot];
}

void VideoDevice::addQuads(const OVert* v, size_t quads, Tex* atlas, Tex* image, bool smooth,
                           const VkRect2D& scissor) {
  beginFrame();
  Slot& s = slots[slot];
  while (quads > 0) {
    const size_t n = std::min(quads, (size_t)kMaxQuads), bytes = n * 4 * sizeof(OVert);
    if (s.used + bytes > kVertexBytes) {
      dropped += (uint32_t)quads;
      return;
    }
    std::memcpy(s.mapped + s.used, v, bytes);
    Batch b{s.used, (uint32_t)n, atlas ? atlas : white8, image ? image : whiteRgba, smooth, scissor, false, {}};
    batches.push_back(b);
    s.used += bytes;
    stats.quads += (uint32_t)n;
    v += n * 4;
    quads -= n;
  }
}

void VideoDevice::addGame(const OVert v[4], Tex* t, const float push[kGamePush]) {
  const size_t before = batches.size();
  addQuads(v, 1, t, t, false, VkRect2D{{0, 0}, {kWidth, kHeight}});
  if (batches.size() == before) return;  // (no room)
  batches.back().game = true;
  std::memcpy(batches.back().push, push, sizeof(float) * kGamePush);
}

VkDescriptorSet VideoDevice::descriptorSet(Tex* atlas, Tex* image, bool smooth) {
  const auto key = std::make_tuple(atlas, image, smooth);
  auto it = sets.find(key);
  if (it != sets.end()) return it->second;
  VkDescriptorSetAllocateInfo da{};
  da.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  da.descriptorPool = descriptorPool;
  da.descriptorSetCount = 1;
  da.pSetLayouts = &setLayout;
  VkDescriptorSet set = VK_NULL_HANDLE;
  if (!ok("vkAllocateDescriptorSets", vkAllocateDescriptorSets(device, &da, &set))) return VK_NULL_HANDLE;
  setsUsed++;
  VkDescriptorImageInfo img[2] = {{linear, atlas->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                  {smooth ? linear : nearest, image->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
  VkWriteDescriptorSet w[2] = {};
  for (int i = 0; i < 2; i++) {
    w[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[i].dstSet = set;
    w[i].dstBinding = (uint32_t)i;
    w[i].descriptorCount = 1;
    w[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[i].pImageInfo = &img[i];
  }
  vkUpdateDescriptorSets(device, 2, w, 0, nullptr);
  sets[key] = set;
  return set;
}

void VideoDevice::present() {
  beginFrame();
  const uint64_t t0 = nowUs();
  uint32_t index = 0;
  if (!ok("vkAcquireNextImageKHR",
          vkAcquireNextImageKHR(device, swapchain, kTimeoutNs, VK_NULL_HANDLE, VK_NULL_HANDLE, &index))) {
    inFrame = false;
    return;
  }
  const uint64_t t1 = nowUs();
  // (the pool full: emptied once nothing in flight uses its sets)
  if (setsUsed + batches.size() > kMaxSets) {
    vkDeviceWaitIdle(device);
    vkResetDescriptorPool(device, descriptorPool, 0);
    sets.clear();
    setsUsed = 0;
  }
  Slot& s = slots[slot];
  VkCommandBuffer cb = s.commands;
  vkResetCommandBuffer(cb, 0);
  VkCommandBufferBeginInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(cb, &bi);
  for (Tex* t : copies) {
    transition(cb, t->image, t->ready ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy region{};
    region.bufferRowLength = t->pitch / (uint32_t)t->bpp;
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {(uint32_t)t->w, (uint32_t)t->h, 1};
    vkCmdCopyBufferToImage(cb, t->staging[t->copySlot], t->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    transition(cb, t->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    t->ready = true;
  }
  stats.copies += (uint32_t)copies.size();
  VkClearValue clear{};
  VkRenderPassBeginInfo rp{};
  rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp.renderPass = renderPass;
  rp.framebuffer = framebuffers[index];
  rp.renderArea = {{0, 0}, {kWidth, kHeight}};
  rp.clearValueCount = 1;
  rp.pClearValues = &clear;
  vkCmdBeginRenderPass(cb, &rp, VK_SUBPASS_CONTENTS_INLINE);
  if (!batches.empty()) {
    const float screen[4] = {2.0f / kWidth, 2.0f / kHeight, 0, 0};
    vkCmdBindIndexBuffer(cb, indices, 0, VK_INDEX_TYPE_UINT16);
    VkDescriptorSet bound = VK_NULL_HANDLE;
    VkRect2D scissor{{-1, -1}, {0, 0}};
    int pipe = -1;  // 0 the canvas's, 1 the game's
    for (const Batch& b : batches) {
      // (a picture never copied has nothing to show)
      if (!b.atlas->ready && b.atlas->copyFrame != frameNo) continue;
      if (!b.image->ready && b.image->copyFrame != frameNo) continue;
      const VkDescriptorSet set = descriptorSet(b.atlas, b.image, b.smooth);
      if (!set) continue;
      if (b.game) {
        // (each game draw pushes its own parameters; the layouts differ, so the set is bound again)
        if (pipe != 1) vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, gamePipeline);
        pipe = 1;
        float push[kGamePush];
        std::memcpy(push, b.push, sizeof push);
        std::memcpy(push, screen, sizeof screen);
        vkCmdPushConstants(cb, gameLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof push,
                           push);
        bound = VK_NULL_HANDLE;
      } else if (pipe != 0) {
        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdPushConstants(cb, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof screen, screen);
        pipe = 0;
        bound = VK_NULL_HANDLE;
      }
      if (set != bound) {
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, b.game ? gameLayout : layout, 0, 1, &set, 0,
                                nullptr);
        bound = set;
      }
      if (std::memcmp(&scissor, &b.scissor, sizeof scissor) != 0) {
        scissor = b.scissor;
        vkCmdSetScissor(cb, 0, 1, &scissor);
      }
      const VkDeviceSize offset = b.offset;
      vkCmdBindVertexBuffers(cb, 0, 1, &s.vertices, &offset);
      vkCmdDrawIndexed(cb, b.quads * 6, 1, 0, 0, 0);
    }
  }
  vkCmdEndRenderPass(cb);
  if (ok("vkEndCommandBuffer", vkEndCommandBuffer(cb))) {
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cb;
    if (ok("vkQueueSubmit", vkQueueSubmit(queue, 1, &submit, s.fence))) {
      s.pending = true;
      VkPresentInfoKHR pr{};
      pr.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      pr.swapchainCount = 1;
      pr.pSwapchains = &swapchain;
      pr.pImageIndices = &index;
      ok("vkQueuePresentKHR", vkQueuePresentKHR(queue, &pr));
    }
  }
  const uint64_t t2 = nowUs();
  stats.waitUs += t1 - t0;
  stats.workUs += t2 - t1;
  stats.frames++;
  if (dropped) {
    std::printf("video: %u quads sin sitio en el frame\n", (unsigned)dropped);
    dropped = 0;
  }
  inFrame = false;
  slot = (slot + 1) % (int)kSlots;
  frameNo++;
}

void VideoDevice::shutdown() {
  if (device) {
    vkDeviceWaitIdle(device);
    for (auto& g : graveyard) destroyTex(g.first);
    graveyard.clear();
    destroyTex(white8);
    destroyTex(whiteRgba);
    white8 = whiteRgba = nullptr;
    for (Slot& s : slots) {
      if (s.fence) vkDestroyFence(device, s.fence, nullptr);
      if (s.mapped) vkUnmapMemory(device, s.vertexMemory);
      if (s.vertices) vkDestroyBuffer(device, s.vertices, nullptr);
      if (s.vertexMemory) vkFreeMemory(device, s.vertexMemory, nullptr);
      s = Slot();
    }
    if (indices) vkDestroyBuffer(device, indices, nullptr);
    if (indexMemory) vkFreeMemory(device, indexMemory, nullptr);
    if (descriptorPool) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    if (nearest) vkDestroySampler(device, nearest, nullptr);
    if (linear) vkDestroySampler(device, linear, nullptr);
    if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
    if (layout) vkDestroyPipelineLayout(device, layout, nullptr);
    if (gamePipeline) vkDestroyPipeline(device, gamePipeline, nullptr);
    if (gameLayout) vkDestroyPipelineLayout(device, gameLayout, nullptr);
    if (setLayout) vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
    for (uint32_t i = 0; i < imageCount; i++) {
      if (framebuffers[i]) vkDestroyFramebuffer(device, framebuffers[i], nullptr);
      if (views[i]) vkDestroyImageView(device, views[i], nullptr);
    }
    if (renderPass) vkDestroyRenderPass(device, renderPass, nullptr);
    if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
    if (pool) vkDestroyCommandPool(device, pool, nullptr);
    vkDestroyDevice(device, nullptr);
    device = VK_NULL_HANDLE;
  }
  if (instance) {
    if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    instance = VK_NULL_HANDLE;
  }
}

// ---------------------------------------------------------------------------
class XenonCanvas : public Canvas {
public:
  XenonCanvas(VideoDevice* dev, unsigned epoch) : dev_(dev), epoch_(epoch) {}
  ~XenonCanvas() override { release(); }
  void release();
  void begin(int w, int h);
  void end() { flush(); }
  // the game picture: a texture's part (u1, v1) on dst, faded by its opacity over the black
  void drawTex(Tex* t, RectF dst, float u1, float v1, float opacity, bool smooth);

  unsigned epoch() const override { return epoch_; }
  void fillRect(RectF r, ColorF c) override {
    const Rgba k = rgba(c);
    quad(r.left, r.top, r.right, r.bottom, k, k, 0, 0, 0, 0, nullptr, 0);
  }
  void strokeRect(RectF r, float width, ColorF c) override { roundRect(grow(r, width * 0.5f), 0, width, c); }
  void fillRound(RectF r, float radius, ColorF c) override { roundRect(r, radius, 0, c); }
  void strokeRound(RectF r, float radius, float width, ColorF c) override {
    roundRect(grow(r, width * 0.5f), radius + width * 0.5f, width, c);
  }
  void strokeEllipse(float cx, float cy, float rx, float ry, float width, ColorF c) override {
    strokeRound(rectF(cx - rx, cy - ry, cx + rx, cy + ry), std::min(rx, ry), width, c);
  }
  void fillGradient(RectF r, ColorF top, ColorF bottom) override {
    quad(r.left, r.top, r.right, r.bottom, rgba(top), rgba(bottom), 0, 0, 0, 0, nullptr, 0);
  }
  void fillPolygon(const PointF* pts, int n, ColorF c) override {
    if (n < 3) return;
    float mx = 0, my = 0;
    for (int i = 0; i < n; i++) mx += pts[i].x, my += pts[i].y;
    mx /= n;
    my /= n;
    const Rgba k = rgba(c);
    for (int i = 0; i < n; i++) {
      const PointF &a = pts[i], &b = pts[(i + 1) % n];
      tri(mx, my, a.x, a.y, b.x, b.y, k);
    }
  }
  void strokePolygon(const PointF* pts, int n, float width, ColorF c) override {
    const Rgba k = rgba(c);
    for (int i = 0; i < n; i++) line(pts[i], pts[(i + 1) % n], width, k);
  }
  int makeShape(const std::vector<PointF>& pts, const std::vector<int>& counts) override;
  void fillShape(int shape, ColorF c, float scale, float tx, float ty) override;
  void freeShape(int shape) override {
    if (shape >= 1 && shape <= (int)shapes_.size()) shapes_[shape - 1].clear();
  }
  void text(const std::wstring& s, RectF r, float size, int weight, ColorF c, TextAlign align,
            ParaAlign para) override {
    drawText(s, r, size, weight, false, c, align, para);
  }
  float measure(const std::wstring& s, float size, int weight) override { return measureText(s, size, weight, false); }
  void titleText(const std::wstring& s, RectF r, float size, ColorF c, TextAlign align, ParaAlign para) override {
    drawText(s, r, size, kWeightNormal, true, c, align, para);
  }
  float measureTitle(const std::wstring& s, float size) override { return measureText(s, size, kWeightNormal, true); }
  void pushClip(RectF r) override;
  void popClip() override;
  int makeImage(int w, int h, bool alpha) override;
  void updateImage(int image, const uint32_t* px, int stridePixels) override;
  void drawImage(int image, RectF dst, float opacity, bool smooth) override;
  void freeImage(int image) override;

private:
  VideoDevice* dev_;
  unsigned epoch_;
  int w_ = 1280, h_ = 720;
  std::vector<OVert> verts_;
  Tex* atlas_ = nullptr;  // (the glyphs' atlas the batch samples)
  std::vector<RectF> clips_;
  VkRect2D scissor_{{0, 0}, {kWidth, kHeight}};
  std::vector<std::vector<float>> shapes_;
  struct Image {
    Tex* tex = nullptr;
    bool alpha = false;
  };
  std::vector<Image> images_;
  struct Glyph {
    float adv;
    int ox, oy, w, h;
    float u0, v0, u1, v1;
  };
  struct Font {
    Tex* tex = nullptr;
    int ascent = 0, lineH = 0;
    std::map<uint32_t, Glyph> glyphs;
    bool ok = false, tried = false;
  };
  std::map<int, Font> fonts_;

  static RectF grow(RectF r, float d) { return rectF(r.left - d, r.top - d, r.right + d, r.bottom + d); }
  void flush(Tex* image = nullptr, bool smooth = false);
  void useAtlas(Tex* t) {
    if (t != atlas_) {
      flush();
      atlas_ = t;
    }
  }
  void push4(const OVert v[4]);
  void quad(float x0, float y0, float x1, float y1, Rgba ctop, Rgba cbot, float u0, float v0, float u1, float v1,
            const float* prm, float mode);
  void tri(float x0, float y0, float x1, float y1, float x2, float y2, Rgba c);
  void line(PointF a, PointF b, float width, Rgba c);
  void roundRect(RectF r, float radius, float stroke, ColorF c);
  Font& font(float size, int weight, bool title, float* scale);
  bool buildFont(Font& f, float px, bool bold, bool title);
  void drawText(const std::wstring& s, RectF r, float size, int weight, bool title, ColorF c, TextAlign align,
                ParaAlign para);
  float measureText(const std::wstring& s, float size, int weight, bool title);
  void applyScissor();
};

void XenonCanvas::release() {
  for (auto& kv : fonts_) dev_->freeTex(kv.second.tex);
  fonts_.clear();
  for (Image& i : images_) dev_->freeTex(i.tex);
  images_.clear();
}

void XenonCanvas::begin(int w, int h) {
  w_ = w;
  h_ = h;
  verts_.clear();
  clips_.clear();
  atlas_ = nullptr;
  applyScissor();
}

void XenonCanvas::flush(Tex* image, bool smooth) {
  if (!verts_.empty()) dev_->addQuads(verts_.data(), verts_.size() / 4, atlas_, image, smooth, scissor_);
  verts_.clear();
}

void XenonCanvas::push4(const OVert v[4]) {
  if (verts_.size() >= (size_t)kMaxQuads * 4) flush();
  verts_.insert(verts_.end(), v, v + 4);
}

void XenonCanvas::quad(float x0, float y0, float x1, float y1, Rgba ctop, Rgba cbot, float u0, float v0, float u1,
                       float v1, const float* prm, float mode) {
  if (x1 <= x0 || y1 <= y0) return;
  float p[4] = {0, 0, 0, 0};
  if (prm) std::memcpy(p, prm, sizeof(p));
  OVert v[4] = {
      {x0, y0, u0, v0, {ctop.c[0], ctop.c[1], ctop.c[2], ctop.c[3]}, {p[0], p[1], p[2], p[3]}, mode},
      {x1, y0, u1, v0, {ctop.c[0], ctop.c[1], ctop.c[2], ctop.c[3]}, {p[0], p[1], p[2], p[3]}, mode},
      {x0, y1, u0, v1, {cbot.c[0], cbot.c[1], cbot.c[2], cbot.c[3]}, {p[0], p[1], p[2], p[3]}, mode},
      {x1, y1, u1, v1, {cbot.c[0], cbot.c[1], cbot.c[2], cbot.c[3]}, {p[0], p[1], p[2], p[3]}, mode},
  };
  push4(v);
}

void XenonCanvas::tri(float x0, float y0, float x1, float y1, float x2, float y2, Rgba c) {
  OVert v[4] = {
      {x0, y0, 0, 0, {c.c[0], c.c[1], c.c[2], c.c[3]}, {0, 0, 0, 0}, 0},
      {x1, y1, 0, 0, {c.c[0], c.c[1], c.c[2], c.c[3]}, {0, 0, 0, 0}, 0},
      {x2, y2, 0, 0, {c.c[0], c.c[1], c.c[2], c.c[3]}, {0, 0, 0, 0}, 0},
      {x2, y2, 0, 0, {c.c[0], c.c[1], c.c[2], c.c[3]}, {0, 0, 0, 0}, 0},
  };
  push4(v);
}

void XenonCanvas::line(PointF a, PointF b, float width, Rgba c) {
  float dx = b.x - a.x, dy = b.y - a.y, len = std::hypot(dx, dy);
  if (len <= 0) return;
  float nx = -dy / len * width * 0.5f, ny = dx / len * width * 0.5f;
  OVert v[4] = {
      {a.x + nx, a.y + ny, 0, 0, {c.c[0], c.c[1], c.c[2], c.c[3]}, {0, 0, 0, 0}, 0},
      {b.x + nx, b.y + ny, 0, 0, {c.c[0], c.c[1], c.c[2], c.c[3]}, {0, 0, 0, 0}, 0},
      {a.x - nx, a.y - ny, 0, 0, {c.c[0], c.c[1], c.c[2], c.c[3]}, {0, 0, 0, 0}, 0},
      {b.x - nx, b.y - ny, 0, 0, {c.c[0], c.c[1], c.c[2], c.c[3]}, {0, 0, 0, 0}, 0},
  };
  push4(v);
}

void XenonCanvas::roundRect(RectF r, float radius, float stroke, ColorF c) {
  float w = r.right - r.left, h = r.bottom - r.top;
  if (w <= 0 || h <= 0) return;
  float hw = w * 0.5f, hh = h * 0.5f;
  float prm[4] = {hw, hh, std::max(0.0f, std::min(radius, std::min(hw, hh))), stroke};
  float cx = r.left + hw, cy = r.top + hh;
  float x0 = r.left - 1, y0 = r.top - 1, x1 = r.right + 1, y1 = r.bottom + 1;
  const Rgba k = rgba(c);
  quad(x0, y0, x1, y1, k, k, x0 - cx, y0 - cy, x1 - cx, y1 - cy, prm, 2.0f);
}

int XenonCanvas::makeShape(const std::vector<PointF>& pts, const std::vector<int>& counts) {
  std::vector<float> t;
  size_t at = 0;
  for (int n : counts) {
    if (n >= 3) {
      float mx = 0, my = 0;
      for (int i = 0; i < n; i++) mx += pts[at + i].x, my += pts[at + i].y;
      mx /= n;
      my /= n;
      for (int i = 0; i < n; i++) {
        const PointF &a = pts[at + i], &b = pts[at + (i + 1) % n];
        t.insert(t.end(), {mx, my, a.x, a.y, b.x, b.y});
      }
    }
    at += n;
  }
  for (size_t i = 0; i < shapes_.size(); i++)
    if (shapes_[i].empty()) {
      shapes_[i] = std::move(t);
      return (int)i + 1;
    }
  shapes_.push_back(std::move(t));
  return (int)shapes_.size();
}

void XenonCanvas::fillShape(int shape, ColorF c, float scale, float tx, float ty) {
  if (shape < 1 || shape > (int)shapes_.size()) return;
  const std::vector<float>& t = shapes_[shape - 1];
  const Rgba k = rgba(c);
  for (size_t i = 0; i + 6 <= t.size(); i += 6)
    tri(t[i] * scale + tx, t[i + 1] * scale + ty, t[i + 2] * scale + tx, t[i + 3] * scale + ty,
        t[i + 4] * scale + tx, t[i + 5] * scale + ty, k);
}

// ---- clipping: the scissor (Vulkan's, from the top left like the canvas's) ---------------------------------
void XenonCanvas::applyScissor() {
  if (clips_.empty()) {
    scissor_ = {{0, 0}, {(uint32_t)w_, (uint32_t)h_}};
    return;
  }
  RectF r = clips_.back();
  int x0 = (int)std::floor(std::max(0.0f, r.left)), y0 = (int)std::floor(std::max(0.0f, r.top));
  int x1 = (int)std::ceil(std::min((float)w_, r.right)), y1 = (int)std::ceil(std::min((float)h_, r.bottom));
  scissor_ = {{x0, y0}, {(uint32_t)std::max(0, x1 - x0), (uint32_t)std::max(0, y1 - y0)}};
}

void XenonCanvas::pushClip(RectF r) {
  flush();
  if (!clips_.empty()) {
    const RectF& p = clips_.back();
    r = rectF(std::max(r.left, p.left), std::max(r.top, p.top), std::min(r.right, p.right),
              std::min(r.bottom, p.bottom));
  }
  clips_.push_back(r);
  applyScissor();
}

void XenonCanvas::popClip() {
  flush();
  if (!clips_.empty()) clips_.pop_back();
  applyScissor();
}

// ---- pictures ----------------------------------------------------------------
int XenonCanvas::makeImage(int w, int h, bool alpha) {
  Image im;
  im.tex = dev_->makeTex(w, h, false);
  im.alpha = alpha;
  if (!im.tex) return 0;
  for (size_t i = 0; i < images_.size(); i++)
    if (!images_[i].tex) {
      images_[i] = im;
      return (int)i + 1;
    }
  images_.push_back(im);
  return (int)images_.size();
}

// (the words as they are: on a big-endian CPU 0xAARRGGBB is the bytes AA RR GG BB, which canvas.frag reads)
void XenonCanvas::updateImage(int image, const uint32_t* px, int stridePixels) {
  if (image < 1 || image > (int)images_.size() || !images_[image - 1].tex || !px) return;
  Tex* t = images_[image - 1].tex;
  uint8_t* dst = dev_->stage(t);
  if (!dst) return;
  for (int y = 0; y < t->h; y++) std::memcpy(dst + (size_t)y * t->pitch, px + (size_t)y * stridePixels, (size_t)t->w * 4);
}

void XenonCanvas::drawImage(int image, RectF dst, float opacity, bool smooth) {
  if (image < 1 || image > (int)images_.size() || !images_[image - 1].tex) return;
  flush();
  const Rgba k = rgba(colorF(1, 1, 1, opacity));
  quad(dst.left, dst.top, dst.right, dst.bottom, k, k, 0, 0, 1, 1, nullptr, images_[image - 1].alpha ? 5.0f : 4.0f);
  flush(images_[image - 1].tex, smooth);
}

void XenonCanvas::drawTex(Tex* t, RectF dst, float u1, float v1, float opacity, bool smooth) {
  flush();
  const Rgba k = rgba(colorF(1, 1, 1, opacity));
  quad(dst.left, dst.top, dst.right, dst.bottom, k, k, 0, 0, u1, v1, nullptr, 4.0f);
  flush(t, smooth);
}

void XenonCanvas::freeImage(int image) {
  if (image < 1 || image > (int)images_.size() || !images_[image - 1].tex) return;
  flush();  // (a batch drawn with it is recorded; the texture outlives the frames in flight)
  dev_->freeTex(images_[image - 1].tex);
  images_[image - 1] = Image();
}

// ---- text ----------------------------------------------------------------------
namespace {
// A code point's glyph, or '?' 's. Ⓐ and Ⓑ (the menus' face buttons) as A and B: DejaVu has no circled letters.
template <class Map>
auto findGlyph(Map& glyphs, wchar_t wc) {
  auto it = glyphs.find((uint32_t)wc);
  if (it == glyphs.end() && (wc == 0x24B6 || wc == 0x24B7)) it = glyphs.find(wc == 0x24B6 ? 'A' : 'B');
  if (it == glyphs.end()) it = glyphs.find('?');
  return it;
}

struct Face {
  stbtt_fontinfo info;
  bool ok = false, tried = false;
};

Face& face(int which) {  // 0 the title's, 1 regular, 2 bold
  static Face faces[3];
  Face& f = faces[which];
  if (!f.tried) {
    f.tried = true;
    const unsigned char* data = which == 0 ? smas_font_title : which == 1 ? smas_font_sans : smas_font_bold;
    f.ok = stbtt_InitFont(&f.info, data, stbtt_GetFontOffsetForIndex(data, 0)) != 0;
    if (!f.ok) std::printf("video: la fuente %d no se pudo leer\n", which);
  }
  return f;
}
}  // namespace

bool XenonCanvas::buildFont(Font& f, float px, bool bold, bool title) {
  std::vector<uint32_t> cps;
  for (uint32_t c = 0x20; c < 0x7F; c++) cps.push_back(c);
  for (uint32_t c = 0xA1; c < 0x100; c++)
    if (c != 0xAD) cps.push_back(c);
  for (uint32_t c : kExtraCps) cps.push_back(c);
  Face* fc = &face(title ? 0 : bold ? 2 : 1);
  if (!fc->ok && !title) fc = &face(1);
  if (!fc->ok) fc = &face(2);
  if (!fc->ok) return false;
  const stbtt_fontinfo* info = &fc->info;
  // (FreeType's FT_Set_Pixel_Sizes: the em square at px pixels)
  const float scale = stbtt_ScaleForMappingEmToPixels(info, (float)std::max(1L, std::lround(px)));
  int asc = 0, desc = 0, gap = 0;
  stbtt_GetFontVMetrics(info, &asc, &desc, &gap);
  std::vector<int> meta(2 + cps.size() * 5, 0);
  std::vector<uint8_t> bits;
  meta[0] = (int)std::ceil(asc * scale);
  meta[1] = (int)std::ceil((asc - desc + gap) * scale);
  std::vector<uint8_t> tmp;
  for (size_t i = 0; i < cps.size(); i++) {
    int* m = &meta[2 + i * 5];
    const int g = stbtt_FindGlyphIndex(info, (int)cps[i]);
    if (g == 0 && cps[i] != ' ') {
      m[0] = -1;
      continue;
    }
    int adv = 0, lsb = 0;
    stbtt_GetGlyphHMetrics(info, g, &adv, &lsb);
    m[0] = (int)std::lround(adv * scale) * 64;  // (26.6, whole pixels as FreeType's hinted advances)
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    stbtt_GetGlyphBitmapBox(info, g, scale, scale, &x0, &y0, &x1, &y1);
    const int bw = x1 - x0, bh = y1 - y0;
    if (bw <= 0 || bh <= 0) continue;
    tmp.assign((size_t)bw * bh, 0);
    stbtt_MakeGlyphBitmap(info, tmp.data(), bw, bh, bw, scale, scale, g);
    const int w = bw + 2, h = bh + 2;  // (a pixel of room for the filtering each side)
    m[1] = x0 - 1;
    m[2] = y0 - 1;
    m[3] = w;
    m[4] = h;
    const size_t at = bits.size();
    bits.resize(at + (size_t)w * h, 0);
    for (int y = 0; y < bh; y++) std::memcpy(&bits[at + (size_t)(y + 1) * w + 1], &tmp[(size_t)y * bw], (size_t)bw);
  }
  f.ascent = meta[0];
  f.lineH = meta[1];
  // shelf-pack the bitmaps into one R8 texture (video_gles.cpp's)
  struct B {
    uint32_t cp;
    Glyph g;
    size_t off;
  };
  std::vector<B> list;
  size_t off = 0;
  for (size_t i = 0; i < cps.size(); i++) {
    const int* k = &meta[2 + i * 5];
    if (k[0] < 0) continue;
    Glyph g = {k[0] / 64.0f, k[1], k[2], k[3], k[4], 0, 0, 0, 0};
    if (g.w > 0 && off + (size_t)g.w * g.h <= bits.size()) {
      list.push_back({cps[i], g, off});
      off += (size_t)g.w * g.h;
    } else {
      g.w = g.h = 0;
      list.push_back({cps[i], g, 0});
    }
  }
  int aw = 256;
  long long area = 0;
  for (auto& b : list) area += (long long)(b.g.w + 1) * (b.g.h + 1);
  while ((long long)aw * aw < area * 3 / 2 && aw < 4096) aw *= 2;
  int x = 0, y = 0, row = 0;
  std::vector<std::pair<int, int>> at(list.size());
  for (size_t i = 0; i < list.size(); i++) {
    const Glyph& g = list[i].g;
    if (x + g.w + 1 > aw) {
      x = 0;
      y += row;
      row = 0;
    }
    at[i] = {x, y};
    x += g.w + 1;
    row = std::max(row, g.h + 1);
  }
  int ah = std::max(4, (y + row + 3) & ~3);
  f.tex = dev_->makeTex(aw, ah, true);
  if (!f.tex) return false;
  uint8_t* atlas = dev_->stage(f.tex);
  if (!atlas) return false;
  std::memset(atlas, 0, (size_t)f.tex->pitch * ah);
  for (size_t i = 0; i < list.size(); i++) {
    Glyph g = list[i].g;
    if (g.w > 0 && at[i].second + g.h <= ah) {
      for (int r = 0; r < g.h; r++)
        std::memcpy(&atlas[(size_t)(at[i].second + r) * f.tex->pitch + at[i].first], &bits[list[i].off + (size_t)r * g.w],
                    (size_t)g.w);
      g.u0 = (float)at[i].first / aw;
      g.v0 = (float)at[i].second / ah;
      g.u1 = (float)(at[i].first + g.w) / aw;
      g.v1 = (float)(at[i].second + g.h) / ah;
    }
    f.glyphs[list[i].cp] = g;
  }
  return true;
}

XenonCanvas::Font& XenonCanvas::font(float size, int weight, bool title, float* scale) {
  const int n = (int)(sizeof(kSizes) / sizeof(kSizes[0]));
  int i = 0;
  while (i < n - 1 && kSizes[i] < size * 0.96f) i++;
  *scale = size / kSizes[i];
  const bool bold = weight >= kWeightSemiBold;
  Font& f = fonts_[i * 4 + (title ? 2 : 0) + (bold ? 1 : 0)];
  if (!f.tried) {
    f.tried = true;
    f.ok = buildFont(f, kSizes[i], bold, title);
  }
  return f;
}

float XenonCanvas::measureText(const std::wstring& s, float size, int weight, bool title) {
  if (s.empty() || size <= 0) return 0;
  float scale;
  Font& f = font(size, weight, title, &scale);
  if (!f.ok) return 0;
  float w = 0;
  for (wchar_t wc : s) {
    auto it = findGlyph(f.glyphs, wc);
    if (it != f.glyphs.end()) w += it->second.adv;
  }
  return w * scale;
}

void XenonCanvas::drawText(const std::wstring& s, RectF r, float size, int weight, bool title, ColorF c,
                           TextAlign align, ParaAlign para) {
  if (s.empty() || size <= 0 || r.right <= r.left || r.bottom <= r.top) return;
  float scale;
  Font& f = font(size, weight, title, &scale);
  if (!f.ok) return;
  useAtlas(f.tex);
  const float lw = measureText(s, size, weight, title), lh = f.lineH * scale;
  float pen = align == kAlignCenter ? (r.left + r.right - lw) * 0.5f : align == kAlignTrailing ? r.right - lw : r.left;
  float top = para == kParaCenter ? (r.top + r.bottom - lh) * 0.5f : para == kParaFar ? r.bottom - lh : r.top;
  const float base = std::floor(top + f.ascent * scale + 0.5f);
  const Rgba k = rgba(c);
  for (wchar_t wc : s) {
    auto it = findGlyph(f.glyphs, wc);
    if (it == f.glyphs.end()) continue;
    const Glyph& g = it->second;
    if (g.w > 0) {
      float x0 = pen + g.ox * scale, y0 = base + g.oy * scale, x1 = x0 + g.w * scale, y1 = y0 + g.h * scale;
      float u0 = g.u0, v0 = g.v0, u1 = g.u1, v1 = g.v1;
      if (x0 < r.left) u0 += (u1 - u0) * (r.left - x0) / (x1 - x0), x0 = r.left;
      if (x1 > r.right) u1 -= (u1 - u0) * (x1 - r.right) / (x1 - x0), x1 = r.right;
      if (y0 < r.top) v0 += (v1 - v0) * (r.top - y0) / (y1 - y0), y0 = r.top;
      if (y1 > r.bottom) v1 -= (v1 - v0) * (y1 - r.bottom) / (y1 - y0), y1 = r.bottom;
      if (x1 > x0 && y1 > y0) quad(x0, y0, x1, y1, k, k, u0, v0, u1, v1, nullptr, 1.0f);
    }
    pen += g.adv * scale;
  }
}

// ---------------------------------------------------------------------------
namespace {
Tex* g_gameTex = nullptr;  // the game picture: kLineW x kFrameH words, as the PPU's frame buffer
int g_gameW = 0, g_gameH = 0;
const uint32_t* g_srcPx = nullptr;  // (the frame buffer last uploaded, for the side borders)
int g_srcStride = 0;

// Side Borders "Blurred Game" (VideoParams::pillarbox 1): video_gles.cpp's pass 1, a blurred, darkened copy of the
// picture behind it over the whole screen, made the cheap way: the picture averaged into a small image on the CPU
// (each cell 4 x 4 samples) and blurred once more (1 2 1 each way), then drawn stretched with linear filtering by
// game.frag's pass 1, which colours, darkens (x 0.32) and vignettes it as there. (49 taps a pixel at 1280 x 720 is
// much of Xenos's fill for a backdrop.)
constexpr int kBackW = 48, kBackH = 27;
Tex* g_backTex = nullptr;

void makeBackdrop(uint8_t* dst, uint32_t pitch, const uint32_t* px, int w, int h, int stride) {
  float cell[kBackH][kBackW][3];
  for (int cy = 0; cy < kBackH; cy++)
    for (int cx = 0; cx < kBackW; cx++) {
      float acc[3] = {0, 0, 0};
      for (int sy = 0; sy < 4; sy++)
        for (int sx = 0; sx < 4; sx++) {
          const int x = std::min(w - 1, (cx * 4 + sx) * w / (kBackW * 4));
          const int y = std::min(h - 1, (cy * 4 + sy) * h / (kBackH * 4));
          const uint32_t v = px[(size_t)y * stride + x];  // (0x00RRGGBB)
          acc[0] += (float)(v >> 16 & 0xff);
          acc[1] += (float)(v >> 8 & 0xff);
          acc[2] += (float)(v & 0xff);
        }
      for (int c = 0; c < 3; c++) cell[cy][cx][c] = acc[c] / 16.0f;
    }
  float tmp[kBackH][kBackW][3];
  for (int y = 0; y < kBackH; y++)
    for (int x = 0; x < kBackW; x++)
      for (int c = 0; c < 3; c++)
        tmp[y][x][c] = (cell[y][std::max(0, x - 1)][c] + 2 * cell[y][x][c] + cell[y][std::min(kBackW - 1, x + 1)][c]) / 4;
  for (int y = 0; y < kBackH; y++) {
    uint32_t* row = (uint32_t*)(dst + (size_t)y * pitch);
    for (int x = 0; x < kBackW; x++) {
      uint32_t out = 0xff000000u;
      for (int c = 0; c < 3; c++) {
        const float b =
            (tmp[std::max(0, y - 1)][x][c] + 2 * tmp[y][x][c] + tmp[std::min(kBackH - 1, y + 1)][x][c]) / 4;
        const uint32_t k = (uint32_t)std::min(255.0f, b + 0.5f);
        out |= k << (16 - 8 * c);
      }
      row[x] = out;  // (0xAARRGGBB: game.frag's picture)
    }
  }
}
}  // namespace

Video::Video() = default;
Video::~Video() { shutdown(); }

bool Video::init() {
  shutdown();
  dev_ = std::make_unique<VideoDevice>();
  if (!dev_->init()) {
    dev_->shutdown();
    dev_.reset();
    return false;
  }
  g_gameTex = dev_->makeTex(kLineW, kFrameH, false);
  g_backTex = dev_->makeTex(kBackW, kBackH, false);
  if (!g_gameTex || !g_backTex) return false;
  static unsigned epochs = 0;
  canvas_ = std::make_unique<XenonCanvas>(dev_.get(), ++epochs);
  return true;
}

void Video::shutdown() {
  if (!dev_) return;
  canvas_.reset();
  if (g_gameTex) dev_->freeTex(g_gameTex);
  if (g_backTex) dev_->freeTex(g_backTex);
  g_gameTex = g_backTex = nullptr;
  g_srcPx = nullptr;
  dev_->shutdown();
  dev_.reset();
}

Canvas* Video::canvas() { return canvas_.get(); }

void Video::upload(const uint32_t* px, int w, int h, int stride) {
  if (!dev_ || !g_gameTex || !px || w <= 0 || h <= 0) return;
  w = std::min(w, (int)kLineW);
  h = std::min(h, (int)kFrameH);
  uint8_t* dst = dev_->stage(g_gameTex);
  if (!dst) return;
  if (w == stride && w == (int)kLineW) std::memcpy(dst, px, (size_t)w * h * 4);
  else
    for (int y = 0; y < h; y++) std::memcpy(dst + (size_t)y * g_gameTex->pitch, px + (size_t)y * stride, (size_t)w * 4);
  g_gameW = w;
  g_gameH = h;
  g_srcPx = px;
  g_srcStride = stride;
}

void Video::clear() {
  if (dev_) dev_->beginFrame();  // (the render pass clears to black)
}

void Video::draw(const VideoParams& p) {
  if (!dev_ || !canvas_ || g_gameW <= 0) return;
  dev_->beginFrame();
  // game.frag's push constants, as video_gles.cpp's pass() sets its uniforms (the first vec4 present() fills)
  auto pass = [&](Tex* t, float x0, float y0, float x1, float y1, float srcW, float srcH, float which) {
    const float w = std::max(x1 - x0, 1.0f), h = std::max(y1 - y0, 1.0f);
    const float push[kGamePush] = {
        0, 0, 0, 0,
        srcW, srcH, 1.0f / t->w, 1.0f / t->h,
        w, h, 1.0f / w, 1.0f / h,
        (float)p.filter, p.scanlines, p.curve, which,
        p.brightness, p.contrast, p.saturation, std::max(p.gamma, 0.05f),
        (float)p.colorProfile, (float)p.colorblind, (float)std::max(p.srcW, 1), (float)std::max(p.srcH, 1),
        p.fade, (float)p.pillarbox, p.temperature, 0,
    };
    const uint8_t white[4] = {255, 255, 255, 255};
    const OVert v[4] = {
        {x0, y0, 0, 0, {white[0], white[1], white[2], white[3]}, {0, 0, 0, 0}, 4.0f},
        {x1, y0, 1, 0, {white[0], white[1], white[2], white[3]}, {0, 0, 0, 0}, 4.0f},
        {x0, y1, 0, 1, {white[0], white[1], white[2], white[3]}, {0, 0, 0, 0}, 4.0f},
        {x1, y1, 1, 1, {white[0], white[1], white[2], white[3]}, {0, 0, 0, 0}, 4.0f},
    };
    dev_->addGame(v, t, push);
  };
  // (the sides, where the picture doesn't reach: as video_gles.cpp's draw)
  const bool fullCover = p.dest.left <= 0 && p.dest.top <= 0 && p.dest.right >= width() && p.dest.bottom >= height();
  if (p.pillarbox == 1 && !fullCover && g_backTex && g_srcPx) {
    if (uint8_t* back = dev_->stage(g_backTex)) {
      makeBackdrop(back, g_backTex->pitch, g_srcPx, g_gameW, g_gameH, g_srcStride);
      pass(g_backTex, 0, 0, (float)width(), (float)height(), (float)kBackW, (float)kBackH, 1.0f);
    }
  }
  pass(g_gameTex, (float)p.dest.left, (float)p.dest.top, (float)p.dest.right, (float)p.dest.bottom, (float)g_gameW,
       (float)g_gameH, 0.0f);
}

void Video::beginUi() {
  if (!canvas_) return;
  dev_->beginFrame();
  canvas_->begin(width(), height());
}

void Video::endUi() {
  if (canvas_) canvas_->end();
}

void Video::present() {
  if (dev_) dev_->present();
}

bool Video::capture(std::vector<uint32_t>&, int*, int*) { return false; }  // (screenshots: hito 7)

Video::Stats Video::takeStats() {
  Stats s;
  if (dev_) {
    s = dev_->stats;
    dev_->stats = Stats();
  }
  return s;
}
