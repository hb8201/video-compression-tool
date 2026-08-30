#include "compressworker.h"
#include <QDebug>
#include <QThread>

CompressWorker::CompressWorker(QObject *parent) : QObject(parent) {}

void CompressWorker::doCompress(const QString &inputPath,
                                const QString &outputPath,
                                int bitrate,
                                const QString &resolution,
                                const QString &format)
{
    qDebug() << "Worker: 压缩任务开始";
    qDebug() << "输入:" << inputPath;
    qDebug() << "输出:" << outputPath;
    qDebug() << "码率:" << bitrate << "kbps";
    qDebug() << "分辨率:" << resolution;
    qDebug() << "格式:" << format;

    // TODO: 在这里调用 FFmpeg 或其他压缩库，并定期发送进度信号
    // 示例：模拟进度
    for (int i = 0; i <= 100; i += 10) {
        if (m_cancelRequested) {
            emit cancelled();
            return;
        }
        QThread::msleep(200);  // 模拟耗时
        emit progressUpdated(i);
    }

    // 压缩成功
    emit finished(true, "压缩完成！");
}