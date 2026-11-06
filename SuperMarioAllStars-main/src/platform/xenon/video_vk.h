// The Xbox 360's video (hito 3, docs/ROADMAP.md): the game's picture on the screen through vulkan4xenon.
// A 1280x720 swapchain on the display (the console scales it to its output), FIFO (paced to vblank); the PPU's frame
// buffer (kLineW x kFrameH words, 0x00RRGGBB) copied each frame into an R8G8B8A8 texture through a staging buffer
// laid out so vulkan4xenon copies it on the GPU (docs/SUPPORT.md, "vkCmdCopyBufferToImage from the GPU's memory"),
// and drawn as one quad: 4:3 for the 256-pixel picture (8:7 pixels), wider by the margins in widescreen.
#pragma once
#include <cstdint>

#include <vulkan/vulkan.h>

class VideoVk {
public:
  bool init();  // false: the log says which call failed
  void shutdown();

  // Where the picture goes on the 1280x720 screen, in pixels.
  struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
  };
  // The PC port's "fit" layout (App::computeLayout, scaling 0): the whole picture (256 + extL + extR pixels by
  // `height` lines) as big as fits, centred (so a lopsided margin moves the 4:3 part, not the HUD the game pins to
  // the picture's edges), with 8:7 pixels (`crt`, the PC port's default) or square ones; `wantExt` the margin asked
  // for (the scale is the full wide picture's, as the PC port keeps it while the game shows a 4:3 screen).
  static Rect fitLayout(int extL, int extR, int height, int wantExt, bool crt);
  // The margin the PC port asks for to fill a screen of this aspect (0: 4:3), as App::computeLayout.
  static int wantExtFor(double aspect, bool crt, int maxExt);

  // Shows the frame buffer `frame` (kLineW x kFrameH, 0x00RRGGBB): its columns kMaxExt - extL .. kMaxExt + 256 +
  // extR, its first `height` lines (224, or 239 with overscan), in `rect`. Waits for a swapchain image (FIFO).
  bool present(const uint32_t* frame, int extL, int extR, int height, const Rect& rect);

  // A pixel of the picture on screen now, read back from the front buffer (r, g, b 0..255); false if the front buffer
  // shown isn't one of the swapchain's.
  bool screenPixel(int x, int y, int* r, int* g, int* b) const;

  // Per-frame times, in microseconds, of what present() does: copying the frame into the staging buffer, recording
  // and submitting, and waiting (for a staging buffer's fence and for the swapchain image).
  struct Stats {
    uint64_t copyUs = 0, submitUs = 0, waitUs = 0;
    uint32_t frames = 0;
  };
  Stats takeStats() {
    Stats s = stats_;
    stats_ = Stats();
    return s;
  }

private:
  static constexpr uint32_t kWidth = 1280, kHeight = 720, kImages = 2, kSlots = 2;
  VkInstance instance_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue queue_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  uint32_t imageCount_ = 0;
  VkImage images_[3] = {};
  VkImageView views_[3] = {};
  VkFramebuffer framebuffers_[3] = {};
  VkRenderPass renderPass_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout setLayout_ = VK_NULL_HANDLE;
  VkPipelineLayout layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkCommandPool pool_ = VK_NULL_HANDLE;
  VkBuffer quad_ = VK_NULL_HANDLE;
  VkDeviceMemory quadMemory_ = VK_NULL_HANDLE;
  VkImage texture_ = VK_NULL_HANDLE;
  VkDeviceMemory textureMemory_ = VK_NULL_HANDLE;
  VkImageView textureView_ = VK_NULL_HANDLE;
  VkSampler sampler_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
  VkDescriptorSet set_ = VK_NULL_HANDLE;
  bool textureReady_ = false;  // (laid out for sampling once)
  // per slot, used in turn: the staging buffer, its command buffer and the fence of its last submit
  struct Slot {
    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    VkCommandBuffer commands = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool pending = false;
  };
  Slot slots_[kSlots];
  uint32_t slot_ = 0;
  Stats stats_;

  bool createSwapchain();
  bool createPipeline();
  bool createTexture();
};
