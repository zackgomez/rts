var renderer_binding = runtime.binding('renderer');

module.exports = {
  spawnEntity: renderer_binding.spawnRenderEntity,
  destroyEntity: renderer_binding.destroyRenderEntity,
  addEffect: renderer_binding.addEffect,
}
