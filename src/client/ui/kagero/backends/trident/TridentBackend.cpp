#include "TridentBackend.hpp"

#include "TridentSurface.hpp"
#include "TridentImage.hpp"
#include "TridentPaint.hpp"
#include "TridentPath.hpp"
#include "TridentTypeface.hpp"
#include "TridentTextBlob.hpp"

namespace mc::client::ui::kagero::backends::trident {

std::unique_ptr<paint::ISurface> TridentBackend::createSurface(i32 width, i32 height) {
    return std::make_unique<TridentSurface>(width, height);
}

std::unique_ptr<paint::ISurface> TridentBackend::createSurfaceFromWindow(void* nativeWindow) {
    (void)nativeWindow;
    return std::make_unique<TridentSurface>(1, 1);
}

std::unique_ptr<paint::IImage> TridentBackend::loadImage(const String& path) {
    return std::make_unique<TridentImage>(1, 1, paint::ImageFormat::RGBA8, path);
}

std::unique_ptr<paint::IImage> TridentBackend::loadImageFromMemory(const u8* data, size_t size) {
    (void)data;
    return std::make_unique<TridentImage>(1, 1, paint::ImageFormat::RGBA8, "memory:" + std::to_string(size));
}

std::unique_ptr<paint::ITypeface> TridentBackend::loadTypeface(const String& path) {
    return std::make_unique<TridentTypeface>(path, false, false);
}

std::unique_ptr<paint::ITypeface> TridentBackend::loadTypefaceFromMemory(const u8* data, size_t size) {
    (void)data;
    return std::make_unique<TridentTypeface>("memory-font-" + std::to_string(size), false, false);
}

std::unique_ptr<paint::IImage> TridentBackend::createImage(i32 width, i32 height, paint::ImageFormat format) {
    return std::make_unique<TridentImage>(width, height, format, "runtime-image");
}

std::unique_ptr<paint::IPath> TridentBackend::createPath() {
    return std::make_unique<TridentPath>();
}

std::unique_ptr<paint::IPaint> TridentBackend::createPaint() {
    return std::make_unique<TridentPaint>();
}

std::unique_ptr<paint::ITextBlob> TridentBackend::createTextBlob(const String& text, const paint::ITypeface& typeface, f32 size) {
    return std::make_unique<TridentTextBlob>(text, typeface, size);
}

String TridentBackend::getName() const {
    return "TridentBackend";
}

String TridentBackend::getAPIVersion() const {
    return "Vulkan-1.3";
}

} // namespace mc::client::ui::kagero::backends::trident
