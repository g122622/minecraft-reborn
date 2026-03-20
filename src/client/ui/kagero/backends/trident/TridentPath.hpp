#pragma once

#include "../../paint/IPath.hpp"
#include <vector>

namespace mc::client::ui::kagero::backends::trident {

class TridentPath final : public paint::IPath {
public:
    struct Command {
        paint::PathCommand type = paint::PathCommand::MoveTo;
        std::vector<paint::PathPoint> points;
    };

    void reset() override;
    void moveTo(f32 x, f32 y) override;
    void lineTo(f32 x, f32 y) override;
    void quadTo(f32 x1, f32 y1, f32 x2, f32 y2) override;
    void cubicTo(f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3) override;
    void close() override;

    void addRect(const Rect& rect) override;
    void addRRect(const paint::RRect& roundRect) override;
    void addCircle(f32 cx, f32 cy, f32 radius) override;

    [[nodiscard]] bool isEmpty() const override;

    [[nodiscard]] const std::vector<Command>& commands() const { return m_commands; }

private:
    void pushCommand(paint::PathCommand type, std::initializer_list<paint::PathPoint> points);

    std::vector<Command> m_commands;
};

} // namespace mc::client::ui::kagero::backends::trident
