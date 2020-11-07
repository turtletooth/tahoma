#pragma once

#ifndef MOTIONPATHPANEL_H
#define MOTIONPATHPANEL_H

#include "toonz/tstageobjectspline.h"
#include "toonzqt/intfield.h"

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QMouseEvent>
#include <QImage>

class TPanelTitleBarButton;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QFrame;
class QToolBar;
class QSlider;
class QComboBox;
class TThickPoint;
class GraphWidget;

//=============================================================================
// ClickablePathLabel
//-----------------------------------------------------------------------------

class ClickablePathLabel : public QLabel {
  Q_OBJECT

protected:
  void mouseReleaseEvent(QMouseEvent*) override;
  void enterEvent(QEvent*) override;
  void leaveEvent(QEvent*) override;

public:
  ClickablePathLabel(const QString& text, QWidget* parent = nullptr,
                     Qt::WindowFlags f = Qt::WindowFlags());
  ~ClickablePathLabel();
  void setSelected();
  void clearSelected();

signals:
  void onMouseRelease(QMouseEvent* event);
};

//=============================================================================
// MotionPathPanel
//-----------------------------------------------------------------------------

class MotionPathPanel final : public QWidget {
  Q_OBJECT

  Q_PROPERTY(QColor SelectedColor READ getSelectedColor WRITE setSelectedColor
          DESIGNABLE true)
      QColor m_selectedColor;
  QColor getSelectedColor() const { return m_selectedColor; }
  void setSelectedColor(const QColor& color) { m_selectedColor = color; }

  Q_PROPERTY(
      QColor HoverColor READ getHoverColor WRITE setHoverColor DESIGNABLE true)
      QColor m_hoverColor;
  QColor getHoverColor() const { return m_hoverColor; }
  void setHoverColor(const QColor& color) { m_hoverColor = color; }

  QHBoxLayout* m_toolLayout;
  QHBoxLayout* m_controlsLayout;
  QGridLayout* m_pathsLayout;
  QVBoxLayout* m_outsideLayout;
  QVBoxLayout* m_insideLayout;
  QFrame* m_mainControlsPage;
  QToolBar* m_toolbar;

  // std::vector<MotionPathControl*> m_motionPathControls;
  std::vector<TStageObjectSpline*> m_splines;
  TStageObjectSpline* m_currentSpline;
  GraphWidget* m_graphArea;
  std::vector<ClickablePathLabel*> m_pathLabels;

public:
  MotionPathPanel(QWidget* parent = 0);
  ~MotionPathPanel();

  void createControl(TStageObjectSpline* spline, int number);
  void highlightActiveSpline();

protected:
  void fillCombo(QComboBox* combo, TStageObjectSpline* spline);
  void clearPathsLayout();
  void newPath();

protected slots:
  void refreshPaths();

  // public slots:
};

#endif  // MOTIONPATHPANEL_H
