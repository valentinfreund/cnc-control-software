#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTimer>
#include <map>
#include <QVector3D>
#include "gcodeviewer.h"
#include "scurve_planner.h"
#include "cnc_host_communicator.h"

class GCodeHighlighter : public QSyntaxHighlighter {
public:
    GCodeHighlighter(QTextDocument *parent = nullptr);
protected:
    void highlightBlock(const QString &text) override;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnLoadFile_clicked();
    void on_btnSaveChanges_clicked();
    void on_btnStart_clicked();
    void on_btnPause_clicked();
    void on_btnStop_clicked();
    void on_btnEstop_clicked();
    void on_btnHoming_clicked();
    void on_btnProbeZ_clicked();
    void updateUI();

private:
    void setupUI();
    void updateViewportPath();

    // gui elements
    QLabel *lblStatus;
    QLabel *lblDroX, *lblDroY, *lblDroZ;
    QPushButton *btnLoadFile, *btnSaveChanges;
    QPushButton *btnStart, *btnPause, *btnStop, *btnEstop;
    QPushButton *btnHoming;
    QPushButton *btnProbeZ;
    QPlainTextEdit *editorGCode;
    GCodeViewer *viewportViewer;
    QTimer *uiTimer;

    // logic
    SCurvePlanner planner;
    CNCHostCommunicator cncComm;
    QString currentFilePath;
    std::map<uint32_t, int> segmentToLine;
    std::map<uint32_t, QVector3D> segmentToPos;

    double hostTargetX = 0.0;
    double hostTargetY = 0.0;
    double hostTargetZ = 0.0;
    uint8_t previous_state = 1;
};

#endif // MAINWINDOW_H