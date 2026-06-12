#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QRegularExpression>
#include <QFont>

// code editor highlighter
GCodeHighlighter::GCodeHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent) {}
void GCodeHighlighter::highlightBlock(const QString &text) {
    QTextCharFormat format;
    format.setForeground(Qt::white);
    setFormat(0, text.length(), format);

    if (text.startsWith(";")) {
        format.setForeground(QColor("#777777"));
        format.setFontItalic(true);
        setFormat(0, text.length(), format);
        return;
    }
    if (text.contains("G0")) {
        format.setForeground(QColor("#ff7b72"));
        setFormat(0, text.length(), format);
    }
    else if (text.contains("G1") || text.contains("G2") || text.contains("G3")) {
        format.setForeground(QColor("#79c0ff"));
        setFormat(0, text.length(), format);
    }
    else if (text.contains("M")) {
        format.setForeground(QColor("#d2a8ff"));
        setFormat(0, text.length(), format);
    }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    
    uiTimer = new QTimer(this);
    connect(uiTimer, &QTimer::timeout, this, &MainWindow::updateUI);
    uiTimer->start(50); 

    cncComm.StartProcessingThread();
    
    // show tool path with dummy file
    viewportViewer->setCurrentPosition(0.0, 0.0);
    currentFilePath = "blank.nc";
    QFile file(currentFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        editorGCode->setPlainText(in.readAll());
        file.close();
        btnSaveChanges->setEnabled(true);
        updateViewportPath();
    }
}

MainWindow::~MainWindow() {
    cncComm.StopProcessingThread();
}

void MainWindow::setupUI() {
    this->resize(1024, 600);
    this->setWindowTitle("Zwergenberg C++ CNC Software");
    this->setStyleSheet("background-color: #1a1a1a; color: #ffffff; font-family: Arial;");

    QWidget *centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);
    QHBoxLayout *bodyLayout = new QHBoxLayout(centralWidget);

    // dro + status
    QVBoxLayout *leftPanelLayout = new QVBoxLayout();

    lblStatus = new QLabel("STATUS: BEREIT");
    lblStatus->setStyleSheet("background-color: #005a9e; color: white; padding: 10px; "
                             "border-radius: 4px; font-weight: bold; font-size: 24px; "
                             "qproperty-alignment: 'AlignHCenter | AlignVCenter';");
    leftPanelLayout->addWidget(lblStatus);

    lblDroX = new QLabel("X +000.000"); lblDroY = new QLabel("Y +000.000"); lblDroZ = new QLabel("Z +000.000");
    QString droStyle = "font-family: Courier; font-size: 36px; font-weight: bold; color: #ffffffff; background-color: #000000; padding: 10px; border-radius: 4px;";
    lblDroX->setStyleSheet(droStyle); lblDroY->setStyleSheet(droStyle); lblDroZ->setStyleSheet(droStyle);
    leftPanelLayout->addWidget(lblDroX); leftPanelLayout->addWidget(lblDroY); leftPanelLayout->addWidget(lblDroZ);
    
    btnEstop = new QPushButton("NOT-HALT (E-STOP)");
    btnEstop->setStyleSheet("background-color: #c92a2a; color: white; font-size: 18px; font-weight: bold; padding: 15px; border-radius: 4px;");
    leftPanelLayout->addWidget(btnEstop);

    // homing and z-probe
    QHBoxLayout *machineLayout = new QHBoxLayout();
    btnHoming = new QPushButton("HOMING");
    btnHoming->setStyleSheet("background-color: #4a4a4a; color: white; font-size: 14px; font-weight: bold; padding: 10px; border-radius: 4px;");
    btnProbeZ = new QPushButton("Z-ANTASTEN");
    btnProbeZ->setStyleSheet("background-color: #4a4a4a; color: white; font-size: 14px; font-weight: bold; padding: 10px; border-radius: 4px;");
    machineLayout->addWidget(btnHoming);
    machineLayout->addWidget(btnProbeZ);
    leftPanelLayout->addLayout(machineLayout);
    
    bodyLayout->addLayout(leftPanelLayout, 1);

    // viewport
    viewportViewer = new GCodeViewer();
    bodyLayout->addWidget(viewportViewer, 6);

    // editor
    QVBoxLayout *controlLayout = new QVBoxLayout();
    btnLoadFile = new QPushButton("G-CODE LADEN...");
    btnLoadFile->setStyleSheet("background-color: #4a4a4a; font-size: 14px; padding: 5px;");
    btnSaveChanges = new QPushButton("ÄNDERUNGEN SPEICHERN");
    btnSaveChanges->setStyleSheet("background-color: #005a9e; color: white; font-size: 14px; padding: 5px;");
    btnSaveChanges->setEnabled(false);

    editorGCode = new QPlainTextEdit();
    editorGCode->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: Courier; font-size: 14px; border: 1px solid #333;");
    editorGCode->setLineWrapMode(QPlainTextEdit::NoWrap);
    new GCodeHighlighter(editorGCode->document());

    // control
    btnStart = new QPushButton("PROGRAMM STARTEN"); btnStart->setStyleSheet("background-color: #2b8a3e; color: white; padding: 10px;");
    btnPause = new QPushButton("PAUSE"); btnPause->setStyleSheet("background-color: #e67700; color: white; padding: 10px;");
    btnStop = new QPushButton("PROGRAMM ABBRECHEN"); btnStop->setStyleSheet("background-color: #555555; color: white; padding: 10px;");

    controlLayout->addWidget(btnLoadFile); controlLayout->addWidget(btnSaveChanges);
    controlLayout->addWidget(editorGCode);
    controlLayout->addWidget(btnStart); controlLayout->addWidget(btnPause); controlLayout->addWidget(btnStop);
    bodyLayout->addLayout(controlLayout, 2);

    connect(btnLoadFile, &QPushButton::clicked, this, &MainWindow::on_btnLoadFile_clicked);
    connect(btnSaveChanges, &QPushButton::clicked, this, &MainWindow::on_btnSaveChanges_clicked);
    connect(btnStart, &QPushButton::clicked, this, &MainWindow::on_btnStart_clicked);
    connect(btnPause, &QPushButton::clicked, this, &MainWindow::on_btnPause_clicked);
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::on_btnStop_clicked);
    connect(btnEstop, &QPushButton::clicked, this, &MainWindow::on_btnEstop_clicked);
    connect(btnHoming, &QPushButton::clicked, this, &MainWindow::on_btnHoming_clicked);
    connect(btnProbeZ, &QPushButton::clicked, this, &MainWindow::on_btnProbeZ_clicked);
}

