#pragma once

#include "../../backend/IRenderBackend.hpp"

namespace mc::client::ui::kagero::backends::trident {

class TridentBackend final : public backend::IRenderBackend {
public:
    TridentBackend() = default;
    ~TridentBackend() override = default;

    [[nodiscard]] std::unique_ptr<paint::ISurface> createSurface(i32 width, i32 height) override;
    [[nodiscard]] std::unique_ptr<paint::ISurface> createSurfaceFromWindow(void* nativeWindow) override;

    [[nodiscard]] std::unique_ptr<paint::IImage> loadImage(const String& path) override;
    [[nodiscard]] std::unique_ptr<paint::IImage> loadImageFromMemory(const u8* data, size_t size) override;
    [[nodiscard]] std::unique_ptr<paint::ITypeface> loadTypeface(const String& path) override;
    [[nodiscard]] std::unique_ptr<paint::ITypeface> loadTypefaceFromMemory(const u8* data, size_t size) override;

    [[nodiscard]] std::unique_ptr<paint::IImage> createImage(i32 width, i32 height, paint::ImageFormat format) override;
    [[nodiscard]] std::unique_ptr<paint::IPath> createPath() override;
    [[nodiscard]] std::unique_ptr<paint::IPaint> createPaint() override;
    [[nodiscard]] std::unique_ptr<paint::ITextBlob> createTextBlob(const String& text, const paint::ITypeface& typeface, f32 size) override;

    [[nodiscard]] String getName() const override;
    [[nodiscard]] String getAPIVersion() const override;
};

} // namespace mc::client::ui::kagero::backends::trident
