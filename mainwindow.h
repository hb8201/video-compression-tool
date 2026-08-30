#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "compressworker.h"

QT_BEGIN_NAMESPACE
class QPushButton;
class QLineEdit;
class QComboBox;
class QProgressBar;
class QLabel;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 界面交互槽函数
    void onSelectInput();
    void onSelectOutput();
    void onStart();
    void onPause();   // 预留，可扩展
    void onCancel();

    // 从工作线程接收信号的槽
    void onProgressUpdated(int percent);
    void onCompressionFinished(bool success, const QString &message);
    void onWorkerCancelled();
    // 输出格式改变时自动补全扩展名
    void onFormatChanged(const QString &format);

private:
    // 初始化界面布局
    void setupUI();

    // 初始化工作线程
    void initWorker();

    // 更新按钮状态
    void setControlsEnabled(bool enabled);

    // UI 控件
    QLineEdit *m_inputEdit;
    QLineEdit *m_outputEdit;
    QComboBox *m_bitrateCombo;
    QComboBox *m_resolutionCombo;
    QComboBox *m_formatCombo;
    QPushButton *m_selectInputBtn;
    QPushButton *m_selectOutputBtn;
    QPushButton *m_startBtn;
    QPushButton *m_pauseBtn;
    QPushButton *m_cancelBtn;
    QProgressBar *m_progressBar;
    QLabel      *m_statusLabel;

    // 工作线程相关
    QThread          *m_workerThread;
    CompressWorker   *m_worker;
};

#endif // MAINWINDOW_H
