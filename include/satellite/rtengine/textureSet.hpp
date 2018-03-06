#pragma once

#include "./structs.hpp"

namespace NSM
{
namespace rt
{
class TextureSet
{
protected:
  friend class TextureSet;

  DeviceQueueType device;
  void init(DeviceQueueType &device);

  std::vector<size_t> freedomTextures;
  std::vector<TextureType> textures;

public:
  TextureSet() {}
  TextureSet(DeviceQueueType &device) { init(device); }
  TextureSet(TextureSet &another);
  TextureSet(TextureSet &&another);

  void freeTexture(const uint32_t &idx);
  void clearTextures();
  void setTexture(uint32_t location, const TextureType &texture);
  bool haveTextures();
  std::vector<TextureType> &getTextures();

  uint32_t loadTexture(const TextureType &texture);
  uint32_t loadTexture(std::string tex, bool force_write = false);
};
} // namespace rt
} // namespace NSM