// path visualization
void MainWindow::updateViewportPath() {
    QStringList lines = editorGCode->toPlainText().split("\n");
    std::vector<ToolpathSegment> parsedPath;
    double currentPosX = 0.0, currentPosY = 0.0;
    bool isRapid = true;
    QRegularExpression rxX("X([-+]?[0-9]*\\.?[0-9]+)"), rxY("Y([-+]?[0-9]*\\.?[0-9]+)");

    for (const QString &line : lines) {
        QString l = line.trimmed().toUpper();
        if (l.isEmpty() || l.startsWith(";")) continue;
        if (l.contains("G0")) isRapid = true;
        if (l.contains("G1") || l.contains("G2") || l.contains("G3")) isRapid = false;

        QRegularExpressionMatch matchX = rxX.match(l), matchY = rxY.match(l);
        bool posChanged = false;
        double targetX = currentPosX, targetY = currentPosY;
        if (matchX.hasMatch()) { targetX = matchX.captured(1).toDouble(); posChanged = true; }
        if (matchY.hasMatch()) { targetY = matchY.captured(1).toDouble(); posChanged = true; }

        if (posChanged) {
            ToolpathSegment seg;
            seg.x1 = currentPosX; seg.y1 = currentPosY; seg.x2 = targetX; seg.y2 = targetY;
            seg.isRapid = isRapid;
            parsedPath.push_back(seg);
            currentPosX = targetX; currentPosY = targetY;
        }
    }
    viewportViewer->setToolpath(parsedPath);
}

void MainWindow::on_btnLoadFile_clicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "G-Code laden", "", "G-Code (*.nc *.gcode *.txt);;Alle Dateien (*)");
    if (fileName.isEmpty()) return; 
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    currentFilePath = fileName;
    QTextStream in(&file);
    editorGCode->setPlainText(in.readAll());
    segmentToLine.clear();
    segmentToPos.clear();
    editorGCode->setExtraSelections(QList<QTextEdit::ExtraSelection>());
    file.close();
    btnSaveChanges->setEnabled(true);
    updateViewportPath();
}

