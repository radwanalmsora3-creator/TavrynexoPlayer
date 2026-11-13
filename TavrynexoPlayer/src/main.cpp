#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>
#include <QLibrary>
#include <cstdint>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QIcon>
#include <iostream>

#include "core/PlayerController.hpp"
#include "vulkan/VulkanRenderer.hpp"

static bool isVulkanAvailable() {
#ifdef _WIN32
    QLibrary vkLib(QStringLiteral("vulkan-1"));
    if (!vkLib.load()) return false;
    auto pCreate = reinterpret_cast<PFN_vkCreateInstance>(vkLib.resolve("vkCreateInstance"));
    auto pEnum = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(vkLib.resolve("vkEnumeratePhysicalDevices"));
    auto pDestroy = reinterpret_cast<PFN_vkDestroyInstance>(vkLib.resolve("vkDestroyInstance"));
    if (!pCreate || !pEnum || !pDestroy) return false;
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Tavrynexo Probe";
    appInfo.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &appInfo;
    VkInstance inst{VK_NULL_HANDLE};
    if (pCreate(&ci, nullptr, &inst) != VK_SUCCESS || inst == VK_NULL_HANDLE) return false;
    uint32_t gpuCount = 0;
    VkResult r = pEnum(inst, &gpuCount, nullptr);
    pDestroy(inst, nullptr);
    return (r == VK_SUCCESS || r == VK_INCOMPLETE) && gpuCount > 0;
#else
    return true;
#endif
}

int main(int argc, char *argv[]) {
    // تفعيل الـ High DPI للـ 4K Monitors
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough
    );

    QGuiApplication app(argc, argv);
    app.setOrganizationName("Tavrynexo");
    app.setOrganizationDomain("tavrynexo.internal");
    app.setApplicationName("Tavrynexo Player");
    app.setApplicationVersion("1.0.0");

    // تفعيل محرك Vulkan لـ Qt Quick Scene Graph
    if (isVulkanAvailable()) {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);
    } else {
        std::cout << "[Tavrynexo] Vulkan unavailable, using Direct3D 11" << std::endl;
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
    }

    // تسجيل أنواع C++ لتكون متاحة مباشرة داخل عناصر QML
    qmlRegisterType<Tavrynexo::Core::PlayerController>("Tavrynexo.Core", 1, 0, "PlayerController");
    qmlRegisterType<Tavrynexo::Vulkan::VulkanRenderer>("Tavrynexo.Vulkan", 1, 0, "VulkanRenderer");

    QQmlApplicationEngine engine;

    // معالجة فتح الملف إذا تم تمريره كمعامل سطر أوامر (e.g. TavrynexoPlayer.exe movie.mkv)
    QString initialFile;
    if (argc > 1) {
        initialFile = QString::fromLocal8Bit(argv[1]);
    }
    engine.rootContext()->setContextProperty("initialFileArgument", initialFile);

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                std::cerr << "[Tavrynexo::Fatal] Failed to load main.qml" << std::endl;
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection
    );

    engine.load(url);

    return app.exec();
}
