#include "TridentSurface.hpp"

namespace mc::client::ui::kagero::backends::trident {

TridentSurface::TridentSurface(i32 width, i32 height)
    : m_canvas(width, height) {}

paint::ICanvas& TridentSurface::canvas() {
    return m_canvas;
}

const paint::ICanvas& TridentSurface::canvas() const {
    return m_canvas;
}

void TridentSurface::flush() {
    m_canvas.clearCommands();
}

void TridentSurface::resize(i32 width, i32 height) {
    m_canvas.resize(width, height);
}

i32 TridentSurface::width() const {
    return m_canvas.width();
}

i32 TridentSurface::height() const {
    return m_canvas.height();
}

} // namespace mc::client::ui::kagero::backends::trident
