

#include "toonzqt/gutil.h"

// TnzQt includes
#include "toonzqt/dvdialog.h"

// TnzCore includes
#include "traster.h"
#include "tpixelutils.h"
#include "tfilepath.h"
#include "tfiletype.h"
#include "tstroke.h"
#include "tcurves.h"
#include "trop.h"
#include "tmsgcore.h"
#include "toonz/preferences.h"

// Qt includes
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QIcon>
#include <QString>
#include <QApplication>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QKeyEvent>
#include <QUrl>
#include <QFileInfo>
#include <QDesktopWidget>
#include <QSvgRenderer>

using namespace DVGui;

//-----------------------------------------------------------------------------

QString fileSizeString(qint64 size, int precision) {
  if (size < 1024)
    return QString::number(size) + " Bytes";
  else if (size < 1024 * 1024)
    return QString::number(size / (1024.0), 'f', precision) + " KB";
  else if (size < 1024 * 1024 * 1024)
    return QString::number(size / (1024 * 1024.0), 'f', precision) + " MB";
  else
    return QString::number(size / (1024 * 1024 * 1024.0), 'f', precision) +
           " GB";
}

//----------------------------------------------------------------

QImage rasterToQImage(const TRasterP &ras, bool premultiplied, bool mirrored) {
  if (TRaster32P ras32 = ras) {
    QImage image(ras->getRawData(), ras->getLx(), ras->getLy(),
                 premultiplied ? QImage::Format_ARGB32_Premultiplied
                               : QImage::Format_ARGB32);
    if (mirrored) return image.mirrored();
    return image;
  } else if (TRasterGR8P ras8 = ras) {
    QImage image(ras->getRawData(), ras->getLx(), ras->getLy(), ras->getWrap(),
                 QImage::Format_Indexed8);
    static QVector<QRgb> colorTable;
    if (colorTable.size() == 0) {
      int i;
      for (i = 0; i < 256; i++) colorTable.append(QColor(i, i, i).rgb());
    }
    image.setColorTable(colorTable);
    if (mirrored) return image.mirrored();
    return image;
  }
  return QImage();
}

//-----------------------------------------------------------------------------

QPixmap rasterToQPixmap(const TRaster32P &ras, bool premultiplied,
                        bool setDevPixRatio) {
  QPixmap pixmap = QPixmap::fromImage(rasterToQImage(ras, premultiplied));
  if (setDevPixRatio) {
    pixmap.setDevicePixelRatio(getDevPixRatio());
  }
  return pixmap;
}

//-----------------------------------------------------------------------------

TRaster32P rasterFromQImage(
    QImage image, bool premultiply,
    bool mirror)  // no need of const& - Qt uses implicit sharing...
{
  QImage copyImage = mirror ? image.mirrored() : image;
  TRaster32P ras(image.width(), image.height(), image.width(),
                 (TPixelRGBM32 *)copyImage.bits(), false);
  if (premultiply) TRop::premultiply(ras);
  return ras->clone();
}

//-----------------------------------------------------------------------------

TRaster32P rasterFromQPixmap(
    QPixmap pixmap, bool premultiply,
    bool mirror)  // no need of const& - Qt uses implicit sharing...
{
  QImage image = pixmap.toImage();
  return rasterFromQImage(image, premultiply, mirror);
}

//-----------------------------------------------------------------------------

void drawPolygon(QPainter &p, const std::vector<QPointF> &points, bool fill,
                 const QColor colorFill, const QColor colorLine) {
  if (points.size() == 0) return;
  p.setPen(colorLine);
  QPolygonF E0Polygon;
  int i = 0;
  for (i = 0; i < (int)points.size(); i++) E0Polygon << QPointF(points[i]);
  E0Polygon << QPointF(points[0]);

  QPainterPath E0Path;
  E0Path.addPolygon(E0Polygon);
  if (fill) p.fillPath(E0Path, QBrush(colorFill));
  p.drawPath(E0Path);
}

//-----------------------------------------------------------------------------

void drawArrow(QPainter &p, const QPointF a, const QPointF b, const QPointF c,
               bool fill, const QColor colorFill, const QColor colorLine) {
  std::vector<QPointF> pts;
  pts.push_back(a);
  pts.push_back(b);
  pts.push_back(c);
  drawPolygon(p, pts, fill, colorFill, colorLine);
}

//-----------------------------------------------------------------------------

