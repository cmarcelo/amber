// Copyright 2018 The Amber Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/vulkan/compute_pipeline.h"

namespace amber {
namespace vulkan {

ComputePipeline::ComputePipeline(
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties& properties,
    uint32_t fence_timeout_ms,
    const std::vector<VkPipelineShaderStageCreateInfo>& shader_stage_info)
    : Pipeline(PipelineType::kCompute,
               device,
               properties,
               fence_timeout_ms,
               shader_stage_info) {}

ComputePipeline::~ComputePipeline() = default;

Result ComputePipeline::Initialize(VkCommandPool pool, VkQueue queue) {
  return Pipeline::InitializeCommandBuffer(pool, queue);
}

Result ComputePipeline::CreateVkComputePipeline() {
  const auto& shader_stage_info = GetShaderStageInfo();
  if (shader_stage_info.size() != 1) {
    return Result(
        "Vulkan::CreateVkComputePipeline number of shaders given to compute "
        "pipeline is not 1");
  }

  Result r = CreateVkDescriptorRelatedObjects();
  if (!r.IsSuccess())
    return r;

  VkComputePipelineCreateInfo pipeline_info = {};
  pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipeline_info.stage = shader_stage_info[0];
  pipeline_info.layout = pipeline_layout_;

  if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info,
                               nullptr, &pipeline_) != VK_SUCCESS) {
    return Result("Vulkan::Calling vkCreateComputePipelines Fail");
  }

  return {};
}

Result ComputePipeline::Compute(uint32_t x, uint32_t y, uint32_t z) {
  if (pipeline_ == VK_NULL_HANDLE) {
    Result r = CreateVkComputePipeline();
    if (!r.IsSuccess())
      return r;
  }

  Result r = command_->BeginIfNotInRecording();
  if (!r.IsSuccess())
    return r;

  r = SendDescriptorDataToGPUIfNeeded();
  if (!r.IsSuccess())
    return r;

  BindVkDescriptorSets();
  BindVkPipeline();

  vkCmdDispatch(command_->GetCommandBuffer(), x, y, z);
  return {};
}

}  // namespace vulkan
}  // namespace amber