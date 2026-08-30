#include "compressworker.h"
#include <QDebug>
#include <QThread>
#include <QRegularExpression>
#include <QDir>
#include <QStandardPaths>

CompressWorker::CompressWorker(QObject *parent)
    : QObject(parent), m_process(nullptr), m_totalDuration(0.0)
{
    m_cancelRequested.store(false);
}
// 在析构时确保进程被终止
CompressWorker::~CompressWorker()
{
    if (m_process && m_process->state() == QProcess::Running)
    {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}
// 获取视频时长
double CompressWorker::getVideoDuration(const QString &path)
{
    QProcess probe;
    QStringList args;
    args << "-v" << "error"
         << "-show_entries" << "format=duration"
         << "-of" << "default=noprint_wrappers=1:nokey=1"
         << path;

    probe.start("ffprobe", args);
    if (!probe.waitForFinished(3000))
        return 0.0;

    QByteArray output = probe.readAllStandardOutput();
    bool ok;
    double duration = output.trimmed().toDouble(&ok);
    return ok ? duration : 0.0;
}

void CompressWorker::doCompress(const QString &inputPath,
                                const QString &outputPath,
                                int bitrate,
                                const QString &resolution,
                                const QString &format)
// {
//     qDebug() << "Worker: 压缩任务开始";
//     qDebug() << "输入:" << inputPath;
//     qDebug() << "输出:" << outputPath;
//     qDebug() << "码率:" << bitrate << "kbps";
//     qDebug() << "分辨率:" << resolution;
//     qDebug() << "格式:" << format;

//     // TODO: 在这里调用 FFmpeg 或其他压缩库，并定期发送进度信号
//     // 示例：模拟进度
//     for (int i = 0; i <= 100; i += 10){
//         if (m_cancelRequested)
//         {
//             emit cancelled();
//             return;
//         }
//         QThread::msleep(200);  // 模拟耗时
//         emit progressUpdated(i);
//     }

//     // 压缩成功
//     emit finished(true, "压缩完成！");
// }
{
    // 重置取消标志
    m_cancelRequested.store(false);

    // 检查输入文件是否存在
    if (!QFile::exists(inputPath))
    {
        emit finished(false, "输入文件不存在");
        return;
    }

    // 获取总时长（用于进度计算）
    m_totalDuration = getVideoDuration(inputPath);
    if (m_totalDuration <= 0)
    {
        qDebug() << "无法获取视频时长，进度将无法精确显示";
        // 仍可继续，但进度只能显示处理时间，不能算百分比
    }

    // 构建 ffmpeg 命令行参数
    QString ffmpeg = "ffmpeg";

    // 参数列表
    QStringList args;
    args << "-i" << inputPath;
    args << "-b:v" << QString::number(bitrate) + "k";

    // 分辨率处理（"原始" 表示不缩放）
    if (resolution != "原始") {
        args << "-vf" << ("scale=" + resolution);
    }

    // 视频编码器（使用 libx264，可调整）
    args << "-c:v" << "libx264";
    args << "-preset" << "fast";

    // 音频编码（直接复制，不重新编码，提高速度）
    args << "-c:a" << "copy";

    // 输出文件（覆盖已有）
    args << "-y" << outputPath;

    qDebug() << "FFmpeg 命令:" << ffmpeg << args.join(' ');

    // 创建 QProcess
    if (m_process)
    {
        delete m_process;
        m_process = nullptr;
    }
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels); // 合并输出，便于读取进度

    // 连接 readyRead 信号以解析进度
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]()
    {
        if (!m_process) return;
        QByteArray data = m_process->readAllStandardOutput();
        QString output = QString::fromLocal8Bit(data);

        // 解析 "time=00:00:05.12" 或 "out_time_ms=5120" 等
        // ffmpeg 输出格式示例: frame= 123 fps= 25 q=28.0 size= 1024kB time=00:00:05.12 bitrate=...
        QRegularExpression re("time=([0-9]+):([0-9]+):([0-9]+)\\.([0-9]+)");
        QRegularExpressionMatch match = re.match(output);
        if (match.hasMatch())
        {
            int h = match.captured(1).toInt();
            int m = match.captured(2).toInt();
            int s = match.captured(3).toInt();
            int ms = match.captured(4).toInt();
            double currentTime = h * 3600 + m * 60 + s + ms / 100.0;

            // 计算进度百分比
            if (m_totalDuration > 0)
            {
                int percent = static_cast<int>((currentTime / m_totalDuration) * 100);
                percent = qBound(0, percent, 100);
                emit progressUpdated(percent);
            }
            else
            {
                // 无法计算百分比，可以发送当前处理时间（但 UI 只接收 int，可另作处理）
                // 这里简单发送 0~100 的递增，但不准确
                // 或者忽略
            }
        }
    });

    // 连接进程结束信号
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus)
            {
                bool success = (exitStatus == QProcess::NormalExit && exitCode == 0);
                if (m_cancelRequested.load())
                {
                    emit cancelled();
                    return;
                }
                if (success)
                {
                    emit finished(true, "压缩完成！输出：" + m_process->arguments().last());
                } else {
                    QString errorMsg = m_process->readAllStandardError();
                    emit finished(false, "压缩失败，错误码：" + QString::number(exitCode) +
                                             "\n" + errorMsg);
                }
                m_process->deleteLater();
                m_process = nullptr;
            });

    // 启动进程
    m_process->start(ffmpeg, args);
    if (!m_process->waitForStarted())
    {
        emit finished(false, "无法启动 FFmpeg，请确保已安装并添加到 PATH");
        return;
    }

    // 注意：进程运行中，进度信号由 readyRead 槽发出
    // 函数返回后，worker 线程仍保持事件循环，等待进程结束
}
// 取消槽实现
void CompressWorker::cancel()
{
    m_cancelRequested.store(true);
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->kill();           // 强制终止
        // 或者使用 terminate() 发送 SIGTERM 然后等待
    }
}