QPixmap scalePixmapKeepingAspectRatio(QPixmap pixmap, QSize size,
                                      QColor color) {
  if (pixmap.isNull()) return pixmap;
  if (pixmap.devicePixelRatio() > 1.0) size *= pixmap.devicePixelRatio();
  if (pixmap.size() == size) return pixmap;
  QPixmap scaledPixmap =
      pixmap.scaled(size.width(), size.height(), Qt::KeepAspectRatio,
                    Qt::SmoothTransformation);
  QPixmap newPixmap(size);
  newPixmap.fill(color);
  QPainter painter(&newPixmap);
  painter.drawPixmap(double(size.width() - scaledPixmap.width()) * 0.5,
                     double(size.height() - scaledPixmap.height()) * 0.5,
                     scaledPixmap);
  newPixmap.setDevicePixelRatio(pixmap.devicePixelRatio());
  return newPixmap;
}

//-----------------------------------------------------------------------------

QPixmap svgToPixmap(const QString &svgFilePath, const QSize &size,
                    Qt::AspectRatioMode aspectRatioMode, QColor bgColor) {
  static int devPixRatio = getDevPixRatio();
  QSvgRenderer svgRenderer(svgFilePath);
  QSize pixmapSize;
  QRectF renderRect;
  if (size.isEmpty()) {
    pixmapSize = svgRenderer.defaultSize() * devPixRatio;
    renderRect = QRectF(QPointF(), QSizeF(pixmapSize));
  } else {
    pixmapSize = size * devPixRatio;
    if (aspectRatioMode == Qt::KeepAspectRatio ||
        aspectRatioMode == Qt::KeepAspectRatioByExpanding) {
      QSize imgSize = svgRenderer.defaultSize();
      QPointF scaleFactor((float)pixmapSize.width() / (float)imgSize.width(),
                          (float)pixmapSize.height() / (float)imgSize.height());
      float factor = (aspectRatioMode == Qt::KeepAspectRatio)
                         ? std::min(scaleFactor.x(), scaleFactor.y())
                         : std::max(scaleFactor.x(), scaleFactor.y());
      QSizeF renderSize(factor * (float)imgSize.width(),
                        factor * (float)imgSize.height());
      QPointF topLeft(
          ((float)pixmapSize.width() - renderSize.width()) * 0.5f,
          ((float)pixmapSize.height() - renderSize.height()) * 0.5f);
      renderRect = QRectF(topLeft, renderSize);
    } else {  // Qt::IgnoreAspectRatio:
      renderRect = QRectF(QPointF(), QSizeF(pixmapSize));
    }
  }
  QPixmap pixmap(pixmapSize);
  QPainter painter;
  pixmap.fill(bgColor);
  painter.begin(&pixmap);
  svgRenderer.render(&painter, renderRect);
  painter.end();
  pixmap.setDevicePixelRatio(devPixRatio);
  return pixmap;
}

//-----------------------------------------------------------------------------

int getDevPixRatio() {
  static int devPixRatio = QApplication::desktop()->devicePixelRatio();
  return devPixRatio;
}

//-----------------------------------------------------------------------------

QPixmap pixmapOpacity(QPixmap &pixmap, double opacity) {
  // Get device ratio, scale pixmap correctly for hdpi
  qreal pixelRatio = getDevPixRatio();
  QSize pixmapSize(pixmap.width() * pixelRatio, pixmap.height() * pixelRatio);

  QPixmap opacityPixmap(pixmapSize);
  opacityPixmap.setDevicePixelRatio(pixelRatio);
  opacityPixmap.fill(Qt::transparent);

  QPixmap normalPixmap = pixmap.scaled(pixmapSize, Qt::KeepAspectRatio);
  normalPixmap.setDevicePixelRatio(pixelRatio);

  QPainter p;
  p.begin(&opacityPixmap);
  p.setBackgroundMode(Qt::TransparentMode);
  p.setBackground(QBrush(Qt::transparent));
  p.eraseRect(normalPixmap.rect());
  p.setOpacity(opacity);
  p.drawPixmap(0, 0, normalPixmap);
  p.end();

  return opacityPixmap;
}

//-----------------------------------------------------------------------------

QString getIconThemePath(const QString &fileSVGPath) {
  QString theme;
  if (Preferences::instance()->getIconTheme())
    theme = ":icons/dark/";
  else
    theme = ":icons/light/";
  // qDebug() << "What is the theme name?" << theme;
  return theme + fileSVGPath;
}

//-----------------------------------------------------------------------------

