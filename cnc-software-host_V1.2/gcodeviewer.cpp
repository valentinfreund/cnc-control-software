#include "gcodeviewer.h"
#include <algorithm>

GCodeViewer::GCodeViewer(QWidget *parent) : QWidget(parent) {
    this->setStyleSheet("background-color: #121212; border: 2px solid #333; border-radius: 4px;");
}

void GCodeViewer::setToolpath(const std::vector<ToolpathSegment>& path) {
    segments = path;
    update();
}

void GCodeViewer::setCurrentPosition(double x, double y) {
    toolX = x;
    toolY = y;
    showTool = true;
    update();
}

void GCodeViewer::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (segments.empty()) return;

    double minX = segments[0].x1, maxX = segments[0].x1;
    double minY = segments[0].y1, maxY = segments[0].y1;

    for (const auto& seg : segments) {
        minX = std::min({minX, seg.x1, seg.x2});
        maxX = std::max({maxX, seg.x1, seg.x2});
        minY = std::min({minY, seg.y1, seg.y2});
        maxY = std::max({maxY, seg.y1, seg.y2});
    }

    double pathWidth = maxX - minX;
    double pathHeight = maxY - minY;
    
    if (pathWidth < 1.0) pathWidth = 1.0;
    if (pathHeight < 1.0) pathHeight = 1.0;

    double scaleX = (this->width() * 0.9) / pathWidth;
    double scaleY = (this->height() * 0.9) / pathHeight;
    double scale = std::min(scaleX, scaleY);
    double centerX = minX + (pathWidth / 2.0);
    double centerY = minY + (pathHeight / 2.0);

    painter.translate(this->width() / 2.0, this->height() / 2.0);
    painter.scale(scale, -scale);
    painter.translate(-centerX, -centerY);

    QPen rapidPen(QColor("#ff7b72"), 0, Qt::DashLine);
    QPen feedPen(QColor("#79c0ff"), 0, Qt::SolidLine);

    for (const auto& seg : segments) {
        if (seg.isRapid) {
            painter.setPen(rapidPen);
        } else {
            painter.setPen(feedPen);
        }
        painter.drawLine(QPointF(seg.x1, seg.y1), QPointF(seg.x2, seg.y2));
    }

    if (showTool) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#ff0000"));
        double dotRadius = 3.0 / scale; 
        painter.drawEllipse(QPointF(toolX, toolY), dotRadius, dotRadius);
    }
}