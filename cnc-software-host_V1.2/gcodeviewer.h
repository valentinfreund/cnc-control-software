#ifndef GCODEVIEWER_H
#define GCODEVIEWER_H

#include <QWidget>
#include <QPainter>
#include <vector>

struct ToolpathSegment {
    double x1, y1;
    double x2, y2;
    bool isRapid;
};

class GCodeViewer : public QWidget
{
    Q_OBJECT
public:
    explicit GCodeViewer(QWidget *parent = nullptr);

    void setToolpath(const std::vector<ToolpathSegment>& path);
    void setCurrentPosition(double x, double y);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::vector<ToolpathSegment> segments;
    double toolX = 0.0;
    double toolY = 0.0;
    bool showTool = false;
};

#endif GCODEVIEWER_H