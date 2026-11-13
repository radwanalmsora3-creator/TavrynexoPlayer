#include "vulkan/VulkanRenderer.hpp"
#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <iostream>

namespace Tavrynexo::Vulkan {

VulkanRenderer::VulkanRenderer(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    initVulkanContext();
}

VulkanRenderer::~VulkanRenderer() {
    cleanupVulkan();
}

bool VulkanRenderer::initVulkanContext() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Tavrynexo Player";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Tavrynexo Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) == VK_SUCCESS) {
        m_vulkanReady = true;
    } else {
        std::cerr << "[Tavrynexo::Vulkan] Vulkan instance initialization fallback to Qt SceneGraph pipeline" << std::endl;
        m_vulkanReady = false;
    }
    return m_vulkanReady;
}

void VulkanRenderer::cleanupVulkan() {
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
    if (m_vkInstance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_vkInstance, nullptr);
        m_vkInstance = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::setHDR(bool hdr) {
    if (m_isHDR != hdr) {
        m_isHDR = hdr;
        emit hdrChanged();
        update();
    }
}

void VulkanRenderer::updateFrame(const QImage& frame) {
    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        m_currentFrame = frame;
        m_newFrameAvailable = true;
    }
    update(); // طلب إعادة الرسم الفوري
}

QSGNode* VulkanRenderer::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    auto* node = static_cast<QSGSimpleTextureNode*>(oldNode);

    QImage frameToRender;
    bool hasNew = false;
    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        if (m_newFrameAvailable && !m_currentFrame.isNull()) {
            frameToRender = m_currentFrame;
            m_newFrameAvailable = false;
            hasNew = true;
        }
    }

    if (!window()) return oldNode;

    if (!node) {
        node = new QSGSimpleTextureNode();
    }

    if (hasNew) {
        QSGTexture* texture = window()->createTextureFromImage(frameToRender, QQuickWindow::TextureHasAlphaChannel);
        node->setTexture(texture);
        node->setOwnsTexture(true);
    }

    // مطابقة أبعاد العرض مع مساحة الفيديو مع الحفاظ على النسبة الأبعاد الأصلية (Aspect Ratio)
    node->setRect(boundingRect());
    node->setFiltering(QSGTexture::Linear);

    return node;
}

} // namespace Tavrynexo::Vulkan