QIcon createQIcon(const char *iconSVGName, bool useFullOpacity) {
  // Normal, full opacity icon
  QIcon iconNormal = QIcon::fromTheme(iconSVGName);

  // Get size of theme icon, we just grab the largest available
  QSize max(0, 0);
  for (QList<QSize> sizes = iconNormal.availableSizes(); !sizes.isEmpty();
       sizes.removeFirst())
    if (sizes.first().width() > max.width()) max = sizes.first();

  // Convert to pixmap with icon size, this is enough to build the icon, we can
  // check for other states conditionally.
  QPixmap normalPixmap = iconNormal.pixmap(max);
  QIcon icon;

  // Check for other states
  if (!iconNormal.isNull()) {
    QIcon iconOn   = QIcon::fromTheme(QString(iconSVGName) + "_on");
    QIcon iconOver = QIcon::fromTheme(QString(iconSVGName) + "_over");

    // The pixmap shown when the icon is idle
    QPixmap inactivePixmap = pixmapOpacity(iconNormal.pixmap(max), 0.8);
    // The pixmap shown when the icon is on/clicked/active
    QPixmap onPixmap       = iconOn.pixmap(max);
    if (!iconOver.isNull()) {
      QPixmap overPixmap = iconOver.pixmap(max);
      icon.addPixmap(overPixmap, QIcon::Active);
    } else {
      icon.addPixmap(normalPixmap, QIcon::Active);
    }

    // The pixmap shown when the icon is disabled
    QPixmap disabledPixmap = pixmapOpacity(normalPixmap, 0.2);
    icon.addPixmap(disabledPixmap, QIcon::Disabled);

    // Check if inactive icon should use full opacity
    if (useFullOpacity)
      icon.addPixmap(normalPixmap, QIcon::Normal, QIcon::Off);
    else
      icon.addPixmap(inactivePixmap, QIcon::Normal, QIcon::Off);

    if (!iconOn.isNull())
      icon.addPixmap(onPixmap, QIcon::Normal, QIcon::On);
    else
      icon.addPixmap(normalPixmap, QIcon::Normal, QIcon::On);
    
  }

  return icon;
}

//-----------------------------------------------------------------------------

QIcon createQIconPNG(const char *iconPNGName) {
  QString normal = QString(":Resources/") + iconPNGName + ".png";
  QString click  = QString(":Resources/") + iconPNGName + "_click.png";
  QString over   = QString(":Resources/") + iconPNGName + "_over.png";

  QIcon icon;
  icon.addFile(normal, QSize(), QIcon::Normal, QIcon::Off);
  icon.addFile(click, QSize(), QIcon::Normal, QIcon::On);
  icon.addFile(over, QSize(), QIcon::Active);

  return icon;
}

//-----------------------------------------------------------------------------

QIcon createQIconOnOff(const char *iconSVGName, bool withOver) {
  QString on   = QString(":Resources/") + iconSVGName + "_on.svg";
  QString off  = QString(":Resources/") + iconSVGName + "_off.svg";
  QString over = QString(":Resources/") + iconSVGName + "_over.svg";

  QIcon icon;
  icon.addFile(off, QSize(), QIcon::Normal, QIcon::Off);
  icon.addFile(on, QSize(), QIcon::Normal, QIcon::On);
  if (withOver)
    icon.addFile(over, QSize(), QIcon::Active);
  else
    icon.addFile(on, QSize(), QIcon::Active);
  return icon;
}

//-----------------------------------------------------------------------------

QIcon createQIconOnOffPNG(const char *iconPNGName, bool withOver) {
  QString on   = QString(":Resources/") + iconPNGName + "_on.png";
  QString off  = QString(":Resources/") + iconPNGName + "_off.png";
  QString over = QString(":Resources/") + iconPNGName + "_over.png";

  QIcon icon;
  icon.addFile(off, QSize(), QIcon::Normal, QIcon::Off);
  icon.addFile(on, QSize(), QIcon::Normal, QIcon::On);
  if (withOver)
    icon.addFile(over, QSize(), QIcon::Active);
  else
    icon.addFile(on, QSize(), QIcon::Active);

  return icon;
}

//-----------------------------------------------------------------------------

QString toQString(const TFilePath &path) {
  return QString::fromStdWString(path.getWideString());
}

//-----------------------------------------------------------------------------

bool isSpaceString(const QString &str) {
  int i;
  QString space(" ");
  for (i = 0; i < str.size(); i++)
    if (str.at(i) != space.at(0)) return false;
  return true;
}

//-----------------------------------------------------------------------------

bool isValidFileName(const QString &fileName) {
  if (fileName.isEmpty() || fileName.contains(":") || fileName.contains("\\") ||
      fileName.contains("/") || fileName.contains(">") ||
      fileName.contains("<") || fileName.contains("*") ||
      fileName.contains("|") || fileName.contains("\"") ||
      fileName.contains("?") || fileName.trimmed().isEmpty())
    return false;
  return true;
}

//-----------------------------------------------------------------------------

bool isValidFileName_message(const QString &fileName) {
  return isValidFileName(fileName)
             ? true
             : (DVGui::error(
                    QObject::tr("The file name cannot be empty or contain any "
                                "of the following "
                                "characters: (new line) \\ / : * ? \" |")),
                false);
}

//-----------------------------------------------------------------------------

