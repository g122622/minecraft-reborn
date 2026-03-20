#include "TridentPath.hpp"

namespace mc::client::ui::kagero::backends::trident {

void TridentPath::reset() {
    m_commands.clear();
}

void TridentPath::moveTo(f32 x, f32 y) {
    pushCommand(paint::PathCommand::MoveTo, {{x, y}});
}

void TridentPath::lineTo(f32 x, f32 y) {
    pushCommand(paint::PathCommand::LineTo, {{x, y}});
}

void TridentPath::quadTo(f32 x1, f32 y1, f32 x2, f32 y2) {
    pushCommand(paint::PathCommand::QuadTo, {{x1, y1}, {x2, y2}});
}

void TridentPath::cubicTo(f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3) {
    pushCommand(paint::PathCommand::CubicTo, {{x1, y1}, {x2, y2}, {x3, y3}});
}

void TridentPath::close() {
    pushCommand(paint::PathCommand::Close, {});
}

void TridentPath::addRect(const Rect& rect) {
    moveTo(static_cast<f32>(rect.x), static_cast<f32>(rect.y));
    lineTo(static_cast<f32>(rect.right()), static_cast<f32>(rect.y));
    lineTo(static_cast<f32>(rect.right()), static_cast<f32>(rect.bottom()));
    lineTo(static_cast<f32>(rect.x), static_cast<f32>(rect.bottom()));
    close();
}

void TridentPath::addRRect(const paint::RRect& roundRect) {
    addRect(roundRect.rect);
}

void TridentPath::addCircle(f32 cx, f32 cy, f32 radius) {
    pushCommand(paint::PathCommand::MoveTo, {{cx + radius, cy}});
    pushCommand(paint::PathCommand::CubicTo, {
        {cx + radius, cy + radius * 0.5523f},
        {cx + radius * 0.5523f, cy + radius},
        {cx, cy + radius}
    });
    pushCommand(paint::PathCommand::CubicTo, {
        {cx - radius * 0.5523f, cy + radius},
        {cx - radius, cy + radius * 0.5523f},
        {cx - radius, cy}
    });
    pushCommand(paint::PathCommand::CubicTo, {
        {cx - radius, cy - radius * 0.5523f},
        {cx - radius * 0.5523f, cy - radius},
        {cx, cy - radius}
    });
    pushCommand(paint::PathCommand::CubicTo, {
        {cx + radius * 0.5523f, cy - radius},
        {cx + radius, cy - radius * 0.5523f},
        {cx + radius, cy}
    });
    close();
}

bool TridentPath::isEmpty() const {
    return m_commands.empty();
}

void TridentPath::pushCommand(paint::PathCommand type, std::initializer_list<paint::PathPoint> points) {
    Command command;
    command.type = type;
    command.points.assign(points.begin(), points.end());
    m_commands.push_back(std::move(command));
}

} // namespace mc::client::ui::kagero::backends::trident
