#pragma once

#include "../paint/ISurface.hpp"
#include "../paint/IImage.hpp"
#include "../paint/ITypeface.hpp"
#include "../paint/IPath.hpp"
#include "../paint/IPaint.hpp"
#include "../paint/ITextBlob.hpp"
#include <memory>

namespace mc::client::ui::kagero::backend {

/**
 * @brief 渲染后端接口
 */
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    [[nodiscard]] virtual std::unique_ptr<paint::ISurface> createSurface(i32 width, i32 height) = 0;
    [[nodiscard]] virtual std::unique_ptr<paint::ISurface> createSurfaceFromWindow(void* nativeWindow) = 0;

    [[nodiscard]] virtual std::unique_ptr<paint::IImage> loadImage(const String& path) = 0;
    [[nodiscard]] virtual std::unique_ptr<paint::IImage> loadImageFromMemory(const u8* data, size_t size) = 0;
    [[nodiscard]] virtual std::unique_ptr<paint::ITypeface> loadTypeface(const String& path) = 0;
    [[nodiscard]] virtual std::unique_ptr<paint::ITypeface> loadTypefaceFromMemory(const u8* data, size_t size) = 0;

    [[nodiscard]] virtual std::unique_ptr<paint::IImage> createImage(i32 width, i32 height, paint::ImageFormat format) = 0;
    [[nodiscard]] virtual std::unique_ptr<paint::IPath> createPath() = 0;
    [[nodiscard]] virtual std::unique_ptr<paint::IPaint> createPaint() = 0;
    [[nodiscard]] virtual std::unique_ptr<paint::ITextBlob> createTextBlob(const String& text, const paint::ITypeface& typeface, f32 size) = 0;

    [[nodiscard]] virtual String getName() const = 0;
    [[nodiscard]] virtual String getAPIVersion() const = 0;
};

} // namespace mc::client::ui::kagero::backend