void MainWindow::on_btnSaveChanges_clicked() {
    if (currentFilePath.isEmpty()) return;
    QFile file(currentFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out << editorGCode->toPlainText();
    file.close();
    updateViewportPath();
}

void MainWindow::on_btnStart_clicked() {
    SPI_Status_Response_t status = cncComm.GetLastStatus();
    if (((status.status >> 4) & 0x0F) == STATE_ESTOP) return;
    if (((status.status >> 4) & 0x0F) == STATE_HOLD) { cncComm.SendResume(); return; }

    segmentToLine.clear(); segmentToPos.clear();
    QStringList lines = editorGCode->toPlainText().split("\n");
    double currentX = hostTargetX, currentY = hostTargetY, currentZ = hostTargetZ, currentF = 1000.0;
    QRegularExpression rxX("X([-+]?[0-9]*\\.?[0-9]+)"), rxY("Y([-+]?[0-9]*\\.?[0-9]+)"), rxZ("Z([-+]?[0-9]*\\.?[0-9]+)"), rxF("F([-+]?[0-9]*\\.?[0-9]+)");

    for(int row = 0; row < lines.size(); ++row) {
        QString line = lines[row].toUpper();
        if (line.isEmpty() || line.startsWith(";")) continue;
        QRegularExpressionMatch matchF = rxF.match(line);
        if (matchF.hasMatch()) currentF = matchF.captured(1).toDouble();

        double targetX = currentX, targetY = currentY, targetZ = currentZ;
        QRegularExpressionMatch mX = rxX.match(line), mY = rxY.match(line), mZ = rxZ.match(line);
        if (mX.hasMatch()) targetX = mX.captured(1).toDouble();
        if (mY.hasMatch()) targetY = mY.captured(1).toDouble();
        if (mZ.hasMatch()) targetZ = mZ.captured(1).toDouble();

        double dx = targetX - currentX, dy = targetY - currentY, dz = targetZ - currentZ;
        if (dx == 0 && dy == 0 && dz == 0) continue;

        double feed = line.contains("G0") ? 5000.0 : currentF;
        std::vector<SPI_Poly_Segment_t> generated = planner.PlanLinearMove(dx, dy, dz, dz, feed);

        int N = generated.size();
        for (int i = 0; i < N; ++i) {
            uint32_t id = cncComm.GetNextSegmentID() + i;
            double ratio = (double)(i + 1) / N;
            segmentToPos[id] = QVector3D(currentX + (ratio * dx), currentY + (ratio * dy), currentZ + (ratio * dz));
            segmentToLine[id] = row;
        }
        for (const auto& seg : generated) cncComm.AddSegment(seg);
        currentX = targetX; currentY = targetY; currentZ = targetZ;
    }
}

void MainWindow::on_btnPause_clicked() { cncComm.SendPause(); }

void MainWindow::on_btnStop_clicked() {
    cncComm.ClearQueue();
    segmentToLine.clear(); 
    segmentToPos.clear();
}
void MainWindow::on_btnEstop_clicked() { cncComm.SendEmergencyStop(); }

void MainWindow::on_btnHoming_clicked() {
    SPI_Status_Response_t status = cncComm.GetLastStatus();
    if (((status.status >> 4) & 0x0F) != STATE_IDLE) return;

    cncComm.SendSystemCommand(CNC_CMD_START_HOMING); 
}

void MainWindow::on_btnProbeZ_clicked() {
    SPI_Status_Response_t status = cncComm.GetLastStatus();

    if (((status.status >> 4) & 0x0F) != 1) return;

    double dz = -20.0;
    std::vector<SPI_Poly_Segment_t> segments = planner.PlanLinearMove(0, 0, dz, dz, 100.0);
    int N = segments.size();

    for (int i = 0; i < N; ++i) {
        uint32_t id = cncComm.GetNextSegmentID() + i;
        segments[i].flags = CNC_CMD_PROBE;
        
        double ratio = (double)(i + 1) / N;
        segmentToPos[id] = QVector3D(hostTargetX, hostTargetY, hostTargetZ + (ratio * dz));
        segmentToLine[id] = -1; 
    }
    
    for (const auto& seg : segments) {
        cncComm.AddSegment(seg);
    }
}

void MainWindow::updateUI() {
    SPI_Status_Response_t status = cncComm.GetLastStatus();
    uint8_t state = (status.status >> 4) & 0x0F;
    uint32_t current_id = status.last_processed_id;
    bool isIdle = (state == STATE_IDLE), isRunning = (state == STATE_RUNNING), isHold = (state == STATE_HOLD), isEstop = (state == STATE_ESTOP);
    
    // host synchronisation
    if (isIdle && current_id > 0) {
        if (segmentToPos.count(current_id)) {
            QVector3D p = segmentToPos[current_id];
            hostTargetX = p.x(); hostTargetY = p.y(); hostTargetZ = p.z();
        }
    }

    // homing
    if (previous_state == STATE_HOMING && isIdle) { 
        hostTargetX = 0.0; hostTargetY = 0.0; hostTargetZ = 0.0;
        segmentToPos.clear();
        viewportViewer->setCurrentPosition(0.0, 0.0);
        lblDroX->setText("X +000.000"); lblDroY->setText("Y +000.000"); lblDroZ->setText("Z +000.000");
    }
    previous_state = state;

    // button guarding
    btnStart->setEnabled(isIdle || isHold); 
    btnPause->setEnabled(isRunning); 
    btnStop->setEnabled(isRunning || isHold); 
    btnLoadFile->setEnabled(isIdle);
    btnSaveChanges->setEnabled(isIdle && !currentFilePath.isEmpty());
    editorGCode->setReadOnly(!isIdle);
    
    // hold safety
    if (isHold) btnStart->setText("FORTSETZEN"); else btnStart->setText("PROGRAMM STARTEN");
    if (btnHoming) btnHoming->setEnabled(isIdle);
    if (btnProbeZ) btnProbeZ->setEnabled(isIdle);

    // status
    QString baseStyle = "color: white; padding: 10px; border-radius: 4px; font-weight: bold; font-size: 24px; qproperty-alignment: 'AlignHCenter | AlignVCenter';";

    if (isRunning) { 
        lblStatus->setText("STATUS: LAUFEND"); 
        lblStatus->setStyleSheet("background-color: #2b8a3e; " + baseStyle); 
    }
    else if (isHold) { 
        lblStatus->setText("STATUS: PAUSE"); 
        lblStatus->setStyleSheet("background-color: #e67700; " + baseStyle); 
    }
    else if (isEstop) { 
        lblStatus->setText("STATUS: NOT-HALT"); 
        lblStatus->setStyleSheet("background-color: #c92a2a; " + baseStyle); 
    }
    else if (state == STATE_HOMING) {
        lblStatus->setText("STATUS: REFERENZ"); 
        lblStatus->setStyleSheet("background-color: #7c3aed; " + baseStyle); 
    }
    else { 
        lblStatus->setText("STATUS: BEREIT"); 
        lblStatus->setStyleSheet("background-color: #005a9e; " + baseStyle); 
    }

    // path visualization
    if (current_id > 0 && !isEstop) {
        if (segmentToLine.count(current_id)) {
            int activeRow = segmentToLine[current_id];
            QTextDocument *doc = editorGCode->document();
            QTextBlock block = doc->findBlockByLineNumber(activeRow);
            
            if (block.isValid()) {
                QTextCursor cursor(block);
                editorGCode->setTextCursor(cursor);
                QList<QTextEdit::ExtraSelection> selections;
                QTextEdit::ExtraSelection sel;
                sel.format.setBackground(QColor("#1f4e79")); 
                sel.format.setProperty(QTextFormat::FullWidthSelection, true);
                sel.cursor = cursor; 
                sel.cursor.clearSelection();
                selections.append(sel); 
                editorGCode->setExtraSelections(selections);
                
            } else {
                editorGCode->setExtraSelections(QList<QTextEdit::ExtraSelection>());
            }
        }
        if (segmentToPos.count(current_id)) {
            QVector3D pos = segmentToPos[current_id];
            viewportViewer->setCurrentPosition(pos.x(), pos.y());
            lblDroX->setText(QString::asprintf("X %+08.3f", pos.x()));
            lblDroY->setText(QString::asprintf("Y %+08.3f", pos.y()));
            lblDroZ->setText(QString::asprintf("Z %+08.3f", pos.z()));
        }
    }
}