#pragma once

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>
#include <QLibrary>
#include <QQuickItem>
#include <QSGSimpleTextureNode>
#include <QImage>
#include <mutex>

namespace Tavrynexo::Vulkan {

class VulkanRenderer : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(bool isHDR READ isHDR WRITE setHDR NOTIFY hdrChanged)

public:
    explicit VulkanRenderer(QQuickItem* parent = nullptr);
    ~VulkanRenderer() override;

    bool isHDR() const { return m_isHDR; }
    void setHDR(bool hdr);

public slots:
    void updateFrame(const QImage& frame);

signals:
    void hdrChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updatePaintNodeData) override;

private:
    bool initVulkanContext();
    void cleanupVulkan();

    VkInstance m_vkInstance{VK_NULL_HANDLE};
    VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
    VkDevice m_device{VK_NULL_HANDLE};
    QLibrary m_vulkanLib;
    PFN_vkDestroyDevice m_vkDestroyDevice{nullptr};
    PFN_vkDestroyInstance m_vkDestroyInstance{nullptr};
    bool m_vulkanReady{false};

    QImage m_currentFrame;
    std::mutex m_frameMutex;
    bool m_newFrameAvailable{false};
    bool m_isHDR{false};
};

} // namespace Tavrynexo::Vulkan
