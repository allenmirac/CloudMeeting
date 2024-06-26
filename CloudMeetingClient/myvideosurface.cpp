#include "myvideosurface.h"

MyVideoSurface::MyVideoSurface(QObject *parent) : QAbstractVideoSurface(parent)
{

}

QList<QVideoFrame::PixelFormat> MyVideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
    // 视频缓冲区句柄格式
    if(handleType == QAbstractVideoBuffer::NoHandle){
        return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32
                                                 << QVideoFrame::Format_ARGB32
                                                 << QVideoFrame::Format_ARGB32_Premultiplied
                                                 << QVideoFrame::Format_RGB565
                                                 << QVideoFrame::Format_RGB555;
    }
    else {
        return QList<QVideoFrame::PixelFormat>();
    }
}

bool MyVideoSurface::isFormatSupported(const QVideoSurfaceFormat &format) const
{
    // imageFormatFromPixelFormat: 返回与视频帧像素格式等效的图像格式
    // pixelFormat: 返回视频流中的像素格式
    return QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat()) != QImage::Format_Invalid;
}

bool MyVideoSurface::start(const QVideoSurfaceFormat &format)
{
    if(isFormatSupported(format) && !format.frameSize().isEmpty()) {
        QAbstractVideoSurface::start(format);
        return true;
    }
    return false;
}

bool MyVideoSurface::present(const QVideoFrame &frame)
{
    if(!frame.isValid()) {
        stop();
        return false;
    }
    // 类似mmap(), 和munmap()
    if(frame.isMapped()) {
        emit frameAvailable(frame);
    } else {
        QVideoFrame f(frame);
        f.map(QAbstractVideoBuffer::ReadOnly);
        emit frameAvailable(f);
    }

    return true;
}

QRect MyVideoSurface::videoRect() const
{

}

void MyVideoSurface::updateVideoRect()
{

}

void MyVideoSurface::paint(QPainter *)
{

}
