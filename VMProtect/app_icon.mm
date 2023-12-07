#include <QtGui/QIcon>
#include <AppKit/AppKit.h>

/*
    Helper class that automates refernce counting for CFtypes.
    After constructing the QCFType object, it can be copied like a
    value-based type.

    Note that you must own the object you are wrapping.
    This is typically the case if you get the object from a Core
    Foundation function with the word "Create" or "Copy" in it. If
    you got the object from a "Get" function, either retain it or use
    constructFromGet(). One exception to this rule is the
    HIThemeGet*Shape functions, which in reality are "Copy" functions.
*/
template <typename T>
class QCFType
{
public:
    inline QCFType(const T &t = 0) : type(t) {}
    inline QCFType(const QCFType &helper) : type(helper.type) { if (type) CFRetain(type); }
    inline ~QCFType() { if (type) CFRelease(type); }
    inline operator T() { return type; }
    inline QCFType operator =(const QCFType &helper)
    {
        if (helper.type)
            CFRetain(helper.type);
        CFTypeRef type2 = type;
        type = helper.type;
        if (type2)
            CFRelease(type2);
        return *this;
    }
    inline T *operator&() { return &type; }
    template <typename X> X as() const { return reinterpret_cast<X>(type); }
    static QCFType constructFromGet(const T &t)
    {
        CFRetain(t);
        return QCFType<T>(t);
    }
protected:
    T type;
};

CGColorSpaceRef qt_mac_genericColorSpace()
{
    CGDirectDisplayID displayID = CGMainDisplayID();
    CGColorSpaceRef colorSpace = CGDisplayCopyColorSpace(displayID);
    if (colorSpace == 0)
        colorSpace = CGColorSpaceCreateDeviceRGB();

    return colorSpace;
}

static void qt_mac_deleteImage(void *image, const void *, size_t)
{
    delete static_cast<QImage *>(image);
}

// Creates a CGDataProvider with the data from the given image.
// The data provider retains a copy of the image.
CGDataProviderRef qt_mac_CGDataProvider(const QImage &image)
{
    return CGDataProviderCreateWithData(new QImage(image), image.bits(),
                                        image.byteCount(), qt_mac_deleteImage);
}

CGImageRef qt_mac_toCGImage(const QImage &inImage)
{
    if (inImage.isNull())
        return 0;

    QImage image = inImage;

    uint cgflags = kCGImageAlphaNone;
    switch (image.format()) {
    case QImage::Format_ARGB32:
        cgflags = kCGImageAlphaFirst | kCGBitmapByteOrder32Host;
        break;
    case QImage::Format_RGB32:
        cgflags = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host;
        break;
    case QImage::Format_RGB888:
        cgflags = kCGImageAlphaNone | kCGBitmapByteOrder32Big;
        break;
    case QImage::Format_RGBA8888_Premultiplied:
        cgflags = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big;
        break;
    case QImage::Format_RGBA8888:
        cgflags = kCGImageAlphaLast | kCGBitmapByteOrder32Big;
        break;
    case QImage::Format_RGBX8888:
        cgflags = kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big;
        break;
    default:
        // Everything not recognized explicitly is converted to ARGB32_Premultiplied.
        image = inImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        // no break;
    case QImage::Format_ARGB32_Premultiplied:
        cgflags = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host;
        break;
    }

    QCFType<CGDataProviderRef> dataProvider = qt_mac_CGDataProvider(image);
    return CGImageCreate(image.width(), image.height(), 8, 32,
                         image.bytesPerLine(),
                         qt_mac_genericColorSpace(),
                         cgflags, dataProvider, 0, false, kCGRenderingIntentDefault);
}

NSImage *qt_mac_create_nsimage(const QIcon &icon)
{
    if (icon.isNull())
        return nil;

    NSImage *nsImage = [[NSImage alloc] init];
    foreach (QSize size, icon.availableSizes()) {
        QPixmap pm = icon.pixmap(size);
        QImage image = pm.toImage();
        CGImageRef cgImage = qt_mac_toCGImage(image);
        NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
        [nsImage addRepresentation:imageRep];
        [imageRep release];
        CGImageRelease(cgImage);
    }
    return nsImage;
}

void qt_mac_set_app_icon(const QIcon &icon)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSImage *image = qt_mac_create_nsimage(icon);
    [NSApp setApplicationIconImage:image];
    [image release];
    [pool release];
}

