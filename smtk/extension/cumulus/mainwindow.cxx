#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "cJSON.h"
#include "job.h"
#include "jobtablemodel.h"
#include "cumulusproxy.h"

#include <QtCore/QDebug>
#include <QtGui/QDesktopWidget>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkCookie>
#include <QtCore/QVariant>
#include <QtCore/QList>
#include <QtCore/QTimer>

namespace cumulus {

MainWindow::MainWindow()
  : m_ui(new Ui::MainWindow),
    m_loginDialog(this),
    m_jobTableModel(new JobTableModel(this)),
    m_cumulusProxy(new CumulusProxy(this)),
    m_timer(NULL)
{
  m_ui->setupUi(this);

  QRect screenGeometry = QApplication::desktop()->screenGeometry();
  int x = (screenGeometry.width()-this->width()) / 2;
  int y = (screenGeometry.height()-this->height()) / 2;
  this->move(x, y);

  this->createJobTable();
  this->createMainMenu();

  connect(&m_loginDialog, SIGNAL(entered(QString, QString)),
            this->m_cumulusProxy, SLOT(authenticateNewt(QString, QString)));
  connect(this->m_cumulusProxy, SIGNAL(authenticationFinished()),
          this, SLOT(startJobFetchLoop()));
  connect(this->m_cumulusProxy, SIGNAL(jobsUpdated(QList<Job>)),
          this->m_jobTableModel, SLOT(jobsUpdated(QList<Job>)));

  if (this->m_cumulusProxy->isAuthenticated()) {
    m_loginDialog.show();
  }
}

MainWindow::~MainWindow()
{
  delete m_ui;
}

void MainWindow::girderUrl(const QString &url)
{
  this->m_cumulusProxy->girderUrl(url);
}

void MainWindow::createJobTable()
{
  m_ui->jobTableWidget->setModel(m_jobTableModel);
}

void MainWindow::createMainMenu()
{
  connect(m_ui->actionQuit, SIGNAL(triggered()),
          qApp, SLOT(quit()));
}

void MainWindow::startJobFetchLoop()
{
  this->m_cumulusProxy->fetchJobs();
  this->m_timer = new QTimer(this);
  connect(m_timer, SIGNAL(timeout()), this->m_cumulusProxy, SLOT(fetchJobs()));
  this->m_timer->start(10000);
}

void MainWindow::closeEvent(QCloseEvent *theEvent)
{
  if (m_timer) {
    this->m_timer->stop();
  }
  qApp->quit();
}

} // end namespace
