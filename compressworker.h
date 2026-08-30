#ifndef COMPRESSWORKER_H
#define COMPRESSWORKER_H

#include <QObject>
#include <QString>

// 工作对象，运行在子线程中
class CompressWorker : public QObject
{
    Q_OBJECT
public:
    explicit CompressWorker(QObject *parent = nullptr);

public slots:
    // 【接口】开始压缩任务，由主线程调用
    void doCompress(const QString &inputPath,
                    const QString &outputPath,
                    int bitrate,          // 码率（kbps）
                    const QString &resolution, // 如 "1920x1080"
                    const QString &format);    // 如 "mp4"

signals:
    // 发送进度 0~100
    void progressUpdated(int percent);
    // 压缩完成
    void finished(bool success, const QString &message);
    // 压缩被取消（可由工作线程检测并发出）
    void cancelled();

private:
    volatile bool m_cancelRequested = false; // 取消标志
};

#endif // COMPRESSWORKER_H