bool isReservedFileName(const QString &fileName) {
#ifdef _WIN32
  std::vector<QString> invalidNames{
      "AUX",  "CON",  "NUL",  "PRN",  "COM1", "COM2", "COM3", "COM4",
      "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3",
      "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};

  if (std::find(invalidNames.begin(), invalidNames.end(), fileName) !=
      invalidNames.end())
    return true;
#endif

  return false;
}

//-----------------------------------------------------------------------------

bool isReservedFileName_message(const QString &fileName) {
  return isReservedFileName(fileName)
             ? (DVGui::error(QObject::tr(
                    "That is a reserved file name and cannot be used.")),
                true)
             : false;
}

//-----------------------------------------------------------------------------

QString elideText(const QString &srcText, const QFont &font, int width) {
  QFontMetrics metrix(font);
  int srcWidth = metrix.width(srcText);
  if (srcWidth < width) return srcText;
  int tilde = metrix.width("~");
  int block = (width - tilde) / 2;
  QString text("");
  int i;
  for (i = 0; i < srcText.size(); i++) {
    text += srcText.at(i);
    if (metrix.width(text) > block) break;
  }
  text[i] = '~';
  QString endText("");
  for (i = srcText.size() - 1; i >= 0; i--) {
    endText.push_front(srcText.at(i));
    if (metrix.width(endText) > block) break;
  }
  endText.remove(0, 1);
  text += endText;
  return text;
}

//-----------------------------------------------------------------------------

QString elideText(const QString &srcText, const QFontMetrics &fm, int width,
                  const QString &elideSymbol) {
  QString text(srcText);

  for (int i = text.size(); i > 1 && fm.width(text) > width;)
    text = srcText.left(--i).append(elideSymbol);

  return text;
}

//-----------------------------------------------------------------------------

QUrl pathToUrl(const TFilePath &path) {
  return QUrl::fromLocalFile(QString::fromStdWString(path.getWideString()));
}

//-----------------------------------------------------------------------------

bool isResource(const QString &path) {
  const TFilePath fp(path.toStdWString());
  TFileType::Type type = TFileType::getInfo(fp);

  return (TFileType::isViewable(type) || type & TFileType::MESH_IMAGE ||
          type == TFileType::AUDIO_LEVEL || type == TFileType::TABSCENE ||
          type == TFileType::TOONZSCENE || fp.getType() == "tpl");
}

//-----------------------------------------------------------------------------

bool isResource(const QUrl &url) { return isResource(url.toLocalFile()); }

//-----------------------------------------------------------------------------

bool isResourceOrFolder(const QUrl &url) {
  struct locals {
    static inline bool isDir(const QString &path) {
      return QFileInfo(path).isDir();
    }
  };  // locals

  const QString &path = url.toLocalFile();
  return (isResource(path) || locals::isDir(path));
}

//-----------------------------------------------------------------------------

bool acceptResourceDrop(const QList<QUrl> &urls) {
  int count = 0;
  for (const QUrl &url : urls) {
    if (isResource(url))
      ++count;
    else
      return false;
  }

  return (count > 0);
}

//-----------------------------------------------------------------------------

bool acceptResourceOrFolderDrop(const QList<QUrl> &urls) {
  int count = 0;
  for (const QUrl &url : urls) {
    if (isResourceOrFolder(url))
      ++count;
    else
      return false;
  }

  return (count > 0);
}

//-----------------------------------------------------------------------------

QPainterPath strokeToPainterPath(TStroke *stroke) {
  QPainterPath path;
  int i, chunkSize = stroke->getChunkCount();
  for (i = 0; i < chunkSize; i++) {
    const TThickQuadratic *q = stroke->getChunk(i);
    if (i == 0) path.moveTo(toQPointF(q->getThickP0()));
    path.quadTo(toQPointF(q->getThickP1()), toQPointF(q->getThickP2()));
  }
  return path;
}

//=============================================================================
// TabBarContainter
//-----------------------------------------------------------------------------

TabBarContainter::TabBarContainter(QWidget *parent) : QFrame(parent) {
  setObjectName("TabBarContainer");
  setFrameStyle(QFrame::StyledPanel);
}

//-----------------------------------------------------------------------------

void TabBarContainter::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.setPen(getBottomAboveLineColor());
  p.drawLine(0, height() - 2, width(), height() - 2);
  p.setPen(getBottomBelowLineColor());
  p.drawLine(0, height() - 1, width(), height() - 1);
}

//=============================================================================
// ToolBarContainer
//-----------------------------------------------------------------------------

ToolBarContainer::ToolBarContainer(QWidget *parent) : QFrame(parent) {
  setObjectName("ToolBarContainer");
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

//-----------------------------------------------------------------------------

void ToolBarContainer::paintEvent(QPaintEvent *event) {
  QPainter p(this);
}

//=============================================================================

QString operator+(const QString &a, const TFilePath &fp) {
  return a + QString::fromStdWString(fp.getWideString());
}
